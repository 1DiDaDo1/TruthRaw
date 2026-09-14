package com.truthraw.adaptiveui

import android.hardware.camera2.CameraCaptureSession
import android.hardware.camera2.CaptureRequest
import android.os.Handler
import android.os.Looper
import java.util.concurrent.Executor

/**
 * Compatibility shim for the dependency-light FotoGraaf prototype.
 * The SDK surface used by this project exposes the Handler overload for
 * setRepeatingRequest. Keep UI-facing preview callbacks on the main looper.
 */
internal fun CameraCaptureSession.setRepeatingRequest(
    request: CaptureRequest,
    executor: Executor,
    callback: CameraCaptureSession.CaptureCallback,
): Int {
    // executor is part of the call-site contract; the current safe-preview
    // bootstrap intentionally serializes UI telemetry on the main looper.
    @Suppress("UNUSED_VARIABLE")
    val requestedExecutor = executor
    return setRepeatingRequest(request, callback, Handler(Looper.getMainLooper()))
}
