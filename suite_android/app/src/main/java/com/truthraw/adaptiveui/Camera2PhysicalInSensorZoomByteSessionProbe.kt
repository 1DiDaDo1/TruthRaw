package com.truthraw.adaptiveui

import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.params.SessionConfiguration
import org.json.JSONObject

/**
 * v0.37 isolated attachment-feasibility + conditional capture intervention for
 * `org.codeaurora.qcamera3.sessionParameters.inSensorZoomEnable`.
 *
 * v0.36 resolved this device's native camera_metadata element type as BYTE:
 * - vendor tag 0x801F0029
 * - exactly one accepted native type
 * - BYTE set/get accepted with count=1 on a disposable logical-0 request template
 *   after vendor-tag lookup from physical Camera-5 characteristics
 * - INT32/FLOAT/INT64/DOUBLE/RATIONAL rejected
 * - no session/capture/HAL submission occurred in the oracle.
 *
 * Availability topology from v0.35/v0.36:
 * - logical request: false
 * - physical request: true
 * - logical session: false
 * - physical session: true
 * - physical override: false
 *
 * Numeric value 1 is only a controlled representation-valid stimulus.
 * No semantic meaning is assumed from the vendor key name or numeric value.
 */
object Camera2PhysicalInSensorZoomByteSessionProbe {
    const val KEY_NAME = "org.codeaurora.qcamera3.sessionParameters.inSensorZoomEnable"
    const val NATIVE_TAG_HEX = "0x801F0029"
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
        val physicalOverrideAdvertised = logical.availablePhysicalCameraRequestKeys.orEmpty().any { it.name == KEY_NAME }

        val physicalOnlyTopology =
            !logicalSessionAdvertised &&
            physicalSessionAdvertised &&
            !logicalRequestAdvertised &&
            physicalRequestAdvertised &&
            !physicalOverrideAdvertised

        val out = JSONObject()
            .put("schema", "truthraw.camera2-physical-insensorzoom-byte-session-intervention.v0.37")
            .put("experiment", "PHYSICAL_ONLY_INSENSORZOOM_SESSION_PARAMETER_BYTE_ONE")
            .put("keyName", KEY_NAME)
            .put("nativeTagHexFromV036", NATIVE_TAG_HEX)
            .put("resolvedNativeTypeFromV036", RESOLVED_NATIVE_TYPE)
            .put("nativeTypeEvidenceSource", "TRUTHRAW_CAM5_PHYSICAL_ROUTE_NATIVE_TYPE_ORACLE_v036.json")
            .put("selectionBasis", "PHYSICAL5_ONLY_AVAILABILITY_PLUS_ONLY_BYTE_MEMBER_OF_V036_SET")
            .put("selectionUsesVendorNameSemantics", false)
            .put("singleUnknownVendorVariable", true)
            .put("logicalSessionAdvertised", logicalSessionAdvertised)
            .put("physicalSessionAdvertised", physicalSessionAdvertised)
            .put("logicalRequestAdvertised", logicalRequestAdvertised)
            .put("physicalRequestAdvertised", physicalRequestAdvertised)
            .put("physicalOverrideAdvertised", physicalOverrideAdvertised)
            .put("physicalOnlyAvailabilityTopologyPass", physicalOnlyTopology)
            .put("writeRepresentation", "CaptureRequest.Key<Byte> + Byte(1)")
            .put("requestedNumericValue", 1)
            .put("requestedValueSemanticsAssumed", false)
            .put("otherV036CandidatesWritten", false)
            .put("pixelAccessBeforeAttachment", false)
            .put("sourceMutation", false)
            .put("semanticMeaningAssumed", false)
            .put("semanticPromotionAllowed", false)
            .put("controlReference", "TruthRaw v0.20 unchanged")
            .put("attachmentFeasibilityExperiment", true)
            .put("captureAllowedOnlyAfterAttachmentPass", true)
            .put("applied", false)

        if (!physicalOnlyTopology) {
            out.put("classification", "BLOCKED_V037_PHYSICAL_ONLY_AVAILABILITY_TOPOLOGY_MISMATCH")
            return ApplyResult(false, out)
        }

        val key = CaptureRequest.Key(KEY_NAME, Byte::class.javaObjectType)
        val builder = runCatching { device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE) }
            .getOrElse { e ->
                out.put("classification", "BLOCKED_V037_BYTE_BUILDER_CREATE_FAILED")
                    .put("error", "${e.javaClass.simpleName}: ${e.message}")
                return ApplyResult(false, out)
            }

        val before = runCatching { builder.get(key) }.getOrNull()
        out.put("builderReadbackBeforeSet", before?.toInt() ?: JSONObject.NULL)

        val setError = runCatching { builder.set(key, 1.toByte()) }.exceptionOrNull()
        if (setError != null) {
            out.put("classification", "BLOCKED_V037_BYTE_SET_FAILED")
                .put("error", "${setError.javaClass.simpleName}: ${setError.message}")
            return ApplyResult(false, out)
        }
        out.put("builderSetPass", true)

        val builderReadback = runCatching { builder.get(key) }.getOrNull()
        val builderReadbackPass = builderReadback?.toInt() == 1
        out.put("builderReadback", builderReadback?.toInt() ?: JSONObject.NULL)
            .put("builderReadbackPass", builderReadbackPass)
        if (!builderReadbackPass) {
            out.put("classification", "BLOCKED_V037_BYTE_BUILDER_READBACK_MISMATCH")
            return ApplyResult(false, out)
        }

        val request = runCatching { builder.build() }.getOrElse { e ->
            out.put("classification", "BLOCKED_V037_BYTE_REQUEST_BUILD_FAILED")
                .put("error", "${e.javaClass.simpleName}: ${e.message}")
            return ApplyResult(false, out)
        }

        val requestReadback = runCatching { request.get(key) }.getOrNull()
        val requestReadbackPass = requestReadback?.toInt() == 1
        out.put("requestReadback", requestReadback?.toInt() ?: JSONObject.NULL)
            .put("requestReadbackPass", requestReadbackPass)
        if (!requestReadbackPass) {
            out.put("classification", "BLOCKED_V037_BYTE_REQUEST_READBACK_MISMATCH")
            return ApplyResult(false, out)
        }

        // This is the actual v0.37 feasibility boundary. The key is not advertised on the
        // logical session surface, so a rejection is a valid negative result and must stop capture.
        val attachError = runCatching { config.setSessionParameters(request) }.exceptionOrNull()
        if (attachError != null) {
            out.put("sessionParametersAttached", false)
                .put("capturePermitted", false)
                .put("classification", "V037_PHYSICAL_ONLY_BYTE_REQUEST_VALID__SESSION_ATTACHMENT_REJECTED__NO_CAPTURE")
                .put("error", "${attachError.javaClass.simpleName}: ${attachError.message}")
                .put("authority", "ATTACHMENT_FEASIBILITY_NEGATIVE_RESULT_ONLY")
            return ApplyResult(false, out)
        }

        out.put("sessionParametersAttached", true)
            .put("capturePermitted", true)
            .put("vendorKeysWritten", 1)
            .put("applied", true)
            .put("classification", "V037_PHYSICAL_ONLY_BYTE_ONE_ATTACHED_AS_SINGLE_VENDOR_SESSION_VARIABLE__SEMANTICS_UNPROVEN")
            .put("authority", "CONTROL_INTERVENTION_ONLY_NOT_SENSOR_OR_RAW_SEMANTICS_PROOF")
        return ApplyResult(true, out)
    }
}
