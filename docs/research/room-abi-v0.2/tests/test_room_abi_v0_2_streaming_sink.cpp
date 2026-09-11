#include "bounded_record_file_sink_v0_1.h"
#include "truthraw/core.h"

// Reuse the already validated concrete TileNativeDngSource fixture builder in
// the same translation unit. Rename its standalone test entry point so the
// DNG fixture helpers remain single-source while this test gets its own main.
#define main truthraw_room_abi_v02_tile_source_fixture_main
#include "test_room_abi_v0_2_tile_source.cpp"
#undef main

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>
#include <unistd.h>

int main() {
    constexpr int width = 32;
    constexpr int height = 24;
    std::vector<std::uint16_t> raw(static_cast<std::size_t>(width) * height);
    for (std::size_t i = 0; i < raw.size(); ++i) {
        raw[i] = static_cast<std::uint16_t>(70U + (i * 17U) % 900U);
    }

    auto bytes = std::make_shared<DngMemSource>(make_dng(width, height, raw));
    OpenOptions open{};
    open.sourceEvidenceId = "room_abi_v02_streaming_sink_fixture";
    open.color.valid = true;
    open.color.bindingId = "room_abi_v02_streaming_sink_color";
    open.color.cameraToXyzD50 = {
        0.62F, 0.21F, 0.08F,
        0.18F, 0.71F, 0.07F,
        0.03F, 0.12F, 0.79F};

    std::unique_ptr<TileNativeDngSource> source;
    const auto openStatus = TileNativeDngSource::open(bytes, open, source);
    require(static_cast<bool>(openStatus) && source != nullptr,
            "real TileNativeDngSource opens streaming fixture");
    require(!source->audit().fullFileMaterialized && !source->audit().fullRawMaterialized,
            "source opens without full file or RAW materialization");

    char path[] = "/tmp/truthraw-room-v02-sink-XXXXXX";
    const int fd = ::mkstemp(path);
    require(fd >= 0, "temporary output file created");
    ::unlink(path);

    truthraw::room_abi::v0_2::BoundedRecordFileSink sink(fd);
    require(sink.residentBytesUpperBound() ==
                truthraw::room_abi::v0_2::BoundedRecordFileSink::kResidentAllowanceBytes,
            "sink resident bound fixed and explicit");

    truthraw::room_abi::v0_2::StreamingEndpointBinding endpoints{};
    require(truthraw::room_abi::v0_2::bind_streaming_endpoints(*source, sink, endpoints) ==
                truthraw::room_abi::v0_2::Status::Ok,
            "Room ABI binds concrete source and bounded file sink");
    require(endpoints.sourceResidentUpperBound == source->residentBytesUpperBound(),
            "source resident bound propagated exactly");
    require(endpoints.sinkResidentUpperBound == sink.residentBytesUpperBound(),
            "sink resident bound propagated exactly");

    auto reconstruction =
        std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<truthraw::SkinSafeDetailedCrispAppearance>();
    truthraw::streaming_v0_1::StreamingTruthRawProcessor processor(reconstruction, appearance);

    truthraw::streaming_v0_1::StreamingOptions options{};
    options.tile = {8, 7};
    options.workers = 1;
    options.hdrEnabled = true;
    options.streamScientificDiagnostics = true;
    options.sdrLutSize = 4096;
    options.memoryBudgetBytes = 4U * 1024U * 1024U;

    truthraw::streaming_v0_1::StreamingResult result{};
    const auto processStatus = processor.process(*source, sink, options, result);
    require(static_cast<bool>(processStatus),
            "TileNativeDngSource -> streaming processor -> bounded file sink succeeds");
    require(sink.begun() && sink.finished(), "sink completes frame lifecycle");
    require(sink.sdrRecordCount() > 1U && sink.gainRecordCount() > 0U &&
                sink.diagnosticRecordCount() > 1U,
            "processor emits multiple bounded output records");

    require(!result.memory.adapterOwnsFullRawFrame &&
                !result.memory.adapterOwnsFullSdrFrame &&
                !result.memory.adapterOwnsFullHalfGainFrame &&
                !result.memory.adapterOwnsFullDiagnosticFrame,
            "streaming processor owns no full-frame adapter buffers");
    require(result.memory.sourceResidentUpperBound == source->residentBytesUpperBound(),
            "processor accounts concrete source resident bound");
    require(result.memory.sinkResidentUpperBound == sink.residentBytesUpperBound(),
            "processor accounts concrete sink resident bound");
    require(result.provenance.physicalFrameCount == 1U &&
                result.provenance.independentEvidenceCount == 1U,
            "single-frame evidence counts preserved");
    require(!source->audit().fullRawMaterialized,
            "source remains tile-native after complete two-pass processing");

    const off_t outputBytes = ::lseek(fd, 0, SEEK_END);
    require(outputBytes > 128, "bounded sink emitted non-empty record stream");
    require(static_cast<std::uint64_t>(outputBytes) == sink.bytesWritten(),
            "sink byte accounting exact");

    std::array<char, 8> magic{};
    const ssize_t got = ::pread(fd, magic.data(), magic.size(), 0);
    require(got == static_cast<ssize_t>(magic.size()), "read bounded sink magic only");
    require(std::memcmp(magic.data(), "TRSINK01", magic.size()) == 0,
            "bounded sink research transport magic exact");

    ::close(fd);

    std::cout << "ROOM_ABI_V0_2_STREAMING_SINK_PASS\n";
    std::cout << "source_resident_bound=" << source->residentBytesUpperBound() << '\n';
    std::cout << "sink_resident_bound=" << sink.residentBytesUpperBound() << '\n';
    std::cout << "output_bytes=" << sink.bytesWritten() << '\n';
    std::cout << "sdr_records=" << sink.sdrRecordCount() << '\n';
    std::cout << "gain_records=" << sink.gainRecordCount() << '\n';
    std::cout << "diagnostic_records=" << sink.diagnosticRecordCount() << '\n';
    std::cout << "adapter_full_frame_buffers=0\n";
    std::cout << "scene_iso_axis=0\n";
    return 0;
}
