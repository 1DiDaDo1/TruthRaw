package com.truthraw.adaptiveui

import android.content.Context
import android.net.Uri
import java.io.File

enum class RestorationProjectionJobPhase(val terminal: Boolean) {
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
    val updatedAtMs: Long,
)

object RestorationProjectionJobStore {
    private const val PREFS = "truthraw_restoration_projection_v071"

    fun begin(
        context: Context,
        sourceUri: Uri,
        trrUri: Uri,
        destinationUri: Uri,
        staging: File,
        format: RestorationProjectionFormat,
    ) {
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit()
            .putString("source", sourceUri.toString())
            .putString("trr", trrUri.toString())
            .putString("destination", destinationUri.toString())
            .putString("staging", staging.absolutePath)
            .putString("format", format.name)
            .putString("phase", RestorationProjectionJobPhase.STAGING.name)
            .putString("message", "${format.label}-projectie foreground staging gestart.")
            .putLong("updated", System.currentTimeMillis())
            .apply()
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
        return RestorationProjectionJobSnapshot(
            sourceUri = source,
            trrUri = trr,
            destinationUri = destination,
            stagingPath = staging,
            format = format,
            phase = phase,
            message = p.getString("message", "") ?: "",
            updatedAtMs = p.getLong("updated", 0L),
        )
    }

    fun recoverInterruptedIfNeeded(context: Context): RestorationProjectionJobSnapshot? {
        val snapshot = read(context) ?: return null
        if (snapshot.phase.terminal || RestorationProjectionForegroundService.isRunning) return snapshot
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
            "Onderbroken ${snapshot.format.label}-projectie opgeruimd; geen gedeeltelijk doelbestand blijft geldig.",
        )
        return read(context)
    }
}
