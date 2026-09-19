#include "multivendor_raw_source_adapter_v0_1.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace tr = truthraw::multivendor_raw_source_adapter::v0_1;
namespace streaming = truthraw::streaming_v0_1;

namespace {

class VectorByteSource final : public tr::IRawByteSource {
public:
    explicit VectorByteSource(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)) {}
    std::uint64_t sizeBytes() const noexcept override { return bytes_.size(); }
    std::size_t residentBytesUpperBound() const noexcept override { return sizeof(*this) + bytes_.capacity(); }
    bool readExact(std::uint64_t offset, void* dst, std::size_t count) noexcept override {
        if (offset > bytes_.size() || count > bytes_.size() - static_cast<std::size_t>(offset)) return false;
        if (count) std::memcpy(dst, bytes_.data() + static_cast<std::size_t>(offset), count);
        return true;
    }
private:
    std::vector<std::uint8_t> bytes_;
};

void put16(std::vector<std::uint8_t>& b, std::size_t off, std::uint16_t v) {
    if (b.size() < off + 2) b.resize(off + 2);
    b[off] = static_cast<std::uint8_t>(v & 0xffu);
    b[off + 1] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
}

void put32(std::vector<std::uint8_t>& b, std::size_t off, std::uint32_t v) {
    if (b.size() < off + 4) b.resize(off + 4);
    b[off] = static_cast<std::uint8_t>(v & 0xffu);
    b[off + 1] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
    b[off + 2] = static_cast<std::uint8_t>((v >> 16u) & 0xffu);
    b[off + 3] = static_cast<std::uint8_t>((v >> 24u) & 0xffu);
}

void putEntry(
    std::vector<std::uint8_t>& b,
    std::size_t off,
    std::uint16_t tag,
    std::uint16_t type,
    std::uint32_t count,
    std::uint32_t valueOrOffset) {
    put16(b, off, tag);
    put16(b, off + 2, type);
    put32(b, off + 4, count);
    put32(b, off + 8, valueOrOffset);
}

std::vector<std::uint8_t> makeNefLikeFixture() {
    constexpr std::uint32_t width = 8;
    constexpr std::uint32_t height = 6;
    constexpr std::uint32_t ifd0 = 8;
    constexpr std::uint16_t entryCount = 13;
    constexpr std::uint32_t makeOffset = 200;
    constexpr std::uint32_t modelOffset = 224;
    constexpr std::uint32_t stripOffset = 256;
    constexpr std::uint32_t rawBytes = width * height * 2;

    std::vector<std::uint8_t> b(stripOffset + rawBytes, 0);
    b[0] = 'I'; b[1] = 'I';
    put16(b, 2, 42);
    put32(b, 4, ifd0);

    put16(b, ifd0, entryCount);
    std::size_t e = ifd0 + 2;
    putEntry(b, e, 256, 4, 1, width); e += 12;
    putEntry(b, e, 257, 4, 1, height); e += 12;
    putEntry(b, e, 258, 3, 1, 16); e += 12;
    putEntry(b, e, 259, 3, 1, 1); e += 12;
    putEntry(b, e, 262, 3, 1, 32803); e += 12;
    putEntry(b, e, 271, 2, 18, makeOffset); e += 12;
    putEntry(b, e, 272, 2, 9, modelOffset); e += 12;
    putEntry(b, e, 273, 4, 1, stripOffset); e += 12;
    putEntry(b, e, 277, 3, 1, 1); e += 12;
    putEntry(b, e, 278, 4, 1, height); e += 12;
    putEntry(b, e, 279, 4, 1, rawBytes); e += 12;
    // 2 SHORTs inline => little-endian bytes 02 00 02 00
    putEntry(b, e, 33421, 3, 2, 0x00020002u); e += 12;
    // 4 BYTEs inline => RGGB = 00 01 01 02
    putEntry(b, e, 33422, 1, 4, 0x02010100u); e += 12;
    put32(b, e, 0);

    const std::string make = "NIKON CORPORATION";
    std::copy(make.begin(), make.end(), b.begin() + makeOffset);
    b[makeOffset + make.size()] = 0;
    const std::string model = "NIKON Z8";
    std::copy(model.begin(), model.end(), b.begin() + modelOffset);
    b[modelOffset + model.size()] = 0;

    std::size_t p = stripOffset;
    for (std::uint16_t i = 0; i < width * height; ++i) {
        const std::uint16_t v = static_cast<std::uint16_t>(1000 + i);
        b[p++] = static_cast<std::uint8_t>(v & 0xffu);
        b[p++] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
    }
    return b;
}

