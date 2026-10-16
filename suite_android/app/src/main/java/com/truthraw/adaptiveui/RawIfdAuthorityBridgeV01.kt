package com.truthraw.adaptiveui

/**
 * Identity-only runtime handoff from the already-validated Source Capability
 * contract into the native DNG open boundary. This does not validate or apply
 * any raw-to-linear sample operation.
 */
object RawIfdAuthorityBridgeV01 {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun registerValidatedRawIfdAuthority(
        sourceSha256: String,
        rawIfdOffset: Long,
    ): Boolean
}
