package com.truthraw.adaptiveui

import org.json.JSONObject

/**
 * Native Camera2 metadata-type oracle bridge.
 *
 * v0.25 used this bridge for RawCbSourceType. v0.27 reuses the same native validator for
 * EnableXCFAOptimization without creating a capture session or submitting a request to HAL.
 * The caller supplies the vendor key name and evidence schema; type resolution is representation
 * evidence only and never assigns semantics to the test value.
 */
object Camera2RawCbSourceTypeNativeTypeOracle {
    const val KEY_NAME = "org.codeaurora.qcamera3.sessionParameters.RawCbSourceType"

    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    fun probe(cameraId: String): JSONObject =
        probeKey(
            cameraId = cameraId,
            keyName = KEY_NAME,
            schema = "truthraw.camera2-rawcb-source-type-native-type-oracle.v0.25",
            bridgeErrorClassification = "RAWCB_NATIVE_TYPE_ORACLE_BRIDGE_ERROR",
        )

    fun probeKey(
        cameraId: String,
        keyName: String,
        schema: String,
        bridgeErrorClassification: String,
    ): JSONObject =
        runCatching { JSONObject(nativeProbe(cameraId, keyName)) }
            .map { raw ->
                raw.put("schema", schema)
                    .put("keyName", keyName)
                    .put("oracleBridgeReusedFrom", "v0.25 native Camera2 metadata validator")
            }
            .getOrElse { e ->
                JSONObject()
                    .put("schema", schema)
                    .put("keyName", keyName)
                    .put("classification", bridgeErrorClassification)
                    .put("error", "${e.javaClass.simpleName}: ${e.message}")
                    .put("sessionCreated", false)
                    .put("sessionParametersAttached", false)
                    .put("captureSubmitted", false)
                    .put("vendorModifiedRequestSubmittedToHal", false)
                    .put("semanticPromotionAllowed", false)
            }

    /**
     * v0.36 decoupled representation probe.
     *
     * Vendor-tag lookup is performed against one camera's characteristics while disposable
     * request templates are created on a separately specified openable camera. This exists
     * because physical Camera 5 is a child of logical 0 and the earlier direct-open route was
     * rejected. It still creates no session and submits no request.
     */
    fun probeKeyDecoupled(
        metadataCameraId: String,
        requestCameraId: String,
        keyName: String,
        schema: String,
        bridgeErrorClassification: String,
    ): JSONObject =
        runCatching { JSONObject(nativeProbeDecoupled(metadataCameraId, requestCameraId, keyName)) }
            .map { raw ->
                raw.put("schema", schema)
                    .put("keyName", keyName)
                    .put("oracleBridgeReusedFrom", "v0.25 native Camera2 metadata validator + v0.36 decoupled lookup/request route")
            }
            .getOrElse { e ->
                JSONObject()
                    .put("schema", schema)
                    .put("metadataCameraId", metadataCameraId)
                    .put("requestCameraId", requestCameraId)
                    .put("keyName", keyName)
                    .put("classification", bridgeErrorClassification)
                    .put("error", "${e.javaClass.simpleName}: ${e.message}")
                    .put("sessionCreated", false)
                    .put("sessionParametersAttached", false)
                    .put("captureSubmitted", false)
                    .put("vendorModifiedRequestSubmittedToHal", false)
                    .put("semanticPromotionAllowed", false)
            }

    private external fun nativeProbe(cameraId: String, keyName: String): String
    private external fun nativeProbeDecoupled(
        metadataCameraId: String,
        requestCameraId: String,
        keyName: String,
    ): String
}