tr::RawSourceOpenRequest makeRequest(std::uint64_t size) {
    tr::RawSourceOpenRequest r;
    r.declaredFormat = tr::RawFormatFamily::NikonNef;
    r.sourceSeal.valid = true;
    r.sourceSeal.byteLength = size;
    r.sourceSeal.sourceEvidenceId = "synthetic-nef-like-container-fixture";
    for (std::size_t i = 0; i < r.sourceSeal.sha256.size(); ++i) {
        r.sourceSeal.sha256[i] = static_cast<std::uint8_t>(0xa0u + i);
    }
    r.color.valid = false;
    r.maxResidentBytes = 1024 * 1024;
    return r;
}

void testStrictNefSampleDecodeAndScientificBlock() {
    auto bytes = std::make_shared<VectorByteSource>(makeNefLikeFixture());

    tr::RawSourceAdapterRegistry registry;
    assert(registry.registerAdapter(tr::makeNikonNefUncompressedAdapter()));

    std::unique_ptr<streaming::IRawTileSource> source;
    tr::RawSourceDescriptor descriptor;
    const auto opened = registry.open(bytes, makeRequest(bytes->sizeBytes()), source, descriptor);
    assert(opened);
    assert(source);

    assert(descriptor.format == tr::RawFormatFamily::NikonNef);
    assert(descriptor.decoderId == "truthraw.nikon-nef-uncompressed16-cfa.v0.1");
    assert(descriptor.sourceSealAcceptedAtBoundary);
    assert(descriptor.cameraMake == "NIKON CORPORATION");
    assert(descriptor.cameraModel == "NIKON Z8");
    assert(descriptor.rawWidth == 8);
    assert(descriptor.rawHeight == 6);
    assert(descriptor.cfaCode == static_cast<int>(truthraw::CfaPattern::RGGB));
    assert(descriptor.storageBitsPerSample == 16);
    assert(descriptor.exactCfaSamplesAvailable);
    assert(descriptor.measurementAdmissionReady);
    assert(!descriptor.scientificColorBindingProvided);
    assert(!descriptor.radiometricBindingProvided);
    assert(!descriptor.blackLevelAuthoritative);
    assert(!descriptor.saturationLevelAuthoritative);
    assert(!descriptor.scientificAdmissionReady);
    assert(!descriptor.directSensorAdcClaimAllowed);
    assert(!descriptor.fullRawFrameMaterialized);

    const auto& m = source->metadata();
    assert(m.width == 8);
    assert(m.height == 6);
    assert(m.cfa == truthraw::CfaPattern::RGGB);
    assert(m.sourceId == "synthetic-nef-like-container-fixture");

    truthraw::TileRect rect{};
    rect.x0 = 2; rect.y0 = 1; rect.x1 = 6; rect.y1 = 4;
    rect.hx0 = 1; rect.hy0 = 1; rect.hx1 = 7; rect.hy1 = 5;

    std::vector<std::uint16_t> raw(6u * 4u, 0);
    const auto read = source->readRawTile(rect, raw.data(), raw.size(), nullptr, 0);
    assert(read);

    std::size_t i = 0;
    for (int y = 1; y < 5; ++y) {
        for (int x = 1; x < 7; ++x) {
            assert(raw[i++] == static_cast<std::uint16_t>(1000 + y * 8 + x));
        }
    }
}

