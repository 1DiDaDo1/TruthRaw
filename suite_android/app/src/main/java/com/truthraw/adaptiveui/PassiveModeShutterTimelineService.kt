package com.truthraw.adaptiveui

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.ContentResolver
import android.content.Intent
import android.database.ContentObserver
import android.hardware.camera2.CameraManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.os.IBinder
import android.os.SystemClock
import android.provider.BaseColumns
import android.provider.MediaStore
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.time.Instant
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicInteger

/**
 * v0.43 reproducibility timeline.
 *
 * TruthRaw remains outside the Honor capture pipeline. It records:
 *  - CameraManager availability callbacks;
 *  - explicit user timing markers;
 *  - MediaStore change notifications;
 *  - metadata-only snapshots at 0/25/100/250/1000 ms after every MediaStore change.
 *
 * It never opens a camera, submits a capture, reads media bytes, decodes EXIF,
 * calls Honor Binder methods, or writes vendor controls.
 */
class PassiveModeShutterTimelineService : Service() {
    private lateinit var workerThread: HandlerThread
    private lateinit var worker: Handler
    private lateinit var cameraManager: CameraManager
    private lateinit var mediaObserver: ContentObserver

    private val running = AtomicBoolean(false)
    private val mediaChangeGroupCounter = AtomicInteger(0)

    private var observerStartEpochMs: Long = 0L
    private var observerStartElapsedNs: Long = 0L
    private var runProfile: String = PROFILE_UNSPECIFIED

    private val imageCollectionUri: Uri
        get() = MediaStore.Images.Media.getContentUri(MediaStore.VOLUME_EXTERNAL)

    private val availability = object : CameraManager.AvailabilityCallback() {
        override fun onCameraAvailable(cameraId: String) {
            appendEvent("CAMERA_AVAILABLE", JSONObject().put("cameraId", cameraId))
        }

        override fun onCameraUnavailable(cameraId: String) {
            appendEvent("CAMERA_UNAVAILABLE", JSONObject().put("cameraId", cameraId))
        }

        override fun onPhysicalCameraAvailable(cameraId: String, physicalCameraId: String) {
            appendEvent(
                "PHYSICAL_CAMERA_AVAILABLE",
                JSONObject()
                    .put("logicalCameraId", cameraId)
                    .put("physicalCameraId", physicalCameraId),
            )
        }

        override fun onPhysicalCameraUnavailable(cameraId: String, physicalCameraId: String) {
            appendEvent(
                "PHYSICAL_CAMERA_UNAVAILABLE",
                JSONObject()
                    .put("logicalCameraId", cameraId)
                    .put("physicalCameraId", physicalCameraId),
            )
        }

        override fun onCameraAccessPrioritiesChanged() {
            appendEvent("CAMERA_ACCESS_PRIORITIES_CHANGED", JSONObject())
        }
    }

