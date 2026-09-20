#include "illumination_state_v0_82.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace truthraw::illumination_state::v0_82 {
namespace {

struct LocusPoint final {
    double reciprocalMegaK;
    double u;
    double v;
};

// Robertson/Wyszecki-Stiles Planckian-locus points in CIE 1960 UCS.
// Used only to derive a metadata-bound chromaticity coordinate. This does not
// identify an illuminant spectrum or classify daylight/artificial light.
constexpr std::array<LocusPoint, 31> kPlanckianLocus{{
    {0,   0.18006, 0.26352},
    {10,  0.18066, 0.26589},
    {20,  0.18133, 0.26846},
    {30,  0.18208, 0.27119},
    {40,  0.18293, 0.27407},
    {50,  0.18388, 0.27709},
    {60,  0.18494, 0.28021},
    {70,  0.18611, 0.28342},
    {80,  0.18740, 0.28668},
    {90,  0.18880, 0.28997},
    {100, 0.19032, 0.29326},
    {125, 0.19462, 0.30141},
    {150, 0.19962, 0.30921},
    {175, 0.20525, 0.31647},
    {200, 0.21142, 0.32312},
    {225, 0.21807, 0.32909},
    {250, 0.22511, 0.33439},
    {275, 0.23247, 0.33904},
    {300, 0.24010, 0.34308},
    {325, 0.24702, 0.34655},
    {350, 0.25591, 0.34951},
    {375, 0.26400, 0.35200},
    {400, 0.27218, 0.35407},
    {425, 0.28039, 0.35577},
    {450, 0.28863, 0.35714},
    {475, 0.29685, 0.35823},
    {500, 0.30505, 0.35907},
    {525, 0.31320, 0.35968},
    {550, 0.32129, 0.36011},
    {575, 0.32931, 0.36038},
    {600, 0.33724, 0.36051},
}};

bool nonzero(const Digest& digest) noexcept {
    return std::any_of(
        digest.begin(), digest.end(),
        [](std::uint8_t v) { return v != 0u; });
}

bool finite(double v) noexcept {
    return std::isfinite(v);
}

bool valid_xy(double x, double y) noexcept {
    return finite(x) && finite(y) &&
           x > 0.0 && y > 0.0 && x < 1.0 && y < 1.0 &&
           x + y < 1.0;
}

bool xy_to_uv1960(double x, double y, double& u, double& v) noexcept {
    const double denom = 1.5 - x + 6.0 * y;
    if (!finite(denom) || std::abs(denom) <= 1.0e-12) return false;
    u = 2.0 * x / denom;
    v = 3.0 * y / denom;
    return finite(u) && finite(v);
}

bool duv_polyline(double x, double y, double& out) noexcept {
    double u = 0.0;
    double v = 0.0;
    if (!xy_to_uv1960(x, y, u, v)) return false;

    double bestDistance2 = std::numeric_limits<double>::infinity();
    double bestSigned = 0.0;

    for (std::size_t i = 0u; i + 1u < kPlanckianLocus.size(); ++i) {
        const double ax = kPlanckianLocus[i].u;
        const double ay = kPlanckianLocus[i].v;
        const double bx = kPlanckianLocus[i + 1u].u;
        const double by = kPlanckianLocus[i + 1u].v;
        const double dx = bx - ax;
        const double dy = by - ay;
        const double len2 = dx * dx + dy * dy;
        if (!(len2 > 0.0) || !finite(len2)) continue;

        const double px = u - ax;
        const double py = v - ay;
        const double t = std::clamp((px * dx + py * dy) / len2, 0.0, 1.0);
        const double qx = ax + t * dx;
        const double qy = ay + t * dy;
        const double ex = u - qx;
        const double ey = v - qy;
        const double distance2 = ex * ex + ey * ey;
        if (!finite(distance2)) continue;

        if (distance2 < bestDistance2) {
            bestDistance2 = distance2;
            const double cross = dx * (v - qy) - dy * (u - qx);
            const double sign = cross < 0.0 ? -1.0 : 1.0;
            bestSigned = sign * std::sqrt(std::max(0.0, distance2));
        }
    }

    if (!finite(bestDistance2) || bestDistance2 == std::numeric_limits<double>::infinity()) {
        return false;
    }
    out = bestSigned;
    return finite(out);
}

void put_u16(truthraw::sha256_v0_69::Hasher& h, std::uint16_t v) noexcept {
    const std::array<std::uint8_t, 2> bytes{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v >> 8u),
    };
    h.update(bytes);
}

