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
import java.util.concurrent.atomic.AtomicBoolean

class TruthRawMediaProcessingForegroundService : Service() {
    private var wakeLock: PowerManager.WakeLock? = null
    private val labels = ConcurrentHashMap<String, String>()
    private val researchWorkerKeys = ConcurrentHashMap.newKeySet<String>()
    private var oldestStartedAtWallMs: Long = 0L

    override fun onCreate() {
        super.onCreate()
        instance = this
        isRunning = true
        ensureChannel()
    }

    override fun onDestroy() {
        releaseWakeLock()
        if (instance === this) instance = null
        activeKeys.clear()
        isRunning = false
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
        activeKeys.add(key)
        val started = snapshot?.startedAtWallMs ?: System.currentTimeMillis()
        oldestStartedAtWallMs =
            if (oldestStartedAtWallMs == 0L) started else minOf(oldestStartedAtWallMs, started)

        val startResearchWorker =
            intent.action ==
                ACTION_RESEARCH_UNIVERSAL_BATCH &&
                researchWorkerKeys.add(key)

        acquireWakeLock()
        startForeground(
            NOTIFICATION_ID,
            notification(currentLabel(), ongoing = true),
            ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PROCESSING,
        )

        if (startResearchWorker) {
            runResearchUniversalBatch(
                operationKey = key,
                redelivered =
                    flags and START_FLAG_REDELIVERY != 0,
            )
            return START_REDELIVER_INTENT
        }

        return if (
            intent.action ==
            ACTION_RESEARCH_UNIVERSAL_BATCH
        ) {
            START_REDELIVER_INTENT
        } else {
            START_NOT_STICKY
        }
    }

    override fun onTimeout(startId: Int, fgsType: Int) {
        val message =
            "Android media-processing achtergrondlimiet bereikt; verwerking is veilig gestopt."
        labels.keys.toList().forEach { key ->
            TruthRawOperationStore.update(this, key, TruthRawOperationPhase.ERROR, message)
            activeKeys.remove(key)
        }
        labels.clear()
        finishNotification("TruthRaw verwerking gestopt", message)
        releaseWakeLock()
        stopSelf(startId)
    }

