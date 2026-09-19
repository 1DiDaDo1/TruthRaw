package com.truthraw.adaptiveui

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.ContentResolver
import android.content.Intent
import android.database.ContentObserver
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
import java.io.FileInputStream
import java.time.Instant
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicInteger

class PassiveHonorProRawDngFingerprintService : Service() {
    private lateinit var workerThread: HandlerThread
    private lateinit var worker: Handler
    private lateinit var observer: ContentObserver

    private val running = AtomicBoolean(false)
    private val changeCounter = AtomicInteger(0)
    private val fingerprinted = mutableSetOf<String>()

    private var observerStartEpochMs = 0L
    private var observerStartElapsedNs = 0L
    private var runProfile = PROFILE_UNSPECIFIED
    private var dngFingerprintCount = 0

    private val imageCollectionUri: Uri
        get() = MediaStore.Images.Media.getContentUri(MediaStore.VOLUME_EXTERNAL)

    override fun onCreate() {
        super.onCreate()
        workerThread = HandlerThread("truthraw-v054-oem-pro-raw-dng").also { it.start() }
        worker = Handler(workerThread.looper)
        ensureNotificationChannel()
        startForeground(NOTIFICATION_ID, buildNotification("Wacht op runprofiel."))
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_START -> worker.post {
                startObservation(intent.getStringExtra(EXTRA_RUN_PROFILE) ?: PROFILE_UNSPECIFIED)
            }
            ACTION_MARK_SHUTTER -> worker.post {
                appendEvent("USER_MARK", JSONObject()
                    .put("label", "SHUTTER_PRESSED")
                    .put("hardwareTimestamp", false)
                    .put("userDeclaredTimingOnly", true))
            }
            ACTION_MARK_RETURNED -> worker.post {
                appendEvent("USER_MARK", JSONObject()
                    .put("label", "RETURNED_FROM_HONOR_CAMERA")
                    .put("hardwareTimestamp", false)
                    .put("userDeclaredTimingOnly", true))
                snapshot("RETURN_SNAPSHOT", imageCollectionUri, null, null)
            }
            ACTION_SNAPSHOT -> worker.post {
                snapshot("USER_REQUESTED_SNAPSHOT", imageCollectionUri, null, null)
            }
            ACTION_STOP -> worker.post {
                appendEvent("OBSERVER_STOP_REQUESTED", JSONObject().put("drainDelayMs", STOP_DRAIN_MS))
                worker.postDelayed({ stopSelf() }, STOP_DRAIN_MS)
            }
        }
        return START_STICKY
    }

    override fun onDestroy() {
        if (::observer.isInitialized) runCatching { contentResolver.unregisterContentObserver(observer) }
        appendEvent("OBSERVER_DESTROYED", JSONObject()
            .put("runProfile", runProfile)
            .put("dngFingerprintCount", dngFingerprintCount)
            .put("cameraOpenedByTruthRaw", false)
            .put("captureSubmittedByTruthRaw", false)
            .put("imageSamplesDecoded", false)
            .put("pixelPayloadBytesRead", false)
            .put("honorBinderInvokedByTruthRaw", false))
        running.set(false)
        workerThread.quitSafely()
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    private fun startObservation(profile: String) {
        if (!running.compareAndSet(false, true)) {
            appendEvent("START_IGNORED_ALREADY_RUNNING", JSONObject()
                .put("currentProfile", runProfile)
                .put("requestedProfile", profile))
            return
        }

        runProfile = when (profile) {
            PROFILE_PRO_MAIN_RAW -> PROFILE_PRO_MAIN_RAW
            PROFILE_PRO_TELE_RAW -> PROFILE_PRO_TELE_RAW
            else -> PROFILE_UNSPECIFIED
        }

        observerStartEpochMs = System.currentTimeMillis()
        observerStartElapsedNs = SystemClock.elapsedRealtimeNanos()

        val report = JSONObject()
            .put("schema", "truthraw.passive-oem-pro-raw-dng-container-fingerprint.v0.54")
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", "PASSIVE_EXPORTED_OEM_DNG_CONTAINER_METADATA_ONLY")
            .put("runProfile", runProfile)
            .put("runProfileIsUserDeclared", true)
            .put("captureEvidenceGranted", false)
            .put("calibrationAuthorityGranted", false)
            .put("scientificMasterModified", false)
            .put("cameraOpenedByTruthRaw", false)
            .put("captureSubmittedByTruthRaw", false)
            .put("imageSamplesDecoded", false)
            .put("pixelPayloadBytesRead", false)
            .put("bitmapDecoded", false)
            .put("gpsIfdFollowed", false)
            .put("vendorRequestWrittenByTruthRaw", false)
            .put("honorBinderInvokedByTruthRaw", false)
            .put("observerStartEpochMs", observerStartEpochMs)
            .put("observerStartElapsedRealtimeNs", observerStartElapsedNs)
            .put("device", JSONObject()
                .put("manufacturer", Build.MANUFACTURER)
                .put("model", Build.MODEL)
                .put("sdkInt", Build.VERSION.SDK_INT)
                .put("release", Build.VERSION.RELEASE)
                .put("fingerprint", Build.FINGERPRINT)
                .put("truthRawTargetSdk", applicationInfo.targetSdkVersion))
            .put("honorCameraPackage", honorCameraPackageSnapshot())
            .put("parserBoundary", JSONObject()
                .put("classicTiffIfdMetadataOnly", true)
                .put("stripTileOffsetArraysMayBeReadAsMetadata", true)
                .put("stripTilePayloadOffsetsNeverFollowed", true)
                .put("imageSampleDecode", false)
                .put("semanticPromotionAllowed", false))
            .put("events", JSONArray())

        reportFile().writeText(report.toString(2))

        observer = object : ContentObserver(worker) {
            override fun onChange(selfChange: Boolean) = handleChange(selfChange, null)
            override fun onChange(selfChange: Boolean, uri: Uri?) = handleChange(selfChange, uri)
            override fun onChange(selfChange: Boolean, uri: Uri?, flags: Int) = handleChange(selfChange, uri)
        }

        runCatching {
            contentResolver.registerContentObserver(imageCollectionUri, true, observer)
        }.onSuccess {
            appendEvent("OBSERVER_STARTED", JSONObject()
                .put("runProfile", runProfile)
                .put("collectionUri", imageCollectionUri.toString()))
            snapshot("BASELINE_SNAPSHOT", imageCollectionUri, null, null)
            getSystemService(NotificationManager::class.java).notify(
                NOTIFICATION_ID,
                buildNotification("Actief: $runProfile · maak één Honor Pro RAW/DNG."),
            )
        }.onFailure {
            appendEvent("OBSERVER_REGISTRATION_FAILED", errorJson(it))
        }
    }

    private fun handleChange(selfChange: Boolean, uri: Uri?) {
        val group = changeCounter.incrementAndGet()
        val triggerNs = SystemClock.elapsedRealtimeNanos()
        appendEvent("MEDIASTORE_CHANGE", JSONObject()
            .put("group", group)
            .put("selfChange", selfChange)
            .put("uri", (uri ?: imageCollectionUri).toString())
            .put("triggerElapsedRealtimeNs", triggerNs))

        for (delay in SNAPSHOT_DELAYS_MS) {
            worker.postDelayed({ snapshot("DELAYED_SNAPSHOT", uri ?: imageCollectionUri, group, delay) }, delay)
        }
    }

    private fun snapshot(type: String, requestedUri: Uri, group: Int?, delayMs: Long?) {
        val rows = queryRows(requestedUri)
        val payload = JSONObject()
            .put("requestedUri", requestedUri.toString())
            .put("rows", rows)
            .put("metadataOnly", true)
            .put("imageSamplesDecoded", false)
        if (group != null) payload.put("changeGroup", group)
        if (delayMs != null) payload.put("scheduledDelayMs", delayMs)
        appendEvent(type, payload)

        for (i in 0 until rows.length()) maybeFingerprintDng(rows.optJSONObject(i) ?: continue)
    }

    private fun queryRows(requestedUri: Uri): JSONArray {
        val rows = JSONArray()
        val numericItem = requestedUri.lastPathSegment?.toLongOrNull() != null
        val queryUri = if (numericItem) requestedUri else imageCollectionUri
        val args = Bundle().apply {
            putStringArray(ContentResolver.QUERY_ARG_SORT_COLUMNS,
                arrayOf(MediaStore.MediaColumns.DATE_ADDED, BaseColumns._ID))
            putInt(ContentResolver.QUERY_ARG_SORT_DIRECTION, ContentResolver.QUERY_SORT_DIRECTION_DESCENDING)
            putInt(ContentResolver.QUERY_ARG_LIMIT, if (numericItem) 1 else 12)
        }

        runCatching {
            contentResolver.query(queryUri, PROJECTION, args, null)?.use { cursor ->
                val indices = PROJECTION.associateWith { cursor.getColumnIndex(it) }
                while (cursor.moveToNext()) {
                    val row = JSONObject()
                    for (col in PROJECTION) {
                        val idx = indices[col] ?: -1
                        if (idx < 0 || cursor.isNull(idx)) {
                            row.put(col, JSONObject.NULL)
                        } else {
                            when (cursor.getType(idx)) {
                                android.database.Cursor.FIELD_TYPE_INTEGER -> row.put(col, cursor.getLong(idx))
                                android.database.Cursor.FIELD_TYPE_FLOAT -> row.put(col, cursor.getDouble(idx))
                                else -> row.put(col, cursor.getString(idx))
                            }
                        }
                    }
                    val id = row.optLong(BaseColumns._ID, -1L)
                    val contentUri = if (id >= 0) Uri.withAppendedPath(imageCollectionUri, id.toString()) else null
                    row.put("contentUri", contentUri?.toString() ?: JSONObject.NULL)

                    val name = row.optString(MediaStore.MediaColumns.DISPLAY_NAME, "")
                    val mime = row.optString(MediaStore.MediaColumns.MIME_TYPE, "")
                    val dateAdded = row.optLong(MediaStore.MediaColumns.DATE_ADDED, -1L)
                    row.put("isDngCandidate", name.endsWith(".dng", true) || mime.contains("dng", true))
                    row.put("createdAfterObserverStart",
                        dateAdded > 0 && dateAdded * 1000L >= observerStartEpochMs - DATE_TOLERANCE_MS)
                    rows.put(row)
                }
            }
        }.onFailure { rows.put(JSONObject().put("queryError", errorJson(it))) }
        return rows
    }

    private fun maybeFingerprintDng(row: JSONObject) {
        if (!row.optBoolean("isDngCandidate", false)) return
        if (!row.optBoolean("createdAfterObserverStart", false)) return

        val size = row.optLong(MediaStore.MediaColumns.SIZE, -1L)
        val pending = if (row.isNull(MediaStore.MediaColumns.IS_PENDING)) 0L
        else row.optLong(MediaStore.MediaColumns.IS_PENDING, 0L)
        if (size <= 0L || pending != 0L) return

        val uriText = row.optString("contentUri", "")
        if (uriText.isBlank()) return
        val modified = row.optLong(MediaStore.MediaColumns.DATE_MODIFIED, -1L)
        val key = "$uriText|$size|$modified"
        if (!fingerprinted.add(key)) return

        val uri = Uri.parse(uriText)
        val startNs = SystemClock.elapsedRealtimeNanos()
        var fdOpened = false

        val payload = JSONObject()
            .put("authority", "EXPORTED_OEM_DNG_CONTAINER_METADATA_ONLY")
            .put("runProfile", runProfile)
            .put("sourceUri", uriText)
            .put("displayName", row.optString(MediaStore.MediaColumns.DISPLAY_NAME, ""))
            .put("mimeType", row.optString(MediaStore.MediaColumns.MIME_TYPE, ""))
            .put("mediaStoreWidth", if (row.isNull(MediaStore.MediaColumns.WIDTH)) JSONObject.NULL else row.optLong(MediaStore.MediaColumns.WIDTH))
            .put("mediaStoreHeight", if (row.isNull(MediaStore.MediaColumns.HEIGHT)) JSONObject.NULL else row.optLong(MediaStore.MediaColumns.HEIGHT))
            .put("mediaStoreSizeBytes", size)
            .put("relativePath", row.optString(MediaStore.MediaColumns.RELATIVE_PATH, ""))
            .put("imageSamplesDecoded", false)
            .put("pixelPayloadBytesRead", false)
            .put("gpsIfdFollowed", false)
            .put("captureEvidenceGranted", false)
            .put("semanticPromotionAllowed", false)

        runCatching {
            contentResolver.openFileDescriptor(uri, "r")?.use { pfd ->
                fdOpened = true
                FileInputStream(pfd.fileDescriptor).channel.use { channel ->
                    val fileSize = runCatching { channel.size() }.getOrDefault(size)
                    payload.put("container", DngContainerMetadataParser.parse(channel, fileSize))
                }
            } ?: error("openFileDescriptor returned null")
        }.onSuccess {
            dngFingerprintCount++
            payload.put("success", true)
        }.onFailure {
            payload.put("success", false).put("error", errorJson(it))
        }

        payload.put("fileDescriptorOpenedReadOnly", fdOpened)
        payload.put("parseStartElapsedRealtimeNs", startNs)
        payload.put("parseEndElapsedRealtimeNs", SystemClock.elapsedRealtimeNanos())
        appendEvent("EXPORTED_DNG_CONTAINER_FINGERPRINT", payload)
    }

    @Suppress("DEPRECATION")
    private fun honorCameraPackageSnapshot(): JSONObject =
        runCatching {
            val info = packageManager.getPackageInfo("com.hihonor.camera", 0)
            val app = info.applicationInfo
            JSONObject()
                .put("installed", true)
                .put("versionName", info.versionName ?: JSONObject.NULL)
                .put("longVersionCode", info.longVersionCode)
                .put("targetSdkVersion", app?.targetSdkVersion ?: JSONObject.NULL)
                .put("compileSdkVersion", app?.compileSdkVersion ?: JSONObject.NULL)
                .put("systemApp", app?.let { (it.flags and android.content.pm.ApplicationInfo.FLAG_SYSTEM) != 0 } ?: false)
        }.getOrElse { JSONObject().put("installed", false).put("error", errorJson(it)) }

    private fun appendEvent(type: String, payload: JSONObject) {
        val file = reportFile()
        if (!file.exists()) return
        runCatching {
            val report = JSONObject(file.readText())
            report.getJSONArray("events").put(
                JSONObject()
                    .put("type", type)
                    .put("atUtc", Instant.now().toString())
                    .put("elapsedRealtimeNs", SystemClock.elapsedRealtimeNanos())
                    .put("payload", payload)
            )
            file.writeText(report.toString(2))
        }
    }

    private fun errorJson(e: Throwable): JSONObject = JSONObject()
        .put("class", e.javaClass.name)
        .put("message", e.message ?: JSONObject.NULL)
        .put("causeClass", e.cause?.javaClass?.name ?: JSONObject.NULL)
        .put("causeMessage", e.cause?.message ?: JSONObject.NULL)

    private fun reportFile(): File = File(filesDir, REPORT_FILENAME)

    private fun ensureNotificationChannel() {
        getSystemService(NotificationManager::class.java).createNotificationChannel(
            NotificationChannel(CHANNEL_ID, "TruthRaw v0.54 Pro RAW observer", NotificationManager.IMPORTANCE_LOW)
        )
    }

    private fun buildNotification(text: String): Notification {
        val pending = PendingIntent.getActivity(
            this, 0, Intent(this, PassiveHonorProRawDngFingerprintActivity::class.java),
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
        )
        return Notification.Builder(this, CHANNEL_ID)
            .setSmallIcon(android.R.drawable.ic_menu_camera)
            .setContentTitle("TruthRaw v0.54 · Honor Pro RAW")
            .setContentText(text)
            .setContentIntent(pending)
            .setOngoing(true)
            .build()
    }

    companion object {
        const val REPORT_FILENAME = "TRUTHRAW_HONOR_PRO_RAW_DNG_FINGERPRINT_v054.json"
        const val ACTION_START = "truthraw.v054.START"
        const val ACTION_MARK_SHUTTER = "truthraw.v054.MARK_SHUTTER"
        const val ACTION_MARK_RETURNED = "truthraw.v054.MARK_RETURNED"
        const val ACTION_SNAPSHOT = "truthraw.v054.SNAPSHOT"
        const val ACTION_STOP = "truthraw.v054.STOP"
        const val EXTRA_RUN_PROFILE = "run_profile"
        const val PROFILE_PRO_MAIN_RAW = "PRO_MAIN_RAW"
        const val PROFILE_PRO_TELE_RAW = "PRO_TELE_RAW"
        const val PROFILE_UNSPECIFIED = "UNSPECIFIED"

        private const val CHANNEL_ID = "truthraw_v054_pro_raw"
        private const val NOTIFICATION_ID = 65400
        private const val STOP_DRAIN_MS = 4500L
        private const val DATE_TOLERANCE_MS = 2000L
        private val SNAPSHOT_DELAYS_MS = longArrayOf(0L, 100L, 500L, 1500L, 4000L)

        private val PROJECTION = arrayOf(
            BaseColumns._ID,
            MediaStore.MediaColumns.DISPLAY_NAME,
            MediaStore.MediaColumns.MIME_TYPE,
            MediaStore.MediaColumns.WIDTH,
            MediaStore.MediaColumns.HEIGHT,
            MediaStore.MediaColumns.SIZE,
            MediaStore.MediaColumns.DATE_ADDED,
            MediaStore.MediaColumns.DATE_MODIFIED,
            MediaStore.MediaColumns.RELATIVE_PATH,
            MediaStore.MediaColumns.IS_PENDING,
        )
    }
}
