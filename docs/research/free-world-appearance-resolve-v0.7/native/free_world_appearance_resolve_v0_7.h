#pragma once

#include "free_world_deep_scene_contribution_v0_4.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstdint>
#include <string>

namespace truthraw::free_world_appearance_resolve::v0_7 {

namespace deep = truthraw::free_world_deep_scene_contribution::v0_4;
namespace free_world = truthraw::free_world_pixel_resolve_2d::v0_2;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "FreeWorldAppearanceResolve/0.7";
inline constexpr const char* kAppearanceMethodId =
    "LUMINANCE_CHROMATICITY_VIEW_DISPLAY_REFERENCE_V0_7";

struct Matrix3 final {
    std::array<double, 9u> m{};
};

enum class Surround : std::uint8_t {
    Dark = 1u,
    Dim = 2u,
    Average = 3u,
};

enum class TransferFunction : std::uint8_t {
    LinearNormalized = 1u,
    Srgb = 2u,
    PqSt2084 = 3u,
};

struct SceneColorimetry final {
    Matrix3 rgbToXyz{};
    std::array<double, 3u> referenceWhiteXyz{0.95047, 1.0, 1.08883};
    double sceneReferenceWhiteNits = 100.0;
    Digest identitySha256{};
};

struct ViewingConditions final {
    std::array<double, 3u> adaptingWhiteXyz{0.95047, 1.0, 1.08883};
    double adaptingLuminanceNits = 20.0;
    double backgroundLuminanceNits = 20.0;
    Surround surround = Surround::Average;
    double viewingDistanceMeters = 0.5;
    Digest identitySha256{};
};

struct DisplayTarget final {
    Matrix3 xyzToRgb{};
    std::array<double, 3u> whitePointXyz{0.95047, 1.0, 1.08883};
    double referenceWhiteNits = 100.0;
    double peakLuminanceNits = 100.0;
    double blackLuminanceNits = 0.0;
    TransferFunction transfer = TransferFunction::Srgb;
    Digest identitySha256{};
};

struct AppearancePolicy final {
    double exposureEv = 0.0;
    double colorfulnessScale = 1.0;
    double highlightCompression = 1.0;
    Digest identitySha256{};
};

struct AppearanceInput final {
    deep::DeepResolvedPixel scene{};
    SceneColorimetry sceneColorimetry{};
    ViewingConditions viewing{};
    DisplayTarget display{};
    AppearancePolicy policy{};
};

struct AppearanceResolvedPixel final {
    std::array<double, 3u> encodedRgb{};
    std::array<double, 3u> displayLinearNits{};
    std::array<double, 3u> mappedXyzNits{};

    std::array<free_world::ResolvedAuthority, 3u> channelAuthority{
        free_world::ResolvedAuthority::Unknown,
        free_world::ResolvedAuthority::Unknown,
        free_world::ResolvedAuthority::Unknown};
    std::array<bool, 3u> uncertaintyKnown{};
    std::array<double, 3u> p95Uncertainty{};

    Digest sourceSceneSha256{};
    Digest appearanceStateSha256{};
    Digest outputSha256{};

    double sourceLuminanceNits = 0.0;
    double mappedLuminanceNits = 0.0;

    bool gamutOrDisplayClampApplied = false;
    bool appearanceApplied = true;
    bool displayEncoded = true;
    bool sourceSceneMutated = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
    std::string methodId = kAppearanceMethodId;
};

bool validateMatrix(const Matrix3& matrix) noexcept;
bool validateInput(const AppearanceInput& input) noexcept;

bool resolveAppearance(
    const AppearanceInput& input,
    AppearanceResolvedPixel& out) noexcept;

double encodeSrgb(double linear) noexcept;
double encodePqSt2084(double luminanceNits) noexcept;

const char* toString(Surround surround) noexcept;
const char* toString(TransferFunction transfer) noexcept;

}  // namespace truthraw::free_world_appearance_resolve::v0_7
