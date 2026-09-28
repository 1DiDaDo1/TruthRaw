#include "illumination_room_v0_2.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace truthraw::illumination_room::v0_2 {
namespace {

constexpr std::uint64_t kMiB = 1024ULL * 1024ULL;
constexpr std::uint64_t kMinimumRoomBudget = 2ULL * kMiB;
constexpr std::uint64_t kMetadataAllowance = 64ULL * 1024ULL;
constexpr std::uint64_t kTileBytesPerPixel = 32ULL;
constexpr std::uint64_t kVectorVertexBytes = 8ULL;
constexpr std::uint32_t kHalo = 4U;
constexpr std::array<std::uint32_t, 5> kGeometryScales{4U, 8U, 16U, 32U, 64U};

bool finite(double x) noexcept { return std::isfinite(x); }

bool valid_scenario_kind(ScenarioKind kind) noexcept {
    return static_cast<std::uint8_t>(kind) <= static_cast<std::uint8_t>(ScenarioKind::CustomRelative);
}

bool valid_tile(std::uint32_t tile) noexcept {
    return tile >= 64U && tile <= 512U && (tile & (tile - 1U)) == 0U;
}

bool safe_mul(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a != 0U && b > std::numeric_limits<std::uint64_t>::max() / a) return false;
    out = a * b;
    return true;
}

bool safe_add(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a > std::numeric_limits<std::uint64_t>::max() - b) return false;
    out = a + b;
    return true;
}

std::uint64_t tile_workspace(std::uint32_t tile) noexcept {
    const std::uint64_t side = static_cast<std::uint64_t>(tile) + 2ULL * kHalo;
    std::uint64_t pixels = 0U;
    std::uint64_t bytes = 0U;
    if (!safe_mul(side, side, pixels) || !safe_mul(pixels, kTileBytesPerPixel, bytes)) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return bytes;
}

std::string to_hex(const technical_backplane::v0_1::Hash256& hash) {
    static constexpr char lut[] = "0123456789abcdef";
    std::string out;
    out.resize(hash.size() * 2U);
    for (std::size_t i = 0; i < hash.size(); ++i) {
        const auto v = hash[i];
        out[2U * i] = lut[(v >> 4U) & 0x0fU];
        out[2U * i + 1U] = lut[v & 0x0fU];
    }
    return out;
}

Status map_capsule_status(room_capsule::v0_1::Status status) noexcept {
    switch (status) {
        case room_capsule::v0_1::Status::Ok: return Status::Ok;
        case room_capsule::v0_1::Status::InvalidInput: return Status::InvalidInput;
        case room_capsule::v0_1::Status::BudgetTooSmall: return Status::BudgetTooSmall;
        case room_capsule::v0_1::Status::NoAdmissiblePlan: return Status::NoAdmissiblePlan;
        case room_capsule::v0_1::Status::OutsideRoom: return Status::OutsideRoom;
        case room_capsule::v0_1::Status::PhysicalRelightBlocked: return Status::PhysicalRelightBlocked;
    }
    return Status::UpstreamFailure;
}

Status map_abi_status(room_abi::v0_1::Status status) noexcept {
    switch (status) {
        case room_abi::v0_1::Status::Ok: return Status::Ok;
        case room_abi::v0_1::Status::InvalidInput: return Status::InvalidInput;
        case room_abi::v0_1::Status::InvalidAuthority: return Status::InvalidAuthority;
        case room_abi::v0_1::Status::InvalidHost:
        case room_abi::v0_1::Status::InvalidAlignment:
        case room_abi::v0_1::Status::LeaseDenied:
        case room_abi::v0_1::Status::LeaseContractViolation:
            return Status::LeaseFailure;
        case room_abi::v0_1::Status::UpstreamFailure: return Status::UpstreamFailure;
    }
    return Status::UpstreamFailure;
}

} // namespace

