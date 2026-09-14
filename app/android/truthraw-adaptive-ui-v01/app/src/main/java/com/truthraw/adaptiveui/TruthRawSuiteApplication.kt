package com.truthraw.adaptiveui

import android.app.Activity
import android.app.Application
import android.os.Bundle
import android.view.ViewGroup
import android.view.WindowInsets

/**
 * Suite-level Android plumbing only.
 *
 * v0.4.3 deliberately performs no CameraManager access from Application.onCreate().
 * Camera2 is entered only from the explicit diagnostic steps. This keeps the
 * bootstrap layer genuinely camera-free and makes crash localization truthful.
 */
class TruthRawSuiteApplication : Application() {
    override fun onCreate() {
        super.onCreate()
        registerActivityLifecycleCallbacks(object : ActivityLifecycleCallbacks {
            override fun onActivityStarted(activity: Activity) {
                val fotoGraaf = activity is FotoGraafCameraActivity ||
                    activity is FotoGraafLiveCameraActivity ||
                    activity is FotoGraafSafePreviewActivity ||
                    activity is FotoGraafDiagnosticBootstrapActivity ||
                    activity is FotoGraafPermissionGateActivity
                if (!fotoGraaf) return

                activity.window.setDecorFitsSystemWindows(false)
                val content = activity.findViewById<ViewGroup>(android.R.id.content) ?: return
                content.setOnApplyWindowInsetsListener { view, insets ->
                    val bars = insets.getInsets(
                        WindowInsets.Type.systemBars() or WindowInsets.Type.displayCutout(),
                    )
                    view.setPadding(bars.left, bars.top, bars.right, bars.bottom)
                    insets
                }
                content.requestApplyInsets()
            }

            override fun onActivityCreated(activity: Activity, savedInstanceState: Bundle?) = Unit
            override fun onActivityResumed(activity: Activity) = Unit
            override fun onActivityPaused(activity: Activity) = Unit
            override fun onActivityStopped(activity: Activity) = Unit
            override fun onActivitySaveInstanceState(activity: Activity, outState: Bundle) = Unit
            override fun onActivityDestroyed(activity: Activity) = Unit
        })
    }
}
