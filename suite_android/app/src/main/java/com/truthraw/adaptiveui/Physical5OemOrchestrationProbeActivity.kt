package com.truthraw.adaptiveui

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.graphics.ImageFormat
import android.hardware.camera2.CameraCaptureSession
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CameraMetadata
import android.hardware.camera2.CaptureFailure
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.CaptureResult
import android.hardware.camera2.TotalCaptureResult
import android.hardware.camera2.params.OutputConfiguration
import android.hardware.camera2.params.SessionConfiguration
import android.media.Image
import android.media.ImageReader
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Range
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.security.MessageDigest
import java.time.Instant
import java.util.concurrent.CountDownLatch
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicReference
import kotlin.math.max
import kotlin.math.min

/**
 * v0.72 semantic-value + locked-acquisition physical Camera-5 capture-effect probe.
 *
 * Static HONOR .452 evidence selects only defensible OEM values:
 * - MasterFilmSensorType = 3 for OEM tele role
 * - cameraSceneMode = 53 UltraHighPixel / 200M
 * - cameraSceneMode = 110 UltraResolution / 50M
 * - cameraSceneMode = 66 Pro Photo RAW reference
 * - teleconverterEnable = true encoded as byteArrayOf(1) for direct Camera2
 *
 * Every frame gets a fresh camera open. Control/candidate order is mirrored across candidates.
 * Both sides use the same manual ISO/exposure/frame-duration/focus request state.
 * No multi-frame fusion, no DNG creation, no Scientific Master mutation.
 */
