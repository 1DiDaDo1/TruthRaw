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

AuthorityFieldAccumulator::AuthorityFieldAccumulator(
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight) noexcept
    : sourceWidth_(sourceWidth),
      sourceHeight_(sourceHeight) {
    try {
        if (sourceWidth_ == 0u || sourceHeight_ == 0u) return;
        constexpr char domain[] =
            "D_RAW_TRUTHNEGATIVE_LOCAL_AUTHORITY_FIELD_V0_5";
        hasher_.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain) - 1u);
        hash_u32(hasher_, sourceWidth_);
        hash_u32(hasher_, sourceHeight_);
        hash_u32(hasher_, field::kCanonicalTileEdge);
        valid_ = true;
    } catch (...) {
        valid_ = false;
    }
}

bool AuthorityFieldAccumulator::valid() const noexcept {
    return valid_ && !finalized_;
}

bool AuthorityFieldAccumulator::beginTile(
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t width,
    std::uint32_t height,
    std::size_t recordCount) noexcept {
    if (!valid_ || finalized_ || width == 0u || height == 0u) {
        return false;
    }
    try {
        if (x != expectedTileX_ || y != expectedTileY_ ||
            x >= sourceWidth_ || y >= sourceHeight_ ||
            width != std::min(field::kCanonicalTileEdge, sourceWidth_ - x) ||
            height != std::min(field::kCanonicalTileEdge, sourceHeight_ - y)) {
            return false;
        }
        const std::size_t pixels =
            static_cast<std::size_t>(width) *
            static_cast<std::size_t>(height);
        if (pixels >
            std::numeric_limits<std::size_t>::max() / 3u ||
            recordCount != pixels * 3u) {
            return false;
        }

        hash_u32(hasher_, x);
        hash_u32(hasher_, y);
        hash_u32(hasher_, width);
        hash_u32(hasher_, height);
        hash_u64(
            hasher_,
            static_cast<std::uint64_t>(recordCount));
        return true;
    } catch (...) {
        valid_ = false;
        return false;
    }
}

bool AuthorityFieldAccumulator::appendRecord(
    const field::ChannelRecord& r) noexcept {
    if (!valid_ || finalized_) return false;
    try {
        if (!field::validate_record(r) || !r.valuePresent) {
            return false;
        }

        const auto role =
            static_cast<std::size_t>(r.role);
        const auto authorityRaw =
            static_cast<std::uint8_t>(r.authority);
        if (role >= partial_.creationRoleCounts.size() ||
            authorityRaw < 1u || authorityRaw > 4u) {
            return false;
        }
        const std::size_t authority =
            static_cast<std::size_t>(authorityRaw - 1u);

        ++partial_.creationRoleCounts[role];
        ++partial_.authorityCounts[authority];
        ++partial_.recordCount;
        if (r.p95Known) ++partial_.p95KnownCount;
        if (r.supportKnown) ++partial_.supportKnownCount;
        if (r.boundKnown) ++partial_.boundKnownCount;

        hash_f32(hasher_, r.value);
        hash_u8(
            hasher_,
            static_cast<std::uint8_t>(r.role));
        hash_u8(
            hasher_,
            static_cast<std::uint8_t>(r.authority));
        hash_u8(
            hasher_,
            static_cast<std::uint8_t>(r.uncertainty));
        hash_u8(
            hasher_,
            static_cast<std::uint8_t>(r.boundDomain));
        hash_u8(hasher_, r.valuePresent ? 1u : 0u);
        hash_u8(hasher_, r.p95Known ? 1u : 0u);
        hash_f32(hasher_, r.p95);
        hash_u8(hasher_, r.supportKnown ? 1u : 0u);
        hash_f32(hasher_, r.support);
        hash_u8(hasher_, r.boundKnown ? 1u : 0u);
        hash_f32(hasher_, r.bound);
        hash_u8(hasher_, r.contributionMask);
        return true;
    } catch (...) {
        valid_ = false;
        return false;
    }
}

