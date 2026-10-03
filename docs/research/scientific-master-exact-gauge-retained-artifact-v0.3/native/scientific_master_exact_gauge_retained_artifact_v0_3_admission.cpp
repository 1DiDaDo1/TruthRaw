#include "scientific_master_exact_gauge_retained_artifact_v0_3.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace truthraw::scientific_master_exact_gauge_retained_artifact::v0_3 {
namespace {

bool requested_retained_bytes(
    const DngMetadata& metadata,
    std::uint64_t& sampleUpperBound,
    std::size_t& byteUpperBound) noexcept {
    if (metadata.width <= 1 || metadata.height <= 1) return false;

    const int borderX = std::min(metadata.width / 2 - 1,
        std::max(0, static_cast<int>(std::floor(
            static_cast<double>(metadata.width) * smsb2::kSelfGaugeBorderFraction))));
    const int borderY = std::min(metadata.height / 2 - 1,
        std::max(0, static_cast<int>(std::floor(
            static_cast<double>(metadata.height) * smsb2::kSelfGaugeBorderFraction))));

    const std::uint64_t width = static_cast<std::uint64_t>(metadata.width - 2 * borderX);
    const std::uint64_t height = static_cast<std::uint64_t>(metadata.height - 2 * borderY);
    if (width != 0u && height > std::numeric_limits<std::uint64_t>::max() / width) {
        return false;
    }
    sampleUpperBound = width * height;
    if (sampleUpperBound >
        static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max() /
                                   sizeof(std::uint32_t))) {
        return false;
    }
    byteUpperBound = static_cast<std::size_t>(sampleUpperBound) * sizeof(std::uint32_t);
    return true;
}

void prepare_forced_fallback_diagnostics(
    const Options& options,
    const ArtifactOptions& artifactOptions,
    std::uint64_t sampleUpperBound,
    std::size_t byteUpperBound,
    Diagnostics& diagnostics) noexcept {
    diagnostics = {};
    diagnostics.routeUsed = RouteUsed::CanonicalV02Fallback;
    diagnostics.fallbackReason = FallbackReason::RetainedArtifactBudgetInsufficient;
    diagnostics.geometricEligibleUpperBound = sampleUpperBound;
    diagnostics.retainedBytesRequested = byteUpperBound;
    diagnostics.retainedArtifactBudgetBytes =
        artifactOptions.retainedArtifactBudgetBytes;
    diagnostics.callerBudgetBytes = options.memoryBudgetBytes;
    diagnostics.optimizationApplied = false;
}

void finish_fallback_diagnostics(
    const Status& fallback,
    const Result& result,
    Diagnostics& diagnostics) noexcept {
    diagnostics.stage2GaugeScanPassesActuallyUsed =
        fallback ? result.stage2GaugeScanPasses : 0u;
    diagnostics.pass2Stage2TileReadsAvoided = 0u;
    diagnostics.eligibleRetainedSamples = 0u;
    diagnostics.retainedBytesReserved = 0u;
    diagnostics.retainedBytesUsed = 0u;
}

}  // namespace

Status bind_scientific_master_retained_exact_gauge(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    const ArtifactOptions& artifactOptions,
    Result& out,
    Diagnostics& diagnostics) noexcept {
    std::uint64_t sampleUpperBound = 0u;
    std::size_t byteUpperBound = 0u;
    if (artifactOptions.retainedArtifactBudgetBytes != 0u &&
        requested_retained_bytes(
            source.metadata(), sampleUpperBound, byteUpperBound) &&
        byteUpperBound > artifactOptions.retainedArtifactBudgetBytes) {
        prepare_forced_fallback_diagnostics(
            options, artifactOptions, sampleUpperBound, byteUpperBound, diagnostics);
        Result fallbackResult{};
        const auto fallback = smsb2::bind_scientific_master_streaming(
            source, reconstruction, options, fallbackResult);
        finish_fallback_diagnostics(fallback, fallbackResult, diagnostics);
        if (fallback) out = fallbackResult;
        return fallback;
    }

    const auto status = bind_scientific_master_retained_exact_gauge(
        source, reconstruction, options, out, diagnostics);
    diagnostics.retainedArtifactBudgetBytes =
        artifactOptions.retainedArtifactBudgetBytes;
    return status;
}

Status bind_scientific_master_retained_exact_gauge_observed(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    const ArtifactOptions& artifactOptions,
    ICanonicalTileObserver& observer,
    Result& out,
    Diagnostics& diagnostics) noexcept {
    std::uint64_t sampleUpperBound = 0u;
    std::size_t byteUpperBound = 0u;
    if (artifactOptions.retainedArtifactBudgetBytes != 0u &&
        requested_retained_bytes(
            source.metadata(), sampleUpperBound, byteUpperBound) &&
        byteUpperBound > artifactOptions.retainedArtifactBudgetBytes) {
        prepare_forced_fallback_diagnostics(
            options, artifactOptions, sampleUpperBound, byteUpperBound, diagnostics);
        Result fallbackResult{};
        const auto fallback = smsb2::bind_scientific_master_streaming_observed(
            source, reconstruction, options, observer, fallbackResult);
        finish_fallback_diagnostics(fallback, fallbackResult, diagnostics);
        if (fallback) out = fallbackResult;
        return fallback;
    }

    const auto status = bind_scientific_master_retained_exact_gauge_observed(
        source, reconstruction, options, observer, out, diagnostics);
    diagnostics.retainedArtifactBudgetBytes =
        artifactOptions.retainedArtifactBudgetBytes;
    return status;
}

}  // namespace truthraw::scientific_master_exact_gauge_retained_artifact::v0_3
