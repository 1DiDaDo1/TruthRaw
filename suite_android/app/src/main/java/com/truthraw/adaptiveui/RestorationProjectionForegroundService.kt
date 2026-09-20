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
import java.util.concurrent.atomic.AtomicBoolean

class RestorationProjectionForegroundService : Service() {
    private val working = AtomicBoolean(false)
    private var wakeLock: PowerManager.WakeLock? = null

    override fun onCreate() {
        super.onCreate()
        isRunning = true
        val nm = getSystemService(NotificationManager::class.java)
        nm?.createNotificationChannel(
            NotificationChannel(
                CHANNEL_ID,
                "TruthRaw restoration projections",
                NotificationManager.IMPORTANCE_LOW,
            ).apply {
                description = "Full-resolution DNG/TIFF/EXR projection van geverifieerde Restoration."
                setShowBadge(false)
            },
        )
    }

    override fun onDestroy() {
        releaseWakeLock()
        isRunning = false
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        startForeground(NOTIFICATION_ID, notification("Restoration projection", "Voorbereiden…", true))
        if (!working.compareAndSet(false, true)) return START_NOT_STICKY

        val source = intent?.getStringExtra(EXTRA_SOURCE)?.let(Uri::parse)
        val trr = intent?.getStringExtra(EXTRA_TRR)?.let(Uri::parse)
        val destination = intent?.getStringExtra(EXTRA_DESTINATION)?.let(Uri::parse)
        val staging = intent?.getStringExtra(EXTRA_STAGING)?.let(::File)
        val format = intent?.getStringExtra(EXTRA_FORMAT)?.let {
            runCatching { RestorationProjectionFormat.valueOf(it) }.getOrNull()
        }
        if (source == null || trr == null || destination == null || staging == null || format == null) {
            finishFailure("Projection service kreeg een onvolledige opdracht.", destination, staging)
            return START_NOT_STICKY
        }

        acquireWakeLock()
        Thread({
            runProjection(source, trr, destination, staging, format)
        }, "truthraw-projection-${format.name.lowercase()}").start()
        return START_NOT_STICKY
    }

    private fun runProjection(
        source: Uri,
        trr: Uri,
        destination: Uri,
        staging: File,
        format: RestorationProjectionFormat,
    ) {
        RestorationProjectionJobStore.update(
            this,
            RestorationProjectionJobPhase.STAGING,
            "${format.label}: full-resolution staging actief · .trr + Open Scene + role-mask worden geverifieerd. Dit kan enkele minuten duren.",
        )
        updateNotification("${format.label}: full-resolution staging actief…")

        val staged = RestorationProjectionExporter.exportToStaging(
            contentResolver, source, trr, format, staging,
        )
        if (staged is RestorationProjectionResult.Failed) {
            finishFailure(staged.reason, destination, staging)
            return
        }
        val metrics = (staged as RestorationProjectionResult.Success).metrics

        RestorationProjectionJobStore.update(
            this,
            RestorationProjectionJobPhase.COMMITTING,
            "${format.label}: geverifieerde staging commit naar gekozen document…",
        )
        updateNotification("${format.label}: geverifieerde staging commit…")

        val committed = RestorationProjectionExporter.commit(
            contentResolver, staging, destination, metrics,
        )
        if (committed is RestorationProjectionResult.Failed) {
            finishFailure(committed.reason, destination, staging)
            return
        }

        RestorationProjectionJobStore.update(
            this,
            RestorationProjectionJobPhase.VERIFYING,
            "${format.label}: exact doel is teruggelezen en whole-file SHA wordt vergeleken.",
        )

        val m = (committed as RestorationProjectionResult.Success).metrics
        staging.delete()
        val msg =
            "${format.label} v0.72 gereed · ${m.width}×${m.height} · " +
                "role0/1/2=${m.role0Pixels}/${m.role1Pixels}/${m.role2Pixels} · " +
                "derivative=${m.derivativeIdentityVerified} · lineage=${m.lineageVerified} · " +
                "post-write=${m.postWriteVerified}."
        RestorationProjectionJobStore.update(this, RestorationProjectionJobPhase.SUCCESS, msg)
        finishNotification("TruthRaw ${format.label} gereed", "Full-resolution Restoration-projectie geverifieerd.")
        working.set(false)
        releaseWakeLock()
        stopSelf()
    }

