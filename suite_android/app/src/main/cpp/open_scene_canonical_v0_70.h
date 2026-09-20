#pragma once

#include "truthraw_sha256_v0_69.h"
#include "full_frame_streaming_v0_1.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace truthraw::open_scene_canonical::v0_70 {

using Digest = truthraw::sha256_v0_69::Digest;

enum class PixelState : std::uint8_t {
    RCalibratedEstimate = 1,
    GCalibratedEstimate = 2,
    BCalibratedEstimate = 3,
    RCensored = 4,
    GCensored = 5,
    BCensored = 6,
};

struct Binding final {
    Digest sourceEvidenceSha256{};
    Digest scientificMasterSha256{};
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
    std::string colourBindingId;
};

struct Summary final {
    Digest dynamicAuthoritySha256{};
    Digest contentSha256{};
    Digest policySha256{};
    Digest artifactSha256{};
    std::array<std::uint64_t, 6> statePixelCounts{};
    std::uint64_t pixelCount = 0;
    std::uint64_t counterfactualPixelCount = 0;
    std::uint64_t scientificWritebackPixelCount = 0;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
    bool createsNewEvidence = false;
    bool chunkingChangesScientificIdentity = false;
};

class Builder final {
public:
    explicit Builder(Binding binding);

    bool valid() const noexcept { return valid_; }

    bool append(
        std::span<const std::uint8_t> dynamicAuthorityRgb,
        std::span<const std::uint8_t> canonicalPixelStates) noexcept;

    bool finalize(Summary& out) noexcept;

private:
    Binding binding_{};
    truthraw::sha256_v0_69::Hasher dynamicHasher_{};
    truthraw::sha256_v0_69::Hasher contentHasher_{};
    std::array<std::uint64_t, 6> counts_{};
    std::uint64_t pixels_ = 0;
    bool valid_ = false;
    bool finalized_ = false;
};

bool build_from_source(
    truthraw::streaming_v0_1::IRawTileSource& source,
    const Binding& binding,
    Summary& out) noexcept;

const char* state_name(PixelState state) noexcept;
const char* schema_name() noexcept;
const char* semantic_parent_region() noexcept;
const char* semantic_parent_stream() noexcept;

}  // namespace truthraw::open_scene_canonical::v0_70
