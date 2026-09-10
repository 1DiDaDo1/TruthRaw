#include "room_capsule_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace truthraw::room_capsule::v0_1;

namespace {
void require(bool cond, const char* msg) {
    if (!cond) {
        std::cerr << "FAIL: " << msg << '\n';
        std::exit(2);
    }
}

bool near(float a, float b, float eps = 1e-5F) { return std::abs(a - b) <= eps; }
}

int main() {
    require(sizeof(PackedRoomSample) == 8, "packed room sample must be 8 bytes");
    require(sizeof(LightState) <= 64, "light state must be <=64 bytes");

    RoomCapsuleRequest phone12{};
    phone12.sourceWidth = 4000;
    phone12.sourceHeight = 3000;
    phone12.room = {1000, 750, 2000, 1500};
    phone12.vectorBoundaryVertexCount = 24;
    phone12.memory.appMemoryClassMiB = 128;
    phone12.memory.lowRamDevice = true;
    RoomCapsulePlan p12{};
    require(plan_room_capsule(phone12, p12) == Status::Ok, "12MP low-RAM plan");
    require(p12.valid && p12.geometryDownsample == 4, "12MP room keeps 1/4 geometry");
    require(p12.estimatedPeakBytes <= p12.budgetBytes, "12MP budget respected");
    require(p12.baselineBackend == ComputeBackend::CpuBaseline, "CPU baseline mandatory");
    const std::uint64_t full12Tiles = ((phone12.sourceWidth + p12.tileSize - 1U) / p12.tileSize) *
        static_cast<std::uint64_t>((phone12.sourceHeight + p12.tileSize - 1U) / p12.tileSize);
    require(p12.roomTileCount < full12Tiles, "scheduler touches only room tiles");
    require(static_cast<double>(p12.roomTileCount) / static_cast<double>(full12Tiles) < 0.30, "25 percent room avoids roughly three quarters of full-frame tile work");

    RoomCapsuleRequest tinyBudget = phone12;
    tinyBudget.memory.explicitMaxWorkingSetBytes = 1ULL * 1024ULL * 1024ULL;
    RoomCapsulePlan tinyPlan{};
    require(plan_room_capsule(tinyBudget, tinyPlan) == Status::BudgetTooSmall, "explicit memory ceiling is never rounded upward");

    RoomCapsuleRequest huge{};
    huge.sourceWidth = 16384;
    huge.sourceHeight = 12288;
    huge.room = {0, 0, 16384, 12288};
    huge.vectorBoundaryVertexCount = 64;
    huge.memory.appMemoryClassMiB = 128;
    huge.memory.lowRamDevice = true;
    RoomCapsulePlan pHuge{};
    require(plan_room_capsule(huge, pHuge) == Status::Ok, "200MP low-RAM plan");
    require(pHuge.geometryDownsample >= 16, "200MP geometry degrades adaptively");
    require(pHuge.estimatedPeakBytes <= pHuge.budgetBytes, "200MP budget respected");
    require(pHuge.tileWorkspaceBytes == p12.tileWorkspaceBytes, "tile workspace independent of megapixels on same tier");

    RoomCapsuleRequest premium = huge;
    premium.memory.appMemoryClassMiB = 512;
    premium.memory.lowRamDevice = false;
    premium.memory.gpuAvailable = true;
    RoomCapsulePlan pPremium{};
    require(plan_room_capsule(premium, pPremium) == Status::Ok, "premium plan");
    require(pPremium.geometryDownsample <= pHuge.geometryDownsample, "more memory never forces coarser geometry");
    require(pPremium.gpuAccelerationOptional, "GPU remains optional");

    require(light_state_storage_bytes(1000) <= 64ULL * 1000ULL, "1000 light states stay tiny");

    RoomSampleRuntime outside{};
    outside.insideRoom = false;
    RelativeLightResult outsideLight{};
    BoundaryIlluminationEnvelope neutral{};
    require(evaluate_relative_room_light(outside, neutral, {}, outsideLight) == Status::OutsideRoom, "outside room passthrough");
    const std::array<float,3> src{0.2F, 0.4F, 0.8F};
    const auto untouched = apply_relative_room_light(src, outsideLight);
    require(untouched == src, "outside room pixels unchanged exactly");

    RoomSampleRuntime facing{};
    facing.position = {0.0F, 0.0F, 0.0F};
    facing.normal = {0.0F, 0.0F, 1.0F};
    facing.visibility = 1.0F;
    facing.confidence = 1.0F;

    BoundaryIlluminationEnvelope dark{};
    dark.ambientRgb = {0.2F, 0.2F, 0.2F};
    RelativeLightResult darkResult{};
    require(evaluate_relative_room_light(facing, dark, {}, darkResult) == Status::Ok, "dark room state");
    const auto darker = apply_relative_room_light(src, darkResult);
    require(near(darker[0], 0.04F) && near(darker[1], 0.08F) && near(darker[2], 0.16F), "dark state lowers appearance predictably");

    LightState key{};
    key.kind = LightKind::Directional;
    key.direction = {0.0F, 0.0F, -1.0F};
    key.rgb = {1.0F, 0.8F, 0.6F};
    key.relativePower = 1.0F;
    std::array<LightState,1> lights{key};
    RelativeLightResult lit{};
    require(evaluate_relative_room_light(facing, dark, lights, lit) == Status::Ok, "directional art light");
    require(lit.relativeMultiplier[0] > darkResult.relativeMultiplier[0], "facing light adds energy");

    RoomSampleRuntime back = facing;
    back.normal = {0.0F, 0.0F, -1.0F};
    RelativeLightResult backLit{};
    require(evaluate_relative_room_light(back, dark, lights, backLit) == Status::Ok, "back-facing sample");
    require(near(backLit.relativeMultiplier[0], dark.ambientRgb[0]), "back face gets no directional Lambert term");

    BoundaryIlluminationEnvelope boundaryDirectional = dark;
    boundaryDirectional.dominantIncomingDirection = {0.0F, 0.0F, 1.0F};
    boundaryDirectional.directionalWeight = 0.5F;
    RelativeLightResult boundaryLit{};
    require(evaluate_relative_room_light(facing, boundaryDirectional, {}, boundaryLit) == Status::Ok, "boundary directional envelope");
    require(boundaryLit.relativeMultiplier[0] > darkResult.relativeMultiplier[0], "boundary envelope carries compact outside-world directional influence");

    RoomSampleRuntime uncertain = facing;
    uncertain.confidence = 0.0F;
    RelativeLightResult uncertainLit{};
    require(evaluate_relative_room_light(uncertain, dark, lights, uncertainLit) == Status::Ok, "uncertain geometry");
    require(near(uncertainLit.relativeMultiplier[0], dark.ambientRgb[0]), "uncertain geometry fails toward boundary light");

    LightState physical = key;
    physical.semantics = RelightSemantics::CalibratedIntrinsicRelightReserved;
    std::array<LightState,1> physicalLights{physical};
    RelativeLightResult blocked{};
    require(evaluate_relative_room_light(facing, dark, physicalLights, blocked) == Status::PhysicalRelightBlocked, "physical relight remains blocked");
    require(!blocked.valid && !blocked.physicalRelightClaimAllowed, "blocked physical path fails closed");
    const auto blockedApply = apply_relative_room_light(src, blocked);
    require(blockedApply == src, "blocked physical request cannot alter RGB even if caller ignores status");

    require(lit.ledger.physicalFrameCount == 1 && lit.ledger.independentEvidenceCount == 1, "single evidence root preserved");
    require(!lit.ledger.lightStatesAreEvidence && !lit.ledger.scientificMasterModified && !lit.ledger.zeroLineModified, "counterfactual states cannot mutate evidence");

    RoomCapsuleRequest bad = phone12;
    bad.room.width = 5000;
    RoomCapsulePlan badPlan{};
    require(plan_room_capsule(bad, badPlan) == Status::InvalidInput, "invalid ROI fails closed");

    std::cout << "packed_sample_bytes=" << sizeof(PackedRoomSample) << '\n';
    std::cout << "light_state_bytes=" << sizeof(LightState) << '\n';
    std::cout << "lowram_12mp_geometry_scale=1/" << p12.geometryDownsample << '\n';
    std::cout << "lowram_200mp_geometry_scale=1/" << pHuge.geometryDownsample << '\n';
    std::cout << "lowram_peak_bytes=" << pHuge.estimatedPeakBytes << '\n';
    std::cout << "lowram_budget_bytes=" << pHuge.budgetBytes << '\n';
    std::cout << "tile_workspace_bytes=" << pHuge.tileWorkspaceBytes << '\n';
    std::cout << "12mp_room_tiles=" << p12.roomTileCount << '\n';
    std::cout << "12mp_full_tiles=" << full12Tiles << '\n';
    std::cout << "1000_light_states_bytes=" << light_state_storage_bytes(1000) << '\n';
    std::cout << "ROOM_CAPSULE_V0_1_PASS\n";
    return 0;
}
