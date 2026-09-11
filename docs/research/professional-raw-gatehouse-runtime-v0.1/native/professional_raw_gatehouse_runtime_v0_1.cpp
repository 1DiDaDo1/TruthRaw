#include "professional_raw_gatehouse_runtime_v0_1.h"

#include <algorithm>
#include <limits>

namespace truthraw::professional_raw_gatehouse::v0_1 {
namespace {

constexpr std::uint64_t kMinBudgetBytes = 4ULL * 1024ULL * 1024ULL;

[[nodiscard]] std::uint64_t safe_sum(
    std::uint64_t a,
    std::uint64_t b) noexcept {
    if (b > std::numeric_limits<std::uint64_t>::max() - a) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return a + b;
}

[[nodiscard]] ResourceTier choose_tier(const DeviceEnvelope& device) noexcept {
    if (device.lowRamDevice || !device.foreground ||
        device.thermal == ThermalState::Severe ||
        device.thermal == ThermalState::Critical ||
        device.cpuThreadBudget <= 2U) {
        return ResourceTier::Low;
    }
    if (device.cpuThreadBudget >= 8U &&
        device.appMemoryClassBytes >= 1024ULL * 1024ULL * 1024ULL) {
        return ResourceTier::High;
    }
    return ResourceTier::Mid;
}

} // namespace

GatehouseResourcePlan plan_resources(const DeviceEnvelope& device) noexcept {
    GatehouseResourcePlan out{};
    if (device.appMemoryClassBytes == 0 ||
        device.currentlyAvailableBytes == 0 ||
        device.cpuThreadBudget == 0) {
        return out;
    }

    std::uint64_t budget = std::min(
        device.appMemoryClassBytes / 8ULL,
        device.currentlyAvailableBytes / 2ULL);
    if (device.explicitWorkingSetCeilingBytes != 0) {
        budget = std::min(budget, device.explicitWorkingSetCeilingBytes);
    }
    if (budget < kMinBudgetBytes) {
        return out;
    }

    out.valid = true;
    out.tier = choose_tier(device);
    out.budgetBytes = budget;
    out.allowMainHouseHeavyOverlap = false; // v0.1 isolation invariant.

    switch (out.tier) {
        case ResourceTier::Low:
            out.workerThreads = 1;
            out.maxConcurrentCompatibleHeavyRooms = 1;
            out.ioChunkBytes = 64ULL * 1024ULL;
            out.retainRebuildableCache = false;
            break;
        case ResourceTier::Mid:
            out.workerThreads = std::min<std::uint32_t>(device.cpuThreadBudget, 4U);
            out.maxConcurrentCompatibleHeavyRooms = 2;
            out.ioChunkBytes = 256ULL * 1024ULL;
            out.retainRebuildableCache = device.foreground;
            break;
        case ResourceTier::High:
            out.workerThreads = std::min<std::uint32_t>(device.cpuThreadBudget, 8U);
            out.maxConcurrentCompatibleHeavyRooms = 4;
            out.ioChunkBytes = 1024ULL * 1024ULL;
            out.retainRebuildableCache = device.foreground &&
                device.thermal == ThermalState::Normal;
            break;
    }

    if (!device.foreground ||
        device.thermal == ThermalState::Severe ||
        device.thermal == ThermalState::Critical) {
        out.tier = ResourceTier::Low;
        out.workerThreads = 1;
        out.maxConcurrentCompatibleHeavyRooms = 1;
        out.ioChunkBytes = 64ULL * 1024ULL;
        out.retainRebuildableCache = false;
    }

    return out;
}

DecodeAdmission admit_decoder(
    const GatehouseResourcePlan& plan,
    const ingress::DecoderResourceProfile& decoder) noexcept {
    DecodeAdmission out{};
    out.gatehouseBudgetBytes = plan.budgetBytes;
    out.decoderResidentUpperBoundBytes = decoder.residentUpperBoundBytes;
    out.decoderScratchUpperBoundBytes = decoder.scratchUpperBoundBytes;
    out.decoderPeakUpperBoundBytes = safe_sum(
        decoder.residentUpperBoundBytes,
        decoder.scratchUpperBoundBytes);

    if (!plan.valid || !ingress::valid(decoder)) {
        return out;
    }
    if (out.decoderPeakUpperBoundBytes > plan.budgetBytes) {
        return out;
    }

    // No quality downgrade or authority downgrade is used to fit a decoder.
    // The decoder either fits the Gatehouse budget or it does not run.
    out.admitted = true;
    return out;
}

EntryRoute route_adapter_output(const adapter::AdapterOutput& output) noexcept {
    const auto decision = adapter::classify_adapter_output(output);
    if (decision.admission == ingress::ScientificAdmission::Blocked) {
        return EntryRoute::FailClosed;
    }
    if (decision.admission == ingress::ScientificAdmission::ResearchOnly ||
        decision.requiresDerivedOrCounterfactualBoundary) {
        return EntryRoute::ResearchOnly;
    }
    if (decision.admission != ingress::ScientificAdmission::SingleFrameDirectCfa ||
        !decision.mayEnterSingleFrameScientificMaster) {
        return EntryRoute::FailClosed;
    }

    if (output.kind == adapter::AdapterKind::NativeTileDng &&
        decision.evidenceClass == ingress::EvidenceClass::DirectNativeCertified) {
        return EntryRoute::MainHouseDirect;
    }
    if (output.kind == adapter::AdapterKind::ExternalDecoder &&
        decision.evidenceClass == ingress::EvidenceClass::LosslessDecodedCertified) {
        return EntryRoute::GatehouseRequired;
    }
    return EntryRoute::FailClosed;
}

bool valid_state_transition(GatehouseState from, GatehouseState to) noexcept {
    if (to == GatehouseState::Failed) {
        return from != GatehouseState::Detached;
    }
    switch (from) {
        case GatehouseState::Idle:
            return to == GatehouseState::ProbeActive;
        case GatehouseState::ProbeActive:
            return to == GatehouseState::DecodeActive ||
                to == GatehouseState::Detached;
        case GatehouseState::DecodeActive:
            return to == GatehouseState::HandoffSealed;
        case GatehouseState::HandoffSealed:
            return to == GatehouseState::Detached;
        case GatehouseState::Detached:
        case GatehouseState::Failed:
            return false;
    }
    return false;
}

bool may_seal_external_handoff(
    const adapter::AdapterOutput& output,
    const DecodeAdmission& decode,
    const TransitHandoff& proposed) noexcept {
    if (route_adapter_output(output) != EntryRoute::GatehouseRequired ||
        !decode.admitted) {
        return false;
    }
    if (!proposed.originalSourceStillSealed ||
        !proposed.sourceEvidenceBindingVerified ||
        !proposed.decodedRepresentationImmutable ||
        !proposed.decodedRepresentationPersistedOrExternallyOwned ||
        !proposed.decodedRepresentationIntegrityVerified) {
        return false;
    }
    if (proposed.zeroLineCreatedInGatehouse ||
        proposed.scientificMasterCreatedInGatehouse ||
        proposed.mutableDecoderStateSharedWithMainHouse) {
        return false;
    }
    if (proposed.physicalFrameCount != 1U ||
        proposed.independentEvidenceCount != 1U) {
        return false;
    }
    if (proposed.evidenceClass != ingress::EvidenceClass::LosslessDecodedCertified ||
        proposed.topology != ingress::MeasurementTopology::Bayer2x2) {
        return false;
    }
    return true;
}

bool may_enter_main_house_after_detach(
    GatehouseState state,
    const TransitHandoff& handoff) noexcept {
    return state == GatehouseState::Detached &&
        handoff.sealed &&
        handoff.originalSourceStillSealed &&
        handoff.sourceEvidenceBindingVerified &&
        handoff.decodedRepresentationImmutable &&
        handoff.decodedRepresentationPersistedOrExternallyOwned &&
        handoff.decodedRepresentationIntegrityVerified &&
        !handoff.decoderContextLive &&
        !handoff.mutableDecoderStateSharedWithMainHouse &&
        !handoff.zeroLineCreatedInGatehouse &&
        !handoff.scientificMasterCreatedInGatehouse &&
        handoff.physicalFrameCount == 1U &&
        handoff.independentEvidenceCount == 1U &&
        handoff.evidenceClass == ingress::EvidenceClass::LosslessDecodedCertified &&
        handoff.topology == ingress::MeasurementTopology::Bayer2x2;
}

bool room_is_heavy(GatehouseRoom room) noexcept {
    return room == GatehouseRoom::DecodeChamber ||
        room == GatehouseRoom::SampleAuditLab;
}

} // namespace truthraw::professional_raw_gatehouse::v0_1