Status plan_from_runtime(const AdaptivePlanRequest& request, AdaptivePlan& out) noexcept {
    out = {};
    if (request.sourceWidth == 0U || request.sourceHeight == 0U ||
        request.room.width == 0U || request.room.height == 0U ||
        request.room.x >= request.sourceWidth || request.room.y >= request.sourceHeight ||
        request.room.width > request.sourceWidth - request.room.x ||
        request.room.height > request.sourceHeight - request.room.y ||
        request.vectorBoundaryVertexCount < 3U ||
        !request.resources.valid || !valid_tile(request.resources.tileSize) ||
        request.resources.totalWorkingSetBudgetBytes == 0U ||
        request.resources.perHeavyRoomBudgetBytes == 0U) {
        return Status::InvalidInput;
    }

    const std::uint64_t roomBudget = std::min(request.resources.totalWorkingSetBudgetBytes,
                                              request.resources.perHeavyRoomBudgetBytes);
    if (roomBudget < kMinimumRoomBudget) return Status::BudgetTooSmall;

    const std::uint64_t tileBytes = tile_workspace(request.resources.tileSize);
    if (tileBytes == std::numeric_limits<std::uint64_t>::max()) return Status::InvalidInput;

    std::uint64_t boundaryBytes = 0U;
    if (!safe_mul(request.vectorBoundaryVertexCount, kVectorVertexBytes, boundaryBytes)) {
        return Status::InvalidInput;
    }

    for (const std::uint32_t scale : kGeometryScales) {
        const std::uint32_t gw = (request.room.width + scale - 1U) / scale;
        const std::uint32_t gh = (request.room.height + scale - 1U) / scale;
        std::uint64_t samples = 0U;
        std::uint64_t packed = 0U;
        if (!safe_mul(gw, gh, samples) ||
            !safe_mul(samples, sizeof(room_capsule::v0_1::PackedRoomSample), packed)) {
            continue;
        }

        std::uint64_t peak = 0U;
        if (!safe_add(packed, boundaryBytes, peak) ||
            !safe_add(peak, tileBytes, peak) ||
            !safe_add(peak, kMetadataAllowance, peak)) {
            continue;
        }
        if (peak > roomBudget) continue;

        RoomCapsulePlan plan{};
        plan.valid = true;
        plan.roomBounds = request.room;
        plan.baselineBackend = room_capsule::v0_1::ComputeBackend::CpuBaseline;
        plan.gpuAccelerationOptional =
            request.resources.preferredBackend == building_runtime::v0_1::ComputeBackend::OptionalVulkan;
        plan.geometryDownsample = scale;
        plan.geometryWidth = gw;
        plan.geometryHeight = gh;
        plan.tileSize = request.resources.tileSize;
        plan.tileHalo = kHalo;
        plan.packedGeometryBytes = packed;
        plan.vectorBoundaryBytes = boundaryBytes;
        plan.tileWorkspaceBytes = tileBytes;
        plan.estimatedPeakBytes = peak;
        plan.budgetBytes = roomBudget;
        plan.roomTileColumns = (request.room.width + request.resources.tileSize - 1U) / request.resources.tileSize;
        plan.roomTileRows = (request.room.height + request.resources.tileSize - 1U) / request.resources.tileSize;
        plan.roomTileCount = static_cast<std::uint64_t>(plan.roomTileColumns) * plan.roomTileRows;
        plan.ledger = {};

        out.valid = true;
        out.roomBudgetBytes = roomBudget;
        out.capsule = plan;
        return Status::Ok;
    }

    return Status::NoAdmissiblePlan;
}

Status bind_scene_from_backplane(const technical_backplane::v0_1::State& backplane,
                                 counterfactual::v1::SceneBinding& out) noexcept {
    out = {};
    if (technical_backplane::v0_1::validate(backplane) != technical_backplane::v0_1::Status::Ok) {
        return Status::BackplaneInvalid;
    }
    out.sourceEvidenceId = to_hex(backplane.sourceEvidenceHash);
    out.scientificMasterSha256 = to_hex(backplane.scientificMasterHash);
    out.zeroLineId = to_hex(backplane.zeroLineHash);
    out.sceneScaleId = to_hex(backplane.sceneScaleHash);
    if (counterfactual::v1::validate_scene_binding(out) != counterfactual::v1::Status::Ok) {
        out = {};
        return Status::BackplaneInvalid;
    }
    return Status::Ok;
}

