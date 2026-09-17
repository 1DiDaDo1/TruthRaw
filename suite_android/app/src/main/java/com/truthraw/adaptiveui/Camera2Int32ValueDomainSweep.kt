package com.truthraw.adaptiveui

import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.params.SessionConfiguration
import org.json.JSONArray
import org.json.JSONObject
import java.util.Locale

/**
 * v0.31 bounded value-domain sweep for the two INT32 vendor controls that were already
 * representation-resolved and included in the completed v0.30 2^4 binary matrix.
 *
 * v0.30 proved that UNSET versus numeric 1, including all combinations with the BYTE factors,
 * produced no measurable RAW envelope/populated-prefix topology differential. v0.31 therefore
 * stops repeating the binary matrix and probes the next smallest bounded INT32 values: 0, 2, 3.
 *
 * Exactly one unknown vendor key is written per run. A and C remain UNSET in all v0.31 runs.
 * Numeric values are stimuli only; no vendor semantics are assumed or promoted.
 */
object Camera2Int32ValueDomainSweep {
    const val KEY_RAWCB = "org.codeaurora.qcamera3.sessionParameters.RawCbSourceType"
    const val KEY_HAL_COMBINED = "org.codeaurora.qcamera3.sessionParameters.HALOutputBufferCombined"
    const val TAG_RAWCB = "0x801F0009"
    const val TAG_HAL_COMBINED = "0x801F0034"
    const val TOTAL_RUNS = 6

    data class Profile(
        val runIndex: Int,
        val target: String,
        val value: Int,
    ) {
        val bits: String get() = when (target) {
            "RAWCB" -> "B=$value · D=UNSET"
            "HALCOMBINED" -> "B=UNSET · D=$value"
            else -> "UNKNOWN"
        }
        val id: String get() = String.format(Locale.ROOT, "S%02d_%s_%d", runIndex + 1, target, value)
        val selectedCount: Int get() = 1
    }

    data class ApplyResult(
        val applied: Boolean,
        val evidence: JSONObject,
    )

    private val profiles = arrayOf(
        Profile(0, "RAWCB", 0),
        Profile(1, "RAWCB", 2),
        Profile(2, "RAWCB", 3),
        Profile(3, "HALCOMBINED", 0),
        Profile(4, "HALCOMBINED", 2),
        Profile(5, "HALCOMBINED", 3),
    )

    fun profileForRun(runIndex: Int): Profile {
        require(runIndex in profiles.indices) { "runIndex=$runIndex outside 0..${profiles.lastIndex}" }
        return profiles[runIndex]
    }

    fun runOrderEvidence(): JSONArray = JSONArray().apply {
        profiles.forEach { p ->
            put(
                JSONObject()
                    .put("runIndex", p.runIndex)
                    .put("runNumber", p.runIndex + 1)
                    .put("profileId", p.id)
                    .put("target", p.target)
                    .put("requestedNumericValue", p.value),
            )
        }
    }

