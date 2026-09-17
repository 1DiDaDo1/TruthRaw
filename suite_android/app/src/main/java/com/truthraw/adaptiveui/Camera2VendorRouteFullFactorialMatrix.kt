package com.truthraw.adaptiveui

import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.params.SessionConfiguration
import org.json.JSONArray
import org.json.JSONObject
import java.util.Locale

/**
 * v0.30 controlled full-factorial route screening across four already type-resolved vendor keys.
 *
 * Factor levels are deliberately NOT interpreted as vendor semantics:
 *   low  = UNSET (no write; preserves the v0.20-style baseline for that key)
 *   high = numeric 1 written with the on-device native metadata representation already resolved
 *          by the earlier type oracles.
 *
 * Factors:
 *   A EnableIdealRAW            BYTE  tag 0x801F0027
 *   B RawCbSourceType           INT32 tag 0x801F0009
 *   C EnableXCFAOptimization    BYTE  tag 0x801F0036
 *   D HALOutputBufferCombined   INT32 tag 0x801F0034
 *
 * All 2^4 = 16 combinations are present exactly once. Run order is complement-paired to reduce
 * monotonic scene/time drift confounding: each run is immediately followed by its four-factor
 * complement. This is a route/topology screening experiment, not a semantic decoder.
 */
object Camera2VendorRouteFullFactorialMatrix {
    const val KEY_IDEAL = "org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW"
    const val KEY_RAWCB = "org.codeaurora.qcamera3.sessionParameters.RawCbSourceType"
    const val KEY_XCFA = "org.codeaurora.qcamera3.sessionParameters.EnableXCFAOptimization"
    const val KEY_HAL_COMBINED = "org.codeaurora.qcamera3.sessionParameters.HALOutputBufferCombined"

    const val TAG_IDEAL = "0x801F0027"
    const val TAG_RAWCB = "0x801F0009"
    const val TAG_XCFA = "0x801F0036"
    const val TAG_HAL_COMBINED = "0x801F0034"

    const val TOTAL_RUNS = 16

    // Binary bits are ABCD = IdealRAW, RawCbSourceType, XCFA, HALOutputBufferCombined.
    // Adjacent entries are complements: 0000/1111, 0101/1010, ...
    private val runOrder = intArrayOf(
        0b0000, 0b1111,
        0b0101, 0b1010,
        0b0011, 0b1100,
        0b0110, 0b1001,
        0b0001, 0b1110,
        0b0010, 0b1101,
        0b0100, 0b1011,
        0b1000, 0b0111,
    )

    data class Profile(
        val runIndex: Int,
        val mask: Int,
    ) {
        val ideal: Boolean get() = mask and 0b1000 != 0
        val rawCb: Boolean get() = mask and 0b0100 != 0
        val xcfa: Boolean get() = mask and 0b0010 != 0
        val halCombined: Boolean get() = mask and 0b0001 != 0
        val selectedCount: Int get() = listOf(ideal, rawCb, xcfa, halCombined).count { it }
        val bits: String get() = Integer.toBinaryString(mask).padStart(4, '0')
        val id: String get() = String.format(Locale.ROOT, "R%02d_ABCD_%s", runIndex + 1, bits)
    }

    data class ApplyResult(
        val applied: Boolean,
        val evidence: JSONObject,
    )

    fun profileForRun(runIndex: Int): Profile {
        require(runIndex in 0 until TOTAL_RUNS) { "runIndex=$runIndex outside 0..${TOTAL_RUNS - 1}" }
        return Profile(runIndex, runOrder[runIndex])
    }

