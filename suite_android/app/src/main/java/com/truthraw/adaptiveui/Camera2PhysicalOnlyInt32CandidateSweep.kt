package com.truthraw.adaptiveui

import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.params.SessionConfiguration
import org.json.JSONArray
import org.json.JSONObject
import java.util.Locale

/**
 * v0.38 bounded key-domain sweep for the three remaining v0.36 physical-only INT32 candidates.
 *
 * v0.36 resolved all three as native INT32 with unique tags. v0.37 separately established
 * that a physical-only advertised candidate can pass framework/session attachment and complete
 * a source-first capture without changing RAW topology. v0.38 therefore screens the remaining
 * INT32 cohort one key at a time at numeric stimulus 1.
 *
 * Exactly one unknown vendor key is written per run. No combinations and no value enumeration.
 * Vendor names and numeric values remain semantically uninterpreted.
 */
object Camera2PhysicalOnlyInt32CandidateSweep {
    const val TOTAL_RUNS = 3

    data class Profile(
        val runIndex: Int,
        val symbol: String,
        val keyName: String,
        val tagHex: String,
    ) {
        val id: String get() = String.format(Locale.ROOT, "P%02d_%s", runIndex + 1, symbol)
        val value: Int get() = 1
        val selectedCount: Int get() = 1
        val bits: String get() = "$symbol=1 · others=UNSET"
    }

    data class ApplyResult(
        val applied: Boolean,
        val evidence: JSONObject,
    )

    private val profiles = arrayOf(
        Profile(
            0,
            "I",
            "org.codeaurora.qcamera3.sessionParameters.EnableVSR",
            "0x801F000B",
        ),
        Profile(
            1,
            "J",
            "org.codeaurora.qcamera3.sessionParameters.ExtendedMaxZoom",
            "0x801F000A",
        ),
        Profile(
            2,
            "K",
            "org.codeaurora.qcamera3.sessionParameters.enableQLL",
            "0x801F000D",
        ),
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
                    .put("candidateSymbol", p.symbol)
                    .put("keyName", p.keyName)
                    .put("nativeTagHex", p.tagHex)
                    .put("requestedNumericValue", 1),
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

        val logicalSession = logical.availableSessionKeys.orEmpty().map { it.name }.toSet()
        val physicalSession = physical.availableSessionKeys.orEmpty().map { it.name }.toSet()
        val logicalRequest = logical.availableCaptureRequestKeys.orEmpty().map { it.name }.toSet()
        val physicalRequest = physical.availableCaptureRequestKeys.orEmpty().map { it.name }.toSet()
        val physicalOverride = logical.availablePhysicalCameraRequestKeys.orEmpty().map { it.name }.toSet()

        val physicalOnlyTopology =
            profile.keyName !in logicalSession &&
            profile.keyName in physicalSession &&
            profile.keyName !in logicalRequest &&
            profile.keyName in physicalRequest &&
            profile.keyName !in physicalOverride

        val out = JSONObject()
            .put("schema", "truthraw.camera2-physical-only-int32-candidate-sweep.v0.38")
            .put("experiment", "V036_PHYSICAL_ONLY_INT32_CANDIDATE_SCREEN_AT_NUMERIC_ONE")
            .put("runIndex", profile.runIndex)
            .put("runNumber", profile.runIndex + 1)
            .put("profileId", profile.id)
            .put("candidateSymbol", profile.symbol)
            .put("keyName", profile.keyName)
            .put("nativeTagHexFromV036", profile.tagHex)
            .put("resolvedNativeTypeFromV036", "INT32")
            .put("nativeTypeEvidenceSource", "TRUTHRAW_CAM5_PHYSICAL_ROUTE_NATIVE_TYPE_ORACLE_v036.json")
            .put("runOrder", runOrderEvidence())
            .put("selectionBasis", "REMAINING_V036_PHYSICAL_ONLY_INT32_COHORT")
            .put("selectionUsesVendorNameSemantics", false)
            .put("singleUnknownVendorVariable", true)
            .put("otherUnknownVendorFactorsWritten", 0)
            .put("logicalSessionAdvertised", profile.keyName in logicalSession)
            .put("physicalSessionAdvertised", profile.keyName in physicalSession)
            .put("logicalRequestAdvertised", profile.keyName in logicalRequest)
            .put("physicalRequestAdvertised", profile.keyName in physicalRequest)
            .put("physicalOverrideAdvertised", profile.keyName in physicalOverride)
            .put("physicalOnlyAvailabilityTopologyPass", physicalOnlyTopology)
            .put("writeRepresentation", "CaptureRequest.Key<Int> + Int(1)")
            .put("requestedNumericValue", 1)
            .put("requestedValueSemanticsAssumed", false)
            .put("semanticMeaningAssumed", false)
            .put("semanticPromotionAllowed", false)
            .put("pixelAccessBeforeAttachment", false)
            .put("sourceMutation", false)
            .put("controlReference", "TruthRaw v0.20 acquisition/payload chain")
            .put("v037Reference", "physical-only BYTE candidate H attachment+capture PASS, no RAW topology differential")
            .put("sessionParametersAttached", false)
            .put("vendorKeysWritten", 0)
            .put("applied", false)

        if (!physicalOnlyTopology) {
            out.put("classification", "BLOCKED_V038_PHYSICAL_ONLY_AVAILABILITY_TOPOLOGY_MISMATCH")
            return ApplyResult(false, out)
        }

        val key = CaptureRequest.Key(profile.keyName, Int::class.javaObjectType)
        val builder = runCatching { device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE) }
            .getOrElse { e ->
                out.put("classification", "BLOCKED_V038_INT32_BUILDER_CREATE_FAILED")
                    .put("error", "${e.javaClass.simpleName}: ${e.message}")
                return ApplyResult(false, out)
            }

