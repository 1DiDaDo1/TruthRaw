package com.truthraw.adaptiveui

import android.app.Activity
import android.app.Application
import android.os.Bundle
import android.view.ViewGroup
import android.view.WindowInsets

/**
 * Suite-level Android plumbing only.
 *
 * targetSdk 35 is edge-to-edge by default. FotoGraaf activities are research
 * instruments and must never hide controls/status behind the status/navigation
 * bars. MainActivity and the Suite launcher already manage their own insets,
 * so this lifecycle hook intentionally scopes itself to the two FotoGraaf
 * activities that do not.
 */
class TruthRawSuiteApplication : Application() {
    override fun onCreate() {
        super.onCreate()
        registerActivityLifecycleCallbacks(object : ActivityLifecycleCallbacks {
            override fun onActivityStarted(activity: Activity) {
                if (activity !is FotoGraafCameraActivity && activity !is FotoGraafPermissionGateActivity) return
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