bool AuthorityFieldAccumulator::accountCanonicalSourceRecord(
    const field::CanonicalSourceChannelRecord& r) noexcept {
    if (!valid_ || finalized_) return false;
    try {
        const auto role =
            static_cast<std::size_t>(r.role);
        const auto authorityRaw =
            static_cast<std::uint8_t>(r.authority);
        if (role >= partial_.creationRoleCounts.size() ||
            authorityRaw < 1u || authorityRaw > 4u) {
            return false;
        }
        const std::size_t authority =
            static_cast<std::size_t>(authorityRaw - 1u);

        ++partial_.creationRoleCounts[role];
        ++partial_.authorityCounts[authority];
        ++partial_.recordCount;
        if (r.p95Known) ++partial_.p95KnownCount;
        if (r.supportKnown) ++partial_.supportKnownCount;
        if (r.boundKnown) ++partial_.boundKnownCount;
        return true;
    } catch (...) {
        valid_ = false;
        return false;
    }
}

bool AuthorityFieldAccumulator::finishTile(
    std::uint32_t x,
    std::uint32_t y) noexcept {
    if (!valid_ || finalized_) return false;
    try {
        ++partial_.tileCount;
        const std::uint32_t nextX =
            x + field::kCanonicalTileEdge;
        if (nextX >= sourceWidth_) {
            expectedTileX_ = 0u;
            expectedTileY_ =
                y + field::kCanonicalTileEdge;
        } else {
            expectedTileX_ = nextX;
            expectedTileY_ = y;
        }
        return true;
    } catch (...) {
        valid_ = false;
        return false;
    }
}

bool AuthorityFieldAccumulator::appendRecords(
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t width,
    std::uint32_t height,
    std::span<const field::ChannelRecord> records) noexcept {
    if (!beginTile(x, y, width, height, records.size())) {
        return false;
    }
    for (const auto& record : records) {
        if (!appendRecord(record)) return false;
    }
    return finishTile(x, y);
}

bool AuthorityFieldAccumulator::appendSourceTile(
    CfaPattern cfa,
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t width,
    std::uint32_t height,
    std::span<const std::uint16_t> raw,
    float whiteLevel,
    std::span<const float> cameraNativeRgb) noexcept {
    if (!valid_ || finalized_ || width == 0u || height == 0u) {
        return false;
    }
    try {
        const std::size_t pixels =
            static_cast<std::size_t>(width) *
            static_cast<std::size_t>(height);
        if (pixels >
            std::numeric_limits<std::size_t>::max() / 3u ||
            raw.size() != pixels ||
            cameraNativeRgb.size() != pixels * 3u ||
            !std::isfinite(whiteLevel) ||
            whiteLevel <= 0.0f ||
            !beginTile(
                x,
                y,
                width,
                height,
                pixels * 3u)) {
            return false;
        }

        std::array<
            std::uint8_t,
            kAuthorityDirectHashBatchBytes> byteBatch{};
        std::size_t batchUsed = 0u;

        const auto flushBatch = [&]() noexcept -> bool {
            if (batchUsed == 0u) return true;
            hasher_.update(byteBatch.data(), batchUsed);
            batchUsed = 0u;
            return true;
        };

        for (std::uint32_t yy = 0u; yy < height; ++yy) {
            for (std::uint32_t xx = 0u; xx < width; ++xx) {
                const std::size_t pi =
                    static_cast<std::size_t>(yy) * width + xx;
                for (int ch = 0; ch < 3; ++ch) {
                    const float cameraNativeValue =
                        cameraNativeRgb[
                            3u * pi +
                            static_cast<std::size_t>(ch)];
                    field::CanonicalSourceChannelRecord encoded{};
                    const auto encodeStatus =
                        field::encode_source_channel_record_canonical_v1(
                            cfa,
                            x + xx,
                            y + yy,
                            raw[pi],
                            whiteLevel,
                            ch,
                            cameraNativeValue,
                            encoded);

                    if (encodeStatus ==
                        field::CanonicalSourceEncodingStatus::Encoded) {
                        if (!accountCanonicalSourceRecord(encoded)) {
                            valid_ = false;
                            return false;
                        }
                        if (batchUsed + encoded.bytes.size() >
                            byteBatch.size()) {
                            if (!flushBatch()) {
                                valid_ = false;
                                return false;
                            }
                        }
                        std::copy(
                            encoded.bytes.begin(),
                            encoded.bytes.end(),
                            byteBatch.begin() +
                                static_cast<std::ptrdiff_t>(batchUsed));
                        batchUsed += encoded.bytes.size();
                        ++directByteRecordCount_;
                        continue;
                    }

                    if (encodeStatus ==
                        field::CanonicalSourceEncodingStatus::
                            UnsupportedSemanticExtension) {
                        if (!flushBatch()) {
                            valid_ = false;
                            return false;
                        }
                        field::ChannelRecord fallback{};
                        if (!field::build_source_channel_record(
                                cfa,
                                x + xx,
                                y + yy,
                                raw[pi],
                                whiteLevel,
                                ch,
                                cameraNativeValue,
                                fallback) ||
                            !appendRecord(fallback)) {
                            valid_ = false;
                            return false;
                        }
                        ++genericFallbackRecordCount_;
                        continue;
                    }

                    valid_ = false;
                    return false;
                }
            }
        }
        if (!flushBatch()) {
            valid_ = false;
            return false;
        }
        return finishTile(x, y);
    } catch (...) {
        valid_ = false;
        return false;
    }
}

