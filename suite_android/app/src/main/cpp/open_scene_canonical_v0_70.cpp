#include "open_scene_canonical_v0_70.h"

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

namespace truthraw::open_scene_canonical::v0_70 {
namespace {

bool nonzero(const Digest& digest) noexcept {
    return std::any_of(digest.begin(), digest.end(), [](std::uint8_t v) { return v != 0u; });
}

void append_u32_le(truthraw::sha256_v0_69::Hasher& hasher, std::uint32_t v) noexcept {
    const std::array<std::uint8_t, 4> b{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v >> 8u),
        static_cast<std::uint8_t>(v >> 16u),
        static_cast<std::uint8_t>(v >> 24u),
    };
    hasher.update(b);
}

Digest policy_hash(const Binding& binding) {
    truthraw::sha256_v0_69::Hasher h;
    const std::string prefix =
        "schema=TruthRawOpenSceneCanonicalState/0.70\n"
        "semantic_parent_region=TruthRawOpenSceneRegion/0.7\n"
        "semantic_parent_stream=TruthRawOpenSceneStateSummary/0.8\n"
        "colour_authority=SOURCE_METADATA_BOUND\n"
        "illumination_authority=SOURCE_BOUND_ESTIMATE\n"
        "detail_status=NEUTRAL_OR_BLOCKED\n"
        "counterfactual_pixels_allowed=0\n"
        "scientific_master_writeback_allowed=0\n"
        "creates_new_evidence=0\n"
        "chunking_changes_scientific_identity=0\n"
        "generic_missing_channel_policy=UNKNOWN_UNTIL_SOURCE_BOUND_UNCERTAINTY_IS_ADMITTED\n";
    h.update(reinterpret_cast<const std::uint8_t*>(prefix.data()), prefix.size());
    h.update(binding.sourceEvidenceSha256);
    h.update(binding.scientificMasterSha256);
    append_u32_le(h, binding.width);
    append_u32_le(h, binding.height);
    append_u32_le(h, binding.physicalFrameCount);
    append_u32_le(h, binding.independentEvidenceCount);
    h.update(
        reinterpret_cast<const std::uint8_t*>(binding.colourBindingId.data()),
        binding.colourBindingId.size());
    return h.finalize();
}

Digest artifact_hash(
    const Digest& dynamicAuthority,
    const Digest& content,
    const Digest& policy) {
    truthraw::sha256_v0_69::Hasher h;
    constexpr char kDomain[] = "TRUTHRAW_OPEN_SCENE_CANONICAL_ARTIFACT_V0_70";
    h.update(reinterpret_cast<const std::uint8_t*>(kDomain), sizeof(kDomain) - 1u);
    h.update(dynamicAuthority);
    h.update(content);
    h.update(policy);
    return h.finalize();
}

int measured_channel(truthraw::CfaPattern cfa, int x, int y) noexcept {
    const bool xe = (x & 1) == 0;
    const bool ye = (y & 1) == 0;
    switch (cfa) {
        case truthraw::CfaPattern::BGGR:
            if (ye && xe) return 2;
            if (!ye && !xe) return 0;
            return 1;
        case truthraw::CfaPattern::RGGB:
            if (ye && xe) return 0;
            if (!ye && !xe) return 2;
            return 1;
        case truthraw::CfaPattern::GRBG:
            if (ye && !xe) return 0;
            if (!ye && xe) return 2;
            return 1;
        case truthraw::CfaPattern::GBRG:
            if (!ye && xe) return 0;
            if (ye && !xe) return 2;
            return 1;
    }
    return -1;
}

}  // namespace

bool build_from_source(
    truthraw::streaming_v0_1::IRawTileSource& source,
    const Binding& binding,
    Summary& out) noexcept {
    try {
        const auto& md = source.metadata();
        if (md.width <= 0 || md.height <= 0 ||
            binding.width != static_cast<std::uint32_t>(md.width) ||
            binding.height != static_cast<std::uint32_t>(md.height)) {
            return false;
        }

        Builder builder(binding);
        if (!builder.valid()) return false;

        constexpr int kCanonicalEdge = 64;
        std::vector<std::uint16_t> raw;
        std::vector<float> gain;
        std::vector<std::uint8_t> authority;
        std::vector<std::uint8_t> states;

        for (int y = 0; y < md.height; y += kCanonicalEdge) {
            const int h = std::min(kCanonicalEdge, md.height - y);
            for (int x = 0; x < md.width; x += kCanonicalEdge) {
                const int w = std::min(kCanonicalEdge, md.width - x);
                const std::size_t pixels =
                    static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
                raw.resize(pixels);
                if (md.hasGainField) gain.resize(pixels); else gain.clear();

                truthraw::TileRect rect{x, y, x + w, y + h, x, y, x + w, y + h};
                const auto status = source.readRawTile(
                    rect,
                    raw.data(),
                    raw.size(),
                    md.hasGainField ? gain.data() : nullptr,
                    md.hasGainField ? gain.size() : 0u);
                if (!status) return false;

                authority.assign(pixels * 3u, 4u);
                states.assign(pixels, 0u);
                for (int yy = 0; yy < h; ++yy) {
                    for (int xx = 0; xx < w; ++xx) {
                        const std::size_t i =
                            static_cast<std::size_t>(yy) * static_cast<std::size_t>(w) +
                            static_cast<std::size_t>(xx);
                        const int ch = measured_channel(md.cfa, x + xx, y + yy);
                        if (ch < 0 || ch > 2) return false;
                        const bool clipped =
                            static_cast<float>(raw[i]) >= md.whiteLevel;
                        authority[3u * i + static_cast<std::size_t>(ch)] =
                            clipped ? 3u : 1u;
                        states[i] = static_cast<std::uint8_t>(
                            clipped
                                ? (ch == 0 ? PixelState::RCensored :
                                   ch == 1 ? PixelState::GCensored :
                                             PixelState::BCensored)
                                : (ch == 0 ? PixelState::RCalibratedEstimate :
                                   ch == 1 ? PixelState::GCalibratedEstimate :
                                             PixelState::BCalibratedEstimate));
                    }
                }
                if (!builder.append(authority, states)) return false;
            }
        }
        return builder.finalize(out);
    } catch (...) {
        return false;
    }
}

