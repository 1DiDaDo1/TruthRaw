package com.truthraw.adaptiveui

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.IBinder
import android.os.PowerManager
import java.io.File
import java.io.FileOutputStream
import java.util.concurrent.atomic.AtomicBoolean

class FullResRestorationForegroundService : Service() {
    private val working = AtomicBoolean(false)
    private var wakeLock: PowerManager.WakeLock? = null

    override fun onCreate() {
        super.onCreate()
        isRunning = true
        ensureNotificationChannel()
    }

    override fun onDestroy() {
        wakeLock?.let { lock ->
            if (lock.isHeld) runCatching { lock.release() }
        }
        wakeLock = null
        isRunning = false
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        startForeground(
            NOTIFICATION_ID,
            notification(
                "Full-resolution Restoration",
                "Foreground staging wordt gestart…",
                ongoing = true,
            ),
        )

        if (!working.compareAndSet(false, true)) {
            return START_NOT_STICKY
        }

        val jobId = intent?.getStringExtra(EXTRA_JOB_ID)
        val sourceUri = intent?.getStringExtra(EXTRA_SOURCE_URI)
        val destinationUri = intent?.getStringExtra(EXTRA_DESTINATION_URI)
        val stagingPath = intent?.getStringExtra(EXTRA_STAGING_PATH)
        val formatId = intent?.getStringExtra(EXTRA_FORMAT_ID)
        val nativeReady = intent?.getBooleanExtra(EXTRA_NATIVE_READY, false) ?: false

        if (jobId.isNullOrBlank() ||
            sourceUri.isNullOrBlank() ||
            destinationUri.isNullOrBlank() ||
            stagingPath.isNullOrBlank() ||
            formatId.isNullOrBlank()
        ) {
            finishFailure(
                "Full-resolution Restoration service kreeg een onvolledige transactie-aanvraag.",
                destinationUri?.let(Uri::parse),
                stagingPath?.let(::File),
            )
            return START_NOT_STICKY
        }

        acquireWakeLock()

        Thread({
            runTransaction(
                jobId = jobId,
                source = FullResRestorationSource(
                    uri = Uri.parse(sourceUri),
                    formatId = formatId,
                    nativeProcessingReady = nativeReady,
                ),
                destination = Uri.parse(destinationUri),
                stagingFile = File(stagingPath),
            )
        }, "truthraw-restoration-fgs-${jobId.take(8)}").start()

        return START_NOT_STICKY
    }

    private fun runTransaction(
        jobId: String,
        source: FullResRestorationSource,
        destination: Uri,
        stagingFile: File,
    ) {
        FullResRestorationJobStore.update(
            this,
            FullResRestorationJobPhase.STAGING,
            "Foreground Restoration actief · private staging · 1:1 pixels · Master replay. " +
                "Je mag TruthRaw nu verlaten; de export blijft doorlopen.",
        )
        updateNotification("Private staging + Scientific Master replay…")

        val staged = FullResRestorationExporter.exportToStaging(
            contentResolver,
            source,
            stagingFile,
        )
        if (staged is FullResRestorationExportResult.Failed) {
            finishFailure(staged.reason, destination, stagingFile)
            return
        }

        val stagedMetrics = (staged as FullResRestorationExportResult.Success).metrics
        FullResRestorationJobStore.update(
            this,
            FullResRestorationJobPhase.STAGING_VERIFIED,
            "Private staging volledig geverifieerd · ${formatBytes(stagedMetrics.outputBytes)} · " +
                "whole-file SHA-256 vastgelegd. Doelbestand is nog niet geldig gecommit.",
            outputBytes = stagedMetrics.outputBytes,
            containerSha256 = stagedMetrics.containerSha256,
        )
        updateNotification("Staging verified · veilige commit wordt voorbereid…")

        FullResRestorationJobStore.update(
            this,
            FullResRestorationJobPhase.COMMITTING,
            "Transactional commit · body eerst met nul-header; geldige 8192-byte header wordt pas als laatste geschreven.",
        )
        updateNotification("Transactional commit · header wordt als laatste gecommit…")

        val committed = FullResRestorationExporter.commitStagingToDestination(
            contentResolver,
            stagingFile,
            destination,
            stagedMetrics,
        )
        if (committed is FullResRestorationExportResult.Failed) {
            finishFailure(committed.reason, destination, stagingFile)
            return
        }

        FullResRestorationJobStore.update(
            this,
            FullResRestorationJobPhase.VERIFYING,
            "Exact doelbestand is teruggelezen · contract, grootte en staging↔doel SHA-256 worden gecontroleerd.",
            outputBytes = stagedMetrics.outputBytes,
            containerSha256 = stagedMetrics.containerSha256,
        )
        updateNotification("Post-write verify afgerond…")

        val metrics = (committed as FullResRestorationExportResult.Success).metrics
        stagingFile.delete()

        val message =
            "Full-resolution Restoration v0.67 transactioneel opgeslagen · " +
                "${metrics.width}×${metrics.height} · ${formatBytes(metrics.outputBytes)} · " +
                "preserved/censored/restored/unresolved=" +
                "${metrics.preservedPixels}/${metrics.censoredPixels}/" +
                "${metrics.restoredPixels}/${metrics.unresolvedPixels} · " +
                "Master replay=${metrics.masterReplayVerified} · " +
                "staging SHA=doel SHA · post-write=${metrics.postWriteVerified}."

        FullResRestorationJobStore.update(
            this,
            FullResRestorationJobPhase.SUCCESS,
            message,
            outputBytes = metrics.outputBytes,
            containerSha256 = metrics.containerSha256,
        )
        finishTerminalNotification(
            title = "TruthRaw Restoration gereed",
            text = "Full-resolution artifact volledig geverifieerd.",
        )
        releaseWakeLock()
        working.set(false)
        stopSelf()
    }