    private fun runResearchUniversalBatch(
        operationKey: String,
        redelivered: Boolean,
    ) {
        Thread({
            var heartbeatStop: AtomicBoolean? = null
            var heartbeatThread: Thread? = null
            try {
                val session =
                    ResearchWorkbenchSessionStoreV01.load(
                        filesDir,
                    )
                        ?: kotlin.error(
                            "Geen persistente Research-selectie gevonden.",
                        )
                if (session.jobs.isEmpty()) {
                    kotlin.error(
                        "Persistente Research-selectie bevat geen RAW-bronnen.",
                    )
                }

                ResearchBatchJournalV02.begin(
                    context = applicationContext,
                    operationKey = operationKey,
                    jobs = session.jobs,
                    redelivered = redelivered,
                )

                val stopHeartbeat = AtomicBoolean(false)
                heartbeatStop = stopHeartbeat
                heartbeatThread =
                    Thread(
                        {
                            while (!stopHeartbeat.get()) {
                                ResearchBatchJournalV02.heartbeat(
                                    applicationContext,
                                    operationKey,
                                )
                                try {
                                    Thread.sleep(
                                        RESEARCH_HEARTBEAT_INTERVAL_MS,
                                    )
                                } catch (_: InterruptedException) {
                                    break
                                }
                            }
                        },
                        "draw-research-heartbeat",
                    ).apply {
                        isDaemon = true
                        start()
                    }

                var measuredCharts = 0
                var failures = 0

                for ((index, job) in session.jobs.withIndex()) {
                    val existing =
                        ResearchUniversalProfileStoreV01.loadForJob(
                            filesDir = filesDir,
                            job = job,
                        )

                    val progress =
                        "Universele bronanalyse " +
                            (index + 1) +
                            "/" +
                            session.jobs.size +
                            " · " +
                            job.source.displayName

                    val progressMessage =
                        if (existing != null) {
                            progress +
                                " · persistent profiel hergebruikt"
                        } else {
                            progress
                        }
                    ResearchBatchJournalV02.stage(
                        context = applicationContext,
                        operationKey = operationKey,
                        job = job,
                        index = index,
                        total = session.jobs.size,
                        stage =
                            if (existing != null) {
                                "PROFILE_REUSED"
                            } else {
                                "PROFILE_BEGIN"
                            },
                    )
                    labels[operationKey] =
                        progressMessage
                    TruthRawOperationStore.update(
                        applicationContext,
                        operationKey,
                        TruthRawOperationPhase.RUNNING,
                        progressMessage,
                    )
                    notifyProgress()

                    val profile =
                        existing
                            ?: runCatching {
                                try {
                                    UniversalSourceProfiler.profile(
                                        resolver = contentResolver,
                                        source = job.source,
                                        cacheDir = cacheDir,
                                        progress = { stage ->
                                            val stageMessage =
                                                progress +
                                                    " · stage=" +
                                                    stage
                                            labels[operationKey] =
                                                stageMessage
                                            TruthRawOperationStore.update(
                                                applicationContext,
                                                operationKey,
                                                TruthRawOperationPhase.RUNNING,
                                                stageMessage,
                                            )
                                            ResearchBatchJournalV02.stage(
                                                context = applicationContext,
                                                operationKey = operationKey,
                                                job = job,
                                                index = index,
                                                total = session.jobs.size,
                                                stage = stage,
                                            )
                                            notifyProgress()
                                        },
                                        derivedStageCacheDir = filesDir,
                                    )
                                } finally {
                                    // The shared native preparation cache is
                                    // intentionally one-source and one-profile
                                    // scoped. Release it before the service
                                    // advances to the next RAW so no large
                                    // Scientific-Master context survives the
                                    // profile boundary.
                                    runCatching {
                                        TruthNegativeN2FactoredConfidenceBridge
                                            .clearSharedPipelineCache()
                                    }
                                }
                            }.onSuccess {
                                ResearchUniversalProfileStoreV01.save(
                                    filesDir = filesDir,
                                    job = job,
                                    profile = it,
                                )
                                ResearchBatchJournalV02.completed(
                                    context = applicationContext,
                                    operationKey = operationKey,
                                    job = job,
                                    sourceSha256 =
                                        it.optString(
                                            "source_sha256",
                                        ),
                                )
                            }.onFailure { error ->
                                failures++
                                val failureMessage =
                                    progress +
                                        " · fout=" +
                                        (
                                            error.message
                                                ?: error.javaClass.simpleName
                                            )
                                labels[operationKey] =
                                    failureMessage
                                ResearchBatchJournalV02.failed(
                                    context = applicationContext,
                                    operationKey = operationKey,
                                    job = job,
                                    message = failureMessage,
                                )
                                TruthRawOperationStore.update(
                                    applicationContext,
                                    operationKey,
                                    TruthRawOperationPhase.RUNNING,
                                    failureMessage,
                                )
                                notifyProgress()
                            }.getOrNull()

                    if (existing != null) {
                        ResearchBatchJournalV02.completed(
                            context = applicationContext,
                            operationKey = operationKey,
                            job = job,
                            sourceSha256 =
                                existing.optString(
                                    "source_sha256",
                                ),
                        )
                    }

                    if (
                        profile
                            ?.optJSONObject(
                                "observation_optical_field_chart",
                            )
                            ?.takeIf {
                                it.optString("status") ==
                                    "FIELD_CHART_AVAILABLE" &&
                                    it.optJSONObject(
                                        "measured_composite_field_signal",
                                    )
                                        ?.optString("status") ==
                                    "MEASURED_COMPOSITE_FIELD_SIGNAL_AVAILABLE"
                            } != null
                    ) {
                        measuredCharts++
                    }

                    // The research batch intentionally does not retain full
                    // profiles in service memory. Each completed derived
                    // profile is committed to private storage before moving
                    // to the next RAW. This bounds peak heap independently of
                    // the number of selected observations.
                    if (profile != null && existing == null) {
                        System.gc()
                    }
                }

                val message =
                    if (failures == 0) {
                        "Field Response bronanalyse gereed · measured-field-chart=" +
                            measuredCharts +
                            " · minimum=3" +
                            if (measuredCharts >= 3) {
                                " · repeatability-export beschikbaar."
                            } else {
                                " · nog onvoldoende measured field charts."
                            }
                    } else {
                        "Field Response bronanalyse gereed met " +
                            failures +
                            " fout(en) · measured-field-chart=" +
                            measuredCharts +
                            " · minimum=3."
                    }

                ResearchBatchJournalV02.finish(
                    context = applicationContext,
                    operationKey = operationKey,
                    success = failures == 0,
                    message = message,
                )
                if (failures == 0) {
                    success(
                        applicationContext,
                        operationKey,
                        message,
                    )
                } else {
                    error(
                        applicationContext,
                        operationKey,
                        message,
                    )
                }
            } catch (error: Throwable) {
                val failure =
                    "Research bronanalyse faalde: " +
                        (
                            error.message
                                ?: error.javaClass.simpleName
                            )
                ResearchBatchJournalV02.finish(
                    context = applicationContext,
                    operationKey = operationKey,
                    success = false,
                    message = failure,
                )
                error(
                    applicationContext,
                    operationKey,
                    failure,
                )
            } finally {
                heartbeatStop?.set(true)
                heartbeatThread?.interrupt()
                researchWorkerKeys.remove(
                    operationKey,
                )
            }
        }, "draw-research-profile-service").start()
    }