Builder::Builder(Binding binding) : binding_(std::move(binding)) {
    const std::uint64_t pixels =
        static_cast<std::uint64_t>(binding_.width) *
        static_cast<std::uint64_t>(binding_.height);
    valid_ =
        binding_.width > 0u &&
        binding_.height > 0u &&
        pixels <= std::numeric_limits<std::uint64_t>::max() / 3u &&
        binding_.physicalFrameCount == 1u &&
        binding_.independentEvidenceCount == 1u &&
        nonzero(binding_.sourceEvidenceSha256) &&
        nonzero(binding_.scientificMasterSha256) &&
        !binding_.colourBindingId.empty();
}

bool Builder::append(
    std::span<const std::uint8_t> dynamicAuthorityRgb,
    std::span<const std::uint8_t> canonicalPixelStates) noexcept {
    if (!valid_ || finalized_) return false;
    if (dynamicAuthorityRgb.size() != canonicalPixelStates.size() * 3u) return false;

    const std::uint64_t total =
        static_cast<std::uint64_t>(binding_.width) *
        static_cast<std::uint64_t>(binding_.height);
    if (pixels_ + canonicalPixelStates.size() > total) return false;

    for (const auto value : canonicalPixelStates) {
        if (value < static_cast<std::uint8_t>(PixelState::RCalibratedEstimate) ||
            value > static_cast<std::uint8_t>(PixelState::BCensored)) {
            return false;
        }
        ++counts_[static_cast<std::size_t>(value - 1u)];
    }

    dynamicHasher_.update(dynamicAuthorityRgb);
    contentHasher_.update(canonicalPixelStates);
    pixels_ += canonicalPixelStates.size();
    return true;
}

bool Builder::finalize(Summary& out) noexcept {
    if (!valid_ || finalized_) return false;
    const std::uint64_t expected =
        static_cast<std::uint64_t>(binding_.width) *
        static_cast<std::uint64_t>(binding_.height);
    if (pixels_ != expected) return false;

    out = {};
    out.dynamicAuthoritySha256 = dynamicHasher_.finalize();
    out.contentSha256 = contentHasher_.finalize();
    out.policySha256 = policy_hash(binding_);
    out.artifactSha256 = artifact_hash(
        out.dynamicAuthoritySha256,
        out.contentSha256,
        out.policySha256);
    out.statePixelCounts = counts_;
    out.pixelCount = pixels_;
    out.counterfactualPixelCount = 0u;
    out.scientificWritebackPixelCount = 0u;
    out.physicalFrameCount = binding_.physicalFrameCount;
    out.independentEvidenceCount = binding_.independentEvidenceCount;
    out.createsNewEvidence = false;
    out.chunkingChangesScientificIdentity = false;
    finalized_ = true;
    return true;
}

const char* state_name(PixelState state) noexcept {
    switch (state) {
        case PixelState::RCalibratedEstimate:
            return "R_CALIBRATED_ESTIMATE__G_UNKNOWN__B_UNKNOWN";
        case PixelState::GCalibratedEstimate:
            return "R_UNKNOWN__G_CALIBRATED_ESTIMATE__B_UNKNOWN";
        case PixelState::BCalibratedEstimate:
            return "R_UNKNOWN__G_UNKNOWN__B_CALIBRATED_ESTIMATE";
        case PixelState::RCensored:
            return "R_CENSORED__G_UNKNOWN__B_UNKNOWN";
        case PixelState::GCensored:
            return "R_UNKNOWN__G_CENSORED__B_UNKNOWN";
        case PixelState::BCensored:
            return "R_UNKNOWN__G_UNKNOWN__B_CENSORED";
    }
    return "INVALID";
}

const char* schema_name() noexcept {
    return "TruthRawOpenSceneCanonicalState/0.70";
}
const char* semantic_parent_region() noexcept {
    return "TruthRawOpenSceneRegion/0.7";
}
const char* semantic_parent_stream() noexcept {
    return "TruthRawOpenSceneStateSummary/0.8";
}

}  // namespace truthraw::open_scene_canonical::v0_70