    fun applyProfile(
        device: CameraDevice,
        logical: CameraCharacteristics,
        physical: CameraCharacteristics,
        config: SessionConfiguration,
        runIndex: Int,
    ): ApplyResult {
        val profile = profileForRun(runIndex)
        val logicalSessionNames = logical.availableSessionKeys.orEmpty().map { it.name }.toSet()
        val physicalSessionNames = physical.availableSessionKeys.orEmpty().map { it.name }.toSet()
        val logicalRequestNames = logical.availableCaptureRequestKeys.orEmpty().map { it.name }.toSet()
        val physicalRequestNames = physical.availableCaptureRequestKeys.orEmpty().map { it.name }.toSet()
        val keyName = if (profile.target == "RAWCB") KEY_RAWCB else KEY_HAL_COMBINED
        val tagHex = if (profile.target == "RAWCB") TAG_RAWCB else TAG_HAL_COMBINED
        val nativeTypeEvidenceSource = if (profile.target == "RAWCB") {
            "TRUTHRAW_CAM5_RAWCB_SOURCE_TYPE_NATIVE_TYPE_ORACLE_v025.json"
        } else {
            "TRUTHRAW_CAM5_HAL_OUTPUT_BUFFER_COMBINED_NATIVE_TYPE_ORACLE_v029.json"
        }

        val out = JSONObject()
            .put("schema", "truthraw.camera2-int32-value-domain-sweep.v0.31")
            .put("experiment", "CAMERA5_INT32_VALUE_DOMAIN_SWEEP_0_2_3")
            .put("runIndex", profile.runIndex)
            .put("runNumber", profile.runIndex + 1)
            .put("profileId", profile.id)
            .put("target", profile.target)
            .put("keyName", keyName)
            .put("nativeTagHex", tagHex)
            .put("resolvedNativeType", "INT32")
            .put("nativeTypeEvidenceSource", nativeTypeEvidenceSource)
            .put("requestedNumericValue", profile.value)
            .put("testedValueDomainThisStage", JSONArray(listOf(0, 2, 3)))
            .put("numericOneReference", "Already screened in completed v0.30 binary matrix; not repeated here")
            .put("unsetReference", "Already screened in completed v0.30 binary matrix; not repeated here")
            .put("runOrder", runOrderEvidence())
            .put("singleUnknownVendorVariable", true)
            .put("otherUnknownVendorFactorsWritten", 0)
            .put("logicalSessionAdvertised", logicalSessionNames.contains(keyName))
            .put("physicalSessionAdvertised", physicalSessionNames.contains(keyName))
            .put("logicalRequestAdvertised", logicalRequestNames.contains(keyName))
            .put("physicalRequestAdvertised", physicalRequestNames.contains(keyName))
            .put("writeRepresentation", "CaptureRequest.Key<Int> + Int(value)")
            .put("requestedValueSemanticsAssumed", false)
            .put("semanticMeaningAssumed", false)
            .put("semanticPromotionAllowed", false)
            .put("pixelAccess", false)
            .put("sourceMutation", false)
            .put("controlReference", "TruthRaw v0.20 acquisition/payload chain")
            .put("completedBinaryMatrixReference", "v0.30 16/16 no measurable topology differential")
            .put("sessionParametersAttached", false)
            .put("vendorKeysWritten", 0)
            .put("applied", false)

        if (!logicalSessionNames.contains(keyName)) {
            out.put("classification", "BLOCKED_V031_TARGET_NOT_ADVERTISED_AS_LOGICAL_SESSION_KEY")
            return ApplyResult(false, out)
        }

        val key = CaptureRequest.Key(keyName, Int::class.javaObjectType)
        val builder = runCatching { device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE) }
            .getOrElse { e ->
                out.put("classification", "BLOCKED_V031_SESSION_PARAMETER_BUILDER_CREATE_FAILED")
                    .put("error", "${e.javaClass.simpleName}: ${e.message}")
                return ApplyResult(false, out)
            }

        val before = runCatching { builder.get(key) }.getOrNull()
        out.put("builderReadbackBeforeSet", before ?: JSONObject.NULL)

        val setError = runCatching { builder.set(key, profile.value) }.exceptionOrNull()
        if (setError != null) {
            out.put("classification", "BLOCKED_V031_INT32_SET_FAILED")
                .put("error", "${setError.javaClass.simpleName}: ${setError.message}")
            return ApplyResult(false, out)
        }

        val builderReadback = runCatching { builder.get(key) }.getOrNull()
        val builderReadbackPass = builderReadback == profile.value
        out.put("builderReadback", builderReadback ?: JSONObject.NULL)
            .put("builderReadbackPass", builderReadbackPass)
        if (!builderReadbackPass) {
            out.put("classification", "BLOCKED_V031_INT32_BUILDER_READBACK_MISMATCH")
            return ApplyResult(false, out)
        }

        val request = runCatching { builder.build() }.getOrElse { e ->
            out.put("classification", "BLOCKED_V031_SESSION_PARAMETER_REQUEST_BUILD_FAILED")
                .put("error", "${e.javaClass.simpleName}: ${e.message}")
            return ApplyResult(false, out)
        }
        val requestReadback = runCatching { request.get(key) }.getOrNull()
        val requestReadbackPass = requestReadback == profile.value
        out.put("requestReadback", requestReadback ?: JSONObject.NULL)
            .put("requestReadbackPass", requestReadbackPass)
        if (!requestReadbackPass) {
            out.put("classification", "BLOCKED_V031_INT32_REQUEST_READBACK_MISMATCH")
            return ApplyResult(false, out)
        }

        val attachError = runCatching { config.setSessionParameters(request) }.exceptionOrNull()
        if (attachError != null) {
            out.put("classification", "BLOCKED_V031_SET_SESSION_PARAMETERS_FAILED")
                .put("error", "${attachError.javaClass.simpleName}: ${attachError.message}")
            return ApplyResult(false, out)
        }

        out.put("sessionParametersAttached", true)
            .put("vendorKeysWritten", 1)
            .put("applied", true)
            .put("classification", "INT32_VALUE_DOMAIN_PROFILE_ATTACHED__SEMANTICS_UNPROVEN")
            .put("authority", "BOUNDED_VALUE_DOMAIN_INTERVENTION_ONLY_NOT_SENSOR_OR_VENDOR_SEMANTICS_PROOF")
        return ApplyResult(true, out)
    }
}