void put_u32(truthraw::sha256_v0_69::Hasher& h, std::uint32_t v) noexcept {
    const std::array<std::uint8_t, 4> bytes{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v >> 8u),
        static_cast<std::uint8_t>(v >> 16u),
        static_cast<std::uint8_t>(v >> 24u),
    };
    h.update(bytes);
}

void put_f64(truthraw::sha256_v0_69::Hasher& h, double v) noexcept {
    const std::uint64_t bits = std::bit_cast<std::uint64_t>(v);
    std::array<std::uint8_t, 8> bytes{};
    for (std::size_t i = 0u; i < bytes.size(); ++i) {
        bytes[i] = static_cast<std::uint8_t>(bits >> (8u * i));
    }
    h.update(bytes);
}

Digest hash_state(const Input& input, const State& state) noexcept {
    truthraw::sha256_v0_69::Hasher h;
    constexpr char domain[] =
        "TruthRawObservedIlluminationState/0.82\n"
        "role=OBSERVED_SOURCE_METADATA_BOUND_ILLUMINATION_STATE\n"
        "rgb_channel_authority_is_separate=1\n"
        "cct_is_not_spd_proof=1\n"
        "profile_calibration_illuminants_are_not_scene_light_proof=1\n"
        "light_kind_not_inferred_from_cct=1\n"
        "spatial_direction_unknown_without_evidence=1\n"
        "temporal_modulation_unknown_without_timing_evidence=1\n"
        "creates_new_evidence=0\n"
        "scientific_master_modified=0\n"
        "channel_authority_modified=0\n"
        "counterfactual=0\n";
    h.update(
        reinterpret_cast<const std::uint8_t*>(domain),
        sizeof(domain) - 1u);
    h.update(input.sourceEvidenceSha256);
    h.update(input.scientificMasterSha256);
    h.update(input.canonicalOpenSceneSha256);
    h.update(
        reinterpret_cast<const std::uint8_t*>(input.sourceEvidenceId.data()),
        input.sourceEvidenceId.size());
    h.update(
        reinterpret_cast<const std::uint8_t*>(input.colourBindingId.data()),
        input.colourBindingId.size());

    const std::array<std::uint8_t, 10> flags{
        static_cast<std::uint8_t>(state.whitePointAuthority),
        static_cast<std::uint8_t>(state.whitePointKnown),
        static_cast<std::uint8_t>(state.sceneLightKind),
        static_cast<std::uint8_t>(state.spectrumAuthority),
        static_cast<std::uint8_t>(state.directionAuthority),
        static_cast<std::uint8_t>(state.spatialExtentAuthority),
        static_cast<std::uint8_t>(state.temporalModulationAuthority),
        static_cast<std::uint8_t>(state.dualCalibrationUsed),
        static_cast<std::uint8_t>(state.physicalFrameCount),
        static_cast<std::uint8_t>(state.independentEvidenceCount),
    };
    h.update(flags);
    put_u16(h, state.profileCalibrationIlluminant1);
    put_u16(h, state.profileCalibrationIlluminant2);
    put_u32(h, state.physicalFrameCount);
    put_u32(h, state.independentEvidenceCount);
    if (state.whitePointKnown) {
        put_f64(h, state.whiteX);
        put_f64(h, state.whiteY);
        put_f64(h, state.correlatedColorTemperatureK);
        put_f64(h, state.duv1960PolylineEstimate);
    }
    return h.finalize();
}

} // namespace

