#include "open_world_native_v03.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace truthraw::open_world::v0_3;
namespace cicm = truthraw::counterfactual::v1;
namespace room = truthraw::room_capsule::v0_1;

namespace {
void req(bool x, const char* m) {
    if (!x) {
        std::cerr << "FAIL: " << m << '\n';
        std::exit(2);
    }
}

cicm::SceneBinding bind() {
    return {"EVIDENCE", "SCENE_SCALE", "TRUTHRANGE_L0", std::string(64, 'a')};
}

IlluminationBinding measured() {
    IlluminationBinding b;
    b.present = true;
    b.authority = IlluminationAuthority::Measured;
    b.recordId = "measured-lamp";
    b.spatialScope = "street -> building facade -> sky dome";
    b.provenanceSha256 = std::string(64, 'b');
    return b;
}

IlluminationBinding counterfactual() {
    IlluminationBinding b;
    b.present = true;
    b.authority = IlluminationAuthority::Counterfactual;
    b.recordId = "virtual-sun";
    b.spatialScope = "unbounded outdoor scene graph";
    b.counterfactualParentId = "scene-master-1";
    return b;
}

cicm::SensorModeCalibration mode() {
    cicm::SensorModeCalibration m;
    m.authority = cicm::CalibrationAuthority::IndependentMeasurement;
    m.calibrationId = "fixture";
    m.calibrationProtocolId = "TEST";
    m.calibrationEvidenceSha256 = std::string(64, 'c');
    m.sourceClassId = "SOURCE";
    m.opticalModeId = "TELE";
    m.sensorModeId = "MAX";
    m.sceneScaleId = "SCENE_SCALE";
    m.illuminationReferenceId = "REF";
    m.nominalIso = 100.0;
    m.sceneUnitToElectronsPerSecond = 1000.0;
    m.readNoiseElectronsRms = 2.0;
    m.darkCurrentElectronsPerSecond = 1.0;
    m.fullWellElectrons = 5000.0;
    m.systemGainDnPerElectron = 1.0;
    m.blackOffsetDn = 64.0;
    m.adcWhiteDn = 4095.0;
    return m;
}
}

int main() {
    auto m = measured();
    req(validate_illumination_binding(m) == Status::Ok, "measured authority binding");

    auto badMeasured = m;
    badMeasured.provenanceSha256.clear();
    req(validate_illumination_binding(badMeasured) == Status::InvalidAuthorityBinding,
        "measured binding requires evidence hash");

    auto cf = counterfactual();
    req(validate_illumination_binding(cf) == Status::Ok, "counterfactual open-world binding");
    auto badCf = cf;
    badCf.counterfactualParentId.clear();
    req(validate_illumination_binding(badCf) == Status::InvalidAuthorityBinding,
        "counterfactual binding requires parent state");

    IlluminationBinding calibrated = m;
    calibrated.authority = IlluminationAuthority::CalibratedEstimate;
    calibrated.calibrationId = "illum-cal-1";
    req(validate_illumination_binding(calibrated) == Status::Ok, "calibrated authority binding");

    IlluminationBinding inferred = m;
    inferred.authority = IlluminationAuthority::Inferred;
    inferred.inferenceMethod = "single-frame local illumination inference";
    req(validate_illumination_binding(inferred) == Status::Ok, "inferred authority binding");

    cicm::CounterfactualWorldSpec rel{"WORLD", cicm::WorldSemantics::RelativeRadianceScaleOnly, 2.0, "", bind()};
    CicmRelativeEnvelope relOut;
    req(simulate_cicm_relative(0.25, rel, {0.5}, m, relOut) == Status::Ok,
        "measured reference may drive CICM relative world");
    req(relOut.valid, "relative envelope valid");
    req(relOut.authority.inputAuthority == IlluminationAuthority::Measured,
        "input measured authority survives corridor");
    req(relOut.authority.outputAuthority == OutputAuthority::Counterfactual,
        "CICM output remains counterfactual");
    req(!relOut.authority.mayModifyScientificMaster && !relOut.authority.independentEvidenceAdded,
        "counterfactual result cannot write scientific master or add evidence");

    CicmRelativeEnvelope cfOut;
    req(simulate_cicm_relative(0.25, rel, {1.0}, cf, cfOut) == Status::Ok,
        "counterfactual illumination may span open world");
    req(cfOut.authority.inputAuthority == IlluminationAuthority::Counterfactual,
        "counterfactual input retained");

    cicm::CounterfactualWorldSpec physical{
        "P", cicm::WorldSemantics::CalibratedNeutralIlluminationForward, 1.5, "REF", bind()};
    cicm::PhysicalCaptureSpec capture{"SOURCE", "TELE", 0.01};
    CicmSensorEnvelope sensorOut;
    req(predict_cicm_calibrated_capture(0.2, physical, capture, mode(), m, sensorOut) == Status::Ok,
        "calibrated sensor prediction with authority envelope");
    req(sensorOut.prediction.physicalForwardClaimAllowed,
        "upstream independent calibration can allow physical forward-model claim");
    req(sensorOut.authority.outputAuthority == OutputAuthority::Counterfactual,
        "even calibrated predicted capture remains counterfactual observation");

    room::RoomSampleRuntime sample{};
    sample.position = {0.0F, 0.0F, 0.0F};
    sample.normal = {0.0F, 0.0F, 1.0F};
    sample.visibility = 1.0F;
    sample.confidence = 1.0F;
    sample.insideRoom = true;

    room::BoundaryIlluminationEnvelope boundary{};
    boundary.ambientRgb = {0.2F, 0.2F, 0.2F};
    room::LightState light{};
    light.kind = room::LightKind::Directional;
    light.direction = {0.0F, 0.0F, -1.0F};
    light.relativePower = 1.0F;
    std::array<room::LightState, 1> lights{light};

    RoomCapsuleEnvelope roomOut;
    req(evaluate_room_capsule_relative(sample, boundary, lights, inferred, roomOut) == Status::Ok,
        "inferred illumination reference transported through local capsule");
    req(roomOut.valid && roomOut.result.valid, "room capsule result valid");
    req(roomOut.authority.inputAuthority == IlluminationAuthority::Inferred,
        "room capsule retains input authority");
    req(roomOut.authority.outputAuthority == OutputAuthority::Counterfactual,
        "room capsule transformed light remains counterfactual");
    req(!roomOut.authority.mayModifyScientificMaster && !roomOut.authority.independentEvidenceAdded,
        "room capsule cannot mutate evidence");

    room::RoomSampleRuntime outside = sample;
    outside.insideRoom = false;
    RoomCapsuleEnvelope outsideOut;
    req(evaluate_room_capsule_relative(outside, boundary, {}, cf, outsideOut) == Status::Ok,
        "outside local computational capsule is not outside the world");
    req(outsideOut.upstreamStatus == room::Status::OutsideRoom,
        "legacy local-capsule status preserved");
    req(outsideOut.valid, "outside local capsule passthrough remains a valid open-world result");

    std::cout << "OPEN_WORLD_NATIVE_V03_PASS\n";
    return 0;
}
