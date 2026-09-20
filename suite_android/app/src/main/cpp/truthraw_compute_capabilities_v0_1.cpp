#include "truthraw_compute_capabilities_v0_1.h"

#include <dlfcn.h>
#include <jni.h>
#include <sys/auxv.h>
#include <unistd.h>

#ifndef VK_USE_PLATFORM_ANDROID_KHR
#define VK_USE_PLATFORM_ANDROID_KHR 1
#endif
#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace truthraw::compute_capabilities::v0_1 {
namespace {

constexpr unsigned long kArm64HwcapAsimd = 1ul << 1u;
constexpr unsigned long kArm64HwcapSha2 = 1ul << 6u;

std::string json_escape(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 8u);
    static constexpr char hex[] = "0123456789abcdef";
    for (const unsigned char c : input) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20u) {
                    out += "\\u00";
                    out.push_back(hex[(c >> 4u) & 0x0fu]);
                    out.push_back(hex[c & 0x0fu]);
                } else {
                    out.push_back(static_cast<char>(c));
                }
        }
    }
    return out;
}

bool has_extension(
    PFN_vkEnumerateDeviceExtensionProperties enumerate,
    VkPhysicalDevice device,
    const char* wanted) noexcept {
    if (enumerate == nullptr || wanted == nullptr) return false;
    std::uint32_t count = 0u;
    if (enumerate(device, nullptr, &count, nullptr) != VK_SUCCESS || count == 0u) {
        return false;
    }
    std::vector<VkExtensionProperties> extensions(count);
    if (enumerate(device, nullptr, &count, extensions.data()) != VK_SUCCESS) {
        return false;
    }
    return std::any_of(
        extensions.begin(),
        extensions.begin() + static_cast<std::ptrdiff_t>(count),
        [wanted](const VkExtensionProperties& e) {
            return std::string(e.extensionName) == wanted;
        });
}

int device_score(VkPhysicalDeviceType type) noexcept {
    switch (type) {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return 4;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return 3;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return 2;
        case VK_PHYSICAL_DEVICE_TYPE_OTHER: return 1;
        case VK_PHYSICAL_DEVICE_TYPE_CPU: return -100;
        default: return 0;
    }
}

} // namespace

