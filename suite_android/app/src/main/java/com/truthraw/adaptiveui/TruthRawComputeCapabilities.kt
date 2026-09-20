package com.truthraw.adaptiveui

import org.json.JSONObject

object TruthRawComputeProbeNativeBridge {
    init { System.loadLibrary("truthraw_ui_preview_bridge") }
    external fun probeJson(): String
}

data class TruthRawComputeCapabilities(
    val onlineCpuCount: Int,
    val arm64: Boolean,
    val neon: Boolean,
    val armSha2: Boolean,
    val vulkanLoaderAvailable: Boolean,
    val vulkanHardwareDeviceAvailable: Boolean,
    val vulkanComputeQueueAvailable: Boolean,
    val vulkanInstanceVersion: Long,
    val vulkanDeviceApiVersion: Long,
    val vulkanVendorId: Long,
    val vulkanDeviceId: Long,
    val vulkanDriverVersion: Long,
    val vulkanComputeQueueFamilyCount: Int,
    val vulkanAhbExternalMemoryExtension: Boolean,
    val vulkanFloat16Int8Extension: Boolean,
    val vulkanShaderFloat64: Boolean,
    val vulkanDeviceName: String,
    val changesScientificAuthority: Boolean,
    val acceleratorSelected: Boolean,
) {
    val genericVulkanCandidate: Boolean
        get() =
            vulkanLoaderAvailable &&
                vulkanHardwareDeviceAvailable &&
                vulkanComputeQueueAvailable

    val exactSha2Candidate: Boolean
        get() = arm64 && armSha2

    val summary: String
        get() = buildString {
            append("CPU ")
            append(onlineCpuCount)
            append(" cores · NEON=")
            append(neon)
            append(" · SHA2=")
            append(armSha2)
            append("\nVulkan hardware=")
            append(genericVulkanCandidate)
            if (vulkanDeviceName.isNotBlank()) {
                append(" · ")
                append(vulkanDeviceName)
            }
            append("\nAHardwareBuffer=")
            append(vulkanAhbExternalMemoryExtension)
            append(" · float16-extension=")
            append(vulkanFloat16Int8Extension)
            append("\naccelerator selected=")
            append(acceleratorSelected)
            append(" · authority changed=")
            append(changesScientificAuthority)
        }
}

object TruthRawComputeCapabilitiesProbe {
    fun probe(): Result<TruthRawComputeCapabilities> = runCatching {
        val json = JSONObject(TruthRawComputeProbeNativeBridge.probeJson())
        require(json.getString("schema") == "TruthRawComputeCapabilities/0.1")
        TruthRawComputeCapabilities(
            onlineCpuCount = json.getInt("online_cpu_count"),
            arm64 = json.getBoolean("arm64"),
            neon = json.getBoolean("neon"),
            armSha2 = json.getBoolean("arm_sha2"),
            vulkanLoaderAvailable = json.getBoolean("vulkan_loader_available"),
            vulkanHardwareDeviceAvailable =
                json.getBoolean("vulkan_hardware_device_available"),
            vulkanComputeQueueAvailable =
                json.getBoolean("vulkan_compute_queue_available"),
            vulkanInstanceVersion = json.getLong("vulkan_instance_version"),
            vulkanDeviceApiVersion = json.getLong("vulkan_device_api_version"),
            vulkanVendorId = json.getLong("vulkan_vendor_id"),
            vulkanDeviceId = json.getLong("vulkan_device_id"),
            vulkanDriverVersion = json.getLong("vulkan_driver_version"),
            vulkanComputeQueueFamilyCount =
                json.getInt("vulkan_compute_queue_family_count"),
            vulkanAhbExternalMemoryExtension =
                json.getBoolean("vulkan_ahb_external_memory_extension"),
            vulkanFloat16Int8Extension =
                json.getBoolean("vulkan_float16_int8_extension"),
            vulkanShaderFloat64 = json.getBoolean("vulkan_shader_float64"),
            vulkanDeviceName = json.getString("vulkan_device_name"),
            changesScientificAuthority =
                json.getBoolean("changes_scientific_authority"),
            acceleratorSelected = json.getBoolean("accelerator_selected"),
        ).also {
            require(!it.changesScientificAuthority)
            require(!it.acceleratorSelected)
        }
    }
}
