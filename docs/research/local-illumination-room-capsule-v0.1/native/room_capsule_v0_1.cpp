#include "room_capsule_v0_1.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace truthraw::room_capsule::v0_1 {
namespace {
constexpr std::uint64_t kMiB = 1024ULL * 1024ULL;
constexpr std::uint32_t kGeometryScales[] = {4, 8, 16, 32, 64};
constexpr std::uint32_t kHalo = 4;
constexpr std::uint64_t kTileBytesPerPixel = 32; // transient RGB/luma/confidence/scratch upper estimate
constexpr std::uint64_t kVectorVertexBytes = 8; // quantized x/y uint32 pair
constexpr std::uint64_t kMinBudget = 2ULL * kMiB;
constexpr std::uint64_t kMaxBudget = 64ULL * kMiB;

bool finite(float x) noexcept { return std::isfinite(static_cast<double>(x)); }

bool finite3(const std::array<float, 3>& v) noexcept {
    return finite(v[0]) && finite(v[1]) && finite(v[2]);
}

float dot3(const std::array<float, 3>& a, const std::array<float, 3>& b) noexcept {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

float norm3(const std::array<float, 3>& v) noexcept {
    return std::sqrt(std::max(0.0F, dot3(v, v)));
}

bool normalize3(std::array<float, 3>& v) noexcept {
    const float n = norm3(v);
    if (!(n > 1.0e-12F) || !finite(n)) return false;
    v[0] /= n; v[1] /= n; v[2] /= n;
    return true;
}

bool safe_mul_u64(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a != 0 && b > std::numeric_limits<std::uint64_t>::max() / a) return false;
    out = a * b;
    return true;
}

std::uint32_t choose_tile_size(const MobileMemoryInput& memory) noexcept {
    if (memory.lowRamDevice || (memory.appMemoryClassMiB > 0 && memory.appMemoryClassMiB < 192)) return 64;
    if (memory.appMemoryClassMiB > 0 && memory.appMemoryClassMiB < 384) return 128;
    return 256;
}

std::uint64_t tile_workspace(std::uint32_t tile) noexcept {
    const std::uint64_t side = static_cast<std::uint64_t>(tile) + 2ULL * kHalo;
    return side * side * kTileBytesPerPixel;
}

RoomEvidenceLedger default_ledger() noexcept { return {}; }
} // namespace

std::uint64_t derive_room_budget_bytes(const MobileMemoryInput& memory) noexcept {
    if (memory.explicitMaxWorkingSetBytes > 0) {
        // An explicit caller ceiling is authoritative. Never silently round it upward.
        return std::min<std::uint64_t>(memory.explicitMaxWorkingSetBytes, kMaxBudget);
    }
    if (memory.appMemoryClassMiB == 0) return 8ULL * kMiB;
    // Reserve most app memory for RAW decode, reconstruction, UI and OS pressure.
    // Room lighting receives only 1/16 of memoryClass, clamped to a small bounded working set.
    const std::uint64_t classBytes = memory.appMemoryClassMiB * kMiB;
    std::uint64_t budget = classBytes / 16ULL;
    if (memory.lowRamDevice) budget = std::min<std::uint64_t>(budget, 8U * kMiB);
    return std::clamp(budget, kMinBudget, kMaxBudget);
}