void testExactScopeRadiometricBindingClosesBlackAndSaturationOnly() {
    auto bytes = std::make_shared<VectorByteSource>(makeNefLikeFixture());
    auto request = makeRequest(bytes->sizeBytes());
    request.radiometric.valid = true;
    request.radiometric.authority = tr::RadiometricBindingAuthority::CalibrationPackValidated;
    request.radiometric.bindingId = "nikon-z8-test-radiometric-pack";
    request.radiometric.format = tr::RawFormatFamily::NikonNef;
    request.radiometric.cameraMake = "NIKON CORPORATION";
    request.radiometric.cameraModel = "NIKON Z8";
    request.radiometric.width = 8;
    request.radiometric.height = 6;
    request.radiometric.cfaCode = static_cast<int>(truthraw::CfaPattern::RGGB);
    request.radiometric.storageBitsPerSample = 16;
    request.radiometric.blackPhase = {64.0f, 65.0f, 66.0f, 67.0f};
    request.radiometric.whiteLevel = 16383.0f;

    tr::RawSourceAdapterRegistry registry;
    assert(registry.registerAdapter(tr::makeNikonNefUncompressedAdapter()));

    std::unique_ptr<streaming::IRawTileSource> source;
    tr::RawSourceDescriptor descriptor;
    const auto opened = registry.open(bytes, request, source, descriptor);
    assert(opened);
    assert(source);
    assert(descriptor.radiometricBindingProvided);
    assert(descriptor.blackLevelAuthoritative);
    assert(descriptor.saturationLevelAuthoritative);
    assert(descriptor.measurementAdmissionReady);
    assert(!descriptor.scientificAdmissionReady);
    assert(!descriptor.scientificColorBindingProvided);

    const auto& m = source->metadata();
    assert(m.blackPhase[0] == 64.0f);
    assert(m.blackPhase[1] == 65.0f);
    assert(m.blackPhase[2] == 66.0f);
    assert(m.blackPhase[3] == 67.0f);
    assert(m.whiteLevel == 16383.0f);
}

void testRadiometricScopeMismatchFailsClosed() {
    auto bytes = std::make_shared<VectorByteSource>(makeNefLikeFixture());
    auto request = makeRequest(bytes->sizeBytes());
    request.radiometric.valid = true;
    request.radiometric.authority = tr::RadiometricBindingAuthority::CalibrationPackValidated;
    request.radiometric.bindingId = "wrong-camera-pack";
    request.radiometric.format = tr::RawFormatFamily::NikonNef;
    request.radiometric.cameraMake = "NIKON CORPORATION";
    request.radiometric.cameraModel = "NIKON Z9";
    request.radiometric.width = 8;
    request.radiometric.height = 6;
    request.radiometric.cfaCode = static_cast<int>(truthraw::CfaPattern::RGGB);
    request.radiometric.storageBitsPerSample = 16;
    request.radiometric.blackPhase = {64.0f, 64.0f, 64.0f, 64.0f};
    request.radiometric.whiteLevel = 16383.0f;

    tr::RawSourceAdapterRegistry registry;
    assert(registry.registerAdapter(tr::makeNikonNefUncompressedAdapter()));

    std::unique_ptr<streaming::IRawTileSource> source;
    tr::RawSourceDescriptor descriptor;
    const auto opened = registry.open(bytes, request, source, descriptor);
    assert(!opened);
    assert(opened.code == tr::AdapterStatusCode::InvalidArgument);
    assert(!source);
}

void testCompressedNefFailsClosed() {
    auto fixture = makeNefLikeFixture();
    // Compression tag is entry 4 (0-based index 3), value at IFD0+2+3*12+8.
    put32(fixture, 8 + 2 + 3 * 12 + 8, 34713u);
    auto bytes = std::make_shared<VectorByteSource>(fixture);

    tr::RawSourceAdapterRegistry registry;
    assert(registry.registerAdapter(tr::makeNikonNefUncompressedAdapter()));

    std::unique_ptr<streaming::IRawTileSource> source;
    tr::RawSourceDescriptor descriptor;
    const auto opened = registry.open(bytes, makeRequest(bytes->sizeBytes()), source, descriptor);
    assert(!opened);
    assert(opened.code == tr::AdapterStatusCode::UnsupportedContainerFeature);
    assert(!source);
}

} // namespace

int main() {
    testStrictNefSampleDecodeAndScientificBlock();
    testExactScopeRadiometricBindingClosesBlackAndSaturationOnly();
    testRadiometricScopeMismatchFailsClosed();
    testCompressedNefFailsClosed();
    std::cout << "nikon nef uncompressed sample adapter v0.1: PASS\n";
    return 0;
}
