package com.truthraw.adaptiveui

import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CaptureRequest
import org.json.JSONArray
import org.json.JSONObject

/**
 * Request-side observation gate placed before Camera2 session/capture submission.
 *
 * Important boundary: this is NOT a pre-HAL pixel interceptor. An ordinary Camera2 app cannot
 * receive sensor bytes before the vendor HAL. This gate records the exact public/vendor control
 * surface visible to the app BEFORE HONOR/QTI chooses/executes the internal pipeline, so it can be
 * compared with the post-HAL HardwareBuffer envelope and physical CaptureResult.
 *
 * This object never writes vendor keys and never upgrades vendor-key names/values to semantics.
 */
object Camera2PreHalGate {
    private val routeCandidateNames = setOf(
        "org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW",
        "org.codeaurora.qcamera3.sessionParameters.RawCbSourceType",
        "org.codeaurora.qcamera3.sessionParameters.EnableXCFAOptimization",
        "org.codeaurora.qcamera3.sessionParameters.EnableInsensorZoom",
        "org.codeaurora.qcamera3.sessionParameters.EnableSnapshotOnlyInsensorZoom",
        "org.codeaurora.qcamera3.sessionParameters.inSensorZoomEnable",
        "org.codeaurora.qcamera3.sessionParameters.HALOutputBufferCombined",
        "org.codeaurora.qcamera3.sessionParameters.EnableMCXMasterCb",
        "org.codeaurora.qcamera3.sessionParameters.EnableOfflineHALZSL",
        "org.codeaurora.qcamera3.sessionParameters.EnableHDRDCGMode",
        "com.hihonor.capture.metadata.hwCamera2Flag",
        "com.hihonor.capture.metadata.thirdPartyCamera",
        "com.hihonor.capture.metadata.teleconverterEnable",
        "com.hihonor.capture.metadata.cameraExtension",
        "com.hihonor.capture.metadata.extStreamSize",
    )

    fun observeSession(
        logical: CameraCharacteristics,
        physical: CameraCharacteristics,
        physicalId: String,
        width: Int,
        height: Int,
        format: Int,
        outputPhysicalBinding: Boolean,
        outputMaximumResolutionModeDeclared: Boolean,
    ): JSONObject {
        val logicalRequest = logical.availableCaptureRequestKeys.orEmpty()
        val physicalRequest = physical.availableCaptureRequestKeys.orEmpty()
        val logicalSession = logical.availableSessionKeys.orEmpty()
        val physicalSession = physical.availableSessionKeys.orEmpty()
        val logicalPhysicalOverride = logical.availablePhysicalCameraRequestKeys.orEmpty()

        return JSONObject()
            .put("schema", "truthraw.camera2-pre-hal-session-gate.v0.1")
            .put("observationOnly", true)
            .put("pixelInterceptor", false)
            .put("vendorKeysWritten", false)
            .put("semanticPromotionAllowed", false)
            .put("boundary", "APP_REQUEST_SIDE_BEFORE_VENDOR_SESSION_EXECUTION_NOT_PRE_HAL_PIXEL_ACCESS")
            .put("physicalCameraId", physicalId)
            .put("target", JSONObject()
                .put("width", width)
                .put("height", height)
                .put("format", format)
                .put("outputPhysicalBinding", outputPhysicalBinding)
                .put("outputMaximumResolutionModeDeclared", outputMaximumResolutionModeDeclared))
            .put("logicalAvailableSessionKeyNames", JSONArray(logicalSession.map { it.name }.sorted()))
            .put("physicalAvailableSessionKeyNames", JSONArray(physicalSession.map { it.name }.sorted()))
            .put("logicalAvailablePhysicalOverrideKeyNames", JSONArray(logicalPhysicalOverride.map { it.name }.sorted()))
            .put("routeCandidates", candidateInventory(logicalRequest, physicalRequest, logicalSession, physicalSession, logicalPhysicalOverride))
    }