    fun runOrderEvidence(): JSONArray = JSONArray().apply {
        for (i in 0 until TOTAL_RUNS) {
            val p = profileForRun(i)
            put(JSONObject().put("runIndex", i).put("runNumber", i + 1).put("profileId", p.id).put("ABCD", p.bits))
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

        fun factor(
            letter: String,
            keyName: String,
            nativeType: String,
            tagHex: String,
            selected: Boolean,
        ): JSONObject = JSONObject()
            .put("factor", letter)
            .put("keyName", keyName)
            .put("nativeType", nativeType)
            .put("tagHex", tagHex)
            .put("selectedHighLevel", selected)
            .put("lowLevel", "UNSET_NO_WRITE")
            .put("highLevel", "NUMERIC_ONE_TYPE_VALIDATED__SEMANTICS_UNPROVEN")
            .put("logicalSessionAdvertised", logicalSessionNames.contains(keyName))
            .put("physicalSessionAdvertised", physicalSessionNames.contains(keyName))
            .put("logicalRequestAdvertised", logicalRequestNames.contains(keyName))
            .put("physicalRequestAdvertised", physicalRequestNames.contains(keyName))

        val factors = JSONArray()
            .put(factor("A", KEY_IDEAL, "BYTE", TAG_IDEAL, profile.ideal))
            .put(factor("B", KEY_RAWCB, "INT32", TAG_RAWCB, profile.rawCb))
            .put(factor("C", KEY_XCFA, "BYTE", TAG_XCFA, profile.xcfa))
            .put(factor("D", KEY_HAL_COMBINED, "INT32", TAG_HAL_COMBINED, profile.halCombined))

        val out = JSONObject()
            .put("schema", "truthraw.camera2-vendor-route-full-factorial-matrix.v0.30")
            .put("experiment", "CAMERA5_VENDOR_ROUTE_FULL_FACTORIAL_2_LEVEL_4_FACTOR")
            .put("runIndex", profile.runIndex)
            .put("runNumber", profile.runIndex + 1)
            .put("profileId", profile.id)
            .put("ABCD", profile.bits)
            .put("factorCount", 4)
            .put("fullFactorialRunCount", TOTAL_RUNS)
            .put("selectedHighFactorCount", profile.selectedCount)
            .put("factorLowLevel", "UNSET_NO_WRITE")
            .put("factorHighLevel", "NUMERIC_ONE_TYPE_VALIDATED__SEMANTICS_UNPROVEN")
            .put("runOrderStrategy", "COMPLEMENT_PAIRED_FULL_FACTORIAL_TO_LIMIT_MONOTONIC_DRIFT")
            .put("runOrder", runOrderEvidence())
            .put("factors", factors)
            .put("singleVariableExperiment", false)
            .put("designedMultiFactorExperiment", true)
            .put("allCombinationsCoveredExactlyOnce", true)
            .put("requestedValueSemanticsAssumed", false)
            .put("semanticMeaningAssumed", false)
            .put("semanticPromotionAllowed", false)
            .put("pixelAccess", false)
            .put("sourceMutation", false)
            .put("controlReference", "TruthRaw v0.20 acquisition/payload chain")
            .put("sessionParametersAttached", false)
            .put("vendorKeysWritten", 0)
            .put("applied", false)

        // R01/0000 is the untouched control. Crucially, low level means UNSET, not explicit zero.
        if (profile.mask == 0) {
            out.put("applied", true)
                .put("classification", "FULL_FACTORIAL_CONTROL__ALL_FOUR_VENDOR_KEYS_UNSET")
                .put("authority", "CONTROL_ROUTE_OBSERVATION_ONLY")
            return ApplyResult(true, out)
        }

        val selectedNames = buildList {
            if (profile.ideal) add(KEY_IDEAL)
            if (profile.rawCb) add(KEY_RAWCB)
            if (profile.xcfa) add(KEY_XCFA)
            if (profile.halCombined) add(KEY_HAL_COMBINED)
        }
        val missingLogicalSession = selectedNames.filterNot { logicalSessionNames.contains(it) }
        if (missingLogicalSession.isNotEmpty()) {
            out.put("classification", "BLOCKED_MATRIX_SELECTED_KEY_NOT_ADVERTISED_AS_LOGICAL_SESSION_KEY")
                .put("missingLogicalSessionKeys", JSONArray(missingLogicalSession))
            return ApplyResult(false, out)
        }

        val idealKey = CaptureRequest.Key(KEY_IDEAL, Byte::class.javaObjectType)
        val rawCbKey = CaptureRequest.Key(KEY_RAWCB, Int::class.javaObjectType)
        val xcfaKey = CaptureRequest.Key(KEY_XCFA, Byte::class.javaObjectType)
        val halKey = CaptureRequest.Key(KEY_HAL_COMBINED, Int::class.javaObjectType)

        val builder = runCatching { device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE) }
            .getOrElse { e ->
                out.put("classification", "BLOCKED_MATRIX_SESSION_PARAMETER_BUILDER_CREATE_FAILED")
                    .put("error", "${e.javaClass.simpleName}: ${e.message}")
                return ApplyResult(false, out)
            }

        val before = JSONObject()
            .put("A", runCatching { builder.get(idealKey) }.getOrNull()?.toInt() ?: JSONObject.NULL)
            .put("B", runCatching { builder.get(rawCbKey) }.getOrNull() ?: JSONObject.NULL)
            .put("C", runCatching { builder.get(xcfaKey) }.getOrNull()?.toInt() ?: JSONObject.NULL)
            .put("D", runCatching { builder.get(halKey) }.getOrNull() ?: JSONObject.NULL)
        out.put("builderReadbackBeforeSet", before)

        val setErrors = JSONArray()
        fun recordSet(letter: String, block: () -> Unit) {
            runCatching(block).exceptionOrNull()?.let { e ->
                setErrors.put(JSONObject().put("factor", letter).put("error", "${e.javaClass.simpleName}: ${e.message}"))
            }
        }
        if (profile.ideal) recordSet("A") { builder.set(idealKey, 1.toByte()) }
        if (profile.rawCb) recordSet("B") { builder.set(rawCbKey, 1) }
        if (profile.xcfa) recordSet("C") { builder.set(xcfaKey, 1.toByte()) }
        if (profile.halCombined) recordSet("D") { builder.set(halKey, 1) }
        if (setErrors.length() > 0) {
            out.put("classification", "BLOCKED_MATRIX_VENDOR_KEY_SET_FAILED").put("setErrors", setErrors)
            return ApplyResult(false, out)
        }

        val after = JSONObject()
            .put("A", runCatching { builder.get(idealKey) }.getOrNull()?.toInt() ?: JSONObject.NULL)
            .put("B", runCatching { builder.get(rawCbKey) }.getOrNull() ?: JSONObject.NULL)
            .put("C", runCatching { builder.get(xcfaKey) }.getOrNull()?.toInt() ?: JSONObject.NULL)
            .put("D", runCatching { builder.get(halKey) }.getOrNull() ?: JSONObject.NULL)
        out.put("builderReadbackAfterSet", after)

        val builderPass = (!profile.ideal || after.optInt("A", -1) == 1) &&
            (!profile.rawCb || after.optInt("B", -1) == 1) &&
            (!profile.xcfa || after.optInt("C", -1) == 1) &&
            (!profile.halCombined || after.optInt("D", -1) == 1)
        out.put("builderReadbackPass", builderPass)
        if (!builderPass) {
            out.put("classification", "BLOCKED_MATRIX_BUILDER_READBACK_MISMATCH")
            return ApplyResult(false, out)
        }

        val request = runCatching { builder.build() }.getOrElse { e ->
            out.put("classification", "BLOCKED_MATRIX_SESSION_PARAMETER_REQUEST_BUILD_FAILED")
                .put("error", "${e.javaClass.simpleName}: ${e.message}")
            return ApplyResult(false, out)
        }
        val requestReadback = JSONObject()
            .put("A", runCatching { request.get(idealKey) }.getOrNull()?.toInt() ?: JSONObject.NULL)
            .put("B", runCatching { request.get(rawCbKey) }.getOrNull() ?: JSONObject.NULL)
            .put("C", runCatching { request.get(xcfaKey) }.getOrNull()?.toInt() ?: JSONObject.NULL)
            .put("D", runCatching { request.get(halKey) }.getOrNull() ?: JSONObject.NULL)
        out.put("requestReadback", requestReadback)

        val requestPass = (!profile.ideal || requestReadback.optInt("A", -1) == 1) &&
            (!profile.rawCb || requestReadback.optInt("B", -1) == 1) &&
            (!profile.xcfa || requestReadback.optInt("C", -1) == 1) &&
            (!profile.halCombined || requestReadback.optInt("D", -1) == 1)
        out.put("requestReadbackPass", requestPass)
        if (!requestPass) {
            out.put("classification", "BLOCKED_MATRIX_REQUEST_READBACK_MISMATCH")
            return ApplyResult(false, out)
        }

        val attachError = runCatching { config.setSessionParameters(request) }.exceptionOrNull()
        if (attachError != null) {
            out.put("classification", "BLOCKED_MATRIX_SET_SESSION_PARAMETERS_FAILED")
                .put("error", "${attachError.javaClass.simpleName}: ${attachError.message}")
            return ApplyResult(false, out)
        }

        out.put("sessionParametersAttached", true)
            .put("vendorKeysWritten", profile.selectedCount)
            .put("applied", true)
            .put("classification", "FULL_FACTORIAL_PROFILE_ATTACHED__SEMANTICS_UNPROVEN")
            .put("authority", "DESIGNED_ROUTE_INTERVENTION_ONLY_NOT_SENSOR_OR_VENDOR_SEMANTICS_PROOF")
        return ApplyResult(true, out)
    }
}
