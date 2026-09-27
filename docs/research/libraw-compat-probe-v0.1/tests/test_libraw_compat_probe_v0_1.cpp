#include "libraw_compat_probe_v0_1.h"

#include <libraw/libraw.h>

#include <cstdlib>
#include <iostream>

using namespace truthraw::libraw_compat_probe::v0_1;
namespace ingress = truthraw::professional_raw_ingress::v0_1;

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(2);
    }
}
}

int main() {
    require(LibRaw::version() != nullptr && *LibRaw::version() != '\0',
            "LibRaw version must be available");
    require(LibRaw::cameraCount() > 0, "LibRaw camera list must be non-empty");

    require(classify_topology({1, 0, 0, 3, 0x94949494u}) ==
                ingress::MeasurementTopology::Bayer2x2,
            "ordinary 3-color Bayer mask should classify Bayer2x2");
    require(classify_topology({1, 0, 0, 3, 9}) ==
                ingress::MeasurementTopology::XTrans6x6,
            "filters=9 must classify X-Trans");
    require(classify_topology({1, 0, 0, 3, 1}) ==
                ingress::MeasurementTopology::Unknown,
            "special Leaf filters=1 must not be coerced to Bayer");
    require(classify_topology({1, 1, 0, 3, 0}) ==
                ingress::MeasurementTopology::LayeredFoveon,
            "Foveon flag must win topology classification");
    require(classify_topology({1, 0, 0, 1, 0}) ==
                ingress::MeasurementTopology::Monochrome,
            "one-color full sample should classify monochrome");
    require(classify_topology({1, 0, 0, 3, 0}) ==
                ingress::MeasurementTopology::LinearRgb,
            "full-color 3-component source should classify linear RGB");
    require(classify_topology({0, 0, 0, 3, 0x94949494u}) ==
                ingress::MeasurementTopology::Unknown,
            "raw_count=0 must stay unknown");

    require(classify_container({1, 0, 0x01070000u, 3, 0x94949494u}) ==
                ingress::ContainerFamily::Dng,
            "nonzero DNG version must classify DNG");
    require(classify_container({1, 0, 0, 3, 0x94949494u}) ==
                ingress::ContainerFamily::Unknown,
            "non-DNG vendor container must remain unknown without signature router");

    ProbeResult missing{};
    const auto missingStatus = probe_file(
        "/definitely/not/a/real/truthraw/raw/file.CR3", missing);
    require(missingStatus.code == ProbeStatusCode::OpenFailed,
            "missing file must fail open");
    require(!missing.pixelsUnpacked, "probe must never report unpacked pixels");

    ProbeResult floor{};
    floor.rawWidth = 16320;
    floor.rawHeight = 12288;
    floor.colors = 3;
    floor.topology = ingress::MeasurementTopology::Bayer2x2;
    require(raw_sample_floor_bytes(floor) == 401080320ull,
            "200.5MP Bayer uint16 sample floor must be deterministic");

    std::cout
        << "LIBRAW_COMPAT_PROBE_V0_1_TEST_PASS\n"
        << "libraw_version=" << LibRaw::version() << "\n"
        << "camera_count=" << LibRaw::cameraCount() << "\n"
        << "probe_unpack_calls=0\n"
        << "extension_as_evidence=0\n"
        << "bayer_classification=1\n"
        << "xtrans_preserved=1\n"
        << "special_filter_fail_closed=1\n";
    return 0;
}
