#include "scientific_master_streaming_binding_v0_1.h"

#include "full_frame_streaming_v0_1_internal.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>

namespace truthraw::scientific_master_streaming_binding::v0_1 {
namespace {

using streaming_v0_1::StreamStatusCode;
using streaming_v0_1::TileRect;
using streaming_v0_1::detail::Workspace;
using streaming_v0_1::detail::fill_stage2;
using streaming_v0_1::detail::vector_bytes;

struct RankSelector final {
    std::uint32_t prefix = 0u;
    std::uint32_t mask = 0u;
    std::uint64_t rank = 0u;
};

using Histogram256 = std::array<std::uint64_t, 256>;

std::uint32_t float_bits(float value) noexcept {
    std::uint32_t bits = 0u;
    static_assert(sizeof(bits) == sizeof(value), "float32 radix selection requires 32-bit float");
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

float float_from_bits(std::uint32_t bits) noexcept {
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
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
    for (int y0 = 0; y0 < metadata.height; y0 += kCanonicalCore) {
        const int y1 = std::min(metadata.height, y0 + kCanonicalCore);
        for (int x0 = 0; x0 < metadata.width; x0 += kCanonicalCore) {
            const int x1 = std::min(metadata.width, x0 + kCanonicalCore);
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
            const auto callbackStatus = fn(tile, workspace);
            if (!callbackStatus) return callbackStatus;
        }
    }
    return Status::ok();
}

bool select_bucket(RankSelector& selector, const Histogram256& histogram, int byteIndex) noexcept {
    std::uint64_t before = 0u;
    for (std::uint32_t bucket = 0u; bucket < 256u; ++bucket) {
        const std::uint64_t count = histogram[static_cast<std::size_t>(bucket)];
        if (selector.rank < before + count) {
            selector.rank -= before;
            const unsigned shift = static_cast<unsigned>(24 - 8 * byteIndex);
            selector.prefix |= bucket << shift;
            selector.mask |= 0xffu << shift;
            return true;
        }
        before += count;
    }
    return false;
}

bool selector_matches(std::uint32_t bits, const RankSelector& selector) noexcept {
    return (bits & selector.mask) == selector.prefix;
}

std::size_t fixed_auxiliary_bytes() noexcept {
    return 2u * sizeof(Histogram256);
}

Status check_budget(
    const Options& options,
    streaming_v0_1::IRawTileSource& source,
    const Workspace& workspace,
    const scientific_master_digest::v0_1::ScientificMasterDigestAccumulator& digest,
    std::size_t& workspacePeak,
    std::size_t& residentPeak) noexcept {
    workspacePeak = std::max(workspacePeak, vector_bytes(workspace));
    const auto digestMetrics = digest.metrics();
    const std::size_t auxiliary = fixed_auxiliary_bytes();

    bool overflow = false;
    const auto safe_add = [&](std::size_t a, std::size_t b) -> std::size_t {
        if (a > std::numeric_limits<std::size_t>::max() - b) {
            overflow = true;
            return 0u;
        }
        return a + b;
    };

    std::size_t resident = safe_add(source.residentBytesUpperBound(), workspacePeak);
    resident = safe_add(resident, digestMetrics.residentBytesUpperBound);
    resident = safe_add(resident, auxiliary);
    if (overflow) {
        return Status::error(StatusCode::BudgetExceeded,
                             "scientific streaming resident accounting overflow");
    }
    residentPeak = std::max(residentPeak, resident);
    if (options.memoryBudgetBytes != 0u && residentPeak > options.memoryBudgetBytes) {
        return Status::error(StatusCode::BudgetExceeded,
                             "scientific streaming resident bound exceeds caller budget");
    }
    return Status::ok();
}

}  // namespace

Status bind_scientific_master_streaming(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    Result& out) noexcept {
    out = {};
    const auto& metadata = source.metadata();
    if (metadata.width <= 1 || metadata.height <= 1) {
        return Status::error(StatusCode::InvalidArgument, "invalid scientific source dimensions");
    }
    if (!(metadata.whiteLevel > 0.0f) || !std::isfinite(metadata.whiteLevel)) {
        return Status::error(StatusCode::InvalidArgument, "invalid source WhiteLevel");
    }
    const int halo = reconstruction.requiredHalo();
    if (halo < 0) {
        return Status::error(StatusCode::InvalidArgument, "reconstruction backend returned negative halo");
    }

    const int borderX = std::min(metadata.width / 2 - 1,
        std::max(0, static_cast<int>(std::floor(
            static_cast<double>(metadata.width) * kSelfGaugeBorderFraction))));
    const int borderY = std::min(metadata.height / 2 - 1,
        std::max(0, static_cast<int>(std::floor(
            static_cast<double>(metadata.height) * kSelfGaugeBorderFraction))));

    scientific_master_digest::v0_1::ScientificMasterDigestAccumulator digest(
        static_cast<std::uint32_t>(metadata.width),
        static_cast<std::uint32_t>(metadata.height));
    if (!digest.valid()) {
        return Status::error(StatusCode::DigestFailed,
                             "Scientific Master digest initialization failed: " + digest.error());
    }

    Workspace workspace{};
    Histogram256 highByteHistogram{};
    std::uint64_t eligibleCount = 0u;
    std::size_t masterTiles = 0u;
    std::size_t workspacePeak = 0u;
    std::size_t residentPeak = 0u;

    auto firstPass = for_each_canonical_tile(
        source, halo, workspace,
        [&](const TileRect& tile, Workspace& w) -> Status {
            const int tileWidth = tile.hx1 - tile.hx0;
            const int tileHeight = tile.hy1 - tile.hy0;
            const int coreWidth = tile.x1 - tile.x0;
            const int coreHeight = tile.y1 - tile.y0;
            const std::size_t coreSamples =
                static_cast<std::size_t>(coreWidth) * static_cast<std::size_t>(coreHeight);
            w.cam.resize(3u * coreSamples);

            const auto reconstructionStatus = reconstruction.reconstructTile(
                w.stage2.data(), tileWidth, tileHeight,
                tile.hx0, tile.hy0,
                tile.x0, tile.y0, coreWidth, coreHeight,
                metadata.cfa, w.cam.data());
            if (!reconstructionStatus) {
                return Status::error(StatusCode::ReconstructionFailed,
                                     "camera-native reconstruction failed: " + reconstructionStatus.message);
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

            for (int y = tile.y0; y < tile.y1; ++y) {
                for (int x = tile.x0; x < tile.x1; ++x) {
                    float stage2 = 0.0f;
                    if (!is_eligible_sample(metadata, tile, w, x, y, borderX, borderY, stage2)) continue;
                    const std::uint32_t bits = float_bits(stage2);
                    const std::uint8_t high = static_cast<std::uint8_t>((bits >> 24u) & 0xffu);
                    ++highByteHistogram[static_cast<std::size_t>(high)];
                    ++eligibleCount;
                }
            }

            ++masterTiles;
            return check_budget(options, source, w, digest, workspacePeak, residentPeak);
        });
    if (!firstPass) return firstPass;

    Hash256 masterHash{};
    if (!digest.finalize(masterHash)) {
        return Status::error(StatusCode::DigestFailed,
                             "Scientific Master digest finalization failed: " + digest.error());
    }
    if (eligibleCount == 0u) {
        return Status::error(StatusCode::GaugeFailed,
                             "no positive finite uncensored Stage-2 evidence for self gauge");
    }

    RankSelector lower{};
    RankSelector upper{};
    lower.rank = (eligibleCount - 1u) / 2u;
    upper.rank = eligibleCount / 2u;
    if (!select_bucket(lower, highByteHistogram, 0) ||
        !select_bucket(upper, highByteHistogram, 0)) {
        return Status::error(StatusCode::GaugeFailed,
                             "failed selecting high byte of exact self-gauge median");
    }

    for (int byteIndex = 1; byteIndex < 4; ++byteIndex) {
        Histogram256 lowerHistogram{};
        Histogram256 upperHistogram{};

        auto scanStatus = for_each_canonical_tile(
            source, halo, workspace,
            [&](const TileRect& tile, Workspace& w) -> Status {
                for (int y = tile.y0; y < tile.y1; ++y) {
                    for (int x = tile.x0; x < tile.x1; ++x) {
                        float stage2 = 0.0f;
                        if (!is_eligible_sample(metadata, tile, w, x, y, borderX, borderY, stage2)) continue;
                        const std::uint32_t bits = float_bits(stage2);
                        const unsigned shift = static_cast<unsigned>(24 - 8 * byteIndex);
                        const std::uint8_t bucket = static_cast<std::uint8_t>((bits >> shift) & 0xffu);
                        if (selector_matches(bits, lower)) {
                            ++lowerHistogram[static_cast<std::size_t>(bucket)];
                        }
                        if (selector_matches(bits, upper)) {
                            ++upperHistogram[static_cast<std::size_t>(bucket)];
                        }
                    }
                }
                return check_budget(options, source, w, digest, workspacePeak, residentPeak);
            });
        if (!scanStatus) return scanStatus;
        if (!select_bucket(lower, lowerHistogram, byteIndex) ||
            !select_bucket(upper, upperHistogram, byteIndex)) {
            return Status::error(StatusCode::GaugeFailed,
                                 "failed refining exact self-gauge median radix selection");
        }
    }

    if (lower.mask != 0xffffffffu || upper.mask != 0xffffffffu) {
        return Status::error(StatusCode::GaugeFailed,
                             "exact self-gauge median selection did not resolve float32 values");
    }
    const float lowerValue = float_from_bits(lower.prefix);
    const float upperValue = float_from_bits(upper.prefix);
    if (!(lowerValue > 0.0f) || !(upperValue > 0.0f) ||
        !std::isfinite(lowerValue) || !std::isfinite(upperValue) ||
        lowerValue > upperValue) {
        return Status::error(StatusCode::GaugeFailed,
                             "resolved self-gauge median values are invalid");
    }
    const double lowerDouble = static_cast<double>(lowerValue);
    const double upperDouble = static_cast<double>(upperValue);
    const double l0 = lowerDouble + (upperDouble - lowerDouble) * 0.5;
    if (!(l0 > 0.0) || !std::isfinite(l0)) {
        return Status::error(StatusCode::GaugeFailed, "derived self-gauge L0 is invalid");
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
    result.stage2GaugeScanPasses = 4u;
    result.logicalWorkspacePeakBytes = workspacePeak;
    result.logicalResidentUpperBound = residentPeak;
    result.physicalFrameCount = 1u;
    result.independentEvidenceCount = 1u;

    out = result;
    return Status::ok();
}

const char* status_name(StatusCode code) noexcept {
    switch (code) {
        case StatusCode::Ok: return "OK";
        case StatusCode::InvalidArgument: return "INVALID_ARGUMENT";
        case StatusCode::SourceFailed: return "SOURCE_FAILED";
        case StatusCode::ReconstructionFailed: return "RECONSTRUCTION_FAILED";
        case StatusCode::DigestFailed: return "DIGEST_FAILED";
        case StatusCode::GaugeFailed: return "GAUGE_FAILED";
        case StatusCode::BudgetExceeded: return "BUDGET_EXCEEDED";
    }
    return "UNKNOWN";
}

}  // namespace truthraw::scientific_master_streaming_binding::v0_1
