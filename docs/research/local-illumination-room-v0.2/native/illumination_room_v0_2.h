#pragma once

#include <cstdint>
#include <span>
#include <string>

#include "cicm_v1.h"
#include "room_abi_v0_1.h"
#include "room_capsule_v0_1.h"
#include "technical_backplane_v0_1.h"

namespace truthraw::illumination_room::v0_2 {

using building_runtime::v0_1::ResourcePolicy;
using room_abi::v0_1::RoomCapsuleLeaseSet;
using room_abi::v0_1::RoomExecutionContext;
using room_capsule::v0_1::BoundaryIlluminationEnvelope;
using room_capsule::v0_1::LightState;
using room_capsule::v0_1::RelativeLightResult;
using room_capsule::v0_1::RoomBounds;
using room_capsule::v0_1::RoomCapsulePlan;
using room_capsule::v0_1::RoomSampleRuntime;

// v0.2 remains a counterfactual/appearance study room. It does not promote
// geometry, light states, or day/night labels to source evidence.
enum class Status : std::uint8_t {
    Ok = 0,
    InvalidInput,
    InvalidAuthority,
    BackplaneInvalid,
    BudgetTooSmall,
    NoAdmissiblePlan,
    LeaseFailure,
    UpstreamFailure,
    OutsideRoom,
    PhysicalRelightBlocked
};

enum class ScenarioKind : std::uint8_t {
    RelativeDaylightStudy = 0,
    RelativeNightStudy = 1,
    CustomRelative = 2
};

// ISO is deliberately absent from the TruthRaw scene/room coordinate system.
// Capture ISO remains immutable provenance outside this room. CICM may carry a
// nominal ISO only inside its separately calibrated sensor-forward model; that
// sensor-mode property never becomes a Scene Master, TruthRange or zero-line
// coordinate and is not an input to relative illumination/EV math.
struct IsoBoundary {
    bool sceneIsoAxisPresent = false;
    bool relativeScenarioHasIsoParameter = false;
    bool relativeEvUsesIso = false;
    bool calibratedSensorForwardMayCarryNominalIso = true;
    bool calibratedSensorForwardIsoPromotesToScene = false;
};

constexpr IsoBoundary iso_boundary() noexcept { return {}; }

struct AdaptivePlanRequest {
    std::uint32_t sourceWidth = 0;
    std::uint32_t sourceHeight = 0;
    RoomBounds room{};
    std::uint32_t vectorBoundaryVertexCount = 0;
    ResourcePolicy resources{};
};

struct AdaptivePlan {
    bool valid = false;
    std::uint64_t roomBudgetBytes = 0;
    RoomCapsulePlan capsule{};
};

// The neutral scale is the CICM relative-world axis used for EV comparison.
// Boundary/light descriptors remain local appearance modulation. Neither is a
// calibrated sun/moon/spectral/BRDF claim. ISO is intentionally not a member:
// the new scene has no ISO axis.
struct RelativeScenario {
    ScenarioKind kind = ScenarioKind::CustomRelative;
    double neutralIlluminationScale = 1.0;
    BoundaryIlluminationEnvelope boundary{};
};

Status plan_from_runtime(const AdaptivePlanRequest& request, AdaptivePlan& out) noexcept;

// Convert the immutable Technical Backplane identities into the exact CICM
// scene-binding form. This copies identities only; no pixel payload is carried.
Status bind_scene_from_backplane(const technical_backplane::v0_1::State& backplane,
                                 counterfactual::v1::SceneBinding& out) noexcept;

// Relative day/night/custom world comparison. The result deliberately has no
// physical SNR and has no ISO dependency. A separately calibrated CICM sensor
// forward path may model a nominal sensor-mode ISO, but that is outside this
// scene/room contract.
Status simulate_relative_scenario(double nonnegativeSceneSignal,
                                  double shutterScale,
                                  const std::string& worldId,
                                  const RelativeScenario& scenario,
                                  const counterfactual::v1::SceneBinding& binding,
                                  counterfactual::v1::RelativeWorldPrediction& out) noexcept;

// Reuse the proven v0.1 local geometry/normal/visibility solver. Plan/resource
// tier does not enter this math, so low/high phones cannot change scene meaning.
Status evaluate_relative_sample(const RoomSampleRuntime& sample,
                                const RelativeScenario& scenario,
                                std::span<const LightState> lights,
                                RelativeLightResult& out) noexcept;

// Runtime/Room-ABI-owned leases. The room never allocates a hidden full frame.
Status acquire_runtime_leases(const RoomExecutionContext& context,
                              const AdaptivePlan& plan,
                              RoomCapsuleLeaseSet& out) noexcept;
void release_runtime_leases(const RoomExecutionContext& context,
                            RoomCapsuleLeaseSet& leases) noexcept;

const char* status_name(Status status) noexcept;
const char* scenario_kind_name(ScenarioKind kind) noexcept;

} // namespace truthraw::illumination_room::v0_2