Status plan_room_capsule(const RoomCapsuleRequest& request, RoomCapsulePlan& out) noexcept {
    out = {};
    if (request.sourceWidth == 0 || request.sourceHeight == 0 ||
        request.room.width == 0 || request.room.height == 0 ||
        request.room.x >= request.sourceWidth || request.room.y >= request.sourceHeight ||
        request.room.width > request.sourceWidth - request.room.x ||
        request.room.height > request.sourceHeight - request.room.y ||
        request.vectorBoundaryVertexCount < 3) {
        return Status::InvalidInput;
    }

    const std::uint64_t budget = derive_room_budget_bytes(request.memory);
    if (budget < kMinBudget) return Status::BudgetTooSmall;
    const std::uint32_t tile = choose_tile_size(request.memory);
    const std::uint64_t tileBytes = tile_workspace(tile);
    const std::uint64_t boundaryBytes = static_cast<std::uint64_t>(request.vectorBoundaryVertexCount) * kVectorVertexBytes;

    for (const std::uint32_t scale : kGeometryScales) {
        const std::uint32_t gw = (request.room.width + scale - 1U) / scale;
        const std::uint32_t gh = (request.room.height + scale - 1U) / scale;
        std::uint64_t samples = 0;
        std::uint64_t packed = 0;
        if (!safe_mul_u64(gw, gh, samples) || !safe_mul_u64(samples, sizeof(PackedRoomSample), packed)) {
            continue;
        }
        // Small fixed metadata allowance keeps planner conservative and architecture-independent.
        constexpr std::uint64_t metadataAllowance = 64ULL * 1024ULL;
        if (packed > std::numeric_limits<std::uint64_t>::max() - boundaryBytes - tileBytes - metadataAllowance) continue;
        const std::uint64_t peak = packed + boundaryBytes + tileBytes + metadataAllowance;
        if (peak <= budget) {
            out.valid = true;
            out.roomBounds = request.room;
            out.baselineBackend = ComputeBackend::CpuBaseline;
            out.gpuAccelerationOptional = request.memory.gpuAvailable;
            out.geometryDownsample = scale;
            out.geometryWidth = gw;
            out.geometryHeight = gh;
            out.tileSize = tile;
            out.tileHalo = kHalo;
            out.packedGeometryBytes = packed;
            out.vectorBoundaryBytes = boundaryBytes;
            out.tileWorkspaceBytes = tileBytes;
            out.estimatedPeakBytes = peak;
            out.budgetBytes = budget;
            out.roomTileColumns = (request.room.width + tile - 1U) / tile;
            out.roomTileRows = (request.room.height + tile - 1U) / tile;
            out.roomTileCount = static_cast<std::uint64_t>(out.roomTileColumns) * out.roomTileRows;
            out.ledger = default_ledger();
            return Status::Ok;
        }
    }
    return Status::NoAdmissiblePlan;
}

