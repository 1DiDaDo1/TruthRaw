package com.truthraw.adaptiveui

import org.json.JSONObject

/**
 * v0.25 native metadata-type oracle for the single vendor key
 * `org.codeaurora.qcamera3.sessionParameters.RawCbSourceType`.
 *
 * The native side opens logical camera 0 only long enough to create disposable NDK
 * capture-request metadata objects. It resolves the vendor tag ID and asks the NDK metadata
 * validator which native metadata element type accepts a one-element test value.
 *
 * No capture session is created, no session parameters are attached and no vendor-modified
 * request is submitted to HAL. This establishes representation/type only, not vendor semantics.
 */
object Camera2RawCbSourceTypeNativeTypeOracle {
    const val KEY_NAME = "org.codeaurora.qcamera3.sessionParameters.RawCbSourceType"

    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    fun probe(cameraId: String): JSONObject =
        runCatching { JSONObject(nativeProbe(cameraId, KEY_NAME)) }
            .getOrElse { e ->
                JSONObject()
                    .put("schema", "truthraw.camera2-rawcb-source-type-native-type-oracle.v0.25")
                    .put("keyName", KEY_NAME)
                    .put("classification", "RAWCB_NATIVE_TYPE_ORACLE_BRIDGE_ERROR")
                    .put("error", "${e.javaClass.simpleName}: ${e.message}")
                    .put("sessionCreated", false)
                    .put("sessionParametersAttached", false)
                    .put("captureSubmitted", false)
                    .put("vendorModifiedRequestSubmittedToHal", false)
                    .put("semanticPromotionAllowed", false)
            }

    private external fun nativeProbe(cameraId: String, keyName: String): String
}
