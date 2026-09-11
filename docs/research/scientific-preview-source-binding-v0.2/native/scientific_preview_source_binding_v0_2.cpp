#include "scientific_preview_source_binding_v0_2.h"

#include <cmath>
#include <string>

namespace truthraw::scientific_preview_binding_v0_2 {
namespace {

using scientific_preview_binding_v0_1::ColorBindingAuthority;

std::string source_id_from_digest(const std::array<std::uint8_t, 32>& digest) {
    static constexpr char hex[] = "0123456789abcdef";
    std::string out = "sha256:";
    out.reserve(71);
    for (const auto b : digest) {
        out.push_back(hex[b >> 4]);
        out.push_back(hex[b & 0x0f]);
    }
    return out;
}

bool finite_matrix(const std::array<float, 9>& matrix) noexcept {
    for (const float v : matrix) if (!std::isfinite(v)) return false;
    return true;
}

ColorClaimScope scope_for(ColorBindingAuthority authority) noexcept {
    if (authority == ColorBindingAuthority::IndependentCalibration)
        return ColorClaimScope::IndependentlyCalibratedPreview;
    if (authority == ColorBindingAuthority::SourceMetadataBound ||
        authority == ColorBindingAuthority::GatehouseCertifiedMetadata)
        return ColorClaimScope::SourceBoundPreview;
    return ColorClaimScope::None;
}

} // namespace

BindingStatus prepare_scientific_color_source(
    const SourceSeal& source,
    const ScientificColorBindingRecord& color,
    PreparedScientificPreviewSource& out) noexcept {
    if (source.byteLength == 0 ||
        !scientific_preview_binding_v0_1::is_canonical_source_evidence_id(source.sourceEvidenceId) ||
        source.sourceEvidenceId != source_id_from_digest(source.sha256)) {
        return BindingStatus::error(BindingStatusCode::InvalidSourceSeal,
                                    "pre-master source preparation requires a canonical non-empty SHA-256 seal");
    }

    const ColorClaimScope eventualScope = scope_for(color.authority);
    if (!color.validated || eventualScope == ColorClaimScope::None) {
        return BindingStatus::error(BindingStatusCode::UnauthorizedColorBinding,
                                    "unverified/preview-sentinel color cannot prepare Main-House scientific compute");
    }
    if (color.sourceEvidenceId != source.sourceEvidenceId) {
        return BindingStatus::error(BindingStatusCode::BindingSourceMismatch,
                                    "color record belongs to different source evidence");
    }
    if (color.bindingId.empty() || !finite_matrix(color.cameraToXyzD50)) {
        return BindingStatus::error(BindingStatusCode::InvalidMatrix,
                                    "source-bound color binding ID/matrix is invalid");
    }
    if (color.physicalFrameCount != 1u || color.independentEvidenceCount != 1u) {
        return BindingStatus::error(BindingStatusCode::EvidenceInvariantViolation,
                                    "pre-master preparation cannot alter frame/evidence counts");
    }

    PreparedScientificPreviewSource prepared;
    prepared.source = source;
    prepared.color = color;
    prepared.eventualClaimScope = eventualScope;
    prepared.mainHouseComputeAllowed = true;
    prepared.previewReleaseAllowed = false;
    prepared.scientificClaimAllowed = false;
    prepared.physicalFrameCount = 1;
    prepared.independentEvidenceCount = 1;
    prepared.tileNativeOptions.sourceEvidenceId = source.sourceEvidenceId;
    prepared.tileNativeOptions.color.valid = true;
    prepared.tileNativeOptions.color.bindingId = color.bindingId;
    prepared.tileNativeOptions.color.cameraToXyzD50 = color.cameraToXyzD50;
    out = prepared;
    return BindingStatus::ok();
}

BindingStatus finalize_scientific_color_lineage(
    const PreparedScientificPreviewSource& prepared,
    const technical_backplane::v0_1::State& backplane,
    ScientificPreviewAdmission& out) noexcept {
    if (!prepared.mainHouseComputeAllowed || prepared.previewReleaseAllowed || prepared.scientificClaimAllowed ||
        prepared.physicalFrameCount != 1u || prepared.independentEvidenceCount != 1u) {
        return BindingStatus::error(BindingStatusCode::EvidenceInvariantViolation,
                                    "prepared source is not in the required pre-master state");
    }

    ScientificPreviewAdmission finalized;
    const auto status = scientific_preview_binding_v0_1::admit_scientific_color_preview(
        prepared.source, prepared.color, backplane, finalized);
    if (!status) return status;

    if (finalized.claimScope != prepared.eventualClaimScope ||
        finalized.tileNativeOptions.sourceEvidenceId != prepared.tileNativeOptions.sourceEvidenceId ||
        finalized.tileNativeOptions.color.bindingId != prepared.tileNativeOptions.color.bindingId ||
        finalized.tileNativeOptions.color.cameraToXyzD50 != prepared.tileNativeOptions.color.cameraToXyzD50) {
        return BindingStatus::error(BindingStatusCode::BindingSourceMismatch,
                                    "post-master admission differs from prepared source/color identity");
    }

    out = finalized;
    return BindingStatus::ok();
}

} // namespace truthraw::scientific_preview_binding_v0_2
