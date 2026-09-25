#include "truthnegative_continuous_v0_5.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <vector>

namespace truthraw::truthnegative_continuous::v0_5 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(
        d.begin(), d.end(), [](std::uint8_t v) { return v != 0u; });
}

void hash_u8(
    truthraw::sha256_v0_69::Hasher& h,
    std::uint8_t v) noexcept {
    h.update(&v, 1u);
}

void hash_u32(
    truthraw::sha256_v0_69::Hasher& h,
    std::uint32_t v) noexcept {
    const std::array<std::uint8_t, 4u> b{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v >> 8u),
        static_cast<std::uint8_t>(v >> 16u),
        static_cast<std::uint8_t>(v >> 24u),
    };
    h.update(b);
}

void hash_u64(
    truthraw::sha256_v0_69::Hasher& h,
    std::uint64_t v) noexcept {
    std::array<std::uint8_t, 8u> b{};
    for (unsigned i = 0u; i < 8u; ++i) {
        b[i] = static_cast<std::uint8_t>(v >> (8u * i));
    }
    h.update(b);
}

void hash_f32(
    truthraw::sha256_v0_69::Hasher& h,
    float v) noexcept {
    hash_u32(h, std::bit_cast<std::uint32_t>(v));
}

void hash_f64(
    truthraw::sha256_v0_69::Hasher& h,
    double v) noexcept {
    hash_u64(h, std::bit_cast<std::uint64_t>(v));
}

void hash_string(
    truthraw::sha256_v0_69::Hasher& h,
    const std::string& v) noexcept {
    hash_u64(h, static_cast<std::uint64_t>(v.size()));
    h.update(
        reinterpret_cast<const std::uint8_t*>(v.data()),
        v.size());
}

bool valid_state_input(const StateInput& in) noexcept {
    return nonzero(in.sourceEvidenceSha256) &&
           nonzero(in.scientificMasterSha256) &&
           nonzero(in.authorityFieldSha256) &&
           in.width > 0u &&
           in.height > 0u &&
           !in.reconstructionBackendId.empty() &&
           !in.colourBindingId.empty() &&
           in.physicalFrameCount == 1u &&
           in.independentEvidenceCount == 1u;
}

void hash_channel_summary(
    truthraw::sha256_v0_69::Hasher& h,
    const free_world::ChannelSupportSummary& s) noexcept {
    hash_f64(h, s.calibratedEstimateWeight);
    hash_f64(h, s.reconstructedWeight);
    hash_f64(h, s.censoredWeight);
    hash_f64(h, s.unknownWeight);
    hash_f64(h, s.sourceMeasuredCfaWeight);
    hash_f64(h, s.scientificReconstructionWeight);
    hash_f64(h, s.denseProjectionWeight);
    hash_f64(h, s.restorationDerivativeWeight);
    hash_f64(h, s.unknownRoleWeight);
    hash_u8(h, s.uncertaintyKnown ? 1u : 0u);
    hash_f64(h, s.p95Uncertainty);
    hash_u8(h, s.boundKnown ? 1u : 0u);
    hash_f64(h, s.lowerBound);
    hash_u8(h, static_cast<std::uint8_t>(s.boundDomain));
    hash_u8(h, s.contributionMask);
    hash_u8(h, static_cast<std::uint8_t>(s.authority));
}

}  // namespace

