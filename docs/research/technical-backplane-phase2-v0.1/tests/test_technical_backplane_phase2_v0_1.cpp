#include "scientific_master_digest_v0_1.h"
#include "technical_backplane_phase2_v0_1.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

using namespace truthraw;
using namespace truthraw::technical_backplane_phase2::v0_1;

namespace {

#define CHECK(expr) do { \
    if (!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while (0)

class MemorySource final : public tile_dng_v0_1::IRandomAccessByteSource {
public:
    explicit MemorySource(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)) {}

    std::uint64_t sizeBytes() const override { return static_cast<std::uint64_t>(bytes_.size()); }
    std::size_t residentBytesUpperBound() const override { return sizeof(*this) + bytes_.size(); }

    bool readExact(std::uint64_t offset, void* dst, std::size_t count) override {
        if (offset > bytes_.size()) return false;
        const auto start = static_cast<std::size_t>(offset);
        if (count > bytes_.size() - start) return false;
        if (count != 0u) std::memcpy(dst, bytes_.data() + start, count);
        return true;
    }

private:
    std::vector<std::uint8_t> bytes_;
};

scientific_preview_binding_v0_2::PreparedScientificPreviewSource make_prepared_source() {
    std::vector<std::uint8_t> sourceBytes(8192u);
    for (std::size_t i = 0; i < sourceBytes.size(); ++i) {
        sourceBytes[i] = static_cast<std::uint8_t>((i * 37u + 11u) & 0xffu);
    }
    MemorySource source(std::move(sourceBytes));

    scientific_preview_binding_v0_1::SourceSeal seal{};
    const auto sealStatus = scientific_preview_binding_v0_1::seal_source_sha256(source, seal, 1024u);
    if (!sealStatus) {
        std::cerr << "source seal failed: " << sealStatus.message << '\n';
        return {};
    }

    scientific_preview_binding_v0_1::ScientificColorBindingRecord color{};
    color.authority = scientific_preview_binding_v0_1::ColorBindingAuthority::SourceMetadataBound;
    color.sourceEvidenceId = seal.sourceEvidenceId;
    color.bindingId = "TEST_SOURCE_METADATA_D50_BINDING";
    color.cameraToXyzD50 = {
        0.721f, 0.183f, 0.096f,
        0.241f, 0.704f, 0.055f,
        0.018f, 0.112f, 0.870f,
    };
    color.normalized = true;
    color.validated = true;
    color.physicalFrameCount = 1u;
    color.independentEvidenceCount = 1u;

    scientific_preview_binding_v0_2::PreparedScientificPreviewSource prepared{};
    const auto prepareStatus = scientific_preview_binding_v0_2::prepare_scientific_color_source(
        seal, color, prepared);
    if (!prepareStatus) {
        std::cerr << "source preparation failed: " << prepareStatus.message << '\n';
        return {};
    }
    return prepared;
}

Hash256 make_real_master_digest() {
    constexpr std::uint32_t width = 128u;
    constexpr std::uint32_t height = 64u;
    std::vector<float> rgb(static_cast<std::size_t>(width) * height * 3u);
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::size_t i = (static_cast<std::size_t>(y) * width + x) * 3u;
            rgb[i + 0u] = static_cast<float>(x) * 0.0031f - 0.021f;
            rgb[i + 1u] = static_cast<float>(x + y) * 0.0017f;
            rgb[i + 2u] = static_cast<float>(y) * 0.0043f - 0.037f;
        }
    }

    scientific_master_digest::v0_1::ScientificMasterDigestAccumulator digest(width, height);
    scientific_master_digest::v0_1::TileView tile{
        0u, 0u, width, height, rgb.data(), static_cast<std::size_t>(width) * 3u};
    if (!digest.valid() || !digest.add_tile(tile)) {
        std::cerr << "master digest tile failed: " << digest.error() << '\n';
        return {};
    }
    scientific_master_digest::v0_1::Sha256 out{};
    if (!digest.finalize(out)) {
        std::cerr << "master digest finalize failed: " << digest.error() << '\n';
        return {};
    }
    return out;
}

