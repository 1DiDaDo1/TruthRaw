#pragma once

#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstdint>
#include <string>

namespace truthraw::illumination_state::v0_82 {

using Digest = truthraw::sha256_v0_69::Digest;

enum class EstimateAuthority : std::uint8_t {
    Unknown = 0,
    SourceMetadataBoundEstimate = 1,
    IndependentlyCalibrated = 2,
};

enum class SceneLightKind : std::uint8_t {
    Unknown = 0,
    Daylight = 1,
    Artificial = 2,
    Mixed = 3,
};

enum class SpectrumAuthority : std::uint8_t {
    Unknown = 0,
    CalibratedSpd = 1,
};

enum class SpatialAuthority : std::uint8_t {
    Unknown = 0,
    ImageSpaceEvidenceOnly = 1,
    CalibratedGeometry = 2,
};

enum class TemporalAuthority : std::uint8_t {
    Unknown = 0,
    CaptureTimingBoundObservation = 1,
    IndependentlyCalibrated = 2,
};

struct Input final {
    Digest sourceEvidenceSha256{};
    Digest scientificMasterSha256{};
    Digest canonicalOpenSceneSha256{};
    std::string sourceEvidenceId;
    std::string colourBindingId;

    bool dualCalibrationUsed = false;
    std::uint16_t profileCalibrationIlluminant1 = 0;
    std::uint16_t profileCalibrationIlluminant2 = 0;

    bool resolvedWhiteAvailable = false;
    double resolvedWhiteX = 0.0;
    double resolvedWhiteY = 0.0;
    double resolvedWhiteCctK = 0.0;

    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;
};

struct State final {
    EstimateAuthority whitePointAuthority = EstimateAuthority::Unknown;
    bool whitePointKnown = false;
    double whiteX = 0.0;
    double whiteY = 0.0;
    double correlatedColorTemperatureK = 0.0;
    double duv1960PolylineEstimate = 0.0;

    SceneLightKind sceneLightKind = SceneLightKind::Unknown;
    SpectrumAuthority spectrumAuthority = SpectrumAuthority::Unknown;
    SpatialAuthority directionAuthority = SpatialAuthority::Unknown;
    SpatialAuthority spatialExtentAuthority = SpatialAuthority::Unknown;
    TemporalAuthority temporalModulationAuthority = TemporalAuthority::Unknown;

    bool cctIsSpdProof = false;
    bool calibrationIlluminantsAreSceneLightProof = false;
    bool createsNewEvidence = false;
    bool scientificMasterModified = false;
    bool channelAuthorityModified = false;
    bool counterfactual = false;

    bool dualCalibrationUsed = false;
    std::uint16_t profileCalibrationIlluminant1 = 0;
    std::uint16_t profileCalibrationIlluminant2 = 0;
    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;

    Digest stateSha256{};
};

bool build(const Input& input, State& out) noexcept;

const char* schema_name() noexcept;
const char* authority_name(EstimateAuthority authority) noexcept;
const char* light_kind_name(SceneLightKind kind) noexcept;
const char* spectrum_authority_name(SpectrumAuthority authority) noexcept;
const char* spatial_authority_name(SpatialAuthority authority) noexcept;
const char* temporal_authority_name(TemporalAuthority authority) noexcept;

}  // namespace truthraw::illumination_state::v0_82