    private fun finishFailure(
        reason: String,
        destination: Uri?,
        stagingFile: File?,
    ) {
        stagingFile?.let { runCatching { it.delete() } }
        destination?.let {
            runCatching { FullResRestorationExporter.cleanupDestination(contentResolver, it) }
        }

        FullResRestorationJobStore.update(
            this,
            FullResRestorationJobPhase.FAILED,
            reason + " Onvolledige staging/doeluitvoer is verwijderd of ongeldig gemaakt.",
            outputBytes = 0L,
        )
        finishTerminalNotification(
            title = "TruthRaw Restoration gestopt",
            text = "Geen onvolledig artifact is als geldig gecommit.",
        )
        releaseWakeLock()
        working.set(false)
        stopSelf()
    }

    private fun acquireWakeLock() {
        val manager = getSystemService(PowerManager::class.java)
        wakeLock = manager?.newWakeLock(
            PowerManager.PARTIAL_WAKE_LOCK,
            "TruthRaw:FullResRestoration",
        )?.apply {
            setReferenceCounted(false)
            acquire(WAKELOCK_TIMEOUT_MS)
        }
    }

    private fun releaseWakeLock() {
        wakeLock?.let { lock ->
            if (lock.isHeld) runCatching { lock.release() }
        }
        wakeLock = null
    }

    private fun ensureNotificationChannel() {
        val manager = getSystemService(NotificationManager::class.java) ?: return
        manager.createNotificationChannel(
            NotificationChannel(
                CHANNEL_ID,
                "TruthRaw full-resolution export",
                NotificationManager.IMPORTANCE_LOW,
            ).apply {
                description =
                    "Houdt lange full-resolution Restoration-exports actief wanneer de app op de achtergrond staat."
                setShowBadge(false)
            },
        )
    }

    private fun updateNotification(text: String) {
        val manager = getSystemService(NotificationManager::class.java) ?: return
        manager.notify(
            NOTIFICATION_ID,
            notification("Full-resolution Restoration", text, ongoing = true),
        )
    }

    private fun finishTerminalNotification(title: String, text: String) {
        val manager = getSystemService(NotificationManager::class.java)
        stopForeground(STOP_FOREGROUND_DETACH)
        manager?.notify(
            NOTIFICATION_ID,
            notification(title, text, ongoing = false),
        )
    }

    private fun notification(
        title: String,
        text: String,
        ongoing: Boolean,
    ): Notification {
        val openApp = PendingIntent.getActivity(
            this,
            0,
            Intent(this, TruthRawSuiteLauncherActivity::class.java).apply {
                flags = Intent.FLAG_ACTIVITY_SINGLE_TOP or Intent.FLAG_ACTIVITY_CLEAR_TOP
            },
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE,
        )

        return Notification.Builder(this, CHANNEL_ID)
            .setSmallIcon(android.R.drawable.stat_sys_download)
            .setContentTitle(title)
            .setContentText(text)
            .setContentIntent(openApp)
            .setOngoing(ongoing)
            .setOnlyAlertOnce(true)
            .setCategory(Notification.CATEGORY_PROGRESS)
            .setProgress(if (ongoing) 0 else 0, 0, ongoing)
            .build()
    }

