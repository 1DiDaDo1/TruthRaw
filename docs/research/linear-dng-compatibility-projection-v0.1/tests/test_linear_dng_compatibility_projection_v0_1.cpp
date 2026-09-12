#include "linear_dng_compatibility_projection_v0_1.h"
#include "truthraw/core.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {

using truthraw::CfaPattern;
using truthraw::DngMetadata;
using truthraw::ReferenceMeasuredPreservingReconstruction;
using truthraw::TileRect;
using truthraw::linear_dng_compatibility_projection::v0_1::Options;
using truthraw::linear_dng_compatibility_projection::v0_1::Result;
using truthraw::linear_dng_compatibility_projection::v0_1::StatusCode;
using truthraw::scientific_preview_binding_v0_1::ColorBindingAuthority;
using truthraw::scientific_preview_binding_v0_1::ScientificColorBindingRecord;
using truthraw::streaming_v0_1::IRawTileSource;
using truthraw::streaming_v0_1::StreamStatus;
using truthraw::streaming_v0_1::StreamStatusCode;

#define CHECK(expr) do { if (!(expr)) { \
    std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); \
    return 1; \
} } while (0)

class SyntheticSource final : public IRawTileSource {
public:
    SyntheticSource() {
        metadata_.width = 17;
        metadata_.height = 19;
        metadata_.cfa = CfaPattern::BGGR;
        metadata_.whiteLevel = 1023.0f;
        metadata_.blackPhase = {64.0f, 64.0f, 64.0f, 64.0f};
        metadata_.cameraToXyzD50 = {
            0.70f, 0.20f, 0.10f,
            0.10f, 0.80f, 0.10f,
            0.05f, 0.15f, 0.80f,
        };
        metadata_.sourceId = "sha256:test-linear-dng";
    }

    const DngMetadata& metadata() const override { return metadata_; }
    std::size_t residentBytesUpperBound() const override { return 1024u; }

    StreamStatus readRawTile(
        const TileRect& rect,
        std::uint16_t* rawOut,
        std::size_t rawCount,
        float* gainOut,
        std::size_t gainCount) override {
        if (gainOut != nullptr || gainCount != 0u) {
            return StreamStatus::error(StreamStatusCode::SourceFailed,
                                       "synthetic source has no gain field");
        }
        const int width = rect.hx1 - rect.hx0;
        const int height = rect.hy1 - rect.hy0;
        const std::size_t expected =
            static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        if (rawOut == nullptr || rawCount != expected) {
            return StreamStatus::error(StreamStatusCode::SourceFailed,
                                       "synthetic raw count mismatch");
        }
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const int gx = rect.hx0 + x;
                const int gy = rect.hy0 + y;
                const int value = 96 + ((gx * 13 + gy * 17) % 760);
                rawOut[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                       static_cast<std::size_t>(x)] = static_cast<std::uint16_t>(value);
            }
        }
        ++tileReads_;
        return StreamStatus::ok();
    }

    StreamStatus readRowBias(int, int, float*, std::size_t count) override {
        if (count != 0u) {
            return StreamStatus::error(StreamStatusCode::SourceFailed,
                                       "unexpected row-bias request");
        }
        return StreamStatus::ok();
    }

    StreamStatus readColBias(int, int, float*, std::size_t count) override {
        if (count != 0u) {
            return StreamStatus::error(StreamStatusCode::SourceFailed,
                                       "unexpected col-bias request");
        }
        return StreamStatus::ok();
    }

    std::uint64_t tileReads() const { return tileReads_; }

private:
    DngMetadata metadata_{};
    std::uint64_t tileReads_ = 0u;
};

std::uint16_t le16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[1]) << 8u);
}

std::uint32_t le32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8u) |
           (static_cast<std::uint32_t>(p[2]) << 16u) |
           (static_cast<std::uint32_t>(p[3]) << 24u);
}

struct Entry final {
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::uint32_t value = 0;
    std::array<std::uint8_t, 4> raw{};
};

bool find_entry(const std::vector<std::uint8_t>& file, std::uint16_t tag, Entry& out) {
    if (file.size() < 10u || file[0] != 'I' || file[1] != 'I' || le16(file.data() + 2) != 42u) {
        return false;
    }
    const std::uint32_t ifd = le32(file.data() + 4);
    if (ifd + 2u > file.size()) return false;
    const std::uint16_t count = le16(file.data() + ifd);
    for (std::uint16_t index = 0u; index < count; ++index) {
        const std::size_t offset = static_cast<std::size_t>(ifd) + 2u + 12u * index;
        if (offset + 12u > file.size()) return false;
        if (le16(file.data() + offset) != tag) continue;
        out.type = le16(file.data() + offset + 2u);
        out.count = le32(file.data() + offset + 4u);
        out.value = le32(file.data() + offset + 8u);
        std::memcpy(out.raw.data(), file.data() + offset + 8u, 4u);
        return true;
    }
    return false;
}