    private fun finishFailure(reason: String, destination: Uri?, staging: File?) {
        staging?.let { runCatching { it.delete() } }
        destination?.let { runCatching { RestorationProjectionExporter.cleanup(contentResolver, it) } }
        RestorationProjectionJobStore.update(
            this,
            RestorationProjectionJobPhase.FAILED,
            reason + " Onvolledig projectiebestand is verwijderd of geleegd.",
        )
        finishNotification("TruthRaw projectie gestopt", "Geen gedeeltelijk bestand is als geldig vrijgegeven.")
        working.set(false)
        releaseWakeLock()
        stopSelf()
    }

    private fun acquireWakeLock() {
        wakeLock = getSystemService(PowerManager::class.java)?.newWakeLock(
            PowerManager.PARTIAL_WAKE_LOCK,
            "TruthRaw:RestorationProjection",
        )?.apply {
            setReferenceCounted(false)
            acquire(2L * 60L * 60L * 1000L)
        }
    }

    private fun releaseWakeLock() {
        wakeLock?.let { if (it.isHeld) runCatching { it.release() } }
        wakeLock = null
    }

    private fun updateNotification(text: String) {
        getSystemService(NotificationManager::class.java)?.notify(
            NOTIFICATION_ID,
            notification("Restoration projection", text, true),
        )
    }

    private fun finishNotification(title: String, text: String) {
        stopForeground(STOP_FOREGROUND_DETACH)
        getSystemService(NotificationManager::class.java)?.notify(
            NOTIFICATION_ID,
            notification(title, text, false),
        )
    }

    private fun notification(title: String, text: String, ongoing: Boolean): Notification {
        val open = PendingIntent.getActivity(
            this, 0,
            Intent(this, MainActivity::class.java).apply {
                flags = Intent.FLAG_ACTIVITY_SINGLE_TOP or Intent.FLAG_ACTIVITY_CLEAR_TOP
            },
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE,
        )
        return Notification.Builder(this, CHANNEL_ID)
            .setSmallIcon(android.R.drawable.stat_sys_download)
            .setContentTitle(title)
            .setContentText(text)
            .setContentIntent(open)
            .setOngoing(ongoing)
            .setOnlyAlertOnce(true)
            .setCategory(Notification.CATEGORY_PROGRESS)
            .setProgress(0, 0, ongoing)
            .build()
    }

    companion object {
        private const val CHANNEL_ID = "truthraw_restoration_projection_v072"
        private const val NOTIFICATION_ID = 6901
        private const val EXTRA_SOURCE = "source"
        private const val EXTRA_TRR = "trr"
        private const val EXTRA_DESTINATION = "destination"
        private const val EXTRA_STAGING = "staging"
        private const val EXTRA_FORMAT = "format"

        @Volatile
        var isRunning: Boolean = false
            private set

        fun start(
            context: Context,
            sourceUri: Uri,
            trrUri: Uri,
            destination: Uri,
            format: RestorationProjectionFormat,
            resultIntentFlags: Int,
        ): Boolean {
            val existing = RestorationProjectionJobStore.recoverInterruptedIfNeeded(context)
            if (existing != null && !existing.phase.terminal) return false

            persistWritePermission(context, destination, resultIntentFlags)
            val dir = File(context.filesDir, "projection_staging").apply { mkdirs() }
            val staging = File(dir, "restoration_v072.${format.extension}.part")
            runCatching { staging.delete() }

            RestorationProjectionJobStore.begin(
                context, sourceUri, trrUri, destination, staging, format,
            )
            val serviceIntent = Intent(context, RestorationProjectionForegroundService::class.java)
                .putExtra(EXTRA_SOURCE, sourceUri.toString())
                .putExtra(EXTRA_TRR, trrUri.toString())
                .putExtra(EXTRA_DESTINATION, destination.toString())
                .putExtra(EXTRA_STAGING, staging.absolutePath)
                .putExtra(EXTRA_FORMAT, format.name)
            return try {
                // Close the MainActivity/onResume race before Service.onCreate() runs.
                isRunning = true
                context.startForegroundService(serviceIntent)
                true
            } catch (error: Throwable) {
                isRunning = false
                staging.delete()
                RestorationProjectionExporter.cleanup(context.contentResolver, destination)
                RestorationProjectionJobStore.update(
                    context,
                    RestorationProjectionJobPhase.FAILED,
                    "Projection foreground service kon niet starten: " +
                        (error.message ?: error.javaClass.simpleName),
                )
                false
            }
        }

        private fun persistWritePermission(context: Context, uri: Uri, flags: Int) {
            if (flags and Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION == 0) return
            val wanted = flags and
                (Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            if (wanted == 0) return
            runCatching { context.contentResolver.takePersistableUriPermission(uri, wanted) }
        }
    }
}