std::size_t AuthorityFieldAccumulator::residentBytesUpperBound() const noexcept {
    return 0u;
}

std::uint64_t AuthorityFieldAccumulator::directByteRecordCount() const noexcept {
    return directByteRecordCount_;
}

std::uint64_t AuthorityFieldAccumulator::genericFallbackRecordCount() const noexcept {
    return genericFallbackRecordCount_;
}

bool AuthorityFieldAccumulator::finalize(
    AuthorityFieldSummary& out) noexcept {
    out = AuthorityFieldSummary{};
    if (!valid_ || finalized_) return false;
    try {
        const std::uint64_t expected =
            static_cast<std::uint64_t>(sourceWidth_) *
            static_cast<std::uint64_t>(sourceHeight_) * 3u;
        const std::uint64_t expectedTilesX =
            (static_cast<std::uint64_t>(sourceWidth_) +
             field::kCanonicalTileEdge - 1u) /
            field::kCanonicalTileEdge;
        const std::uint64_t expectedTilesY =
            (static_cast<std::uint64_t>(sourceHeight_) +
             field::kCanonicalTileEdge - 1u) /
            field::kCanonicalTileEdge;
        if (partial_.recordCount != expected ||
            partial_.tileCount != expectedTilesX * expectedTilesY ||
            expectedTileX_ != 0u ||
            expectedTileY_ < sourceHeight_) {
            return false;
        }

        partial_.contentSha256 = hasher_.finalize();
        partial_.createsNewEvidence = false;
        partial_.scientificWritebackAllowed = false;
        if (!nonzero(partial_.contentSha256)) return false;

        out = partial_;
        finalized_ = true;
        return true;
    } catch (...) {
        out = AuthorityFieldSummary{};
        valid_ = false;
        return false;
    }
}

bool summarizeAuthorityField(
    local::IFieldTileSource& source,
    AuthorityFieldSummary& out) noexcept {
    out = AuthorityFieldSummary{};
    try {
        const auto g = source.geometry();
        if (g.sourceWidth == 0u || g.sourceHeight == 0u) return false;

        AuthorityFieldAccumulator accumulator(
            g.sourceWidth,
            g.sourceHeight);
        if (!accumulator.valid()) return false;

        std::vector<field::ChannelRecord> records;
        for (std::uint32_t y = 0u; y < g.sourceHeight;
             y += field::kCanonicalTileEdge) {
            const std::uint32_t height =
                std::min(
                    field::kCanonicalTileEdge,
                    g.sourceHeight - y);
            for (std::uint32_t x = 0u; x < g.sourceWidth;
                 x += field::kCanonicalTileEdge) {
                const std::uint32_t width =
                    std::min(
                        field::kCanonicalTileEdge,
                        g.sourceWidth - x);
                const std::size_t pixels =
                    static_cast<std::size_t>(width) *
                    static_cast<std::size_t>(height);
                if (pixels >
                    std::numeric_limits<std::size_t>::max() / 3u) {
                    return false;
                }
                records.assign(
                    pixels * 3u,
                    field::ChannelRecord{});
                if (!source.readSourceTile(
                        x,
                        y,
                        width,
                        height,
                        records.data(),
                        records.size()) ||
                    !accumulator.appendRecords(
                        x,
                        y,
                        width,
                        height,
                        records)) {
                    return false;
                }
            }
        }
        return accumulator.finalize(out);
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