    private fun formatBytes(bytes: Long): String = when {
        bytes >= 1024L * 1024L * 1024L ->
            String.format("%.2f GiB", bytes.toDouble() / (1024.0 * 1024.0 * 1024.0))
        bytes >= 1024L * 1024L ->
            String.format("%.2f MiB", bytes.toDouble() / (1024.0 * 1024.0))
        bytes >= 1024L ->
            String.format("%.2f KiB", bytes.toDouble() / 1024.0)
        else -> "$bytes B"
    }

    companion object {
        private const val CHANNEL_ID = "truthraw_fullres_restoration_v068"
        private const val NOTIFICATION_ID = 6801
        private const val EXTRA_JOB_ID = "job_id"
        private const val EXTRA_SOURCE_URI = "source_uri"
        private const val EXTRA_DESTINATION_URI = "destination_uri"
        private const val EXTRA_STAGING_PATH = "staging_path"
        private const val EXTRA_FORMAT_ID = "format_id"
        private const val EXTRA_NATIVE_READY = "native_ready"
        private const val WAKELOCK_TIMEOUT_MS = 2L * 60L * 60L * 1000L

        @Volatile
        var isRunning: Boolean = false
            private set

        fun start(
            context: Context,
            job: RawJob,
            destination: Uri,
            resultIntentFlags: Int,
        ): Boolean {
            val existing = FullResRestorationJobStore.read(context)
            if (existing != null && !existing.phase.terminal && isRunning) {
                return false
            }

            persistWritePermissionIfAvailable(
                context,
                destination,
                resultIntentFlags,
            )

            val stagingDir = File(context.filesDir, "restoration_staging")
            stagingDir.mkdirs()
            val stagingFile = File(
                stagingDir,
                "${job.id}_truthraw_fullres_restoration_v0_67.trr.part",
            )

            // Destination document already exists after ACTION_CREATE_DOCUMENT.
            // Keep it empty until the verified staging artifact is ready.
            runCatching {
                context.contentResolver.openFileDescriptor(destination, "rw")?.use { pfd ->
                    FileOutputStream(pfd.fileDescriptor).channel.use { channel ->
                        channel.truncate(0L)
                        channel.force(true)
                    }
                }
            }

            FullResRestorationJobStore.begin(
                context,
                job.id,
                job.source.uri,
                destination,
                stagingFile,
            )

            val intent = Intent(context, FullResRestorationForegroundService::class.java)
                .putExtra(EXTRA_JOB_ID, job.id)
                .putExtra(EXTRA_SOURCE_URI, job.source.uri.toString())
                .putExtra(EXTRA_DESTINATION_URI, destination.toString())
                .putExtra(EXTRA_STAGING_PATH, stagingFile.absolutePath)
                .putExtra(EXTRA_FORMAT_ID, job.source.format.id)
                .putExtra(EXTRA_NATIVE_READY, job.source.format.nativeProcessingReady)

            return try {
                context.startForegroundService(intent)
                true
            } catch (error: Throwable) {
                stagingFile.delete()
                FullResRestorationExporter.cleanupDestination(
                    context.contentResolver,
                    destination,
                )
                FullResRestorationJobStore.update(
                    context,
                    FullResRestorationJobPhase.FAILED,
                    "Foreground Restoration kon niet starten: " +
                        (error.message ?: error.javaClass.simpleName),
                    outputBytes = 0L,
                )
                false
            }
        }

        private fun persistWritePermissionIfAvailable(
            context: Context,
            uri: Uri,
            resultIntentFlags: Int,
        ) {
            val hasWrite =
                resultIntentFlags and Intent.FLAG_GRANT_WRITE_URI_PERMISSION != 0
            val hasPersistable =
                resultIntentFlags and Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION != 0
            if (!hasWrite || !hasPersistable) return
            try {
                context.contentResolver.takePersistableUriPermission(
                    uri,
                    Intent.FLAG_GRANT_WRITE_URI_PERMISSION or
                        (resultIntentFlags and Intent.FLAG_GRANT_READ_URI_PERMISSION),
                )
            } catch (_: SecurityException) {
                // Current grant is still sufficient for the live foreground job.
            }
        }
    }
}