Phase2Input make_valid_input() {
    Phase2Input input{};
    input.prepared = make_prepared_source();
    input.scientificMasterHash = make_real_master_digest();

    input.zeroLineGauge.mode = TruthRangeGaugeModeV02::SelfGauge;
    input.zeroLineGauge.L0 = 0.2375;
    input.zeroLineGauge.gaugeId = "SELF_GAUGE_STAGE2_Q0.500000";
    input.zeroLineGauge.crossSceneComparable = false;
    input.zeroLineGauge.absolutePhysicalUnits = false;

    input.sceneBinding.reconstructionBackend = "truthraw_canonical_v4.7i";
    input.sceneBinding.reconstructionCoreCppSha256 = "bound-upstream";
    input.sceneBinding.reconstructionCoreHSha256 = "bound-upstream";
    input.sceneBinding.uncertaintyModelSha256 = "bound-upstream";
    input.sceneBinding.uncertaintyBindingSha256 = "bound-upstream";
    input.sceneBinding.sceneScaleId = "TRUTHRANGE_SELF_GAUGE_STAGE2_V0_2";
    input.sceneBinding.gainMapAppliedExactlyOnce = true;
    input.sceneBinding.exposureNormalizedToCommonScene = false;
    input.sceneBinding.gainNormalizedToCommonScene = false;

    input.roomStatus.fill(technical_backplane::v0_1::RoomStatus::ResearchOnly);
    input.roomStatus[0] = technical_backplane::v0_1::RoomStatus::Available; // Archivist/source identity
    input.roomStatus[1] = technical_backplane::v0_1::RoomStatus::Available; // Measurement binding
    input.roomStatus[2] = technical_backplane::v0_1::RoomStatus::Available; // Architect/reconstruction present
    input.claimStatus = technical_backplane::v0_1::ClaimStatus::Candidate;
    return input;
}

bool nonzero(const Hash256& hash) {
    return std::any_of(hash.begin(), hash.end(), [](std::uint8_t v) { return v != 0u; });
}

int test_success_path_without_fixture_hashes() {
    const auto input = make_valid_input();
    CHECK(nonzero(input.prepared.source.sha256));
    CHECK(nonzero(input.scientificMasterHash));

    Phase2Result result{};
    const auto status = finalize_phase2(input, result);
    if (!status) std::cerr << "phase2 failed: " << status.message << '\n';
    CHECK(status);
    CHECK(result.backplane.sourceEvidenceHash == input.prepared.source.sha256);
    CHECK(result.backplane.scientificMasterHash == input.scientificMasterHash);
    CHECK(result.backplane.zeroLineHash == result.zeroLineHash);
    CHECK(result.backplane.sceneScaleHash == result.sceneScaleHash);
    CHECK(nonzero(result.zeroLineHash));
    CHECK(nonzero(result.sceneScaleHash));
    CHECK(result.backplane.physicalFrameCount == 1u);
    CHECK(result.backplane.independentEvidenceCount == 1u);
    CHECK(result.backplane.forbiddenFlags == 0u);
    CHECK(result.serializedBackplane.size() == technical_backplane::v0_1::kSerializedBytes);
    CHECK(result.admission.sourceSeal.sha256 == input.prepared.source.sha256);
    CHECK(result.admission.claimScope == scientific_preview_binding_v0_1::ColorClaimScope::SourceBoundPreview);

    technical_backplane::v0_1::State roundTrip{};
    CHECK(technical_backplane::v0_1::deserialize(result.serializedBackplane, roundTrip) ==
          technical_backplane::v0_1::Status::Ok);
    CHECK(roundTrip.scientificMasterHash == input.scientificMasterHash);
    CHECK(roundTrip.zeroLineHash == result.zeroLineHash);
    CHECK(roundTrip.sceneScaleHash == result.sceneScaleHash);
    return 0;
}

