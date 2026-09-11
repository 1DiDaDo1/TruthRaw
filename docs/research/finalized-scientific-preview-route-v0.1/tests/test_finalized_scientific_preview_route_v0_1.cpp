#include "finalized_scientific_preview_route_v0_1.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

using namespace truthraw;
using namespace truthraw::finalized_scientific_preview_route::v0_1;

namespace {

void require_active(bool ok, const char* expr, int line) {
    if (!ok) {
        std::cerr << "REQUIRE_FAIL line=" << line << " expr=" << expr << '\n';
        std::exit(2);
    }
}
#define REQUIRE(expr) require_active(bool(expr), #expr, __LINE__)

class DngMemSource final : public tile_dng_v0_1::IRandomAccessByteSource {
public:
    explicit DngMemSource(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)) {}
    std::uint64_t sizeBytes() const override { return static_cast<std::uint64_t>(bytes_.size()); }
    std::size_t residentBytesUpperBound() const override { return sizeof(*this) + bytes_.capacity(); }
    bool readExact(std::uint64_t offset, void* dst, std::size_t count) override {
        if (dst == nullptr && count != 0u) return false;
        if (offset > bytes_.size()) return false;
        const auto start = static_cast<std::size_t>(offset);
        if (count > bytes_.size() - start) return false;
        if (count != 0u) std::memcpy(dst, bytes_.data() + start, count);
        return true;
    }
    void xor_byte(std::size_t index, std::uint8_t value) { bytes_.at(index) ^= value; }
private:
    std::vector<std::uint8_t> bytes_;
};

void p16(std::vector<std::uint8_t>& b, std::size_t o, std::uint16_t v) {
    if (b.size() < o + 2u) b.resize(o + 2u);
    b[o] = static_cast<std::uint8_t>(v);
    b[o + 1u] = static_cast<std::uint8_t>(v >> 8u);
}
void p32(std::vector<std::uint8_t>& b, std::size_t o, std::uint32_t v) {
    if (b.size() < o + 4u) b.resize(o + 4u);
    for (unsigned i = 0; i < 4u; ++i) b[o + i] = static_cast<std::uint8_t>(v >> (8u * i));
}
void p64(std::vector<std::uint8_t>& b, std::size_t o, std::uint64_t v) {
    if (b.size() < o + 8u) b.resize(o + 8u);
    for (unsigned i = 0; i < 8u; ++i) b[o + i] = static_cast<std::uint8_t>(v >> (8u * i));
}
std::vector<std::uint8_t> shorts(std::initializer_list<std::uint16_t> xs) {
    std::vector<std::uint8_t> b(xs.size() * 2u);
    std::size_t p = 0u;
    for (auto v : xs) { p16(b, p, v); p += 2u; }
    return b;
}
std::vector<std::uint8_t> longs(const std::vector<std::uint32_t>& xs) {
    std::vector<std::uint8_t> b(xs.size() * 4u);
    for (std::size_t i = 0; i < xs.size(); ++i) p32(b, 4u * i, xs[i]);
    return b;
}
std::vector<std::uint8_t> rationals(const std::array<std::uint32_t, 4>& xs) {
    std::vector<std::uint8_t> b(32u);
    for (std::size_t i = 0; i < xs.size(); ++i) { p32(b, 8u * i, xs[i]); p32(b, 8u * i + 4u, 1u); }
    return b;
}
std::vector<std::uint8_t> doubles(const std::array<double, 6>& xs) {
    std::vector<std::uint8_t> b(48u);
    for (std::size_t i = 0; i < xs.size(); ++i) {
        std::uint64_t u = 0u;
        std::memcpy(&u, &xs[i], sizeof(u));
        p64(b, 8u * i, u);
    }
    return b;
}

struct Entry {
    std::uint16_t tag = 0;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::vector<std::uint8_t> data;
    std::size_t q = 0;
    std::size_t payload = 0;
};

