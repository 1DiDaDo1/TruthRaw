package com.truthraw.adaptiveui

import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.params.SessionConfiguration
import org.json.JSONObject

/**
 * v0.33 single-variable session intervention for
 * `org.codeaurora.qcamera3.sessionParameters.EnableInsensorZoom`.
 *
 * v0.32 resolved this device's native camera_metadata element type as INT32:
 * - vendor tag 0x801F0013
 * - exactly one accepted native type
 * - INT32 set/get accepted with count=1
 * - BYTE/FLOAT/INT64/DOUBLE/RATIONAL rejected
 * - no session/capture/HAL submission occurred in the oracle.
 *
 * Numeric value 1 is used only as a controlled intervention stimulus. No vendor semantic
 * meaning is assumed from the key name or the numeric value.
 */
object Camera2InsensorZoomInt32SessionProbe {
    const val KEY_NAME = "org.codeaurora.qcamera3.sessionParameters.EnableInsensorZoom"
    const val NATIVE_TAG_HEX = "0x801F0013"
    const val RESOLVED_NATIVE_TYPE = "INT32"

    data class ApplyResult(
        val applied: Boolean,
        val evidence: JSONObject,
    )

    fun applyExperiment(
        device: CameraDevice,
        logical: CameraCharacteristics,
        physical: CameraCharacteristics,
        config: SessionConfiguration,
    ): ApplyResult {
        val logicalSessionAdvertised = logical.availableSessionKeys.orEmpty().any { it.name == KEY_NAME }
        val physicalSessionAdvertised = physical.availableSessionKeys.orEmpty().any { it.name == KEY_NAME }
        val logicalRequestAdvertised = logical.availableCaptureRequestKeys.orEmpty().any { it.name == KEY_NAME }
        val physicalRequestAdvertised = physical.availableCaptureRequestKeys.orEmpty().any { it.name == KEY_NAME }
        val physicalOverrideAdvertised = physical.availablePhysicalCameraRequestKeys.orEmpty().any { it.name == KEY_NAME }

        val out = JSONObject()
            .put("schema", "truthraw.camera2-insensorzoom-int32-session-intervention.v0.33")
            .put("experiment", "ENABLE_INSENSOR_ZOOM_SESSION_PARAMETER_INT32_ONE")
            .put("keyName", KEY_NAME)
            .put("nativeTagHexFromV032", NATIVE_TAG_HEX)
            .put("resolvedNativeTypeFromV032", RESOLVED_NATIVE_TYPE)
            .put("nativeTypeEvidenceSource", "TRUTHRAW_CAM5_MULTI_KEY_NATIVE_TYPE_ORACLE_v032.json")
            .put("singleUnknownVendorVariable", true)
            .put("logicalSessionAdvertised", logicalSessionAdvertised)
            .put("physicalSessionAdvertised", physicalSessionAdvertised)
            .put("logicalRequestAdvertised", logicalRequestAdvertised)
            .put("physicalRequestAdvertised", physicalRequestAdvertised)
            .put("physicalOverrideAdvertised", physicalOverrideAdvertised)
            .put("writeRepresentation", "CaptureRequest.Key<Int> + Int(1)")
            .put("requestedNumericValue", 1)
            .put("requestedValueSemanticsAssumed", false)
            .put("otherV032CandidatesWritten", false)
            .put("pixelAccess", false)
            .put("sourceMutation", false)
            .put("semanticMeaningAssumed", false)
            .put("semanticPromotionAllowed", false)
            .put("controlReference", "TruthRaw v0.20 unchanged")
            .put("applied", false)

        if (!logicalSessionAdvertised) {
            out.put("classification", "BLOCKED_INSENSORZOOM_NOT_ADVERTISED_AS_LOGICAL_SESSION_KEY")
            return ApplyResult(false, out)
        }

        val key = CaptureRequest.Key(KEY_NAME, Int::class.javaObjectType)
        val builder = runCatching { device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE) }
            .getOrElse { e ->
                out.put("classification", "BLOCKED_INSENSORZOOM_INT32_BUILDER_CREATE_FAILED")
                    .put("error", "${e.javaClass.simpleName}: ${e.message}")
                return ApplyResult(false, out)
            }

        val before = runCatching { builder.get(key) }.getOrNull()
        out.put("builderReadbackBeforeSet", before ?: JSONObject.NULL)

        val setError = runCatching { builder.set(key, 1) }.exceptionOrNull()
        if (setError != null) {
            out.put("classification", "BLOCKED_INSENSORZOOM_INT32_SET_FAILED")
                .put("error", "${setError.javaClass.simpleName}: ${setError.message}")
            return ApplyResult(false, out)
        }
        out.put("builderSetPass", true)

        val builderReadback = runCatching { builder.get(key) }.getOrNull()
        val builderReadbackPass = builderReadback == 1
        out.put("builderReadback", builderReadback ?: JSONObject.NULL)
            .put("builderReadbackPass", builderReadbackPass)
        if (!builderReadbackPass) {
            out.put("classification", "BLOCKED_INSENSORZOOM_INT32_BUILDER_READBACK_MISMATCH")
            return ApplyResult(false, out)
        }

        val request = runCatching { builder.build() }.getOrElse { e ->
            out.put("classification", "BLOCKED_INSENSORZOOM_INT32_REQUEST_BUILD_FAILED")
                .put("error", "${e.javaClass.simpleName}: ${e.message}")
            return ApplyResult(false, out)
        }
        val requestReadback = runCatching { request.get(key) }.getOrNull()
        val requestReadbackPass = requestReadback == 1
        out.put("requestReadback", requestReadback ?: JSONObject.NULL)
            .put("requestReadbackPass", requestReadbackPass)
        if (!requestReadbackPass) {
            out.put("classification", "BLOCKED_INSENSORZOOM_INT32_REQUEST_READBACK_MISMATCH")
            return ApplyResult(false, out)
        }

        val attachError = runCatching { config.setSessionParameters(request) }.exceptionOrNull()
        if (attachError != null) {
            out.put("classification", "BLOCKED_INSENSORZOOM_INT32_SET_SESSION_PARAMETERS_FAILED")
                .put("error", "${attachError.javaClass.simpleName}: ${attachError.message}")
            return ApplyResult(false, out)
        }

        out.put("sessionParametersAttached", true)
            .put("vendorKeysWritten", 1)
            .put("applied", true)
            .put("classification", "INSENSORZOOM_INT32_ONE_ATTACHED_AS_SINGLE_VENDOR_SESSION_VARIABLE__SEMANTICS_UNPROVEN")
            .put("authority", "CONTROL_INTERVENTION_ONLY_NOT_SENSOR_OR_RAW_SEMANTICS_PROOF")
        return ApplyResult(true, out)
    }
}
