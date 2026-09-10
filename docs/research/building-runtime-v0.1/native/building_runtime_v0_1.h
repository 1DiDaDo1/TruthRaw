#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace truthraw::building_runtime::v0_1 {

constexpr std::size_t kMaxRooms = 8;
constexpr std::size_t kMaxDependencies = 4;
constexpr std::size_t kMaxEvents = 16;

enum class Status : std::uint8_t {
    Ok = 0,
    InvalidInput,
    InvalidGraph,
    DependencyBlocked,
    MissingScientificState,
    EvidenceInvariantViolation,
    ScientificMasterAlreadyModified,
    CapacityExceeded
};

enum class RoomId : std::uint8_t {
    ManifoldConditioning = 0,
    CounterfactualIllumination = 1,
    RoomCapsule = 2,
    Uncertainty = 3,
    Appearance = 4,
    OutputProjection = 5,
    Count = 6
};

enum class Domain : std::uint8_t {
    Scientific = 0,
    Appearance = 1,
    Projection = 2
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
    RoomId id = RoomId::ManifoldConditioning;
    Domain domain = Domain::Scientific;
    RoomStatus declaredStatus = RoomStatus::ResearchOnly;
    std::array<RoomId, kMaxDependencies> dependencies{};
    std::uint8_t dependencyCount = 0;
    bool requiresKnownSigma = false;
    bool mayReadAppearance = false;
    bool mayWriteAppearance = false;
    bool mayModifyScientificMaster = false;
    bool mayIncreaseIndependentEvidence = false;
};

struct ActivationRequest {
    std::array<bool, kMaxRooms> requested{};
};

struct Decision {
    RoomId room = RoomId::ManifoldConditioning;
    RoomStatus status = RoomStatus::ResearchOnly;
    Reason reason = Reason::None;
    bool execute = false;
};

struct DecisionEvent {
    RoomId room = RoomId::ManifoldConditioning;
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

std::array<RoomDescriptor, static_cast<std::size_t>(RoomId::Count)> default_room_graph() noexcept;
Status validate_graph(const std::array<RoomDescriptor, static_cast<std::size_t>(RoomId::Count)>& graph) noexcept;
Status evaluate(const ScientificState& scientific,
                const AppearanceState& appearance,
                const CaptureProvenance& provenance,
                const ActivationRequest& request,
                const std::array<RoomDescriptor, static_cast<std::size_t>(RoomId::Count)>& graph,
                RuntimeResult& out) noexcept;

const char* status_name(Status s) noexcept;
const char* room_status_name(RoomStatus s) noexcept;
const char* reason_name(Reason r) noexcept;

} // namespace truthraw::building_runtime::v0_1