        val before = runCatching { builder.get(key) }.getOrNull()
        out.put("builderReadbackBeforeSet", before ?: JSONObject.NULL)

        val setError = runCatching { builder.set(key, 1) }.exceptionOrNull()
        if (setError != null) {
            out.put("classification", "BLOCKED_V038_INT32_SET_FAILED")
                .put("error", "${setError.javaClass.simpleName}: ${setError.message}")
            return ApplyResult(false, out)
        }
        out.put("builderSetPass", true)

        val builderReadback = runCatching { builder.get(key) }.getOrNull()
        val builderReadbackPass = builderReadback == 1
        out.put("builderReadback", builderReadback ?: JSONObject.NULL)
            .put("builderReadbackPass", builderReadbackPass)
        if (!builderReadbackPass) {
            out.put("classification", "BLOCKED_V038_INT32_BUILDER_READBACK_MISMATCH")
            return ApplyResult(false, out)
        }

        val request = runCatching { builder.build() }.getOrElse { e ->
            out.put("classification", "BLOCKED_V038_REQUEST_BUILD_FAILED")
                .put("error", "${e.javaClass.simpleName}: ${e.message}")
            return ApplyResult(false, out)
        }
        val requestReadback = runCatching { request.get(key) }.getOrNull()
        val requestReadbackPass = requestReadback == 1
        out.put("requestReadback", requestReadback ?: JSONObject.NULL)
            .put("requestReadbackPass", requestReadbackPass)
        if (!requestReadbackPass) {
            out.put("classification", "BLOCKED_V038_INT32_REQUEST_READBACK_MISMATCH")
            return ApplyResult(false, out)
        }

        val attachError = runCatching { config.setSessionParameters(request) }.exceptionOrNull()
        if (attachError != null) {
            out.put("sessionParametersAttached", false)
                .put("capturePermitted", false)
                .put("classification", "V038_PHYSICAL_ONLY_INT32_REQUEST_VALID__SESSION_ATTACHMENT_REJECTED__NO_CAPTURE")
                .put("error", "${attachError.javaClass.simpleName}: ${attachError.message}")
                .put("authority", "ATTACHMENT_FEASIBILITY_NEGATIVE_RESULT_ONLY")
            return ApplyResult(false, out)
        }

        out.put("sessionParametersAttached", true)
            .put("capturePermitted", true)
            .put("vendorKeysWritten", 1)
            .put("applied", true)
            .put("classification", "V038_PHYSICAL_ONLY_INT32_ONE_ATTACHED__SEMANTICS_UNPROVEN")
            .put("authority", "CONTROL_INTERVENTION_ONLY_NOT_SENSOR_OR_VENDOR_SEMANTICS_PROOF")
        return ApplyResult(true, out)
    }
}
