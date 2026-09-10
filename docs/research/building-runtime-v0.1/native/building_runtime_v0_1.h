#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace truthraw::building_runtime::v0_1 {

constexpr std::size_t kMaxRooms = 12;
constexpr std::size_t kMaxDependencies = 4;
constexpr std::size_t kMaxEvents = 32;
constexpr std::uint64_t kMiB = 1024ULL * 1024ULL;

enum class Status : std::uint8_t {
    Ok = 0,
    InvalidInput,
    InvalidGraph,
    DependencyBlocked,
    MissingScientificState,
    EvidenceInvariantViolation,
    ScientificMasterAlreadyModified,
    CapacityExceeded,
    InvalidCorridor,
    BudgetTooSmall
};

enum class RoomId : std::uint8_t {
    Archivist = 0,
    MeasurementLab = 1,
    Architect = 2,
    Restorer = 3,
    SceneRegistry = 4,
    Surveyor = 5,
    ManifoldConditioning = 6,
    LightingStudioCicm = 7,
    RoomCapsule = 8,
    Colorist = 9,
    Finisher = 10,
    Exporter = 11,
    Count = 12
};

enum class Domain : std::uint8_t {
    Scientific = 0,
    Counterfactual = 1,
    Appearance = 2,
    Projection = 3
};

enum class TruthFloor : std::uint8_t {
    Foundation = 0,
    Measurement = 1,
    Reconstruction = 2,
    Scene = 3,
    Counterfactual = 4,
    Appearance = 5,
    Projection = 6
};

enum class RoomStatus : std::uint8_t {
    Available = 0,
    ResearchOnly,
    BlockedMissingEvidence,
    IdentityFallback,
    Rejected
};

enum class ClaimStatus : std::uint8_t {
    Open = 0,
    Candidate,
    Rejected,
    Promoted
};

enum class Reason : std::uint8_t {
    None = 0,
    NotRequested,
    DependencyNotSatisfied,
    RequiredSigmaUnknown,
    CriticalScientificStateInvalid,
    ResearchOnlyByContract,
    ExplicitlyRejected,
    IdentityByContract,
    RequestedAndAdmissible
};

enum class ThermalState : std::uint8_t {
    Nominal = 0,
    Moderate,
    Severe,
    Critical
};

enum class ResourceTier : std::uint8_t {
    Low = 0,
    Mid,
    High
};

enum class ComputeBackend : std::uint8_t {
    CpuBaseline = 0,
    OptionalVulkan
};

struct ScientificState {
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
    bool scientificMasterModified = false;
    bool sigmaKnown = false;
    double sigma = 0.0;
    double physicalCaptureEv = 0.0;
    bool bestConditioningEvKnown = false;
    double bestConditioningEv = 0.0;
    ClaimStatus claimStatus = ClaimStatus::Open;
};

struct AppearanceState {
    bool appearanceEvKnown = false;
    double appearanceEv = 0.0;
};

struct CaptureProvenance {
    std::uint64_t immutableEvidenceFingerprint = 0;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
};

struct RoomDescriptor {
    RoomId id = RoomId::Archivist;
    Domain domain = Domain::Scientific;
    TruthFloor floor = TruthFloor::Foundation;
    RoomStatus declaredStatus = RoomStatus::ResearchOnly;
    std::array<RoomId, kMaxDependencies> dependencies{};
    std::uint8_t dependencyCount = 0;
    bool heavy = false;
    bool requiresKnownSigma = false;
    bool mayReadAppearance = false;
    bool mayWriteAppearance = false;
    bool mayReadCounterfactual = false;
    bool mayWriteCounterfactual = false;
    bool mayMutateInputScientificMaster = false;
    bool mayIncreaseIndependentEvidence = false;
};

struct ActivationRequest {
    std::array<bool, kMaxRooms> requested{};
};

struct Decision {
    RoomId room = RoomId::Archivist;
    RoomStatus status = RoomStatus::ResearchOnly;
    Reason reason = Reason::None;
    bool execute = false;
};

struct DecisionEvent {
    RoomId room = RoomId::Archivist;
    RoomStatus status = RoomStatus::ResearchOnly;
    Reason reason = Reason::None;
};

