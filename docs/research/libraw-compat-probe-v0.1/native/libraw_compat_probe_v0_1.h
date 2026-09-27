#pragma once

#include "professional_raw_ingress_v0_1.h"

#include <cstdint>
#include <string>

namespace truthraw::libraw_compat_probe::v0_1 {

namespace ingress = truthraw::professional_raw_ingress::v0_1;

enum class ProbeStatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    OpenFailed,
    Unrecognized,
};

struct ProbeStatus {
    ProbeStatusCode code = ProbeStatusCode::Ok;
    int librawCode = 0;
    std::string message;
    explicit operator bool() const noexcept { return code == ProbeStatusCode::Ok; }
};

struct TopologyInput {
    unsigned rawCount = 0;
    unsigned isFoveon = 0;
    unsigned dngVersion = 0;
    int colors = 0;
    unsigned filters = 0;
};

struct ProbeResult {
    std::string librawVersion;
    std::string decoderName;
    std::string cameraMake;
    std::string cameraModel;
    std::string normalizedMake;
    std::string normalizedModel;
    std::string software;
    unsigned rawCount = 0;
    unsigned dngVersion = 0;
    unsigned filters = 0;
    int colors = 0;
    unsigned rawWidth = 0;
    unsigned rawHeight = 0;
    unsigned visibleWidth = 0;
    unsigned visibleHeight = 0;
    ingress::MeasurementTopology topology = ingress::MeasurementTopology::Unknown;
    ingress::ContainerFamily container = ingress::ContainerFamily::Unknown;
    bool requiresFrameSelection = false;
    bool pixelsUnpacked = false;
};

[[nodiscard]] ingress::MeasurementTopology classify_topology(
    const TopologyInput& input) noexcept;

[[nodiscard]] ingress::ContainerFamily classify_container(
    const TopologyInput& input) noexcept;

[[nodiscard]] ProbeStatus probe_file(
    const char* path,
    ProbeResult& out) noexcept;

[[nodiscard]] std::uint64_t raw_sample_floor_bytes(
    const ProbeResult& result) noexcept;

} // namespace truthraw::libraw_compat_probe::v0_1