Probe probe() noexcept {
    Probe out{};
    try {
        const long cores = ::sysconf(_SC_NPROCESSORS_ONLN);
        out.onlineCpuCount = static_cast<int>(std::max(1L, cores));

#if defined(__aarch64__)
        out.arm64 = true;
        const unsigned long hwcap = ::getauxval(AT_HWCAP);
        out.neon = (hwcap & kArm64HwcapAsimd) != 0u;
        out.armSha2 = (hwcap & kArm64HwcapSha2) != 0u;
#endif

        void* loader = ::dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
        if (loader == nullptr) return out;
        out.vulkanLoaderAvailable = true;

        const auto getInstanceProcAddr =
            reinterpret_cast<PFN_vkGetInstanceProcAddr>(
                ::dlsym(loader, "vkGetInstanceProcAddr"));
        if (getInstanceProcAddr == nullptr) {
            ::dlclose(loader);
            return out;
        }

        std::uint32_t instanceVersion = VK_API_VERSION_1_0;
        const auto enumerateInstanceVersion =
            reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
                getInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
        if (enumerateInstanceVersion != nullptr) {
            std::uint32_t queried = VK_API_VERSION_1_0;
            if (enumerateInstanceVersion(&queried) == VK_SUCCESS) {
                instanceVersion = queried;
            }
        }
        out.vulkanInstanceVersion = instanceVersion;

        const auto createInstance =
            reinterpret_cast<PFN_vkCreateInstance>(
                getInstanceProcAddr(nullptr, "vkCreateInstance"));
        if (createInstance == nullptr) {
            ::dlclose(loader);
            return out;
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "TruthRawComputeProbe";
        appInfo.applicationVersion = 1u;
        appInfo.pEngineName = "TruthRaw";
        appInfo.engineVersion = 1u;
        appInfo.apiVersion =
            instanceVersion >= VK_API_VERSION_1_1
                ? VK_API_VERSION_1_1
                : VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        VkInstance instance = VK_NULL_HANDLE;
        if (createInstance(&createInfo, nullptr, &instance) != VK_SUCCESS ||
            instance == VK_NULL_HANDLE) {
            ::dlclose(loader);
            return out;
        }

        const auto destroyInstance =
            reinterpret_cast<PFN_vkDestroyInstance>(
                getInstanceProcAddr(instance, "vkDestroyInstance"));
        const auto enumeratePhysicalDevices =
            reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
                getInstanceProcAddr(instance, "vkEnumeratePhysicalDevices"));
        const auto getProperties =
            reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
                getInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties"));
        const auto getFeatures =
            reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures>(
                getInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures"));
        const auto getQueueFamilies =
            reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
                getInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
        const auto enumerateExtensions =
            reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
                getInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties"));

        if (enumeratePhysicalDevices != nullptr &&
            getProperties != nullptr &&
            getFeatures != nullptr &&
            getQueueFamilies != nullptr) {
            std::uint32_t deviceCount = 0u;
            if (enumeratePhysicalDevices(instance, &deviceCount, nullptr) == VK_SUCCESS &&
                deviceCount > 0u) {
                std::vector<VkPhysicalDevice> devices(deviceCount);
                if (enumeratePhysicalDevices(
                        instance, &deviceCount, devices.data()) == VK_SUCCESS) {
                    int bestScore = -1000;
                    VkPhysicalDevice best = VK_NULL_HANDLE;
                    VkPhysicalDeviceProperties bestProperties{};
                    std::uint32_t bestComputeFamilies = 0u;

                    for (std::uint32_t i = 0u; i < deviceCount; ++i) {
                        VkPhysicalDeviceProperties properties{};
                        getProperties(devices[i], &properties);

                        std::uint32_t queueCount = 0u;
                        getQueueFamilies(devices[i], &queueCount, nullptr);
                        std::vector<VkQueueFamilyProperties> queues(queueCount);
                        if (queueCount > 0u) {
                            getQueueFamilies(devices[i], &queueCount, queues.data());
                        }
                        std::uint32_t computeFamilies = 0u;
                        for (std::uint32_t q = 0u; q < queueCount; ++q) {
                            if ((queues[q].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0u &&
                                queues[q].queueCount > 0u) {
                                ++computeFamilies;
                            }
                        }
                        if (computeFamilies == 0u) continue;

                        const int score = device_score(properties.deviceType);
                        if (score > bestScore) {
                            bestScore = score;
                            best = devices[i];
                            bestProperties = properties;
                            bestComputeFamilies = computeFamilies;
                        }
                    }

                    if (best != VK_NULL_HANDLE &&
                        bestProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_CPU) {
                        out.vulkanHardwareDeviceAvailable = true;
                        out.vulkanComputeQueueAvailable = bestComputeFamilies > 0u;
                        out.vulkanComputeQueueFamilyCount = bestComputeFamilies;
                        out.vulkanDeviceApiVersion = bestProperties.apiVersion;
                        out.vulkanVendorId = bestProperties.vendorID;
                        out.vulkanDeviceId = bestProperties.deviceID;
                        out.vulkanDriverVersion = bestProperties.driverVersion;
                        out.vulkanDeviceName = bestProperties.deviceName;

                        VkPhysicalDeviceFeatures features{};
                        getFeatures(best, &features);
                        out.vulkanShaderFloat64 = features.shaderFloat64 == VK_TRUE;

                        out.vulkanAhbExternalMemoryExtension = has_extension(
                            enumerateExtensions,
                            best,
                            VK_ANDROID_EXTERNAL_MEMORY_ANDROID_HARDWARE_BUFFER_EXTENSION_NAME);
                        out.vulkanFloat16Int8Extension = has_extension(
                            enumerateExtensions,
                            best,
                            VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME);
                    }
                }
            }
        }

        if (destroyInstance != nullptr) destroyInstance(instance, nullptr);
        ::dlclose(loader);
    } catch (...) {
        // Capability discovery must never make processing fail.
    }
    return out;
}

std::string to_json(const Probe& p) {
    std::ostringstream s;
    s << "{";
    s << "\"schema\":\"TruthRawComputeCapabilities/0.1\",";
    s << "\"online_cpu_count\":" << p.onlineCpuCount << ",";
    s << "\"arm64\":" << (p.arm64 ? "true" : "false") << ",";
    s << "\"neon\":" << (p.neon ? "true" : "false") << ",";
    s << "\"arm_sha2\":" << (p.armSha2 ? "true" : "false") << ",";
    s << "\"vulkan_loader_available\":" << (p.vulkanLoaderAvailable ? "true" : "false") << ",";
    s << "\"vulkan_hardware_device_available\":" << (p.vulkanHardwareDeviceAvailable ? "true" : "false") << ",";
    s << "\"vulkan_compute_queue_available\":" << (p.vulkanComputeQueueAvailable ? "true" : "false") << ",";
    s << "\"vulkan_instance_version\":" << p.vulkanInstanceVersion << ",";
    s << "\"vulkan_device_api_version\":" << p.vulkanDeviceApiVersion << ",";
    s << "\"vulkan_vendor_id\":" << p.vulkanVendorId << ",";
    s << "\"vulkan_device_id\":" << p.vulkanDeviceId << ",";
    s << "\"vulkan_driver_version\":" << p.vulkanDriverVersion << ",";
    s << "\"vulkan_compute_queue_family_count\":" << p.vulkanComputeQueueFamilyCount << ",";
    s << "\"vulkan_ahb_external_memory_extension\":" <<
        (p.vulkanAhbExternalMemoryExtension ? "true" : "false") << ",";
    s << "\"vulkan_float16_int8_extension\":" <<
        (p.vulkanFloat16Int8Extension ? "true" : "false") << ",";
    s << "\"vulkan_shader_float64\":" << (p.vulkanShaderFloat64 ? "true" : "false") << ",";
    s << "\"vulkan_device_name\":\"" << json_escape(p.vulkanDeviceName) << "\",";
    s << "\"changes_scientific_authority\":false,";
    s << "\"accelerator_selected\":false";
    s << "}";
    return s.str();
}

} // namespace truthraw::compute_capabilities::v0_1

extern "C" JNIEXPORT jstring JNICALL
Java_com_truthraw_adaptiveui_TruthRawComputeProbeNativeBridge_probeJson(
    JNIEnv* env,
    jobject) {
    if (env == nullptr) return nullptr;
    const auto result = truthraw::compute_capabilities::v0_1::probe();
    const auto json = truthraw::compute_capabilities::v0_1::to_json(result);
    return env->NewStringUTF(json.c_str());
}
