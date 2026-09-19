#include "multivendor_raw_source_adapter_v0_1.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

namespace tr = truthraw::multivendor_raw_source_adapter::v0_1;
namespace streaming = truthraw::streaming_v0_1;

namespace {

class VectorByteSource final : public tr::IRawByteSource {
public:
    explicit VectorByteSource(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)) {}

    std::uint64_t sizeBytes() const noexcept override { return bytes_.size(); }
    std::size_t residentBytesUpperBound() const noexcept override {
        return sizeof(*this) + bytes_.capacity();
    }

    bool readExact(std::uint64_t offset, void* dst, std::size_t count) noexcept override {
        if (dst == nullptr && count != 0u) return false;
        if (offset > bytes_.size() || count > bytes_.size() - static_cast<std::size_t>(offset)) return false;
        if (count != 0u) {
            std::memcpy(dst, bytes_.data() + static_cast<std::size_t>(offset), count);
        }
        return true;
    }

private:
    std::vector<std::uint8_t> bytes_;
};

void appendLe16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
}

void appendLe32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 16u) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 24u) & 0xffu));
}

std::vector<std::uint8_t> makeSyntheticFixture(std::uint32_t width, std::uint32_t height) {
    std::vector<std::uint8_t> bytes;
    const std::array<std::uint8_t, 8> magic = {'T','R','A','W','V','0','0','1'};
    bytes.insert(bytes.end(), magic.begin(), magic.end());
    appendLe32(bytes, width);
    appendLe32(bytes, height);
    bytes.push_back(static_cast<std::uint8_t>(truthraw::CfaPattern::RGGB));
    bytes.push_back(1u);
    appendLe16(bytes, 4095u);
    appendLe16(bytes, 64u);
    appendLe16(bytes, 65u);
    appendLe16(bytes, 66u);
    appendLe16(bytes, 67u);

    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            appendLe16(bytes, static_cast<std::uint16_t>(100u + y * width + x));
        }
    }
    return bytes;
}

tr::RawSourceOpenRequest requestFor(
    tr::RawFormatFamily format,
    std::uint64_t byteLength,
    const char* sourceId) {
    tr::RawSourceOpenRequest request;
    request.declaredFormat = format;
    request.sourceSeal.valid = true;
    request.sourceSeal.byteLength = byteLength;
    request.sourceSeal.sourceEvidenceId = sourceId;
    for (std::size_t i = 0; i < request.sourceSeal.sha256.size(); ++i) {
        request.sourceSeal.sha256[i] = static_cast<std::uint8_t>(i + 1u);
    }
    request.color.valid = true;
    request.color.bindingId = "synthetic-test-color-binding";
    request.color.cameraToXyzD50 = {1,0,0, 0,1,0, 0,0,1};
    request.maxResidentBytes = 1024u * 1024u;
    return request;
}

void testRegistryAndPendingVendorFailClosed() {
    tr::RawSourceAdapterRegistry registry;
    assert(registry.registerAdapter(tr::makeDngAdapter()));
    assert(registry.registerAdapter(tr::makeSyntheticConformanceFixtureAdapter()));

    const auto duplicate = registry.registerAdapter(tr::makeDngAdapter());
    assert(!duplicate);
    assert(duplicate.code == tr::AdapterStatusCode::DuplicateAdapter);

    auto bytes = std::make_shared<VectorByteSource>(makeSyntheticFixture(8u, 6u));
    auto request = requestFor(tr::RawFormatFamily::CanonCr3, bytes->sizeBytes(), "pending-cr3");
    std::unique_ptr<streaming::IRawTileSource> source;
    tr::RawSourceDescriptor descriptor;
    const auto opened = registry.open(bytes, request, source, descriptor);
    assert(!opened);
    assert(opened.code == tr::AdapterStatusCode::AdapterUnavailable);
    assert(!source);
}

