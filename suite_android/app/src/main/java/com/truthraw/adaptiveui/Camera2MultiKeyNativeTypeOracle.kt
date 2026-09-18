package com.truthraw.adaptiveui

import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import org.json.JSONArray
import org.json.JSONObject

/**
 * v0.32 multi-key native metadata-type oracle screen.
 *
 * The candidate names are carried forward only because they were already observed in the
 * v0.20 Camera-5 route evidence. Their names are clues, not semantics. This diagnostic resolves
 * representation only by reusing the disposable NDK metadata validator from v0.25/v0.27/v0.29.
 *
 * No capture session is created, no session parameters are attached and no capture is submitted.
 * v0.20 remains source/payload authority; v0.30/v0.31 remain closed negative intervention domains.
 */
object Camera2MultiKeyNativeTypeOracle {
    const val KEY_INSENSOR_ZOOM =
        "org.codeaurora.qcamera3.sessionParameters.EnableInsensorZoom"
    const val KEY_SNAPSHOT_ONLY_INSENSOR_ZOOM =
        "org.codeaurora.qcamera3.sessionParameters.EnableSnapshotOnlyInsensorZoom"
    const val KEY_MCX_MASTER_CB =
        "org.codeaurora.qcamera3.sessionParameters.EnableMCXMasterCb"

    private data class Candidate(
        val symbol: String,
        val keyName: String,
    )

    private val candidates = listOf(
        Candidate("E", KEY_INSENSOR_ZOOM),
        Candidate("F", KEY_SNAPSHOT_ONLY_INSENSOR_ZOOM),
        Candidate("G", KEY_MCX_MASTER_CB),
    )

    fun probe(
        manager: CameraManager,
        logicalCameraId: String,
        physicalCameraId: String,
    ): JSONObject {
        val logical = manager.getCameraCharacteristics(logicalCameraId)
        val physical = manager.getCameraCharacteristics(physicalCameraId)

        val logicalSession = logical.availableSessionKeys.orEmpty().map { it.name }.toSet()
        val physicalSession = physical.availableSessionKeys.orEmpty().map { it.name }.toSet()
        val logicalRequest = logical.availableCaptureRequestKeys.orEmpty().map { it.name }.toSet()
        val physicalRequest = physical.availableCaptureRequestKeys.orEmpty().map { it.name }.toSet()
        val physicalOverride = physical.availablePhysicalCameraRequestKeys.orEmpty().map { it.name }.toSet()

        val results = JSONArray()
        var resolvedCount = 0
        var unresolvedCount = 0
        var ambiguousCount = 0

        candidates.forEach { candidate ->
            val result = Camera2RawCbSourceTypeNativeTypeOracle.probeKey(
                cameraId = logicalCameraId,
                keyName = candidate.keyName,
                schema = "truthraw.camera2-v032-multi-key-native-type-oracle.per-key.v0.32",
                bridgeErrorClassification = "V032_MULTI_KEY_NATIVE_TYPE_ORACLE_BRIDGE_ERROR",
            )
            val resolved = result.optString("resolvedNativeType", "UNRESOLVED")
            when {
                result.optBoolean("nativeMetadataTypeResolved", false) -> resolvedCount += 1
                resolved == "AMBIGUOUS" -> ambiguousCount += 1
                else -> unresolvedCount += 1
            }

            result.put("candidateSymbol", candidate.symbol)
                .put("candidateSelectionSource", "v0.20 vendorObservations.routeKeyNames")
                .put("logicalSessionAdvertised", candidate.keyName in logicalSession)
                .put("physicalSessionAdvertised", candidate.keyName in physicalSession)
                .put("logicalRequestAdvertised", candidate.keyName in logicalRequest)
                .put("physicalRequestAdvertised", candidate.keyName in physicalRequest)
                .put("physicalOverrideAdvertised", candidate.keyName in physicalOverride)
                .put("candidateNameSemanticsAssumed", false)
                .put("routeEffectAssumed", false)
                .put("interventionAllowedInThisBuild", false)

            results.put(result)
        }

        val classification = when {
            resolvedCount == candidates.size ->
                "MULTI_KEY_NATIVE_TYPE_ORACLE_COMPLETE_3_OF_3__REPRESENTATION_ONLY"
            resolvedCount > 0 ->
                "MULTI_KEY_NATIVE_TYPE_ORACLE_PARTIAL__REPRESENTATION_ONLY"
            else ->
                "MULTI_KEY_NATIVE_TYPE_ORACLE_NO_TYPE_RESOLUTION"
        }

        return JSONObject()
            .put("schema", "truthraw.camera2-v032-multi-key-native-type-oracle.v0.32")
            .put("experiment", "CAMERA5_MULTI_KEY_NATIVE_TYPE_ORACLE_SCREEN")
            .put("classification", classification)
            .put("logicalCameraId", logicalCameraId)
            .put("physicalCameraId", physicalCameraId)
            .put("controlReference", "TruthRaw v0.20 unchanged")
            .put(
                "closedPredecessor",
                "v0.31 completed tested INT32 domain {UNSET,0,1,2,3} with no measurable RAW topology differential",
            )
            .put(
                "candidateSelectionAuthority",
                "Observed v0.20 route-key names only; names do not establish vendor semantics or causal effect",
            )
            .put("candidateCount", candidates.size)
            .put("resolvedCount", resolvedCount)
            .put("unresolvedCount", unresolvedCount)
            .put("ambiguousCount", ambiguousCount)
            .put("results", results)
            .put("testValuePurpose", "TYPE_VALIDATION_ONLY_NOT_VENDOR_VALUE_SEMANTICS")
            .put("requestTemplatesCreatedOnly", true)
            .put("sessionCreated", false)
            .put("sessionParametersAttached", false)
            .put("captureSubmitted", false)
            .put("vendorModifiedRequestSubmittedToHal", false)
            .put("rawPixelAccess", false)
            .put("sourceMutation", false)
            .put("semanticMeaningAssumed", false)
            .put("semanticPromotionAllowed", false)
            .put("interventionAllowed", false)
            .put(
                "nextDecisionRule",
                "Only a uniquely type-resolved candidate with a separately justified route hypothesis may advance to a future isolated intervention",
            )
    }
}
