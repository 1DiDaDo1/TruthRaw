package com.truthraw.adaptiveui

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.content.pm.PackageManager
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import android.os.Build
import android.os.Handler
import android.os.HandlerThread
import android.os.IBinder
import android.os.SystemClock
import androidx.core.app.NotificationCompat
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.security.MessageDigest
import java.time.Instant
import java.util.concurrent.atomic.AtomicBoolean

class HonorPassiveRouteObserverService : Service() {
    private val running = AtomicBoolean(false)
    private lateinit var workerThread: HandlerThread
    private lateinit var worker: Handler
    private lateinit var cameraManager: CameraManager
    private var accessoryBound = false
    private var accessoryConnection: ServiceConnection? = null

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
                JSONObject().put("logicalCameraId", cameraId).put("physicalCameraId", physicalCameraId),
            )
        }

        override fun onPhysicalCameraUnavailable(cameraId: String, physicalCameraId: String) {
            appendEvent(
                "PHYSICAL_CAMERA_UNAVAILABLE",
                JSONObject().put("logicalCameraId", cameraId).put("physicalCameraId", physicalCameraId),
            )
        }

        override fun onCameraAccessPrioritiesChanged() {
            appendEvent("CAMERA_ACCESS_PRIORITIES_CHANGED", JSONObject())
        }
    }

    override fun onCreate() {
        super.onCreate()
        workerThread = HandlerThread("truthraw-v040-honor-passive-route-observer").also { it.start() }
        worker = Handler(workerThread.looper)
        cameraManager = getSystemService(CameraManager::class.java)
        ensureNotificationChannel()
        startForeground(
            NOTIFICATION_ID,
            NotificationCompat.Builder(this, CHANNEL_ID)
                .setSmallIcon(android.R.drawable.ic_menu_camera)
                .setContentTitle("TruthRaw v0.40 passive observer")
                .setContentText("Observeert Camera2 availability; opent of bestuurt geen camera.")
                .setOngoing(true)
                .setPriority(NotificationCompat.PRIORITY_LOW)
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
            ACTION_PROBE_ACCESSORY -> worker.post { probeAccessoriseService() }
            ACTION_STOP -> worker.post {
                appendEvent("OBSERVER_STOP_REQUESTED", JSONObject())
                stopSelf()
            }
        }
        return START_STICKY
    }

    override fun onDestroy() {
        runCatching { cameraManager.unregisterAvailabilityCallback(availability) }
        if (accessoryBound && accessoryConnection != null) {
            runCatching { unbindService(accessoryConnection!!) }
        }
        appendEvent("OBSERVER_DESTROYED", JSONObject())
        running.set(false)
        workerThread.quitSafely()
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    private fun startObservation() {
        if (!running.compareAndSet(false, true)) return

        val report = JSONObject()
            .put("schema", "truthraw.honor-passive-route-observer.v0.40")
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", "PASSIVE_ROUTE_OBSERVATION_ONLY")
            .put("scientificMasterModified", false)
            .put("captureEvidenceGranted", false)
            .put("honorApkSemanticsGranted", false)
            .put("cameraOpenedByTruthRaw", false)
            .put("captureSubmittedByTruthRaw", false)
            .put("imageBufferAccessedByTruthRaw", false)
            .put("vendorRequestWrittenByTruthRaw", false)
            .put("device", buildDeviceSnapshot())
            .put("honorCameraPackage", buildHonorPackageSnapshot())
            .put("cameraSnapshotBeforeObservation", buildCameraSnapshot())
            .put("events", JSONArray())

        REPORT_FILE.writeText(report.toString(2))
        appendEvent(
            "OBSERVER_STARTED",
            JSONObject()
                .put("sdkInt", Build.VERSION.SDK_INT)
                .put("targetSdk", applicationInfo.targetSdkVersion)
                .put("observerMode", "ANDROID16_CAMERA_AVAILABILITY_AND_PHYSICAL_ROUTE_TIMELINE"),
        )

        runCatching {
            cameraManager.registerAvailabilityCallback(mainExecutor, availability)
        }.onFailure {
            appendEvent(
                "REGISTER_AVAILABILITY_CALLBACK_FAILED",
                JSONObject().put("error", "${it.javaClass.simpleName}: ${it.message}"),
            )
        }
    }

    private fun buildDeviceSnapshot(): JSONObject =
        JSONObject()
            .put("manufacturer", Build.MANUFACTURER)
            .put("brand", Build.BRAND)
            .put("model", Build.MODEL)
            .put("device", Build.DEVICE)
            .put("product", Build.PRODUCT)
            .put("sdkInt", Build.VERSION.SDK_INT)
            .put("release", Build.VERSION.RELEASE)
            .put("securityPatch", Build.VERSION.SECURITY_PATCH)
            .put("fingerprint", Build.FINGERPRINT)
            .put("truthRawTargetSdk", applicationInfo.targetSdkVersion)
            .put("sharedCameraApi37Used", false)
            .put("interpretation", "Android-16 runtime unless sdkInt says otherwise; Honor APK/runtime state kept separate.")

    private fun buildHonorPackageSnapshot(): JSONObject {
        val pm = packageManager
        val out = JSONObject()
            .put("packageName", HONOR_PACKAGE)
            .put("source", "INSTALLED_PACKAGE_MANAGER_STATE_ONLY")
        return runCatching {
            val info = pm.getPackageInfo(
                HONOR_PACKAGE,
                PackageManager.PackageInfoFlags.of(PackageManager.GET_SIGNING_CERTIFICATES.toLong()),
            )
            val app = info.applicationInfo
            val sigs = info.signingInfo?.apkContentsSigners.orEmpty()
            val sigHashes = JSONArray()
            sigs.forEach { sig ->
                val md = MessageDigest.getInstance("SHA-256")
                sigHashes.put(md.digest(sig.toByteArray()).joinToString("") { "%02x".format(it) })
            }
            out.put("installed", true)
                .put("versionName", info.versionName ?: JSONObject.NULL)
                .put("longVersionCode", info.longVersionCode)
                .put("targetSdkVersion", app?.targetSdkVersion ?: JSONObject.NULL)
                .put("minSdkVersion", app?.minSdkVersion ?: JSONObject.NULL)
                .put("compileSdkVersion", if (Build.VERSION.SDK_INT >= 31) app?.compileSdkVersion ?: JSONObject.NULL else JSONObject.NULL)
                .put("compileSdkVersionCodename", if (Build.VERSION.SDK_INT >= 31) app?.compileSdkVersionCodename ?: JSONObject.NULL else JSONObject.NULL)
                .put("systemApp", app?.let { (it.flags and android.content.pm.ApplicationInfo.FLAG_SYSTEM) != 0 } ?: false)
                .put("updatedSystemApp", app?.let { (it.flags and android.content.pm.ApplicationInfo.FLAG_UPDATED_SYSTEM_APP) != 0 } ?: false)
                .put("signingCertificateSha256", sigHashes)
        }.getOrElse {
            out.put("installed", false)
                .put("error", "${it.javaClass.simpleName}: ${it.message}")
        }
    }

    private fun buildCameraSnapshot(): JSONObject {
        val out = JSONObject()
        val ids = runCatching { cameraManager.cameraIdList.toList() }.getOrElse {
            return out.put("error", "${it.javaClass.simpleName}: ${it.message}")
        }
        out.put("cameraIdList", JSONArray(ids))

        val cameras = JSONObject()
        ids.forEach { id ->
            cameras.put(id, buildCameraCharacteristicsSnapshot(id))
        }
        // Physical Camera 5 can be queryable even if not returned as a top-level openable ID.
        if (!cameras.has("5")) {
            runCatching { cameras.put("5", buildCameraCharacteristicsSnapshot("5")) }
        }
        out.put("cameras", cameras)
        return out
    }

    @Suppress("UNCHECKED_CAST")
    private fun buildCameraCharacteristicsSnapshot(cameraId: String): JSONObject {
        val c = cameraManager.getCameraCharacteristics(cameraId)
        val out = JSONObject()
            .put("cameraId", cameraId)
            .put("physicalCameraIds", JSONArray(c.physicalCameraIds.sorted()))

        val standard = c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
        val maximum = c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)
        fun sizes(a: Array<android.util.Size>?): JSONArray =
            JSONArray(a.orEmpty().map { "${it.width}x${it.height}" })

        out.put("rawStandardOutputs", runCatching {
            sizes(standard?.getOutputSizes(android.graphics.ImageFormat.RAW_SENSOR))
        }.getOrElse { JSONArray() })
        out.put("rawMaximumOutputs", runCatching {
            sizes(maximum?.getOutputSizes(android.graphics.ImageFormat.RAW_SENSOR))
        }.getOrElse { JSONArray() })
        out.put("rawMaximumHighResolutionOutputs", runCatching {
            sizes(maximum?.getHighResolutionOutputSizes(android.graphics.ImageFormat.RAW_SENSOR))
        }.getOrElse { JSONArray() })

        val terms = listOf("raw", "remosaic", "sensorzoom", "insensor", "tele", "capturestreamresolution", "binning")
        val vendor = JSONArray()
        c.keys.orEmpty().forEach { key ->
            val name = key.name
            if (terms.any { name.lowercase().contains(it) }) {
                val value = runCatching {
                    c.get(key as CameraCharacteristics.Key<Any>)
                }.getOrNull()
                vendor.put(
                    JSONObject()
                        .put("name", name)
                        .put("valueString", valueToString(value)),
                )
            }
        }
        out.put("filteredVendorCharacteristics", vendor)
        out.put("availableSessionKeys", JSONArray(c.availableSessionKeys.orEmpty().map { it.name }))
        out.put("availableCaptureRequestKeysFiltered", JSONArray(
            c.availableCaptureRequestKeys.orEmpty()
                .map { it.name }
                .filter { n -> terms.any { n.lowercase().contains(it) } }
        ))
        return out
    }

    private fun valueToString(v: Any?): Any =
        when (v) {
            null -> JSONObject.NULL
            is IntArray -> JSONArray(v.toList())
            is LongArray -> JSONArray(v.toList())
            is FloatArray -> JSONArray(v.toList())
            is DoubleArray -> JSONArray(v.toList())
            is ByteArray -> JSONArray(v.map { it.toInt() and 0xff })
            is Array<*> -> JSONArray(v.map { it?.toString() })
            else -> v.toString()
        }

    private fun probeAccessoriseService() {
        appendEvent(
            "ACCESSORISE_BIND_PROBE_START",
            JSONObject()
                .put("component", HONOR_ACCESSORY_COMPONENT.flattenToString())
                .put("action", HONOR_ACCESSORY_ACTION)
                .put("methodCallsPlanned", 0)
                .put("allowlistBypassAttempted", false),
        )

        if (accessoryBound) {
            appendEvent("ACCESSORISE_BIND_PROBE_ALREADY_BOUND", JSONObject())
            return
        }

        val connection = object : ServiceConnection {
            override fun onServiceConnected(name: ComponentName, service: IBinder) {
                accessoryBound = true
                val descriptor = runCatching { service.interfaceDescriptor }.getOrNull()
                appendEvent(
                    "ACCESSORISE_SERVICE_CONNECTED",
                    JSONObject()
                        .put("component", name.flattenToString())
                        .put("binderAlive", service.isBinderAlive)
                        .put("binderPing", service.pingBinder())
                        .put("interfaceDescriptor", descriptor ?: JSONObject.NULL)
                        .put("aidlMethodInvoked", false)
                        .put("semanticAuthorityGranted", false),
                )
            }

            override fun onServiceDisconnected(name: ComponentName) {
                accessoryBound = false
                appendEvent(
                    "ACCESSORISE_SERVICE_DISCONNECTED",
                    JSONObject().put("component", name.flattenToString()),
                )
            }

            override fun onNullBinding(name: ComponentName) {
                appendEvent(
                    "ACCESSORISE_SERVICE_NULL_BINDING",
                    JSONObject().put("component", name.flattenToString()),
                )
            }

            override fun onBindingDied(name: ComponentName) {
                accessoryBound = false
                appendEvent(
                    "ACCESSORISE_SERVICE_BINDING_DIED",
                    JSONObject().put("component", name.flattenToString()),
                )
            }
        }
        accessoryConnection = connection

        val intent = Intent(HONOR_ACCESSORY_ACTION).setComponent(HONOR_ACCESSORY_COMPONENT)
        runCatching {
            bindService(intent, connection, Context.BIND_AUTO_CREATE)
        }.onSuccess { accepted ->
            appendEvent(
                "ACCESSORISE_BIND_REQUEST_RESULT",
                JSONObject().put("bindServiceReturned", accepted),
            )
        }.onFailure {
            appendEvent(
                "ACCESSORISE_BIND_REQUEST_EXCEPTION",
                JSONObject()
                    .put("error", "${it.javaClass.simpleName}: ${it.message}")
                    .put("allowlistBypassAttempted", false),
            )
        }
    }

    @Synchronized
    private fun appendEvent(type: String, payload: JSONObject) {
        val now = SystemClock.elapsedRealtimeNanos()
        val report = runCatching { JSONObject(REPORT_FILE.readText()) }.getOrElse {
            JSONObject()
                .put("schema", "truthraw.honor-passive-route-observer.v0.40")
                .put("events", JSONArray())
        }
        val events = report.optJSONArray("events") ?: JSONArray().also { report.put("events", it) }
        events.put(
            JSONObject()
                .put("sequence", events.length())
                .put("type", type)
                .put("utc", Instant.now().toString())
                .put("elapsedRealtimeNs", now)
                .put("payload", payload),
        )
        report.put("lastUpdatedAtUtc", Instant.now().toString())
        REPORT_FILE.writeText(report.toString(2))
    }

    private fun ensureNotificationChannel() {
        val nm = getSystemService(NotificationManager::class.java)
        nm.createNotificationChannel(
            NotificationChannel(
                CHANNEL_ID,
                "TruthRaw passive camera observer",
                NotificationManager.IMPORTANCE_LOW,
            ),
        )
    }

    private val REPORT_FILE: File
        get() = File(filesDir, REPORT_FILENAME)

    companion object {
        const val ACTION_MARK = "com.truthraw.adaptiveui.v040.MARK"
        const val ACTION_PROBE_ACCESSORY = "com.truthraw.adaptiveui.v040.PROBE_ACCESSORY"
        const val ACTION_STOP = "com.truthraw.adaptiveui.v040.STOP"
        const val EXTRA_LABEL = "label"
        const val EXTRA_NOTE = "note"
        const val REPORT_FILENAME = "TRUTHRAW_HONOR_PASSIVE_ROUTE_OBSERVER_v040.json"

        const val HONOR_PACKAGE = "com.hihonor.camera"
        const val HONOR_ACCESSORY_ACTION = "com.hihonor.camera.aidl.CameraAccessoriseService"
        val HONOR_ACCESSORY_COMPONENT = ComponentName(
            HONOR_PACKAGE,
            "com.hihonor.camera.accessorise.aidl.CameraAccessoriseService",
        )

        private const val CHANNEL_ID = "truthraw_v040_passive_observer"
        private const val NOTIFICATION_ID = 40040
    }
}
