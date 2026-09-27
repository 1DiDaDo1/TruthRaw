#include "professional_raw_gatehouse_runtime_v0_1.h"

#include <cstdint>
#include <iostream>
#include <limits>

namespace gate = truthraw::professional_raw_gatehouse::v0_1;
namespace adapter = truthraw::professional_raw_decoder_adapter::v0_1;
namespace ingress = truthraw::professional_raw_ingress::v0_1;

#define CHECK_TRUE(expr) do { \
    if (!(expr)) { \
        std::cerr << "CHECK failed at line " << __LINE__ << ": " #expr "\n"; \
        return __LINE__; \
    } \
} while (false)

static adapter::AdapterOutput make_external_bayer(std::uint64_t resident,
                                                   std::uint64_t scratch) {
    ingress::DecoderResourceProfile resources{};
    resources.memoryMode = ingress::DecoderMemoryMode::FullFrameMaterialized;
    resources.residentUpperBoundBytes = resident;
    resources.scratchUpperBoundBytes = scratch;
    resources.requiresFullFrameMaterialization = true;
    resources.supportsRandomAccessTiles = false;

    ingress::DecoderProvenance provenance{};
    provenance.adapterId = "fixture.external";
    provenance.adapterVersion = "1";
    provenance.adapterBuildHash = "fixture-hash";
    provenance.cameraMake = "Fixture";
    provenance.cameraModel = "Professional RAW";
    provenance.codecVariant = "lossless-fixture";
    provenance.sourceBitDepth = 14;
    provenance.decoderBytePathVerified = true;

    adapter::AdapterEvidenceBinding evidence{};
    evidence.originalSourceSealed = true;
    evidence.sourceEvidenceBindingVerified = true;
    evidence.decodedOutputBoundToSource = true;
    evidence.sampleEquivalenceVerified = true;

    return adapter::describe_external_decoder(
        ingress::ContainerFamily::CanonCr3,
        ingress::MeasurementTopology::Bayer2x2,
        ingress::CompressionSemantics::LosslessVerified,
        ingress::DecodeCertification::AdapterCertified,
        resources,
        provenance,
        adapter::SampleSemantics::LosslessDecodedEquivalentSamples,
        evidence,
        true,
        true,
        true);
}

static adapter::AdapterOutput make_native_direct_fixture() {
    adapter::AdapterOutput out{};
    out.kind = adapter::AdapterKind::NativeTileDng;
    out.samples = adapter::SampleSemantics::StoredCodeValues;
    out.evidence.originalSourceSealed = true;
    out.evidence.sourceEvidenceBindingVerified = true;
    out.evidence.decodedOutputBoundToSource = true;
    out.evidence.sampleEquivalenceVerified = true;

    out.descriptor.container = ingress::ContainerFamily::Dng;
    out.descriptor.topology = ingress::MeasurementTopology::Bayer2x2;
    out.descriptor.compression = ingress::CompressionSemantics::Uncompressed;
    out.descriptor.decodeCertification = ingress::DecodeCertification::NativeCertified;
    out.descriptor.resources.memoryMode = ingress::DecoderMemoryMode::RandomAccessTile;
    out.descriptor.resources.residentUpperBoundBytes = 64U * 1024U;
    out.descriptor.resources.supportsRandomAccessTiles = true;
    out.descriptor.provenance.adapterId = "tile-native-dng";
    out.descriptor.provenance.adapterVersion = "0.1";
    out.descriptor.provenance.adapterBuildHash = "fixture";
    out.descriptor.provenance.decoderBytePathVerified = true;
    out.descriptor.singlePhysicalFrameVerified = true;
    out.descriptor.singleIndependentEvidenceVerified = true;
    out.descriptor.downstreamTopologyCertified = true;
    return out;
}

