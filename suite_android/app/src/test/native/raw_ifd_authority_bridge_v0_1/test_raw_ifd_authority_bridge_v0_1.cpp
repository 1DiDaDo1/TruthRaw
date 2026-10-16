#include "raw_source_adapter_bridge_common.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace bridge = truthraw::android_raw_adapter_bridge::v0_1;
namespace adapter = truthraw::multivendor_raw_source_adapter::v0_1;
namespace ifd_authority = truthraw::raw_ifd_authority::v0_1;
namespace binding = truthraw::scientific_preview_binding_v0_1;
namespace dng = truthraw::tile_dng_v0_1;

#define REQUIRE(x) \
    do { \
        if (!(x)) throw std::runtime_error(std::string("REQUIRE failed: ") + #x); \
    } while (0)

namespace {

class DngMemSource final : public dng::IRandomAccessByteSource {
public:
    explicit DngMemSource(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)) {}

    std::uint64_t sizeBytes() const override { return bytes_.size(); }
    std::size_t residentBytesUpperBound() const override {
        return sizeof(*this) + bytes_.capacity();
    }
    bool readExact(std::uint64_t offset, void* dst, std::size_t count) override {
        if (offset > bytes_.size() || count > bytes_.size() - static_cast<std::size_t>(offset)) {
            return false;
        }
        if (count != 0u) {
            std::memcpy(dst, bytes_.data() + static_cast<std::size_t>(offset), count);
        }
        return true;
    }

private:
    std::vector<std::uint8_t> bytes_;
};

void put16(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t value) {
    if (bytes.size() < offset + 2u) bytes.resize(offset + 2u);
    bytes[offset] = static_cast<std::uint8_t>(value);
    bytes[offset + 1u] = static_cast<std::uint8_t>(value >> 8u);
}

void put32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
    if (bytes.size() < offset + 4u) bytes.resize(offset + 4u);
    for (int i = 0; i < 4; ++i) {
        bytes[offset + static_cast<std::size_t>(i)] =
            static_cast<std::uint8_t>(value >> (8u * static_cast<unsigned>(i)));
    }
}

void put64(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint64_t value) {
    if (bytes.size() < offset + 8u) bytes.resize(offset + 8u);
    for (int i = 0; i < 8; ++i) {
        bytes[offset + static_cast<std::size_t>(i)] =
            static_cast<std::uint8_t>(value >> (8u * static_cast<unsigned>(i)));
    }
}

std::vector<std::uint8_t> shorts(std::initializer_list<std::uint16_t> values) {
    std::vector<std::uint8_t> bytes(values.size() * 2u);
    std::size_t offset = 0u;
    for (const auto value : values) {
        put16(bytes, offset, value);
        offset += 2u;
    }
    return bytes;
}

std::vector<std::uint8_t> longs(const std::vector<std::uint32_t>& values) {
    std::vector<std::uint8_t> bytes(values.size() * 4u);
    for (std::size_t i = 0; i < values.size(); ++i) put32(bytes, i * 4u, values[i]);
    return bytes;
}

std::vector<std::uint8_t> rationals(const std::array<std::uint32_t, 4>& values) {
    std::vector<std::uint8_t> bytes(32u);
    for (std::size_t i = 0; i < values.size(); ++i) {
        put32(bytes, i * 8u, values[i]);
        put32(bytes, i * 8u + 4u, 1u);
    }
    return bytes;
}

std::vector<std::uint8_t> doubles(const std::array<double, 6>& values) {
    std::vector<std::uint8_t> bytes(48u);
    for (std::size_t i = 0; i < values.size(); ++i) {
        std::uint64_t bits = 0u;
        std::memcpy(&bits, &values[i], sizeof(bits));
        put64(bytes, i * 8u, bits);
    }
    return bytes;
}

struct Entry final {
    std::uint16_t tag = 0u;
    std::uint16_t type = 0u;
    std::uint32_t count = 0u;
    std::vector<std::uint8_t> data;
    std::size_t entryOffset = 0u;
    std::size_t payloadOffset = 0u;
};

