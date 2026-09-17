package com.truthraw.adaptiveui

import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.params.SessionConfiguration
import org.json.JSONObject

/**
 * v0.28 single-variable session intervention for
 * `org.codeaurora.qcamera3.sessionParameters.EnableXCFAOptimization`.
 *
 * v0.27 resolved this device's native camera_metadata element type as BYTE:
 * - vendor tag 0x801F0036
 * - exactly one accepted native type
 * - BYTE set/get accepted with count=1
 * - INT32/FLOAT/INT64/DOUBLE/RATIONAL rejected
 * - no session/capture/HAL submission occurred in the oracle.
 *
 * Numeric value 1 is used only as a controlled A/B intervention value. No vendor semantic
 * meaning is assumed from the key name or the numeric value.
 */
object Camera2XcfaByteSessionProbe {
    const val KEY_NAME = "org.codeaurora.qcamera3.sessionParameters.EnableXCFAOptimization"
    const val NATIVE_TAG_HEX = "0x801F0036"
    const val RESOLVED_NATIVE_TYPE = "BYTE"

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

        val out = JSONObject()
            .put("schema", "truthraw.camera2-xcfa-byte-session-intervention.v0.1")
            .put("experiment", "ENABLE_XCFA_OPTIMIZATION_SESSION_PARAMETER_BYTE_ONE")
            .put("keyName", KEY_NAME)
            .put("nativeTagHexFromV027", NATIVE_TAG_HEX)
            .put("resolvedNativeTypeFromV027", RESOLVED_NATIVE_TYPE)
            .put("nativeTypeEvidenceSource", "TRUTHRAW_CAM5_XCFA_NATIVE_TYPE_ORACLE_v027.json")
            .put("singleUnknownVendorVariable", true)
            .put("logicalSessionAdvertised", logicalSessionAdvertised)
            .put("physicalSessionAdvertised", physicalSessionAdvertised)
            .put("logicalRequestAdvertised", logicalRequestAdvertised)
            .put("physicalRequestAdvertised", physicalRequestAdvertised)
            .put("writeRepresentation", "CaptureRequest.Key<Byte> + Byte(1)")
            .put("requestedNumericValue", 1)
            .put("requestedValueSemanticsAssumed", false)
            .put("pixelAccess", false)
            .put("sourceMutation", false)
            .put("semanticMeaningAssumed", false)
            .put("semanticPromotionAllowed", false)
            .put("controlReference", "TruthRaw v0.20 unchanged")
            .put("applied", false)

        if (!logicalSessionAdvertised) {
            out.put("classification", "BLOCKED_XCFA_NOT_ADVERTISED_AS_LOGICAL_SESSION_KEY")
            return ApplyResult(false, out)
        }

        val key = CaptureRequest.Key(KEY_NAME, Byte::class.javaObjectType)
        val builder = runCatching { device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE) }
            .getOrElse { e ->
                out.put("classification", "BLOCKED_XCFA_BYTE_BUILDER_CREATE_FAILED")
                    .put("error", "${e.javaClass.simpleName}: ${e.message}")
                return ApplyResult(false, out)
            }

        val before = runCatching { builder.get(key) }.getOrNull()
        out.put("builderReadbackBeforeSet", before?.toInt() ?: JSONObject.NULL)

        val setError = runCatching { builder.set(key, 1.toByte()) }.exceptionOrNull()
        if (setError != null) {
            out.put("classification", "BLOCKED_XCFA_BYTE_SET_FAILED")
                .put("error", "${setError.javaClass.simpleName}: ${setError.message}")
            return ApplyResult(false, out)
        }
        out.put("builderSetPass", true)

        val builderReadback = runCatching { builder.get(key) }.getOrNull()
        val builderReadbackPass = builderReadback?.toInt() == 1
        out.put("builderReadback", builderReadback?.toInt() ?: JSONObject.NULL)
            .put("builderReadbackPass", builderReadbackPass)
        if (!builderReadbackPass) {
            out.put("classification", "BLOCKED_XCFA_BYTE_BUILDER_READBACK_MISMATCH")
            return ApplyResult(false, out)
        }

        val request = runCatching { builder.build() }.getOrElse { e ->
            out.put("classification", "BLOCKED_XCFA_BYTE_REQUEST_BUILD_FAILED")
                .put("error", "${e.javaClass.simpleName}: ${e.message}")
            return ApplyResult(false, out)
        }
        val requestReadback = runCatching { request.get(key) }.getOrNull()
        val requestReadbackPass = requestReadback?.toInt() == 1
        out.put("requestReadback", requestReadback?.toInt() ?: JSONObject.NULL)
            .put("requestReadbackPass", requestReadbackPass)
        if (!requestReadbackPass) {
            out.put("classification", "BLOCKED_XCFA_BYTE_REQUEST_READBACK_MISMATCH")
            return ApplyResult(false, out)
        }

        val attachError = runCatching { config.setSessionParameters(request) }.exceptionOrNull()
        if (attachError != null) {
            out.put("classification", "BLOCKED_XCFA_BYTE_SET_SESSION_PARAMETERS_FAILED")
                .put("error", "${attachError.javaClass.simpleName}: ${attachError.message}")
            return ApplyResult(false, out)
        }

        out.put("sessionParametersAttached", true)
            .put("applied", true)
            .put("classification", "XCFA_BYTE_ONE_ATTACHED_AS_SINGLE_VENDOR_SESSION_VARIABLE__SEMANTICS_UNPROVEN")
            .put("authority", "CONTROL_INTERVENTION_ONLY_NOT_SENSOR_OR_RAW_SEMANTICS_PROOF")
        return ApplyResult(true, out)
    }
}