    private fun notifyProgress() {
        getSystemService(
            NotificationManager::class.java,
        )?.notify(
            NOTIFICATION_ID,
            notification(
                currentLabel(),
                ongoing = true,
            ),
        )
    }

    private fun completeKey(key: String) {
        researchWorkerKeys.remove(key)
        labels.remove(key)
        activeKeys.remove(key)
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
                flags =
                    Intent.FLAG_ACTIVITY_SINGLE_TOP or
                        Intent.FLAG_ACTIVITY_CLEAR_TOP
                if (
                    researchWorkerKeys.isNotEmpty()
                ) {
                    putExtra(
                        MainActivity.EXTRA_OPEN_RESEARCH_WORKBENCH,
                        true,
                    )
                }
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
        private const val ACTION_RESEARCH_UNIVERSAL_BATCH =
            "com.truthraw.adaptiveui.action.RESEARCH_UNIVERSAL_BATCH"
        private const val WAKELOCK_TIMEOUT_MS = 6L * 60L * 60L * 1000L
        private const val RESEARCH_HEARTBEAT_INTERVAL_MS = 5_000L

        @Volatile
        private var instance: TruthRawMediaProcessingForegroundService? = null

        @Volatile
        var isRunning: Boolean = false
            private set

        private val activeKeys = ConcurrentHashMap.newKeySet<String>()

        fun isActive(key: String): Boolean = activeKeys.contains(key)

        fun start(context: Context, key: String, label: String): Boolean {
            val app = context.applicationContext
            if (!activeKeys.add(key)) return false
            TruthRawOperationStore.begin(app, key, label)
            val intent = Intent(app, TruthRawMediaProcessingForegroundService::class.java)
                .putExtra(EXTRA_KEY, key)
                .putExtra(EXTRA_LABEL, label)
            return try {
                app.startForegroundService(intent)
                true
            } catch (error: Throwable) {
                activeKeys.remove(key)
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

        fun startResearchBatch(
            context: Context,
            key: String,
            label: String,
        ): Boolean {
            val app = context.applicationContext
            if (!activeKeys.add(key)) return false
            TruthRawOperationStore.begin(
                app,
                key,
                label,
            )
            val intent =
                Intent(
                    app,
                    TruthRawMediaProcessingForegroundService::class.java,
                )
                    .setAction(
                        ACTION_RESEARCH_UNIVERSAL_BATCH,
                    )
                    .putExtra(
                        EXTRA_KEY,
                        key,
                    )
                    .putExtra(
                        EXTRA_LABEL,
                        label,
                    )
            return try {
                app.startForegroundService(intent)
                true
            } catch (error: Throwable) {
                activeKeys.remove(key)
                TruthRawOperationStore.update(
                    app,
                    key,
                    TruthRawOperationPhase.ERROR,
                    "Research achtergrondverwerking kon niet starten: " +
                        (
                            error.message
                                ?: error.javaClass.simpleName
                            ),
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
