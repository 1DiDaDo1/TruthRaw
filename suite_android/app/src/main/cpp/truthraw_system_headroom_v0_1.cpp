#include "truthraw_system_headroom_v0_1.h"

#include <dlfcn.h>
#include <jni.h>

#include <cerrno>
#include <cmath>
#include <cstdint>
#include <sstream>

namespace truthraw::system_headroom::v0_1 {
namespace {

using GetHeadroomFn = int (*)(const void*, float*);
using GetMinIntervalFn = int (*)(std::int64_t*);

bool valid_headroom(float value) noexcept {
    return std::isfinite(value) && value >= 0.0f && value <= 100.0f;
}

} // namespace

Probe probe() noexcept {
    Probe out{};
    try {
        void* libandroid = ::dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
        if (libandroid == nullptr) return out;

        const auto getCpu = reinterpret_cast<GetHeadroomFn>(
            ::dlsym(libandroid, "ASystemHealth_getCpuHeadroom"));
        const auto getGpu = reinterpret_cast<GetHeadroomFn>(
            ::dlsym(libandroid, "ASystemHealth_getGpuHeadroom"));
        const auto getCpuInterval = reinterpret_cast<GetMinIntervalFn>(
            ::dlsym(libandroid, "ASystemHealth_getCpuHeadroomMinIntervalMillis"));
        const auto getGpuInterval = reinterpret_cast<GetMinIntervalFn>(
            ::dlsym(libandroid, "ASystemHealth_getGpuHeadroomMinIntervalMillis"));

        out.apiSymbolsAvailable = getCpu != nullptr || getGpu != nullptr;

        if (getCpu != nullptr) {
            float value = -1.0f;
            const int status = getCpu(nullptr, &value);
            if (status == 0 && valid_headroom(value)) {
                out.cpuSupported = true;
                out.cpuHeadroom = value;
            } else if (status == 0 && std::isnan(value)) {
                out.cpuSupported = true;
                out.cpuTemporarilyUnavailable = true;
            } else if (status == EPIPE) {
                out.cpuSupported = true;
                out.cpuTemporarilyUnavailable = true;
            }
        }

        if (getGpu != nullptr) {
            float value = -1.0f;
            const int status = getGpu(nullptr, &value);
            if (status == 0 && valid_headroom(value)) {
                out.gpuSupported = true;
                out.gpuHeadroom = value;
            } else if (status == 0 && std::isnan(value)) {
                out.gpuSupported = true;
                out.gpuTemporarilyUnavailable = true;
            } else if (status == EPIPE) {
                out.gpuSupported = true;
                out.gpuTemporarilyUnavailable = true;
            }
        }

        if (getCpuInterval != nullptr) {
            std::int64_t interval = -1;
            if (getCpuInterval(&interval) == 0 && interval >= 0) {
                out.cpuMinPollIntervalMs = interval;
            }
        }
        if (getGpuInterval != nullptr) {
            std::int64_t interval = -1;
            if (getGpuInterval(&interval) == 0 && interval >= 0) {
                out.gpuMinPollIntervalMs = interval;
            }
        }

        ::dlclose(libandroid);
    } catch (...) {
        // Resource-headroom telemetry must never make TruthRaw processing fail.
    }
    return out;
}

std::string to_json(const Probe& p) {
    std::ostringstream s;
    s << "{";
    s << "\"schema\":\"TruthRawSystemHeadroom/0.1\",";
    s << "\"api_symbols_available\":" << (p.apiSymbolsAvailable ? "true" : "false") << ",";
    s << "\"cpu_supported\":" << (p.cpuSupported ? "true" : "false") << ",";
    s << "\"gpu_supported\":" << (p.gpuSupported ? "true" : "false") << ",";
    s << "\"cpu_temporarily_unavailable\":" <<
        (p.cpuTemporarilyUnavailable ? "true" : "false") << ",";
    s << "\"gpu_temporarily_unavailable\":" <<
        (p.gpuTemporarilyUnavailable ? "true" : "false") << ",";
    s << "\"cpu_headroom\":" << p.cpuHeadroom << ",";
    s << "\"gpu_headroom\":" << p.gpuHeadroom << ",";
    s << "\"cpu_min_poll_interval_ms\":" << p.cpuMinPollIntervalMs << ",";
    s << "\"gpu_min_poll_interval_ms\":" << p.gpuMinPollIntervalMs << ",";
    s << "\"changes_scientific_authority\":false";
    s << "}";
    return s.str();
}

} // namespace truthraw::system_headroom::v0_1

extern "C" JNIEXPORT jstring JNICALL
Java_com_truthraw_adaptiveui_TruthRawSystemHeadroomNativeBridge_probeJson(
    JNIEnv* env,
    jobject) {
    if (env == nullptr) return nullptr;
    const auto result = truthraw::system_headroom::v0_1::probe();
    const auto json = truthraw::system_headroom::v0_1::to_json(result);
    return env->NewStringUTF(json.c_str());
}