bool build(const Input& input, State& out) noexcept {
    out = State{};

    if (!nonzero(input.sourceEvidenceSha256) ||
        !nonzero(input.scientificMasterSha256) ||
        !nonzero(input.canonicalOpenSceneSha256) ||
        input.sourceEvidenceId.empty() ||
        input.colourBindingId.empty() ||
        input.physicalFrameCount != 1u ||
        input.independentEvidenceCount != 1u) {
        return false;
    }

    out.dualCalibrationUsed = input.dualCalibrationUsed;
    out.profileCalibrationIlluminant1 = input.profileCalibrationIlluminant1;
    out.profileCalibrationIlluminant2 = input.profileCalibrationIlluminant2;
    out.physicalFrameCount = input.physicalFrameCount;
    out.independentEvidenceCount = input.independentEvidenceCount;

    if (input.resolvedWhiteAvailable) {
        if (!valid_xy(input.resolvedWhiteX, input.resolvedWhiteY) ||
            !finite(input.resolvedWhiteCctK) ||
            !(input.resolvedWhiteCctK > 0.0)) {
            return false;
        }
        double duv = 0.0;
        if (!duv_polyline(input.resolvedWhiteX, input.resolvedWhiteY, duv)) {
            return false;
        }

        out.whitePointAuthority = EstimateAuthority::SourceMetadataBoundEstimate;
        out.whitePointKnown = true;
        out.whiteX = input.resolvedWhiteX;
        out.whiteY = input.resolvedWhiteY;
        out.correlatedColorTemperatureK = input.resolvedWhiteCctK;
        out.duv1960PolylineEstimate = duv;
    }

    // Intentionally fail closed on semantic illumination claims.
    out.sceneLightKind = SceneLightKind::Unknown;
    out.spectrumAuthority = SpectrumAuthority::Unknown;
    out.directionAuthority = SpatialAuthority::Unknown;
    out.spatialExtentAuthority = SpatialAuthority::Unknown;
    out.temporalModulationAuthority = TemporalAuthority::Unknown;

    out.cctIsSpdProof = false;
    out.calibrationIlluminantsAreSceneLightProof = false;
    out.createsNewEvidence = false;
    out.scientificMasterModified = false;
    out.channelAuthorityModified = false;
    out.counterfactual = false;

    out.stateSha256 = hash_state(input, out);
    return nonzero(out.stateSha256);
}

const char* schema_name() noexcept {
    return "TruthRawObservedIlluminationState/0.82";
}

const char* authority_name(EstimateAuthority authority) noexcept {
    switch (authority) {
        case EstimateAuthority::Unknown: return "UNKNOWN";
        case EstimateAuthority::SourceMetadataBoundEstimate:
            return "SOURCE_METADATA_BOUND_ESTIMATE";
        case EstimateAuthority::IndependentlyCalibrated:
            return "INDEPENDENTLY_CALIBRATED";
    }
    return "INVALID";
}

const char* light_kind_name(SceneLightKind kind) noexcept {
    switch (kind) {
        case SceneLightKind::Unknown: return "UNKNOWN";
        case SceneLightKind::Daylight: return "DAYLIGHT";
        case SceneLightKind::Artificial: return "ARTIFICIAL";
        case SceneLightKind::Mixed: return "MIXED";
    }
    return "INVALID";
}

const char* spectrum_authority_name(SpectrumAuthority authority) noexcept {
    switch (authority) {
        case SpectrumAuthority::Unknown: return "UNKNOWN";
        case SpectrumAuthority::CalibratedSpd: return "CALIBRATED_SPD";
    }
    return "INVALID";
}

const char* spatial_authority_name(SpatialAuthority authority) noexcept {
    switch (authority) {
        case SpatialAuthority::Unknown: return "UNKNOWN";
        case SpatialAuthority::ImageSpaceEvidenceOnly:
            return "IMAGE_SPACE_EVIDENCE_ONLY";
        case SpatialAuthority::CalibratedGeometry:
            return "CALIBRATED_GEOMETRY";
    }
    return "INVALID";
}

const char* temporal_authority_name(TemporalAuthority authority) noexcept {
    switch (authority) {
        case TemporalAuthority::Unknown: return "UNKNOWN";
        case TemporalAuthority::CaptureTimingBoundObservation:
            return "CAPTURE_TIMING_BOUND_OBSERVATION";
        case TemporalAuthority::IndependentlyCalibrated:
            return "INDEPENDENTLY_CALIBRATED";
    }
    return "INVALID";
}

} // namespace truthraw::illumination_state::v0_82
