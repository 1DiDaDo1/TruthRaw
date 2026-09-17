package com.truthraw.adaptiveui

import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.params.SessionConfiguration
import org.json.JSONObject

/**
 * v0.26 first real single-variable HAL-route intervention for
 * `org.codeaurora.qcamera3.sessionParameters.RawCbSourceType`.
 *
 * v0.25 resolved the native camera_metadata element type on this device as INT32:
 * - vendor tag 0x801F0009
 * - ACaptureRequest_setEntry_i32 accepted, type=INT32, count=1, value=1
 * - all other native metadata element types tested were rejected
 * - no session/capture/HAL submission was performed by v0.25.
 *
 * v0.26 therefore uses exactly one INT32 value with numeric value 1. No second unknown
 * vendor key is touched. A successful set/attach is an intervention fact only; it does
 * not prove the semantic meaning of RawCbSourceType or of numeric value 1.
 */
object Camera2RawCbSourceTypeInt32SessionProbe {
    const val KEY_NAME = "org.codeaurora.qcamera3.sessionParameters.RawCbSourceType"
    const val NATIVE_TAG_HEX = "0x801F0009"
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

        val out = JSONObject()
            .put("schema", "truthraw.camera2-rawcb-source-type-int32-session-intervention.v0.1")
            .put("experiment", "RAW_CB_SOURCE_TYPE_SESSION_PARAMETER_INT32_ONE")
            .put("keyName", KEY_NAME)
            .put("nativeTagHexFromV025", NATIVE_TAG_HEX)
            .put("resolvedNativeTypeFromV025", RESOLVED_NATIVE_TYPE)
            .put("nativeTypeEvidenceSource", "TRUTHRAW_CAM5_RAWCB_SOURCE_TYPE_NATIVE_TYPE_ORACLE_v025.json")
            .put("singleUnknownVendorVariable", true)
            .put("logicalSessionAdvertised", logicalSessionAdvertised)
            .put("physicalSessionAdvertised", physicalSessionAdvertised)
            .put("logicalRequestAdvertised", logicalRequestAdvertised)
            .put("physicalRequestAdvertised", physicalRequestAdvertised)
            .put("writeRepresentation", "CaptureRequest.Key<Int> + Int(1)")
            .put("requestedNumericValue", 1)
            .put("requestedValueSemanticsAssumed", false)
            .put("pixelAccess", false)
            .put("sourceMutation", false)
            .put("semanticMeaningAssumed", false)
            .put("semanticPromotionAllowed", false)
            .put("controlReference", "TruthRaw v0.20 unchanged")
            .put("applied", false)

        if (!logicalSessionAdvertised) {
            out.put("classification", "BLOCKED_RAWCB_NOT_ADVERTISED_AS_LOGICAL_SESSION_KEY")
            return ApplyResult(false, out)
        }

        val key = CaptureRequest.Key(KEY_NAME, Int::class.javaObjectType)
        val builder = runCatching { device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE) }
            .getOrElse { e ->
                out.put("classification", "BLOCKED_RAWCB_INT32_BUILDER_CREATE_FAILED")
                    .put("error", "${e.javaClass.simpleName}: ${e.message}")
                return ApplyResult(false, out)
            }

        val preSetReadback = runCatching { builder.get(key) }.getOrNull()
        out.put("builderReadbackBeforeSet", preSetReadback ?: JSONObject.NULL)

        val setError = runCatching { builder.set(key, 1) }.exceptionOrNull()
        if (setError != null) {
            out.put("classification", "BLOCKED_RAWCB_INT32_SET_FAILED")
                .put("error", "${setError.javaClass.simpleName}: ${setError.message}")
            return ApplyResult(false, out)
        }
        out.put("builderSetPass", true)

        val builderReadback = runCatching { builder.get(key) }.getOrNull()
        val builderReadbackPass = builderReadback == 1
        out.put("builderReadback", builderReadback ?: JSONObject.NULL)
            .put("builderReadbackPass", builderReadbackPass)
        if (!builderReadbackPass) {
            out.put("classification", "BLOCKED_RAWCB_INT32_BUILDER_READBACK_MISMATCH")
            return ApplyResult(false, out)
        }

        val request = runCatching { builder.build() }.getOrElse { e ->
            out.put("classification", "BLOCKED_RAWCB_INT32_REQUEST_BUILD_FAILED")
                .put("error", "${e.javaClass.simpleName}: ${e.message}")
            return ApplyResult(false, out)
        }
        val requestReadback = runCatching { request.get(key) }.getOrNull()
        val requestReadbackPass = requestReadback == 1
        out.put("requestReadback", requestReadback ?: JSONObject.NULL)
            .put("requestReadbackPass", requestReadbackPass)
        if (!requestReadbackPass) {
            out.put("classification", "BLOCKED_RAWCB_INT32_REQUEST_READBACK_MISMATCH")
            return ApplyResult(false, out)
        }

        val attachError = runCatching { config.setSessionParameters(request) }.exceptionOrNull()
        if (attachError != null) {
            out.put("classification", "BLOCKED_RAWCB_INT32_SET_SESSION_PARAMETERS_FAILED")
                .put("error", "${attachError.javaClass.simpleName}: ${attachError.message}")
            return ApplyResult(false, out)
        }

        out.put("sessionParametersAttached", true)
            .put("applied", true)
            .put("classification", "RAWCB_SOURCE_TYPE_INT32_ONE_ATTACHED_AS_SINGLE_VENDOR_SESSION_VARIABLE__SEMANTICS_UNPROVEN")
            .put("authority", "CONTROL_INTERVENTION_ONLY_NOT_SENSOR_OR_RAW_SEMANTICS_PROOF")
        return ApplyResult(true, out)
    }
}