std::vector<std::uint8_t> make_dng(int width, int height) {
    std::vector<std::uint16_t> raw(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int value = 70 + ((x * 37 + y * 53 + x * y * 3) % 900);
            if ((x + y) % 97 == 0) value = 1023;
            raw[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                static_cast<std::size_t>(x)] = static_cast<std::uint16_t>(value);
        }
    }

    std::vector<Entry> entries;
    auto add = [&](std::uint16_t tag, std::uint16_t type, std::uint32_t count,
                   std::vector<std::uint8_t> data) {
        entries.push_back({tag, type, count, std::move(data), 0u, 0u});
    };
    add(256,4,1,longs({static_cast<std::uint32_t>(width)}));
    add(257,4,1,longs({static_cast<std::uint32_t>(height)}));
    add(258,3,1,shorts({16})); add(259,3,1,shorts({1}));
    add(262,3,1,shorts({32803})); add(274,3,1,shorts({1}));
    add(277,3,1,shorts({1})); add(284,3,1,shorts({1})); add(339,3,1,shorts({1}));
    add(33421,3,2,shorts({2,2})); add(33422,1,4,{2,1,1,0}); add(50710,1,3,{0,1,2});
    add(50713,3,2,shorts({2,2})); add(50714,5,4,rationals({64,65,66,67}));
    add(50717,4,1,longs({1023}));
    add(51041,12,6,doubles({0.0009,1e-6,0.0010,1.2e-6,0.0011,1.4e-6}));

    constexpr std::uint32_t rowsPerStrip = 2u;
    const auto strips = static_cast<std::uint32_t>((height + 1) / 2);
    add(273,4,strips,std::vector<std::uint8_t>(static_cast<std::size_t>(strips) * 4u));
    add(278,4,1,longs({rowsPerStrip}));
    add(279,4,strips,std::vector<std::uint8_t>(static_cast<std::size_t>(strips) * 4u));

    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.tag < b.tag; });
    std::vector<std::uint8_t> bytes(8u + 2u + entries.size() * 12u + 4u, 0u);
    bytes[0] = 'I'; bytes[1] = 'I'; p16(bytes,2,42); p32(bytes,4,8); p16(bytes,8,static_cast<std::uint16_t>(entries.size()));
    std::size_t ext = bytes.size();
    for (std::size_t i = 0; i < entries.size(); ++i) {
        auto& e = entries[i];
        e.q = 10u + 12u * i;
        p16(bytes,e.q,e.tag); p16(bytes,e.q+2u,e.type); p32(bytes,e.q+4u,e.count);
        if (e.data.size() <= 4u) {
            std::copy(e.data.begin(), e.data.end(), bytes.begin() + static_cast<std::ptrdiff_t>(e.q + 8u));
        } else {
            e.payload = ext;
            p32(bytes,e.q+8u,static_cast<std::uint32_t>(ext));
            bytes.insert(bytes.end(),e.data.begin(),e.data.end());
            ext = bytes.size();
        }
    }

    std::vector<std::uint32_t> offsets;
    std::vector<std::uint32_t> counts;
    for (std::uint32_t s = 0; s < strips; ++s) {
        const int y0 = static_cast<int>(s * rowsPerStrip);
        const int rows = std::min<int>(static_cast<int>(rowsPerStrip), height - y0);
        offsets.push_back(static_cast<std::uint32_t>(bytes.size()));
        counts.push_back(static_cast<std::uint32_t>(rows * width * 2));
        for (int y = y0; y < y0 + rows; ++y) {
            for (int x = 0; x < width; ++x) {
                const auto o = bytes.size();
                bytes.resize(o + 2u);
                p16(bytes,o,raw[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)]);
            }
        }
    }
    for (auto& e : entries) {
        if (e.tag == 273) { auto data = longs(offsets); std::copy(data.begin(),data.end(),bytes.begin()+static_cast<std::ptrdiff_t>(e.payload)); }
        if (e.tag == 279) { auto data = longs(counts); std::copy(data.begin(),data.end(),bytes.begin()+static_cast<std::ptrdiff_t>(e.payload)); }
    }
    return bytes;
}

scientific_preview_binding_v0_2::PreparedScientificPreviewSource prepare(
    const std::shared_ptr<DngMemSource>& bytes,
    scientific_preview_binding_v0_1::ColorBindingAuthority authority) {
    scientific_preview_binding_v0_1::SourceSeal seal{};
    REQUIRE(scientific_preview_binding_v0_1::seal_source_sha256(*bytes, seal, 1024u));

    scientific_preview_binding_v0_1::ScientificColorBindingRecord color{};
    color.authority = authority;
    color.sourceEvidenceId = seal.sourceEvidenceId;
    color.bindingId = authority == scientific_preview_binding_v0_1::ColorBindingAuthority::IndependentCalibration
        ? "independent_fixture_color_v1" : "source_metadata_fixture_color_v1";
    color.cameraToXyzD50 = {0.62f,0.21f,0.08f, 0.18f,0.71f,0.07f, 0.03f,0.12f,0.79f};
    color.normalized = true;
    color.validated = true;
    color.physicalFrameCount = 1u;
    color.independentEvidenceCount = 1u;

    scientific_preview_binding_v0_2::PreparedScientificPreviewSource prepared{};
    REQUIRE(scientific_preview_binding_v0_2::prepare_scientific_color_source(seal,color,prepared));
    return prepared;
}

