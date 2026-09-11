#pragma once

#include "professional_raw_decoder_adapter_v0_1.h"

#include <cstdint>

namespace truthraw::professional_raw_gatehouse::v0_1 {

namespace adapter = truthraw::professional_raw_decoder_adapter::v0_1;
namespace ingress = truthraw::professional_raw_ingress::v0_1;

enum class GatehouseRoom : std::uint8_t {
    SourceVestibule = 0,
    FormatProbe,
    CodecResolver,
    DecodeChamber,
    SampleAuditLab,
    TopologyLab,
    MetadataSemanticsLab,
    FrameEvidenceLab,
    ResourceQuarantine,
    ProvenanceBinder,
    AdmissionInspector,
    HandoffAirlock,
};

enum class ThermalState : std::uint8_t {
    Normal = 0,
    Elevated,
    Severe,
    Critical,
};

enum class ResourceTier : std::uint8_t {
    Low = 0,
    Mid,
    High,
};

enum class EntryRoute : std::uint8_t {
    FailClosed = 0,
    MainHouseDirect,
    GatehouseRequired,
    ResearchOnly,
};

enum class GatehouseState : std::uint8_t {
    Idle = 0,
    ProbeActive,
    DecodeActive,
    HandoffSealed,
    Detached,
    Failed,
};

struct DeviceEnvelope {
    std::uint64_t appMemoryClassBytes = 0;
    std::uint64_t currentlyAvailableBytes = 0;
    std::uint64_t explicitWorkingSetCeilingBytes = 0;
    std::uint32_t cpuThreadBudget = 1;
    bool lowRamDevice = false;
    bool foreground = true;
    ThermalState thermal = ThermalState::Normal;
};

struct GatehouseResourcePlan {
    bool valid = false;
    ResourceTier tier = ResourceTier::Low;
    std::uint64_t budgetBytes = 0;
    std::uint32_t workerThreads = 1;
    std::uint32_t maxConcurrentCompatibleHeavyRooms = 1;
    std::uint64_t ioChunkBytes = 64U * 1024U;
    bool retainRebuildableCache = false;
    bool allowMainHouseHeavyOverlap = false;
};

struct DecodeAdmission {
    bool admitted = false;
    std::uint64_t decoderResidentUpperBoundBytes = 0;
    std::uint64_t decoderScratchUpperBoundBytes = 0;
    std::uint64_t decoderPeakUpperBoundBytes = 0;
    std::uint64_t gatehouseBudgetBytes = 0;
};

struct TransitHandoff {
    bool sealed = false;
    bool originalSourceStillSealed = false;
    bool sourceEvidenceBindingVerified = false;
    bool decodedRepresentationImmutable = false;
    bool decodedRepresentationPersistedOrExternallyOwned = false;
    bool decodedRepresentationIntegrityVerified = false;
    bool decoderContextLive = false;
    bool mutableDecoderStateSharedWithMainHouse = false;
    bool zeroLineCreatedInGatehouse = false;
    bool scientificMasterCreatedInGatehouse = false;
    bool fullFrameMaterializedDuringDecode = false;
    std::uint64_t decodedRepresentationResidentBytes = 0;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
    ingress::EvidenceClass evidenceClass = ingress::EvidenceClass::FailClosedUnsupported;
    ingress::MeasurementTopology topology = ingress::MeasurementTopology::Unknown;
};

[[nodiscard]] GatehouseResourcePlan plan_resources(
    const DeviceEnvelope& device) noexcept;

[[nodiscard]] DecodeAdmission admit_decoder(
    const GatehouseResourcePlan& plan,
    const ingress::DecoderResourceProfile& decoder) noexcept;

[[nodiscard]] EntryRoute route_adapter_output(
    const adapter::AdapterOutput& output) noexcept;

[[nodiscard]] bool valid_state_transition(
    GatehouseState from,
    GatehouseState to) noexcept;

[[nodiscard]] bool may_seal_external_handoff(
    const adapter::AdapterOutput& output,
    const DecodeAdmission& decode,
    const TransitHandoff& proposed) noexcept;

[[nodiscard]] bool may_enter_main_house_after_detach(
    GatehouseState state,
    const TransitHandoff& handoff) noexcept;

[[nodiscard]] bool room_is_heavy(GatehouseRoom room) noexcept;

} // namespace truthraw::professional_raw_gatehouse::v0_1
