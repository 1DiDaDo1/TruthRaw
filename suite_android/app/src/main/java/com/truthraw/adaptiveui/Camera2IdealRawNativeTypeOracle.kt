package com.truthraw.adaptiveui

import org.json.JSONObject

/**
 * v0.23 native metadata-type oracle for the single vendor key
 * `org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW`.
 *
 * The native side opens logical camera 0 only long enough to create disposable NDK
 * capture-request metadata objects. It resolves the vendor tag ID and asks the NDK metadata
 * validator whether the tag accepts BYTE or INT32. No capture session is created and no
 * vendor-modified request is submitted to HAL.
 *
 * This can establish native metadata value type only. It does not establish vendor semantics.
 */
object Camera2IdealRawNativeTypeOracle {
    const val KEY_NAME = "org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW"

    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    fun probe(cameraId: String): JSONObject =
        runCatching { JSONObject(nativeProbe(cameraId, KEY_NAME)) }
            .getOrElse { e ->
                JSONObject()
                    .put("schema", "truthraw.camera2-idealraw-native-type-oracle.v0.1")
                    .put("classification", "NATIVE_TYPE_ORACLE_BRIDGE_ERROR")
                    .put("error", "${e.javaClass.simpleName}: ${e.message}")
                    .put("sessionCreated", false)
                    .put("captureSubmitted", false)
                    .put("vendorModifiedRequestSubmittedToHal", false)
                    .put("semanticPromotionAllowed", false)
            }

    private external fun nativeProbe(cameraId: String, keyName: String): String
}
