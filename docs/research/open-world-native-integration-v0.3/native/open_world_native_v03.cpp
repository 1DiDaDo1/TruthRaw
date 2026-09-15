#include "open_world_native_v03.h"

namespace truthraw::open_world::v0_3 {
namespace {

bool hex_sha256(const std::string& s) noexcept {
    if (s.size() != 64) return false;
    for (const char c : s) {
        const bool digit = c >= '0' && c <= '9';
        const bool lower = c >= 'a' && c <= 'f';
        const bool upper = c >= 'A' && c <= 'F';
        if (!(digit || lower || upper)) return false;
    }
    return true;
}

AuthorityLedger make_counterfactual_ledger(const IlluminationBinding& b) {
    AuthorityLedger out;
    out.bindingPresent = b.present;
    out.inputAuthority = b.authority;
    out.outputAuthority = OutputAuthority::Counterfactual;
    out.mayModifyScientificMaster = false;
    out.independentEvidenceAdded = false;
    out.provenanceSha256 = b.provenanceSha256;
    return out;
}

} // namespace

Status validate_illumination_binding(const IlluminationBinding& b) noexcept {
    if (!b.present || b.recordId.empty() || b.spatialScope.empty()) {
        return Status::InvalidAuthorityBinding;
    }
    switch (b.authority) {
        case IlluminationAuthority::Measured:
            return hex_sha256(b.provenanceSha256) ? Status::Ok : Status::InvalidAuthorityBinding;
        case IlluminationAuthority::CalibratedEstimate:
            return (hex_sha256(b.provenanceSha256) && !b.calibrationId.empty())
                ? Status::Ok : Status::InvalidAuthorityBinding;
        case IlluminationAuthority::Inferred:
            return (hex_sha256(b.provenanceSha256) && !b.inferenceMethod.empty())
                ? Status::Ok : Status::InvalidAuthorityBinding;
        case IlluminationAuthority::Counterfactual:
            return !b.counterfactualParentId.empty() ? Status::Ok : Status::InvalidAuthorityBinding;
    }
    return Status::InvalidAuthorityBinding;
}

Status simulate_cicm_relative(
    double scene,
    const truthraw::counterfactual::v1::CounterfactualWorldSpec& world,
    const truthraw::counterfactual::v1::RelativeCaptureSpec& capture,
    const IlluminationBinding& illumination,
    CicmRelativeEnvelope& out) noexcept {
    out = {};
    out.authority = make_counterfactual_ledger(illumination);
    const auto auth = validate_illumination_binding(illumination);
    if (auth != Status::Ok) {
        out.status = auth;
        return auth;
    }

    out.upstreamStatus = truthraw::counterfactual::v1::simulate_relative_world(
        scene, world, capture, out.prediction);
    if (out.upstreamStatus != truthraw::counterfactual::v1::Status::Ok) {
        out.status = Status::UpstreamRejected;
        return out.status;
    }

    // Hard authority gate: a CICM prediction is a new hypothetical observation,
    // never a second admitted exposure, regardless of the authority of the
    // illumination reference used to define that hypothetical world.
    if (out.prediction.ledger.independentEvidenceCount != 1 ||
        out.prediction.ledger.counterfactualObservationsAreEvidence ||
        out.prediction.ledger.scientificMasterModified ||
        out.prediction.ledger.zeroLineModified) {
        out = {};
        out.status = Status::UpstreamRejected;
        return out.status;
    }

    out.valid = true;
    out.status = Status::Ok;
    return Status::Ok;
}

Status predict_cicm_calibrated_capture(
    double scene,
    const truthraw::counterfactual::v1::CounterfactualWorldSpec& world,
    const truthraw::counterfactual::v1::PhysicalCaptureSpec& capture,
    const truthraw::counterfactual::v1::SensorModeCalibration& calibration,
    const IlluminationBinding& illumination,
    CicmSensorEnvelope& out) noexcept {
    out = {};
    out.authority = make_counterfactual_ledger(illumination);
    const auto auth = validate_illumination_binding(illumination);
    if (auth != Status::Ok) {
        out.status = auth;
        return auth;
    }

    out.upstreamStatus = truthraw::counterfactual::v1::predict_calibrated_capture(
        scene, world, capture, calibration, out.prediction);
    if (out.upstreamStatus != truthraw::counterfactual::v1::Status::Ok) {
        out.status = Status::UpstreamRejected;
        return out.status;
    }
    if (out.prediction.ledger.independentEvidenceCount != 1 ||
        out.prediction.ledger.counterfactualObservationsAreEvidence ||
        out.prediction.ledger.scientificMasterModified ||
        out.prediction.ledger.zeroLineModified) {
        out = {};
        out.status = Status::UpstreamRejected;
        return out.status;
    }

    out.valid = true;
    out.status = Status::Ok;
    return Status::Ok;
}

Status evaluate_room_capsule_relative(
    const truthraw::room_capsule::v0_1::RoomSampleRuntime& sample,
    const truthraw::room_capsule::v0_1::BoundaryIlluminationEnvelope& boundary,
    std::span<const truthraw::room_capsule::v0_1::LightState> lights,
    const IlluminationBinding& illumination,
    RoomCapsuleEnvelope& out) noexcept {
    out = {};
    out.authority = make_counterfactual_ledger(illumination);
    const auto auth = validate_illumination_binding(illumination);
    if (auth != Status::Ok) {
        out.status = auth;
        return auth;
    }

    out.upstreamStatus = truthraw::room_capsule::v0_1::evaluate_relative_room_light(
        sample, boundary, lights, out.result);
    if (out.upstreamStatus != truthraw::room_capsule::v0_1::Status::Ok &&
        out.upstreamStatus != truthraw::room_capsule::v0_1::Status::OutsideRoom) {
        out.status = Status::UpstreamRejected;
        return out.status;
    }
    if (out.result.ledger.independentEvidenceCount != 1 ||
        out.result.ledger.lightStatesAreEvidence ||
        out.result.ledger.scientificMasterModified ||
        out.result.ledger.zeroLineModified) {
        out = {};
        out.status = Status::UpstreamRejected;
        return out.status;
    }

    out.valid = true;
    out.status = Status::Ok;
    return Status::Ok;
}

const char* status_name(Status s) noexcept {
    switch (s) {
        case Status::Ok: return "OK";
        case Status::InvalidAuthorityBinding: return "INVALID_AUTHORITY_BINDING";
        case Status::UpstreamRejected: return "UPSTREAM_REJECTED";
    }
    return "UNKNOWN";
}

const char* illumination_authority_name(IlluminationAuthority a) noexcept {
    switch (a) {
        case IlluminationAuthority::Measured: return "MEASURED";
        case IlluminationAuthority::CalibratedEstimate: return "CALIBRATED_ESTIMATE";
        case IlluminationAuthority::Inferred: return "INFERRED";
        case IlluminationAuthority::Counterfactual: return "COUNTERFACTUAL";
    }
    return "UNKNOWN";
}

const char* output_authority_name(OutputAuthority a) noexcept {
    switch (a) {
        case OutputAuthority::Measured: return "MEASURED";
        case OutputAuthority::CalibratedEstimate: return "CALIBRATED_ESTIMATE";
        case OutputAuthority::Inferred: return "INFERRED";
        case OutputAuthority::Counterfactual: return "COUNTERFACTUAL";
        case OutputAuthority::AppearanceOnly: return "APPEARANCE_ONLY";
    }
    return "UNKNOWN";
}

} // namespace truthraw::open_world::v0_3
