#pragma once

#include "free_world_pixel_resolve_2d_v0_2.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace truthraw::truthnegative_camera5_color_highlight_oracle::v0_1 {

namespace fw = truthraw::free_world_pixel_resolve_2d::v0_2;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "TruthNegativeCamera5ColorHighlightOracle/0.1";
inline constexpr std::array<double, 5u> kExposureEv{
    0.0, -0.5, -1.0, -2.0, -3.0};

enum class RemosaicState : std::uint8_t {
    Unknown = 0u,
    Disabled = 1u,
    Enabled = 2u,
    OemHintOnly = 3u,
};

enum class FirstFailureStage : std::uint8_t {
    None = 0u,
    SourceCensoring = 1u,
    MetadataNeutralMismatch = 2u,
    ColorBinding = 3u,
    AppearanceDisplay = 4u,
    Unresolved = 5u,
};

struct Sample final {
    std::array<double, 3u> cameraNativeRgb{};
    std::array<double, 3u> xyzD50{};
    std::array<std::array<double, 3u>, 5u> encodedRgbByExposure{};
    std::array<bool, 5u> displayClampByExposure{};
    std::array<fw::ResolvedAuthority, 3u> authority{
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown};
};

struct Input final {
    Digest sourceEvidenceSha256{};
    Digest scientificMasterSha256{};
    Digest truthNegativeStateSha256{};
    Digest colorBindingSha256{};

    std::uint32_t physicalCameraId = 0u;
    double whiteLevel = 0.0;
    std::array<double, 4u> blackPhase{};
    std::array<double, 3u> asShotNeutral{};
    bool asShotNeutralKnown = false;
    RemosaicState remosaicState = RemosaicState::Unknown;

    std::vector<Sample> samples;
};

struct Report final {
    Digest oracleSha256{};
    FirstFailureStage firstFailureStage =
        FirstFailureStage::Unresolved;

    std::uint64_t sampleCount = 0u;
    std::uint64_t apparentWhiteHighlightCandidates = 0u;
    std::uint64_t censoredChannelCount = 0u;
    std::uint64_t clampedEv0CandidateCount = 0u;

    std::array<double, 3u> empiricalCameraNeutral{};
    bool empiricalCameraNeutralKnown = false;

    double metadataNeutralLogError = 0.0;
    double lowExposureGreenBias = 0.0;
    double exposureGreenDrift = 0.0;
    double unclampedExposureHueDrift = 0.0;
    double censoredChannelFraction = 0.0;

    bool metadataNeutralMismatch = false;
    bool sourceCensoringDominant = false;
    bool colorBindingBiasDetected = false;
    bool appearanceDisplayDriftDetected = false;
    bool remosaicScientificallyResolved = false;

    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

bool run(const Input& input, Report& out) noexcept;

const char* toString(RemosaicState state) noexcept;
const char* toString(FirstFailureStage stage) noexcept;
const char* schema_name() noexcept;

}  // namespace truthraw::truthnegative_camera5_color_highlight_oracle::v0_1