int test_binding_determinism_and_mutation() {
    auto input = make_valid_input();
    Phase2Result a{};
    Phase2Result b{};
    CHECK(finalize_phase2(input, a));
    CHECK(finalize_phase2(input, b));
    CHECK(a.serializedBackplane == b.serializedBackplane);
    CHECK(a.zeroLineHash == b.zeroLineHash);
    CHECK(a.sceneScaleHash == b.sceneScaleHash);

    auto changedGauge = input;
    changedGauge.zeroLineGauge.L0 = 0.475;
    Phase2Result c{};
    CHECK(finalize_phase2(changedGauge, c));
    CHECK(c.zeroLineHash != a.zeroLineHash);
    CHECK(c.sceneScaleHash == a.sceneScaleHash);
    CHECK(c.serializedBackplane != a.serializedBackplane);

    auto changedScale = input;
    changedScale.sceneBinding.sceneScaleId = "TRUTHRANGE_SELF_GAUGE_STAGE2_V0_2_ALT";
    Phase2Result d{};
    CHECK(finalize_phase2(changedScale, d));
    CHECK(d.zeroLineHash == a.zeroLineHash);
    CHECK(d.sceneScaleHash != a.sceneScaleHash);
    return 0;
}

int test_fail_closed_paths() {
    {
        auto input = make_valid_input();
        input.scientificMasterHash.fill(0u);
        Phase2Result out{};
        const auto status = finalize_phase2(input, out);
        CHECK(!status);
        CHECK(status.code == StatusCode::InvalidScientificMasterDigest);
    }
    {
        auto input = make_valid_input();
        input.zeroLineGauge.crossSceneComparable = true;
        Phase2Result out{};
        const auto status = finalize_phase2(input, out);
        CHECK(!status);
        CHECK(status.code == StatusCode::InvalidZeroLine);
    }
    {
        auto input = make_valid_input();
        input.sceneBinding.sceneScaleId.clear();
        Phase2Result out{};
        const auto status = finalize_phase2(input, out);
        CHECK(!status);
        CHECK(status.code == StatusCode::InvalidSceneScale);
    }
    {
        auto input = make_valid_input();
        input.sceneBinding.gainMapAppliedExactlyOnce = false;
        Phase2Result out{};
        const auto status = finalize_phase2(input, out);
        CHECK(!status);
        CHECK(status.code == StatusCode::InvalidSceneScale);
    }
    {
        auto input = make_valid_input();
        input.prepared.source.sourceEvidenceId = "sha256:0000000000000000000000000000000000000000000000000000000000000000";
        Phase2Result out{};
        const auto status = finalize_phase2(input, out);
        CHECK(!status);
        CHECK(status.code == StatusCode::InvalidPreparedSource);
    }
    {
        auto input = make_valid_input();
        input.zeroLineGauge.mode = TruthRangeGaugeModeV02::PhysicalAbsoluteGauge;
        input.zeroLineGauge.crossSceneComparable = true;
        input.zeroLineGauge.absolutePhysicalUnits = true;
        input.zeroLineGauge.gaugeId = "UNSUPPORTED_PHYSICAL_TEST";
        Phase2Result out{};
        const auto status = finalize_phase2(input, out);
        CHECK(!status);
        CHECK(status.code == StatusCode::InvalidZeroLine);
    }
    return 0;
}

}  // namespace

int main() {
    CHECK(test_success_path_without_fixture_hashes() == 0);
    CHECK(test_binding_determinism_and_mutation() == 0);
    CHECK(test_fail_closed_paths() == 0);

    std::cout << "TECHNICAL_BACKPLANE_PHASE2_V0_1_TEST_PASS\n";
    std::cout << "scientificMasterHash=REAL_DIGEST_ENGINE_OUTPUT\n";
    std::cout << "zeroLineHash=CANONICAL_TRUTHRANGE_GAUGE_HASH\n";
    std::cout << "sceneScaleHash=CANONICAL_SCENE_SCALE_HASH\n";
    std::cout << "scientificPreviewFinalization=NO_FIXTURE_HASH_PATH\n";
    return 0;
}