bool summarizeAuthorityField(
    local::IFieldTileSource& source,
    AuthorityFieldSummary& out) noexcept {
    out = AuthorityFieldSummary{};
    try {
        const auto g = source.geometry();
        if (g.sourceWidth == 0u || g.sourceHeight == 0u) return false;

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[] =
            "D_RAW_TRUTHNEGATIVE_LOCAL_AUTHORITY_FIELD_V0_5";
        h.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain) - 1u);
        hash_u32(h, g.sourceWidth);
        hash_u32(h, g.sourceHeight);
        hash_u32(h, field::kCanonicalTileEdge);

        std::vector<field::ChannelRecord> records;
        for (std::uint32_t y = 0u; y < g.sourceHeight;
             y += field::kCanonicalTileEdge) {
            const std::uint32_t height =
                std::min(field::kCanonicalTileEdge, g.sourceHeight - y);
            for (std::uint32_t x = 0u; x < g.sourceWidth;
                 x += field::kCanonicalTileEdge) {
                const std::uint32_t width =
                    std::min(field::kCanonicalTileEdge, g.sourceWidth - x);
                const std::size_t pixels =
                    static_cast<std::size_t>(width) * height;
                if (pixels >
                    std::numeric_limits<std::size_t>::max() / 3u) {
                    return false;
                }
                const std::size_t count = pixels * 3u;
                records.assign(count, field::ChannelRecord{});
                if (!source.readSourceTile(
                        x, y, width, height,
                        records.data(), records.size())) {
                    return false;
                }

                hash_u32(h, x);
                hash_u32(h, y);
                hash_u32(h, width);
                hash_u32(h, height);
                hash_u64(h, static_cast<std::uint64_t>(count));

                for (const auto& r : records) {
                    if (!field::validate_record(r) || !r.valuePresent) {
                        return false;
                    }

                    const auto role =
                        static_cast<std::size_t>(r.role);
                    const auto authorityRaw =
                        static_cast<std::uint8_t>(r.authority);
                    if (role >= out.creationRoleCounts.size() ||
                        authorityRaw < 1u || authorityRaw > 4u) {
                        return false;
                    }
                    const std::size_t authority =
                        static_cast<std::size_t>(authorityRaw - 1u);

                    ++out.creationRoleCounts[role];
                    ++out.authorityCounts[authority];
                    ++out.recordCount;
                    if (r.p95Known) ++out.p95KnownCount;
                    if (r.supportKnown) ++out.supportKnownCount;
                    if (r.boundKnown) ++out.boundKnownCount;

                    hash_f32(h, r.value);
                    hash_u8(h, static_cast<std::uint8_t>(r.role));
                    hash_u8(h, static_cast<std::uint8_t>(r.authority));
                    hash_u8(h, static_cast<std::uint8_t>(r.uncertainty));
                    hash_u8(h, static_cast<std::uint8_t>(r.boundDomain));
                    hash_u8(h, r.valuePresent ? 1u : 0u);
                    hash_u8(h, r.p95Known ? 1u : 0u);
                    hash_f32(h, r.p95);
                    hash_u8(h, r.supportKnown ? 1u : 0u);
                    hash_f32(h, r.support);
                    hash_u8(h, r.boundKnown ? 1u : 0u);
                    hash_f32(h, r.bound);
                    hash_u8(h, r.contributionMask);
                }
                ++out.tileCount;
            }
        }

        const std::uint64_t expected =
            static_cast<std::uint64_t>(g.sourceWidth) *
            static_cast<std::uint64_t>(g.sourceHeight) * 3u;
        if (out.recordCount != expected) return false;

        out.contentSha256 = h.finalize();
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;
        return nonzero(out.contentSha256);
    } catch (...) {
        out = AuthorityFieldSummary{};
        return false;
    }
}

bool finalizeState(
    const StateInput& input,
    State& out) noexcept {
    out = State{};
    try {
        if (!valid_state_input(input)) return false;

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[] =
            "D_RAW_TRUTHNEGATIVE_CONTINUOUS_STATE_V0_5";
        h.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain) - 1u);
        h.update(input.sourceEvidenceSha256);
        h.update(input.scientificMasterSha256);
        h.update(input.authorityFieldSha256);
        hash_u32(h, input.width);
        hash_u32(h, input.height);
        hash_string(h, input.reconstructionBackendId);
        hash_string(h, input.colourBindingId);
        hash_u32(h, input.physicalFrameCount);
        hash_u32(h, input.independentEvidenceCount);

        out.input = input;
        out.stateSha256 = h.finalize();
        out.finalized = nonzero(out.stateSha256);
        out.isRasterIndependent = true;
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;
        return out.finalized;
    } catch (...) {
        out = State{};
        return false;
    }
}

namespace {

bool finalize_query(
    const State& state,
    std::uint32_t targetWidth,
    std::uint32_t targetHeight,
    std::uint32_t targetX,
    std::uint32_t targetY,
    free_world::ResolvedPixel pixel,
    QueryResult& out) noexcept {
    out = QueryResult{};
    try {
        if (pixel.createsNewEvidence ||
            pixel.measuredTargetClaimCount != 0u ||
            pixel.physicalFrameCount != 1u ||
            pixel.independentEvidenceCount != 1u) {
            return false;
        }

        double footprintSum = 0.0;
        for (const auto& f : pixel.footprint) {
            if (f.x >= state.input.width ||
                f.y >= state.input.height ||
                !std::isfinite(f.weight) ||
                f.weight <= 0.0) {
                return false;
            }
            footprintSum += f.weight;
        }
        if (pixel.footprint.empty() ||
            !std::isfinite(footprintSum) ||
            std::abs(footprintSum - 1.0) > 1e-12) {
            return false;
        }

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[] =
            "D_RAW_TRUTHNEGATIVE_CONTINUOUS_QUERY_V0_5";
        h.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain) - 1u);
        h.update(state.stateSha256);
        hash_u32(h, targetWidth);
        hash_u32(h, targetHeight);
        hash_u32(h, targetX);
        hash_u32(h, targetY);
        for (double v : pixel.sceneLinear) hash_f64(h, v);
        for (const auto& support : pixel.support) {
            hash_channel_summary(h, support);
        }
        hash_u64(
            h,
            static_cast<std::uint64_t>(pixel.footprint.size()));
        for (const auto& f : pixel.footprint) {
            hash_u32(h, f.x);
            hash_u32(h, f.y);
            hash_f64(h, f.weight);
        }

