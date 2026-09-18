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
import android.media.ExifInterface
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
 * v0.45 passive exported-JPEG metadata fingerprint timeline.
 *
 * TruthRaw remains outside the Honor capture pipeline. It records CameraManager availability,
 * explicit user UI-state markers, MediaStore row-state transitions, and selected EXIF metadata
 * from stable Honor JPEG outputs.
 *
 * v0.45 deliberately crosses one boundary that v0.44 did not: it opens a read-only media file
 * descriptor after a stable non-null Honor JPEG size is visible, and parses selected exported
 * EXIF tags. That metadata has EXPORTED_OUTPUT_METADATA_ONLY authority. No camera is opened,
 * no capture is submitted, no image pixel array is decoded, no GPS tags are requested,
 * no Honor Binder method is invoked, and no vendor request key is written.
 */
class PassiveExportedJpegMetadataFingerprintService : Service() {
    private lateinit var workerThread: HandlerThread
    private lateinit var worker: Handler
    private lateinit var cameraManager: CameraManager
    private lateinit var mediaObserver: ContentObserver

    private val running = AtomicBoolean(false)
    private val mediaChangeGroupCounter = AtomicInteger(0)

    private var observerStartEpochMs: Long = 0L
    private var observerStartElapsedNs: Long = 0L
    private var runProfile: String = PROFILE_UNSPECIFIED
    private var lastHumanMarkerLabel: String? = null
    private var lastHumanMarkerElapsedNs: Long? = null
    private var currentUserStateLabel: String? = null
    private val lastRowStateByUri = mutableMapOf<String, MediaRowState>()
    private val emittedExifFingerprintKeys = mutableSetOf<String>()
    private var exportedExifFingerprintCount: Int = 0

    private data class MediaRowState(
        val width: Long?,
        val height: Long?,
        val sizeBytes: Long?,
        val dateModified: Long?,
        val isPending: Long?,
    ) {
        fun toJson(): JSONObject = JSONObject()
            .put("width", width ?: JSONObject.NULL)
            .put("height", height ?: JSONObject.NULL)
            .put("sizeBytes", sizeBytes ?: JSONObject.NULL)
            .put("dateModified", dateModified ?: JSONObject.NULL)
            .put("isPending", isPending ?: JSONObject.NULL)
    }

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
        workerThread = HandlerThread("truthraw-v045-mode-shutter-timeline").also { it.start() }
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

            ACTION_MARK_PHOTO_CONFIRMED -> worker.post {
                appendUserMarker(
                    MARK_PHOTO_MAIN_CONFIRMED,
                    "User reports the normal main/PHOTO state is visible and stable. This is a user-visible state marker, not a camera-pipeline semantic claim.",
                    intent.getStringExtra(EXTRA_MARK_SOURCE) ?: "notification_action",
                )
            }

            ACTION_MARK_PRO_SELECTED -> worker.post {
                appendUserMarker(
                    MARK_PRO_SELECTED,
                    "User reports PRO was manually selected and is visible/stable. Human UI-state marker only.",
                    intent.getStringExtra(EXTRA_MARK_SOURCE) ?: "notification_action",
                )
            }

            ACTION_MARK_HIRES_MAIN_CONFIRMED -> worker.post {
                appendUserMarker(
                    MARK_HIRES_MAIN_CONFIRMED,
                    "User reports HI-RES is active while still on the main-camera UI state. User-visible state only; active physical camera ID is not proven.",
                    intent.getStringExtra(EXTRA_MARK_SOURCE) ?: "notification_action",
                )
            }

