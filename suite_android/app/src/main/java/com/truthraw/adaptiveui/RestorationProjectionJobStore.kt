package com.truthraw.adaptiveui

import android.content.Context
import android.net.Uri
import java.io.File

enum class RestorationProjectionJobPhase(val terminal: Boolean) {
    STARTING(false),
    STAGING(false),
    COMMITTING(false),
    VERIFYING(false),
    SUCCESS(true),
    FAILED(true),
    STALE_CLEANED(true),
}

data class RestorationProjectionJobSnapshot(
    val sourceUri: String,
    val trrUri: String,
    val destinationUri: String,
    val stagingPath: String,
    val format: RestorationProjectionFormat,
    val phase: RestorationProjectionJobPhase,
    val message: String,
    val startedAtMs: Long,
    val updatedAtMs: Long,
)

object RestorationProjectionJobStore {
    private const val PREFS = "truthraw_restoration_projection_v072"
    private const val STARTUP_GRACE_MS = 30_000L

    fun begin(
        context: Context,
        sourceUri: Uri,
        trrUri: Uri,
        destinationUri: Uri,
        staging: File,
        format: RestorationProjectionFormat,
    ) {
        val now = System.currentTimeMillis()
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit()
            .putString("source", sourceUri.toString())
            .putString("trr", trrUri.toString())
            .putString("destination", destinationUri.toString())
            .putString("staging", staging.absolutePath)
            .putString("format", format.name)
            .putString("phase", RestorationProjectionJobPhase.STARTING.name)
            .putString("message", "${format.label}-projectie wordt gestart…")
            .putLong("started", now)
            .putLong("updated", now)
            .commit()
    }

    fun update(
        context: Context,
        phase: RestorationProjectionJobPhase,
        message: String,
    ) {
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit()
            .putString("phase", phase.name)
            .putString("message", message)
            .putLong("updated", System.currentTimeMillis())
            .apply()
    }

    fun read(context: Context): RestorationProjectionJobSnapshot? {
        val p = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
        val source = p.getString("source", null) ?: return null
        val trr = p.getString("trr", null) ?: return null
        val destination = p.getString("destination", null) ?: return null
        val staging = p.getString("staging", null) ?: return null
        val format = runCatching {
            RestorationProjectionFormat.valueOf(p.getString("format", "") ?: "")
        }.getOrNull() ?: return null
        val phase = runCatching {
            RestorationProjectionJobPhase.valueOf(p.getString("phase", "") ?: "")
        }.getOrNull() ?: return null
        val updated = p.getLong("updated", 0L)
        val started = p.getLong("started", updated)
        return RestorationProjectionJobSnapshot(
            sourceUri = source,
            trrUri = trr,
            destinationUri = destination,
            stagingPath = staging,
            format = format,
            phase = phase,
            message = p.getString("message", "") ?: "",
            startedAtMs = started,
            updatedAtMs = updated,
        )
    }

    fun recoverInterruptedIfNeeded(context: Context): RestorationProjectionJobSnapshot? {
        val snapshot = read(context) ?: return null
        if (snapshot.phase.terminal || RestorationProjectionForegroundService.isRunning) {
            return snapshot
        }

        // ACTION_CREATE_DOCUMENT returns to MainActivity very close to the moment
        // startForegroundService() is issued. Service.onCreate() may not have run yet.
        // Never classify that normal startup window as an interrupted export.
        val age = System.currentTimeMillis() - snapshot.updatedAtMs
        if (age in 0 until STARTUP_GRACE_MS) {
            return snapshot
        }

        runCatching { File(snapshot.stagingPath).delete() }
        runCatching {
            RestorationProjectionExporter.cleanup(
                context.contentResolver,
                Uri.parse(snapshot.destinationUri),
            )
        }
        update(
            context,
            RestorationProjectionJobPhase.STALE_CLEANED,
            "Onderbroken ${snapshot.format.label}-projectie veilig opgeruimd na startup-grace; " +
                "geen gedeeltelijk doelbestand blijft geldig.",
        )
        return read(context)
    }

    fun clear(context: Context) {
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit().clear().apply()
    }
}