std::vector<std::uint8_t> makeDngFixture() {
    constexpr int width = 8;
    constexpr int height = 6;
    std::vector<Entry> entries;
    const auto add = [&](std::uint16_t tag,
                         std::uint16_t type,
                         std::uint32_t count,
                         std::vector<std::uint8_t> data) {
        entries.push_back({tag, type, count, std::move(data), 0u, 0u});
    };

    add(256, 4, 1, longs({width}));
    add(257, 4, 1, longs({height}));
    add(258, 3, 1, shorts({16}));
    add(259, 3, 1, shorts({1}));
    add(262, 3, 1, shorts({32803}));
    add(274, 3, 1, shorts({1}));
    add(277, 3, 1, shorts({1}));
    add(284, 3, 1, shorts({1}));
    add(339, 3, 1, shorts({1}));
    add(33421, 3, 2, shorts({2, 2}));
    add(33422, 1, 4, {2, 1, 1, 0});
    add(50710, 1, 3, {0, 1, 2});
    add(50713, 3, 2, shorts({2, 2}));
    add(50714, 5, 4, rationals({64, 65, 66, 67}));
    add(50717, 4, 1, longs({1023}));
    add(51041, 12, 6, doubles({0.0009, 1e-6, 0.0010, 1.2e-6, 0.0011, 1.4e-6}));

    constexpr std::uint32_t rowsPerStrip = 2u;
    constexpr std::uint32_t stripCount = 3u;
    add(273, 4, stripCount, std::vector<std::uint8_t>(stripCount * 4u));
    add(278, 4, 1, longs({rowsPerStrip}));
    add(279, 4, stripCount, std::vector<std::uint8_t>(stripCount * 4u));

    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        return a.tag < b.tag;
    });

    std::vector<std::uint8_t> bytes(8u + 2u + entries.size() * 12u + 4u, 0u);
    bytes[0] = 'I';
    bytes[1] = 'I';
    put16(bytes, 2u, 42u);
    put32(bytes, 4u, 8u);
    put16(bytes, 8u, static_cast<std::uint16_t>(entries.size()));

    std::size_t externalOffset = bytes.size();
    for (std::size_t i = 0; i < entries.size(); ++i) {
        auto& entry = entries[i];
        entry.entryOffset = 10u + i * 12u;
        put16(bytes, entry.entryOffset, entry.tag);
        put16(bytes, entry.entryOffset + 2u, entry.type);
        put32(bytes, entry.entryOffset + 4u, entry.count);
        if (entry.data.size() <= 4u) {
            std::copy(
                entry.data.begin(),
                entry.data.end(),
                bytes.begin() + static_cast<std::ptrdiff_t>(entry.entryOffset + 8u));
        } else {
            entry.payloadOffset = externalOffset;
            put32(bytes, entry.entryOffset + 8u, static_cast<std::uint32_t>(externalOffset));
            bytes.insert(bytes.end(), entry.data.begin(), entry.data.end());
            externalOffset = bytes.size();
        }
    }

    std::vector<std::uint32_t> stripOffsets;
    std::vector<std::uint32_t> stripByteCounts;
    for (std::uint32_t strip = 0; strip < stripCount; ++strip) {
        const int y0 = static_cast<int>(strip * rowsPerStrip);
        const int rows = std::min<int>(static_cast<int>(rowsPerStrip), height - y0);
        stripOffsets.push_back(static_cast<std::uint32_t>(bytes.size()));
        stripByteCounts.push_back(static_cast<std::uint32_t>(rows * width * 2));
        for (int y = y0; y < y0 + rows; ++y) {
            for (int x = 0; x < width; ++x) {
                const auto offset = bytes.size();
                bytes.resize(offset + 2u);
                put16(bytes, offset, static_cast<std::uint16_t>(64 + y * 100 + x));
            }
        }
    }

    for (const auto& entry : entries) {
        if (entry.tag == 273u) {
            const auto data = longs(stripOffsets);
            std::copy(
                data.begin(),
                data.end(),
                bytes.begin() + static_cast<std::ptrdiff_t>(entry.payloadOffset));
        } else if (entry.tag == 279u) {
            const auto data = longs(stripByteCounts);
            std::copy(
                data.begin(),
                data.end(),
                bytes.begin() + static_cast<std::ptrdiff_t>(entry.payloadOffset));
        }
    }

    return bytes;
}