        out.pixel = std::move(pixel);
        out.stateSha256 = state.stateSha256;
        out.querySha256 = h.finalize();
        out.targetWidth = targetWidth;
        out.targetHeight = targetHeight;
        out.targetX = targetX;
        out.targetY = targetY;
        out.stateIdentityChangedByTargetRaster = false;
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;
        return nonzero(out.querySha256);
    } catch (...) {
        out = QueryResult{};
        return false;
    }
}

bool valid_query_state(
    const free_world::IScenePlaneSource& scene,
    const State& state,
    std::uint32_t targetWidth,
    std::uint32_t targetHeight) noexcept {
    return state.finalized &&
           nonzero(state.stateSha256) &&
           !state.createsNewEvidence &&
           !state.scientificWritebackAllowed &&
           scene.width() == state.input.width &&
           scene.height() == state.input.height &&
           targetWidth > 0u &&
           targetHeight > 0u;
}

}  // namespace

bool resolvePixel(
    const free_world::IScenePlaneSource& scene,
    const State& state,
    std::uint32_t targetWidth,
    std::uint32_t targetHeight,
    std::uint32_t targetX,
    std::uint32_t targetY,
    QueryResult& out) noexcept {
    out = QueryResult{};
    try {
        if (!valid_query_state(scene, state, targetWidth, targetHeight) ||
            targetX >= targetWidth ||
            targetY >= targetHeight) {
            return false;
        }

        free_world::Resolver resolver(scene, targetWidth, targetHeight);
        if (!resolver.valid()) return false;

        free_world::ResolvedPixel pixel{};
        if (!resolver.resolvePixel(targetX, targetY, pixel)) return false;
        return finalize_query(
            state,
            targetWidth,
            targetHeight,
            targetX,
            targetY,
            std::move(pixel),
            out);
    } catch (...) {
        out = QueryResult{};
        return false;
    }
}

RasterResolver::RasterResolver(
    const free_world::IScenePlaneSource& scene,
    const State& state,
    std::uint32_t targetWidth,
    std::uint32_t targetHeight) noexcept
    : scene_(scene),
      state_(state),
      targetWidth_(targetWidth),
      targetHeight_(targetHeight) {
    try {
        if (!valid_query_state(scene_, state_, targetWidth_, targetHeight_)) {
            error_ = "TruthNegative continuous raster geometry/state mismatch";
            return;
        }

        xWeights_.resize(targetWidth_);
        for (std::uint32_t x = 0u; x < targetWidth_; ++x) {
            xWeights_[x] = free_world::axisAreaWeights(
                scene_.width(), x, targetWidth_);
            if (xWeights_[x].empty()) {
                error_ = "TruthNegative continuous X footprint precompute failed";
                xWeights_.clear();
                return;
            }
        }

        yWeights_.resize(targetHeight_);
        for (std::uint32_t y = 0u; y < targetHeight_; ++y) {
            yWeights_[y] = free_world::axisAreaWeights(
                scene_.height(), y, targetHeight_);
            if (yWeights_[y].empty()) {
                error_ = "TruthNegative continuous Y footprint precompute failed";
                xWeights_.clear();
                yWeights_.clear();
                return;
            }
        }

        valid_ = true;
    } catch (...) {
        xWeights_.clear();
        yWeights_.clear();
        error_ = "TruthNegative continuous raster precompute allocation failed";
    }
}

bool RasterResolver::valid() const noexcept {
    return valid_;
}

const std::string& RasterResolver::error() const noexcept {
    return error_;
}

std::uint32_t RasterResolver::targetWidth() const noexcept {
    return targetWidth_;
}

std::uint32_t RasterResolver::targetHeight() const noexcept {
    return targetHeight_;
}

bool RasterResolver::resolvePixel(
    std::uint32_t targetX,
    std::uint32_t targetY,
    QueryResult& out) const noexcept {
    out = QueryResult{};
    if (!valid_ ||
        targetX >= targetWidth_ ||
        targetY >= targetHeight_) {
        return false;
    }

    free_world::ResolvedPixel pixel{};
    if (!free_world::resolvePixelFromAxisWeights(
            scene_,
            xWeights_[targetX],
            yWeights_[targetY],
            pixel)) {
        return false;
    }

    return finalize_query(
        state_,
        targetWidth_,
        targetHeight_,
        targetX,
        targetY,
        std::move(pixel),
        out);
}

const char* schema_name() noexcept {
    return kSchemaName;
}

}  // namespace truthraw::truthnegative_continuous::v0_5
