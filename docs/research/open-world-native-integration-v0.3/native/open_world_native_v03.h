#pragma once

#include "cicm_v1.h"
#include "room_capsule_v0_1.h"

#include <cstdint>
#include <span>
#include <string>

namespace truthraw::open_world::v0_3 {

enum class Status : std::uint8_t {
    Ok = 0,
    InvalidAuthorityBinding,
    UpstreamRejected
};

enum class IlluminationAuthority : std::uint8_t {
    Measured = 0,
    CalibratedEstimate = 1,
    Inferred = 2,
    Counterfactual = 3
};

enum class OutputAuthority : std::uint8_t {
    Measured = 0,
    CalibratedEstimate = 1,
    Inferred = 2,
    Counterfactual = 3,
    AppearanceOnly = 4
};

// Compact corridor-side provenance for illumination. Pixel payloads stay in the
// owning scientific/counterfactual artifact; this object transports authority.
// spatialScope is descriptive and deliberately imposes no sealed-room/world
// boundary: it may name a tile, room, street, landscape, sky dome or arbitrary
// open-world scene graph region.
struct IlluminationBinding {
    bool present = false;
    IlluminationAuthority authority = IlluminationAuthority::Counterfactual;
    std::string recordId;
    std::string spatialScope;
    std::string provenanceSha256;
    std::string calibrationId;
    std::string inferenceMethod;
    std::string counterfactualParentId;
};

struct AuthorityLedger {
    bool bindingPresent = false;
    IlluminationAuthority inputAuthority = IlluminationAuthority::Counterfactual;
    OutputAuthority outputAuthority = OutputAuthority::Counterfactual;
    bool mayModifyScientificMaster = false;
    bool independentEvidenceAdded = false;
    std::string provenanceSha256;
};

struct CicmRelativeEnvelope {
    bool valid = false;
    Status status = Status::InvalidAuthorityBinding;
    truthraw::counterfactual::v1::Status upstreamStatus = truthraw::counterfactual::v1::Status::InvalidInput;
    truthraw::counterfactual::v1::RelativeWorldPrediction prediction;
    AuthorityLedger authority;
};

struct CicmSensorEnvelope {
    bool valid = false;
    Status status = Status::InvalidAuthorityBinding;
    truthraw::counterfactual::v1::Status upstreamStatus = truthraw::counterfactual::v1::Status::InvalidInput;
    truthraw::counterfactual::v1::SensorPrediction prediction;
    AuthorityLedger authority;
};

struct RoomCapsuleEnvelope {
    bool valid = false;
    Status status = Status::InvalidAuthorityBinding;
    truthraw::room_capsule::v0_1::Status upstreamStatus = truthraw::room_capsule::v0_1::Status::InvalidInput;
    truthraw::room_capsule::v0_1::RelativeLightResult result;
    AuthorityLedger authority;
};

Status validate_illumination_binding(const IlluminationBinding& binding) noexcept;

Status simulate_cicm_relative(
    double nonnegativeSceneSignal,
    const truthraw::counterfactual::v1::CounterfactualWorldSpec& world,
    const truthraw::counterfactual::v1::RelativeCaptureSpec& capture,
    const IlluminationBinding& illumination,
    CicmRelativeEnvelope& out) noexcept;

Status predict_cicm_calibrated_capture(
    double nonnegativeSceneSignal,
    const truthraw::counterfactual::v1::CounterfactualWorldSpec& world,
    const truthraw::counterfactual::v1::PhysicalCaptureSpec& capture,
    const truthraw::counterfactual::v1::SensorModeCalibration& calibration,
    const IlluminationBinding& illumination,
    CicmSensorEnvelope& out) noexcept;

Status evaluate_room_capsule_relative(
    const truthraw::room_capsule::v0_1::RoomSampleRuntime& sample,
    const truthraw::room_capsule::v0_1::BoundaryIlluminationEnvelope& boundary,
    std::span<const truthraw::room_capsule::v0_1::LightState> lights,
    const IlluminationBinding& illumination,
    RoomCapsuleEnvelope& out) noexcept;

const char* status_name(Status status) noexcept;
const char* illumination_authority_name(IlluminationAuthority authority) noexcept;
const char* output_authority_name(OutputAuthority authority) noexcept;

} // namespace truthraw::open_world::v0_3