struct RuntimeResult {
    Status status = Status::InvalidInput;
    std::array<Decision, kMaxRooms> decisions{};
    std::size_t decisionCount = 0;
    std::array<DecisionEvent, kMaxEvents> ledger{};
    std::size_t ledgerCount = 0;
    std::uint32_t physicalFrameCount = 0;
    std::uint32_t independentEvidenceCount = 0;
    bool scientificMasterModified = false;
    double physicalCaptureEv = 0.0;
    bool bestConditioningEvKnown = false;
    double bestConditioningEv = 0.0;
    bool appearanceEvKnown = false;
    double appearanceEv = 0.0;
    ClaimStatus claimStatus = ClaimStatus::Open;
    std::uint64_t provenanceFingerprint = 0;
};

// Corridor token contains identity/authority only. Pixel payload stays behind an external handle.
struct CorridorToken {
    std::uint64_t artifactHandle = 0;
    std::uint64_t provenanceFingerprint = 0;
    TruthFloor floor = TruthFloor::Foundation;
    ClaimStatus claimStatus = ClaimStatus::Open;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
    bool scientificMasterModified = false;
};

struct DeviceEnvelope {
    std::uint64_t appMemoryClassMiB = 0;
    std::uint64_t explicitMaxWorkingSetBytes = 0;
    std::uint64_t currentlyAvailableBytes = 0;
    std::uint32_t cpuThreadBudget = 1;
    bool lowRamDevice = false;
    bool vulkanAvailable = false;
    bool foreground = true;
    ThermalState thermalState = ThermalState::Nominal;
};

struct ResourcePolicy {
    bool valid = false;
    ResourceTier tier = ResourceTier::Low;
    ComputeBackend correctnessBaseline = ComputeBackend::CpuBaseline;
    ComputeBackend preferredBackend = ComputeBackend::CpuBaseline;
    std::uint64_t totalWorkingSetBudgetBytes = 0;
    std::uint64_t perHeavyRoomBudgetBytes = 0;
    std::uint32_t maxConcurrentHeavyRooms = 1;
    std::uint32_t tileSize = 128;
    std::uint32_t cpuThreadsPerHeavyRoom = 1;
    bool speculativePrefetch = false;
    bool retainRebuildableCaches = false;
};

struct ExecutionPlacement {
    RoomId room = RoomId::Archivist;
    std::uint8_t wave = 0;
    std::uint8_t lane = 0;
    bool heavyLease = false;
    ComputeBackend backend = ComputeBackend::CpuBaseline;
    std::uint64_t memoryLeaseBytes = 0;
};

struct ExecutionPlan {
    Status status = Status::InvalidInput;
    std::array<ExecutionPlacement, kMaxRooms> placements{};
    std::size_t placementCount = 0;
    std::uint8_t waveCount = 0;
    ResourcePolicy resources{};
};

std::array<RoomDescriptor, static_cast<std::size_t>(RoomId::Count)> default_room_graph() noexcept;
Status validate_graph(const std::array<RoomDescriptor, static_cast<std::size_t>(RoomId::Count)>& graph) noexcept;
Status evaluate(const ScientificState& scientific,
                const AppearanceState& appearance,
                const CaptureProvenance& provenance,
                const ActivationRequest& request,
                const std::array<RoomDescriptor, static_cast<std::size_t>(RoomId::Count)>& graph,
                RuntimeResult& out) noexcept;
Status validate_corridor_transition(const RoomDescriptor& from,
                                    const RoomDescriptor& to,
                                    const CorridorToken& token) noexcept;
Status derive_resource_policy(const DeviceEnvelope& device, ResourcePolicy& out) noexcept;
Status plan_execution(const RuntimeResult& runtime,
                      const std::array<RoomDescriptor, static_cast<std::size_t>(RoomId::Count)>& graph,
                      const ResourcePolicy& resources,
                      ExecutionPlan& out) noexcept;

const char* status_name(Status s) noexcept;
const char* room_status_name(RoomStatus s) noexcept;
const char* reason_name(Reason r) noexcept;
const char* resource_tier_name(ResourceTier t) noexcept;

} // namespace truthraw::building_runtime::v0_1