            ACTION_MARK_TELE_200MP_UI_SELECTED -> worker.post {
                appendUserMarker(
                    MARK_TELE_200MP_UI_SELECTED,
                    "User reports the tele / UI-described 200MP state was manually selected inside HI-RES. This does not prove active physical camera ID 5 or native 200MP sensor sampling.",
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
                .put("imagePixelArrayDecoded", false)
                .put("mediaInputStreamOpened", false)
                .put("mediaFileDescriptorMayHaveBeenOpenedForExif", exportedExifFingerprintCount > 0)
                .put("exportedExifFingerprintCount", exportedExifFingerprintCount)
                .put("exportedExifMetadataAuthority", "EXPORTED_OUTPUT_METADATA_ONLY")
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
            PROFILE_PHOTO -> PROFILE_PHOTO
            PROFILE_PRO -> PROFILE_PRO
            PROFILE_HIRES_TELE -> PROFILE_HIRES_TELE
            else -> PROFILE_UNSPECIFIED
        }

        observerStartEpochMs = System.currentTimeMillis()
        observerStartElapsedNs = SystemClock.elapsedRealtimeNanos()

        val report = JSONObject()
            .put("schema", "truthraw.passive-hires-tele-state-timeline.v0.45")
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", "PASSIVE_SYSTEM_VISIBLE_OUTPUT_OBSERVATION_PLUS_EXPORTED_OUTPUT_METADATA")
            .put("exportedMetadataAuthority", "EXPORTED_OUTPUT_METADATA_ONLY")
            .put("runProfile", runProfile)
            .put("runProfileIsUserDeclaredExperimentalLabel", true)
            .put("scientificMasterModified", false)
            .put("captureEvidenceGranted", false)
            .put("cameraOpenedByTruthRaw", false)
            .put("captureSubmittedByTruthRaw", false)
            .put("imagePixelArrayDecoded", false)
            .put("mediaInputStreamOpened", false)
            .put("mediaFileDescriptorMayBeOpenedForExif", true)
            .put("exifMetadataParsingEnabled", true)
            .put("gpsExifTagsRequested", false)
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
            .put(
                "v045ObservationSemantics",
                JSONObject()
                    .put("screenshotRowsAreUiAnchorCandidatesOnly", true)
                    .put("honorCameraRowsAreSystemVisibleOutputCandidatesOnly", true)
                    .put("tele200mpMarkerIsUserVisibleStateOnly", true)
                    .put("activePhysicalCameraId5ProvenByMarker", false)
                    .put("sameRowMetadataTransitionsTracked", true)
                    .put("stableHonorJpegExifFingerprintEnabled", true)
                    .put("exportedExifAuthority", "EXPORTED_OUTPUT_METADATA_ONLY")
                    .put("exifDoesNotPromoteToCaptureEvidence", true)
                    .put("gpsTagsRead", false)
                    .put("imagePixelArrayDecoded", false),
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
                    "ANDROID16_HIRES_MAIN_TELE_UI_PLUS_MEDIASTORE_STATE_TRANSITIONS_PLUS_STABLE_EXPORTED_JPEG_EXIF",
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
        val markerElapsedNs = SystemClock.elapsedRealtimeNanos()
        lastHumanMarkerLabel = label
        lastHumanMarkerElapsedNs = markerElapsedNs
        if (label == MARK_PHOTO_MAIN_CONFIRMED ||
            label == MARK_PRO_SELECTED ||
            label == MARK_HIRES_MAIN_CONFIRMED ||
            label == MARK_TELE_200MP_UI_SELECTED ||
            label == MARK_MAIN_STABLE
        ) {
            currentUserStateLabel = label
        }

        appendEvent(
            "USER_MARK",
            JSONObject()
                .put("label", label)
                .put("note", note)
                .put("source", source)
                .put("markerRequestedElapsedRealtimeNs", markerElapsedNs)
                .put("hardwareTimestamp", false)
                .put("userDeclaredStateOnly", true)
                .put("semanticPromotionAllowed", false),
        )
    }

    private fun handleMediaChange(selfChange: Boolean, uri: Uri?, flags: Int) {
        val effective = uri ?: imageCollectionUri
        val groupId = mediaChangeGroupCounter.incrementAndGet()
        val triggerElapsedNs = SystemClock.elapsedRealtimeNanos()

        val changePayload = JSONObject()
            .put("changeGroupId", groupId)
            .put("selfChange", selfChange)
            .put("uri", effective.toString())
            .put("flags", flags)
            .put("triggerElapsedRealtimeNs", triggerElapsedNs)
            .put("changeObservedWithoutOpeningMediaFile", true)

        lastHumanMarkerLabel?.let { changePayload.put("lastHumanMarkerLabel", it) }
        currentUserStateLabel?.let { changePayload.put("currentUserStateLabel", it) }
        lastHumanMarkerElapsedNs?.let {
            changePayload.put("lastHumanMarkerElapsedRealtimeNs", it)
            changePayload.put("deltaFromLastHumanMarkerNs", triggerElapsedNs - it)
        }

        appendEvent("MEDIASTORE_CHANGE", changePayload)

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
            .put("imagePixelArrayDecoded", false)
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
        maybeAppendExportedExifFingerprints(result)
    }

    private fun maybeAppendExportedExifFingerprints(queryResult: JSONObject) {
        val rows = queryResult.optJSONArray("rows") ?: return
        for (i in 0 until rows.length()) {
            val row = rows.optJSONObject(i) ?: continue
            if (row.optString("observationClass") != "HONOR_CAMERA_SYSTEM_VISIBLE_OUTPUT_CANDIDATE") continue
            if (row.optString(MediaStore.MediaColumns.MIME_TYPE) != "image/jpeg") continue
            if (row.isNull(MediaStore.MediaColumns.SIZE)) continue

            val sizeBytes = row.optLong(MediaStore.MediaColumns.SIZE, -1L)
            if (sizeBytes <= 0L) continue

            val contentUriText = row.optString("contentUri", "")
            if (contentUriText.isBlank()) continue

            val dateModified =
                if (row.isNull(MediaStore.MediaColumns.DATE_MODIFIED)) -1L
                else row.optLong(MediaStore.MediaColumns.DATE_MODIFIED, -1L)
            val fingerprintKey = "$contentUriText|$sizeBytes|$dateModified"
            if (!emittedExifFingerprintKeys.add(fingerprintKey)) continue

            appendExportedExifFingerprint(row, Uri.parse(contentUriText), fingerprintKey)
        }
    }

    @Suppress("DEPRECATION")
    private fun appendExportedExifFingerprint(
        row: JSONObject,
        uri: Uri,
        fingerprintKey: String,
    ) {
        val parseStartNs = SystemClock.elapsedRealtimeNanos()
        val tags = JSONObject()
        var fileDescriptorOpened = false

        val parseResult = runCatching {
            contentResolver.openFileDescriptor(uri, "r")?.use { pfd ->
                fileDescriptorOpened = true
                val exif = ExifInterface(pfd.fileDescriptor)
                for (tag in EXPORTED_EXIF_TAGS) {
                    val value = exif.getAttribute(tag)
                    if (value != null) tags.put(tag, value)
                }
            } ?: error("openFileDescriptor returned null")
        }

        val parseEndNs = SystemClock.elapsedRealtimeNanos()
        val payload = JSONObject()
            .put("authority", "EXPORTED_OUTPUT_METADATA_ONLY")
            .put("sourceUri", uri.toString())
            .put("displayName", row.optString(MediaStore.MediaColumns.DISPLAY_NAME, ""))
            .put("mediaStoreWidth", if (row.isNull(MediaStore.MediaColumns.WIDTH)) JSONObject.NULL else row.optLong(MediaStore.MediaColumns.WIDTH))
            .put("mediaStoreHeight", if (row.isNull(MediaStore.MediaColumns.HEIGHT)) JSONObject.NULL else row.optLong(MediaStore.MediaColumns.HEIGHT))
            .put("mediaStoreSizeBytes", if (row.isNull(MediaStore.MediaColumns.SIZE)) JSONObject.NULL else row.optLong(MediaStore.MediaColumns.SIZE))
            .put("mediaStoreGeometryClass", row.optString("systemVisibleGeometryClass", "UNKNOWN_GEOMETRY"))
            .put("fingerprintKey", fingerprintKey)
            .put("fileDescriptorOpenedReadOnly", fileDescriptorOpened)
            .put("mediaInputStreamOpened", false)
            .put("imagePixelArrayDecoded", false)
            .put("bitmapDecoded", false)
            .put("gpsTagsRequested", false)
            .put("selectedExifTags", tags)
            .put("parseStartElapsedRealtimeNs", parseStartNs)
            .put("parseEndElapsedRealtimeNs", parseEndNs)
            .put("parseDurationNs", parseEndNs - parseStartNs)
            .put("semanticPromotionAllowed", false)
            .put("captureEvidenceGranted", false)

        currentUserStateLabel?.let { payload.put("associatedUserStateLabel", it) }
        lastHumanMarkerLabel?.let { payload.put("lastHumanMarkerLabel", it) }

        parseResult.onSuccess {
            exportedExifFingerprintCount += 1
            payload.put("success", true)
        }.onFailure {
            payload
                .put("success", false)
                .put("errorClass", it.javaClass.name)
                .put("errorMessage", it.message ?: JSONObject.NULL)
        }

        appendEvent("EXPORTED_JPEG_METADATA_FINGERPRINT", payload)
    }

    private fun queryMetadata(requestedUri: Uri): JSONObject {
        val out = JSONObject()
            .put("requestedUri", requestedUri.toString())
            .put("projection", JSONArray(PROJECTION.toList()))
            .put("queryAuthority", "MEDIASTORE_DATABASE_METADATA_ONLY")
            .put("pixelArrayDecode", false)
            .put("exifDecodeInMediaStoreQuery", false)

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

                    val contentUriText =
                        if (id >= 0L) Uri.withAppendedPath(imageCollectionUri, id.toString()).toString() else null
                    val widthValue =
                        if (row.isNull(MediaStore.MediaColumns.WIDTH)) null else row.optLong(MediaStore.MediaColumns.WIDTH)
                    val heightValue =
                        if (row.isNull(MediaStore.MediaColumns.HEIGHT)) null else row.optLong(MediaStore.MediaColumns.HEIGHT)
                    val sizeValue =
                        if (row.isNull(MediaStore.MediaColumns.SIZE)) null else row.optLong(MediaStore.MediaColumns.SIZE)
                    val modifiedValue =
                        if (row.isNull(MediaStore.MediaColumns.DATE_MODIFIED)) null else row.optLong(MediaStore.MediaColumns.DATE_MODIFIED)
                    val pendingValue =
                        if (row.isNull(MediaStore.MediaColumns.IS_PENDING)) null else row.optLong(MediaStore.MediaColumns.IS_PENDING)

                    row.put("observationClass", classifyMediaRow(row))
                    row.put(
                        "systemVisiblePixelCount",
                        if (widthValue != null && heightValue != null) widthValue * heightValue else JSONObject.NULL,
                    )
                    row.put("systemVisibleGeometryClass", geometryClass(widthValue, heightValue))

                    if (contentUriText != null) {
                        val currentState = MediaRowState(
                            width = widthValue,
                            height = heightValue,
                            sizeBytes = sizeValue,
                            dateModified = modifiedValue,
                            isPending = pendingValue,
                        )
                        val previousState = lastRowStateByUri[contentUriText]
                        row.put("firstMetadataObservationForUri", previousState == null)
                        row.put(
                            "metadataStateChangedSincePreviousObservation",
                            previousState != null && previousState != currentState,
                        )
                        if (previousState != null && previousState != currentState) {
                            row.put(
                                "metadataStateTransition",
                                JSONObject()
                                    .put("previous", previousState.toJson())
                                    .put("current", currentState.toJson())
                                    .put("sameMediaStoreUri", true)
                                    .put("pixelBytesRead", false),
                            )
                        }
                        lastRowStateByUri[contentUriText] = currentState
                    }

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

    private fun classifyMediaRow(row: JSONObject): String {
        val name =
            if (row.isNull(MediaStore.MediaColumns.DISPLAY_NAME)) "" else row.optString(MediaStore.MediaColumns.DISPLAY_NAME)
        val path =
            if (row.isNull(MediaStore.MediaColumns.RELATIVE_PATH)) "" else row.optString(MediaStore.MediaColumns.RELATIVE_PATH)
        val owner =
            if (row.isNull(MediaStore.MediaColumns.OWNER_PACKAGE_NAME)) "" else row.optString(MediaStore.MediaColumns.OWNER_PACKAGE_NAME)

        val lowerName = name.lowercase()
        val lowerPath = path.lowercase()
        return when {
            lowerName.contains("screenshot") || lowerPath.contains("screenshot") ->
                "SCREENSHOT_UI_ANCHOR_CANDIDATE"
            owner == "com.hihonor.camera" && lowerPath.startsWith("dcim/camera") ->
                "HONOR_CAMERA_SYSTEM_VISIBLE_OUTPUT_CANDIDATE"
            else ->
                "OTHER_MEDIASTORE_IMAGE"
        }
    }

    private fun geometryClass(width: Long?, height: Long?): String {
        if (width == null || height == null) return "UNKNOWN_GEOMETRY"
        return when (width * height) {
            12_582_912L -> "SYSTEM_VISIBLE_12_582_912PX_CLASS"
            50_331_648L -> "SYSTEM_VISIBLE_50_331_648PX_CLASS"
            200_540_160L -> "SYSTEM_VISIBLE_200_540_160PX_CLASS"
            else -> "SYSTEM_VISIBLE_OTHER_PIXEL_COUNT"
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
                Intent(this, PassiveExportedJpegMetadataFingerprintService::class.java).setAction(action),
                flags,
            )

        fun action(label: String, actionName: String, requestCode: Int): Notification.Action =
            Notification.Action.Builder(
                null,
                label,
                markerPendingIntent(actionName, requestCode),
            ).build()

        val builder = Notification.Builder(this, CHANNEL_ID)
            .setSmallIcon(android.R.drawable.ic_menu_gallery)
            .setContentTitle("TruthRaw v0.45 exported JPEG metadata")
            .setContentText(stateText)
            .setStyle(
                Notification.BigTextStyle().bigText(
                    "$stateText\nMarkers zijn menselijke UI-state/tijdankers; Honor blijft camera-eigenaar.",
                ),
            )
            .setOngoing(true)
            .setCategory(Notification.CATEGORY_SERVICE)

        when (runProfile) {
            PROFILE_PHOTO -> {
                builder.addAction(action("MAIN/PHOTO", ACTION_MARK_PHOTO_CONFIRMED, 4401))
                builder.addAction(action("SHUTTER", ACTION_MARK_SHUTTER_PRESSED, 4402))
            }

            PROFILE_PRO -> {
                builder.addAction(action("MAIN STABLE", ACTION_MARK_MAIN_STABLE, 4411))
                builder.addAction(action("PRO SELECTED", ACTION_MARK_PRO_SELECTED, 4412))
                builder.addAction(action("SHUTTER", ACTION_MARK_SHUTTER_PRESSED, 4413))
            }

            PROFILE_HIRES_TELE -> {
                builder.addAction(action("HIRES MAIN", ACTION_MARK_HIRES_MAIN_CONFIRMED, 4421))
                builder.addAction(action("TELE / 200MP UI", ACTION_MARK_TELE_200MP_UI_SELECTED, 4422))
                builder.addAction(action("SHUTTER", ACTION_MARK_SHUTTER_PRESSED, 4423))
            }

            else -> Unit
        }

        return builder.build()
    }

    @Synchronized
    private fun appendEvent(type: String, payload: JSONObject): Int {
        val report = runCatching { JSONObject(REPORT_FILE.readText()) }.getOrElse {
            JSONObject()
                .put("schema", "truthraw.passive-hires-tele-state-timeline.v0.45")
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
                "TruthRaw v0.45 passive timeline",
                NotificationManager.IMPORTANCE_LOW,
            ),
        )
    }

    private val REPORT_FILE: File
        get() = File(filesDir, REPORT_FILENAME)

    companion object {
        const val ACTION_START = "com.truthraw.adaptiveui.v045.START"
        const val ACTION_MARK_LAUNCH = "com.truthraw.adaptiveui.v045.MARK_LAUNCH"
        const val ACTION_MARK_MAIN_STABLE = "com.truthraw.adaptiveui.v045.MARK_MAIN_STABLE"
        const val ACTION_MARK_PHOTO_CONFIRMED = "com.truthraw.adaptiveui.v045.MARK_PHOTO_CONFIRMED"
        const val ACTION_MARK_PRO_SELECTED = "com.truthraw.adaptiveui.v045.MARK_PRO_SELECTED"
        const val ACTION_MARK_HIRES_MAIN_CONFIRMED = "com.truthraw.adaptiveui.v045.MARK_HIRES_MAIN_CONFIRMED"
        const val ACTION_MARK_TELE_200MP_UI_SELECTED = "com.truthraw.adaptiveui.v045.MARK_TELE_200MP_UI_SELECTED"
        const val ACTION_MARK_SHUTTER_PRESSED = "com.truthraw.adaptiveui.v045.MARK_SHUTTER_PRESSED"
        const val ACTION_MARK_RETURNED = "com.truthraw.adaptiveui.v045.MARK_RETURNED"
        const val ACTION_SNAPSHOT_MEDIA = "com.truthraw.adaptiveui.v045.SNAPSHOT_MEDIA"
        const val ACTION_STOP = "com.truthraw.adaptiveui.v045.STOP"

        const val EXTRA_RUN_PROFILE = "run_profile"
        const val EXTRA_MARK_SOURCE = "mark_source"

        const val PROFILE_PHOTO = "PHOTO_MAIN_BASELINE_SESSION"
        const val PROFILE_PRO = "PRO_SWITCH_SESSION"
        const val PROFILE_HIRES_TELE = "HIRES_MAIN_TO_TELE_200MP_UI_SESSION"
        const val PROFILE_UNSPECIFIED = "UNSPECIFIED"

        const val MARK_WILL_OPEN_HONOR = "USER_WILL_OPEN_HONOR_CAMERA_MANUALLY"
        const val MARK_MAIN_STABLE = "MAIN_STABLE"
        const val MARK_PHOTO_MAIN_CONFIRMED = "PHOTO_MAIN_CONFIRMED_NO_MODE_SWITCH"
        const val MARK_PRO_SELECTED = "PRO_UI_SELECTED"
        const val MARK_HIRES_MAIN_CONFIRMED = "HIRES_MAIN_CONFIRMED"
        const val MARK_TELE_200MP_UI_SELECTED = "TELE_200MP_UI_SELECTED"
        const val MARK_SHUTTER_PRESSED = "SHUTTER_PRESSED"
        const val MARK_RETURNED = "USER_RETURNED_FROM_HONOR_CAMERA"

        const val REPORT_FILENAME = "TRUTHRAW_PASSIVE_HIRES_TELE_STATE_TIMELINE_v045.json"

        private const val CHANNEL_ID = "truthraw_v045_hires_tele_state"
        private const val NOTIFICATION_ID = 45045
        private const val DATE_TOLERANCE_MS = 5000L
        private const val STOP_DRAIN_MS = 1250L

        private val EXPORTED_EXIF_TAGS = arrayOf(
            "Make",
            "Model",
            "Software",
            "DateTimeOriginal",
            "OffsetTimeOriginal",
            "FNumber",
            "ExposureTime",
            "ISOSpeedRatings",
            "PhotographicSensitivity",
            "FocalLength",
            "FocalLengthIn35mmFilm",
            "DigitalZoomRatio",
            "LensMake",
            "LensModel",
            "LensSpecification",
            "WhiteBalance",
            "ExposureProgram",
            "MeteringMode",
            "Flash",
            "Orientation",
            "ImageWidth",
            "ImageLength",
            "PixelXDimension",
            "PixelYDimension",
        )

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
