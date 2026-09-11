#include <jni.h>

#include <chrono>
#include <cstdint>
#include <string>

#include "android_on_device_validation_v0_1.h"

namespace {

using truthraw::android_validation::v0_1::MemorySample;
using truthraw::android_validation::v0_1::Phase;
using truthraw::android_validation::v0_1::Status;
using truthraw::building_runtime::v0_1::ThermalState;

ThermalState thermal_from_int(jint value) noexcept {
    switch (value) {
        case 0: return ThermalState::Nominal;
        case 1: return ThermalState::Moderate;
        case 2: return ThermalState::Severe;
        default: return ThermalState::Critical;
    }
}

const char* thermal_name(ThermalState state) noexcept {
    switch (state) {
        case ThermalState::Nominal: return "NOMINAL";
        case ThermalState::Moderate: return "MODERATE";
        case ThermalState::Severe: return "SEVERE";
        case ThermalState::Critical: return "CRITICAL";
    }
    return "UNKNOWN";
}

} // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_truthraw_validation_MainActivity_nativeProbe(JNIEnv* env,
                                                       jclass,
                                                       jint thermalState,
                                                       jboolean foreground) {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    auto nowNs = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
    if (nowNs == 0U) nowNs = 1U;

    const ThermalState mapped = thermal_from_int(thermalState);
    MemorySample sample{};
    const Status status = truthraw::android_validation::v0_1::capture_proc_memory_sample(
        nowNs,
        mapped,
        true,
        foreground == JNI_TRUE,
        Phase::Baseline,
        sample);

    std::string json = "{";
    json += "\"contract\":\"TruthRawAndroidOnDeviceValidation/0.1\",";
    json += "\"status\":\"";
    json += truthraw::android_validation::v0_1::status_name(status);
    json += "\",";
    json += "\"rssKnown\":" + std::string(sample.rssKnown ? "true" : "false") + ",";
    json += "\"rssBytes\":" + std::to_string(sample.rssBytes) + ",";
    json += "\"highWaterKnown\":" + std::string(sample.highWaterKnown ? "true" : "false") + ",";
    json += "\"highWaterBytes\":" + std::to_string(sample.highWaterBytes) + ",";
    json += "\"nativeHeapKnown\":" + std::string(sample.nativeHeapKnown ? "true" : "false") + ",";
    json += "\"thermalKnown\":true,";
    json += "\"thermalState\":\"" + std::string(thermal_name(mapped)) + "\",";
    json += "\"foreground\":" + std::string(foreground == JNI_TRUE ? "true" : "false") + ",";
    json += "\"sceneIsoAxisPresent\":false,";
    json += "\"scientificStateModified\":false";
    json += "}";

    return env->NewStringUTF(json.c_str());
}
