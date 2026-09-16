package com.truthraw.adaptiveui

import android.app.Activity
import android.app.Application
import android.os.Bundle
import android.view.ViewGroup
import android.view.WindowInsets
import java.io.PrintWriter
import java.io.StringWriter
import java.time.Instant

/** Suite-level Android plumbing and fail-safe crash provenance. */
class TruthRawSuiteApplication : Application() {
    override fun onCreate() {
        super.onCreate()

        val previous = Thread.getDefaultUncaughtExceptionHandler()
        Thread.setDefaultUncaughtExceptionHandler { thread, throwable ->
            runCatching {
                val sw = StringWriter()
                throwable.printStackTrace(PrintWriter(sw))
                val report = buildString {
                    appendLine("TruthRaw Android crash report v0.8")
                    appendLine("createdAtUtc=${Instant.now()}")
                    appendLine("thread=${thread.name}")
                    appendLine("exception=${throwable.javaClass.name}")
                    appendLine("message=${throwable.message ?: ""}")
                    appendLine("--- stacktrace ---")
                    append(sw.toString())
                }
                openFileOutput(CRASH_FILE, MODE_PRIVATE).bufferedWriter().use { it.write(report) }
            }
            previous?.uncaughtException(thread, throwable)
        }

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

    companion object {
        const val CRASH_FILE = "truthraw_last_crash.txt"
    }
}
