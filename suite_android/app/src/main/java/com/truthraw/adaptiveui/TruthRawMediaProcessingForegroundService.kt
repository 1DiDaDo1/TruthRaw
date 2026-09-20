package com.truthraw.adaptiveui

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.IBinder
import android.os.PowerManager
import java.util.concurrent.ConcurrentHashMap

class TruthRawMediaProcessingForegroundService : Service() {
    private var wakeLock: PowerManager.WakeLock? = null
    private val labels = ConcurrentHashMap<String, String>()
    private var oldestStartedAtWallMs: Long = 0L

    override fun onCreate() {
        super.onCreate()
        instance = this
        ensureChannel()
    }

    override fun onDestroy() {
        releaseWakeLock()
        if (instance === this) instance = null
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val key = intent?.getStringExtra(EXTRA_KEY)
        val label = intent?.getStringExtra(EXTRA_LABEL)
        if (key.isNullOrBlank() || label.isNullOrBlank()) {
            stopSelf(startId)
            return START_NOT_STICKY
        }

        val snapshot = TruthRawOperationStore.read(this, key)
        if (snapshot?.terminal == true) {
            if (labels.isEmpty()) stopSelf(startId)
            return START_NOT_STICKY
        }
        labels[key] = label
        val started = snapshot?.startedAtWallMs ?: System.currentTimeMillis()
        oldestStartedAtWallMs =
            if (oldestStartedAtWallMs == 0L) started else minOf(oldestStartedAtWallMs, started)

        acquireWakeLock()
        startForeground(
            NOTIFICATION_ID,
            notification(currentLabel(), ongoing = true),
            ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PROCESSING,
        )
        return START_NOT_STICKY
    }

    override fun onTimeout(startId: Int, fgsType: Int) {
        val message =
            "Android media-processing achtergrondlimiet bereikt; verwerking is veilig gestopt."
        labels.keys.toList().forEach { key ->
            TruthRawOperationStore.update(this, key, TruthRawOperationPhase.ERROR, message)
        }
        labels.clear()
        finishNotification("TruthRaw verwerking gestopt", message)
        releaseWakeLock()
        stopSelf(startId)
    }

    private fun completeKey(key: String) {
        labels.remove(key)
        if (labels.isEmpty()) {
            finishNotification(
                "TruthRaw verwerking gereed",
                "Alle achtergrondbewerkingen zijn afgerond.",
            )
            releaseWakeLock()
            stopSelf()
        } else {
            getSystemService(NotificationManager::class.java)?.notify(
                NOTIFICATION_ID,
                notification(currentLabel(), ongoing = true),
            )
        }
    }

    private fun currentLabel(): String =
        labels.values.lastOrNull() ?: "TruthRaw verwerkt foto…"

    private fun acquireWakeLock() {
        if (wakeLock?.isHeld == true) return
        wakeLock = getSystemService(PowerManager::class.java)?.newWakeLock(
            PowerManager.PARTIAL_WAKE_LOCK,
            "TruthRaw:MediaProcessing",
        )?.apply {
            setReferenceCounted(false)
            acquire(WAKELOCK_TIMEOUT_MS)
        }
    }

    private fun releaseWakeLock() {
        wakeLock?.let { if (it.isHeld) runCatching { it.release() } }
        wakeLock = null
    }

    private fun ensureChannel() {
        getSystemService(NotificationManager::class.java)?.createNotificationChannel(
            NotificationChannel(
                CHANNEL_ID,
                "TruthRaw fotoverwerking",
                NotificationManager.IMPORTANCE_LOW,
            ).apply {
                description =
                    "Laat renders en exports werkelijk doorwerken wanneer TruthRaw niet op de voorgrond staat."
                setShowBadge(false)
            },
        )
    }

    private fun notification(text: String, ongoing: Boolean): Notification {
        val open = PendingIntent.getActivity(
            this,
            0,
            Intent(this, MainActivity::class.java).apply {
                flags = Intent.FLAG_ACTIVITY_SINGLE_TOP or Intent.FLAG_ACTIVITY_CLEAR_TOP
            },
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE,
        )
        val started =
            if (oldestStartedAtWallMs > 0L) oldestStartedAtWallMs else System.currentTimeMillis()
        return Notification.Builder(this, CHANNEL_ID)
            .setSmallIcon(android.R.drawable.stat_sys_download)
            .setContentTitle("TruthRaw werkt door")
            .setContentText(text)
            .setContentIntent(open)
            .setOngoing(ongoing)
            .setOnlyAlertOnce(true)
            .setCategory(Notification.CATEGORY_PROGRESS)
            .setProgress(0, 0, ongoing)
            .setWhen(started)
            .setUsesChronometer(ongoing)
            .build()
    }

    private fun finishNotification(title: String, text: String) {
        stopForeground(STOP_FOREGROUND_DETACH)
        getSystemService(NotificationManager::class.java)?.notify(
            NOTIFICATION_ID,
            Notification.Builder(this, CHANNEL_ID)
                .setSmallIcon(android.R.drawable.stat_sys_download_done)
                .setContentTitle(title)
                .setContentText(text)
                .setOnlyAlertOnce(true)
                .setOngoing(false)
                .build(),
        )
    }

    companion object {
        private const val CHANNEL_ID = "truthraw_media_processing_v0_1"
        private const val NOTIFICATION_ID = 7001
        private const val EXTRA_KEY = "operation_key"
        private const val EXTRA_LABEL = "operation_label"
        private const val WAKELOCK_TIMEOUT_MS = 6L * 60L * 60L * 1000L

        @Volatile
        private var instance: TruthRawMediaProcessingForegroundService? = null

        fun start(context: Context, key: String, label: String): Boolean {
            val app = context.applicationContext
            TruthRawOperationStore.begin(app, key, label)
            val intent = Intent(app, TruthRawMediaProcessingForegroundService::class.java)
                .putExtra(EXTRA_KEY, key)
                .putExtra(EXTRA_LABEL, label)
            return try {
                app.startForegroundService(intent)
                true
            } catch (error: Throwable) {
                TruthRawOperationStore.update(
                    app,
                    key,
                    TruthRawOperationPhase.ERROR,
                    "Achtergrondverwerking kon niet starten: " +
                        (error.message ?: error.javaClass.simpleName),
                )
                false
            }
        }

        fun success(context: Context, key: String, message: String) {
            val app = context.applicationContext
            TruthRawOperationStore.update(app, key, TruthRawOperationPhase.SUCCESS, message)
            instance?.completeKey(key)
        }

        fun error(context: Context, key: String, message: String) {
            val app = context.applicationContext
            TruthRawOperationStore.update(app, key, TruthRawOperationPhase.ERROR, message)
            instance?.completeKey(key)
        }

        fun cancel(context: Context, key: String, message: String) {
            val app = context.applicationContext
            TruthRawOperationStore.update(app, key, TruthRawOperationPhase.CANCELLED, message)
            instance?.completeKey(key)
        }
    }
}
