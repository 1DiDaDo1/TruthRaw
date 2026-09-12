#include "dng_compatibility_projection_v0_2.h"

#include <cmath>

namespace truthraw::dng_compatibility_projection::v0_2 {
namespace {

bool valid_source_white(const ProjectionMetadata& metadata) noexcept {
    return std::isfinite(metadata.sourceResolvedWhiteX) &&
           std::isfinite(metadata.sourceResolvedWhiteY) &&
           metadata.sourceResolvedWhiteX > 0.0 &&
           metadata.sourceResolvedWhiteY > 0.0 &&
           metadata.sourceResolvedWhiteX + metadata.sourceResolvedWhiteY < 1.0;
}

v0_1::ProjectionMetadata make_v01_metadata(
    const ProjectionMetadata& metadata) {
    v0_1::ProjectionMetadata out{};
    out.sourceSeal = metadata.sourceSeal;
    out.color = metadata.color;
    out.expectedScientificMasterHash = metadata.expectedScientificMasterHash;

    // color.cameraToXyzD50 is already the finalized as-shot transform into
    // XYZ D50. v0.1 writes inverse(cameraToXyzD50) as ColorMatrix1. Advertising
    // D50 here makes the DNG no-ForwardMatrix chromatic adaptation D50->D50,
    // i.e. identity, instead of adapting the already-adapted matrix again.
    out.asShotWhiteX = kCompatibilityD50X;
    out.asShotWhiteY = kCompatibilityD50Y;
    out.sourceDisplayName = metadata.sourceDisplayName;
    return out;
}

void copy_result(ProjectionRole role,
                 const v0_1::Result& writer,
                 Result& out) noexcept {
    out = {};
    out.role = role;
    out.colorEncoding = ColorEncoding::FixedAsShotCameraToXyzD50;
    out.writer = writer;
    out.fixedD50CompatibilityWhite = writer.scientificMasterMatched;

    // v0.1 private data binds source/master/color authority but does not yet
    // serialize the source-resolved xy/CCT. Keep this false rather than
    // claiming provenance bytes that are not present in the DNG itself.
    out.sourceResolvedWhiteRetainedAsProvenance = false;
    out.projectionIsEvidence = writer.projectionIsEvidence;
    out.colorAuthorityPromoted = writer.colorAuthorityPromoted;
}

Status invalid_source_white_status() {
    return Status::error(
        StatusCode::InvalidWhitePoint,
        "resolved source white must be finite/valid provenance before fixed-D50 projection");
}

} // namespace

Status write_linear_scientific_master_dng(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const ProjectionMetadata& metadata,
    ISequentialByteSink& sink,
    Result& out) noexcept {
    out = {};
    out.role = ProjectionRole::LinearScientificMaster;
    if (!valid_source_white(metadata)) return invalid_source_white_status();

    const auto v1Metadata = make_v01_metadata(metadata);
    v0_1::Result writer{};
    const auto status = v0_1::write_linear_scientific_master_dng(
        source, reconstruction, v1Metadata, sink, writer);
    copy_result(ProjectionRole::LinearScientificMaster, writer, out);
    return status;
}

Status write_reconstructed_cfa_dng(
    streaming_v0_1::IRawTileSource& source,
    ResearchEdgeAwareMeasuredPreservingReconstruction& reconstruction,
    const ProjectionMetadata& metadata,
    ISequentialByteSink& sink,
    Result& out) noexcept {
    out = {};
    out.role = ProjectionRole::ReconstructedCfa;
    if (!valid_source_white(metadata)) return invalid_source_white_status();

    const auto v1Metadata = make_v01_metadata(metadata);
    v0_1::Result writer{};
    const auto status = v0_1::write_measured_preserving_cfa_dng(
        source, reconstruction, v1Metadata, sink, writer);
    copy_result(ProjectionRole::ReconstructedCfa, writer, out);
    return status;
}

const char* role_name(ProjectionRole role) noexcept {
    switch (role) {
        case ProjectionRole::LinearScientificMaster:
            return "LINEAR_SCIENTIFIC_MASTER_COMPATIBILITY_PROJECTION";
        case ProjectionRole::ReconstructedCfa:
            return "RECONSTRUCTED_CFA_PROJECTION";
    }
    return "UNKNOWN_PROJECTION";
}

const char* color_encoding_name(ColorEncoding encoding) noexcept {
    switch (encoding) {
        case ColorEncoding::FixedAsShotCameraToXyzD50:
            return "FIXED_AS_SHOT_CAMERA_TO_XYZ_D50";
    }
    return "UNKNOWN_COLOR_ENCODING";
}

} // namespace truthraw::dng_compatibility_projection::v0_2
