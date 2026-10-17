#include "tile_native_dng_source_v0_1.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace truthraw::tile_dng_v0_1;

#define REQUIRE(x) do { if(!(x)) throw std::runtime_error(std::string("REQUIRE failed: ") + #x); } while(0)

namespace {

struct MemSource final : IRandomAccessByteSource {
    explicit MemSource(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)) {}
    std::uint64_t sizeBytes() const override { return bytes_.size(); }
    std::size_t residentBytesUpperBound() const override { return sizeof(*this) + bytes_.capacity(); }
    bool readExact(std::uint64_t offset, void* dst, std::size_t count) override {
        if (offset > bytes_.size() || count > bytes_.size() - offset) return false;
        std::memcpy(dst, bytes_.data() + offset, count);
        return true;
    }
    std::vector<std::uint8_t> bytes_;
};

void put16(std::vector<std::uint8_t>& b, std::size_t offset, std::uint16_t value) {
    b[offset] = static_cast<std::uint8_t>(value & 0xffu);
    b[offset + 1] = static_cast<std::uint8_t>(value >> 8);
}

void put32(std::vector<std::uint8_t>& b, std::size_t offset, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) b[offset + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(value >> (8 * i));
}

struct DependencySpec {
    std::uint16_t tag;
    std::uint16_t type;
};

std::vector<std::uint8_t> makeMinimalCfaIfd(const DependencySpec* dependency) {
    const std::uint16_t entryCount = dependency == nullptr ? 1u : 2u;
    const std::size_t ifdEnd = 8u + 2u + static_cast<std::size_t>(entryCount) * 12u + 4u;
    const bool externalPayload = dependency != nullptr && dependency->type == 10u;
    std::vector<std::uint8_t> bytes(ifdEnd + (externalPayload ? 8u : 0u), 0u);

    bytes[0] = 'I';
    bytes[1] = 'I';
    put16(bytes, 2, 42u);
    put32(bytes, 4, 8u);
    put16(bytes, 8, entryCount);

    std::size_t entry = 10u;
    put16(bytes, entry, 262u);       // PhotometricInterpretation
    put16(bytes, entry + 2u, 3u);    // SHORT
    put32(bytes, entry + 4u, 1u);
    put16(bytes, entry + 8u, 32803u); // CFA

    if (dependency != nullptr) {
        entry += 12u;
        put16(bytes, entry, dependency->tag);
        put16(bytes, entry + 2u, dependency->type);
        put32(bytes, entry + 4u, 1u);
        if (externalPayload) {
            put32(bytes, entry + 8u, static_cast<std::uint32_t>(ifdEnd));
            put32(bytes, ifdEnd, 0u);      // SRATIONAL numerator.
            put32(bytes, ifdEnd + 4u, 1u); // SRATIONAL denominator.
        } else {
            put16(bytes, entry + 8u, 1u);
        }
    }

    put32(bytes, 8u + 2u + static_cast<std::size_t>(entryCount) * 12u, 0u);
    return bytes;
}

OpenOptions makeOptions(bool explicitAuthority) {
    OpenOptions options;
    options.sourceEvidenceId = "raw-to-linear-dependency-guard-v0.1";
    options.color.valid = true;
    options.color.bindingId = "identity-d50-test-binding";
    if (explicitAuthority) options.explicitRawIfdOffset = 8u;
    return options;
}

DngSourceStatus openFixture(const DependencySpec* dependency, bool explicitAuthority) {
    auto bytes = std::make_shared<MemSource>(makeMinimalCfaIfd(dependency));
    std::unique_ptr<TileNativeDngSource> opened;
    auto status = TileNativeDngSource::open(bytes, makeOptions(explicitAuthority), opened);
    REQUIRE(opened == nullptr);
    return status;
}

void requireBaselineStillReachesExistingBindingChecks(bool explicitAuthority) {
    const auto status = openFixture(nullptr, explicitAuthority);
    REQUIRE(!status);
    REQUIRE(status.code == DngSourceCode::MissingRequiredTag);
}

void requireUnsupportedDependencyFailsClosed(const DependencySpec dependency, bool explicitAuthority) {
    const auto status = openFixture(&dependency, explicitAuthority);
    REQUIRE(!status);
    REQUIRE(status.code == DngSourceCode::UnsupportedTopology);
    REQUIRE(status.message.find("unsupported raw-to-linear dependency tag=") != std::string::npos);
    REQUIRE(status.message.find(std::to_string(dependency.tag)) != std::string::npos);
}

} // namespace

int main() {
    try {
        requireBaselineStillReachesExistingBindingChecks(false);
        requireBaselineStillReachesExistingBindingChecks(true);

        constexpr std::array<DependencySpec, 3> dependencies{{
            {50712u, 3u},  // LinearizationTable: SHORT; presence only, semantics remain unsupported.
            {50715u, 10u}, // BlackLevelDeltaH: SRATIONAL; presence only.
            {50716u, 10u}, // BlackLevelDeltaV: SRATIONAL; presence only.
        }};
        for (const auto dependency : dependencies) {
            requireUnsupportedDependencyFailsClosed(dependency, false);
            requireUnsupportedDependencyFailsClosed(dependency, true);
        }

        std::cout << "RAW_TO_LINEAR_UNSUPPORTED_DEPENDENCY_GUARD_V0_1_PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
