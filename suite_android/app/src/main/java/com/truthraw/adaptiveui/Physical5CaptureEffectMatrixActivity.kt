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
import android.util.Size
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

/**
 * v0.70 physical-camera-5 capture-effect matrix.
 *
 * One APK, but every vendor candidate remains scientifically isolated:
 * - fresh logical-camera-0 open for each pair;
 * - one control RAW10 frame first;
 * - one candidate RAW10 frame second;
 * - candidate is attached as both a session parameter and the physical capture-request key;
 * - both frames use the same physical camera 5, 16320x12288 RAW10 envelope and physical
 *   SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION request topology;
 * - no multi-frame fusion and exactly one physical frame per side of a pair.
 *
 * To keep storage bounded, exact app-visible Image.Plane bytes are hashed and topology-audited
 * in memory, then the Image is closed. This probe does not export sixteen ~250 MB envelopes.
 */
class Physical5CaptureEffectMatrixActivity : Activity() {
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
        body.addView(label("TruthRaw v0.70 · Physical-5 capture-effect matrix", 21f, true))
        body.addView(label(
            "8 high-value vendor controls. Iedere kandidaat krijgt een verse camera-open en een exact control/candidate single-frame RAW10-paar. " +
                "De 16320×12288 envelope wordt in memory gehasht en topologisch gemeten; geen 4 GB tijdelijke RAW-set.",
            12f, false, Color.rgb(190, 198, 210)
        ))
        body.addView(space(10))
        body.addView(button("1 · Run 8 control/candidate RAW pairs") { runMatrix() })
        saveButton = button("2 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)
        body.addView(space(10))
        body.addView(label(
            "Deze test zoekt capture-effect. Een verschil in SHA alleen is geen vendor-effect; structurele payload/topologie en result-metadata worden apart vergeleken.",
            11f, false, Color.rgb(155, 165, 180)
        ))
        body.addView(space(10))
        status = label("Nog geen v0.70 report.", 10f, false)
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
            status.text = "v0.70 vraagt CAMERA-toestemming."
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
        status.text = "v0.70 initialiseert RAW10 capture-effect matrix…"
        Thread {
            val report = runCatching { buildReport() }.getOrElse { e ->
                JSONObject()
                    .put("schema", SCHEMA)
                    .put("createdAtUtc", Instant.now().toString())
                    .put("authority", AUTHORITY)
                    .put("classification", "V070_FATAL_ERROR")
                    .put("errorClass", e.javaClass.name)
                    .put("errorMessage", e.message ?: JSONObject.NULL)
                    .put("physicalFrameCount", 0)
                    .put("independentEvidenceCount", 0)
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

        val standardMap = physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
        val maximumMap = physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)
        val standardRaw10 = safeSizes { standardMap?.getOutputSizes(ImageFormat.RAW10) }
        val maximumRaw10 = safeSizes { maximumMap?.getOutputSizes(ImageFormat.RAW10) }
        val maximumRaw10High = safeSizes { maximumMap?.getHighResolutionOutputSizes(ImageFormat.RAW10) }
        require((maximumRaw10 + maximumRaw10High).any { it.width == TARGET_W && it.height == TARGET_H }) {
            "physical camera 5 does not advertise 16320x12288 RAW10 in maximum-resolution map"
        }

        val requestByName = physical.availableCaptureRequestKeys.orEmpty().associateBy { it.name }
        val sessionNames = physical.availableSessionKeys.orEmpty().map { it.name }.toSet()

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
                .put("declaredPixels", TARGET_W.toLong() * TARGET_H.toLong())
                .put("physicalOutputBinding", true)
                .put("outputMaximumResolutionModeDeclared", true)
                .put("physicalSensorPixelModeRequested", CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION))
            .put("advertisedRaw10", JSONObject()
                .put("standard", sizeArray(standardRaw10))
                .put("maximum", sizeArray(maximumRaw10))
                .put("maximumHighResolution", sizeArray(maximumRaw10High)))
            .put("candidateCount", CANDIDATES.size)
            .put("results", results)
            .put("candidateIndexInFlight", JSONObject.NULL)
            .put("stage", "INITIALIZED")
            .put("controlCandidatePairsComplete", 0)
            .put("physicalFrameCount", 0)
            .put("independentEvidenceCount", 0)
            .put("oneControlFramePerCandidate", true)
            .put("oneCandidateFramePerCandidate", true)
            .put("multiFrameFusionUsed", false)
            .put("rawSourceFilesPersisted", false)
            .put("fullPlaneSha256ComputedInMemory", true)
            .put("knownV059TopologyComparisonApplied", true)
            .put("scientificMasterModified", false)
            .put("calibrationAuthorityGranted", false)
            .put("boundary", BOUNDARY)
        writeCheckpoint(report)

        var pairComplete = 0
        var frames = 0
        var structuralDifferentials = 0
        var metadataDifferentials = 0
        var failedPairs = 0

        for ((index, spec) in CANDIDATES.withIndex()) {
            runOnUiThread {
                status.text = "v0.70 pair " + (index + 1) + "/" + CANDIDATES.size + " · " + spec.shortName
            }

            val item = JSONObject()
                .put("index", index)
                .put("shortName", spec.shortName)
                .put("name", spec.keyName)
                .put("resolvedRepresentation", spec.type.label)
                .put("candidateValue", jsonValue(spec.value()))
                .put("physicalRequestPresent", requestByName.containsKey(spec.keyName))
                .put("physicalSessionPresent", spec.keyName in sessionNames)
                .put("pairCompleted", false)

            results.put(item)
            report
                .put("candidateIndexInFlight", index)
                .put("candidateNameInFlight", spec.keyName)
                .put("stage", "BEFORE_PAIR")
            writeCheckpoint(report)

            val keyAny = requestByName[spec.keyName]
            if (keyAny == null || spec.keyName !in sessionNames) {
                failedPairs++
                item.put("classification", "SKIPPED_KEY_NOT_AVAILABLE_FOR_REQUIRED_REQUEST_AND_SESSION_SURFACES")
                report.put("stage", "PAIR_SKIPPED_KEY_SURFACE")
                writeCheckpoint(report)
                continue
            }

            @Suppress("UNCHECKED_CAST")
            val key = keyAny as CaptureRequest.Key<Any>

            val opened = openLogicalCamera(cm)
            val device = opened.device
            if (device == null) {
                failedPairs++
                item.put("classification", "CAMERA_OPEN_FAILED")
                    .put("cameraOpenError", opened.error ?: JSONObject.NULL)
                report.put("stage", "PAIR_CAMERA_OPEN_FAILED")
                writeCheckpoint(report)
                continue
            }

            try {
                report.put("stage", "BEFORE_CONTROL_CAPTURE")
                writeCheckpoint(report)

                val control = runSingleFrame(
                    device = device,
                    executor = opened.executor!!,
                    physical = physical,
                    candidateKey = null,
                    candidateValue = null,
                    label = "CONTROL"
                )
                item.put("control", control.toJson())
                if (control.frameCaptured) frames++
                report.put("physicalFrameCount", frames)
                    .put("independentEvidenceCount", frames)
                    .put("stage", "CONTROL_CAPTURE_RETURNED")
                writeCheckpoint(report)

                if (!control.frameCaptured || control.raw == null || control.capture == null) {
                    failedPairs++
                    item.put("classification", "CONTROL_CAPTURE_FAILED__CANDIDATE_NOT_ATTEMPTED")
                    report.put("stage", "CONTROL_FAILED")
                    writeCheckpoint(report)
                    continue
                }

                report.put("stage", "BEFORE_CANDIDATE_CAPTURE")
                writeCheckpoint(report)

                val candidate = runSingleFrame(
                    device = device,
                    executor = opened.executor,
                    physical = physical,
                    candidateKey = key,
                    candidateValue = spec.value(),
                    label = spec.shortName
                )
                item.put("candidate", candidate.toJson())
                if (candidate.frameCaptured) frames++
                report.put("physicalFrameCount", frames)
                    .put("independentEvidenceCount", frames)
                    .put("stage", "CANDIDATE_CAPTURE_RETURNED")
                writeCheckpoint(report)

                if (!candidate.frameCaptured || candidate.raw == null || candidate.capture == null) {
                    failedPairs++
                    item.put("classification", "CANDIDATE_CAPTURE_FAILED")
                    report.put("stage", "CANDIDATE_FAILED")
                    writeCheckpoint(report)
                    continue
                }

                val comparison = comparePair(control, candidate)
                item.put("comparison", comparison)
                item.put("pairCompleted", true)
                item.put("classification", "CONTROL_CANDIDATE_SINGLE_FRAME_PAIR_COMPLETE")

                pairComplete++
                if (comparison.optBoolean("structuralTopologyDifferentialObserved", false)) structuralDifferentials++
                if (comparison.optBoolean("selectedResultMetadataDifferentialObserved", false)) metadataDifferentials++

                report
                    .put("controlCandidatePairsComplete", pairComplete)
                    .put("structuralTopologyDifferentialCount", structuralDifferentials)
                    .put("selectedResultMetadataDifferentialCount", metadataDifferentials)
                    .put("failedOrSkippedPairCount", failedPairs)
                    .put("stage", "PAIR_RECORDED")
                writeCheckpoint(report)
            } finally {
                device.close()
                opened.closedLatch?.await(2, TimeUnit.SECONDS)
                opened.executor?.shutdown()
                opened.executor?.awaitTermination(2, TimeUnit.SECONDS)
                opened.executor?.shutdownNow()
            }

            Thread.sleep(350L)
        }

        return report
            .put("candidateIndexInFlight", JSONObject.NULL)
            .put("candidateNameInFlight", JSONObject.NULL)
            .put("stage", "MATRIX_COMPLETE")
            .put("controlCandidatePairsComplete", pairComplete)
            .put("physicalFrameCount", frames)
            .put("independentEvidenceCount", frames)
            .put("structuralTopologyDifferentialCount", structuralDifferentials)
            .put("selectedResultMetadataDifferentialCount", metadataDifferentials)
            .put("failedOrSkippedPairCount", failedPairs)
            .put("classification", "PHYSICAL5_SINGLE_FRAME_CAPTURE_EFFECT_MATRIX_COMPLETE_OR_PARTIAL")
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

    private data class FrameOutcome(
        val label: String,
        val sessionConfigured: Boolean,
        val frameCaptured: Boolean,
        val candidateSessionParameterAttached: Boolean,
        val candidateRequestKeyWritten: Boolean,
        val candidateReadback: Any?,
        val physicalPixelModeWritten: Boolean,
        val physicalPixelModeReadback: Any?,
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
            .put("physicalSensorPixelModeWritten", physicalPixelModeWritten)
            .put("physicalSensorPixelModeReadback", jsonValue(physicalPixelModeReadback))
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
    }

    private data class RawSummary(
        val accessibleBytes: Long,
        val rowStride: Int,
        val pixelStride: Int,
        val fullPlaneSha256: String,
        val firstNonZeroByte: Long?,
        val lastNonZeroByte: Long?,
        val tailBoundaryByte: Long,
        val tailNonZeroByteCount: Long,
        val effectivePrefixSha256: String?,
        val packedDataOnlySha256: String?,
        val effectivePadNonZeroByteCount: Long?,
        val effectiveRowsWithImageData: Int?,
        val maxDeclaredRowWithNonZero: Int?,
        val decodedMin: Int?,
        val decodedMax: Int?,
        val decodedMean: Double?,
        val decodedZeroCount: Long?,
        val decoded1023Count: Long?,
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
            .put("knownEffectivePrefixBoundaryByte", tailBoundaryByte)
            .put("tailNonZeroByteCount", tailNonZeroByteCount)
            .put("effectivePrefixSha256", effectivePrefixSha256 ?: JSONObject.NULL)
            .put("packedDataOnlySha256", packedDataOnlySha256 ?: JSONObject.NULL)
            .put("effectivePadNonZeroByteCount", effectivePadNonZeroByteCount ?: JSONObject.NULL)
            .put("effectiveRowsWithImageData", effectiveRowsWithImageData ?: JSONObject.NULL)
            .put("maxDeclaredRowWithNonZero", maxDeclaredRowWithNonZero ?: JSONObject.NULL)
            .put("decodedMin", decodedMin ?: JSONObject.NULL)
            .put("decodedMax", decodedMax ?: JSONObject.NULL)
            .put("decodedMean", decodedMean ?: JSONObject.NULL)
            .put("decodedZeroCount", decodedZeroCount ?: JSONObject.NULL)
            .put("decoded1023Count", decoded1023Count ?: JSONObject.NULL)
            .put("parityMeans", parityMeans?.let { JSONArray(it.toList()) } ?: JSONObject.NULL)
            .put("knownV059TopologyMatch", knownV059TopologyMatch)
            .put("boundaryInterpretation", "Known v0.59 comparison geometry only; 5120-byte effective rows are a host reinterpretation, not the reported RAW10 rowStride.")
    }

    private fun runSingleFrame(
        device: CameraDevice,
        executor: ExecutorService,
        physical: CameraCharacteristics,
        candidateKey: CaptureRequest.Key<Any>?,
        candidateValue: Any?,
        label: String,
    ): FrameOutcome {
        val reader = runCatching {
            ImageReader.newInstance(TARGET_W, TARGET_H, ImageFormat.RAW10, 1)
        }.getOrElse {
            return FrameOutcome(label, false, false, candidateKey != null, false, null, false, null, null, null, it.javaClass.name, it.message)
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
        }, android.os.Handler(android.os.Looper.getMainLooper()))

        val sessionLatch = CountDownLatch(1)
        val sessionClosedLatch = CountDownLatch(1)
        val sessionRef = AtomicReference<CameraCaptureSession?>(null)
        var sessionConfigured = false
        var sessionError: String? = null
        var candidateSessionParameterAttached = false

        val output = OutputConfiguration(reader.surface)
        try {
            output.setPhysicalCameraId(PHYSICAL_ID)
            output.addSensorPixelModeUsed(CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
        } catch (e: Throwable) {
            reader.close()
            return FrameOutcome(label, false, false, candidateKey != null, false, null, false, null, null, null, e.javaClass.name, e.message)
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

        var candidateReadback: Any? = null
        try {
            if (candidateKey != null) {
                require(candidateValue != null)
                val sessionBuilder = device.createCaptureRequest(
                    CameraDevice.TEMPLATE_STILL_CAPTURE,
                    setOf(PHYSICAL_ID)
                )
                sessionBuilder.setPhysicalCameraKey(candidateKey, candidateValue, PHYSICAL_ID)
                candidateReadback = sessionBuilder.getPhysicalCameraKey(candidateKey, PHYSICAL_ID)
                config.setSessionParameters(sessionBuilder.build())
                candidateSessionParameterAttached = true
            }

            device.createCaptureSession(config)
            if (!sessionLatch.await(8, TimeUnit.SECONDS) || !sessionConfigured) {
                runCatching { sessionRef.get()?.close() }
                sessionClosedLatch.await(2, TimeUnit.SECONDS)
                reader.close()
                return FrameOutcome(
                    label, false, false, candidateSessionParameterAttached, false, candidateReadback,
                    false, null, null, null,
                    "SESSION_CONFIGURATION_FAILED_OR_TIMED_OUT", sessionError
                )
            }

            val session = sessionRef.get() ?: error("configured session missing")
            val requestBuilder = device.createCaptureRequest(
                CameraDevice.TEMPLATE_STILL_CAPTURE,
                setOf(PHYSICAL_ID)
            )
            requestBuilder.addTarget(reader.surface)

            var physicalPixelModeWritten = false
            var physicalPixelModeReadback: Any? = null
            runCatching {
                requestBuilder.setPhysicalCameraKey(
                    CaptureRequest.SENSOR_PIXEL_MODE,
                    CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION,
                    PHYSICAL_ID
                )
                physicalPixelModeWritten = true
                physicalPixelModeReadback = requestBuilder.getPhysicalCameraKey(
                    CaptureRequest.SENSOR_PIXEL_MODE,
                    PHYSICAL_ID
                )
            }

            var candidateRequestWritten = false
            if (candidateKey != null) {
                require(candidateValue != null)
                requestBuilder.setPhysicalCameraKey(candidateKey, candidateValue, PHYSICAL_ID)
                candidateRequestWritten = true
                candidateReadback = requestBuilder.getPhysicalCameraKey(candidateKey, PHYSICAL_ID)
            }

            setIfAdvertised(requestBuilder, CaptureRequest.CONTROL_ENABLE_ZSL, false, physical)
            setIfAdvertised(requestBuilder, CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO, physical)
            setIfAdvertised(requestBuilder, CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_ON, physical)
            val nr = physical.get(CameraCharacteristics.NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES) ?: intArrayOf()
            if (nr.contains(CameraMetadata.NOISE_REDUCTION_MODE_OFF)) {
                setIfAdvertised(requestBuilder, CaptureRequest.NOISE_REDUCTION_MODE, CameraMetadata.NOISE_REDUCTION_MODE_OFF, physical)
            }
            val edge = physical.get(CameraCharacteristics.EDGE_AVAILABLE_EDGE_MODES) ?: intArrayOf()
            if (edge.contains(CameraMetadata.EDGE_MODE_OFF)) {
                setIfAdvertised(requestBuilder, CaptureRequest.EDGE_MODE, CameraMetadata.EDGE_MODE_OFF, physical)
            }

            val resultRef = AtomicReference<TotalCaptureResult?>(null)
            val resultLatch = CountDownLatch(1)
            var captureFailureText: String? = null

            session.captureSingleRequest(
                requestBuilder.build(),
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
                    label, true, false, candidateSessionParameterAttached, candidateRequestWritten,
                    candidateReadback, physicalPixelModeWritten, physicalPixelModeReadback,
                    null, null,
                    "CAPTURE_PAIRING_FAILED",
                    captureFailureText ?: "resultReady=" + resultReady + " imageReady=" + imageReady
                )
            }

            try {
                val physicalResult = logicalResult.physicalCameraResults[PHYSICAL_ID]
                    ?: error("physical Camera-5 result missing")
                val sensorTs = physicalResult.get(CaptureResult.SENSOR_TIMESTAMP)
                require(sensorTs == image.timestamp) {
                    "physical timestamp " + sensorTs + " != image timestamp " + image.timestamp
                }

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
                )

                return FrameOutcome(
                    label = label,
                    sessionConfigured = true,
                    frameCaptured = true,
                    candidateSessionParameterAttached = candidateSessionParameterAttached,
                    candidateRequestKeyWritten = candidateRequestWritten,
                    candidateReadback = candidateReadback,
                    physicalPixelModeWritten = physicalPixelModeWritten,
                    physicalPixelModeReadback = physicalPixelModeReadback,
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
                label, sessionConfigured, false, candidateSessionParameterAttached, false,
                candidateReadback, false, null, null, null, e.javaClass.name, e.message
            )
        }
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
        var decodedZeroCount: Long? = null
        var decoded1023Count: Long? = null
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
            var zeros = 0L
            var clips = 0L
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
                    val v0 = (b0 shl 2) or (b4 and 0x03)
                    val v1 = (b1 shl 2) or ((b4 shr 2) and 0x03)
                    val v2 = (b2 shl 2) or ((b4 shr 4) and 0x03)
                    val v3 = (b3 shl 2) or ((b4 shr 6) and 0x03)
                    val vs = intArrayOf(v0, v1, v2, v3)
                    for (k in 0..3) {
                        val v = vs[k]
                        if (v < minV) minV = v
                        if (v > maxV) maxV = v
                        if (v == 0) zeros++
                        if (v == 1023) clips++
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
            decodedZeroCount = zeros
            decoded1023Count = clips
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
            tailBoundaryByte = EFFECTIVE_PREFIX_BYTES,
            tailNonZeroByteCount = tailNonZero,
            effectivePrefixSha256 = effectivePrefixSha,
            packedDataOnlySha256 = packedOnlySha,
            effectivePadNonZeroByteCount = padNonZero,
            effectiveRowsWithImageData = rowsWithData,
            maxDeclaredRowWithNonZero = maxDeclaredRow,
            decodedMin = decodedMin,
            decodedMax = decodedMax,
            decodedMean = decodedMean,
            decodedZeroCount = decodedZeroCount,
            decoded1023Count = decoded1023Count,
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

        val metadata =
            cc.sensorPixelMode != vc.sensorPixelMode ||
                cc.rawBinningFactorUsed != vc.rawBinningFactorUsed ||
                cc.noiseReductionMode != vc.noiseReductionMode ||
                cc.edgeMode != vc.edgeMode

        val exposureExact = cc.iso == vc.iso && cc.exposureNs == vc.exposureNs
        val meansComparable = exposureExact && cr.decodedMean != null && vr.decodedMean != null
        val meanDelta = if (cr.decodedMean != null && vr.decodedMean != null) {
            vr.decodedMean - cr.decodedMean
        } else null

        return JSONObject()
            .put("structuralTopologyDifferentialObserved", structural)
            .put("selectedResultMetadataDifferentialObserved", metadata)
            .put("fullPlaneSha256Equal", cr.fullPlaneSha256 == vr.fullPlaneSha256)
            .put("effectivePrefixSha256Equal", cr.effectivePrefixSha256 == vr.effectivePrefixSha256)
            .put("packedDataOnlySha256Equal", cr.packedDataOnlySha256 == vr.packedDataOnlySha256)
            .put("controlKnownV059TopologyMatch", cr.knownV059TopologyMatch)
            .put("candidateKnownV059TopologyMatch", vr.knownV059TopologyMatch)
            .put("exposureExactlyMatched", exposureExact)
            .put("sampleMeanDeltaCandidateMinusControl", meanDelta ?: JSONObject.NULL)
            .put("sampleStatisticsAttributionAllowed", meansComparable)
            .put(
                "sampleStatisticsAttributionBoundary",
                if (meansComparable)
                    "Exposure and ISO match exactly, but scene variation and unmeasured processing can still confound sample-statistic differences."
                else
                    "Exposure/ISO differ or statistics unavailable; sample-value differences cannot be attributed to the candidate key."
            )
            .put("interpretation", "Different RAW hashes are expected for independent frames and are not, by themselves, evidence of a vendor-key effect.")
    }

    private fun <T> setIfAdvertised(
        builder: CaptureRequest.Builder,
        key: CaptureRequest.Key<T>,
        value: T,
        physical: CameraCharacteristics,
    ) {
        if (physical.availableCaptureRequestKeys.orEmpty().any { it.name == key.name }) {
            runCatching { builder.set(key, value) }
        }
    }

    private fun safeSizes(block: () -> Array<Size>?): List<Size> =
        try { block()?.toList().orEmpty() } catch (_: Throwable) { emptyList() }

    private fun sizeArray(sizes: List<Size>): JSONArray =
        JSONArray(sizes.map { JSONObject().put("width", it.width).put("height", it.height) })

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
            status.text = "Nog geen v0.70 report."
            return
        }
        val report = runCatching { JSONObject(file.readText()) }.getOrNull()
        if (report == null) {
            status.text = "v0.70 report bestaat maar kon niet worden gelezen."
            return
        }
        status.text = buildString {
            append("stage=").append(report.optString("stage", "?")).append('\n')
            append("pairs complete=").append(report.optInt("controlCandidatePairsComplete", 0))
                .append("/").append(report.optInt("candidateCount", 0)).append('\n')
            append("physical frames=").append(report.optInt("physicalFrameCount", 0)).append('\n')
            append("structural differentials=").append(report.optInt("structuralTopologyDifferentialCount", 0)).append('\n')
            append("metadata differentials=").append(report.optInt("selectedResultMetadataDifferentialCount", 0)).append('\n')
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
            status.text = "v0.70 JSON opgeslagen."
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
    ) {
        fun value(): Any = when (type) {
            ValueType.INT_ARRAY -> intArrayOf(1)
            ValueType.BYTE_ARRAY -> byteArrayOf(1)
        }
    }

    companion object {
        private const val SCHEMA = "truthraw.physical5-capture-effect-matrix.v0.70"
        private const val AUTHORITY = "CAMERA2_SINGLE_FRAME_CONTROL_CANDIDATE_CAPTURE_EFFECT"
        private const val REPORT_FILENAME = "TRUTHRAW_PHYSICAL5_CAPTURE_EFFECT_MATRIX_v070.json"
        private const val REQUEST_CAMERA_PERMISSION = 67062
        private const val REQUEST_SAVE_JSON = 67063
        private const val LOGICAL_ID = "0"
        private const val PHYSICAL_ID = "5"

        private const val TARGET_W = 16320
        private const val TARGET_H = 12288
        private const val DECLARED_RAW10_ROW_STRIDE = 20400
        private const val EXPECTED_DECLARED_RAW10_BYTES = 250675200L

        // Established v0.59 comparison geometry only. This is not the reported RAW10 stride.
        private const val EFFECTIVE_ROWS = 3072
        private const val EFFECTIVE_ROW_BYTES = 5120
        private const val EFFECTIVE_PACKED_BYTES_PER_ROW = 5100
        private const val EFFECTIVE_PREFIX_BYTES = 15728640L

        private const val BOUNDARY =
            "A_CONTROL_CANDIDATE_SINGLE_FRAME_DIFFERENTIAL_CAN_ESTABLISH_AN_APP_VISIBLE_CAPTURE_EFFECT_IN_THIS_EXACT_CONTEXT_ONLY; IT_DOES_NOT_BY_ITSELF_PROVE_VENDOR_SEMANTICS, NATIVE_SENSOR_GEOMETRY, DIRECT_CFA_200MP, ADC_BIT_DEPTH, OR_CALIBRATION_TRUTH"

        private val CANDIDATES = listOf(
            CandidateSpec("MasterFilmSensorType", "com.hihonor.capture.metadata.MasterFilmSensorType", ValueType.INT_ARRAY),
            CandidateSpec("aoRunningMode", "com.hihonor.capture.metadata.aoRunningMode", ValueType.INT_ARRAY),
            CandidateSpec("EnableVSR", "org.codeaurora.qcamera3.sessionParameters.EnableVSR", ValueType.INT_ARRAY),
            CandidateSpec("ExtendedMaxZoom", "org.codeaurora.qcamera3.sessionParameters.ExtendedMaxZoom", ValueType.INT_ARRAY),
            CandidateSpec("inSensorZoomEnable", "org.codeaurora.qcamera3.sessionParameters.inSensorZoomEnable", ValueType.BYTE_ARRAY),
            CandidateSpec("cameraSceneMode", "com.hihonor.capture.metadata.cameraSceneMode", ValueType.INT_ARRAY),
            CandidateSpec("extStreamSize", "com.hihonor.capture.metadata.extStreamSize", ValueType.INT_ARRAY),
            CandidateSpec("teleconverterEnable", "com.hihonor.capture.metadata.teleconverterEnable", ValueType.BYTE_ARRAY),
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
