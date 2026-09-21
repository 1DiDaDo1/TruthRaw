#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace truthraw::truthnegative_vulkan_dense::v0_1 {

struct Probe final {
    bool loaderAvailable = false;
    bool hardwareDeviceAvailable = false;
    bool computeQueueAvailable = false;
    bool pipelineCreated = false;
    bool selfTestPassed = false;
    bool exactScientificEligible = false;
    std::uint32_t vendorId = 0u;
    std::uint32_t deviceId = 0u;
    std::uint32_t driverVersion = 0u;
    std::string deviceName;
    std::string reason;
};

struct PatchRequest final {
    std::uint32_t sourceFullWidth = 0u;
    std::uint32_t sourceFullHeight = 0u;
    std::uint32_t patchOriginX = 0u;
    std::uint32_t patchOriginY = 0u;
    std::uint32_t patchWidth = 0u;
    std::uint32_t patchHeight = 0u;
    std::uint32_t targetOriginX = 0u;
    std::uint32_t targetOriginY = 0u;
    std::uint32_t targetWidth = 0u;
    std::uint32_t targetHeight = 0u;
};

class Backend final {
public:
    Backend() noexcept;
    ~Backend();

    Backend(const Backend&) = delete;
    Backend& operator=(const Backend&) = delete;

    const Probe& probe() const noexcept;
    bool available() const noexcept;

    // Exact-scientific dispatch. This method returns false unless the runtime
    // self-test proved bit identity against the canonical CPU Float32 formula.
    bool projectPatch(
        const PatchRequest& request,
        const float* sourceRgb,
        std::size_t sourceFloatCount,
        float* targetRgb,
        std::size_t targetFloatCount,
        std::string* error = nullptr) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace truthraw::truthnegative_vulkan_dense::v0_1
