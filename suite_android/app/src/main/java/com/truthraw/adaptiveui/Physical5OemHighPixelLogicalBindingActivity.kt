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
 * v0.72 tests the binding dimension missing from v0.71.
 *
 * Exact HONOR .452 consumer code writes high-pixel vendor parameters through Mode.CaptureFlow
 * without an explicit physical-ID binding at that callsite. This probe therefore tests:
 * - logical-only MasterFilmSensorType=3
 * - logical-only cameraSceneMode=53 (UltraHighPixel/200M)
 * - logical-only qcomRemosaicEnable=1
 * - logical-only cameraSceneMode=53 + qcomRemosaicEnable=1
 * - mirrored logical+physical cameraSceneMode=53 + qcomRemosaicEnable=1
 *
 * Every comparison remains one control frame versus one candidate frame, fresh camera open per
 * frame, same manual acquisition request state, physical Camera-5 RAW10 16320x12288 envelope.
 */
class Physical5OemHighPixelLogicalBindingActivity : Activity() {
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
        body.addView(label("TruthRaw v0.72 · OEM high-pixel binding", 21f, true))
        body.addView(label(
            "Test nu de ontbrekende OEM-bindingsdimensie: logical CaptureFlow-achtige writes en een minimale scene53 + qcomRemosaicEnable combinatie. Iedere zijde blijft single-frame en geïsoleerd.",
            12f, false, Color.rgb(190, 198, 210)
        ))
        body.addView(space(10))
        body.addView(button("1 · Run logical-binding matrix") { runMatrix() })
        saveButton = button("2 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)
        body.addView(space(10))
        body.addView(label(
            "De test leest ook HONOR hintUserValue terug wanneer die result-key beschikbaar is. Geen multi-frame, geen Scientific-Master-mutatie.",
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
        val raw10 = safeSizes { maximumMap?.getOutputSizes(ImageFormat.RAW10) } +
            safeSizes { maximumMap?.getHighResolutionOutputSizes(ImageFormat.RAW10) }
        require(raw10.any { it.width == TARGET_W && it.height == TARGET_H }) {
            "physical camera 5 does not advertise 16320x12288 RAW10 maximum-resolution output"
        }

        val logicalRequestByName = logical.availableCaptureRequestKeys.orEmpty().associateBy { it.name }
        val physicalRequestByName = physical.availableCaptureRequestKeys.orEmpty().associateBy { it.name }
        val logicalSessionNames = logical.availableSessionKeys.orEmpty().map { it.name }.toSet()
        val physicalSessionNames = physical.availableSessionKeys.orEmpty().map { it.name }.toSet()
        val logicalResultByName = logical.availableCaptureResultKeys.orEmpty().associateBy { it.name }
        val physicalResultByName = physical.availableCaptureResultKeys.orEmpty().associateBy { it.name }

        val locked = chooseLockedSettings(physical)
        val report = JSONObject()
            .put("schema", SCHEMA)
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", AUTHORITY)
            .put("logicalCameraId", LOGICAL_ID)
            .put("physicalCameraId", PHYSICAL_ID)
            .put("target", JSONObject()
                .put("format", "RAW10")
                .put("width", TARGET_W)
                .put("height", TARGET_H)
                .put("physicalOutputBinding", true)
                .put("physicalSensorPixelModeRequested", CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION))
            .put("lockedRequest", locked.toJson())
            .put("surface", JSONObject()
                .put("MasterFilmSensorType", surfaceInfo(MASTER_FILM, logicalRequestByName, physicalRequestByName, logicalSessionNames, physicalSessionNames))
                .put("cameraSceneMode", surfaceInfo(CAMERA_SCENE, logicalRequestByName, physicalRequestByName, logicalSessionNames, physicalSessionNames))
                .put("qcomRemosaicEnable", surfaceInfo(QCOM_REMOSAIC, logicalRequestByName, physicalRequestByName, logicalSessionNames, physicalSessionNames))
                .put("hintUserValueLogicalResultPresent", logicalResultByName.containsKey(HINT_USER_VALUE))
                .put("hintUserValuePhysicalResultPresent", physicalResultByName.containsKey(HINT_USER_VALUE)))
            .put("freshCameraOpenPerFrame", true)
            .put("mirroredPairOrder", true)
            .put("candidateCount", CANDIDATES.size)
            .put("results", JSONArray())
            .put("pairsComplete", 0)
            .put("physicalFrameCount", 0)
            .put("independentEvidenceCount", 0)
            .put("multiFrameFusionUsed", false)
            .put("rawSourceFilesPersisted", false)
            .put("scientificMasterModified", false)
            .put("calibrationAuthorityGranted", false)
            .put("stage", "INITIALIZED")
            .put("boundary", BOUNDARY)
        writeCheckpoint(report)

        val results = report.getJSONArray("results")
        var pairs = 0
        var frames = 0
        var topologyDiffs = 0
        var metadataDiffs = 0
        var exactPairs = 0
        var failed = 0
        var hintDiffs = 0

        for ((index, spec) in CANDIDATES.withIndex()) {
            runOnUiThread {
                status.text = "v0.72 " + (index + 1) + "/" + CANDIDATES.size + " · " + spec.shortName
            }

            val item = JSONObject()
                .put("index", index)
                .put("shortName", spec.shortName)
                .put("semanticBasis", spec.semanticBasis)
                .put("binding", spec.binding.name)
                .put("assignments", JSONArray(spec.assignments.map { it.toJson() }))
                .put("order", if (index % 2 == 0) "CONTROL_THEN_CANDIDATE" else "CANDIDATE_THEN_CONTROL")
                .put("pairCompleted", false)
            results.put(item)

            report
                .put("candidateIndexInFlight", index)
                .put("candidateNameInFlight", spec.shortName)
                .put("stage", "BEFORE_PAIR")
            writeCheckpoint(report)

            val control: FrameOutcome
            val candidate: FrameOutcome

            if (index % 2 == 0) {
                control = captureFresh(
                    cm, logical, physical, locked,
                    logicalRequestByName, physicalRequestByName,
                    logicalSessionNames, physicalSessionNames,
                    logicalResultByName, physicalResultByName,
                    emptyList(), Binding.LOGICAL_ONLY, "CONTROL"
                )
                if (control.frameCaptured) frames++
                item.put("control", control.toJson())
                report.put("physicalFrameCount", frames).put("independentEvidenceCount", frames).put("stage", "CONTROL_RETURNED")
                writeCheckpoint(report)

                candidate = captureFresh(
                    cm, logical, physical, locked,
                    logicalRequestByName, physicalRequestByName,
                    logicalSessionNames, physicalSessionNames,
                    logicalResultByName, physicalResultByName,
                    spec.assignments, spec.binding, spec.shortName
                )
                if (candidate.frameCaptured) frames++
                item.put("candidate", candidate.toJson())
            } else {
                candidate = captureFresh(
                    cm, logical, physical, locked,
                    logicalRequestByName, physicalRequestByName,
                    logicalSessionNames, physicalSessionNames,
                    logicalResultByName, physicalResultByName,
                    spec.assignments, spec.binding, spec.shortName
                )
                if (candidate.frameCaptured) frames++
                item.put("candidate", candidate.toJson())
                report.put("physicalFrameCount", frames).put("independentEvidenceCount", frames).put("stage", "CANDIDATE_RETURNED")
                writeCheckpoint(report)

                control = captureFresh(
                    cm, logical, physical, locked,
                    logicalRequestByName, physicalRequestByName,
                    logicalSessionNames, physicalSessionNames,
                    logicalResultByName, physicalResultByName,
                    emptyList(), Binding.LOGICAL_ONLY, "CONTROL"
                )
                if (control.frameCaptured) frames++
                item.put("control", control.toJson())
            }

            report.put("physicalFrameCount", frames).put("independentEvidenceCount", frames).put("stage", "BOTH_FRAMES_RETURNED")
            writeCheckpoint(report)

            if (!control.frameCaptured || !candidate.frameCaptured ||
                control.capture == null || candidate.capture == null ||
                control.raw == null || candidate.raw == null
            ) {
                failed++
                item.put("classification", "PAIR_CAPTURE_FAILED")
                report.put("failedOrSkippedPairCount", failed).put("stage", "PAIR_CAPTURE_FAILED")
                writeCheckpoint(report)
                continue
            }

            val cmp = comparePair(control, candidate)
            item.put("comparison", cmp)
                .put("pairCompleted", true)
                .put("classification", "OEM_BINDING_CONTROL_CANDIDATE_PAIR_COMPLETE")

            pairs++
            if (cmp.optBoolean("structuralTopologyDifferentialObserved", false)) topologyDiffs++
            if (cmp.optBoolean("selectedResultMetadataDifferentialObserved", false)) metadataDiffs++
            if (cmp.optBoolean("acquisitionStateExactlyMatched", false)) exactPairs++
            if (cmp.optBoolean("hintUserValueDifferentialObserved", false)) hintDiffs++

            report
                .put("pairsComplete", pairs)
                .put("physicalFrameCount", frames)
                .put("independentEvidenceCount", frames)
                .put("structuralTopologyDifferentialCount", topologyDiffs)
                .put("selectedResultMetadataDifferentialCount", metadataDiffs)
                .put("acquisitionStateExactlyMatchedPairCount", exactPairs)
                .put("hintUserValueDifferentialCount", hintDiffs)
                .put("failedOrSkippedPairCount", failed)
                .put("stage", "PAIR_RECORDED")
            writeCheckpoint(report)
        }

        return report
            .put("candidateIndexInFlight", JSONObject.NULL)
            .put("candidateNameInFlight", JSONObject.NULL)
            .put("stage", "MATRIX_COMPLETE")
            .put("classification", "PHYSICAL5_OEM_HIGHPIXEL_LOGICAL_BINDING_MATRIX_COMPLETE_OR_PARTIAL")
    }

    private fun surfaceInfo(
        name: String,
        logicalRequestByName: Map<String, CaptureRequest.Key<*>>,
        physicalRequestByName: Map<String, CaptureRequest.Key<*>>,
        logicalSessionNames: Set<String>,
        physicalSessionNames: Set<String>,
    ): JSONObject = JSONObject()
        .put("logicalRequestPresent", logicalRequestByName.containsKey(name))
        .put("physicalRequestPresent", physicalRequestByName.containsKey(name))
        .put("logicalSessionPresent", name in logicalSessionNames)
        .put("physicalSessionPresent", name in physicalSessionNames)

    private data class LockedSettings(
        val iso: Int,
        val exposureNs: Long,
        val frameDurationNs: Long,
        val focusDistanceDiopters: Float,
    ) {
        fun toJson(): JSONObject = JSONObject()
            .put("iso", iso)
            .put("exposureTimeNs", exposureNs)
            .put("frameDurationNs", frameDurationNs)
            .put("focusDistanceDiopters", focusDistanceDiopters.toDouble())
    }

    private fun chooseLockedSettings(physical: CameraCharacteristics): LockedSettings {
        val isoRange: Range<Int> = physical.get(CameraCharacteristics.SENSOR_INFO_SENSITIVITY_RANGE)
            ?: error("SENSOR_INFO_SENSITIVITY_RANGE missing")
        val exposureRange: Range<Long> = physical.get(CameraCharacteristics.SENSOR_INFO_EXPOSURE_TIME_RANGE)
            ?: error("SENSOR_INFO_EXPOSURE_TIME_RANGE missing")
        val maxFrame = physical.get(CameraCharacteristics.SENSOR_INFO_MAX_FRAME_DURATION)
            ?: error("SENSOR_INFO_MAX_FRAME_DURATION missing")

        val aeModes = physical.get(CameraCharacteristics.CONTROL_AE_AVAILABLE_MODES) ?: intArrayOf()
        val afModes = physical.get(CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES) ?: intArrayOf()
        require(aeModes.contains(CameraMetadata.CONTROL_AE_MODE_OFF)) { "physical-5 AE OFF unavailable" }
        require(afModes.contains(CameraMetadata.CONTROL_AF_MODE_OFF)) { "physical-5 AF OFF unavailable" }

        val iso = 800.coerceIn(isoRange.lower, isoRange.upper)
        val exposure = 10_000_000L.coerceIn(exposureRange.lower, exposureRange.upper)
        val frame = min(max(33_322_225L, exposure), maxFrame)
        return LockedSettings(iso, exposure, frame, 1.0f)
    }

    private data class OpenResult(
        val device: CameraDevice?,
        val executor: ExecutorService?,
        val closedLatch: CountDownLatch?,
        val error: String?,
    )

    private fun openLogicalCamera(cm: CameraManager): OpenResult {
        val opened = CountDownLatch(1)
        val closed = CountDownLatch(1)
        val executor = Executors.newSingleThreadExecutor()
        var device: CameraDevice? = null
        var error: String? = null

        try {
            cm.openCamera(LOGICAL_ID, executor, object : CameraDevice.StateCallback() {
                override fun onOpened(camera: CameraDevice) {
                    device = camera
                    opened.countDown()
                }
                override fun onDisconnected(camera: CameraDevice) {
                    error = "CAMERA_DISCONNECTED"
                    camera.close()
                    opened.countDown()
                }
                override fun onError(camera: CameraDevice, code: Int) {
                    error = "CAMERA_OPEN_ERROR_" + code
                    camera.close()
                    opened.countDown()
                }
                override fun onClosed(camera: CameraDevice) {
                    closed.countDown()
                }
            })
            if (!opened.await(6, TimeUnit.SECONDS)) error = "CAMERA_OPEN_TIMEOUT"
        } catch (e: Throwable) {
            error = e.javaClass.name + ": " + e.message
        }

        if (device == null) {
            executor.shutdownNow()
            return OpenResult(null, null, null, error)
        }
        return OpenResult(device, executor, closed, error)
    }

    private fun captureFresh(
        cm: CameraManager,
        logical: CameraCharacteristics,
        physical: CameraCharacteristics,
        locked: LockedSettings,
        logicalRequestByName: Map<String, CaptureRequest.Key<*>>,
        physicalRequestByName: Map<String, CaptureRequest.Key<*>>,
        logicalSessionNames: Set<String>,
        physicalSessionNames: Set<String>,
        logicalResultByName: Map<String, CaptureResult.Key<*>>,
        physicalResultByName: Map<String, CaptureResult.Key<*>>,
        assignments: List<Assignment>,
        binding: Binding,
        label: String,
    ): FrameOutcome {
        val opened = openLogicalCamera(cm)
        val device = opened.device ?: return FrameOutcome(
            label, false, false, JSONArray(), false, null, null, null,
            "CAMERA_OPEN_FAILED", opened.error
        )

        return try {
            runSingleFrame(
                device, opened.executor!!, logical, physical, locked,
                logicalRequestByName, physicalRequestByName,
                logicalSessionNames, physicalSessionNames,
                logicalResultByName, physicalResultByName,
                assignments, binding, label
            )
        } finally {
            device.close()
            opened.closedLatch?.await(2, TimeUnit.SECONDS)
            opened.executor?.shutdown()
            opened.executor?.awaitTermination(2, TimeUnit.SECONDS)
            opened.executor?.shutdownNow()
        }
    }

    private data class FrameOutcome(
        val label: String,
        val sessionConfigured: Boolean,
        val frameCaptured: Boolean,
        val assignmentWrites: JSONArray,
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
            .put("assignmentWrites", assignmentWrites)
            .put("lockedControlWriteComplete", lockedControlWriteComplete)
            .put("capture", capture?.toJson() ?: JSONObject.NULL)
            .put("raw", raw?.toJson() ?: JSONObject.NULL)
            .put("errorClass", errorClass ?: JSONObject.NULL)
            .put("errorMessage", errorMessage ?: JSONObject.NULL)
    }

    private data class CaptureSummary(
        val timestampNs: Long?,
        val imageTimestampNs: Long?,
        val iso: Int?,
        val exposureNs: Long?,
        val frameDurationNs: Long?,
        val focusDistanceDiopters: Float?,
        val sensorPixelMode: Int?,
        val rawBinningFactorUsed: Boolean?,
        val noiseReductionMode: Int?,
        val edgeMode: Int?,
        val dynamicBlackLevel: FloatArray?,
        val physicalResultCameraId: String?,
        val logicalHintUserValue: Any?,
        val physicalHintUserValue: Any?,
    ) {
        fun toJson(): JSONObject = JSONObject()
            .put("sensorTimestampNs", timestampNs ?: JSONObject.NULL)
            .put("imageTimestampNs", imageTimestampNs ?: JSONObject.NULL)
            .put("timestampIdentityPass", timestampNs != null && timestampNs == imageTimestampNs)
            .put("iso", iso ?: JSONObject.NULL)
            .put("exposureTimeNs", exposureNs ?: JSONObject.NULL)
            .put("frameDurationNs", frameDurationNs ?: JSONObject.NULL)
            .put("focusDistanceDiopters", focusDistanceDiopters?.toDouble() ?: JSONObject.NULL)
            .put("sensorPixelMode", sensorPixelMode ?: JSONObject.NULL)
            .put("rawBinningFactorUsed", rawBinningFactorUsed ?: JSONObject.NULL)
            .put("noiseReductionMode", noiseReductionMode ?: JSONObject.NULL)
            .put("edgeMode", edgeMode ?: JSONObject.NULL)
            .put("dynamicBlackLevel", dynamicBlackLevel?.let { JSONArray(it.map { v -> v.toDouble() }) } ?: JSONObject.NULL)
            .put("physicalResultCameraId", physicalResultCameraId ?: JSONObject.NULL)
            .put("logicalHintUserValue", jsonValue(logicalHintUserValue))
            .put("physicalHintUserValue", jsonValue(physicalHintUserValue))
    }

    private data class RawSummary(
        val accessibleBytes: Long,
        val rowStride: Int,
        val pixelStride: Int,
        val fullPlaneSha256: String,
        val firstNonZeroByte: Long?,
        val lastNonZeroByte: Long?,
        val tailNonZeroByteCount: Long,
        val effectivePadNonZeroByteCount: Long?,
        val effectiveRowsWithImageData: Int?,
        val maxDeclaredRowWithNonZero: Int?,
        val decodedMean: Double?,
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
            .put("effectivePadNonZeroByteCount", effectivePadNonZeroByteCount ?: JSONObject.NULL)
            .put("effectiveRowsWithImageData", effectiveRowsWithImageData ?: JSONObject.NULL)
            .put("maxDeclaredRowWithNonZero", maxDeclaredRowWithNonZero ?: JSONObject.NULL)
            .put("decodedMean", decodedMean ?: JSONObject.NULL)
            .put("knownV059TopologyMatch", knownV059TopologyMatch)
    }

    private fun runSingleFrame(
        device: CameraDevice,
        executor: ExecutorService,
        logical: CameraCharacteristics,
        physical: CameraCharacteristics,
        locked: LockedSettings,
        logicalRequestByName: Map<String, CaptureRequest.Key<*>>,
        physicalRequestByName: Map<String, CaptureRequest.Key<*>>,
        logicalSessionNames: Set<String>,
        physicalSessionNames: Set<String>,
        logicalResultByName: Map<String, CaptureResult.Key<*>>,
        physicalResultByName: Map<String, CaptureResult.Key<*>>,
        assignments: List<Assignment>,
        binding: Binding,
        label: String,
    ): FrameOutcome {
        val writes = JSONArray()
        val reader = runCatching { ImageReader.newInstance(TARGET_W, TARGET_H, ImageFormat.RAW10, 1) }
            .getOrElse {
                return FrameOutcome(label, false, false, writes, false, null, null, it.javaClass.name, it.message)
            }

        val imageRef = AtomicReference<Image?>(null)
        val imageLatch = CountDownLatch(1)
        reader.setOnImageAvailableListener({ source ->
            val image = runCatching { source.acquireNextImage() }.getOrNull()
            if (image != null && imageRef.compareAndSet(null, image)) imageLatch.countDown() else image?.close()
        }, Handler(Looper.getMainLooper()))

        val sessionLatch = CountDownLatch(1)
        val sessionClosed = CountDownLatch(1)
        val sessionRef = AtomicReference<CameraCaptureSession?>(null)
        var sessionConfigured = false

        val output = OutputConfiguration(reader.surface)
        try {
            output.setPhysicalCameraId(PHYSICAL_ID)
            output.addSensorPixelModeUsed(CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
        } catch (e: Throwable) {
            reader.close()
            return FrameOutcome(label, false, false, writes, false, null, null, e.javaClass.name, e.message)
        }

        val config = SessionConfiguration(
            SessionConfiguration.SESSION_REGULAR,
            listOf(output),
            executor,
            object : CameraCaptureSession.StateCallback() {
                override fun onConfigured(session: CameraCaptureSession) {
                    sessionConfigured = true
                    sessionRef.set(session)
                    sessionLatch.countDown()
                }
                override fun onConfigureFailed(session: CameraCaptureSession) {
                    sessionRef.set(session)
                    sessionLatch.countDown()
                }
                override fun onClosed(session: CameraCaptureSession) {
                    sessionClosed.countDown()
                }
            }
        )

        try {
            if (assignments.isNotEmpty()) {
                val sb = device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE, setOf(PHYSICAL_ID))
                var sessionValueWritten = false
                for (a in assignments) {
                    val logKey = logicalRequestByName[a.keyName]
                    val phyKey = physicalRequestByName[a.keyName]
                    if (a.keyName in logicalSessionNames && logKey != null) {
                        @Suppress("UNCHECKED_CAST")
                        sb.set(logKey as CaptureRequest.Key<Any>, a.value())
                        sessionValueWritten = true
                    }
                    if (binding == Binding.MIRRORED_LOGICAL_PHYSICAL &&
                        a.keyName in physicalSessionNames && phyKey != null
                    ) {
                        @Suppress("UNCHECKED_CAST")
                        sb.setPhysicalCameraKey(phyKey as CaptureRequest.Key<Any>, a.value(), PHYSICAL_ID)
                        sessionValueWritten = true
                    }
                }
                if (sessionValueWritten) config.setSessionParameters(sb.build())
            }

            device.createCaptureSession(config)
            if (!sessionLatch.await(8, TimeUnit.SECONDS) || !sessionConfigured) {
                runCatching { sessionRef.get()?.close() }
                sessionClosed.await(2, TimeUnit.SECONDS)
                reader.close()
                return FrameOutcome(label, false, false, writes, false, null, null, "SESSION_CONFIG_FAILED", null)
            }

            val session = sessionRef.get() ?: error("configured session missing")
            val b = device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE, setOf(PHYSICAL_ID))
            b.addTarget(reader.surface)

            b.setPhysicalCameraKey(CaptureRequest.SENSOR_PIXEL_MODE, CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION, PHYSICAL_ID)
            val lockedOk = applyLockedControls(b, locked)

            for (a in assignments) {
                val entry = JSONObject()
                    .put("key", a.keyName)
                    .put("value", jsonValue(a.value()))
                    .put("binding", binding.name)

                val logicalKey = logicalRequestByName[a.keyName] ?: physicalRequestByName[a.keyName]
                var logicalWritten = false
                var physicalWritten = false
                var logicalReadback: Any? = null
                var physicalReadback: Any? = null

                if (logicalKey != null) {
                    @Suppress("UNCHECKED_CAST")
                    val k = logicalKey as CaptureRequest.Key<Any>
                    runCatching {
                        b.set(k, a.value())
                        logicalWritten = true
                        logicalReadback = b.get(k)
                    }.onFailure {
                        entry.put("logicalWriteError", it.javaClass.name + ": " + it.message)
                    }
                }

                if (binding == Binding.MIRRORED_LOGICAL_PHYSICAL) {
                    val pk = physicalRequestByName[a.keyName] ?: logicalRequestByName[a.keyName]
                    if (pk != null) {
                        @Suppress("UNCHECKED_CAST")
                        val k = pk as CaptureRequest.Key<Any>
                        runCatching {
                            b.setPhysicalCameraKey(k, a.value(), PHYSICAL_ID)
                            physicalWritten = true
                            physicalReadback = b.getPhysicalCameraKey(k, PHYSICAL_ID)
                        }.onFailure {
                            entry.put("physicalWriteError", it.javaClass.name + ": " + it.message)
                        }
                    }
                }

                entry
                    .put("logicalWritten", logicalWritten)
                    .put("logicalReadback", jsonValue(logicalReadback))
                    .put("physicalWritten", physicalWritten)
                    .put("physicalReadback", jsonValue(physicalReadback))
                writes.put(entry)
            }

            val resultRef = AtomicReference<TotalCaptureResult?>(null)
            val resultLatch = CountDownLatch(1)
            var failureText: String? = null

            session.captureSingleRequest(
                b.build(),
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
                        failureText = "reason=" + failure.reason + " wasImageCaptured=" + failure.wasImageCaptured()
                        resultLatch.countDown()
                    }
                }
            )

            val resultReady = resultLatch.await(10, TimeUnit.SECONDS)
            val imageReady = imageLatch.await(10, TimeUnit.SECONDS)
            val total = resultRef.get()
            val image = imageRef.get()

            if (!resultReady || !imageReady || total == null || image == null) {
                image?.close()
                session.close()
                sessionClosed.await(2, TimeUnit.SECONDS)
                reader.close()
                return FrameOutcome(
                    label, true, false, writes, lockedOk, null, null,
                    "CAPTURE_PAIRING_FAILED", failureText ?: "resultReady=$resultReady imageReady=$imageReady"
                )
            }

            try {
                val pr = total.physicalCameraResults[PHYSICAL_ID] ?: error("physical Camera-5 result missing")
                val ts = pr.get(CaptureResult.SENSOR_TIMESTAMP)
                require(ts == image.timestamp) { "physical timestamp $ts != image timestamp undefined" }

                val logicalHint = getVendorResult(total, logicalResultByName[HINT_USER_VALUE])
                val physicalHint = getVendorResult(pr, physicalResultByName[HINT_USER_VALUE])

                val capture = CaptureSummary(
                    timestampNs = ts,
                    imageTimestampNs = image.timestamp,
                    iso = pr.get(CaptureResult.SENSOR_SENSITIVITY),
                    exposureNs = pr.get(CaptureResult.SENSOR_EXPOSURE_TIME),
                    frameDurationNs = pr.get(CaptureResult.SENSOR_FRAME_DURATION),
                    focusDistanceDiopters = pr.get(CaptureResult.LENS_FOCUS_DISTANCE),
                    sensorPixelMode = pr.get(CaptureResult.SENSOR_PIXEL_MODE),
                    rawBinningFactorUsed = pr.get(CaptureResult.SENSOR_RAW_BINNING_FACTOR_USED),
                    noiseReductionMode = pr.get(CaptureResult.NOISE_REDUCTION_MODE),
                    edgeMode = pr.get(CaptureResult.EDGE_MODE),
                    dynamicBlackLevel = pr.get(CaptureResult.SENSOR_DYNAMIC_BLACK_LEVEL),
                    physicalResultCameraId = pr.cameraId,
                    logicalHintUserValue = logicalHint,
                    physicalHintUserValue = physicalHint,
                )
                val raw = analyzeRaw10(image)

                return FrameOutcome(label, true, true, writes, lockedOk, capture, raw, null, null)
            } finally {
                image.close()
                session.close()
                sessionClosed.await(2, TimeUnit.SECONDS)
                reader.close()
            }
        } catch (e: Throwable) {
            runCatching { imageRef.getAndSet(null)?.close() }
            runCatching { sessionRef.get()?.close() }
            sessionClosed.await(2, TimeUnit.SECONDS)
            reader.close()
            return FrameOutcome(label, sessionConfigured, false, writes, false, null, null, e.javaClass.name, e.message)
        }
    }

    private fun applyLockedControls(builder: CaptureRequest.Builder, locked: LockedSettings): Boolean {
        fun <T> setBoth(key: CaptureRequest.Key<T>, value: T) {
            builder.set(key, value)
            builder.setPhysicalCameraKey(key, value, PHYSICAL_ID)
        }
        setBoth(CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_OFF)
        setBoth(CaptureRequest.SENSOR_SENSITIVITY, locked.iso)
        setBoth(CaptureRequest.SENSOR_EXPOSURE_TIME, locked.exposureNs)
        setBoth(CaptureRequest.SENSOR_FRAME_DURATION, locked.frameDurationNs)
        setBoth(CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_OFF)
        setBoth(CaptureRequest.LENS_FOCUS_DISTANCE, locked.focusDistanceDiopters)
        setBoth(CaptureRequest.NOISE_REDUCTION_MODE, CameraMetadata.NOISE_REDUCTION_MODE_OFF)
        setBoth(CaptureRequest.EDGE_MODE, CameraMetadata.EDGE_MODE_OFF)
        return true
    }

    private fun getVendorResult(
        result: CaptureResult,
        key: CaptureResult.Key<*>?,
    ): Any? {
        if (key == null) return null
        @Suppress("UNCHECKED_CAST")
        return runCatching { result.get(key as CaptureResult.Key<Any>) }.getOrNull()
    }

    private fun analyzeRaw10(image: Image): RawSummary {
        val p = image.planes.singleOrNull() ?: error("RAW10 plane count != 1")
        val src = p.buffer.duplicate().apply { rewind() }
        val accessible = src.remaining().toLong()
        val md = MessageDigest.getInstance("SHA-256")
        val scratch = ByteArray(1024 * 1024)
        var absolute = 0L
        var first: Long? = null
        var last: Long? = null
        var tail = 0L
        var maxDeclaredRow: Int? = null

        while (src.hasRemaining()) {
            val n = minOf(src.remaining(), scratch.size)
            src.get(scratch, 0, n)
            md.update(scratch, 0, n)
            for (i in 0 until n) {
                if (scratch[i].toInt() != 0) {
                    val pos = absolute + i
                    if (first == null) first = pos
                    last = pos
                    if (pos >= EFFECTIVE_PREFIX_BYTES) tail++
                    val row = (pos / p.rowStride.toLong()).toInt()
                    if (maxDeclaredRow == null || row > maxDeclaredRow!!) maxDeclaredRow = row
                }
            }
            absolute += n
        }

        var padNonZero: Long? = null
        var rowsWithData: Int? = null
        var decodedMean: Double? = null

        if (accessible >= EFFECTIVE_PREFIX_BYTES) {
            val row = ByteArray(EFFECTIVE_ROW_BYTES)
            val buf = p.buffer.duplicate().apply { rewind() }
            var pads = 0L
            var rows = 0
            var sum = 0L
            var count = 0L

            for (y in 0 until EFFECTIVE_ROWS) {
                buf.position(y * EFFECTIVE_ROW_BYTES)
                buf.get(row, 0, EFFECTIVE_ROW_BYTES)
                var any = false
                for (i in 0 until EFFECTIVE_PACKED_BYTES_PER_ROW) {
                    if (row[i].toInt() != 0) { any = true; break }
                }
                if (any) rows++
                for (i in EFFECTIVE_PACKED_BYTES_PER_ROW until EFFECTIVE_ROW_BYTES) {
                    if (row[i].toInt() != 0) pads++
                }

                var i = 0
                while (i + 4 < EFFECTIVE_PACKED_BYTES_PER_ROW) {
                    val b0 = row[i].toInt() and 0xff
                    val b1 = row[i + 1].toInt() and 0xff
                    val b2 = row[i + 2].toInt() and 0xff
                    val b3 = row[i + 3].toInt() and 0xff
                    val b4 = row[i + 4].toInt() and 0xff
                    sum += (b0 shl 2) or (b4 and 3)
                    sum += (b1 shl 2) or ((b4 shr 2) and 3)
                    sum += (b2 shl 2) or ((b4 shr 4) and 3)
                    sum += (b3 shl 2) or ((b4 shr 6) and 3)
                    count += 4
                    i += 5
                }
            }
            padNonZero = pads
            rowsWithData = rows
            decodedMean = if (count > 0) sum.toDouble() / count.toDouble() else null
        }

        val known =
            accessible == EXPECTED_DECLARED_RAW10_BYTES &&
                p.rowStride == DECLARED_RAW10_ROW_STRIDE &&
                first == 0L &&
                last != null &&
                last!! < EFFECTIVE_PREFIX_BYTES &&
                tail == 0L &&
                padNonZero == 0L &&
                rowsWithData == EFFECTIVE_ROWS

        return RawSummary(
            accessible, p.rowStride, p.pixelStride, hex(md.digest()), first, last, tail,
            padNonZero, rowsWithData, maxDeclaredRow, decodedMean, known
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

        val metadata =
            cc.sensorPixelMode != vc.sensorPixelMode ||
                cc.rawBinningFactorUsed != vc.rawBinningFactorUsed ||
                cc.noiseReductionMode != vc.noiseReductionMode ||
                cc.edgeMode != vc.edgeMode

        val exact =
            cc.iso == vc.iso &&
                cc.exposureNs == vc.exposureNs &&
                cc.frameDurationNs == vc.frameDurationNs &&
                cc.focusDistanceDiopters == vc.focusDistanceDiopters

        val hintDiff =
            jsonValue(cc.logicalHintUserValue).toString() != jsonValue(vc.logicalHintUserValue).toString() ||
                jsonValue(cc.physicalHintUserValue).toString() != jsonValue(vc.physicalHintUserValue).toString()

        return JSONObject()
            .put("structuralTopologyDifferentialObserved", structural)
            .put("selectedResultMetadataDifferentialObserved", metadata)
            .put("acquisitionStateExactlyMatched", exact)
            .put("hintUserValueDifferentialObserved", hintDiff)
            .put("controlLogicalHintUserValue", jsonValue(cc.logicalHintUserValue))
            .put("candidateLogicalHintUserValue", jsonValue(vc.logicalHintUserValue))
            .put("controlPhysicalHintUserValue", jsonValue(cc.physicalHintUserValue))
            .put("candidatePhysicalHintUserValue", jsonValue(vc.physicalHintUserValue))
            .put("controlKnownV059TopologyMatch", cr.knownV059TopologyMatch)
            .put("candidateKnownV059TopologyMatch", vr.knownV059TopologyMatch)
            .put("sampleMeanDeltaCandidateMinusControl",
                if (cr.decodedMean != null && vr.decodedMean != null) vr.decodedMean - cr.decodedMean else JSONObject.NULL)
    }

    private enum class ValueType(val label: String) {
        INT("java.lang.Integer"),
        INT_ARRAY("int[]"),
    }

    private enum class Binding {
        LOGICAL_ONLY,
        MIRRORED_LOGICAL_PHYSICAL,
    }

    private data class Assignment(
        val keyName: String,
        val type: ValueType,
        val intValue: Int,
    ) {
        fun value(): Any = when (type) {
            ValueType.INT -> intValue
            ValueType.INT_ARRAY -> intArrayOf(intValue)
        }
        fun toJson(): JSONObject = JSONObject()
            .put("key", keyName)
            .put("representation", type.label)
            .put("value", jsonValue(value()))
    }

    private data class CandidateSpec(
        val shortName: String,
        val semanticBasis: String,
        val binding: Binding,
        val assignments: List<Assignment>,
    )

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
            append("pairs=").append(report.optInt("pairsComplete", 0)).append("/")
                .append(report.optInt("candidateCount", 0)).append('\n')
            append("frames=").append(report.optInt("physicalFrameCount", 0)).append('\n')
            append("topology diffs=").append(report.optInt("structuralTopologyDifferentialCount", 0)).append('\n')
            append("hint diffs=").append(report.optInt("hintUserValueDifferentialCount", 0)).append('\n')
            append("failed=").append(report.optInt("failedOrSkippedPairCount", 0)).append('\n')
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
        this.text = text; isAllCaps = false; minHeight = dp(52); setOnClickListener { action() }
    }
    private fun label(text: String, size: Float, bold: Boolean, color: Int = Color.WHITE): TextView =
        TextView(this).apply {
            this.text = text; textSize = size; setTextColor(color)
            if (bold) setTypeface(typeface, android.graphics.Typeface.BOLD)
        }
    private fun space(height: Int): View =
        View(this).apply { layoutParams = LinearLayout.LayoutParams(1, dp(height)) }
    private fun dp(v: Int): Int = (v * resources.displayMetrics.density).toInt()

    companion object {
        private const val SCHEMA = "truthraw.physical5-oem-highpixel-logical-binding.v0.72"
        private const val AUTHORITY = "CAMERA2_SINGLE_FRAME_OEM_DERIVED_LOGICAL_BINDING_EFFECT"
        private const val REPORT_FILENAME = "TRUTHRAW_PHYSICAL5_OEM_HIGHPIXEL_LOGICAL_BINDING_v072.json"
        private const val REQUEST_CAMERA_PERMISSION = 67262
        private const val REQUEST_SAVE_JSON = 67263
        private const val LOGICAL_ID = "0"
        private const val PHYSICAL_ID = "5"

        private const val MASTER_FILM = "com.hihonor.capture.metadata.MasterFilmSensorType"
        private const val CAMERA_SCENE = "com.hihonor.capture.metadata.cameraSceneMode"
        private const val QCOM_REMOSAIC = "com.hihonor.capture.metadata.qcomRemosaicEnable"
        private const val HINT_USER_VALUE = "com.hihonor.capture.metadata.hintUserValue"

        private const val TARGET_W = 16320
        private const val TARGET_H = 12288
        private const val DECLARED_RAW10_ROW_STRIDE = 20400
        private const val EXPECTED_DECLARED_RAW10_BYTES = 250675200L
        private const val EFFECTIVE_ROWS = 3072
        private const val EFFECTIVE_ROW_BYTES = 5120
        private const val EFFECTIVE_PACKED_BYTES_PER_ROW = 5100
        private const val EFFECTIVE_PREFIX_BYTES = 15728640L

        private const val BOUNDARY =
            "LOGICAL_OR_MIRRORED_VENDOR_BINDING_EFFECT_IS_APP_VISIBLE_EVIDENCE_ONLY; IT_DOES_NOT_PROVE_ACCESS_TO_HONOR_SERVICEHOST, NATIVE_SENSOR_GEOMETRY, DIRECT_CFA_200MP, ADC_BIT_DEPTH, OR_CALIBRATION_TRUTH"

        private val CANDIDATES = listOf(
            CandidateSpec(
                "MasterFilmSensorType_tele3_LOGICAL",
                "HONOR .452 OEM tele role value 3, now tested on logical request binding",
                Binding.LOGICAL_ONLY,
                listOf(Assignment(MASTER_FILM, ValueType.INT_ARRAY, 3))
            ),
            CandidateSpec(
                "cameraSceneMode_53_LOGICAL",
                "HONOR .452 UltraHighPixel/200M scene value 53 on logical request binding",
                Binding.LOGICAL_ONLY,
                listOf(Assignment(CAMERA_SCENE, ValueType.INT_ARRAY, 53))
            ),
            CandidateSpec(
                "qcomRemosaicEnable_1_LOGICAL",
                "HONOR .452 PhotoResolutionFunction pre-capture qcom remosaic value 1",
                Binding.LOGICAL_ONLY,
                listOf(Assignment(QCOM_REMOSAIC, ValueType.INT, 1))
            ),
            CandidateSpec(
                "scene53_plus_qcom1_LOGICAL",
                "Minimal OEM-derived logical high-pixel vector",
                Binding.LOGICAL_ONLY,
                listOf(
                    Assignment(CAMERA_SCENE, ValueType.INT_ARRAY, 53),
                    Assignment(QCOM_REMOSAIC, ValueType.INT, 1)
                )
            ),
            CandidateSpec(
                "scene53_plus_qcom1_MIRRORED",
                "Same minimal vector mirrored into logical and physical-5 request namespaces",
                Binding.MIRRORED_LOGICAL_PHYSICAL,
                listOf(
                    Assignment(CAMERA_SCENE, ValueType.INT_ARRAY, 53),
                    Assignment(QCOM_REMOSAIC, ValueType.INT, 1)
                )
            ),
        )

        private fun jsonValue(value: Any?): Any = when (value) {
            null -> JSONObject.NULL
            is IntArray -> JSONArray(value.toList())
            is ByteArray -> JSONArray(value.map { it.toInt() and 0xff })
            is LongArray -> JSONArray(value.toList())
            is FloatArray -> JSONArray(value.map { it.toDouble() })
            is DoubleArray -> JSONArray(value.toList())
            is Number, is Boolean, is String -> value
            else -> value.toString()
        }

        private fun hex(bytes: ByteArray): String =
            bytes.joinToString("") { b -> "%02x".format(b) }
    }
}
