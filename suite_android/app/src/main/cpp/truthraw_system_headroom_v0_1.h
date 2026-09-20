#pragma once

#include <string>

namespace truthraw::system_headroom::v0_1 {

struct Probe final {
    bool apiSymbolsAvailable = false;
    bool cpuSupported = false;
    bool gpuSupported = false;
    bool cpuTemporarilyUnavailable = false;
    bool gpuTemporarilyUnavailable = false;
    float cpuHeadroom = -1.0f;
    float gpuHeadroom = -1.0f;
    long long cpuMinPollIntervalMs = -1;
    long long gpuMinPollIntervalMs = -1;

    // Performance telemetry only. Never changes image authority.
    bool changesScientificAuthority = false;
};

Probe probe() noexcept;
std::string to_json(const Probe& probe);

} // namespace truthraw::system_headroom::v0_1
