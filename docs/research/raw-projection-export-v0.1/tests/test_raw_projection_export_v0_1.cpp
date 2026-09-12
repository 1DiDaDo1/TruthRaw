#include "raw_projection_export_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "streaming_test_support_v0_1.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {

using truthraw::raw_projection_export::v0_1::ProjectionKind;
using truthraw::raw_projection_export::v0_1::Result;
using truthraw::raw_projection_export::v0_1::StatusCode;

struct TempFile final {
    std::string path;
    int fd = -1;
    TempFile() {
        char pattern[] = "/tmp/truthraw_projection_XXXXXX";
        fd = ::mkstemp(pattern);
        if (fd < 0) {
            std::perror("mkstemp");
            std::exit(3);
        }
        path = pattern;
    }
    ~TempFile() {
        if (fd >= 0) ::close(fd);
        if (!path.empty()) ::unlink(path.c_str());
    }
};

std::vector<std::uint8_t> read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    REQUIRE(in.good());
    in.seekg(0, std::ios::end);
    const auto end = in.tellg();
    REQUIRE(end >= 0);
    std::vector<std::uint8_t> data(static_cast<std::size_t>(end));
    in.seekg(0, std::ios::beg);
    if (!data.empty()) in.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    REQUIRE(in.good() || in.eof());
    return data;
}

std::uint16_t u16(const std::vector<std::uint8_t>& data, std::size_t off) {
    REQUIRE(off + 2u <= data.size());
    return static_cast<std::uint16_t>(data[off]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(data[off + 1u]) << 8u);
}

std::uint32_t u32(const std::vector<std::uint8_t>& data, std::size_t off) {
    REQUIRE(off + 4u <= data.size());
    return static_cast<std::uint32_t>(data[off]) |
           (static_cast<std::uint32_t>(data[off + 1u]) << 8u) |
           (static_cast<std::uint32_t>(data[off + 2u]) << 16u) |
           (static_cast<std::uint32_t>(data[off + 3u]) << 24u);
}

struct IfdEntry final {
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::uint32_t value = 0;
    std::size_t entryOffset = 0;
};

IfdEntry find_tag(const std::vector<std::uint8_t>& data, std::uint16_t tag) {
    REQUIRE(data.size() >= 8u);
    REQUIRE(data[0] == 'I' && data[1] == 'I');
    REQUIRE(u16(data, 2u) == 42u);
    const std::uint32_t ifd = u32(data, 4u);
    REQUIRE(ifd + 2u <= data.size());
    const std::uint16_t count = u16(data, ifd);
    for (std::uint16_t i = 0; i < count; ++i) {
        const std::size_t off = static_cast<std::size_t>(ifd) + 2u + static_cast<std::size_t>(i) * 12u;
        REQUIRE(off + 12u <= data.size());
        if (u16(data, off) == tag) {
            return {u16(data, off + 2u), u32(data, off + 4u), u32(data, off + 8u), off};
        }
    }
    std::cerr << "missing tag " << tag << "\n";
    std::exit(4);
}

std::uint16_t short_scalar(const std::vector<std::uint8_t>& data, std::uint16_t tag) {
    const auto e = find_tag(data, tag);
    REQUIRE(e.type == 3u);
    REQUIRE(e.count == 1u);
    return static_cast<std::uint16_t>(e.value & 0xffffu);
}

std::vector<std::uint32_t> long_values(const std::vector<std::uint8_t>& data, std::uint16_t tag) {
    const auto e = find_tag(data, tag);
    REQUIRE(e.type == 4u);
    REQUIRE(e.count > 0u);
    std::vector<std::uint32_t> out;
    out.reserve(e.count);
    if (e.count == 1u) {
        out.push_back(e.value);
        return out;
    }
    const std::size_t off = e.value;
    REQUIRE(off + static_cast<std::size_t>(e.count) * 4u <= data.size());
    for (std::uint32_t i = 0; i < e.count; ++i) out.push_back(u32(data, off + static_cast<std::size_t>(i) * 4u));
    return out;
}

std::vector<std::uint8_t> cfa_payload(const std::vector<std::uint8_t>& dng) {
    const auto offsets = long_values(dng, 273u);
    const auto counts = long_values(dng, 279u);
    REQUIRE(offsets.size() == counts.size());
    std::vector<std::uint8_t> payload;
    for (std::size_t i = 0; i < offsets.size(); ++i) {
        const std::size_t off = offsets[i];
        const std::size_t n = counts[i];
        REQUIRE(off + n <= dng.size());
        payload.insert(payload.end(), dng.begin() + static_cast<std::ptrdiff_t>(off),
                       dng.begin() + static_cast<std::ptrdiff_t>(off + n));
    }
    return payload;
}

bool same_hash(const truthraw::scientific_master_streaming_binding::v0_2::Hash256& a,
               const truthraw::scientific_master_streaming_binding::v0_2::Hash256& b) {
    return a == b;
}

}  // namespace