bool nonzero(const technical_backplane::v0_1::Hash256& hash) {
    return std::any_of(hash.begin(),hash.end(),[](std::uint8_t b){ return b != 0u; });
}

Options default_options() {
    Options options{};
    options.sourceReverifyChunkBytes = 1024u;
    options.roomStatus.fill(technical_backplane::v0_1::RoomStatus::ResearchOnly);
    options.claimStatus = technical_backplane::v0_1::ClaimStatus::Candidate;
    return options;
}

}  // namespace

int main() {
    auto bytes = std::make_shared<DngMemSource>(make_dng(66,50));
    auto prepared = prepare(bytes, scientific_preview_binding_v0_1::ColorBindingAuthority::SourceMetadataBound);
    ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction;

    Result result{};
    const auto status = finalize_direct_native(prepared,bytes,reconstruction,default_options(),result);
    REQUIRE(status);
    REQUIRE(result.scientificPreviewReleaseAllowed);
    REQUIRE(!result.scientificClaimAllowed);
    REQUIRE(result.phase2.admission.claimScope == scientific_preview_binding_v0_1::ColorClaimScope::SourceBoundPreview);
    REQUIRE(result.phase2.backplane.sourceEvidenceHash == prepared.source.sha256);
    REQUIRE(result.phase2.backplane.scientificMasterHash == result.scientific.scientificMasterHash);
    REQUIRE(nonzero(result.phase2.backplane.scientificMasterHash));
    REQUIRE(nonzero(result.phase2.backplane.zeroLineHash));
    REQUIRE(nonzero(result.phase2.backplane.sceneScaleHash));
    REQUIRE(result.phase2.backplane.physicalFrameCount == 1u);
    REQUIRE(result.phase2.backplane.independentEvidenceCount == 1u);
    REQUIRE(result.scientific.zeroLineGauge.mode == TruthRangeGaugeModeV02::SelfGauge);
    REQUIRE(result.scientific.zeroLineGauge.L0 > 0.0);
    REQUIRE(!result.tileSourceAudit.fullFileMaterialized);
    REQUIRE(!result.tileSourceAudit.fullRawMaterialized);

    technical_backplane::v0_1::State roundtrip{};
    REQUIRE(technical_backplane::v0_1::deserialize(result.phase2.serializedBackplane,roundtrip) ==
            technical_backplane::v0_1::Status::Ok);
    REQUIRE(roundtrip.scientificMasterHash == result.scientific.scientificMasterHash);

    // Exact source bytes must still match immediately before finalization.
    bytes->xor_byte(32u,0x01u);
    Result mutated{};
    const auto mutatedStatus = finalize_direct_native(prepared,bytes,reconstruction,default_options(),mutated);
    REQUIRE(!mutatedStatus);
    REQUIRE(mutatedStatus.code == StatusCode::SourceReverificationFailed);
    bytes->xor_byte(32u,0x01u);

    // Independent calibration changes color claim authority, not frame/evidence counts.
    auto calibratedPrepared = prepare(bytes, scientific_preview_binding_v0_1::ColorBindingAuthority::IndependentCalibration);
    Result calibrated{};
    REQUIRE(finalize_direct_native(calibratedPrepared,bytes,reconstruction,default_options(),calibrated));
    REQUIRE(calibrated.scientificPreviewReleaseAllowed);
    REQUIRE(calibrated.scientificClaimAllowed);
    REQUIRE(calibrated.phase2.admission.claimScope ==
            scientific_preview_binding_v0_1::ColorClaimScope::IndependentlyCalibratedPreview);
    REQUIRE(calibrated.phase2.backplane.scientificMasterHash == result.phase2.backplane.scientificMasterHash);

    std::cout << "FINALIZED_SCIENTIFIC_PREVIEW_ROUTE_V0_1_PASS\n";
    std::cout << "master_tiles=" << result.scientific.masterTilesProcessed << '\n';
    std::cout << "self_gauge_samples=" << result.scientific.selfGaugeEligibleSamples << '\n';
    std::cout << "source_bound_preview_release=1\n";
    std::cout << "source_bound_full_physical_claim=0\n";
    std::cout << "independent_calibration_claim=1\n";
    std::cout << "source_mutation_rejected=1\n";
    return 0;
}
