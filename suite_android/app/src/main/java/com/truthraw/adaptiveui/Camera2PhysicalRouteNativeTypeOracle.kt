package com.truthraw.adaptiveui

import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import org.json.JSONArray
import org.json.JSONObject

/**
 * v0.36 representation-only oracle for QTI session/request keys that are advertised
 * on physical Camera 5 but not on logical Camera 0 in the v0.35 device evidence.
 *
 * Candidate selection is based on availability topology, not on vendor-name semantics.
 * Tag lookup uses physical Camera-5 characteristics. Disposable request templates use
 * logical Camera 0 so this diagnostic does not repeat the rejected direct-open physical-5 route.
 *
 * No session is created, no session parameters are attached, no request is submitted and
 * no RAW pixels are accessed.
 */
object Camera2PhysicalRouteNativeTypeOracle {
    private data class Candidate(val symbol: String, val keyName: String)

    private val candidates = listOf(
        Candidate("H", "org.codeaurora.qcamera3.sessionParameters.inSensorZoomEnable"),
        Candidate("I", "org.codeaurora.qcamera3.sessionParameters.EnableVSR"),
        Candidate("J", "org.codeaurora.qcamera3.sessionParameters.ExtendedMaxZoom"),
        Candidate("K", "org.codeaurora.qcamera3.sessionParameters.enableQLL"),
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

        val results = JSONArray()
        var resolved = 0
        var unresolved = 0
        var ambiguous = 0

        candidates.forEach { c ->
            val availabilityPass =
                c.keyName !in logicalSession &&
                c.keyName in physicalSession &&
                c.keyName !in logicalRequest &&
                c.keyName in physicalRequest

            val result = Camera2RawCbSourceTypeNativeTypeOracle.probeKeyDecoupled(
                metadataCameraId = physicalCameraId,
                requestCameraId = logicalCameraId,
                keyName = c.keyName,
                schema = "truthraw.camera2-v036-physical-route-native-type-oracle.per-key.v0.36",
                bridgeErrorClassification = "V036_PHYSICAL_ROUTE_NATIVE_TYPE_ORACLE_BRIDGE_ERROR",
            )

            val resolvedType = result.optString("resolvedNativeType", "UNRESOLVED")
            when {
                result.optBoolean("nativeMetadataTypeResolved", false) -> resolved += 1
                resolvedType == "AMBIGUOUS" -> ambiguous += 1
                else -> unresolved += 1
            }

            result.put("candidateSymbol", c.symbol)
                .put("selectionBasis", "PHYSICAL5_ONLY_SESSION_AND_REQUEST_AVAILABILITY_IN_V035_EVIDENCE")
                .put("logicalSessionAdvertised", c.keyName in logicalSession)
                .put("physicalSessionAdvertised", c.keyName in physicalSession)
                .put("logicalRequestAdvertised", c.keyName in logicalRequest)
                .put("physicalRequestAdvertised", c.keyName in physicalRequest)
                .put("availabilityTopologyPass", availabilityPass)
                .put("candidateNameSemanticsAssumed", false)
                .put("routeEffectAssumed", false)
                .put("interventionAllowedInThisBuild", false)
            results.put(result)
        }

        val classification = when {
            resolved == candidates.size ->
                "PHYSICAL_ROUTE_NATIVE_TYPE_ORACLE_COMPLETE_4_OF_4__REPRESENTATION_ONLY"
            resolved > 0 ->
                "PHYSICAL_ROUTE_NATIVE_TYPE_ORACLE_PARTIAL__REPRESENTATION_ONLY"
            else ->
                "PHYSICAL_ROUTE_NATIVE_TYPE_ORACLE_NO_TYPE_RESOLUTION"
        }

        return JSONObject()
            .put("schema", "truthraw.camera2-v036-physical-route-native-type-oracle.v0.36")
            .put("experiment", "CAMERA5_PHYSICAL_ONLY_ROUTE_NATIVE_TYPE_SCREEN")
            .put("classification", classification)
            .put("logicalCameraId", logicalCameraId)
            .put("physicalCameraId", physicalCameraId)
            .put("controlReference", "TruthRaw v0.20 unchanged")
            .put("closedPredecessors", "v0.33/v0.34/v0.35 isolated INT32(1) interventions: no RAW topology differential")
            .put("candidateSelectionAuthority", "Physical-vs-logical availability topology in v0.35 evidence only")
            .put("candidateCount", candidates.size)
            .put("resolvedCount", resolved)
            .put("unresolvedCount", unresolved)
            .put("ambiguousCount", ambiguous)
            .put("results", results)
            .put("directPhysicalCameraOpenAttempted", false)
            .put("previousDirectPhysicalOpenKnowledgeUsed", "v0.15 rejected direct-open physical Camera 5 route")
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
                "Only candidates with unique native-type resolution and preserved physical-only availability topology may be considered for a later isolated context experiment; no blind value sweep.",
            )
    }
}