int main() {
    auto frame = make_frame(66, 50);
    FrameSource source(frame);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction;

    truthraw::scientific_master_streaming_binding::v0_2::Options scienceOptions;
    scienceOptions.memoryBudgetBytes = 64u * 1024u * 1024u;
    truthraw::scientific_master_streaming_binding::v0_2::Result before;
    auto scienceBefore = truthraw::scientific_master_streaming_binding::v0_2::bind_scientific_master_streaming(
        source, reconstruction, scienceOptions, before);
    REQUIRE(static_cast<bool>(scienceBefore));
    REQUIRE(before.stage2GaugeScanPasses == 2u);

    truthraw::raw_projection_export::v0_1::Options options;
    options.rowsPerStrip = 8;
    options.memoryBudgetBytes = 64u * 1024u * 1024u;

    TempFile rawsensorFile;
    Result rawsensor;
    auto rawStatus = truthraw::raw_projection_export::v0_1::export_projection(
        source, reconstruction, rawsensorFile.fd, ProjectionKind::RawSensorCfa16, options, rawsensor);
    REQUIRE(static_cast<bool>(rawStatus));
    REQUIRE(rawsensor.projectionOnly);
    REQUIRE(!rawsensor.sourcePixelsClaimedMeasured);
    REQUIRE(!rawsensor.fullScientificMasterMaterialized);
    REQUIRE(rawsensor.physicalFrameCount == 1u);
    REQUIRE(rawsensor.independentEvidenceCount == 1u);
    const auto rawBytes = read_file(rawsensorFile.path);
    REQUIRE(rawBytes.size() == static_cast<std::size_t>(frame.meta.width) * static_cast<std::size_t>(frame.meta.height) * 2u);
    REQUIRE(rawsensor.outputBytes == rawBytes.size());

    TempFile cfaDngFile;
    Result cfaDng;
    auto cfaStatus = truthraw::raw_projection_export::v0_1::export_projection(
        source, reconstruction, cfaDngFile.fd, ProjectionKind::ReconstructedCfaDng16, options, cfaDng);
    REQUIRE(static_cast<bool>(cfaStatus));
    const auto cfaDngBytes = read_file(cfaDngFile.path);
    REQUIRE(cfaDng.outputBytes == cfaDngBytes.size());
    REQUIRE(short_scalar(cfaDngBytes, 262u) == 32803u);
    REQUIRE(short_scalar(cfaDngBytes, 277u) == 1u);
    REQUIRE(short_scalar(cfaDngBytes, 259u) == 1u);
    REQUIRE(find_tag(cfaDngBytes, 33421u).count == 2u);
    REQUIRE(find_tag(cfaDngBytes, 33422u).count == 4u);
    REQUIRE(find_tag(cfaDngBytes, 50706u).count == 4u);
    REQUIRE(find_tag(cfaDngBytes, 50721u).count == 9u);
    REQUIRE(find_tag(cfaDngBytes, 50964u).count == 9u);
    REQUIRE(cfa_payload(cfaDngBytes) == rawBytes);

    TempFile linearDngFile;
    Result linearDng;
    auto linearStatus = truthraw::raw_projection_export::v0_1::export_projection(
        source, reconstruction, linearDngFile.fd, ProjectionKind::LinearDng16, options, linearDng);
    REQUIRE(static_cast<bool>(linearStatus));
    const auto linearBytes = read_file(linearDngFile.path);
    REQUIRE(linearDng.outputBytes == linearBytes.size());
    REQUIRE(short_scalar(linearBytes, 262u) == 34892u);
    REQUIRE(short_scalar(linearBytes, 277u) == 3u);
    REQUIRE(find_tag(linearBytes, 258u).count == 3u);
    REQUIRE(find_tag(linearBytes, 339u).count == 3u);
    REQUIRE(find_tag(linearBytes, 50721u).count == 9u);
    REQUIRE(find_tag(linearBytes, 50964u).count == 9u);
    REQUIRE(linearDng.projectedSamples == static_cast<std::uint64_t>(frame.meta.width) *
                                          static_cast<std::uint64_t>(frame.meta.height) * 3u);

    // Projection is downstream only: re-binding the Scientific Master after all
    // exports must yield the exact same digest and exact same self-gauge.
    truthraw::scientific_master_streaming_binding::v0_2::Result after;
    auto scienceAfter = truthraw::scientific_master_streaming_binding::v0_2::bind_scientific_master_streaming(
        source, reconstruction, scienceOptions, after);
    REQUIRE(static_cast<bool>(scienceAfter));
    REQUIRE(same_hash(before.scientificMasterHash, after.scientificMasterHash));
    REQUIRE(std::memcmp(&before.zeroLineGauge.L0, &after.zeroLineGauge.L0, sizeof(double)) == 0);
    REQUIRE(before.zeroLineGauge.gaugeId == after.zeroLineGauge.gaugeId);
    REQUIRE(before.sceneBinding.sceneScaleId == after.sceneBinding.sceneScaleId);

    TempFile budgetFile;
    Result budgetResult;
    auto tiny = options;
    tiny.memoryBudgetBytes = 1u;
    auto budgetStatus = truthraw::raw_projection_export::v0_1::export_projection(
        source, reconstruction, budgetFile.fd, ProjectionKind::LinearDng16, tiny, budgetResult);
    REQUIRE(!static_cast<bool>(budgetStatus));
    REQUIRE(budgetStatus.code == StatusCode::BudgetExceeded);

    std::cout << "RAW_PROJECTION_EXPORT_V0_1_PASS\n";
    std::cout << "rawsensor_bytes=" << rawBytes.size() << "\n";
    std::cout << "cfa_dng_bytes=" << cfaDngBytes.size() << "\n";
    std::cout << "linear_dng_bytes=" << linearBytes.size() << "\n";
    std::cout << "cfa_payload_matches_rawsensor=1\n";
    std::cout << "scientific_master_unchanged=1\n";
    std::cout << "projection_claims_measured_pixels=0\n";
    return 0;
}