void testSealLengthMismatchFailsBeforeDecode() {
    tr::RawSourceAdapterRegistry registry;
    assert(registry.registerAdapter(tr::makeSyntheticConformanceFixtureAdapter()));

    auto bytes = std::make_shared<VectorByteSource>(makeSyntheticFixture(8u, 6u));
    auto request = requestFor(
        tr::RawFormatFamily::SyntheticConformanceFixture,
        bytes->sizeBytes() + 1u,
        "synthetic-length-mismatch");

    std::unique_ptr<streaming::IRawTileSource> source;
    tr::RawSourceDescriptor descriptor;
    const auto opened = registry.open(bytes, request, source, descriptor);
    assert(!opened);
    assert(opened.code == tr::AdapterStatusCode::SourceSealMismatch);
    assert(!source);
}

void testSyntheticNonDngAdapterPopulatesCommonTileAbi() {
    constexpr std::uint32_t width = 8u;
    constexpr std::uint32_t height = 6u;

    tr::RawSourceAdapterRegistry registry;
    assert(registry.registerAdapter(tr::makeSyntheticConformanceFixtureAdapter()));

    auto bytes = std::make_shared<VectorByteSource>(makeSyntheticFixture(width, height));
    auto request = requestFor(
        tr::RawFormatFamily::SyntheticConformanceFixture,
        bytes->sizeBytes(),
        "synthetic-nondng-adapter-fixture");

    std::unique_ptr<streaming::IRawTileSource> source;
    tr::RawSourceDescriptor descriptor;
    const auto opened = registry.open(bytes, request, source, descriptor);
    assert(opened);
    assert(source);

    assert(descriptor.format == tr::RawFormatFamily::SyntheticConformanceFixture);
    assert(descriptor.decoderId == "truthraw.synthetic-proprietary-conformance.v0.1");
    assert(descriptor.sourceSealVerifiedAtBoundary);
    assert(descriptor.exactCfaSamplesAvailable);
    assert(descriptor.scientificColorBindingProvided);
    assert(descriptor.syntheticConformanceOnly);
    assert(!descriptor.directSensorAdcClaimAllowed);
    assert(!descriptor.fullRawFrameMaterialized);

    const auto& metadata = source->metadata();
    assert(metadata.width == static_cast<int>(width));
    assert(metadata.height == static_cast<int>(height));
    assert(metadata.cfa == truthraw::CfaPattern::RGGB);
    assert(metadata.whiteLevel == 4095.0f);
    assert(metadata.blackPhase[0] == 64.0f);
    assert(metadata.blackPhase[3] == 67.0f);
    assert(metadata.sourceId == "synthetic-nondng-adapter-fixture");

    truthraw::TileRect rect{};
    rect.x0 = 2;
    rect.y0 = 2;
    rect.x1 = 6;
    rect.y1 = 4;
    rect.hx0 = 1;
    rect.hy0 = 1;
    rect.hx1 = 7;
    rect.hy1 = 5;

    const std::size_t tileWidth = 6u;
    const std::size_t tileHeight = 4u;
    std::vector<std::uint16_t> raw(tileWidth * tileHeight, 0u);
    const auto read = source->readRawTile(rect, raw.data(), raw.size(), nullptr, 0u);
    assert(read);

    std::size_t i = 0u;
    for (int y = rect.hy0; y < rect.hy1; ++y) {
        for (int x = rect.hx0; x < rect.hx1; ++x) {
            const auto expected = static_cast<std::uint16_t>(100 + y * static_cast<int>(width) + x);
            assert(raw[i++] == expected);
        }
    }

    std::vector<float> rowBias(3u, 1.0f);
    assert(source->readRowBias(1, 4, rowBias.data(), rowBias.size()));
    assert(std::all_of(rowBias.begin(), rowBias.end(), [](float v) { return v == 0.0f; }));

    std::vector<float> colBias(4u, 1.0f);
    assert(source->readColBias(2, 6, colBias.data(), colBias.size()));
    assert(std::all_of(colBias.begin(), colBias.end(), [](float v) { return v == 0.0f; }));
}

} // namespace

int main() {
    testRegistryAndPendingVendorFailClosed();
    testSealLengthMismatchFailsBeforeDecode();
    testSyntheticNonDngAdapterPopulatesCommonTileAbi();

    std::cout << "multivendor raw source adapter v0.1: PASS\n";
    return 0;
}
