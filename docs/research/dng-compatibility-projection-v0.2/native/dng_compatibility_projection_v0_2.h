#pragma once

#include "dng_compatibility_projection_v0_1.h"
#include "truthraw/core.h"

#include <cstdint>
#include <string>

namespace truthraw::dng_compatibility_projection::v0_2 {

using Hash256 = v0_1::Hash256;
using StatusCode = v0_1::StatusCode;
using Status = v0_1::Status;
using ISequentialByteSink = v0_1::ISequentialByteSink;

inline constexpr double kCompatibilityD50X = 0.3457;
inline constexpr double kCompatibilityD50Y = 0.3585;

enum class ProjectionRole : std::uint8_t {
    LinearScientificMaster = 0,
    ReconstructedCfa = 1,
};

enum class ColorEncoding : std::uint8_t {
    FixedAsShotCameraToXyzD50 = 0,
};

struct ProjectionMetadata final {
    scientific_preview_binding_v0_1::SourceSeal sourceSeal{};
    scientific_preview_binding_v0_1::ScientificColorBindingRecord color{};
    Hash256 expectedScientificMasterHash{};

    // The real resolved source white remains provenance, not the white point
    // advertised by this compatibility profile. The output DNG intentionally
    // advertises D50 as AsShotWhiteXY because color.cameraToXyzD50 is already
    // the finalized as-shot camera -> XYZ D50 transform. This prevents a DNG
    // reader from applying a second chromatic adaptation to that transform.
    double sourceResolvedWhiteX = 0.0;
    double sourceResolvedWhiteY = 0.0;
    double sourceResolvedWhiteTemperatureK = 0.0;

    std::string sourceDisplayName;
};

struct Result final {
    ProjectionRole role = ProjectionRole::LinearScientificMaster;
    ColorEncoding colorEncoding = ColorEncoding::FixedAsShotCameraToXyzD50;
    v0_1::Result writer{};

    bool fixedD50CompatibilityWhite = false;
    bool sourceResolvedWhiteRetainedAsProvenance = false;
    bool projectionIsEvidence = false;
    bool colorAuthorityPromoted = false;
};

// Writes the exact camera-native reconstructed Scientific Master RGB samples
// through the validated v0.1 bounded writer, but encodes the DNG profile as a
// fixed as-shot D50 compatibility transform. The source's original resolved
// white remains provenance and is not re-applied by the DNG reader.
Status write_linear_scientific_master_dng(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const ProjectionMetadata& metadata,
    ISequentialByteSink& sink,
    Result& out) noexcept;

// Derived CFA compatibility projection. This overload deliberately requires
// TruthRaw's measured-preserving v4.7i reconstruction backend rather than an
// arbitrary IReconstructionBackend. It is still reconstructed/derived output,
// never original sensor-code evidence and never a second truth.
Status write_reconstructed_cfa_dng(
    streaming_v0_1::IRawTileSource& source,
    ResearchEdgeAwareMeasuredPreservingReconstruction& reconstruction,
    const ProjectionMetadata& metadata,
    ISequentialByteSink& sink,
    Result& out) noexcept;

const char* role_name(ProjectionRole role) noexcept;
const char* color_encoding_name(ColorEncoding encoding) noexcept;

} // namespace truthraw::dng_compatibility_projection::v0_2
