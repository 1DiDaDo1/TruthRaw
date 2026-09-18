package com.truthraw.adaptiveui

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
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

/**
 * v0.42 passive system-visible output timeline.
 *
 * TruthRaw does not open a camera, submit capture requests, read image bytes, decode EXIF,
 * invoke Honor Binder callbacks, or write vendor controls. It observes only:
 *   1) CameraManager availability/physical-availability callbacks;
 *   2) MediaStore Images change notifications and database metadata rows.
 */
class PassiveMediaStoreCameraTimelineService : Service() {
    private lateinit var workerThread: HandlerThread
    private lateinit var worker: Handler
    private lateinit var cameraManager: CameraManager
    private lateinit var mediaObserver: ContentObserver
    private val running = AtomicBoolean(false)

    private var observerStartEpochMs: Long = 0L
    private var observerStartElapsedNs: Long = 0L

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
        workerThread = HandlerThread("truthraw-v042-passive-media-camera-timeline").also { it.start() }
        worker = Handler(workerThread.looper)
        cameraManager = getSystemService(CameraManager::class.java)

        ensureNotificationChannel()
        startForeground(
            NOTIFICATION_ID,
            Notification.Builder(this, CHANNEL_ID)
                .setSmallIcon(android.R.drawable.ic_menu_gallery)
                .setContentTitle("TruthRaw v0.42 passive timeline")
                .setContentText("Camera availability + MediaStore metadata; geen pixel-read.")
                .setOngoing(true)
                .setCategory(Notification.CATEGORY_SERVICE)
                .build(),
        )

        worker.post { startObservation() }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_MARK -> worker.post {
                appendEvent(
                    "USER_MARK",
                    JSONObject()
                        .put("label", intent.getStringExtra(EXTRA_LABEL) ?: "unspecified")
                        .put("note", intent.getStringExtra(EXTRA_NOTE) ?: JSONObject.NULL),
                )
            }

            ACTION_SNAPSHOT_MEDIA -> worker.post {
                appendMediaSnapshot("USER_REQUESTED_MEDIA_SNAPSHOT", imageCollectionUri)
            }

            ACTION_STOP -> worker.post {
                appendEvent("OBSERVER_STOP_REQUESTED", JSONObject())
                stopSelf()
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
                .put("cameraOpenedByTruthRaw", false)
                .put("captureSubmittedByTruthRaw", false)
                .put("imagePixelBytesRead", false)
                .put("exifDecoded", false)
                .put("honorBinderMethodInvoked", false),
        )
        running.set(false)
        workerThread.quitSafely()
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    private fun startObservation() {
        if (!running.compareAndSet(false, true)) return

        observerStartEpochMs = System.currentTimeMillis()
        observerStartElapsedNs = SystemClock.elapsedRealtimeNanos()

        val report = JSONObject()
            .put("schema", "truthraw.passive-mediastore-camera-timeline.v0.42")
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", "PASSIVE_SYSTEM_VISIBLE_OUTPUT_OBSERVATION_ONLY")
            .put("scientificMasterModified", false)
            .put("captureEvidenceGranted", false)
            .put("cameraOpenedByTruthRaw", false)
            .put("captureSubmittedByTruthRaw", false)
            .put("imagePixelBytesRead", false)
            .put("exifDecoded", false)
            .put("mediaInputStreamOpened", false)
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
                .put("sdkInt", Build.VERSION.SDK_INT)
                .put("targetSdk", applicationInfo.targetSdkVersion)
                .put("observerMode", "ANDROID16_CAMERA_AVAILABILITY_PLUS_MEDIASTORE_METADATA_TIMELINE")
                .put("mediaPermission", permissionSnapshot()),
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
            appendMediaSnapshot("MEDIASTORE_BASELINE_SNAPSHOT", imageCollectionUri)
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

    private fun handleMediaChange(selfChange: Boolean, uri: Uri?, flags: Int) {
        val effective = uri ?: imageCollectionUri
        appendEvent(
            "MEDIASTORE_CHANGE",
            JSONObject()
                .put("selfChange", selfChange)
                .put("uri", effective.toString())
                .put("flags", flags)
                .put("changeObservedWithoutOpeningMediaFile", true),
        )
        appendMediaSnapshot("MEDIASTORE_CHANGE_METADATA_SNAPSHOT", effective)
    }

    private fun appendMediaSnapshot(type: String, requestedUri: Uri) {
        val result = queryMetadata(requestedUri)
        appendEvent(
            type,
            JSONObject()
                .put("requestedUri", requestedUri.toString())
                .put("permission", permissionSnapshot())
                .put("query", result)
                .put("imagePixelBytesRead", false)
                .put("mediaInputStreamOpened", false)
                .put("exifDecoded", false),
        )
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
                    val dateAddedSeconds =
                        row.optLong(MediaStore.MediaColumns.DATE_ADDED, -1L)
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

    @Synchronized
    private fun appendEvent(type: String, payload: JSONObject) {
        val report = runCatching { JSONObject(REPORT_FILE.readText()) }.getOrElse {
            JSONObject()
                .put("schema", "truthraw.passive-mediastore-camera-timeline.v0.42")
                .put("events", JSONArray())
        }
        val events = report.optJSONArray("events") ?: JSONArray().also {
            report.put("events", it)
        }
        events.put(
            JSONObject()
                .put("sequence", events.length())
                .put("type", type)
                .put("utc", Instant.now().toString())
                .put("elapsedRealtimeNs", SystemClock.elapsedRealtimeNanos())
                .put("payload", payload),
        )
        report.put("lastUpdatedAtUtc", Instant.now().toString())
        report.put("mediaPermissionCurrent", permissionSnapshot())
        REPORT_FILE.writeText(report.toString(2))
    }

    private fun ensureNotificationChannel() {
        getSystemService(NotificationManager::class.java).createNotificationChannel(
            NotificationChannel(
                CHANNEL_ID,
                "TruthRaw passive MediaStore timeline",
                NotificationManager.IMPORTANCE_LOW,
            ),
        )
    }

    private val REPORT_FILE: File
        get() = File(filesDir, REPORT_FILENAME)

    companion object {
        const val ACTION_MARK = "com.truthraw.adaptiveui.v042.MARK"
        const val ACTION_SNAPSHOT_MEDIA = "com.truthraw.adaptiveui.v042.SNAPSHOT_MEDIA"
        const val ACTION_STOP = "com.truthraw.adaptiveui.v042.STOP"
        const val EXTRA_LABEL = "label"
        const val EXTRA_NOTE = "note"
        const val REPORT_FILENAME = "TRUTHRAW_PASSIVE_MEDIASTORE_CAMERA_TIMELINE_v042.json"

        private const val CHANNEL_ID = "truthraw_v042_passive_media_camera"
        private const val NOTIFICATION_ID = 42042
        private const val DATE_TOLERANCE_MS = 5000L

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
