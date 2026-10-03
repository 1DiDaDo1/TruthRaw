#include "scientific_master_exact_gauge_retained_artifact_v0_3.h"

#include "full_frame_streaming_v0_1_internal.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

namespace truthraw::scientific_master_exact_gauge_retained_artifact::v0_3 {
namespace {

using streaming_v0_1::TileRect;
using streaming_v0_1::detail::Workspace;
using streaming_v0_1::detail::fill_stage2;
using streaming_v0_1::detail::vector_bytes;

constexpr std::size_t kRadixBucketCount = smsb2::kRadix16BucketCount;
constexpr std::size_t kRadixAuxiliaryBytes = smsb2::kMaximumRadixAuxiliaryBytes;
constexpr std::size_t kFloat32Bytes = sizeof(std::uint32_t);

struct Rank16 final {
    std::uint16_t high = 0u;
    std::uint64_t rankWithinHigh = 0u;
};

std::uint32_t float_bits(float value) noexcept {
    std::uint32_t bits = 0u;
    static_assert(sizeof(bits) == sizeof(value),
                  "retained exact gauge requires IEEE-size float32 storage");
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

float float_from_bits(std::uint32_t bits) noexcept {
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

bool safe_add(std::size_t a, std::size_t b, std::size_t& out) noexcept {
    if (a > std::numeric_limits<std::size_t>::max() - b) return false;
    out = a + b;
    return true;
}

bool safe_mul(std::size_t a, std::size_t b, std::size_t& out) noexcept {
    if (a != 0u && b > std::numeric_limits<std::size_t>::max() / a) return false;
    out = a * b;
    return true;
}

Status fallback_request(Diagnostics& diagnostics,
                        FallbackReason reason,
                        const char* message) {
    diagnostics.fallbackReason = reason;
    return Status::error(StatusCode::BudgetExceeded, message);
}

bool is_eligible_sample(
    const DngMetadata& metadata,
    const TileRect& tile,
    const Workspace& workspace,
    int x,
    int y,
    int borderX,
    int borderY,
    float& stage2Out) noexcept {
    if (x < borderX || x >= metadata.width - borderX ||
        y < borderY || y >= metadata.height - borderY) {
        return false;
    }

    const int tileWidth = tile.hx1 - tile.hx0;
    const int localX = x - tile.hx0;
    const int localY = y - tile.hy0;
    const std::size_t index =
        static_cast<std::size_t>(localY) * static_cast<std::size_t>(tileWidth) +
        static_cast<std::size_t>(localX);
    if (index >= workspace.stage2.size() || index >= workspace.raw.size()) return false;
    if (static_cast<float>(workspace.raw[index]) >= metadata.whiteLevel) return false;

    const float value = workspace.stage2[index];
    if (!(value > 0.0f) || !std::isfinite(value)) return false;
    stage2Out = value;
    return true;
}

template <typename Fn>
Status for_each_canonical_tile(
    streaming_v0_1::IRawTileSource& source,
    int halo,
    Workspace& workspace,
    Fn&& fn) noexcept {
    const auto& metadata = source.metadata();
    for (int y0 = 0; y0 < metadata.height; y0 += smsb2::kCanonicalCore) {
        const int y1 = std::min(metadata.height, y0 + smsb2::kCanonicalCore);
        for (int x0 = 0; x0 < metadata.width; x0 += smsb2::kCanonicalCore) {
            const int x1 = std::min(metadata.width, x0 + smsb2::kCanonicalCore);
            TileRect tile{};
            tile.x0 = x0;
            tile.y0 = y0;
            tile.x1 = x1;
            tile.y1 = y1;
            tile.hx0 = std::max(0, x0 - halo);
            tile.hy0 = std::max(0, y0 - halo);
            tile.hx1 = std::min(metadata.width, x1 + halo);
            tile.hy1 = std::min(metadata.height, y1 + halo);

            const auto fill = fill_stage2(source, tile, workspace);
            if (!fill) {
                return Status::error(StatusCode::SourceFailed,
                                     "Stage-2 source read failed: " + fill.message);
            }
            const auto callback = fn(tile, workspace);
            if (!callback) return callback;
        }
    }
    return Status::ok();
}

bool select_bucket16(
    std::uint64_t& rank,
    const std::uint64_t* histogram,
    std::uint16_t& bucketOut) noexcept {
    std::uint64_t before = 0u;
    for (std::uint32_t bucket = 0u; bucket < kRadixBucketCount; ++bucket) {
        const std::uint64_t count = histogram[static_cast<std::size_t>(bucket)];
        if (rank < before + count) {
            rank -= before;
            bucketOut = static_cast<std::uint16_t>(bucket);
            return true;
        }
        before += count;
    }
    return false;
}

bool derive_geometry_and_requested_bytes(
    const DngMetadata& metadata,
    int borderX,
    int borderY,
    Diagnostics& diagnostics) noexcept {
    const std::uint64_t interiorWidth =
        static_cast<std::uint64_t>(metadata.width - 2 * borderX);
    const std::uint64_t interiorHeight =
        static_cast<std::uint64_t>(metadata.height - 2 * borderY);
    if (interiorWidth != 0u &&
        interiorHeight > std::numeric_limits<std::uint64_t>::max() / interiorWidth) {
        diagnostics.fallbackReason = FallbackReason::GeometricBoundOverflow;
        return false;
    }
    diagnostics.geometricEligibleUpperBound = interiorWidth * interiorHeight;
    if (diagnostics.geometricEligibleUpperBound >
        static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max() / kFloat32Bytes)) {
        diagnostics.fallbackReason = FallbackReason::RetainedByteBoundOverflow;
        return false;
    }
    diagnostics.retainedBytesRequested =
        static_cast<std::size_t>(diagnostics.geometricEligibleUpperBound) * kFloat32Bytes;
    return true;
}

bool compute_workspace_shape(
    const DngMetadata& metadata,
    int halo,
    std::size_t& haloSamples,
    std::size_t& coreSamples,
    std::size_t& maxTileWidth,
    std::size_t& maxTileHeight,
    std::size_t& maxCoreWidth,
    std::size_t& maxCoreHeight) noexcept {
    if (halo < 0) return false;
    maxTileWidth = static_cast<std::size_t>(
        std::min(metadata.width, smsb2::kCanonicalCore + 2 * halo));
    maxTileHeight = static_cast<std::size_t>(
        std::min(metadata.height, smsb2::kCanonicalCore + 2 * halo));
    maxCoreWidth = static_cast<std::size_t>(
        std::min(metadata.width, smsb2::kCanonicalCore));
    maxCoreHeight = static_cast<std::size_t>(
        std::min(metadata.height, smsb2::kCanonicalCore));
    return safe_mul(maxTileWidth, maxTileHeight, haloSamples) &&
           safe_mul(maxCoreWidth, maxCoreHeight, coreSamples);
}

bool estimate_candidate_static_resident(
    const DngMetadata& metadata,
    int halo,
    std::size_t sourceBytes,
    std::size_t digestBytes,
    std::size_t observerOwnedBytes,
    bool hasObserver,
    const Diagnostics& diagnostics,
    std::size_t& out) noexcept {
    std::size_t haloSamples = 0u;
    std::size_t coreSamples = 0u;
    std::size_t maxTileWidth = 0u;
    std::size_t maxTileHeight = 0u;
    std::size_t maxCoreWidth = 0u;
    std::size_t maxCoreHeight = 0u;
    if (!compute_workspace_shape(metadata, halo, haloSamples, coreSamples,
                                 maxTileWidth, maxTileHeight,
                                 maxCoreWidth, maxCoreHeight)) {
        return false;
    }
    (void)maxTileWidth;
    (void)maxCoreWidth;

    std::size_t workspaceBytes = 0u;
    const auto add_term = [&](std::size_t count, std::size_t elementBytes) -> bool {
        std::size_t bytes = 0u;
        if (!safe_mul(count, elementBytes, bytes)) return false;
        return safe_add(workspaceBytes, bytes, workspaceBytes);
    };

    if (!add_term(haloSamples, sizeof(std::uint16_t))) return false;
    if (metadata.hasGainField && !add_term(haloSamples, sizeof(float))) return false;
    if (metadata.hasResidualBlack) {
        if (!add_term(maxTileHeight, sizeof(float)) ||
            !add_term(maxTileWidth, sizeof(float))) return false;
    }
    if (!add_term(haloSamples, sizeof(float))) return false;
    std::size_t cameraSamples = 0u;
    if (!safe_mul(coreSamples, 3u, cameraSamples) ||
        !add_term(cameraSamples, sizeof(float))) {
        return false;
    }
    if (hasObserver && !add_term(coreSamples, sizeof(std::uint16_t))) return false;

    std::size_t resident = sourceBytes;
    if (!safe_add(resident, digestBytes, resident) ||
        !safe_add(resident, observerOwnedBytes, resident) ||
        !safe_add(resident, workspaceBytes, resident) ||
        !safe_add(resident, diagnostics.retainedBytesRequested, resident) ||
        !safe_add(resident, kRadixAuxiliaryBytes, resident)) {
        return false;
    }
    out = resident;
    return true;
}

bool exact_reserved_resident(
    const streaming_v0_1::IRawTileSource& source,
    const Workspace& workspace,
    const scientific_master_digest::v0_1::ScientificMasterDigestAccumulator& digest,
    const std::vector<std::uint32_t>& retainedBits,
    const std::vector<std::uint16_t>& observerRaw,
    std::size_t observerOwnedBytes,
    bool hasObserver,
    std::size_t& out) noexcept {
    std::size_t retainedBytes = 0u;
    std::size_t observerRawBytes = 0u;
    if (!safe_mul(retainedBits.capacity(), sizeof(std::uint32_t), retainedBytes)) return false;
    if (hasObserver &&
        !safe_mul(observerRaw.capacity(), sizeof(std::uint16_t), observerRawBytes)) {
        return false;
    }

    std::size_t resident = source.residentBytesUpperBound();
    if (!safe_add(resident, vector_bytes(workspace), resident) ||
        !safe_add(resident, digest.metrics().residentBytesUpperBound, resident) ||
        !safe_add(resident, observerOwnedBytes, resident) ||
        !safe_add(resident, observerRawBytes, resident) ||
        !safe_add(resident, retainedBytes, resident) ||
        !safe_add(resident, kRadixAuxiliaryBytes, resident)) {
        return false;
    }
    out = resident;
    return true;
}

Status candidate_attempt(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    ICanonicalTileObserver* observer,
    Result& out,
    Diagnostics& diagnostics) noexcept {
    out = {};
    const auto& metadata = source.metadata();
    if (metadata.width <= 1 || metadata.height <= 1) {
        return Status::error(StatusCode::InvalidArgument,
                             "invalid scientific source dimensions");
    }
    if (!(metadata.whiteLevel > 0.0f) || !std::isfinite(metadata.whiteLevel)) {
        return Status::error(StatusCode::InvalidArgument,
                             "invalid source WhiteLevel");
    }

    const int halo = reconstruction.requiredHalo();
    if (halo < 0) {
        return Status::error(StatusCode::InvalidArgument,
                             "reconstruction backend returned negative halo");
    }

    const int borderX = std::min(metadata.width / 2 - 1,
        std::max(0, static_cast<int>(std::floor(
            static_cast<double>(metadata.width) * smsb2::kSelfGaugeBorderFraction))));
    const int borderY = std::min(metadata.height / 2 - 1,
        std::max(0, static_cast<int>(std::floor(
            static_cast<double>(metadata.height) * smsb2::kSelfGaugeBorderFraction))));

    if (!derive_geometry_and_requested_bytes(metadata, borderX, borderY, diagnostics)) {
        return Status::error(StatusCode::BudgetExceeded,
                             "retained exact gauge geometric bound cannot be represented");
    }

    scientific_master_digest::v0_1::ScientificMasterDigestAccumulator digest(
        static_cast<std::uint32_t>(metadata.width),
        static_cast<std::uint32_t>(metadata.height));
    if (!digest.valid()) {
        return Status::error(StatusCode::DigestFailed,
                             "Scientific Master digest initialization failed: " + digest.error());
    }

    const bool hasObserver = observer != nullptr;
    const std::size_t observerOwnedBytes =
        hasObserver ? observer->residentBytesUpperBound() : 0u;

    std::size_t staticResidentEstimate = 0u;
    if (!estimate_candidate_static_resident(
            metadata,
            halo,
            source.residentBytesUpperBound(),
            digest.metrics().residentBytesUpperBound,
            observerOwnedBytes,
            hasObserver,
            diagnostics,
            staticResidentEstimate)) {
        return fallback_request(
            diagnostics,
            FallbackReason::RetainedByteBoundOverflow,
            "retained exact gauge resident bound overflow before admission");
    }
    if (options.memoryBudgetBytes != 0u &&
        staticResidentEstimate > options.memoryBudgetBytes) {
        return fallback_request(
            diagnostics,
            FallbackReason::MemoryBudgetInsufficient,
            "retained exact gauge does not fit caller resident budget before admission");
    }

    std::unique_ptr<std::uint64_t[]> lowerHistogram;
    std::unique_ptr<std::uint64_t[]> upperHistogram;
    std::vector<std::uint32_t> retainedBits;
    Workspace workspace{};
    std::vector<std::uint16_t> observerRaw{};

    std::size_t haloSamples = 0u;
    std::size_t coreSamples = 0u;
    std::size_t maxTileWidth = 0u;
    std::size_t maxTileHeight = 0u;
    std::size_t maxCoreWidth = 0u;
    std::size_t maxCoreHeight = 0u;
    if (!compute_workspace_shape(metadata, halo, haloSamples, coreSamples,
                                 maxTileWidth, maxTileHeight,
                                 maxCoreWidth, maxCoreHeight)) {
        return fallback_request(
            diagnostics,
            FallbackReason::RetainedByteBoundOverflow,
            "retained exact gauge workspace bound overflow before admission");
    }
    (void)maxTileWidth;
    (void)maxCoreWidth;

    try {
        lowerHistogram.reset(new std::uint64_t[kRadixBucketCount]());
        upperHistogram.reset(new std::uint64_t[kRadixBucketCount]());
        retainedBits.reserve(
            static_cast<std::size_t>(diagnostics.geometricEligibleUpperBound));
        workspace.raw.reserve(haloSamples);
        if (metadata.hasGainField) workspace.gain.reserve(haloSamples);
        if (metadata.hasResidualBlack) {
            workspace.rowBias.reserve(maxTileHeight);
            workspace.colBias.reserve(maxTileWidth);
        }
        workspace.stage2.reserve(haloSamples);
        std::size_t cameraSamples = 0u;
        if (!safe_mul(coreSamples, 3u, cameraSamples)) {
            return fallback_request(
                diagnostics,
                FallbackReason::RetainedByteBoundOverflow,
                "retained exact gauge camera workspace bound overflow before admission");
        }
        workspace.cam.reserve(cameraSamples);
        if (hasObserver) observerRaw.reserve(coreSamples);
    } catch (const std::bad_alloc&) {
        return fallback_request(
            diagnostics,
            FallbackReason::ArtifactAllocationFailed,
            "retained exact gauge allocation failed before admission");
    } catch (const std::length_error&) {
        return fallback_request(
            diagnostics,
            FallbackReason::ArtifactAllocationFailed,
            "retained exact gauge allocation length rejected before admission");
    }

    diagnostics.retainedBytesReserved = retainedBits.capacity() * kFloat32Bytes;

    std::size_t admittedResidentUpperBound = 0u;
    if (!exact_reserved_resident(source, workspace, digest, retainedBits,
                                 observerRaw, observerOwnedBytes, hasObserver,
                                 admittedResidentUpperBound)) {
        return fallback_request(
            diagnostics,
            FallbackReason::RetainedByteBoundOverflow,
            "retained exact gauge exact reserved resident accounting overflow before admission");
    }
    if (options.memoryBudgetBytes != 0u &&
        admittedResidentUpperBound > options.memoryBudgetBytes) {
        return fallback_request(
            diagnostics,
            FallbackReason::MemoryBudgetInsufficient,
            "retained exact gauge reserved capacity exceeds caller budget before admission");
    }

    // From this point onward the specialized route is committed. No condition
    // may set fallbackReason. Any contract breach is a hard fail-closed error,
    // preventing source/reconstruction/observer replay through v0.2.
    diagnostics.fallbackReason = FallbackReason::None;
    diagnostics.candidateLogicalResidentUpperBound = admittedResidentUpperBound;
    std::size_t workspacePeak = vector_bytes(workspace);
    std::size_t residentPeak = admittedResidentUpperBound;

    const auto verify_post_admission_contract = [&]() -> Status {
        if (hasObserver && observer->residentBytesUpperBound() > observerOwnedBytes) {
            return Status::error(
                StatusCode::BudgetExceeded,
                "canonical observer resident upper-bound contract increased after v0.3 admission");
        }
        std::size_t currentResident = 0u;
        if (!exact_reserved_resident(source, workspace, digest, retainedBits,
                                     observerRaw, observerOwnedBytes, hasObserver,
                                     currentResident)) {
            return Status::error(
                StatusCode::BudgetExceeded,
                "retained exact gauge resident accounting overflow after admission");
        }
        residentPeak = std::max(residentPeak, currentResident);
        diagnostics.candidateLogicalResidentUpperBound = residentPeak;
        workspacePeak = std::max(workspacePeak, vector_bytes(workspace));
        if (options.memoryBudgetBytes != 0u &&
            currentResident > options.memoryBudgetBytes) {
            return Status::error(
                StatusCode::BudgetExceeded,
                "retained exact gauge resident contract exceeded after admission");
        }
        return Status::ok();
    };

    std::size_t masterTiles = 0u;
    const auto firstPass = for_each_canonical_tile(
        source, halo, workspace,
        [&](const TileRect& tile, Workspace& w) -> Status {
            const int tileWidth = tile.hx1 - tile.hx0;
            const int coreWidth = tile.x1 - tile.x0;
            const int coreHeight = tile.y1 - tile.y0;
            const std::size_t coreSampleCount =
                static_cast<std::size_t>(coreWidth) * static_cast<std::size_t>(coreHeight);
            w.cam.resize(3u * coreSampleCount);

            const auto reconstructionStatus = reconstruction.reconstructTile(
                w.stage2.data(), tileWidth, tile.hy1 - tile.hy0,
                tile.hx0, tile.hy0,
                tile.x0, tile.y0, coreWidth, coreHeight,
                metadata.cfa, w.cam.data());
            if (!reconstructionStatus) {
                return Status::error(StatusCode::ReconstructionFailed,
                                     "camera-native reconstruction failed: " +
                                         reconstructionStatus.message);
            }

            scientific_master_digest::v0_1::TileView digestTile{};
            digestTile.x = static_cast<std::uint32_t>(tile.x0);
            digestTile.y = static_cast<std::uint32_t>(tile.y0);
            digestTile.width = static_cast<std::uint32_t>(coreWidth);
            digestTile.height = static_cast<std::uint32_t>(coreHeight);
            digestTile.rgb = w.cam.data();
            digestTile.rowStrideSamples = static_cast<std::size_t>(coreWidth) * 3u;
            if (!digest.add_tile(digestTile)) {
                return Status::error(StatusCode::DigestFailed,
                                     "Scientific Master digest tile rejected: " + digest.error());
            }

            if (hasObserver) {
                observerRaw.resize(coreSampleCount);
                const int coreOffsetX = tile.x0 - tile.hx0;
                const int coreOffsetY = tile.y0 - tile.hy0;
                for (int yy = 0; yy < coreHeight; ++yy) {
                    const std::size_t sourceOffset =
                        static_cast<std::size_t>(coreOffsetY + yy) *
                            static_cast<std::size_t>(tileWidth) +
                        static_cast<std::size_t>(coreOffsetX);
                    const std::size_t destinationOffset =
                        static_cast<std::size_t>(yy) *
                        static_cast<std::size_t>(coreWidth);
                    std::copy_n(
                        w.raw.data() + sourceOffset,
                        static_cast<std::size_t>(coreWidth),
                        observerRaw.data() + destinationOffset);
                }
                if (!observer->observeCanonicalTile(
                        static_cast<std::uint32_t>(tile.x0),
                        static_cast<std::uint32_t>(tile.y0),
                        static_cast<std::uint32_t>(coreWidth),
                        static_cast<std::uint32_t>(coreHeight),
                        observerRaw.data(), observerRaw.size(),
                        w.cam.data(), w.cam.size())) {
                    return Status::error(
                        StatusCode::DigestFailed,
                        "canonical Scientific Master observer rejected tile");
                }
            }

            for (int y = tile.y0; y < tile.y1; ++y) {
                for (int x = tile.x0; x < tile.x1; ++x) {
                    float stage2 = 0.0f;
                    if (!is_eligible_sample(metadata, tile, w, x, y,
                                            borderX, borderY, stage2)) {
                        continue;
                    }
                    if (retainedBits.size() >= diagnostics.geometricEligibleUpperBound) {
                        return Status::error(
                            StatusCode::BudgetExceeded,
                            "retained exact gauge cardinality violated proven geometric bound after admission");
                    }
                    const std::uint32_t bits = float_bits(stage2);
                    retainedBits.push_back(bits);
                    const std::uint16_t high =
                        static_cast<std::uint16_t>((bits >> 16u) & 0xffffu);
                    ++lowerHistogram[static_cast<std::size_t>(high)];
                }
            }

            ++masterTiles;
            return verify_post_admission_contract();
        });
    if (!firstPass) return firstPass;

    Hash256 masterHash{};
    if (!digest.finalize(masterHash)) {
        return Status::error(StatusCode::DigestFailed,
                             "Scientific Master digest finalization failed: " + digest.error());
    }

    const std::uint64_t eligibleCount =
        static_cast<std::uint64_t>(retainedBits.size());
    if (eligibleCount == 0u) {
        return Status::error(StatusCode::GaugeFailed,
                             "no positive finite uncensored Stage-2 evidence for self gauge");
    }

    Rank16 lower{};
    Rank16 upper{};
    lower.rankWithinHigh = (eligibleCount - 1u) / 2u;
    upper.rankWithinHigh = eligibleCount / 2u;
    if (!select_bucket16(lower.rankWithinHigh, lowerHistogram.get(), lower.high) ||
        !select_bucket16(upper.rankWithinHigh, lowerHistogram.get(), upper.high)) {
        return Status::error(StatusCode::GaugeFailed,
                             "failed selecting high 16 bits of retained exact self-gauge median");
    }

    std::fill_n(lowerHistogram.get(), kRadixBucketCount, 0u);
    std::fill_n(upperHistogram.get(), kRadixBucketCount, 0u);
    for (const std::uint32_t bits : retainedBits) {
        const std::uint16_t high =
            static_cast<std::uint16_t>((bits >> 16u) & 0xffffu);
        const std::uint16_t low =
            static_cast<std::uint16_t>(bits & 0xffffu);
        if (high == lower.high) ++lowerHistogram[static_cast<std::size_t>(low)];
        if (high == upper.high) ++upperHistogram[static_cast<std::size_t>(low)];
    }

    std::uint16_t lowerLow = 0u;
    std::uint16_t upperLow = 0u;
    if (!select_bucket16(lower.rankWithinHigh, lowerHistogram.get(), lowerLow) ||
        !select_bucket16(upper.rankWithinHigh, upperHistogram.get(), upperLow)) {
        return Status::error(StatusCode::GaugeFailed,
                             "failed selecting low 16 bits of retained exact self-gauge median");
    }

    const std::uint32_t lowerBits =
        (static_cast<std::uint32_t>(lower.high) << 16u) |
        static_cast<std::uint32_t>(lowerLow);
    const std::uint32_t upperBits =
        (static_cast<std::uint32_t>(upper.high) << 16u) |
        static_cast<std::uint32_t>(upperLow);
    const float lowerValue = float_from_bits(lowerBits);
    const float upperValue = float_from_bits(upperBits);
    if (!(lowerValue > 0.0f) || !(upperValue > 0.0f) ||
        !std::isfinite(lowerValue) || !std::isfinite(upperValue) ||
        lowerValue > upperValue) {
        return Status::error(StatusCode::GaugeFailed,
                             "resolved retained self-gauge median values are invalid");
    }

    const double lowerDouble = static_cast<double>(lowerValue);
    const double upperDouble = static_cast<double>(upperValue);
    const double l0 = lowerDouble + (upperDouble - lowerDouble) * 0.5;
    if (!(l0 > 0.0) || !std::isfinite(l0)) {
        return Status::error(StatusCode::GaugeFailed,
                             "derived retained self-gauge L0 is invalid");
    }

    Result result{};
    result.scientificMasterHash = masterHash;
    result.zeroLineGauge.mode = TruthRangeGaugeModeV02::SelfGauge;
    result.zeroLineGauge.L0 = l0;
    result.zeroLineGauge.gaugeId = "SELF_GAUGE_STAGE2_Q0.500000";
    result.zeroLineGauge.crossSceneComparable = false;
    result.zeroLineGauge.absolutePhysicalUnits = false;
    result.sceneBinding.reconstructionBackend = reconstruction.name();
    result.sceneBinding.sceneScaleId = "TRUTHRANGE_SELF_GAUGE_STAGE2_V0_2";
    result.sceneBinding.gainMapAppliedExactlyOnce = true;
    result.sceneBinding.exposureNormalizedToCommonScene = false;
    result.sceneBinding.gainNormalizedToCommonScene = false;
    result.selfGaugeEligibleSamples = eligibleCount;
    result.masterTilesProcessed = masterTiles;
    result.stage2GaugeScanPasses = 1u;
    result.logicalWorkspacePeakBytes = workspacePeak;
    result.logicalResidentUpperBound = residentPeak;
    result.physicalFrameCount = 1u;
    result.independentEvidenceCount = 1u;

    diagnostics.routeUsed = RouteUsed::RetainedExactFloat32Bits;
    diagnostics.fallbackReason = FallbackReason::None;
    diagnostics.eligibleRetainedSamples = eligibleCount;
    diagnostics.retainedBytesUsed = retainedBits.size() * kFloat32Bytes;
    diagnostics.lowerMedianFloat32Bits = lowerBits;
    diagnostics.upperMedianFloat32Bits = upperBits;
    diagnostics.stage2GaugeScanPassesActuallyUsed = 1u;
    diagnostics.pass2Stage2TileReadsAvoided = masterTiles;
    diagnostics.optimizationApplied = true;
    diagnostics.candidateLogicalResidentUpperBound = residentPeak;

    out = result;
    return Status::ok();
}

Status bind_impl(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    ICanonicalTileObserver* observer,
    Result& out,
    Diagnostics& diagnostics) noexcept {
    diagnostics = {};
    diagnostics.callerBudgetBytes = options.memoryBudgetBytes;

    Result candidateResult{};
    const auto candidate = candidate_attempt(
        source, reconstruction, options, observer, candidateResult, diagnostics);
    if (candidate) {
        out = candidateResult;
        return candidate;
    }

    // Only pre-admission code is allowed to set fallbackReason. Once semantic
    // processing starts, all failures leave fallbackReason=None and propagate.
    if (diagnostics.fallbackReason == FallbackReason::None) return candidate;

    const FallbackReason reason = diagnostics.fallbackReason;
    Result fallbackResult{};
    const auto fallback = observer == nullptr
        ? smsb2::bind_scientific_master_streaming(
              source, reconstruction, options, fallbackResult)
        : smsb2::bind_scientific_master_streaming_observed(
              source, reconstruction, options, *observer, fallbackResult);

    diagnostics.routeUsed = RouteUsed::CanonicalV02Fallback;
    diagnostics.fallbackReason = reason;
    diagnostics.optimizationApplied = false;
    diagnostics.eligibleRetainedSamples = 0u;
    diagnostics.retainedBytesReserved = 0u;
    diagnostics.retainedBytesUsed = 0u;
    diagnostics.stage2GaugeScanPassesActuallyUsed =
        fallback ? fallbackResult.stage2GaugeScanPasses : 0u;
    diagnostics.pass2Stage2TileReadsAvoided = 0u;
    diagnostics.lowerMedianFloat32Bits = 0u;
    diagnostics.upperMedianFloat32Bits = 0u;
    if (fallback) out = fallbackResult;
    return fallback;
}

}  // namespace

Status bind_scientific_master_retained_exact_gauge(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    Result& out,
    Diagnostics& diagnostics) noexcept {
    return bind_impl(source, reconstruction, options, nullptr, out, diagnostics);
}

Status bind_scientific_master_retained_exact_gauge_observed(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    ICanonicalTileObserver& observer,
    Result& out,
    Diagnostics& diagnostics) noexcept {
    return bind_impl(source, reconstruction, options, &observer, out, diagnostics);
}

const char* route_name(RouteUsed route) noexcept {
    switch (route) {
        case RouteUsed::RetainedExactFloat32Bits:
            return "retained_exact_float32_bits_v0.3";
        case RouteUsed::CanonicalV02Fallback:
            return "canonical_v0.2_fallback";
    }
    return "unknown";
}

const char* fallback_reason_name(FallbackReason reason) noexcept {
    switch (reason) {
        case FallbackReason::None: return "none";
        case FallbackReason::GeometricBoundOverflow: return "geometric_bound_overflow";
        case FallbackReason::RetainedByteBoundOverflow: return "retained_byte_bound_overflow";
        case FallbackReason::MemoryBudgetInsufficient: return "memory_budget_insufficient";
        case FallbackReason::ArtifactAllocationFailed: return "artifact_allocation_failed";
        case FallbackReason::ArtifactCardinalityExceeded: return "artifact_cardinality_exceeded";
        case FallbackReason::CandidateResidentBudgetExceeded: return "candidate_resident_budget_exceeded";
    }
    return "unknown";
}

}  // namespace truthraw::scientific_master_exact_gauge_retained_artifact::v0_3