    fun observeRequest(
        builder: CaptureRequest.Builder,
        logical: CameraCharacteristics,
        physical: CameraCharacteristics,
        physicalId: String,
        scopedPhysicalRequestUsed: Boolean,
        globalPixelModeWritten: Boolean,
        physicalPixelModeWritten: Boolean,
    ): JSONObject {
        val logicalRequest = logical.availableCaptureRequestKeys.orEmpty()
        val physicalRequest = physical.availableCaptureRequestKeys.orEmpty()
        val logicalSession = logical.availableSessionKeys.orEmpty()
        val physicalSession = physical.availableSessionKeys.orEmpty()
        val logicalPhysicalOverride = logical.availablePhysicalCameraRequestKeys.orEmpty()

        val candidateValues = JSONArray()
        val requestKeys = (logicalRequest + physicalRequest)
            .associateBy { it.name }
            .toSortedMap()
        routeCandidateNames.sorted().forEach { name ->
            val key = requestKeys[name]
            val item = JSONObject()
                .put("name", name)
                .put("availableOnLogicalRequest", logicalRequest.any { it.name == name })
                .put("availableOnPhysicalRequest", physicalRequest.any { it.name == name })
                .put("availableAsLogicalSessionKey", logicalSession.any { it.name == name })
                .put("availableAsPhysicalSessionKey", physicalSession.any { it.name == name })
                .put("availableAsPhysicalOverride", logicalPhysicalOverride.any { it.name == name })
            if (key != null) {
                item.put("builderDefaultOrCurrentValue", runCatching { requestValue(builder, key) }.fold(
                    onSuccess = { encodeValue(it) },
                    onFailure = { "ERROR:${it.javaClass.simpleName}:${it.message}" },
                ))
            } else {
                item.put("builderDefaultOrCurrentValue", JSONObject.NULL)
            }
            candidateValues.put(item)
        }

        return JSONObject()
            .put("schema", "truthraw.camera2-pre-hal-request-gate.v0.1")
            .put("observationOnly", true)
            .put("pixelInterceptor", false)
            .put("vendorKeysWritten", false)
            .put("semanticPromotionAllowed", false)
            .put("capturedBeforeRequestBuild", true)
            .put("capturedBeforeCaptureSubmit", true)
            .put("physicalCameraId", physicalId)
            .put("scopedPhysicalRequestUsed", scopedPhysicalRequestUsed)
            .put("globalSensorPixelModeWritten", globalPixelModeWritten)
            .put("physicalSensorPixelModeWritten", physicalPixelModeWritten)
            .put("routeCandidateRequestState", candidateValues)
    }

    private fun candidateInventory(
        logicalRequest: List<CaptureRequest.Key<*>>,
        physicalRequest: List<CaptureRequest.Key<*>>,
        logicalSession: List<CaptureRequest.Key<*>>,
        physicalSession: List<CaptureRequest.Key<*>>,
        logicalPhysicalOverride: List<CaptureRequest.Key<*>>,
    ): JSONArray {
        val out = JSONArray()
        routeCandidateNames.sorted().forEach { name ->
            out.put(JSONObject()
                .put("name", name)
                .put("logicalRequest", logicalRequest.any { it.name == name })
                .put("physicalRequest", physicalRequest.any { it.name == name })
                .put("logicalSession", logicalSession.any { it.name == name })
                .put("physicalSession", physicalSession.any { it.name == name })
                .put("physicalOverride", logicalPhysicalOverride.any { it.name == name })
                .put("semantics", "UNKNOWN_VENDOR_SEMANTICS_DO_NOT_SET_FROM_NAME_ALONE"))
        }
        return out
    }

    @Suppress("UNCHECKED_CAST")
    private fun requestValue(builder: CaptureRequest.Builder, key: CaptureRequest.Key<*>): Any? =
        builder.get(key as CaptureRequest.Key<Any>)

    private fun encodeValue(value: Any?): Any = when (value) {
        null -> JSONObject.NULL
        is Boolean, is Number, is String -> value
        is ByteArray -> encodedArray("byte[]", value.size, value.take(64).map { it.toInt() and 0xff })
        is IntArray -> encodedArray("int[]", value.size, value.take(64))
        is LongArray -> encodedArray("long[]", value.size, value.take(64))
        is FloatArray -> encodedArray("float[]", value.size, value.take(64))
        is DoubleArray -> encodedArray("double[]", value.size, value.take(64))
        is ShortArray -> encodedArray("short[]", value.size, value.take(64))
        is BooleanArray -> encodedArray("boolean[]", value.size, value.take(64))
        is Array<*> -> encodedArray("Object[]", value.size, value.take(64).map { it?.toString() })
        else -> value.toString()
    }

    private fun encodedArray(type: String, length: Int, preview: List<*>): JSONObject =
        JSONObject()
            .put("type", type)
            .put("length", length)
            .put("preview", JSONArray(preview))
            .put("truncated", length > preview.size)
}
