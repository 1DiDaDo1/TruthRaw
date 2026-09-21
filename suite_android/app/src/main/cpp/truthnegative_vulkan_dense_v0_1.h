#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "truthnegative_dense_projection_v0_3.h"

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

namespace dense = truthraw::truthnegative_dense_projection::v0_3;
using PatchRequest = dense::AcceleratorPatchRequest;

class Backend final : public dense::IExactDenseAccelerator {
public:
    Backend() noexcept;
    ~Backend();

    Backend(const Backend&) = delete;
    Backend& operator=(const Backend&) = delete;

    const Probe& probe() const noexcept;
    bool available() const noexcept;
    bool exactScientificEligible() const noexcept override;
    const char* backendName() const noexcept override;

    // Exact-scientific dispatch. This method returns false unless the runtime
    // self-test proved bit identity against the canonical CPU Float32 formula.
    bool projectPatch(
        const PatchRequest& request,
        const float* sourceRgb,
        std::size_t sourceFloatCount,
        float* targetRgb,
        std::size_t targetFloatCount,
        std::string& error) noexcept override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace truthraw::truthnegative_vulkan_dense::v0_1
