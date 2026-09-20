package com.truthraw.adaptiveui

import android.content.Context
import android.net.Uri
import java.io.File

enum class FullResRestorationJobPhase(val terminal: Boolean) {
    STAGING(false),
    STAGING_VERIFIED(false),
    COMMITTING(false),
    VERIFYING(false),
    SUCCESS(true),
    FAILED(true),
    STALE_CLEANED(true),
}

data class FullResRestorationJobSnapshot(
    val jobId: String,
    val sourceUri: String,
    val destinationUri: String,
    val stagingPath: String,
    val phase: FullResRestorationJobPhase,
    val message: String,
    val startedAtMs: Long,
    val updatedAtMs: Long,
    val outputBytes: Long,
    val containerSha256: String?,
)

object FullResRestorationJobStore {
    private const val PREFS = "truthraw_fullres_restoration_transaction_v068"
    private const val KEY_JOB_ID = "job_id"
    private const val KEY_SOURCE_URI = "source_uri"
    private const val KEY_DESTINATION_URI = "destination_uri"
    private const val KEY_STAGING_PATH = "staging_path"
    private const val KEY_PHASE = "phase"
    private const val KEY_MESSAGE = "message"
    private const val KEY_STARTED_AT = "started_at"
    private const val KEY_UPDATED_AT = "updated_at"
    private const val KEY_OUTPUT_BYTES = "output_bytes"
    private const val KEY_SHA256 = "sha256"

    fun begin(
        context: Context,
        jobId: String,
        sourceUri: Uri,
        destinationUri: Uri,
        stagingFile: File,
    ) {
        val now = System.currentTimeMillis()
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit()
            .putString(KEY_JOB_ID, jobId)
            .putString(KEY_SOURCE_URI, sourceUri.toString())
            .putString(KEY_DESTINATION_URI, destinationUri.toString())
            .putString(KEY_STAGING_PATH, stagingFile.absolutePath)
            .putString(KEY_PHASE, FullResRestorationJobPhase.STAGING.name)
            .putString(
                KEY_MESSAGE,
                "Full-resolution Restoration draait als foreground job · private staging · Scientific Master replay.",
            )
            .putLong(KEY_STARTED_AT, now)
            .putLong(KEY_UPDATED_AT, now)
            .putLong(KEY_OUTPUT_BYTES, 0L)
            .remove(KEY_SHA256)
            .apply()
    }

    fun update(
        context: Context,
        phase: FullResRestorationJobPhase,
        message: String,
        outputBytes: Long? = null,
        containerSha256: String? = null,
    ) {
        val editor = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit()
            .putString(KEY_PHASE, phase.name)
            .putString(KEY_MESSAGE, message)
            .putLong(KEY_UPDATED_AT, System.currentTimeMillis())
        if (outputBytes != null) editor.putLong(KEY_OUTPUT_BYTES, outputBytes)
        if (containerSha256 != null) editor.putString(KEY_SHA256, containerSha256)
        editor.apply()
    }

    fun read(context: Context): FullResRestorationJobSnapshot? {
        val p = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
        val jobId = p.getString(KEY_JOB_ID, null) ?: return null
        val sourceUri = p.getString(KEY_SOURCE_URI, null) ?: return null
        val destinationUri = p.getString(KEY_DESTINATION_URI, null) ?: return null
        val stagingPath = p.getString(KEY_STAGING_PATH, null) ?: return null
        val phaseName = p.getString(KEY_PHASE, null) ?: return null
        val phase = runCatching { FullResRestorationJobPhase.valueOf(phaseName) }.getOrNull()
            ?: return null
        return FullResRestorationJobSnapshot(
            jobId = jobId,
            sourceUri = sourceUri,
            destinationUri = destinationUri,
            stagingPath = stagingPath,
            phase = phase,
            message = p.getString(KEY_MESSAGE, "") ?: "",
            startedAtMs = p.getLong(KEY_STARTED_AT, 0L),
            updatedAtMs = p.getLong(KEY_UPDATED_AT, 0L),
            outputBytes = p.getLong(KEY_OUTPUT_BYTES, 0L),
            containerSha256 = p.getString(KEY_SHA256, null),
        )
    }

    /**
     * Called when the UI returns after a process interruption. A live foreground
     * service owns its transaction and must never be cleaned. If no service is
     * alive and the persisted phase is non-terminal, the prior transaction
     * cannot resume safely: delete the private stage and delete/truncate the SAF
     * placeholder so no partial artifact remains visible as valid.
     */
    fun recoverInterruptedIfNeeded(context: Context): FullResRestorationJobSnapshot? {
        val snapshot = read(context) ?: return null
        if (snapshot.phase.terminal || FullResRestorationForegroundService.isRunning) {
            return snapshot
        }

        runCatching { File(snapshot.stagingPath).delete() }
        runCatching {
            FullResRestorationExporter.cleanupDestination(
                context.contentResolver,
                Uri.parse(snapshot.destinationUri),
            )
        }
        val message =
            "Vorige Full-resolution Restoration-job werd onderbroken. Private staging is verwijderd; " +
                "de onvolledige doeluitvoer is verwijderd of tot een ongeldig leeg document teruggebracht."
        update(
            context,
            FullResRestorationJobPhase.STALE_CLEANED,
            message,
            outputBytes = 0L,
        )
        return read(context)
    }
}
