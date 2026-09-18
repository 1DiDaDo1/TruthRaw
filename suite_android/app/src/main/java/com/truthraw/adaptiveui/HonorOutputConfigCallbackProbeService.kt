package com.truthraw.adaptiveui

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.hardware.camera2.CameraManager
import android.os.Build
import android.os.Handler
import android.os.HandlerThread
import android.os.IBinder
import android.os.SystemClock
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.time.Instant
import java.util.concurrent.atomic.AtomicBoolean

/**
 * v0.41 single-boundary probe:
 *
 * 1. passive Camera2 availability observation;
 * 2. bind to Honor CameraAccessoriseService;
 * 3. invoke ONLY registerOutputConfigCallback (service transaction 3);
 * 4. stop immediately at that authorization boundary:
 *    - reject => record and do not invoke any other Honor registration method;
 *    - success => keep only this callback alive and observe values while the user operates Honor Camera.
 *
 * No Surface, no camera open, no capture request, no ImageReader, no vendor key.
 */
class HonorOutputConfigCallbackProbeService : Service() {
    private lateinit var workerThread: HandlerThread
    private lateinit var worker: Handler
    private lateinit var cameraManager: CameraManager

    private val running = AtomicBoolean(false)
    private var honorService: IBinder? = null
    private var honorConnection: ServiceConnection? = null
    private var callbackBinder: HonorAccessoriseOutputConfigBinderProtocol.OutputConfigCallbackBinder? = null
    private var registrationAttempted = false
    private var callbackRegistered = false

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
        workerThread = HandlerThread("truthraw-v041-output-config-probe").also { it.start() }
        worker = Handler(workerThread.looper)
        cameraManager = getSystemService(CameraManager::class.java)
        ensureNotificationChannel()
        startForeground(
            NOTIFICATION_ID,
            Notification.Builder(this, CHANNEL_ID)
                .setSmallIcon(android.R.drawable.ic_menu_camera)
                .setContentTitle("TruthRaw v0.41 callback probe")
                .setContentText("Honor output-config callback only; TruthRaw opent geen camera.")
                .setOngoing(true)
                .setCategory(Notification.CATEGORY_SERVICE)
                .build(),
        )
        worker.post { startObservation() }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_PROBE_OUTPUT_CONFIG -> worker.post { beginOutputConfigProbe() }
            ACTION_MARK -> worker.post {
                appendEvent(
                    "USER_MARK",
                    JSONObject()
                        .put("label", intent.getStringExtra(EXTRA_LABEL) ?: "unspecified")
                        .put("note", intent.getStringExtra(EXTRA_NOTE) ?: JSONObject.NULL),
                )
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

        val service = honorService
        val callback = callbackBinder
        if (callbackRegistered && service != null && callback != null) {
            val unreg = HonorAccessoriseOutputConfigBinderProtocol.unregisterOutputConfigCallback(
                service,
                callback,
            )
            appendEvent(
                "HONOR_OUTPUT_CONFIG_CALLBACK_UNREGISTER_RESULT",
                unreg.toJson()
                    .put(
                        "serviceTransactionCode",
                        HonorAccessoriseOutputConfigBinderProtocol
                            .SERVICE_TX_UNREGISTER_OUTPUT_CONFIG_CALLBACK,
                    )
                    .put("invokedOnlyBecauseRegistrationSucceeded", true),
            )
        }

        val connection = honorConnection
        if (connection != null) {
            runCatching { unbindService(connection) }
        }

        appendEvent(
            "OBSERVER_DESTROYED",
            JSONObject()
                .put("registrationAttempted", registrationAttempted)
                .put("callbackRegistered", callbackRegistered),
        )

        callbackRegistered = false
        honorService = null
        callbackBinder = null
        honorConnection = null
        running.set(false)
        workerThread.quitSafely()
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    private fun startObservation() {
        if (!running.compareAndSet(false, true)) return

        val report = JSONObject()
            .put("schema", "truthraw.honor-output-config-callback-probe.v0.41")
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", "PASSIVE_SOFTWARE_ROUTE_OBSERVATION_ONLY")
            .put("scientificMasterModified", false)
            .put("captureEvidenceGranted", false)
            .put("honorCallbackSemanticAuthorityGranted", false)
            .put("cameraOpenedByTruthRaw", false)
            .put("captureSubmittedByTruthRaw", false)
            .put("imageBufferAccessedByTruthRaw", false)
            .put("vendorRequestWrittenByTruthRaw", false)
            .put("previewSurfaceProvidedByTruthRaw", false)
            .put("exitPreviewInvokedByTruthRaw", false)
            .put("captureEventCallbackRegistrationInvoked", false)
            .put("previewStateCallbackRegistrationInvoked", false)
            .put(
                "singleMethodBoundary",
                JSONObject()
                    .put(
                        "serviceDescriptor",
                        HonorAccessoriseOutputConfigBinderProtocol.SERVICE_DESCRIPTOR,
                    )
                    .put(
                        "method",
                        "registerOutputConfigCallback(IOutputConfigCallback)",
                    )
                    .put(
                        "transactionCode",
                        HonorAccessoriseOutputConfigBinderProtocol
                            .SERVICE_TX_REGISTER_OUTPUT_CONFIG_CALLBACK,
                    )
                    .put("otherHonorServiceMethodsInvokedBeforeResultReview", false)
                    .put("allowlistBypassAttempted", false)
                    .put("packageOrSignatureSpoofingAttempted", false),
            )
            .put(
                "staticApkBoundary",
                JSONObject()
                    .put("sourceApkSha256", HONOR_APK_SHA256)
                    .put("registrationMethodsShareAuthorizationGate", true)
                    .put("expectedGateMarker", "enforceCallingPackage / client_not_allowed")
                    .put("staticAllowedPackages", JSONArray(listOf("com.huamei.badge", "com.hihonor.camera")))
                    .put(
                        "staticAllowedCertificateSha256",
                        JSONArray(
                            listOf(
                                "0EA6FCE70AB2A77DB537318F45FC12DEFC0D95C4EC8946150FB2E40567CD5D3E",
                            ),
                        ),
                    )
                    .put("staticPredictionIsNotDeviceResult", true),
            )
            .put("deviceRuntimeSdkInt", Build.VERSION.SDK_INT)
            .put("deviceRuntimeRelease", Build.VERSION.RELEASE)
            .put("truthRawTargetSdk", applicationInfo.targetSdkVersion)
            .put("events", JSONArray())

        reportFile.writeText(report.toString(2))
        appendEvent(
            "OBSERVER_STARTED",
            JSONObject()
                .put("sdkInt", Build.VERSION.SDK_INT)
                .put("targetSdk", applicationInfo.targetSdkVersion)
                .put("observerMode", "ANDROID16_SINGLE_OUTPUT_CONFIG_CALLBACK_BOUNDARY_PROBE"),
        )

        runCatching {
            cameraManager.registerAvailabilityCallback(mainExecutor, availability)
        }.onFailure {
            appendEvent(
                "REGISTER_AVAILABILITY_CALLBACK_FAILED",
                JSONObject().put("error", "${it.javaClass.name}: ${it.message}"),
            )
        }
    }

    private fun beginOutputConfigProbe() {
        if (registrationAttempted) {
            appendEvent(
                "HONOR_OUTPUT_CONFIG_REGISTRATION_PROBE_NOT_REPEATED",
                JSONObject()
                    .put("reason", "single-boundary probe already attempted")
                    .put("callbackRegistered", callbackRegistered),
            )
            return
        }

        appendEvent(
            "HONOR_OUTPUT_CONFIG_REGISTRATION_PROBE_START",
            JSONObject()
                .put("serviceTransactionCode", 3)
                .put("callbackDescriptor", HonorAccessoriseOutputConfigBinderProtocol.OUTPUT_CALLBACK_DESCRIPTOR)
                .put("setPreviewSurfaceInvoked", false)
                .put("exitPreviewInvoked", false)
                .put("otherCallbackRegistrationsInvoked", false)
                .put("allowlistBypassAttempted", false),
        )

        if (honorService?.isBinderAlive == true) {
            attemptRegistration(honorService!!)
            return
        }

        val connection = object : ServiceConnection {
            override fun onServiceConnected(name: ComponentName, service: IBinder) {
                worker.post {
                    honorService = service
                    appendEvent(
                        "HONOR_ACCESSORISE_SERVICE_CONNECTED",
                        JSONObject()
                            .put("component", name.flattenToString())
                            .put("binderAlive", service.isBinderAlive)
                            .put("binderPing", service.pingBinder())
                            .put(
                                "interfaceDescriptor",
                                runCatching { service.interfaceDescriptor }.getOrNull()
                                    ?: JSONObject.NULL,
                            ),
                    )
                    attemptRegistration(service)
                }
            }

            override fun onServiceDisconnected(name: ComponentName) {
                worker.post {
                    honorService = null
                    appendEvent(
                        "HONOR_ACCESSORISE_SERVICE_DISCONNECTED",
                        JSONObject().put("component", name.flattenToString()),
                    )
                }
            }

            override fun onNullBinding(name: ComponentName) {
                worker.post {
                    appendEvent(
                        "HONOR_ACCESSORISE_SERVICE_NULL_BINDING",
                        JSONObject().put("component", name.flattenToString()),
                    )
                }
            }

            override fun onBindingDied(name: ComponentName) {
                worker.post {
                    honorService = null
                    appendEvent(
                        "HONOR_ACCESSORISE_SERVICE_BINDING_DIED",
                        JSONObject().put("component", name.flattenToString()),
                    )
                }
            }
        }
        honorConnection = connection

        val intent = Intent(HONOR_ACCESSORY_ACTION).setComponent(HONOR_ACCESSORY_COMPONENT)
        runCatching {
            bindService(intent, connection, Context.BIND_AUTO_CREATE)
        }.onSuccess { accepted ->
            appendEvent(
                "HONOR_ACCESSORISE_BIND_REQUEST_RESULT",
                JSONObject().put("bindServiceReturned", accepted),
            )
            if (!accepted) {
                registrationAttempted = true
                appendEvent(
                    "HONOR_OUTPUT_CONFIG_REGISTRATION_RESULT",
                    JSONObject()
                        .put("attempted", false)
                        .put("success", false)
                        .put("classification", "BIND_REQUEST_REJECTED__REGISTRATION_NOT_ATTEMPTED"),
                )
            }
        }.onFailure {
            registrationAttempted = true
            appendEvent(
                "HONOR_ACCESSORISE_BIND_REQUEST_EXCEPTION",
                JSONObject()
                    .put("errorClass", it.javaClass.name)
                    .put("errorMessage", it.message ?: JSONObject.NULL),
            )
        }
    }

    private fun attemptRegistration(service: IBinder) {
        if (registrationAttempted) return
        registrationAttempted = true

        val callback = HonorAccessoriseOutputConfigBinderProtocol.OutputConfigCallbackBinder { type, payload ->
            worker.post {
                appendEvent(type, payload)
            }
        }
        callbackBinder = callback

        val result = HonorAccessoriseOutputConfigBinderProtocol.registerOutputConfigCallback(
            service,
            callback,
        )
        callbackRegistered = result.success

        val classification = when {
            result.success ->
                "OUTPUT_CONFIG_CALLBACK_REGISTRATION_PASS__KEEP_ONLY_THIS_CALLBACK_REGISTERED"
            result.exceptionClass?.contains("SecurityException") == true ->
                "OUTPUT_CONFIG_CALLBACK_REGISTRATION_REJECTED_BY_SECURITY_BOUNDARY"
            else ->
                "OUTPUT_CONFIG_CALLBACK_REGISTRATION_FAILED_OTHER"
        }

        appendEvent(
            "HONOR_OUTPUT_CONFIG_REGISTRATION_RESULT",
            result.toJson()
                .put("attempted", true)
                .put(
                    "serviceTransactionCode",
                    HonorAccessoriseOutputConfigBinderProtocol
                        .SERVICE_TX_REGISTER_OUTPUT_CONFIG_CALLBACK,
                )
                .put("callbackRegistered", callbackRegistered)
                .put("classification", classification)
                .put("otherHonorRegistrationMethodsInvoked", false)
                .put("setPreviewSurfaceInvoked", false)
                .put("exitPreviewInvoked", false)
                .put("allowlistBypassAttempted", false)
                .put("semanticPromotionAllowed", false),
        )

        if (!callbackRegistered) {
            callbackBinder = null
        }
    }

    @Synchronized
    private fun appendEvent(type: String, payload: JSONObject) {
        val report = runCatching { JSONObject(reportFile.readText()) }.getOrElse {
            JSONObject()
                .put("schema", "truthraw.honor-output-config-callback-probe.v0.41")
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
        report.put("registrationAttempted", registrationAttempted)
        report.put("callbackRegistered", callbackRegistered)
        reportFile.writeText(report.toString(2))
    }

    private fun ensureNotificationChannel() {
        getSystemService(NotificationManager::class.java).createNotificationChannel(
            NotificationChannel(
                CHANNEL_ID,
                "TruthRaw Honor callback boundary probe",
                NotificationManager.IMPORTANCE_LOW,
            ),
        )
    }

    private val reportFile: File
        get() = File(filesDir, REPORT_FILENAME)

    companion object {
        const val ACTION_PROBE_OUTPUT_CONFIG = "com.truthraw.adaptiveui.v041.PROBE_OUTPUT_CONFIG"
        const val ACTION_MARK = "com.truthraw.adaptiveui.v041.MARK"
        const val ACTION_STOP = "com.truthraw.adaptiveui.v041.STOP"
        const val EXTRA_LABEL = "label"
        const val EXTRA_NOTE = "note"
        const val REPORT_FILENAME = "TRUTHRAW_HONOR_OUTPUT_CONFIG_CALLBACK_PROBE_v041.json"

        const val HONOR_PACKAGE = "com.hihonor.camera"
        const val HONOR_ACCESSORY_ACTION = "com.hihonor.camera.aidl.CameraAccessoriseService"
        val HONOR_ACCESSORY_COMPONENT = ComponentName(
            HONOR_PACKAGE,
            "com.hihonor.camera.accessorise.aidl.CameraAccessoriseService",
        )

        const val HONOR_APK_SHA256 =
            "3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52"

        private const val CHANNEL_ID = "truthraw_v041_honor_output_callback"
        private const val NOTIFICATION_ID = 41041
    }
}