Status evaluate_relative_room_light(const RoomSampleRuntime& sample,
    const BoundaryIlluminationEnvelope& boundary,
    std::span<const LightState> lights,
    RelativeLightResult& out) noexcept {
    out = {};
    out.ledger = default_ledger();
    out.ledger.counterfactualLightStateCount = static_cast<std::uint32_t>(std::min<std::size_t>(lights.size(), std::numeric_limits<std::uint32_t>::max()));

    if (!sample.insideRoom) {
        out.valid = true;
        out.relativeMultiplier = {1.0F, 1.0F, 1.0F};
        return Status::OutsideRoom;
    }
    if (!finite3(sample.position) || !finite3(sample.normal) ||
        !finite(sample.visibility) || !finite(sample.confidence) ||
        sample.visibility < 0.0F || sample.visibility > 1.0F ||
        sample.confidence < 0.0F || sample.confidence > 1.0F ||
        !finite3(boundary.ambientRgb) || !finite3(boundary.dominantIncomingDirection) ||
        !finite(boundary.directionalWeight) || boundary.directionalWeight < 0.0F) {
        return Status::InvalidInput;
    }

    std::array<float, 3> n = sample.normal;
    if (!normalize3(n)) return Status::InvalidInput;

    for (const float c : boundary.ambientRgb) if (c < 0.0F) return Status::InvalidInput;

    std::array<float, 3> rgb = boundary.ambientRgb;
    float directScalar = 0.0F;
    bool physicalRequested = false;

    if (boundary.directionalWeight > 0.0F) {
        std::array<float, 3> towardBoundary = boundary.dominantIncomingDirection;
        if (!normalize3(towardBoundary)) return Status::InvalidInput;
        const float boundaryDirectional = boundary.directionalWeight * std::max(0.0F, dot3(n, towardBoundary)) * sample.visibility;
        for (std::size_t c = 0; c < 3; ++c) rgb[c] += boundaryDirectional * boundary.ambientRgb[c];
        directScalar += boundaryDirectional;
    }

    for (const auto& light : lights) {
        if (!finite3(light.position) || !finite3(light.direction) || !finite3(light.rgb) ||
            !finite(light.relativePower) || !finite(light.radius) || light.relativePower < 0.0F || light.radius < 0.0F) {
            return Status::InvalidInput;
        }
        for (const float c : light.rgb) if (c < 0.0F) return Status::InvalidInput;
        if (light.semantics == RelightSemantics::CalibratedIntrinsicRelightReserved) physicalRequested = true;

        float geometric = 0.0F;
        if (light.kind == LightKind::AmbientOnly) {
            geometric = 1.0F;
        } else if (light.kind == LightKind::Directional) {
            std::array<float, 3> towardLight{-light.direction[0], -light.direction[1], -light.direction[2]};
            if (!normalize3(towardLight)) return Status::InvalidInput;
            geometric = std::max(0.0F, dot3(n, towardLight)) * sample.visibility;
        } else if (light.kind == LightKind::Point) {
            std::array<float, 3> towardLight{
                light.position[0] - sample.position[0],
                light.position[1] - sample.position[1],
                light.position[2] - sample.position[2]};
            const float d = norm3(towardLight);
            if (!(d > 1.0e-6F) || !normalize3(towardLight)) return Status::InvalidInput;
            const float lambert = std::max(0.0F, dot3(n, towardLight));
            const float softenedD2 = d * d + std::max(1.0e-6F, light.radius * light.radius);
            geometric = lambert * sample.visibility / softenedD2;
        } else {
            return Status::InvalidInput;
        }

        const float contribution = light.relativePower * geometric;
        directScalar += contribution;
        for (std::size_t c = 0; c < 3; ++c) rgb[c] += contribution * light.rgb[c];
    }

    // Confidence only attenuates synthesized direct-light departure from the boundary envelope.
    // Low-confidence geometry fails toward the boundary illumination rather than inventing structure.
    for (std::size_t c = 0; c < 3; ++c) {
        const float directPart = rgb[c] - boundary.ambientRgb[c];
        rgb[c] = boundary.ambientRgb[c] + sample.confidence * directPart;
        if (!finite(rgb[c]) || rgb[c] < 0.0F) return Status::InvalidInput;
    }

    if (physicalRequested) {
        // Fail closed: a reserved physical request must not leave an apply-able synthetic result behind.
        out = {};
        out.ledger = default_ledger();
        out.ledger.counterfactualLightStateCount = static_cast<std::uint32_t>(std::min<std::size_t>(lights.size(), std::numeric_limits<std::uint32_t>::max()));
        return Status::PhysicalRelightBlocked;
    }

    out.valid = true;
    out.physicalRelightClaimAllowed = false;
    out.relativeMultiplier = rgb;
    out.directTerm = directScalar * sample.confidence;
    out.ambientTerm = (boundary.ambientRgb[0] + boundary.ambientRgb[1] + boundary.ambientRgb[2]) / 3.0F;
    return Status::Ok;
}

std::array<float, 3> apply_relative_room_light(const std::array<float, 3>& sourceRgb,
    const RelativeLightResult& light) noexcept {
    if (!light.valid) return sourceRgb;
    std::array<float, 3> out = sourceRgb;
    for (std::size_t c = 0; c < 3; ++c) {
        if (!finite(sourceRgb[c]) || sourceRgb[c] < 0.0F || !finite(light.relativeMultiplier[c]) || light.relativeMultiplier[c] < 0.0F) {
            return sourceRgb;
        }
        out[c] = sourceRgb[c] * light.relativeMultiplier[c];
    }
    return out;
}

std::uint64_t light_state_storage_bytes(std::size_t count) noexcept {
    if (count > std::numeric_limits<std::uint64_t>::max() / sizeof(LightState)) return std::numeric_limits<std::uint64_t>::max();
    return static_cast<std::uint64_t>(count) * sizeof(LightState);
}

const char* status_name(Status status) noexcept {
    switch (status) {
        case Status::Ok: return "OK";
        case Status::InvalidInput: return "INVALID_INPUT";
        case Status::BudgetTooSmall: return "BUDGET_TOO_SMALL";
        case Status::NoAdmissiblePlan: return "NO_ADMISSIBLE_PLAN";
        case Status::OutsideRoom: return "OUTSIDE_ROOM_UNCHANGED";
        case Status::PhysicalRelightBlocked: return "PHYSICAL_RELIGHT_BLOCKED";
    }
    return "UNKNOWN";
}

} // namespace truthraw::room_capsule::v0_1