ScientificColorBindingRecord make_color(const DngMetadata& metadata) {
    ScientificColorBindingRecord color;
    color.authority = ColorBindingAuthority::SourceMetadataBound;
    color.sourceEvidenceId = metadata.sourceId;
    color.bindingId = "test-linear-dng-binding";
    color.cameraToXyzD50 = metadata.cameraToXyzD50;
    color.normalized = true;
    color.validated = true;
    color.physicalFrameCount = 1u;
    color.independentEvidenceCount = 1u;
    return color;
}

int run() {
    SyntheticSource source;
    ReferenceMeasuredPreservingReconstruction reconstruction;
    const auto color = make_color(source.metadata());

    char path[] = "/tmp/truthraw-linear-dng-XXXXXX";
    const int fd = ::mkstemp(path);
    CHECK(fd >= 0);
    ::unlink(path);

    Options options;
    options.tileEdge = 16;
    options.memoryBudgetBytes = 8u * 1024u * 1024u;
    Result result;
    const auto status = truthraw::linear_dng_compatibility_projection::v0_1::write_linear_dng(
        source, reconstruction, color, fd, options, result);
    CHECK(static_cast<bool>(status));
    CHECK(result.width == 17);
    CHECK(result.height == 19);
    CHECK(result.tilesWritten == 4u);
    CHECK(result.samplesWritten == 17u * 19u * 3u);
    CHECK(result.fullFrameMaterialized == false);
    CHECK(result.appearanceApplied == false);
    CHECK(result.scientificMasterModified == false);
    CHECK(result.physicalFrameCount == 1u);
    CHECK(result.independentEvidenceCount == 1u);
    CHECK(source.tileReads() == 4u);

    struct stat st{};
    CHECK(::fstat(fd, &st) == 0);
    CHECK(st.st_size > 0);
    CHECK(static_cast<std::uint64_t>(st.st_size) == result.outputBytes);
    CHECK(::lseek(fd, 0, SEEK_SET) == 0);
    std::vector<std::uint8_t> file(static_cast<std::size_t>(st.st_size));
    std::size_t readBytes = 0u;
    while (readBytes < file.size()) {
        const ssize_t got = ::read(fd, file.data() + readBytes, file.size() - readBytes);
        CHECK(got > 0);
        readBytes += static_cast<std::size_t>(got);
    }

    Entry entry{};
    CHECK(find_entry(file, 256u, entry) && entry.value == 17u);
    CHECK(find_entry(file, 257u, entry) && entry.value == 19u);
    CHECK(find_entry(file, 262u, entry) && le16(entry.raw.data()) == 34892u);
    CHECK(find_entry(file, 277u, entry) && le16(entry.raw.data()) == 3u);
    CHECK(find_entry(file, 322u, entry) && entry.value == 16u);
    CHECK(find_entry(file, 323u, entry) && entry.value == 16u);
    CHECK(find_entry(file, 324u, entry) && entry.count == 4u);
    CHECK(find_entry(file, 325u, entry) && entry.count == 4u);
    CHECK(find_entry(file, 50706u, entry));
    CHECK(entry.raw[0] == 1u && entry.raw[1] == 4u && entry.raw[2] == 0u && entry.raw[3] == 0u);
    CHECK(find_entry(file, 50721u, entry) && entry.count == 9u);
    CHECK(find_entry(file, 50964u, entry) && entry.count == 9u);

    auto unauthorized = color;
    unauthorized.authority = ColorBindingAuthority::PreviewSentinel;
    Result rejected{};
    const auto unauthorizedStatus =
        truthraw::linear_dng_compatibility_projection::v0_1::write_linear_dng(
            source, reconstruction, unauthorized, fd, options, rejected);
    CHECK(!static_cast<bool>(unauthorizedStatus));
    CHECK(unauthorizedStatus.code == StatusCode::UnauthorizedColorBinding);

    auto mismatched = color;
    mismatched.cameraToXyzD50[0] += 0.01f;
    const auto mismatchStatus =
        truthraw::linear_dng_compatibility_projection::v0_1::write_linear_dng(
            source, reconstruction, mismatched, fd, options, rejected);
    CHECK(!static_cast<bool>(mismatchStatus));
    CHECK(mismatchStatus.code == StatusCode::ColorBindingMismatch);

    Options tinyBudget = options;
    tinyBudget.memoryBudgetBytes = 1024u;
    const auto budgetStatus =
        truthraw::linear_dng_compatibility_projection::v0_1::write_linear_dng(
            source, reconstruction, color, fd, tinyBudget, rejected);
    CHECK(!static_cast<bool>(budgetStatus));
    CHECK(budgetStatus.code == StatusCode::BudgetExceeded);

    ::close(fd);
    std::printf("LINEAR_DNG_COMPATIBILITY_PROJECTION_V0_1_PASS\n");
    std::printf("tiles=%llu output_bytes=%llu clipped_low=%llu clipped_high=%llu\n",
                static_cast<unsigned long long>(result.tilesWritten),
                static_cast<unsigned long long>(result.outputBytes),
                static_cast<unsigned long long>(result.clippedBelowZero),
                static_cast<unsigned long long>(result.clippedAboveOne));
    return 0;
}

} // namespace

int main() { return run(); }
