#pragma once

#include <cstdint>
#include <string>

namespace truthraw::compute_capabilities::v0_1 {

struct Probe final {
    int onlineCpuCount = 1;
    bool arm64 = false;
    bool neon = false;
    bool armSha2 = false;

    bool vulkanLoaderAvailable = false;
    bool vulkanHardwareDeviceAvailable = false;
    bool vulkanComputeQueueAvailable = false;
    std::uint32_t vulkanInstanceVersion = 0u;
    std::uint32_t vulkanDeviceApiVersion = 0u;
    std::uint32_t vulkanVendorId = 0u;
    std::uint32_t vulkanDeviceId = 0u;
    std::uint32_t vulkanDriverVersion = 0u;
    std::uint32_t vulkanComputeQueueFamilyCount = 0u;
    bool vulkanAhbExternalMemoryExtension = false;
    bool vulkanFloat16Int8Extension = false;
    bool vulkanShaderFloat64 = false;
    std::string vulkanDeviceName;

    // Probe only. No accelerator is authorized for pixels by this record.
    bool changesScientificAuthority = false;
    bool acceleratorSelected = false;
};

Probe probe() noexcept;
std::string to_json(const Probe& probe);

} // namespace truthraw::compute_capabilities::v0_1
