#include "libraw_compat_probe_v0_1.h"

#include <libraw/libraw.h>

#include <limits>

namespace truthraw::libraw_compat_probe::v0_1 {
namespace {
std::string safe_string(const char* s) {
    return s ? std::string{s} : std::string{};
}
}

ingress::MeasurementTopology classify_topology(const TopologyInput& input) noexcept {
    if (input.rawCount == 0) return ingress::MeasurementTopology::Unknown;
    if (input.isFoveon != 0) return ingress::MeasurementTopology::LayeredFoveon;
    if (input.filters == 9) return ingress::MeasurementTopology::XTrans6x6;
    if (input.filters == 0) {
        if (input.colors == 1) return ingress::MeasurementTopology::Monochrome;
        if (input.colors >= 3) return ingress::MeasurementTopology::LinearRgb;
        return ingress::MeasurementTopology::Unknown;
    }
    // LibRaw reserves filters < 1000 for special topologies (e.g. Leaf=1,
    // X-Trans=9). Do not coerce them to Bayer.
    if (input.filters >= 1000 && input.colors == 3) {
        return ingress::MeasurementTopology::Bayer2x2;
    }
    return ingress::MeasurementTopology::Unknown;
}

ingress::ContainerFamily classify_container(const TopologyInput& input) noexcept {
    // LibRaw exposes DNG version explicitly. For other vendor containers this
    // probe intentionally stays Unknown until a signature/container router is
    // independently certified; filename extension is never evidence.
    if (input.dngVersion != 0) return ingress::ContainerFamily::Dng;
    return ingress::ContainerFamily::Unknown;
}

ProbeStatus probe_file(const char* path, ProbeResult& out) noexcept {
    out = {};
    if (path == nullptr || *path == '\0') {
        return {ProbeStatusCode::InvalidArgument, 0, "path is empty"};
    }

    LibRaw raw;
    const int rc = raw.open_file(path);
    if (rc != LIBRAW_SUCCESS) {
        return {ProbeStatusCode::OpenFailed, rc, safe_string(libraw_strerror(rc))};
    }

    const auto& idata = raw.imgdata.idata;
    const auto& sizes = raw.imgdata.sizes;
    if (idata.raw_count == 0) {
        raw.recycle();
        return {ProbeStatusCode::Unrecognized, 0, "LibRaw reported raw_count=0"};
    }

    TopologyInput topologyInput{};
    topologyInput.rawCount = idata.raw_count;
    topologyInput.isFoveon = idata.is_foveon;
    topologyInput.dngVersion = idata.dng_version;
    topologyInput.colors = idata.colors;
    topologyInput.filters = idata.filters;

    out.librawVersion = safe_string(LibRaw::version());
    out.cameraMake = safe_string(idata.make);
    out.cameraModel = safe_string(idata.model);
    out.normalizedMake = safe_string(idata.normalized_make);
    out.normalizedModel = safe_string(idata.normalized_model);
    out.software = safe_string(idata.software);
    out.rawCount = idata.raw_count;
    out.dngVersion = idata.dng_version;
    out.filters = idata.filters;
    out.colors = idata.colors;
    out.rawWidth = sizes.raw_width;
    out.rawHeight = sizes.raw_height;
    out.visibleWidth = sizes.width;
    out.visibleHeight = sizes.height;
    out.topology = classify_topology(topologyInput);
    out.container = classify_container(topologyInput);
    out.requiresFrameSelection = idata.raw_count > 1;
    out.pixelsUnpacked = false;

    libraw_decoder_info_t decoder{};
    if (raw.get_decoder_info(&decoder) == LIBRAW_SUCCESS && decoder.decoder_name) {
        out.decoderName = decoder.decoder_name;
    }

    raw.recycle();
    return {};
}

std::uint64_t raw_sample_floor_bytes(const ProbeResult& result) noexcept {
    const std::uint64_t pixels =
        static_cast<std::uint64_t>(result.rawWidth) *
        static_cast<std::uint64_t>(result.rawHeight);
    std::uint64_t components = 1;
    if (result.topology == ingress::MeasurementTopology::LayeredFoveon) {
        components = 3;
    } else if (result.topology == ingress::MeasurementTopology::LinearRgb) {
        components = result.colors > 0 ? static_cast<std::uint64_t>(result.colors) : 3;
    }
    if (pixels > std::numeric_limits<std::uint64_t>::max() / components / 2u) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    // Informational lower-bound for a uint16-per-sample materialization only.
    // It is not a decoder resident-memory upper bound.
    return pixels * components * 2u;
}

} // namespace truthraw::libraw_compat_probe::v0_1