int main() {
    constexpr std::uint64_t MiB = 1024ULL * 1024ULL;

    gate::DeviceEnvelope low{};
    low.appMemoryClassBytes = 256ULL * MiB;
    low.currentlyAvailableBytes = 160ULL * MiB;
    low.cpuThreadBudget = 2;
    low.lowRamDevice = true;
    const auto lowPlan = gate::plan_resources(low);
    CHECK_TRUE(lowPlan.valid);
    CHECK_TRUE(lowPlan.tier == gate::ResourceTier::Low);
    CHECK_TRUE(lowPlan.budgetBytes == 32ULL * MiB);
    CHECK_TRUE(lowPlan.workerThreads == 1U);
    CHECK_TRUE(lowPlan.maxConcurrentCompatibleHeavyRooms == 1U);
    CHECK_TRUE(!lowPlan.retainRebuildableCache);
    CHECK_TRUE(!lowPlan.allowMainHouseHeavyOverlap);

    gate::DeviceEnvelope high{};
    high.appMemoryClassBytes = 4ULL * 1024ULL * MiB;
    high.currentlyAvailableBytes = 3ULL * 1024ULL * MiB;
    high.cpuThreadBudget = 12;
    high.foreground = true;
    high.thermal = gate::ThermalState::Normal;
    const auto highPlan = gate::plan_resources(high);
    CHECK_TRUE(highPlan.valid);
    CHECK_TRUE(highPlan.tier == gate::ResourceTier::High);
    CHECK_TRUE(highPlan.budgetBytes == 512ULL * MiB);
    CHECK_TRUE(highPlan.workerThreads == 8U);
    CHECK_TRUE(highPlan.maxConcurrentCompatibleHeavyRooms == 4U);
    CHECK_TRUE(highPlan.retainRebuildableCache);
    CHECK_TRUE(!highPlan.allowMainHouseHeavyOverlap);

    const auto external = make_external_bayer(416ULL * MiB, 32ULL * MiB);
    CHECK_TRUE(gate::route_adapter_output(external) == gate::EntryRoute::GatehouseRequired);

    const auto lowDecode = gate::admit_decoder(lowPlan, external.descriptor.resources);
    const auto highDecode = gate::admit_decoder(highPlan, external.descriptor.resources);
    CHECK_TRUE(!lowDecode.admitted);
    CHECK_TRUE(highDecode.admitted);
    CHECK_TRUE(highDecode.decoderPeakUpperBoundBytes == 448ULL * MiB);

    // Resource tier changes execution only, not adapter/evidence classification.
    const auto decision = adapter::classify_adapter_output(external);
    CHECK_TRUE(decision.evidenceClass == ingress::EvidenceClass::LosslessDecodedCertified);
    CHECK_TRUE(decision.admission == ingress::ScientificAdmission::SingleFrameDirectCfa);

    const auto nativeDirect = make_native_direct_fixture();
    CHECK_TRUE(gate::route_adapter_output(nativeDirect) == gate::EntryRoute::MainHouseDirect);

    gate::TransitHandoff handoff{};
    handoff.originalSourceStillSealed = true;
    handoff.sourceEvidenceBindingVerified = true;
    handoff.decodedRepresentationImmutable = true;
    handoff.decodedRepresentationPersistedOrExternallyOwned = true;
    handoff.decoderContextLive = true;
    handoff.fullFrameMaterializedDuringDecode = true;
    handoff.decodedRepresentationResidentBytes = 416ULL * MiB;
    handoff.physicalFrameCount = 1;
    handoff.independentEvidenceCount = 1;
    handoff.evidenceClass = ingress::EvidenceClass::LosslessDecodedCertified;
    handoff.topology = ingress::MeasurementTopology::Bayer2x2;

    CHECK_TRUE(gate::may_seal_external_handoff(external, highDecode, handoff));
    handoff.sealed = true;
    CHECK_TRUE(!gate::may_enter_main_house_after_detach(gate::GatehouseState::HandoffSealed, handoff));
    CHECK_TRUE(!gate::may_enter_main_house_after_detach(gate::GatehouseState::Detached, handoff));

    // Detach means the external decoder context is gone before Main House starts.
    handoff.decoderContextLive = false;
    CHECK_TRUE(gate::may_enter_main_house_after_detach(gate::GatehouseState::Detached, handoff));

    auto invalidHandoff = handoff;
    invalidHandoff.zeroLineCreatedInGatehouse = true;
    invalidHandoff.sealed = false;
    CHECK_TRUE(!gate::may_seal_external_handoff(external, highDecode, invalidHandoff));

    invalidHandoff = handoff;
    invalidHandoff.scientificMasterCreatedInGatehouse = true;
    CHECK_TRUE(!gate::may_enter_main_house_after_detach(gate::GatehouseState::Detached, invalidHandoff));

    invalidHandoff = handoff;
    invalidHandoff.independentEvidenceCount = 2;
    CHECK_TRUE(!gate::may_enter_main_house_after_detach(gate::GatehouseState::Detached, invalidHandoff));

    ingress::DecoderResourceProfile overflow{};
    overflow.memoryMode = ingress::DecoderMemoryMode::FullFrameMaterialized;
    overflow.residentUpperBoundBytes = std::numeric_limits<std::uint64_t>::max();
    overflow.scratchUpperBoundBytes = 1;
    overflow.requiresFullFrameMaterialization = true;
    CHECK_TRUE(!gate::admit_decoder(highPlan, overflow).admitted);

    gate::DeviceEnvelope thermal = high;
    thermal.thermal = gate::ThermalState::Severe;
    const auto thermalPlan = gate::plan_resources(thermal);
    CHECK_TRUE(thermalPlan.valid);
    CHECK_TRUE(thermalPlan.tier == gate::ResourceTier::Low);
    CHECK_TRUE(thermalPlan.workerThreads == 1U);
    CHECK_TRUE(!thermalPlan.retainRebuildableCache);

    CHECK_TRUE(gate::valid_state_transition(gate::GatehouseState::Idle, gate::GatehouseState::ProbeActive));
    CHECK_TRUE(gate::valid_state_transition(gate::GatehouseState::ProbeActive, gate::GatehouseState::DecodeActive));
    CHECK_TRUE(gate::valid_state_transition(gate::GatehouseState::DecodeActive, gate::GatehouseState::HandoffSealed));
    CHECK_TRUE(gate::valid_state_transition(gate::GatehouseState::HandoffSealed, gate::GatehouseState::Detached));
    CHECK_TRUE(!gate::valid_state_transition(gate::GatehouseState::Detached, gate::GatehouseState::DecodeActive));

    CHECK_TRUE(gate::room_is_heavy(gate::GatehouseRoom::DecodeChamber));
    CHECK_TRUE(gate::room_is_heavy(gate::GatehouseRoom::SampleAuditLab));
    CHECK_TRUE(!gate::room_is_heavy(gate::GatehouseRoom::FormatProbe));

    std::cout << "Professional RAW Gatehouse Runtime v0.1 PASS\n";
    std::cout << "low_budget_bytes=" << lowPlan.budgetBytes << "\n";
    std::cout << "high_budget_bytes=" << highPlan.budgetBytes << "\n";
    std::cout << "decoder_peak_bytes=" << highDecode.decoderPeakUpperBoundBytes << "\n";
    std::cout << "main_house_overlap=0\n";
    std::cout << "physical_frame_count=1\n";
    std::cout << "independent_evidence_count=1\n";
    return 0;
}