Status simulate_relative_scenario(double nonnegativeSceneSignal,
                                  double shutterScale,
                                  const std::string& worldId,
                                  const RelativeScenario& scenario,
                                  const counterfactual::v1::SceneBinding& binding,
                                  counterfactual::v1::RelativeWorldPrediction& out) noexcept {
    out = {};
    if (!valid_scenario_kind(scenario.kind) || !finite(scenario.neutralIlluminationScale) ||
        !(scenario.neutralIlluminationScale > 0.0) || !finite(shutterScale) ||
        !(shutterScale > 0.0) || worldId.empty()) {
        return Status::InvalidInput;
    }

    counterfactual::v1::CounterfactualWorldSpec world{};
    world.worldId = worldId;
    world.semantics = counterfactual::v1::WorldSemantics::RelativeRadianceScaleOnly;
    world.illuminationScale = scenario.neutralIlluminationScale;
    world.sceneBinding = binding;

    counterfactual::v1::RelativeCaptureSpec capture{};
    capture.shutterScale = shutterScale;
    const auto s = counterfactual::v1::simulate_relative_world(nonnegativeSceneSignal, world, capture, out);
    if (s != counterfactual::v1::Status::Ok) return Status::UpstreamFailure;
    if (!out.valid || out.physicalSnrAvailable || out.ledger.physicalFrameCount != 1U ||
        out.ledger.independentEvidenceCount != 1U || out.ledger.counterfactualObservationsAreEvidence ||
        out.ledger.scientificMasterModified || out.ledger.zeroLineModified) {
        out = {};
        return Status::UpstreamFailure;
    }
    return Status::Ok;
}

Status evaluate_relative_sample(const RoomSampleRuntime& sample,
                                const RelativeScenario& scenario,
                                std::span<const LightState> lights,
                                RelativeLightResult& out) noexcept {
    out = {};
    if (!valid_scenario_kind(scenario.kind) || !finite(scenario.neutralIlluminationScale) ||
        !(scenario.neutralIlluminationScale > 0.0)) {
        return Status::InvalidInput;
    }
    const auto s = room_capsule::v0_1::evaluate_relative_room_light(sample, scenario.boundary, lights, out);
    return map_capsule_status(s);
}

Status acquire_runtime_leases(const RoomExecutionContext& context,
                              const AdaptivePlan& plan,
                              RoomCapsuleLeaseSet& out) noexcept {
    out = {};
    if (!plan.valid || !plan.capsule.valid || plan.roomBudgetBytes == 0U ||
        plan.capsule.budgetBytes != plan.roomBudgetBytes) {
        return Status::InvalidInput;
    }
    return map_abi_status(room_abi::v0_1::acquire_room_capsule_leases(context, plan.capsule, out));
}

void release_runtime_leases(const RoomExecutionContext& context,
                            RoomCapsuleLeaseSet& leases) noexcept {
    room_abi::v0_1::release_room_capsule_leases(context, leases);
}

const char* status_name(Status status) noexcept {
    switch (status) {
        case Status::Ok: return "OK";
        case Status::InvalidInput: return "INVALID_INPUT";
        case Status::InvalidAuthority: return "INVALID_AUTHORITY";
        case Status::BackplaneInvalid: return "BACKPLANE_INVALID";
        case Status::BudgetTooSmall: return "BUDGET_TOO_SMALL";
        case Status::NoAdmissiblePlan: return "NO_ADMISSIBLE_PLAN";
        case Status::LeaseFailure: return "LEASE_FAILURE";
        case Status::UpstreamFailure: return "UPSTREAM_FAILURE";
        case Status::OutsideRoom: return "OUTSIDE_ROOM_UNCHANGED";
        case Status::PhysicalRelightBlocked: return "PHYSICAL_RELIGHT_BLOCKED";
    }
    return "UNKNOWN";
}

const char* scenario_kind_name(ScenarioKind kind) noexcept {
    switch (kind) {
        case ScenarioKind::RelativeDaylightStudy: return "RELATIVE_DAYLIGHT_STUDY";
        case ScenarioKind::RelativeNightStudy: return "RELATIVE_NIGHT_STUDY";
        case ScenarioKind::CustomRelative: return "CUSTOM_RELATIVE";
    }
    return "UNKNOWN";
}

} // namespace truthraw::illumination_room::v0_2