    override fun onCreate() {
        super.onCreate()
        workerThread = HandlerThread("truthraw-v043-mode-shutter-timeline").also { it.start() }
        worker = Handler(workerThread.looper)
        cameraManager = getSystemService(CameraManager::class.java)
        ensureNotificationChannel()
        startForeground(NOTIFICATION_ID, buildNotification("Wacht op gekozen runprofiel."))
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_START -> worker.post {
                val requestedProfile = intent.getStringExtra(EXTRA_RUN_PROFILE) ?: PROFILE_UNSPECIFIED
                startObservation(requestedProfile)
            }

            ACTION_MARK_MAIN_STABLE -> worker.post {
                appendUserMarker(
                    MARK_MAIN_STABLE,
                    "User reports Honor default/main preview is stable. Human timing marker only.",
                    intent.getStringExtra(EXTRA_MARK_SOURCE) ?: "notification_action",
                )
            }

            ACTION_MARK_MODE_SELECTED -> worker.post {
                val modeLabel = when (runProfile) {
                    PROFILE_PRO -> MARK_PRO_SELECTED
                    PROFILE_HIRES -> MARK_HIRES_SELECTED
                    PROFILE_NORMAL -> MARK_PHOTO_SELECTED
                    else -> MARK_MODE_SELECTED_UNSPECIFIED
                }
                appendUserMarker(
                    modeLabel,
                    "User reports the selected Honor UI mode is visible/stable. The UI label is preserved as an observation only; no camera-pipeline semantics are assumed.",
                    intent.getStringExtra(EXTRA_MARK_SOURCE) ?: "notification_action",
                )
            }

            ACTION_MARK_SHUTTER_PRESSED -> worker.post {
                appendUserMarker(
                    MARK_SHUTTER_PRESSED,
                    "User reports shutter was pressed. Human timing marker; not a hardware shutter timestamp.",
                    intent.getStringExtra(EXTRA_MARK_SOURCE) ?: "notification_action",
                )
            }

            ACTION_MARK_RETURNED -> worker.post {
                appendUserMarker(
                    MARK_RETURNED,
                    "User returned to TruthRaw after manual Honor Camera operation.",
                    intent.getStringExtra(EXTRA_MARK_SOURCE) ?: "activity",
                )
            }

            ACTION_MARK_LAUNCH -> worker.post {
                appendUserMarker(
                    MARK_WILL_OPEN_HONOR,
                    "TruthRaw moved to background only; user opens Honor Camera manually.",
                    intent.getStringExtra(EXTRA_MARK_SOURCE) ?: "activity",
                )
            }

            ACTION_SNAPSHOT_MEDIA -> worker.post {
                appendMediaSnapshot(
                    eventType = "USER_REQUESTED_MEDIA_SNAPSHOT",
                    requestedUri = imageCollectionUri,
                    changeGroupId = null,
                    scheduledDelayMs = null,
                    triggerElapsedNs = null,
                )
            }

            ACTION_STOP -> worker.post {
                appendEvent(
                    "OBSERVER_STOP_REQUESTED",
                    JSONObject()
                        .put("drainDelayMs", STOP_DRAIN_MS)
                        .put("reason", "Allow delayed metadata snapshots already scheduled to finish."),
                )
                worker.postDelayed({ stopSelf() }, STOP_DRAIN_MS)
            }
        }
        return START_STICKY
    }

    override fun onDestroy() {
        runCatching { cameraManager.unregisterAvailabilityCallback(availability) }
        if (::mediaObserver.isInitialized) {
            runCatching { contentResolver.unregisterContentObserver(mediaObserver) }
        }

        appendEvent(
            "OBSERVER_DESTROYED",
            JSONObject()
                .put("runProfile", runProfile)
                .put("cameraOpenedByTruthRaw", false)
                .put("captureSubmittedByTruthRaw", false)
                .put("imagePixelBytesRead", false)
                .put("mediaInputStreamOpened", false)
                .put("exifDecoded", false)
                .put("honorBinderMethodInvoked", false)
                .put("honorCallbackRegistered", false),
        )

        running.set(false)
        workerThread.quitSafely()
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    private fun startObservation(requestedProfile: String) {
        if (!running.compareAndSet(false, true)) {
            appendEvent(
                "START_REQUEST_IGNORED_ALREADY_RUNNING",
                JSONObject()
                    .put("currentRunProfile", runProfile)
                    .put("requestedRunProfile", requestedProfile),
            )
            return
        }

        runProfile = when (requestedProfile) {
            PROFILE_NORMAL -> PROFILE_NORMAL
            PROFILE_PRO -> PROFILE_PRO
            PROFILE_HIRES -> PROFILE_HIRES
            else -> PROFILE_UNSPECIFIED
        }

        observerStartEpochMs = System.currentTimeMillis()
        observerStartElapsedNs = SystemClock.elapsedRealtimeNanos()

        val report = JSONObject()
            .put("schema", "truthraw.passive-mode-shutter-mediastore-timeline.v0.43")
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", "PASSIVE_SYSTEM_VISIBLE_OUTPUT_OBSERVATION_ONLY")
            .put("runProfile", runProfile)
            .put("runProfileIsUserDeclaredExperimentalLabel", true)
            .put("scientificMasterModified", false)
            .put("captureEvidenceGranted", false)
            .put("cameraOpenedByTruthRaw", false)
            .put("captureSubmittedByTruthRaw", false)
            .put("imagePixelBytesRead", false)
            .put("mediaInputStreamOpened", false)
            .put("exifDecoded", false)
            .put("vendorRequestWrittenByTruthRaw", false)
            .put("honorBinderMethodInvoked", false)
            .put("honorCallbackRegistered", false)
            .put("observerStartEpochMs", observerStartEpochMs)
            .put("observerStartElapsedRealtimeNs", observerStartElapsedNs)
            .put("deviceRuntimeSdkInt", Build.VERSION.SDK_INT)
            .put("deviceRuntimeRelease", Build.VERSION.RELEASE)
            .put("truthRawTargetSdk", applicationInfo.targetSdkVersion)
            .put("mediaPermission", permissionSnapshot())
            .put(
                "humanMarkerBoundary",
                JSONObject()
                    .put("markersAreHardwareTimestamps", false)
                    .put("markersUseElapsedRealtimeNs", true)
                    .put("markersCanBeIssuedFromNotification", true)
                    .put("semanticPromotionAllowed", false),
            )
            .put(
                "delayedMetadataScheduleMs",
                JSONArray(DELAYED_SNAPSHOT_MS.toList()),
            )
            .put(
                "metadataProjectionAuthority",
                JSONObject()
                    .put("readsMediaDatabaseRowsOnly", true)
                    .put("opensMediaFile", false)
                    .put("decodesImage", false)
                    .put("decodesExif", false)
                    .put("semanticPromotionAllowed", false),
            )
            .put("events", JSONArray())

        REPORT_FILE.writeText(report.toString(2))

        appendEvent(
            "OBSERVER_STARTED",
            JSONObject()
                .put("runProfile", runProfile)
                .put("sdkInt", Build.VERSION.SDK_INT)
                .put("targetSdk", applicationInfo.targetSdkVersion)
                .put(
                    "observerMode",
                    "ANDROID16_EXPLICIT_USER_MARKERS_PLUS_CAMERA_AVAILABILITY_PLUS_DELAYED_MEDIASTORE_METADATA",
                )
                .put("mediaPermission", permissionSnapshot()),
        )

        getSystemService(NotificationManager::class.java).notify(
            NOTIFICATION_ID,
            buildNotification("Run: $runProfile · gebruik markers terwijl Honor Camera open is."),
        )

        runCatching {
            cameraManager.registerAvailabilityCallback(mainExecutor, availability)
        }.onFailure {
            appendEvent(
                "REGISTER_CAMERA_AVAILABILITY_CALLBACK_FAILED",
                JSONObject()
                    .put("errorClass", it.javaClass.name)
                    .put("errorMessage", it.message ?: JSONObject.NULL),
            )
        }

        mediaObserver = object : ContentObserver(worker) {
            override fun onChange(selfChange: Boolean) {
                handleMediaChange(selfChange, null, 0)
            }

            override fun onChange(selfChange: Boolean, uri: Uri?) {
                handleMediaChange(selfChange, uri, 0)
            }

            override fun onChange(selfChange: Boolean, uri: Uri?, flags: Int) {
                handleMediaChange(selfChange, uri, flags)
            }
        }

        runCatching {
            contentResolver.registerContentObserver(
                imageCollectionUri,
                true,
                mediaObserver,
            )
        }.onSuccess {
            appendEvent(
                "MEDIASTORE_OBSERVER_REGISTERED",
                JSONObject()
                    .put("collectionUri", imageCollectionUri.toString())
                    .put("notifyForDescendants", true),
            )
            appendMediaSnapshot(
                eventType = "MEDIASTORE_BASELINE_SNAPSHOT",
                requestedUri = imageCollectionUri,
                changeGroupId = null,
                scheduledDelayMs = null,
                triggerElapsedNs = null,
            )
        }.onFailure {
            appendEvent(
                "MEDIASTORE_OBSERVER_REGISTRATION_FAILED",
                JSONObject()
                    .put("collectionUri", imageCollectionUri.toString())
                    .put("errorClass", it.javaClass.name)
                    .put("errorMessage", it.message ?: JSONObject.NULL),
            )
        }
    }

    private fun appendUserMarker(label: String, note: String, source: String) {
        appendEvent(
            "USER_MARK",
            JSONObject()
                .put("label", label)
                .put("note", note)
                .put("source", source)
                .put("hardwareTimestamp", false)
                .put("userDeclaredStateOnly", true)
                .put("semanticPromotionAllowed", false),
        )
    }

    private fun handleMediaChange(selfChange: Boolean, uri: Uri?, flags: Int) {
        val effective = uri ?: imageCollectionUri
        val groupId = mediaChangeGroupCounter.incrementAndGet()
        val triggerElapsedNs = SystemClock.elapsedRealtimeNanos()

        appendEvent(
            "MEDIASTORE_CHANGE",
            JSONObject()
                .put("changeGroupId", groupId)
                .put("selfChange", selfChange)
                .put("uri", effective.toString())
                .put("flags", flags)
                .put("triggerElapsedRealtimeNs", triggerElapsedNs)
                .put("changeObservedWithoutOpeningMediaFile", true),
        )

        for (delayMs in DELAYED_SNAPSHOT_MS) {
            worker.postDelayed(
                {
                    appendMediaSnapshot(
                        eventType = "MEDIASTORE_DELAYED_METADATA_SNAPSHOT",
                        requestedUri = effective,
                        changeGroupId = groupId,
                        scheduledDelayMs = delayMs,
                        triggerElapsedNs = triggerElapsedNs,
                    )
                },
                delayMs,
            )
        }
    }

    private fun appendMediaSnapshot(
        eventType: String,
        requestedUri: Uri,
        changeGroupId: Int?,
        scheduledDelayMs: Long?,
        triggerElapsedNs: Long?,
    ) {
        val queryStartElapsedNs = SystemClock.elapsedRealtimeNanos()
        val result = queryMetadata(requestedUri)
        val queryEndElapsedNs = SystemClock.elapsedRealtimeNanos()

        val payload = JSONObject()
            .put("requestedUri", requestedUri.toString())
            .put("permission", permissionSnapshot())
            .put("queryStartElapsedRealtimeNs", queryStartElapsedNs)
            .put("queryEndElapsedRealtimeNs", queryEndElapsedNs)
            .put("queryDurationNs", queryEndElapsedNs - queryStartElapsedNs)
            .put("query", result)
            .put("imagePixelBytesRead", false)
            .put("mediaInputStreamOpened", false)
            .put("exifDecoded", false)

        if (changeGroupId != null) payload.put("changeGroupId", changeGroupId)
        if (scheduledDelayMs != null) payload.put("scheduledDelayMs", scheduledDelayMs)
        if (triggerElapsedNs != null) {
            payload.put("triggerElapsedRealtimeNs", triggerElapsedNs)
            payload.put(
                "actualSnapshotStartDelayNs",
                queryStartElapsedNs - triggerElapsedNs,
            )
        }

        appendEvent(eventType, payload)
    }

    private fun queryMetadata(requestedUri: Uri): JSONObject {
        val out = JSONObject()
            .put("requestedUri", requestedUri.toString())
            .put("projection", JSONArray(PROJECTION.toList()))
            .put("queryAuthority", "MEDIASTORE_DATABASE_METADATA_ONLY")
            .put("pixelRead", false)
            .put("exifDecode", false)

        return runCatching {
            val numericItem = requestedUri.lastPathSegment?.toLongOrNull() != null
            val queryUri = if (numericItem) requestedUri else imageCollectionUri

            val args = Bundle().apply {
                putStringArray(
                    ContentResolver.QUERY_ARG_SORT_COLUMNS,
                    arrayOf(MediaStore.MediaColumns.DATE_ADDED, BaseColumns._ID),
                )
                putInt(
                    ContentResolver.QUERY_ARG_SORT_DIRECTION,
                    ContentResolver.QUERY_SORT_DIRECTION_DESCENDING,
                )
                putInt(ContentResolver.QUERY_ARG_LIMIT, if (numericItem) 1 else 8)
            }

            val rows = JSONArray()
            contentResolver.query(queryUri, PROJECTION, args, null)?.use { cursor ->
                val indices = PROJECTION.associateWith { cursor.getColumnIndex(it) }
                while (cursor.moveToNext()) {
                    val row = JSONObject()
                    for (column in PROJECTION) {
                        val index = indices[column] ?: -1
                        if (index < 0 || cursor.isNull(index)) {
                            row.put(column, JSONObject.NULL)
                            continue
                        }
                        when (cursor.getType(index)) {
                            android.database.Cursor.FIELD_TYPE_INTEGER ->
                                row.put(column, cursor.getLong(index))
                            android.database.Cursor.FIELD_TYPE_FLOAT ->
                                row.put(column, cursor.getDouble(index))
                            android.database.Cursor.FIELD_TYPE_STRING ->
                                row.put(column, cursor.getString(index))
                            else ->
                                row.put(column, cursor.getString(index))
                        }
                    }

                    val id = row.optLong(BaseColumns._ID, -1L)
                    val dateAddedSeconds = row.optLong(MediaStore.MediaColumns.DATE_ADDED, -1L)

                    row.put(
                        "contentUri",
                        if (id >= 0L)
                            Uri.withAppendedPath(imageCollectionUri, id.toString()).toString()
                        else JSONObject.NULL,
                    )
                    row.put(
                        "dateAddedAfterObserverStart",
                        dateAddedSeconds > 0 &&
                            dateAddedSeconds * 1000L >= observerStartEpochMs - DATE_TOLERANCE_MS,
                    )
                    row.put("metadataOnly", true)
                    rows.put(row)
                }
            }

            out.put("success", true)
                .put("rowCount", rows.length())
                .put("rows", rows)
        }.getOrElse {
            out.put("success", false)
                .put("errorClass", it.javaClass.name)
                .put("errorMessage", it.message ?: JSONObject.NULL)
                .put("rows", JSONArray())
        }
    }

    private fun permissionSnapshot(): JSONObject {
        val out = JSONObject()

        if (Build.VERSION.SDK_INT >= 33) {
            out.put(
                "READ_MEDIA_IMAGES",
                checkSelfPermission(android.Manifest.permission.READ_MEDIA_IMAGES) ==
                    android.content.pm.PackageManager.PERMISSION_GRANTED,
            )
        } else {
            out.put("READ_MEDIA_IMAGES", JSONObject.NULL)
        }

        if (Build.VERSION.SDK_INT >= 34) {
            out.put(
                "READ_MEDIA_VISUAL_USER_SELECTED",
                checkSelfPermission(android.Manifest.permission.READ_MEDIA_VISUAL_USER_SELECTED) ==
                    android.content.pm.PackageManager.PERMISSION_GRANTED,
            )
        } else {
            out.put("READ_MEDIA_VISUAL_USER_SELECTED", JSONObject.NULL)
        }

        return out
    }

    private fun buildNotification(stateText: String): Notification {
        val flags = PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE

        fun markerPendingIntent(action: String, requestCode: Int): PendingIntent =
            PendingIntent.getService(
                this,
                requestCode,
                Intent(this, PassiveModeShutterTimelineService::class.java).setAction(action),
                flags,
            )

        return Notification.Builder(this, CHANNEL_ID)
            .setSmallIcon(android.R.drawable.ic_menu_gallery)
            .setContentTitle("TruthRaw v0.43 passive timeline")
            .setContentText(stateText)
            .setStyle(Notification.BigTextStyle().bigText(
                "$stateText\nMarkers zijn menselijke tijdsmarkeringen; Honor blijft camera-eigenaar.",
            ))
            .setOngoing(true)
            .setCategory(Notification.CATEGORY_SERVICE)
            .addAction(
                Notification.Action.Builder(
                    null,
                    "MAIN STABLE",
                    markerPendingIntent(ACTION_MARK_MAIN_STABLE, 4301),
                ).build(),
            )
            .addAction(
                Notification.Action.Builder(
                    null,
                    "MODE SELECTED",
                    markerPendingIntent(ACTION_MARK_MODE_SELECTED, 4302),
                ).build(),
            )
            .addAction(
                Notification.Action.Builder(
                    null,
                    "SHUTTER PRESSED",
                    markerPendingIntent(ACTION_MARK_SHUTTER_PRESSED, 4303),
                ).build(),
            )
            .build()
    }

    @Synchronized
    private fun appendEvent(type: String, payload: JSONObject): Int {
        val report = runCatching { JSONObject(REPORT_FILE.readText()) }.getOrElse {
            JSONObject()
                .put("schema", "truthraw.passive-mode-shutter-mediastore-timeline.v0.43")
                .put("events", JSONArray())
        }

        val events = report.optJSONArray("events") ?: JSONArray().also {
            report.put("events", it)
        }

        val sequence = events.length()
        events.put(
            JSONObject()
                .put("sequence", sequence)
                .put("type", type)
                .put("utc", Instant.now().toString())
                .put("elapsedRealtimeNs", SystemClock.elapsedRealtimeNanos())
                .put("payload", payload),
        )

        report.put("lastUpdatedAtUtc", Instant.now().toString())
        report.put("mediaPermissionCurrent", permissionSnapshot())
        report.put("runProfileCurrent", runProfile)
        REPORT_FILE.writeText(report.toString(2))
        return sequence
    }

    private fun ensureNotificationChannel() {
        getSystemService(NotificationManager::class.java).createNotificationChannel(
            NotificationChannel(
                CHANNEL_ID,
                "TruthRaw v0.43 passive timeline",
                NotificationManager.IMPORTANCE_LOW,
            ),
        )
    }

    private val REPORT_FILE: File
        get() = File(filesDir, REPORT_FILENAME)

    companion object {
        const val ACTION_START = "com.truthraw.adaptiveui.v043.START"
        const val ACTION_MARK_LAUNCH = "com.truthraw.adaptiveui.v043.MARK_LAUNCH"
        const val ACTION_MARK_MAIN_STABLE = "com.truthraw.adaptiveui.v043.MARK_MAIN_STABLE"
        const val ACTION_MARK_MODE_SELECTED = "com.truthraw.adaptiveui.v043.MARK_MODE_SELECTED"
        const val ACTION_MARK_SHUTTER_PRESSED = "com.truthraw.adaptiveui.v043.MARK_SHUTTER_PRESSED"
        const val ACTION_MARK_RETURNED = "com.truthraw.adaptiveui.v043.MARK_RETURNED"
        const val ACTION_SNAPSHOT_MEDIA = "com.truthraw.adaptiveui.v043.SNAPSHOT_MEDIA"
        const val ACTION_STOP = "com.truthraw.adaptiveui.v043.STOP"

        const val EXTRA_RUN_PROFILE = "run_profile"
        const val EXTRA_MARK_SOURCE = "mark_source"

        const val PROFILE_NORMAL = "FRESH_PHOTO_CONTROL_REPLICATE"
        const val PROFILE_PRO = "FRESH_PRO_CONTROL_REPLICATE"
        const val PROFILE_HIRES = "FRESH_HIRES_CANDIDATE_REPLICATE"
        const val PROFILE_UNSPECIFIED = "UNSPECIFIED"

        const val MARK_WILL_OPEN_HONOR = "USER_WILL_OPEN_HONOR_CAMERA_MANUALLY"
        const val MARK_MAIN_STABLE = "MAIN_STABLE"
        const val MARK_PHOTO_SELECTED = "MODE_PHOTO_SELECTED"
        const val MARK_PRO_SELECTED = "MODE_PRO_SELECTED"
        const val MARK_HIRES_SELECTED = "MODE_HIRES_SELECTED"
        const val MARK_MODE_SELECTED_UNSPECIFIED = "MODE_SELECTED_UNSPECIFIED"
        const val MARK_SHUTTER_PRESSED = "SHUTTER_PRESSED"
        const val MARK_RETURNED = "USER_RETURNED_FROM_HONOR_CAMERA"

        const val REPORT_FILENAME = "TRUTHRAW_PASSIVE_MODE_SHUTTER_TIMELINE_v043.json"

        private const val CHANNEL_ID = "truthraw_v043_passive_mode_shutter"
        private const val NOTIFICATION_ID = 43043
        private const val DATE_TOLERANCE_MS = 5000L
        private const val STOP_DRAIN_MS = 1250L

        private val DELAYED_SNAPSHOT_MS = longArrayOf(0L, 25L, 100L, 250L, 1000L)

        private val PROJECTION = arrayOf(
            BaseColumns._ID,
            MediaStore.MediaColumns.DISPLAY_NAME,
            MediaStore.MediaColumns.MIME_TYPE,
            MediaStore.MediaColumns.WIDTH,
            MediaStore.MediaColumns.HEIGHT,
            MediaStore.MediaColumns.SIZE,
            MediaStore.Images.ImageColumns.DATE_TAKEN,
            MediaStore.MediaColumns.DATE_ADDED,
            MediaStore.MediaColumns.DATE_MODIFIED,
            MediaStore.MediaColumns.RELATIVE_PATH,
            MediaStore.MediaColumns.IS_PENDING,
            MediaStore.MediaColumns.OWNER_PACKAGE_NAME,
            MediaStore.MediaColumns.VOLUME_NAME,
        )
    }
}