class Physical5OemOrchestrationProbeActivity : Activity() {
    private lateinit var status: TextView
    private lateinit var saveButton: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        setContentView(buildUi())
        refreshStatus()
    }

    override fun onResume() {
        super.onResume()
        if (::status.isInitialized && ::saveButton.isInitialized) refreshStatus()
    }

    private fun buildUi(): View {
        val body = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(16), dp(12), dp(16), dp(20))
            setBackgroundColor(Color.rgb(12, 14, 18))
        }
        body.addView(label("TruthRaw v0.72 · OEM orchestration RAW test", 21f, true))
        body.addView(label(
            "Alleen OEM-onderbouwde waarden. Iedere control/candidate frame krijgt een verse camera-open en dezelfde handmatige ISO, exposure, frame duration en focus.",
            12f, false, Color.rgb(190, 198, 210)
        ))
        body.addView(space(10))
        body.addView(button("1 · Run semantic locked matrix") { runMatrix() })
        saveButton = button("2 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)
        body.addView(space(10))
        body.addView(label(
            "Een verschil bewijst alleen een app-zichtbaar effect in deze exacte route. Geen claim over native sensorgeometrie, Direct-CFA 200MP of ADC-bitdiepte.",
            11f, false, Color.rgb(155, 165, 180)
        ))
        body.addView(space(10))
        status = label("Nog geen v0.72 report.", 10f, false)
        body.addView(status)

        return ScrollView(this).apply {
            isFillViewport = true
            setBackgroundColor(Color.rgb(12, 14, 18))
            addView(body, ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ))
            setOnApplyWindowInsetsListener { view, insets ->
                val bars = insets.getInsets(WindowInsets.Type.systemBars())
                view.setPadding(bars.left, bars.top, bars.right, bars.bottom)
                insets
            }
        }
    }

    private fun runMatrix() {
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            status.text = "v0.72 vraagt CAMERA-toestemming."
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA_PERMISSION)
            return
        }
        executeMatrix()
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray,
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode != REQUEST_CAMERA_PERMISSION) return
        if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            executeMatrix()
        } else {
            status.text = "CAMERA-toestemming niet verleend; niets getest."
        }
    }

    private fun executeMatrix() {
        saveButton.isEnabled = false
        status.text = "v0.72 initialiseert…"
        Thread {
            val report = runCatching { buildReport() }.getOrElse { e ->
                JSONObject()
                    .put("schema", SCHEMA)
                    .put("createdAtUtc", Instant.now().toString())
                    .put("authority", AUTHORITY)
                    .put("classification", "V072_FATAL_ERROR")
                    .put("errorClass", e.javaClass.name)
                    .put("errorMessage", e.message ?: JSONObject.NULL)
                    .put("scientificMasterModified", false)
                    .put("calibrationAuthorityGranted", false)
            }
            writeCheckpoint(report)
            runOnUiThread { refreshStatus() }
        }.start()
    }

    private fun buildReport(): JSONObject {
        val cm = getSystemService(CameraManager::class.java)
        val logical = cm.getCameraCharacteristics(LOGICAL_ID)
        val physical = cm.getCameraCharacteristics(PHYSICAL_ID)

        require(logical.physicalCameraIds.contains(PHYSICAL_ID)) {
            "physical camera 5 is not disclosed by logical camera 0"
        }

        val maximumMap = physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)
        val maximumRaw10 = safeSizes { maximumMap?.getOutputSizes(ImageFormat.RAW10) }
        val maximumRaw10High = safeSizes { maximumMap?.getHighResolutionOutputSizes(ImageFormat.RAW10) }
        require((maximumRaw10 + maximumRaw10High).any { it.width == TARGET_W && it.height == TARGET_H }) {
            "physical camera 5 does not advertise 16320x12288 RAW10 in maximum-resolution map"
        }

        val logicalRequestNames = logical.availableCaptureRequestKeys.orEmpty().map { it.name }.toSet()
        val physicalRequestNames = physical.availableCaptureRequestKeys.orEmpty().map { it.name }.toSet()
        val logicalSessionNames = logical.availableSessionKeys.orEmpty().map { it.name }.toSet()
        val physicalSessionNames = physical.availableSessionKeys.orEmpty().map { it.name }.toSet()
        val logicalResultNames = logical.availableCaptureResultKeys.orEmpty().map { it.name }.toSet()
        val physicalResultNames = physical.availableCaptureResultKeys.orEmpty().map { it.name }.toSet()
        val locked = chooseLockedSettings(physical)

        val surfaces = JSONObject()
        for (setting in UNIQUE_SETTINGS) {
            surfaces.put(setting.keyName, JSONObject()
                .put("logicalRequestPresent", setting.keyName in logicalRequestNames)
                .put("physicalRequestPresent", setting.keyName in physicalRequestNames)
                .put("logicalSessionPresent", setting.keyName in logicalSessionNames)
                .put("physicalSessionPresent", setting.keyName in physicalSessionNames)
                .put("staticPreferredRepresentation", setting.preferredRepresentation)
                .put("semanticValue", setting.semanticValue))
        }

        val results = JSONArray()
        val report = JSONObject()
            .put("schema", SCHEMA)
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", AUTHORITY)
            .put("logicalCameraId", LOGICAL_ID)
            .put("physicalCameraId", PHYSICAL_ID)
            .put("physicalId5ListedByLogicalCharacteristics", true)
            .put("target", JSONObject()
                .put("format", "RAW10")
                .put("width", TARGET_W)
                .put("height", TARGET_H)
                .put("physicalOutputBinding", true)
                .put("outputMaximumResolutionModeDeclared", true)
                .put("physicalSensorPixelModeRequested", CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION))
            .put("lockedRequest", locked.toJson())
            .put("vendorKeySurfaces", surfaces)
            .put("hintUserValue", JSONObject()
                .put("name", HINT_USER_VALUE_KEY)
                .put("logicalResultPresent", HINT_USER_VALUE_KEY in logicalResultNames)
                .put("physicalResultPresent", HINT_USER_VALUE_KEY in physicalResultNames)
                .put("rawMfUltraHighPixelProcessorCodes", JSONArray(listOf(23, 24, 32, 33))))
            .put("freshCameraOpenPerFrame", true)
            .put("mirroredPairOrder", true)
            .put("candidateCount", VECTORS.size)
            .put("results", results)
            .put("stage", "INITIALIZED")
            .put("pairsComplete", 0)
            .put("physicalFrameCount", 0)
            .put("independentEvidenceCount", 0)
            .put("multiFrameFusionUsed", false)
            .put("rawSourceFilesPersisted", false)
            .put("scientificMasterModified", false)
            .put("calibrationAuthorityGranted", false)
            .put("boundary", BOUNDARY)
        writeCheckpoint(report)

        var pairs = 0
        var frames = 0
        var topologyDiffs = 0
        var selectedMetadataDiffs = 0
        var hintDiffs = 0
        var rawMfHintHits = 0
        var exactStatePairs = 0
        var failed = 0

        for ((index, vector) in VECTORS.withIndex()) {
            runOnUiThread {
                status.text = "v0.72 " + (index + 1) + "/" + VECTORS.size + " · " + vector.shortName
            }

            val settingsJson = JSONArray()
            for (s in vector.settings) {
                settingsJson.put(JSONObject()
                    .put("name", s.keyName)
                    .put("semanticValue", s.semanticValue)
                    .put("semanticBasis", s.semanticBasis)
                    .put("preferredRepresentation", s.preferredRepresentation)
                    .put("logicalRequestPresent", s.keyName in logicalRequestNames)
                    .put("physicalRequestPresent", s.keyName in physicalRequestNames)
                    .put("logicalSessionPresent", s.keyName in logicalSessionNames)
                    .put("physicalSessionPresent", s.keyName in physicalSessionNames))
            }

            val item = JSONObject()
                .put("index", index)
                .put("shortName", vector.shortName)
                .put("semanticBasis", vector.semanticBasis)
                .put("settings", settingsJson)
                .put("order", if (index % 2 == 0) "CONTROL_THEN_CANDIDATE" else "CANDIDATE_THEN_CONTROL")
                .put("pairCompleted", false)
            results.put(item)

            if (vector.settings.any { it.keyName !in logicalRequestNames && it.keyName !in physicalRequestNames }) {
                failed++
                item.put("classification", "SKIPPED_REQUIRED_VENDOR_KEY_NOT_ADVERTISED_ON_LOGICAL_OR_PHYSICAL_REQUEST_SURFACE")
                report.put("stage", "PAIR_SKIPPED_KEY_SURFACE")
                    .put("failedOrSkippedPairCount", failed)
                writeCheckpoint(report)
                continue
            }

            report.put("candidateIndexInFlight", index)
                .put("candidateNameInFlight", vector.shortName)
                .put("stage", "BEFORE_PAIR")
            writeCheckpoint(report)

            val control: FrameOutcome
            val candidate: FrameOutcome

            if (index % 2 == 0) {
                control = captureFresh(cm, logical, physical, locked, emptyList(), "CONTROL")
                if (control.frameCaptured) frames++
                item.put("control", control.toJson())
                report.put("physicalFrameCount", frames)
                    .put("independentEvidenceCount", frames)
                    .put("stage", "CONTROL_RETURNED")
                writeCheckpoint(report)

                candidate = captureFresh(cm, logical, physical, locked, vector.settings, vector.shortName)
                if (candidate.frameCaptured) frames++
                item.put("candidate", candidate.toJson())
            } else {
                candidate = captureFresh(cm, logical, physical, locked, vector.settings, vector.shortName)
                if (candidate.frameCaptured) frames++
                item.put("candidate", candidate.toJson())
                report.put("physicalFrameCount", frames)
                    .put("independentEvidenceCount", frames)
                    .put("stage", "CANDIDATE_RETURNED")
                writeCheckpoint(report)

                control = captureFresh(cm, logical, physical, locked, emptyList(), "CONTROL")
                if (control.frameCaptured) frames++
                item.put("control", control.toJson())
            }

            report.put("physicalFrameCount", frames)
                .put("independentEvidenceCount", frames)
                .put("stage", "BOTH_FRAMES_RETURNED")
            writeCheckpoint(report)

            if (!control.frameCaptured || !candidate.frameCaptured ||
                control.raw == null || candidate.raw == null ||
                control.capture == null || candidate.capture == null
            ) {
                failed++
                item.put("classification", "PAIR_CAPTURE_FAILED")
                report.put("stage", "PAIR_CAPTURE_FAILED")
                    .put("failedOrSkippedPairCount", failed)
                writeCheckpoint(report)
                continue
            }

            val comparison = comparePair(control, candidate)
            item.put("comparison", comparison)
                .put("pairCompleted", true)
                .put("classification", "OEM_VECTOR_LOCKED_CONTROL_CANDIDATE_PAIR_COMPLETE")

            pairs++
            if (comparison.optBoolean("structuralTopologyDifferentialObserved", false)) topologyDiffs++
            if (comparison.optBoolean("selectedResultMetadataDifferentialObserved", false)) selectedMetadataDiffs++
            if (comparison.optBoolean("hintUserValueDifferentialObserved", false)) hintDiffs++
            if (candidate.capture.rawMfUltraHighPixelHintObserved) rawMfHintHits++
            if (comparison.optBoolean("acquisitionStateExactlyMatched", false)) exactStatePairs++

            report
                .put("pairsComplete", pairs)
                .put("physicalFrameCount", frames)
                .put("independentEvidenceCount", frames)
                .put("structuralTopologyDifferentialCount", topologyDiffs)
                .put("selectedResultMetadataDifferentialCount", selectedMetadataDiffs)
                .put("hintUserValueDifferentialCount", hintDiffs)
                .put("candidateRawMfUltraHighPixelHintHitCount", rawMfHintHits)
                .put("acquisitionStateExactlyMatchedPairCount", exactStatePairs)
                .put("failedOrSkippedPairCount", failed)
                .put("stage", "PAIR_RECORDED")
            writeCheckpoint(report)
        }

        return report
            .put("candidateIndexInFlight", JSONObject.NULL)
            .put("candidateNameInFlight", JSONObject.NULL)
            .put("stage", "MATRIX_COMPLETE")
            .put("pairsComplete", pairs)
            .put("physicalFrameCount", frames)
            .put("independentEvidenceCount", frames)
            .put("structuralTopologyDifferentialCount", topologyDiffs)
            .put("selectedResultMetadataDifferentialCount", selectedMetadataDiffs)
            .put("hintUserValueDifferentialCount", hintDiffs)
            .put("candidateRawMfUltraHighPixelHintHitCount", rawMfHintHits)
            .put("acquisitionStateExactlyMatchedPairCount", exactStatePairs)
            .put("failedOrSkippedPairCount", failed)
            .put("classification", "PHYSICAL5_OEM_ORCHESTRATION_CAPTURE_EFFECT_COMPLETE_OR_PARTIAL")
    }

    private data class LockedSettings(
        val iso: Int,
        val exposureNs: Long,
        val frameDurationNs: Long,
        val focusDistanceDiopters: Float,
        val aeOffAvailable: Boolean,
        val afOffAvailable: Boolean,
    ) {
        fun toJson(): JSONObject = JSONObject()
            .put("iso", iso)
            .put("exposureTimeNs", exposureNs)
            .put("frameDurationNs", frameDurationNs)
            .put("focusDistanceDiopters", focusDistanceDiopters.toDouble())
            .put("aeOffAvailable", aeOffAvailable)
            .put("afOffAvailable", afOffAvailable)
    }

    private fun chooseLockedSettings(physical: CameraCharacteristics): LockedSettings {
        val isoRange: Range<Int> = physical.get(CameraCharacteristics.SENSOR_INFO_SENSITIVITY_RANGE)
            ?: error("SENSOR_INFO_SENSITIVITY_RANGE missing")
        val exposureRange: Range<Long> = physical.get(CameraCharacteristics.SENSOR_INFO_EXPOSURE_TIME_RANGE)
            ?: error("SENSOR_INFO_EXPOSURE_TIME_RANGE missing")
        val maxFrame = physical.get(CameraCharacteristics.SENSOR_INFO_MAX_FRAME_DURATION)
            ?: error("SENSOR_INFO_MAX_FRAME_DURATION missing")
        val minFocus = physical.get(CameraCharacteristics.LENS_INFO_MINIMUM_FOCUS_DISTANCE) ?: 0f
        val aeModes = physical.get(CameraCharacteristics.CONTROL_AE_AVAILABLE_MODES) ?: intArrayOf()
        val afModes = physical.get(CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES) ?: intArrayOf()

        require(aeModes.contains(CameraMetadata.CONTROL_AE_MODE_OFF)) { "physical-5 AE OFF unavailable" }
        require(afModes.contains(CameraMetadata.CONTROL_AF_MODE_OFF)) { "physical-5 AF OFF unavailable" }

        val iso = 800.coerceIn(isoRange.lower, isoRange.upper)
        val exposure = 10_000_000L.coerceIn(exposureRange.lower, exposureRange.upper)
        val preferredFrame = max(33_322_225L, exposure)
        val frame = min(preferredFrame, maxFrame)
        require(frame >= exposure) { "chosen frame duration < exposure" }
        val focus = if (minFocus > 0f) min(1.0f, minFocus) else 0f

        return LockedSettings(
            iso = iso,
            exposureNs = exposure,
            frameDurationNs = frame,
            focusDistanceDiopters = focus,
            aeOffAvailable = true,
            afOffAvailable = true,
        )
    }

    private data class OpenResult(
        val device: CameraDevice?,
        val executor: ExecutorService?,
        val closedLatch: CountDownLatch?,
        val error: String?,
    )

    private fun openLogicalCamera(cm: CameraManager): OpenResult {
        val openedLatch = CountDownLatch(1)
        val closedLatch = CountDownLatch(1)
        val executor = Executors.newSingleThreadExecutor()
        var device: CameraDevice? = null
        var error: String? = null

        try {
            cm.openCamera(LOGICAL_ID, executor, object : CameraDevice.StateCallback() {
                override fun onOpened(camera: CameraDevice) {
                    device = camera
                    openedLatch.countDown()
                }

                override fun onDisconnected(camera: CameraDevice) {
                    error = "CAMERA_DISCONNECTED"
                    camera.close()
                    openedLatch.countDown()
                }

                override fun onError(camera: CameraDevice, code: Int) {
                    error = "CAMERA_OPEN_ERROR_" + code
                    camera.close()
                    openedLatch.countDown()
                }

                override fun onClosed(camera: CameraDevice) {
                    closedLatch.countDown()
                }
            })
            if (!openedLatch.await(6, TimeUnit.SECONDS)) error = "CAMERA_OPEN_TIMEOUT"
        } catch (e: Throwable) {
            error = e.javaClass.name + ": " + e.message
        }

        if (device == null) {
            executor.shutdownNow()
            return OpenResult(null, null, null, error)
        }
        return OpenResult(device, executor, closedLatch, error)
    }

    private data class ResolvedSetting(
        val spec: SettingSpec,
        val logicalKey: CaptureRequest.Key<Any>?,
        val logicalValue: Any?,
        val logicalRepresentation: String?,
        val physicalKey: CaptureRequest.Key<Any>?,
        val physicalValue: Any?,
        val physicalRepresentation: String?,
        val logicalSessionEligible: Boolean,
        val physicalSessionEligible: Boolean,
    ) {
        fun hasAnyResolvedScope(): Boolean = logicalKey != null || physicalKey != null

        fun toJson(): JSONObject = JSONObject()
            .put("name", spec.keyName)
            .put("semanticValue", spec.semanticValue)
            .put("preferredRepresentation", spec.preferredRepresentation)
            .put("logicalResolved", logicalKey != null)
            .put("logicalRepresentation", logicalRepresentation ?: JSONObject.NULL)
            .put("logicalSessionEligible", logicalSessionEligible)
            .put("physicalResolved", physicalKey != null)
            .put("physicalRepresentation", physicalRepresentation ?: JSONObject.NULL)
            .put("physicalSessionEligible", physicalSessionEligible)
    }

    private data class ResolvedValue(
        val value: Any,
        val representation: String,
        val readback: Any?,
    )

    private fun captureFresh(
        cm: CameraManager,
        logical: CameraCharacteristics,
        physical: CameraCharacteristics,
        locked: LockedSettings,
        settings: List<SettingSpec>,
        label: String,
    ): FrameOutcome {
        val opened = openLogicalCamera(cm)
        val device = opened.device ?: return FrameOutcome(
            label = label,
            sessionConfigured = false,
            frameCaptured = false,
            candidateSessionParameterAttached = false,
            candidateRequestKeyWritten = false,
            candidateReadback = JSONObject.NULL,
            vendorEvidence = JSONObject().put("resolution", "CAMERA_OPEN_FAILED"),
            physicalPixelModeWritten = false,
            physicalPixelModeReadback = null,
            lockedControlWriteComplete = false,
            capture = null,
            raw = null,
            errorClass = "CAMERA_OPEN_FAILED",
            errorMessage = opened.error,
        )

        return try {
            val resolved = resolveSettings(device, logical, physical, settings)
            if (resolved.any { !it.hasAnyResolvedScope() }) {
                FrameOutcome(
                    label = label,
                    sessionConfigured = false,
                    frameCaptured = false,
                    candidateSessionParameterAttached = false,
                    candidateRequestKeyWritten = false,
                    candidateReadback = JSONObject.NULL,
                    vendorEvidence = JSONObject()
                        .put("resolvedSettings", JSONArray(resolved.map { it.toJson() }))
                        .put("failure", "ONE_OR_MORE_VENDOR_SETTINGS_COULD_NOT_BE_MARSHALED_ON_ANY_ADVERTISED_SCOPE"),
                    physicalPixelModeWritten = false,
                    physicalPixelModeReadback = null,
                    lockedControlWriteComplete = false,
                    capture = null,
                    raw = null,
                    errorClass = "VENDOR_VALUE_RESOLUTION_FAILED",
                    errorMessage = null,
                )
            } else {
                runSingleFrame(
                    device = device,
                    executor = opened.executor!!,
                    logical = logical,
                    physical = physical,
                    locked = locked,
                    settings = resolved,
                    label = label,
                )
            }
        } finally {
            device.close()
            opened.closedLatch?.await(2, TimeUnit.SECONDS)
            opened.executor?.shutdown()
            opened.executor?.awaitTermination(2, TimeUnit.SECONDS)
            opened.executor?.shutdownNow()
        }
    }

    private fun resolveSettings(
        device: CameraDevice,
        logical: CameraCharacteristics,
        physical: CameraCharacteristics,
        specs: List<SettingSpec>,
    ): List<ResolvedSetting> {
        val logicalRequest = logical.availableCaptureRequestKeys.orEmpty().associateBy { it.name }
        val physicalRequest = physical.availableCaptureRequestKeys.orEmpty().associateBy { it.name }
        val logicalSession = logical.availableSessionKeys.orEmpty().map { it.name }.toSet()
        val physicalSession = physical.availableSessionKeys.orEmpty().map { it.name }.toSet()

        return specs.map { spec ->
            @Suppress("UNCHECKED_CAST")
            val logicalKey = logicalRequest[spec.keyName] as CaptureRequest.Key<Any>?
            @Suppress("UNCHECKED_CAST")
            val physicalKey = physicalRequest[spec.keyName] as CaptureRequest.Key<Any>?

            val logicalResolved = logicalKey?.let {
                resolveValue(device, it, spec, physicalScope = false)
            }
            val physicalResolved = physicalKey?.let {
                resolveValue(device, it, spec, physicalScope = true)
            }

            ResolvedSetting(
                spec = spec,
                logicalKey = if (logicalResolved != null) logicalKey else null,
                logicalValue = logicalResolved?.value,
                logicalRepresentation = logicalResolved?.representation,
                physicalKey = if (physicalResolved != null) physicalKey else null,
                physicalValue = physicalResolved?.value,
                physicalRepresentation = physicalResolved?.representation,
                logicalSessionEligible = logicalResolved != null && spec.keyName in logicalSession,
                physicalSessionEligible = physicalResolved != null && spec.keyName in physicalSession,
            )
        }
    }

    private fun resolveValue(
        device: CameraDevice,
        key: CaptureRequest.Key<Any>,
        spec: SettingSpec,
        physicalScope: Boolean,
    ): ResolvedValue? {
        for ((representation, value) in candidateRepresentations(spec)) {
            val builder = runCatching {
                if (physicalScope) {
                    device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE, setOf(PHYSICAL_ID))
                } else {
                    device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)
                }
            }.getOrNull() ?: continue

            val readback = runCatching {
                if (physicalScope) {
                    builder.setPhysicalCameraKey(key, value, PHYSICAL_ID)
                    builder.getPhysicalCameraKey(key, PHYSICAL_ID)
                } else {
                    builder.set(key, value)
                    builder.get(key)
                }
            }.getOrNull()

            if (readback != null && valuesEquivalent(value, readback)) {
                return ResolvedValue(value, representation, readback)
            }
        }
        return null
    }

    private fun candidateRepresentations(spec: SettingSpec): List<Pair<String, Any>> {
        val scalar = "INT" to spec.semanticValue
        val array = "INT_ARRAY" to intArrayOf(spec.semanticValue)
        return if (spec.preferredRepresentation == "INT") {
            listOf(scalar, array)
        } else {
            listOf(array, scalar)
        }
    }

    private fun valuesEquivalent(expected: Any, actual: Any): Boolean = when {
        expected is Int && actual is Int -> expected == actual
        expected is IntArray && actual is IntArray -> expected.contentEquals(actual)
        expected is ByteArray && actual is ByteArray -> expected.contentEquals(actual)
        else -> expected.toString() == actual.toString()
    }

    private data class FrameOutcome(
        val label: String,
        val sessionConfigured: Boolean,
        val frameCaptured: Boolean,
        val candidateSessionParameterAttached: Boolean,
        val candidateRequestKeyWritten: Boolean,
        val candidateReadback: Any?,
        val vendorEvidence: JSONObject,
        val physicalPixelModeWritten: Boolean,
        val physicalPixelModeReadback: Any?,
        val lockedControlWriteComplete: Boolean,
        val capture: CaptureSummary?,
        val raw: RawSummary?,
        val errorClass: String?,
        val errorMessage: String?,
    ) {
        fun toJson(): JSONObject = JSONObject()
            .put("label", label)
            .put("sessionConfigured", sessionConfigured)
            .put("frameCaptured", frameCaptured)
            .put("candidateSessionParameterAttached", candidateSessionParameterAttached)
            .put("candidateRequestKeyWritten", candidateRequestKeyWritten)
            .put("candidateReadback", jsonValue(candidateReadback))
            .put("vendorEvidence", vendorEvidence)
            .put("physicalSensorPixelModeWritten", physicalPixelModeWritten)
            .put("physicalSensorPixelModeReadback", jsonValue(physicalPixelModeReadback))
            .put("lockedControlWriteComplete", lockedControlWriteComplete)
            .put("capture", capture?.toJson() ?: JSONObject.NULL)
            .put("raw", raw?.toJson() ?: JSONObject.NULL)
            .put("errorClass", errorClass ?: JSONObject.NULL)
            .put("errorMessage", errorMessage ?: JSONObject.NULL)
    }

    private data class CaptureSummary(
        val timestampNs: Long?,
        val imageTimestampNs: Long?,
        val timestampIdentity: Boolean,
        val iso: Int?,
        val exposureNs: Long?,
        val frameDurationNs: Long?,
        val focalLengthMm: Float?,
        val focusDistanceDiopters: Float?,
        val sensorPixelMode: Int?,
        val rawBinningFactorUsed: Boolean?,
        val noiseReductionMode: Int?,
        val edgeMode: Int?,
        val dynamicBlackLevel: FloatArray?,
        val physicalResultCameraId: String?,
        val logicalHintUserValue: Any?,
        val physicalHintUserValue: Any?,
        val rawMfUltraHighPixelHintObserved: Boolean,
    ) {
        fun toJson(): JSONObject = JSONObject()
            .put("sensorTimestampNs", timestampNs ?: JSONObject.NULL)
            .put("imageTimestampNs", imageTimestampNs ?: JSONObject.NULL)
            .put("timestampIdentityPass", timestampIdentity)
            .put("iso", iso ?: JSONObject.NULL)
            .put("exposureTimeNs", exposureNs ?: JSONObject.NULL)
            .put("frameDurationNs", frameDurationNs ?: JSONObject.NULL)
            .put("focalLengthMm", focalLengthMm?.toDouble() ?: JSONObject.NULL)
            .put("focusDistanceDiopters", focusDistanceDiopters?.toDouble() ?: JSONObject.NULL)
            .put("sensorPixelMode", sensorPixelMode ?: JSONObject.NULL)
            .put("rawBinningFactorUsed", rawBinningFactorUsed ?: JSONObject.NULL)
            .put("noiseReductionMode", noiseReductionMode ?: JSONObject.NULL)
            .put("edgeMode", edgeMode ?: JSONObject.NULL)
            .put("dynamicBlackLevel", dynamicBlackLevel?.let { JSONArray(it.map { v -> v.toDouble() }) } ?: JSONObject.NULL)
            .put("physicalResultCameraId", physicalResultCameraId ?: JSONObject.NULL)
            .put("logicalHintUserValue", jsonValue(logicalHintUserValue))
            .put("physicalHintUserValue", jsonValue(physicalHintUserValue))
            .put("rawMfUltraHighPixelHintObserved", rawMfUltraHighPixelHintObserved)
    }

    private data class RawSummary(
        val accessibleBytes: Long,
        val rowStride: Int,
        val pixelStride: Int,
        val fullPlaneSha256: String,
        val firstNonZeroByte: Long?,
        val lastNonZeroByte: Long?,
        val tailNonZeroByteCount: Long,
        val effectivePrefixSha256: String?,
        val packedDataOnlySha256: String?,
        val effectivePadNonZeroByteCount: Long?,
        val effectiveRowsWithImageData: Int?,
        val maxDeclaredRowWithNonZero: Int?,
        val decodedMin: Int?,
        val decodedMax: Int?,
        val decodedMean: Double?,
        val parityMeans: DoubleArray?,
        val knownV059TopologyMatch: Boolean,
    ) {
        fun toJson(): JSONObject = JSONObject()
            .put("accessibleBytes", accessibleBytes)
            .put("rowStride", rowStride)
            .put("pixelStride", pixelStride)
            .put("fullPlaneSha256", fullPlaneSha256)
            .put("firstNonZeroByte", firstNonZeroByte ?: JSONObject.NULL)
            .put("lastNonZeroByte", lastNonZeroByte ?: JSONObject.NULL)
            .put("knownEffectivePrefixBoundaryByte", EFFECTIVE_PREFIX_BYTES)
            .put("tailNonZeroByteCount", tailNonZeroByteCount)
            .put("effectivePrefixSha256", effectivePrefixSha256 ?: JSONObject.NULL)
            .put("packedDataOnlySha256", packedDataOnlySha256 ?: JSONObject.NULL)
            .put("effectivePadNonZeroByteCount", effectivePadNonZeroByteCount ?: JSONObject.NULL)
            .put("effectiveRowsWithImageData", effectiveRowsWithImageData ?: JSONObject.NULL)
            .put("maxDeclaredRowWithNonZero", maxDeclaredRowWithNonZero ?: JSONObject.NULL)
            .put("decodedMin", decodedMin ?: JSONObject.NULL)
            .put("decodedMax", decodedMax ?: JSONObject.NULL)
            .put("decodedMean", decodedMean ?: JSONObject.NULL)
            .put("parityMeans", parityMeans?.let { JSONArray(it.toList()) } ?: JSONObject.NULL)
            .put("knownV059TopologyMatch", knownV059TopologyMatch)
    }

    private fun runSingleFrame(
        device: CameraDevice,
        executor: ExecutorService,
        logical: CameraCharacteristics,
        physical: CameraCharacteristics,
        locked: LockedSettings,
        settings: List<ResolvedSetting>,
        label: String,
    ): FrameOutcome {
        val resolvedJson = JSONArray(settings.map { it.toJson() })
        val vendorEvidence = JSONObject()
            .put("resolvedSettings", resolvedJson)
            .put("logicalAndPhysicalScopesMayBothBeWrittenWhenAdvertised", true)

        val reader = runCatching {
            ImageReader.newInstance(TARGET_W, TARGET_H, ImageFormat.RAW10, 1)
        }.getOrElse {
            return FrameOutcome(
                label = label,
                sessionConfigured = false,
                frameCaptured = false,
                candidateSessionParameterAttached = false,
                candidateRequestKeyWritten = false,
                candidateReadback = JSONObject.NULL,
                vendorEvidence = vendorEvidence,
                physicalPixelModeWritten = false,
                physicalPixelModeReadback = null,
                lockedControlWriteComplete = false,
                capture = null,
                raw = null,
                errorClass = it.javaClass.name,
                errorMessage = it.message,
            )
        }

        val imageRef = AtomicReference<Image?>(null)
        val imageLatch = CountDownLatch(1)
        reader.setOnImageAvailableListener({ source: ImageReader ->
            val image = runCatching { source.acquireNextImage() }.getOrNull()
            if (image != null && imageRef.compareAndSet(null, image)) {
                imageLatch.countDown()
            } else {
                image?.close()
            }
        }, Handler(Looper.getMainLooper()))

        val sessionLatch = CountDownLatch(1)
        val sessionClosedLatch = CountDownLatch(1)
        val sessionRef = AtomicReference<CameraCaptureSession?>(null)
        var sessionConfigured = false
        var sessionError: String? = null

        val output = OutputConfiguration(reader.surface)
        try {
            output.setPhysicalCameraId(PHYSICAL_ID)
            output.addSensorPixelModeUsed(CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
        } catch (e: Throwable) {
            reader.close()
            return FrameOutcome(
                label = label,
                sessionConfigured = false,
                frameCaptured = false,
                candidateSessionParameterAttached = false,
                candidateRequestKeyWritten = false,
                candidateReadback = JSONObject.NULL,
                vendorEvidence = vendorEvidence,
                physicalPixelModeWritten = false,
                physicalPixelModeReadback = null,
                lockedControlWriteComplete = false,
                capture = null,
                raw = null,
                errorClass = e.javaClass.name,
                errorMessage = e.message,
            )
        }

        val callback = object : CameraCaptureSession.StateCallback() {
            override fun onConfigured(session: CameraCaptureSession) {
                sessionConfigured = true
                sessionRef.set(session)
                sessionLatch.countDown()
            }

            override fun onConfigureFailed(session: CameraCaptureSession) {
                sessionError = "onConfigureFailed"
                sessionRef.set(session)
                sessionLatch.countDown()
            }

            override fun onClosed(session: CameraCaptureSession) {
                sessionClosedLatch.countDown()
            }
        }

        val config = SessionConfiguration(
            SessionConfiguration.SESSION_REGULAR,
            listOf(output),
            executor,
            callback
        )

        val sessionReadbacks = JSONObject()
        var sessionAttachedCount = 0
        val requestReadbacks = JSONObject()
        var requestWriteCount = 0

        try {
            val anyPhysicalSession = settings.any { it.physicalKey != null && it.physicalSessionEligible }
            val anyLogicalSession = settings.any { it.logicalKey != null && it.logicalSessionEligible }

            if (anyPhysicalSession || anyLogicalSession) {
                val sessionBuilder = if (anyPhysicalSession) {
                    device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE, setOf(PHYSICAL_ID))
                } else {
                    device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)
                }

                for (setting in settings) {
                    if (setting.logicalKey != null && setting.logicalValue != null && setting.logicalSessionEligible) {
                        sessionBuilder.set(setting.logicalKey, setting.logicalValue)
                        val rb = sessionBuilder.get(setting.logicalKey)
                        sessionReadbacks.put("logical:" + setting.spec.keyName, jsonValue(rb))
                        sessionAttachedCount++
                    }
                    if (setting.physicalKey != null && setting.physicalValue != null && setting.physicalSessionEligible) {
                        sessionBuilder.setPhysicalCameraKey(setting.physicalKey, setting.physicalValue, PHYSICAL_ID)
                        val rb = sessionBuilder.getPhysicalCameraKey(setting.physicalKey, PHYSICAL_ID)
                        sessionReadbacks.put("physical:" + setting.spec.keyName, jsonValue(rb))
                        sessionAttachedCount++
                    }
                }

                config.setSessionParameters(sessionBuilder.build())
            }

            vendorEvidence
                .put("sessionAttachedScopeCount", sessionAttachedCount)
                .put("sessionReadbacks", sessionReadbacks)

            device.createCaptureSession(config)
            if (!sessionLatch.await(8, TimeUnit.SECONDS) || !sessionConfigured) {
                runCatching { sessionRef.get()?.close() }
                sessionClosedLatch.await(2, TimeUnit.SECONDS)
                reader.close()
                return FrameOutcome(
                    label = label,
                    sessionConfigured = false,
                    frameCaptured = false,
                    candidateSessionParameterAttached = sessionAttachedCount > 0,
                    candidateRequestKeyWritten = false,
                    candidateReadback = requestReadbacks,
                    vendorEvidence = vendorEvidence,
                    physicalPixelModeWritten = false,
                    physicalPixelModeReadback = null,
                    lockedControlWriteComplete = false,
                    capture = null,
                    raw = null,
                    errorClass = "SESSION_CONFIGURATION_FAILED_OR_TIMED_OUT",
                    errorMessage = sessionError,
                )
            }

            val session = sessionRef.get() ?: error("configured session missing")
            val builder = device.createCaptureRequest(
                CameraDevice.TEMPLATE_STILL_CAPTURE,
                setOf(PHYSICAL_ID)
            )
            builder.addTarget(reader.surface)

            var physicalPixelModeWritten = false
            var physicalPixelModeReadback: Any? = null
            builder.setPhysicalCameraKey(
                CaptureRequest.SENSOR_PIXEL_MODE,
                CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION,
                PHYSICAL_ID
            )
            physicalPixelModeWritten = true
            physicalPixelModeReadback = builder.getPhysicalCameraKey(
                CaptureRequest.SENSOR_PIXEL_MODE,
                PHYSICAL_ID
            )

            val lockedComplete = applyLockedControls(builder, locked)

            for (setting in settings) {
                if (setting.logicalKey != null && setting.logicalValue != null) {
                    builder.set(setting.logicalKey, setting.logicalValue)
                    val rb = builder.get(setting.logicalKey)
                    requestReadbacks.put("logical:" + setting.spec.keyName, jsonValue(rb))
                    requestWriteCount++
                }
                if (setting.physicalKey != null && setting.physicalValue != null) {
                    builder.setPhysicalCameraKey(setting.physicalKey, setting.physicalValue, PHYSICAL_ID)
                    val rb = builder.getPhysicalCameraKey(setting.physicalKey, PHYSICAL_ID)
                    requestReadbacks.put("physical:" + setting.spec.keyName, jsonValue(rb))
                    requestWriteCount++
                }
            }

            vendorEvidence
                .put("requestWrittenScopeCount", requestWriteCount)
                .put("requestReadbacks", requestReadbacks)

            val resultRef = AtomicReference<TotalCaptureResult?>(null)
            val resultLatch = CountDownLatch(1)
            var captureFailureText: String? = null

            session.captureSingleRequest(
                builder.build(),
                executor,
                object : CameraCaptureSession.CaptureCallback() {
                    override fun onCaptureCompleted(
                        session: CameraCaptureSession,
                        request: CaptureRequest,
                        result: TotalCaptureResult,
                    ) {
                        resultRef.set(result)
                        resultLatch.countDown()
                    }

                    override fun onCaptureFailed(
                        session: CameraCaptureSession,
                        request: CaptureRequest,
                        failure: CaptureFailure,
                    ) {
                        captureFailureText =
                            "reason=" + failure.reason +
                                " wasImageCaptured=" + failure.wasImageCaptured() +
                                " frameNumber=" + failure.frameNumber
                        resultLatch.countDown()
                    }
                }
            )

            val resultReady = resultLatch.await(10, TimeUnit.SECONDS)
            val imageReady = imageLatch.await(10, TimeUnit.SECONDS)
            val logicalResult = resultRef.get()
            val image = imageRef.get()

            if (!resultReady || !imageReady || logicalResult == null || image == null) {
                image?.close()
                session.close()
                sessionClosedLatch.await(2, TimeUnit.SECONDS)
                reader.close()
                return FrameOutcome(
                    label = label,
                    sessionConfigured = true,
                    frameCaptured = false,
                    candidateSessionParameterAttached = sessionAttachedCount > 0,
                    candidateRequestKeyWritten = requestWriteCount > 0,
                    candidateReadback = requestReadbacks,
                    vendorEvidence = vendorEvidence,
                    physicalPixelModeWritten = physicalPixelModeWritten,
                    physicalPixelModeReadback = physicalPixelModeReadback,
                    lockedControlWriteComplete = lockedComplete,
                    capture = null,
                    raw = null,
                    errorClass = "CAPTURE_PAIRING_FAILED",
                    errorMessage = captureFailureText ?: "resultReady=" + resultReady + " imageReady=" + imageReady,
                )
            }

            try {
                val physicalResult = logicalResult.physicalCameraResults[PHYSICAL_ID]
                    ?: error("physical Camera-5 result missing")
                val sensorTs = physicalResult.get(CaptureResult.SENSOR_TIMESTAMP)
                require(sensorTs == image.timestamp) {
                    "physical timestamp " + sensorTs + " != image timestamp " + image.timestamp
                }

                @Suppress("UNCHECKED_CAST")
                val logicalHintKey = logical.availableCaptureResultKeys.orEmpty()
                    .firstOrNull { it.name == HINT_USER_VALUE_KEY } as CaptureResult.Key<Any>?
                @Suppress("UNCHECKED_CAST")
                val physicalHintKey = physical.availableCaptureResultKeys.orEmpty()
                    .firstOrNull { it.name == HINT_USER_VALUE_KEY } as CaptureResult.Key<Any>?

                val logicalHint = logicalHintKey?.let { runCatching { logicalResult.get(it) }.getOrNull() }
                val physicalHint = physicalHintKey?.let { runCatching { physicalResult.get(it) }.getOrNull() }
                val rawMfHint = isRawMfUltraHighPixelHint(logicalHint) || isRawMfUltraHighPixelHint(physicalHint)

                val raw = analyzeRaw10(image)
                val capture = CaptureSummary(
                    timestampNs = sensorTs,
                    imageTimestampNs = image.timestamp,
                    timestampIdentity = sensorTs == image.timestamp,
                    iso = physicalResult.get(CaptureResult.SENSOR_SENSITIVITY),
                    exposureNs = physicalResult.get(CaptureResult.SENSOR_EXPOSURE_TIME),
                    frameDurationNs = physicalResult.get(CaptureResult.SENSOR_FRAME_DURATION),
                    focalLengthMm = physicalResult.get(CaptureResult.LENS_FOCAL_LENGTH),
                    focusDistanceDiopters = physicalResult.get(CaptureResult.LENS_FOCUS_DISTANCE),
                    sensorPixelMode = physicalResult.get(CaptureResult.SENSOR_PIXEL_MODE),
                    rawBinningFactorUsed = physicalResult.get(CaptureResult.SENSOR_RAW_BINNING_FACTOR_USED),
                    noiseReductionMode = physicalResult.get(CaptureResult.NOISE_REDUCTION_MODE),
                    edgeMode = physicalResult.get(CaptureResult.EDGE_MODE),
                    dynamicBlackLevel = physicalResult.get(CaptureResult.SENSOR_DYNAMIC_BLACK_LEVEL),
                    physicalResultCameraId = physicalResult.cameraId,
                    logicalHintUserValue = logicalHint,
                    physicalHintUserValue = physicalHint,
                    rawMfUltraHighPixelHintObserved = rawMfHint,
                )

                return FrameOutcome(
                    label = label,
                    sessionConfigured = true,
                    frameCaptured = true,
                    candidateSessionParameterAttached = sessionAttachedCount > 0,
                    candidateRequestKeyWritten = requestWriteCount > 0,
                    candidateReadback = requestReadbacks,
                    vendorEvidence = vendorEvidence,
                    physicalPixelModeWritten = physicalPixelModeWritten,
                    physicalPixelModeReadback = physicalPixelModeReadback,
                    lockedControlWriteComplete = lockedComplete,
                    capture = capture,
                    raw = raw,
                    errorClass = null,
                    errorMessage = null,
                )
            } finally {
                image.close()
                session.close()
                sessionClosedLatch.await(2, TimeUnit.SECONDS)
                reader.close()
            }
        } catch (e: Throwable) {
            runCatching { imageRef.getAndSet(null)?.close() }
            runCatching { sessionRef.get()?.close() }
            sessionClosedLatch.await(2, TimeUnit.SECONDS)
            reader.close()
            return FrameOutcome(
                label = label,
                sessionConfigured = sessionConfigured,
                frameCaptured = false,
                candidateSessionParameterAttached = sessionAttachedCount > 0,
                candidateRequestKeyWritten = requestWriteCount > 0,
                candidateReadback = requestReadbacks,
                vendorEvidence = vendorEvidence,
                physicalPixelModeWritten = false,
                physicalPixelModeReadback = null,
                lockedControlWriteComplete = false,
                capture = null,
                raw = null,
                errorClass = e.javaClass.name,
                errorMessage = e.message,
            )
        }
    }

    private fun isRawMfUltraHighPixelHint(value: Any?): Boolean {
        val scalar = when (value) {
            is Int -> value
            is IntArray -> value.firstOrNull()
            is Number -> value.toInt()
            else -> null
        }
        return scalar in setOf(23, 24, 32, 33)
    }

    private fun applyLockedControls(
        builder: CaptureRequest.Builder,
        locked: LockedSettings,
    ): Boolean {
        fun <T> setBoth(key: CaptureRequest.Key<T>, value: T) {
            builder.set(key, value)
            builder.setPhysicalCameraKey(key, value, PHYSICAL_ID)
            val readback = builder.getPhysicalCameraKey(key, PHYSICAL_ID)
            require(readback == value || (readback is Number && value is Number && readback.toString() == value.toString())) {
                "physical readback mismatch for " + key.name + ": " + readback + " != " + value
            }
        }

        setBoth(CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_OFF)
        setBoth(CaptureRequest.SENSOR_SENSITIVITY, locked.iso)
        setBoth(CaptureRequest.SENSOR_EXPOSURE_TIME, locked.exposureNs)
        setBoth(CaptureRequest.SENSOR_FRAME_DURATION, locked.frameDurationNs)
        setBoth(CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_OFF)
        setBoth(CaptureRequest.LENS_FOCUS_DISTANCE, locked.focusDistanceDiopters)

        val physical = getSystemService(CameraManager::class.java)
            .getCameraCharacteristics(PHYSICAL_ID)
        val nr = physical.get(CameraCharacteristics.NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES) ?: intArrayOf()
        if (nr.contains(CameraMetadata.NOISE_REDUCTION_MODE_OFF)) {
            setBoth(CaptureRequest.NOISE_REDUCTION_MODE, CameraMetadata.NOISE_REDUCTION_MODE_OFF)
        }
        val edge = physical.get(CameraCharacteristics.EDGE_AVAILABLE_EDGE_MODES) ?: intArrayOf()
        if (edge.contains(CameraMetadata.EDGE_MODE_OFF)) {
            setBoth(CaptureRequest.EDGE_MODE, CameraMetadata.EDGE_MODE_OFF)
        }
        return true
    }

    private fun analyzeRaw10(image: Image): RawSummary {
        require(image.width == TARGET_W && image.height == TARGET_H) {
            "RAW10 image dimensions " + image.width + "x" + image.height + " != target"
        }
        val plane = image.planes.singleOrNull() ?: error("RAW10 plane count != 1")
        val source = plane.buffer.duplicate().apply { rewind() }
        val accessible = source.remaining().toLong()

        val fullMd = MessageDigest.getInstance("SHA-256")
        val prefixMd = MessageDigest.getInstance("SHA-256")
        val scratch = ByteArray(1024 * 1024)

        var absolute = 0L
        var firstNonZero: Long? = null
        var lastNonZero: Long? = null
        var tailNonZero = 0L
        var maxDeclaredRow: Int? = null

        while (source.hasRemaining()) {
            val n = minOf(source.remaining(), scratch.size)
            source.get(scratch, 0, n)
            fullMd.update(scratch, 0, n)

            if (absolute < EFFECTIVE_PREFIX_BYTES) {
                val prefixN = minOf(n.toLong(), EFFECTIVE_PREFIX_BYTES - absolute).toInt()
                if (prefixN > 0) prefixMd.update(scratch, 0, prefixN)
            }

            for (i in 0 until n) {
                if (scratch[i].toInt() != 0) {
                    val pos = absolute + i
                    if (firstNonZero == null) firstNonZero = pos
                    lastNonZero = pos
                    if (pos >= EFFECTIVE_PREFIX_BYTES) tailNonZero++
                    if (plane.rowStride > 0) {
                        val row = (pos / plane.rowStride.toLong()).toInt()
                        if (maxDeclaredRow == null || row > maxDeclaredRow!!) maxDeclaredRow = row
                    }
                }
            }
            absolute += n
        }

        var effectivePrefixSha: String? = null
        var packedOnlySha: String? = null
        var padNonZero: Long? = null
        var rowsWithData: Int? = null
        var decodedMin: Int? = null
        var decodedMax: Int? = null
        var decodedMean: Double? = null
        var parityMeans: DoubleArray? = null

        if (accessible >= EFFECTIVE_PREFIX_BYTES) {
            effectivePrefixSha = hex(prefixMd.digest())
            val packedMd = MessageDigest.getInstance("SHA-256")
            val row = ByteArray(EFFECTIVE_ROW_BYTES)
            val buf = plane.buffer.duplicate().apply { rewind() }

            var pads = 0L
            var rows = 0
            var minV = 1023
            var maxV = 0
            var sum = 0L
            var count = 0L
            val paritySum = LongArray(4)
            val parityCount = LongArray(4)

            for (y in 0 until EFFECTIVE_ROWS) {
                val offset = y * EFFECTIVE_ROW_BYTES
                buf.position(offset)
                buf.get(row, 0, EFFECTIVE_ROW_BYTES)
                packedMd.update(row, 0, EFFECTIVE_PACKED_BYTES_PER_ROW)

                var anyData = false
                for (i in 0 until EFFECTIVE_PACKED_BYTES_PER_ROW) {
                    if (row[i].toInt() != 0) {
                        anyData = true
                        break
                    }
                }
                if (anyData) rows++

                for (i in EFFECTIVE_PACKED_BYTES_PER_ROW until EFFECTIVE_ROW_BYTES) {
                    if (row[i].toInt() != 0) pads++
                }

                var x = 0
                var p = 0
                while (p + 4 < EFFECTIVE_PACKED_BYTES_PER_ROW) {
                    val b0 = row[p].toInt() and 0xff
                    val b1 = row[p + 1].toInt() and 0xff
                    val b2 = row[p + 2].toInt() and 0xff
                    val b3 = row[p + 3].toInt() and 0xff
                    val b4 = row[p + 4].toInt() and 0xff
                    val values = intArrayOf(
                        (b0 shl 2) or (b4 and 0x03),
                        (b1 shl 2) or ((b4 shr 2) and 0x03),
                        (b2 shl 2) or ((b4 shr 4) and 0x03),
                        (b3 shl 2) or ((b4 shr 6) and 0x03),
                    )
                    for (k in 0..3) {
                        val v = values[k]
                        if (v < minV) minV = v
                        if (v > maxV) maxV = v
                        sum += v
                        count++
                        val parity = (y and 1) * 2 + ((x + k) and 1)
                        paritySum[parity] += v
                        parityCount[parity]++
                    }
                    x += 4
                    p += 5
                }
            }

            packedOnlySha = hex(packedMd.digest())
            padNonZero = pads
            rowsWithData = rows
            decodedMin = minV
            decodedMax = maxV
            decodedMean = if (count > 0) sum.toDouble() / count.toDouble() else null
            parityMeans = DoubleArray(4) { i ->
                if (parityCount[i] > 0) paritySum[i].toDouble() / parityCount[i].toDouble() else Double.NaN
            }
        }

        val knownTopologyMatch =
            accessible == EXPECTED_DECLARED_RAW10_BYTES &&
                plane.rowStride == DECLARED_RAW10_ROW_STRIDE &&
                firstNonZero == 0L &&
                lastNonZero != null &&
                lastNonZero!! < EFFECTIVE_PREFIX_BYTES &&
                tailNonZero == 0L &&
                padNonZero == 0L &&
                rowsWithData == EFFECTIVE_ROWS

        return RawSummary(
            accessibleBytes = accessible,
            rowStride = plane.rowStride,
            pixelStride = plane.pixelStride,
            fullPlaneSha256 = hex(fullMd.digest()),
            firstNonZeroByte = firstNonZero,
            lastNonZeroByte = lastNonZero,
            tailNonZeroByteCount = tailNonZero,
            effectivePrefixSha256 = effectivePrefixSha,
            packedDataOnlySha256 = packedOnlySha,
            effectivePadNonZeroByteCount = padNonZero,
            effectiveRowsWithImageData = rowsWithData,
            maxDeclaredRowWithNonZero = maxDeclaredRow,
            decodedMin = decodedMin,
            decodedMax = decodedMax,
            decodedMean = decodedMean,
            parityMeans = parityMeans,
            knownV059TopologyMatch = knownTopologyMatch,
        )
    }

    private fun comparePair(control: FrameOutcome, candidate: FrameOutcome): JSONObject {
        val cr = control.raw ?: error("control raw missing")
        val vr = candidate.raw ?: error("candidate raw missing")
        val cc = control.capture ?: error("control capture missing")
        val vc = candidate.capture ?: error("candidate capture missing")

        val structural =
            cr.accessibleBytes != vr.accessibleBytes ||
                cr.rowStride != vr.rowStride ||
                cr.pixelStride != vr.pixelStride ||
                cr.firstNonZeroByte != vr.firstNonZeroByte ||
                cr.lastNonZeroByte != vr.lastNonZeroByte ||
                cr.tailNonZeroByteCount != vr.tailNonZeroByteCount ||
                cr.effectivePadNonZeroByteCount != vr.effectivePadNonZeroByteCount ||
                cr.effectiveRowsWithImageData != vr.effectiveRowsWithImageData ||
                cr.maxDeclaredRowWithNonZero != vr.maxDeclaredRowWithNonZero ||
                cr.knownV059TopologyMatch != vr.knownV059TopologyMatch

        val selectedMetadata =
            cc.sensorPixelMode != vc.sensorPixelMode ||
                cc.rawBinningFactorUsed != vc.rawBinningFactorUsed ||
                cc.noiseReductionMode != vc.noiseReductionMode ||
                cc.edgeMode != vc.edgeMode

        val acquisitionExact =
            cc.iso == vc.iso &&
                cc.exposureNs == vc.exposureNs &&
                cc.frameDurationNs == vc.frameDurationNs &&
                cc.focusDistanceDiopters == vc.focusDistanceDiopters

        val meanDelta = if (cr.decodedMean != null && vr.decodedMean != null) {
            vr.decodedMean - cr.decodedMean
        } else null

        return JSONObject()
            .put("structuralTopologyDifferentialObserved", structural)
            .put("selectedResultMetadataDifferentialObserved", selectedMetadata)
            .put("acquisitionStateExactlyMatched", acquisitionExact)
            .put("controlLockedControlWriteComplete", control.lockedControlWriteComplete)
            .put("candidateLockedControlWriteComplete", candidate.lockedControlWriteComplete)
            .put("fullPlaneSha256Equal", cr.fullPlaneSha256 == vr.fullPlaneSha256)
            .put("effectivePrefixSha256Equal", cr.effectivePrefixSha256 == vr.effectivePrefixSha256)
            .put("packedDataOnlySha256Equal", cr.packedDataOnlySha256 == vr.packedDataOnlySha256)
            .put("controlKnownV059TopologyMatch", cr.knownV059TopologyMatch)
            .put("candidateKnownV059TopologyMatch", vr.knownV059TopologyMatch)
            .put("sampleMeanDeltaCandidateMinusControl", meanDelta ?: JSONObject.NULL)
            .put("sampleStatisticsAttributionAllowed", acquisitionExact)
            .put("interpretation",
                if (acquisitionExact)
                    "Manual ISO/exposure/frame-duration/focus matched exactly. Remaining sample differences can be compared, but scene motion, sensor noise and unmeasured processing remain possible confounds."
                else
                    "Requested manual lock did not produce an exact matched physical result; sample-value differences are not attributable to the candidate key.")
    }

    private fun safeSizes(block: () -> Array<android.util.Size>?): List<android.util.Size> =
        try { block()?.toList().orEmpty() } catch (_: Throwable) { emptyList() }

    private fun writeCheckpoint(report: JSONObject) {
        val target = reportFile()
        val tmp = File(filesDir, REPORT_FILENAME + ".tmp")
        tmp.writeText(report.toString(2))
        if (target.exists() && !target.delete()) {
            target.writeText(report.toString(2))
            tmp.delete()
            return
        }
        if (!tmp.renameTo(target)) {
            target.writeText(report.toString(2))
            tmp.delete()
        }
    }

    private fun refreshStatus() {
        val file = reportFile()
        saveButton.isEnabled = file.exists() && file.length() > 0L
        if (!file.exists()) {
            status.text = "Nog geen v0.72 report."
            return
        }
        val report = runCatching { JSONObject(file.readText()) }.getOrNull()
        if (report == null) {
            status.text = "v0.72 report bestaat maar kon niet worden gelezen."
            return
        }
        status.text = buildString {
            append("stage=").append(report.optString("stage", "?")).append('\n')
            append("pairs=").append(report.optInt("pairsComplete", 0))
                .append("/").append(report.optInt("candidateCount", 0)).append('\n')
            append("frames=").append(report.optInt("physicalFrameCount", 0)).append('\n')
            append("exact locked pairs=").append(report.optInt("acquisitionStateExactlyMatchedPairCount", 0)).append('\n')
            append("topology diffs=").append(report.optInt("structuralTopologyDifferentialCount", 0)).append('\n')
            append("metadata diffs=").append(report.optInt("selectedResultMetadataDifferentialCount", 0)).append('\n')
            append("failed/skipped=").append(report.optInt("failedOrSkippedPairCount", 0)).append('\n')
            append(report.optString("classification", "IN_PROGRESS"))
        }
    }

    @Suppress("DEPRECATION")
    private fun saveReport() {
        val source = reportFile()
        if (!source.exists()) return
        startActivityForResult(Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(Intent.EXTRA_TITLE, source.name)
        }, REQUEST_SAVE_JSON)
    }

    @Deprecated("Document export bridge")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode != REQUEST_SAVE_JSON || resultCode != RESULT_OK) return
        val uri = data?.data ?: return
        val source = reportFile()
        runCatching {
            contentResolver.openOutputStream(uri)?.use { out ->
                source.inputStream().use { input -> input.copyTo(out) }
            } ?: error("Geen output stream")
        }.onSuccess {
            status.text = "v0.72 JSON opgeslagen."
        }.onFailure {
            status.text = "Opslaan faalde: " + it.javaClass.simpleName + ": " + it.message
        }
    }

    private fun reportFile(): File = File(filesDir, REPORT_FILENAME)

    private fun button(text: String, action: () -> Unit): Button = Button(this).apply {
        this.text = text
        isAllCaps = false
        minHeight = dp(52)
        setOnClickListener { action() }
    }

    private fun label(text: String, size: Float, bold: Boolean, color: Int = Color.WHITE): TextView =
        TextView(this).apply {
            this.text = text
            textSize = size
            setTextColor(color)
            if (bold) setTypeface(typeface, android.graphics.Typeface.BOLD)
        }

    private fun space(height: Int): View =
        View(this).apply { layoutParams = LinearLayout.LayoutParams(1, dp(height)) }

    private fun dp(v: Int): Int = (v * resources.displayMetrics.density).toInt()

    private enum class ValueType(val label: String) {
        INT_ARRAY("int[]"),
        BYTE_ARRAY("byte[]"),
    }

    private data class CandidateSpec(
        val shortName: String,
        val keyName: String,
        val type: ValueType,
        val semanticBasis: String,
        val intValue: Int,
    ) {
        fun value(): Any = when (type) {
            ValueType.INT_ARRAY -> intArrayOf(intValue)
            ValueType.BYTE_ARRAY -> byteArrayOf(intValue.toByte())
        }
    }

    companion object {
        private const val SCHEMA = "truthraw.physical5-semantic-locked-capture-effect.v0.72"
        private const val AUTHORITY = "CAMERA2_SINGLE_FRAME_SEMANTIC_VALUE_LOCKED_CONTROL_CANDIDATE_EFFECT"
        private const val REPORT_FILENAME = "TRUTHRAW_PHYSICAL5_OEM_ORCHESTRATION_v072.json"
        private const val REQUEST_CAMERA_PERMISSION = 67262
        private const val REQUEST_SAVE_JSON = 67263
        private const val LOGICAL_ID = "0"
        private const val PHYSICAL_ID = "5"

        private const val TARGET_W = 16320
        private const val TARGET_H = 12288
        private const val DECLARED_RAW10_ROW_STRIDE = 20400
        private const val EXPECTED_DECLARED_RAW10_BYTES = 250675200L

        private const val EFFECTIVE_ROWS = 3072
        private const val EFFECTIVE_ROW_BYTES = 5120
        private const val EFFECTIVE_PACKED_BYTES_PER_ROW = 5100
        private const val EFFECTIVE_PREFIX_BYTES = 15728640L

        private const val BOUNDARY =
            "A_LOCKED_CONTROL_CANDIDATE_DIFFERENTIAL_CAN_ESTABLISH_AN_APP_VISIBLE_EFFECT_IN_THIS_EXACT_CONTEXT_ONLY; STATIC_OEM_SEMANTICS_DO_NOT_PROVE_RUNTIME_EFFECT; NO_NATIVE_SENSOR_GEOMETRY, DIRECT_CFA_200MP, ADC_BIT_DEPTH, OR_CALIBRATION_TRUTH_IS_INFERRED"

        private val CANDIDATES = listOf(
            CandidateSpec(
                "MasterFilmSensorType_tele_3",
                "com.hihonor.capture.metadata.MasterFilmSensorType",
                ValueType.INT_ARRAY,
                "HONOR .452 CameraUtil.getMasterVideoSensorType(): OEM tele role -> 3",
                3
            ),
            CandidateSpec(
                "cameraSceneMode_UltraHighPixel_53",
                "com.hihonor.capture.metadata.cameraSceneMode",
                ValueType.INT_ARRAY,
                "HONOR .452 CameraSceneModeUtil.getHighPixelSceneMode(): UltraHighPixel/200M -> 53",
                53
            ),
            CandidateSpec(
                "cameraSceneMode_UltraResolution_110",
                "com.hihonor.capture.metadata.cameraSceneMode",
                ValueType.INT_ARRAY,
                "HONOR .452 CameraSceneModeUtil.getHighPixelSceneMode(): UltraResolution/50M -> 110",
                110
            ),
            CandidateSpec(
                "cameraSceneMode_ProRaw_66",
                "com.hihonor.capture.metadata.cameraSceneMode",
                ValueType.INT_ARRAY,
                "HONOR .452 CameraSceneModeUtil.getProPhotoSceneMode(): RAW branch -> 66",
                66
            ),
            CandidateSpec(
                "teleconverterEnable_on",
                "com.hihonor.capture.metadata.teleconverterEnable",
                ValueType.BYTE_ARRAY,
                "HONOR .452 TeleConverterFunction: \"on\" -> Boolean true; direct Camera2 type resolved byte[]",
                1
            ),
        )

        private fun jsonValue(value: Any?): Any = when (value) {
            null -> JSONObject.NULL
            is IntArray -> JSONArray(value.toList())
            is ByteArray -> JSONArray(value.map { it.toInt() and 0xff })
            is Number, is Boolean, is String -> value
            else -> value.toString()
        }

        private fun hex(bytes: ByteArray): String =
            bytes.joinToString("") { b -> "%02x".format(b) }
    }
}