std::shared_ptr<DngMemSource> makeSource() {
    return std::make_shared<DngMemSource>(makeDngFixture());
}

binding::SourceSeal makeSeal(
    const std::shared_ptr<DngMemSource>& source,
    std::uint8_t seed,
    const char* evidenceId) {
    binding::SourceSeal seal;
    for (std::size_t i = 0; i < seal.sha256.size(); ++i) {
        seal.sha256[i] = static_cast<std::uint8_t>(seed + static_cast<std::uint8_t>(i));
    }
    seal.byteLength = source->sizeBytes();
    seal.sourceEvidenceId = evidenceId;
    return seal;
}

dng::OpenOptions makeOpenOptions() {
    dng::OpenOptions options;
    options.color.valid = true;
    options.color.bindingId = "raw-ifd-authority-bridge-fixture-color";
    options.color.cameraToXyzD50 = {1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f};
    options.maxResidentBytes = 1024u * 1024u;
    return options;
}

void testPostOpenEqualityGuard() {
    const auto equal = bridge::verifyOpenedRawIfdAuthority(8u, 8u);
    REQUIRE(equal);

    const auto mismatch = bridge::verifyOpenedRawIfdAuthority(8u, 16u);
    REQUIRE(!mismatch);
    REQUIRE(mismatch.code == adapter::AdapterStatusCode::SourceSealMismatch);

    const auto legacyNoAuthority = bridge::verifyOpenedRawIfdAuthority(0u, 16u);
    REQUIRE(legacyNoAuthority);
}

void testValidatedAuthorityReachesExactOpenedSourceAudit() {
    auto source = makeSource();
    const auto seal = makeSeal(source, 0x10u, "raw-ifd-authority-exact-audit");
    REQUIRE(ifd_authority::registerValidated(seal.sha256, 8u));

    auto options = makeOpenOptions();
    bridge::OpenedDngSource opened;
    const auto status = bridge::openDngViaAdapter(source, seal, options, opened);

    REQUIRE(status);
    REQUIRE(opened.source != nullptr);
    REQUIRE(opened.dngAuditSource != nullptr);
    REQUIRE(opened.dngAuditSource->audit().rawIfdOffset == 8u);
    REQUIRE(opened.descriptor.sourceSealAcceptedAtBoundary);
    REQUIRE(opened.descriptor.decoderId ==
            "truthraw.tile-native-dng-source.v0.2-android-dngcreator-compat");
}

void testPreparedOffsetContradictionFailsClosedBeforeOpen() {
    auto source = makeSource();
    const auto seal = makeSeal(source, 0x40u, "raw-ifd-authority-conflict");
    REQUIRE(ifd_authority::registerValidated(seal.sha256, 8u));

    auto options = makeOpenOptions();
    options.explicitRawIfdOffset = 16u;
    bridge::OpenedDngSource opened;
    const auto status = bridge::openDngViaAdapter(source, seal, options, opened);

    REQUIRE(!status);
    REQUIRE(status.code == adapter::AdapterStatusCode::SourceSealMismatch);
    REQUIRE(opened.source == nullptr);
    REQUIRE(opened.dngAuditSource == nullptr);
}

void testNoAuthorityPreservesLegacyUniqueIfdOpen() {
    auto source = makeSource();
    const auto seal = makeSeal(source, 0x80u, "raw-ifd-authority-legacy-none");

    auto options = makeOpenOptions();
    REQUIRE(options.explicitRawIfdOffset == 0u);
    bridge::OpenedDngSource opened;
    const auto status = bridge::openDngViaAdapter(source, seal, options, opened);

    REQUIRE(status);
    REQUIRE(opened.dngAuditSource != nullptr);
    REQUIRE(opened.dngAuditSource->audit().rawIfdOffset == 8u);
}

} // namespace

int main() {
    testPostOpenEqualityGuard();
    testValidatedAuthorityReachesExactOpenedSourceAudit();
    testPreparedOffsetContradictionFailsClosedBeforeOpen();
    testNoAuthorityPreservesLegacyUniqueIfdOpen();

    std::cout << "RAW_IFD_AUTHORITY_BRIDGE_V0_1_PASS\n";
    return 0;
}
