package com.truthraw.adaptiveui

import android.app.Activity
import android.content.ClipData
import android.content.Intent
import android.content.res.Configuration
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.Button
import android.widget.Chronometer
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.ScrollView
import android.widget.Space
import android.widget.TextView
import java.io.File
import java.io.IOException

class MainActivity : Activity() {
    private var session = BatchSession()
    private var activeJobId: String? = null
    private var previewState: TilePreviewUiState = TilePreviewUiState.Idle
    private var unifiedOutputPreviewState: UnifiedOutputPreviewResult.Ready? = null
    private var restorationUnifiedPreviewKey: String? = null
    private var previewGeneration: Long = 0
    private var loadingStartedAtElapsedMs: Long? = null
    private var pendingJpegJobId: String? = null
    private var jpegStatus: String? = null
    private var pendingFullColourMasterJobId: String? = null
    private var fullColourMasterStatus: String? = null
    private var pendingTruthNegative200MpJobId: String? = null
    private var truthNegative200MpStatus: String? = null
    private var pendingRenderEditJobId: String? = null
    private var renderEditStatus: String? = null
    private var pendingRenderEditFlags: Int = 0
    private var pendingRenderEditQuarterTurns: Int = 0
    private var pendingPhotoRoute: String? = null
    private var pendingPhotoFlags: Int = 0
    private var pendingPhotoQuarterTurns: Int = 0
    private var pendingPureFloatDngJobId: String? = null
    private var pendingPureQuarterTurns: Int = 0
    private var pureFloatDngStatus: String? = null
    private var pendingTruthNegativeJobId: String? = null
    private var truthNegativeStatus: String? = null
    private var truthNegativeContinuousStatus: String? = null
    private var camera5ColorHighlightStatus: String? = null
    private var pendingTruthNegativeNativeContainerJobId: String? = null
    private var truthNegativeNativeContainerStatus: String? = null
    private var pendingFullResRestorationJobId: String? = null
    private var fullResRestorationStatus: String? = null
    private var pendingProjectionFormat: RestorationProjectionFormat? = null
    private var projectionStatus: String? = null
    private val restorationStatusHandler = Handler(Looper.getMainLooper())
    private val restorationStatusPoll = object : Runnable {
        override fun run() {
            syncFullResRestorationStatus()
            syncRestorationProjectionStatus()
            val restoration = FullResRestorationJobStore.read(this@MainActivity)
            val projection = RestorationProjectionJobStore.read(this@MainActivity)
            if ((restoration != null && !restoration.phase.terminal) ||
                (projection != null && !projection.phase.terminal)
            ) {
                restorationStatusHandler.postDelayed(this, 1000L)
            }
        }
    }
    private var pendingLinearDngJobId: String? = null
    private var linearDngStatus: String? = null
    private var empiricalAudit: EmpiricalRunAudit? = null
    private var pendingEmpiricalJobId: String? = null
    private var pendingEmpiricalJson: String? = null
    private var empiricalStatus: String? = null
    private var nefMeasurementResult: NefMeasurementResult? = null
    private var nefMeasurementLoading: Boolean = false
    private var pendingNefMeasurementJobId: String? = null
    private var pendingNefMeasurementJson: String? = null
    private var nefMeasurementExportStatus: String? = null

    private enum class LayoutTier { COMPACT, MEDIUM, EXPANDED }

    private data class Palette(
        val background: Int,
        val surface: Int,
        val surfaceAlt: Int,
        val text: Int,
        val textMuted: Int,
        val accent: Int,
    )

    private val palette: Palette
        get() = Palette(
            background = DrawVisualTheme.PAPER_YELLOW_SOFT,
            surface = DrawVisualTheme.PAPER_WHITE,
            surfaceAlt = DrawVisualTheme.PAPER_BLUE,
            text = DrawVisualTheme.INK,
            textMuted = DrawVisualTheme.MUTED,
            accent = DrawVisualTheme.BLUE,
        )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        DrawVisualTheme.applyWindow(this)

        var cameraJobToAutoStart: RawJob? = null
        if (savedInstanceState == null) {
            val cameraJob = readInternalCameraJob(intent)
            if (cameraJob != null) {
                installInternalCameraJob(cameraJob)
                if (intent.getBooleanExtra(EXTRA_AUTO_START_TRUTHRAW, false)) {
                    cameraJobToAutoStart = cameraJob
                }
            }
        }

        if (session.jobs.isEmpty() && hasPendingProjectionPicker()) {
            restoreProjectionSourceSession()
        }

        render()

        cameraJobToAutoStart?.let { job ->
            window.decorView.post {
                if (activeJobId == job.id && previewState is TilePreviewUiState.Idle) {
                    requestPreview(job)
                }
            }
        }

        if (savedInstanceState == null &&
            intent.getBooleanExtra(EXTRA_AUTO_OPEN_RAW_PICKER, false) &&
            session.jobs.isEmpty()
        ) {
            window.decorView.post { launchRawPicker() }
        }
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        setIntent(intent)
        val cameraJob = readInternalCameraJob(intent) ?: return
        installInternalCameraJob(cameraJob)
        render()
        if (intent.getBooleanExtra(EXTRA_AUTO_START_TRUTHRAW, false)) {
            window.decorView.post {
                if (activeJobId == cameraJob.id && previewState is TilePreviewUiState.Idle) {
                    requestPreview(cameraJob)
                }
            }
        }
    }

    private fun readInternalCameraJob(sourceIntent: Intent): RawJob? {
        val sourcePath = sourceIntent.getStringExtra(EXTRA_INTERNAL_CAMERA_SOURCE_PATH)
            ?.takeIf { it.isNotBlank() }
            ?: return null
        val evidencePath = sourceIntent.getStringExtra(EXTRA_INTERNAL_CAMERA_EVIDENCE_PATH)
            ?.takeIf { it.isNotBlank() }
        val upstreamSha = sourceIntent.getStringExtra(EXTRA_INTERNAL_CAMERA_UPSTREAM_SHA256)
            ?.takeIf { it.isNotBlank() }
        return runCatching {
            RawIngress.readInternalCameraFile(
                file = File(sourcePath),
                acquisitionEvidenceFile = evidencePath?.let(::File),
                upstreamSealedSourceSha256 = upstreamSha,
            )
        }.getOrNull()
    }

    private fun installInternalCameraJob(cameraJob: RawJob) {
        (previewState as? TilePreviewUiState.Ready)?.bitmap?.recycle()
        ++previewGeneration
        session = session.withJobs(listOf(cameraJob))
        activeJobId = cameraJob.id
        previewState = TilePreviewUiState.Idle
        loadingStartedAtElapsedMs = null
        empiricalAudit = null
        jpegStatus = null
        fullColourMasterStatus = null
        truthNegative200MpStatus = null
        pendingTruthNegative200MpJobId = null
        renderEditStatus = null
        pendingRenderEditJobId = null
        pendingRenderEditFlags = 0
        pendingRenderEditQuarterTurns = 0
        pureFloatDngStatus = null
        truthNegativeStatus = null
        truthNegativeContinuousStatus = null
        camera5ColorHighlightStatus = null
        pendingTruthNegativeNativeContainerJobId = null
        truthNegativeNativeContainerStatus = null
        fullResRestorationStatus = null
        projectionStatus = null
    }

    override fun onResume() {
        super.onResume()
        FullResRestorationJobStore.recoverInterruptedIfNeeded(this)
        RestorationProjectionJobStore.recoverInterruptedIfNeeded(this)
        recoverBackgroundOperationStatuses()
        syncFullResRestorationStatus()
        syncRestorationProjectionStatus()
        restorationStatusHandler.removeCallbacks(restorationStatusPoll)
        val restoration = FullResRestorationJobStore.read(this)
        val projection = RestorationProjectionJobStore.read(this)
        if ((restoration != null && !restoration.phase.terminal) ||
            (projection != null && !projection.phase.terminal)
        ) {
            restorationStatusHandler.post(restorationStatusPoll)
        }
        render()
    }

    override fun onPause() {
        restorationStatusHandler.removeCallbacks(restorationStatusPoll)
        super.onPause()
    }

    override fun onDestroy() {
        restorationStatusHandler.removeCallbacks(restorationStatusPoll)
        (previewState as? TilePreviewUiState.Ready)?.bitmap?.recycle()
        unifiedOutputPreviewState?.bitmap?.recycle()
        unifiedOutputPreviewState = null
        (nefMeasurementResult as? NefMeasurementResult.Ready)?.bitmap?.recycle()
        super.onDestroy()
    }

    private fun recoverBackgroundOperationStatuses() {
        val jobId = activeJobId ?: return
        val running = TruthRawMediaProcessingForegroundService.isRunning

        fun recover(kind: String): TruthRawPersistedOperation? =
            TruthRawOperationStore.recoverInterruptedIfNeeded(
                this,
                backgroundOperationKey(kind, jobId),
                running,
            )

        recover("jpeg")?.let { jpegStatus = it.message }
        recover("full-colour-scientific-master")?.let { fullColourMasterStatus = it.message }
        recover("truthnegative-200mp-full-colour")?.let { truthNegative200MpStatus = it.message }
        recover("advanced-render-edit")?.let { renderEditStatus = it.message }
        recover("pure-float32")?.let { pureFloatDngStatus = it.message }
        recover("truthnegative")?.let { truthNegativeStatus = it.message }
        recover("linear-dng")?.let { linearDngStatus = it.message }

        recover("nef-measurement")?.let { op ->
            if (op.phase == TruthRawOperationPhase.ERROR && nefMeasurementLoading) {
                nefMeasurementLoading = false
                nefMeasurementResult = NefMeasurementResult.Failed(op.message)
            }
        }

        recover("preview")?.let { op ->
            if (op.phase == TruthRawOperationPhase.ERROR &&
                previewState is TilePreviewUiState.Loading
            ) {
                loadingStartedAtElapsedMs = null
                previewState = TilePreviewUiState.Failed(jobId, op.message)
            }
        }
    }

    private fun syncFullResRestorationStatus() {
        val snapshot = FullResRestorationJobStore.read(this) ?: return
        if (snapshot.message != fullResRestorationStatus) {
            fullResRestorationStatus = snapshot.message
            render()
        }
        if (
            snapshot.phase == FullResRestorationJobPhase.SUCCESS &&
            activeJobId == snapshot.jobId
        ) {
            requestRestorationUnifiedOutputPreview(
                sourceUri = android.net.Uri.parse(snapshot.sourceUri),
                trrUri = android.net.Uri.parse(snapshot.destinationUri),
                outputLabel = "Full-res Restoration .trr",
                applyStoredOrientation = true,
                requestKey =
                    "trr:" + snapshot.jobId + ":" +
                        (snapshot.containerSha256 ?: snapshot.updatedAtMs.toString()),
            )
        }
    }

    private fun syncRestorationProjectionStatus() {
        val snapshot = RestorationProjectionJobStore.read(this) ?: return
        if (snapshot.message != projectionStatus) {
            projectionStatus = snapshot.message
            render()
        }
        val active = session.jobs.firstOrNull { it.id == activeJobId }
        if (
            snapshot.phase == RestorationProjectionJobPhase.SUCCESS &&
            active != null &&
            active.source.uri.toString() == snapshot.sourceUri
        ) {
            requestRestorationUnifiedOutputPreview(
                sourceUri = android.net.Uri.parse(snapshot.sourceUri),
                trrUri = android.net.Uri.parse(snapshot.trrUri),
                outputLabel = "Restoration " + snapshot.format.label + " projectie",
                applyStoredOrientation =
                    snapshot.format != RestorationProjectionFormat.EXR,
                requestKey =
                    "projection:" + snapshot.format.name + ":" +
                        snapshot.destinationUri + ":" + snapshot.updatedAtMs,
            )
        }
    }

    private fun requestRestorationUnifiedOutputPreview(
        sourceUri: android.net.Uri,
        trrUri: android.net.Uri,
        outputLabel: String,
        applyStoredOrientation: Boolean,
        requestKey: String,
    ) {
        if (restorationUnifiedPreviewKey == requestKey) return
        restorationUnifiedPreviewKey = requestKey
        val operationKey =
            "main:unified-restoration-preview:" + requestKey.hashCode().toUInt().toString(16)
        if (!startBackgroundOperation(operationKey, outputLabel + " preview opbouwen")) {
            restorationUnifiedPreviewKey = null
            return
        }

        startGuardedBackgroundThread(
            name = "truthraw-uop-restoration",
            operationKey = operationKey,
            onUnexpected = {
                if (restorationUnifiedPreviewKey == requestKey) {
                    restorationUnifiedPreviewKey = null
                }
            },
        ) {
            val staging = File(
                filesDir,
                "unified_output_preview/restoration_" +
                    requestKey.hashCode().toUInt().toString(16) + ".uop1",
            )
            val result = RestorationUnifiedOutputPreviewBuilder.build(
                contentResolver,
                sourceUri,
                trrUri,
                staging,
                outputLabel,
                applyStoredOrientation = applyStoredOrientation,
            )
            finishBackgroundOperation(
                operationKey,
                result is UnifiedOutputPreviewResult.Ready,
                when (result) {
                    is UnifiedOutputPreviewResult.Ready ->
                        outputLabel + " preview gereed."
                    is UnifiedOutputPreviewResult.Failed -> result.reason
                },
            )
            runOnUiThread {
                if (restorationUnifiedPreviewKey != requestKey) {
                    (result as? UnifiedOutputPreviewResult.Ready)?.bitmap?.recycle()
                    return@runOnUiThread
                }
                if (result is UnifiedOutputPreviewResult.Ready) {
                    unifiedOutputPreviewState?.bitmap?.recycle()
                    unifiedOutputPreviewState = result
                    render()
                }
            }
        }
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        render()
    }

    @Suppress("DEPRECATION")
    private fun launchRawPicker() {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "*/*"
            putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true)
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(intent, REQUEST_OPEN_RAW)
    }

    @Suppress("DEPRECATION")
    private fun launchJpegExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        pendingJpegJobId = job.id
        jpegStatus = null
        val route = preferredRoute()
        pendingPhotoRoute = route
        pendingPhotoFlags = photoFlagsForRoute(route)
        pendingPhotoQuarterTurns = TruthRawOrientationOverride.quarterTurns(this, job.source)
        val stem = job.source.displayName.substringBeforeLast('.', job.source.displayName)
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "image/jpeg"
            putExtra(Intent.EXTRA_TITLE, "${stem}_draw_${route.lowercase()}_fullres.jpg")
        }
        startActivityForResult(intent, REQUEST_SAVE_JPEG)
    }

    @Suppress("DEPRECATION")
    private fun launchFullColourScientificMasterExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        pendingFullColourMasterJobId = job.id
        fullColourMasterStatus = null
        val route = preferredRoute()
        pendingPhotoRoute = route
        pendingPhotoFlags = 0
        pendingPhotoQuarterTurns = TruthRawOrientationOverride.quarterTurns(this, job.source)
        val stem = job.source.displayName.substringBeforeLast('.', job.source.displayName)
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            // Full-colour Scientific Master: camera-native IEEE Float32 LinearRaw
            // is the primary image. JPEG is secondary preview only.
            type = "image/x-adobe-dng"
            putExtra(
                Intent.EXTRA_TITLE,
                "${stem}_draw_full_colour_scientific_master_float32_v0_1.dng",
            )
        }
        startActivityForResult(intent, REQUEST_SAVE_FULL_COLOUR_MASTER)
    }

    @Suppress("DEPRECATION")
    private fun launchTruthNegative200MpFullColourExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        if (!job.source.verifiedCamera5TruthNegative200MpEnvelope) {
            truthNegative200MpStatus =
                "TruthNegative 200MP geblokkeerd: physical Camera-5 envelope/admission-lineage is niet exact geverifieerd."
            render()
            return
        }
        if (!job.source.format.nativeProcessingReady || job.source.format.id != "DNG") {
            truthNegative200MpStatus =
                "TruthNegative 200MP vereist de volledig admitted DNG/Camera-5-route."
            render()
            return
        }
        pendingTruthNegative200MpJobId = job.id
        truthNegative200MpStatus = null
        pendingPhotoRoute = preferredRoute()
        pendingPhotoFlags = 0
        pendingPhotoQuarterTurns =
            TruthRawOrientationOverride.quarterTurns(this, job.source)
        val stem = job.source.displayName.substringBeforeLast('.', job.source.displayName)
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "image/x-adobe-dng"
            putExtra(
                Intent.EXTRA_TITLE,
                "${stem}_draw_truthnegative_200mp_full_colour_float32_v0_1.dng",
            )
        }
        startActivityForResult(intent, REQUEST_SAVE_TRUTHNEGATIVE_200MP_FULL_COLOUR)
    }

    @Suppress("DEPRECATION")
    private fun launchAdvancedRenderEditExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        if (!job.source.format.nativeProcessingReady || job.source.format.id != "DNG") {
            renderEditStatus =
                "ADVANCED Render/Edit is momenteel alleen beschikbaar voor de volledig admitted DNG-route."
            render()
            return
        }
        pendingRenderEditJobId = job.id
        pendingRenderEditFlags = photoFlagsForRoute(preferredRoute())
        pendingRenderEditQuarterTurns =
            TruthRawOrientationOverride.quarterTurns(this, job.source)
        renderEditStatus = null
        val stem = job.source.displayName.substringBeforeLast('.', job.source.displayName)
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "image/x-adobe-dng"
            putExtra(
                Intent.EXTRA_TITLE,
                "${stem}_draw_advanced_render_edit_float32_v0_1.dng",
            )
        }
        startActivityForResult(intent, REQUEST_SAVE_ADVANCED_RENDER_EDIT)
    }

    @Suppress("DEPRECATION")
    private fun launchPureFloatDngExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        if (!job.source.format.nativeProcessingReady || job.source.format.id != "DNG") {
            pureFloatDngStatus =
                "D.RAW PURE Float32 is momenteel alleen beschikbaar voor de volledig admitted DNG-route."
            render()
            return
        }
        pendingPureFloatDngJobId = job.id
        pendingPureQuarterTurns = TruthRawOrientationOverride.quarterTurns(this, job.source)
        pureFloatDngStatus = null
        val stem = job.source.displayName.substringBeforeLast('.', job.source.displayName)
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "image/x-adobe-dng"
            putExtra(Intent.EXTRA_TITLE, "${stem}_draw_pure_float32_v0_63.dng")
        }
        startActivityForResult(intent, REQUEST_SAVE_PURE_FLOAT_DNG)
    }

    private fun truthNegativeHeavyOperationActive(
        jobId: String,
        exceptKey: String,
    ): Boolean {
        val kinds = listOf(
            "truthnegative-continuous-preview",
            "camera5-color-highlight-oracle",
            "truthnegative-native-container",
        )
        return kinds
            .map { backgroundOperationKey(it, jobId) }
            .any { it != exceptKey &&
                TruthRawMediaProcessingForegroundService.isActive(it) }
    }

    private fun launchTruthNegativeContinuousPreview(job: RawJob) {
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            truthNegativeContinuousStatus =
                "TruthNegative Continuous v0.5 vereist de volledig admitted DNG-route."
            render()
            return
        }

        val operationKey =
            backgroundOperationKey("truthnegative-continuous-preview", job.id)
        if (truthNegativeHeavyOperationActive(job.id, operationKey)) {
            truthNegativeContinuousStatus =
                "Wacht op de andere TruthNegative/Camera-5 analysetaak. " +
                    "Deze zware Scientific Master-routes draaien bewust niet meer tegelijk."
            render()
            return
        }
        if (!startBackgroundOperation(
                operationKey,
                "TruthNegative Continuous v0.5 · Scientific Negative → Free-World preview",
            )
        ) {
            truthNegativeContinuousStatus =
                "TruthNegative Continuous preview kon niet veilig starten."
            render()
            return
        }

        truthNegativeContinuousStatus =
            "TruthNegative Continuous v0.5 bouwt de raster-onafhankelijke scientific-negative state, " +
                "lokale authority en area-integrated PRO preview…"
        render()

        startGuardedBackgroundThread(
            name = "draw-tn-continuous-${job.id.take(8)}",
            operationKey = operationKey,
            onUnexpected = { message ->
                truthNegativeContinuousStatus = message
            },
        ) {
            val result =
                TruthNegativeContinuousPreviewLoader.load(contentResolver, job)
            finishBackgroundOperation(
                operationKey,
                result is TruthNegativeContinuousPreviewResult.Ready,
                when (result) {
                    is TruthNegativeContinuousPreviewResult.Ready ->
                        "TruthNegative Continuous v0.5 preview gereed."
                    is TruthNegativeContinuousPreviewResult.Failed ->
                        result.reason
                },
            )

            runOnUiThread {
                if (activeJobId != job.id) {
                    (result as? TruthNegativeContinuousPreviewResult.Ready)
                        ?.bitmap
                        ?.recycle()
                    return@runOnUiThread
                }

                when (result) {
                    is TruthNegativeContinuousPreviewResult.Failed -> {
                        truthNegativeContinuousStatus = result.reason
                    }
                    is TruthNegativeContinuousPreviewResult.Ready -> {
                        unifiedOutputPreviewState?.bitmap?.recycle()
                        unifiedOutputPreviewState =
                            TruthNegativeContinuousPreviewLoader.toUnifiedPreview(
                                result,
                                TruthRawOrientationOverride.quarterTurns(
                                    this@MainActivity,
                                    job.source,
                                ),
                            )
                        val m = result.metrics
                        truthNegativeContinuousStatus =
                            "TN Continuous v0.5 · ${m.width}×${m.height} uit " +
                                "${m.sourceWidth}×${m.sourceHeight} · " +
                                "authority-field CAL/REC/CENS/UNK=" +
                                "${m.calibratedEstimateRecords}/" +
                                "${m.reconstructedRecords}/" +
                                "${m.censoredRecords}/${m.unknownRecords} · " +
                                "target REC/CENS/UNK=" +
                                "${m.resolvedReconstructedChannels}/" +
                                "${m.resolvedCensoredChannels}/" +
                                "${m.resolvedUnknownChannels} · " +
                                "footprint-links=${m.sourceFootprintLinks} · " +
                                "state=${m.stateSha256.take(16)}… · " +
                                "display-clamp=${m.displayClampPixels} px · " +
                                "frame/evidence=${m.physicalFrameCount}/" +
                                "${m.independentEvidenceCount} · writeback=false."
                    }
                }
                render()
            }
        }
    }


    private fun launchCamera5ColorHighlightOracle(job: RawJob) {
        if (!job.source.verifiedCamera5TruthNegative200MpEnvelope) {
            camera5ColorHighlightStatus =
                "Camera-5 Oracle geblokkeerd: exact physical-5 acquisition/envelope bewijs ontbreekt."
            render()
            return
        }
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            camera5ColorHighlightStatus =
                "Camera-5 Oracle vereist de admitted Camera-5 DNG-route."
            render()
            return
        }

        val operationKey =
            backgroundOperationKey("camera5-color-highlight-oracle", job.id)
        if (truthNegativeHeavyOperationActive(job.id, operationKey)) {
            camera5ColorHighlightStatus =
                "Wacht op de andere TruthNegative/Camera-5 analysetaak. " +
                    "Parallelle volledige Scientific Master-scans zijn uitgeschakeld."
            render()
            return
        }
        if (!startBackgroundOperation(
                operationKey,
                "Camera-5 Color/Highlight Oracle v0.1",
            )
        ) {
            camera5ColorHighlightStatus =
                "Camera-5 Oracle kon niet veilig starten."
            render()
            return
        }

        camera5ColorHighlightStatus =
            "Camera-5 Oracle analyseert Scientific Master, lokale censoring, " +
                "AsShotNeutral en EV 0/-0.5/-1/-2/-3 display-resolves…"
        render()

        startGuardedBackgroundThread(
            name = "draw-camera5-color-oracle-" + job.id.take(8),
            operationKey = operationKey,
            onUnexpected = { camera5ColorHighlightStatus = it },
        ) {
            val result =
                Camera5ColorHighlightOracleLoader.run(contentResolver, job)
            finishBackgroundOperation(
                operationKey,
                result is Camera5ColorHighlightResult.Ready,
                when (result) {
                    is Camera5ColorHighlightResult.Ready ->
                        "Camera-5 Color/Highlight Oracle gereed."
                    is Camera5ColorHighlightResult.Failed ->
                        result.reason
                },
            )
            runOnUiThread {
                if (activeJobId != job.id) return@runOnUiThread
                camera5ColorHighlightStatus = when (result) {
                    is Camera5ColorHighlightResult.Failed ->
                        result.reason
                    is Camera5ColorHighlightResult.Ready -> {
                        val m = result.report
                        val asShot =
                            if (m.asShotNeutralKnown) {
                                "%.3f/%.3f/%.3f".format(
                                    m.asShotNeutral.first,
                                    m.asShotNeutral.second,
                                    m.asShotNeutral.third,
                                )
                            } else "UNKNOWN"
                        val empirical =
                            if (m.empiricalNeutralKnown) {
                                "%.3f/%.3f/%.3f".format(
                                    m.empiricalNeutral.first,
                                    m.empiricalNeutral.second,
                                    m.empiricalNeutral.third,
                                )
                            } else "UNRESOLVED"
                        "Camera-5 Oracle v0.1 · eerste afwijkingslaag=" +
                            m.firstFailureStage +
                            " · highlight candidates=" + m.candidates +
                            "/" + m.sampleCount +
                            " · censored=" +
                            "%.2f%%".format(m.censoredFraction * 100.0) +
                            " · AsShotNeutral=" + asShot +
                            " · empirical R/G-G-B/G=" + empirical +
                            " · neutral log-error=" +
                            "%.4f".format(m.metadataNeutralLogError) +
                            " · low-EV green bias=" +
                            "%.4f".format(m.lowExposureGreenBias) +
                            " · green drift=" +
                            "%.4f".format(m.exposureGreenDrift) +
                            " · remosaic=" + m.remosaicState +
                            " (DNG-only unresolved) · oracle=" +
                            m.oracleSha256.take(16) +
                            "… · writeback=false."
                    }
                }
                render()
            }
        }
    }

    @Suppress("DEPRECATION")
    private fun launchTruthNegativeNativeContainerExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            truthNegativeNativeContainerStatus =
                "Native TruthNegative container vereist de admitted DNG-route."
            render()
            return
        }
        pendingTruthNegativeNativeContainerJobId = job.id
        truthNegativeNativeContainerStatus = null
        val stem =
            job.source.displayName.substringBeforeLast(
                '.',
                job.source.displayName,
            )
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/octet-stream"
            putExtra(
                Intent.EXTRA_TITLE,
                stem + "_draw_truthnegative_native_v0_1.tnc",
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_SAVE_TRUTHNEGATIVE_NATIVE_CONTAINER,
        )
    }

    @Suppress("DEPRECATION")
    private fun launchTruthNegativeExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        if (!job.source.format.nativeProcessingReady || job.source.format.id != "DNG") {
            truthNegativeStatus =
                "TRUTHNEGATIVE is momenteel alleen beschikbaar voor de volledig admitted DNG-route."
            render()
            return
        }
        pendingTruthNegativeJobId = job.id
        truthNegativeStatus = null
        val stem = job.source.displayName.substringBeforeLast('.', job.source.displayName)
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/octet-stream"
            putExtra(Intent.EXTRA_TITLE, "${stem}_truthnegative_tn3_v0_69.trn")
        }
        startActivityForResult(intent, REQUEST_SAVE_TRUTHNEGATIVE)
    }

    @Suppress("DEPRECATION")
    private fun launchFullResRestorationExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        if (!job.source.format.nativeProcessingReady || job.source.format.id != "DNG") {
            fullResRestorationStatus =
                "Full-resolution Restoration is momenteel alleen beschikbaar voor de volledig admitted DNG-route."
            render()
            return
        }
        pendingFullResRestorationJobId = job.id
        fullResRestorationStatus = null
        val stem = job.source.displayName.substringBeforeLast('.', job.source.displayName)
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/octet-stream"
            putExtra(Intent.EXTRA_TITLE, "${stem}_draw_fullres_restoration_v0_67.trr")
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(intent, REQUEST_SAVE_FULLRES_RESTORATION)
    }

    @Suppress("DEPRECATION")
    private fun launchRestorationProjection(
        job: RawJob,
        format: RestorationProjectionFormat,
    ) {
        val restoration = FullResRestorationJobStore.read(this)
        if (restoration == null ||
            restoration.jobId != job.id ||
            restoration.phase != FullResRestorationJobPhase.SUCCESS
        ) {
            projectionStatus =
                "Projectie geblokkeerd: eerst een volledig geverifieerde full-resolution .trr voor deze bron maken."
            render()
            return
        }
        val existingProjection = RestorationProjectionJobStore.recoverInterruptedIfNeeded(this)
        if (existingProjection != null && !existingProjection.phase.terminal) {
            projectionStatus =
                "${existingProjection.format.label} is nog bezig (${existingProjection.phase.name.lowercase()}). " +
                    "Wacht op de gereedmelding voordat je een tweede projectie start."
            render()
            return
        }

        setPendingProjectionFormat(format)
        projectionStatus = null
        val stem = job.source.displayName.substringBeforeLast('.', job.source.displayName)
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = format.mimeType
            putExtra(Intent.EXTRA_TITLE, "${stem}_draw_restoration_v0_72.${format.extension}")
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(intent, REQUEST_SAVE_RESTORATION_PROJECTION)
    }

    private fun hasPendingProjectionPicker(): Boolean =
        getSharedPreferences(PROJECTION_PICKER_PREFS, MODE_PRIVATE)
            .contains(KEY_PENDING_PROJECTION_FORMAT)

    private fun restoreProjectionSourceSession() {
        val restoration = FullResRestorationJobStore.read(this) ?: return
        if (restoration.phase != FullResRestorationJobPhase.SUCCESS) return
        val sourceUri = runCatching { android.net.Uri.parse(restoration.sourceUri) }.getOrNull() ?: return
        val restored = runCatching {
            RawIngress.readHandlesOnly(
                contentResolver,
                listOf(sourceUri),
                Intent.FLAG_GRANT_READ_URI_PERMISSION,
            ).firstOrNull()
        }.getOrNull() ?: return
        val job = restored.copy(id = restoration.jobId)
        session = session.withJobs(listOf(job))
        activeJobId = job.id
        previewState = TilePreviewUiState.Idle
        requestPreview(job)
    }

    private fun setPendingProjectionFormat(format: RestorationProjectionFormat) {
        pendingProjectionFormat = format
        getSharedPreferences(PROJECTION_PICKER_PREFS, MODE_PRIVATE).edit()
            .putString(KEY_PENDING_PROJECTION_FORMAT, format.name)
            .commit()
    }

    private fun consumePendingProjectionFormat(): RestorationProjectionFormat? {
        val persisted = getSharedPreferences(PROJECTION_PICKER_PREFS, MODE_PRIVATE)
            .getString(KEY_PENDING_PROJECTION_FORMAT, null)
        val format = pendingProjectionFormat ?: persisted?.let {
            runCatching { RestorationProjectionFormat.valueOf(it) }.getOrNull()
        }
        pendingProjectionFormat = null
        getSharedPreferences(PROJECTION_PICKER_PREFS, MODE_PRIVATE).edit()
            .remove(KEY_PENDING_PROJECTION_FORMAT)
            .apply()
        return format
    }

    @Suppress("DEPRECATION")
    private fun launchLinearDngExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        pendingLinearDngJobId = job.id
        pureFloatDngStatus = null
        truthNegativeStatus = null
        fullResRestorationStatus = null
        linearDngStatus = null
        val stem = job.source.displayName.substringBeforeLast('.', job.source.displayName)
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "image/x-adobe-dng"
            putExtra(Intent.EXTRA_TITLE, "${stem}_draw_linear_v0_1.dng")
        }
        startActivityForResult(intent, REQUEST_SAVE_LINEAR_DNG)
    }

    @Suppress("DEPRECATION")
    private fun launchEmpiricalExport(job: RawJob) {
        val audit = empiricalAudit ?: return
        if (job.id != activeJobId) return
        pendingEmpiricalJobId = job.id
        pendingEmpiricalJson = EmpiricalReportEncoder.toJson(this, job, previewState, audit)
        empiricalStatus = null
        val stem = job.source.displayName.substringBeforeLast('.', job.source.displayName)
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(Intent.EXTRA_TITLE, "${stem}_draw_raw_ingress_empirical_v0_2.json")
        }
        startActivityForResult(intent, REQUEST_SAVE_EMPIRICAL_JSON)
    }

    @Suppress("DEPRECATION")
    private fun launchNefMeasurementExport(job: RawJob) {
        val ready = nefMeasurementResult as? NefMeasurementResult.Ready ?: return
        if (job.id != activeJobId) return
        pendingNefMeasurementJobId = job.id
        pendingNefMeasurementJson = NefMeasurementReportEncoder.toJson(job, ready)
        nefMeasurementExportStatus = null
        val stem = job.source.displayName.substringBeforeLast('.', job.source.displayName)
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(Intent.EXTRA_TITLE, "${stem}_draw_nef_measurement_v0_58.json")
        }
        startActivityForResult(intent, REQUEST_SAVE_NEF_MEASUREMENT_JSON)
    }

    @Deprecated("Platform result bridge is intentionally dependency-light in this research prototype")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)

        if (requestCode == REQUEST_SAVE_JPEG) {
            val expectedJob = pendingJpegJobId
            pendingJpegJobId = null
            val route = pendingPhotoRoute ?: preferredRoute()
            val flags = pendingPhotoFlags
            val quarterTurns = pendingPhotoQuarterTurns
            pendingPhotoRoute = null
            pendingPhotoFlags = 0
            pendingPhotoQuarterTurns = 0
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                jpegStatus = "JPG-export geannuleerd."
                render()
                return
            }
            val job = session.jobs.firstOrNull { it.id == expectedJob }
            val ready = previewState as? TilePreviewUiState.Ready
            if (expectedJob == null || job == null || ready == null ||
                ready.jobId != expectedJob || activeJobId != expectedJob
            ) {
                jpegStatus = "JPG-export geblokkeerd: actieve D.RAW-route veranderde."
                render()
                return
            }

            val operationKey = backgroundOperationKey("jpeg", expectedJob)
            if (!startBackgroundOperation(operationKey, "JPG full-resolution opbouwen")) {
                jpegStatus = "JPG achtergrondverwerking kon niet veilig starten."
                render()
                return
            }
            jpegStatus = "JPG · full-resolution $route wordt opgebouwd… 384px-preview wordt niet gebruikt."
            render()
            startGuardedBackgroundThread(
                name = "truthraw-fullres-jpg-${job.id.take(8)}",
                operationKey = operationKey,
                onUnexpected = { jpegStatus = it },
            ) {
                val dir = File(filesDir, "photo_export/$expectedJob").apply { mkdirs() }
                val rendered = FullResJpegExporter.renderToPrivateJpeg(
                    contentResolver, job, flags, quarterTurns, dir,
                )
                var jpegOutputPreview: UnifiedOutputPreviewResult.Ready? = null
                var status = when (rendered) {
                    is FullResJpegResult.Failed -> rendered.reason
                    is FullResJpegResult.Success -> {
                        val ok = FullResJpegExporter.commit(
                            contentResolver,
                            rendered.file,
                            destination,
                            rendered.metrics.jpegSha256,
                        )
                        val m = rendered.metrics
                        rendered.file.delete()
                        if (!ok) {
                            runCatching { contentResolver.delete(destination, null, null) }
                            "JPG commit/post-write SHA-verify faalde."
                        } else {
                            when (
                                val preview = UnifiedOutputPreviewLoader.loadSavedJpeg(
                                    contentResolver,
                                    destination,
                                    "JPG full-resolution " + route,
                                    384,
                                )
                            ) {
                                is UnifiedOutputPreviewResult.Ready ->
                                    jpegOutputPreview = preview
                                is UnifiedOutputPreviewResult.Failed ->
                                    jpegStatus =
                                        "JPG opgeslagen; uitkomst-preview faalde: " +
                                            preview.reason
                            }
                            "JPG full-resolution gereed · ${m.width}×${m.height} · " +
                                "${formatBytes(m.jpegBytes)} · route=$route · detail=${m.detailApplied} · " +
                                "Light pixels=${m.lightAdjustedPixels} · Scientific Master/Backplane=${m.scientificMasterBound}/${m.backplaneBound} · " +
                                "rotatie=${quarterTurns * 90}° · HDR-front=${m.hdrBakedIntoFront} (APPEARANCE_ONLY) · " +
                                "Restoration-front=${m.restorationBakedIntoFront} (AESTHETIC_REINTEGRATION_ONLY)."
                        }
                    }
                }
                finishBackgroundOperation(
                    operationKey,
                    rendered is FullResJpegResult.Success &&
                        status.startsWith("JPG full-resolution gereed"),
                    status,
                )
                runOnUiThread {
                    if (activeJobId == expectedJob) {
                        jpegOutputPreview?.let { preview ->
                            unifiedOutputPreviewState?.bitmap?.recycle()
                            unifiedOutputPreviewState = preview
                        }
                        jpegStatus = status
                        render()
                    } else {
                        jpegOutputPreview?.bitmap?.recycle()
                    }
                }
            }
            return
        }

        if (requestCode == REQUEST_SAVE_FULL_COLOUR_MASTER) {
            val expectedJob = pendingFullColourMasterJobId
            pendingFullColourMasterJobId = null
            val route = pendingPhotoRoute ?: preferredRoute()
            val flags = pendingPhotoFlags
            val quarterTurns = pendingPhotoQuarterTurns
            pendingPhotoRoute = null
            pendingPhotoFlags = 0
            pendingPhotoQuarterTurns = 0
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                fullColourMasterStatus = "Full Colour Scientific Master-export geannuleerd."
                render()
                return
            }
            val job = session.jobs.firstOrNull { it.id == expectedJob }
            val ready = previewState as? TilePreviewUiState.Ready
            if (expectedJob == null || job == null || ready == null ||
                ready.jobId != expectedJob || activeJobId != expectedJob
            ) {
                fullColourMasterStatus = "Full Colour Scientific Master geblokkeerd: actieve D.RAW-route veranderde."
                render()
                return
            }

            val operationKey = backgroundOperationKey("full-colour-scientific-master", expectedJob)
            if (!startBackgroundOperation(operationKey, "Full Colour Scientific Master Float32 DNG opbouwen")) {
                fullColourMasterStatus = "Full Colour Scientific Master kon niet veilig in de achtergrond starten."
                render()
                return
            }
            fullColourMasterStatus =
                "Full Colour Scientific Master · camera-native 32-bit Float DNG wordt opgebouwd… " +
                    "Scientific Master is de primaire LinearRaw; JPEG blijft alleen preview."
            render()
            startGuardedBackgroundThread(
                name = "truthraw-full-colour-master-${job.id.take(8)}",
                operationKey = operationKey,
                onUnexpected = { fullColourMasterStatus = it },
            ) {
                val dir = File(filesDir, "full_colour_scientific_master/$expectedJob").apply { mkdirs() }
                val previewResult = FullResJpegExporter.renderToPrivateJpeg(
                    contentResolver,
                    job,
                    flags,
                    quarterTurns,
                    dir,
                )
                val exportResult = when (previewResult) {
                    is FullResJpegResult.Failed ->
                        PureFloat32DngExportResult.Failed(
                            "Full Colour Scientific Master preview faalde: ${previewResult.reason}",
                        )
                    is FullResJpegResult.Success -> {
                        try {
                            PureFloat32DngExporter.export(
                                contentResolver,
                                job,
                                destination,
                                quarterTurns,
                                Float32DngExportFlavor.FULL_COLOUR_SCIENTIFIC_MASTER,
                                flags,
                                previewResult.file,
                                previewResult.metrics.width,
                                previewResult.metrics.height,
                                unifiedOutputPreviewFile =
                                    File(dir, "unified_output_preview.uop1"),
                                unifiedOutputPreviewMaxEdge = 384,
                            )
                        } finally {
                            previewResult.file.delete()
                        }
                    }
                }
                finishBackgroundOperation(
                    operationKey,
                    exportResult is PureFloat32DngExportResult.Success,
                    when (exportResult) {
                        is PureFloat32DngExportResult.Success -> "Full Colour Scientific Master gereed."
                        is PureFloat32DngExportResult.Failed -> exportResult.reason
                    },
                )
                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    if (exportResult is PureFloat32DngExportResult.Success) {
                        unifiedOutputPreviewState?.bitmap?.recycle()
                        unifiedOutputPreviewState = exportResult.unifiedOutputPreview
                    }
                    fullColourMasterStatus = when (exportResult) {
                        is PureFloat32DngExportResult.Failed -> exportResult.reason
                        is PureFloat32DngExportResult.Success -> {
                            val m = exportResult.metrics
                            "Full Colour Scientific Master v0.1 gereed · ${m.width}×${m.height} · " +
                                "${formatBytes(m.outputBytes)} · camera-native IEEE Float32 LinearRaw primary · " +
                                "negatief/>1=${m.negativeComponentCount}/${m.overOneComponentCount} · " +
                                "Scientific Master replay=${m.scientificMasterIdentityVerified} · " +
                                "self-binding=${m.postWriteSelfBindingVerified} · " +
                                "JPEG=preview-only · rotatie=${quarterTurns * 90}°."
                        }
                    }
                    render()
                }
            }
            return
        }

        if (requestCode == REQUEST_SAVE_TRUTHNEGATIVE_200MP_FULL_COLOUR) {
            val expectedJob = pendingTruthNegative200MpJobId
            pendingTruthNegative200MpJobId = null
            val route = pendingPhotoRoute ?: preferredRoute()
            val quarterTurns = pendingPhotoQuarterTurns
            pendingPhotoRoute = null
            pendingPhotoFlags = 0
            pendingPhotoQuarterTurns = 0
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                truthNegative200MpStatus = "TruthNegative 200MP-export geannuleerd."
                render()
                return
            }
            val job = session.jobs.firstOrNull { it.id == expectedJob }
            val ready = previewState as? TilePreviewUiState.Ready
            if (expectedJob == null || job == null || ready == null ||
                ready.jobId != expectedJob || activeJobId != expectedJob
            ) {
                truthNegative200MpStatus =
                    "TruthNegative 200MP geblokkeerd: actieve D.RAW-route veranderde."
                render()
                return
            }

            val operationKey =
                backgroundOperationKey("truthnegative-200mp-full-colour", expectedJob)
            if (!startBackgroundOperation(
                    operationKey,
                    "TruthNegative 200MP Full Colour Float32 DNG opbouwen",
                )
            ) {
                truthNegative200MpStatus =
                    "TruthNegative 200MP kon niet veilig in de achtergrond starten."
                render()
                return
            }
            truthNegative200MpStatus =
                "TruthNegative 200MP · 16320×12288 camera-native Float32 wordt opgebouwd… " +
                    "targetpixels zijn RECONSTRUCTED_DENSE_SUPPORT; gemeten-targetclaims=0."
            render()

            startGuardedBackgroundThread(
                name = "truthraw-truthnegative-200mp-${job.id.take(8)}",
                operationKey = operationKey,
                onUnexpected = { truthNegative200MpStatus = it },
            ) {
                val dir = File(
                    filesDir,
                    "truthnegative_200mp_full_colour/$expectedJob",
                ).apply { mkdirs() }
                val previewResult = FullResJpegExporter.renderToPrivateJpeg(
                    contentResolver,
                    job,
                    0,
                    quarterTurns,
                    dir,
                )
                val exportResult = when (previewResult) {
                    is FullResJpegResult.Failed ->
                        PureFloat32DngExportResult.Failed(
                            "TruthNegative 200MP preview faalde: ${previewResult.reason}",
                        )
                    is FullResJpegResult.Success -> {
                        try {
                            PureFloat32DngExporter.export(
                                contentResolver,
                                job,
                                destination,
                                quarterTurns,
                                Float32DngExportFlavor.TRUTHNEGATIVE_200MP_FULL_COLOUR,
                                0,
                                previewResult.file,
                                previewResult.metrics.width,
                                previewResult.metrics.height,
                                unifiedOutputPreviewFile =
                                    File(dir, "unified_output_preview.uop1"),
                                unifiedOutputPreviewMaxEdge = 384,
                            )
                        } finally {
                            previewResult.file.delete()
                        }
                    }
                }
                finishBackgroundOperation(
                    operationKey,
                    exportResult is PureFloat32DngExportResult.Success,
                    when (exportResult) {
                        is PureFloat32DngExportResult.Success ->
                            "TruthNegative 200MP Full Colour gereed."
                        is PureFloat32DngExportResult.Failed -> exportResult.reason
                    },
                )
                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    if (exportResult is PureFloat32DngExportResult.Success) {
                        unifiedOutputPreviewState?.bitmap?.recycle()
                        unifiedOutputPreviewState = exportResult.unifiedOutputPreview
                    }
                    truthNegative200MpStatus = when (exportResult) {
                        is PureFloat32DngExportResult.Failed -> exportResult.reason
                        is PureFloat32DngExportResult.Success -> {
                            val m = exportResult.metrics
                            "TruthNegative 200MP v0.1 gereed · ${m.width}×${m.height} · " +
                                "${formatBytes(m.outputBytes)} · IEEE Float32 LinearRaw · " +
                                "negatief/>1=${m.negativeComponentCount}/${m.overOneComponentCount} · " +
                                "projected identity=verified · source Scientific Master ongewijzigd · " +
                                "authority=RESAMPLED_FAIL_CLOSED_UNKNOWN · " +
                                "rotatie=${quarterTurns * 90}°."
                        }
                    }
                    render()
                }
            }
            return
        }

        if (requestCode == REQUEST_SAVE_ADVANCED_RENDER_EDIT) {
            val expectedJob = pendingRenderEditJobId
            pendingRenderEditJobId = null
            val flags = pendingRenderEditFlags
            val quarterTurns = pendingRenderEditQuarterTurns
            pendingRenderEditFlags = 0
            pendingRenderEditQuarterTurns = 0
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                renderEditStatus = "ADVANCED Render/Edit-export geannuleerd."
                render()
                return
            }

            val job = session.jobs.firstOrNull { it.id == expectedJob }
            val ready = previewState as? TilePreviewUiState.Ready
            if (expectedJob == null || job == null || ready == null ||
                ready.jobId != expectedJob || activeJobId != expectedJob
            ) {
                renderEditStatus =
                    "ADVANCED Render/Edit geblokkeerd: actieve D.RAW-route veranderde."
                render()
                return
            }

            val operationKey =
                backgroundOperationKey("advanced-render-edit", expectedJob)
            if (!startBackgroundOperation(
                    operationKey,
                    "ADVANCED Render/Edit Float32 DNG opbouwen",
                )
            ) {
                renderEditStatus =
                    "ADVANCED Render/Edit achtergrondverwerking kon niet veilig starten."
                render()
                return
            }

            renderEditStatus =
                "ADVANCED Render/Edit · extended-linear Float32 derivative wordt opgebouwd… " +
                    "projected-raster SHA → replay-verificatie → DNG commit."
            render()

            startGuardedBackgroundThread(
                name = "truthraw-render-edit-${job.id.take(8)}",
                operationKey = operationKey,
                onUnexpected = { renderEditStatus = it },
            ) {
                val dir = File(
                    filesDir,
                    "advanced_render_edit/$expectedJob",
                ).apply { mkdirs() }

                val previewResult = FullResJpegExporter.renderToPrivateJpeg(
                    contentResolver,
                    job,
                    flags,
                    quarterTurns,
                    dir,
                )

                val exportResult = when (previewResult) {
                    is FullResJpegResult.Failed ->
                        PureFloat32DngExportResult.Failed(
                            "Render/Edit embedded preview faalde: " +
                                previewResult.reason,
                        )

                    is FullResJpegResult.Success -> {
                        try {
                            PureFloat32DngExporter.export(
                                contentResolver,
                                job,
                                destination,
                                quarterTurns,
                                Float32DngExportFlavor.ADVANCED_RENDER_EDIT,
                                flags,
                                previewResult.file,
                                previewResult.metrics.width,
                                previewResult.metrics.height,
                                unifiedOutputPreviewFile =
                                    File(dir, "unified_output_preview.uop1"),
                                unifiedOutputPreviewMaxEdge = 384,
                            )
                        } finally {
                            previewResult.file.delete()
                        }
                    }
                }

                val finalMessage = when (exportResult) {
                    is PureFloat32DngExportResult.Failed -> exportResult.reason
                    is PureFloat32DngExportResult.Success -> {
                        val m = exportResult.metrics
                        "ADVANCED Render/Edit v0.1 gereed · " +
                            "${m.width}×${m.height} · ${formatBytes(m.outputBytes)} · " +
                            "IEEE Float32 derivative · negatief/>1=" +
                            "${m.negativeComponentCount}/${m.overOneComponentCount} · " +
                            "appearance in primary=${m.appearanceApplied} · " +
                            "Scientific Master blijft parent · " +
                            "projected/edit binding=${m.postWriteSelfBindingVerified} · " +
                            "Natural HDR=recipe-only · Output Acutance=not baked · " +
                            "rotatie=${quarterTurns * 90}°."
                    }
                }

                finishBackgroundOperation(
                    operationKey,
                    exportResult is PureFloat32DngExportResult.Success,
                    finalMessage,
                )

                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    if (exportResult is PureFloat32DngExportResult.Success) {
                        unifiedOutputPreviewState?.bitmap?.recycle()
                        unifiedOutputPreviewState = exportResult.unifiedOutputPreview
                    }
                    renderEditStatus = finalMessage
                    render()
                }
            }
            return
        }

        if (requestCode == REQUEST_SAVE_PURE_FLOAT_DNG) {
            val expectedJob = pendingPureFloatDngJobId
            pendingPureFloatDngJobId = null
            val quarterTurns = pendingPureQuarterTurns
            pendingPureQuarterTurns = 0
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                pureFloatDngStatus = "D.RAW PURE Float32-export geannuleerd."
                render()
                return
            }
            val job = session.jobs.firstOrNull { it.id == expectedJob }
            val ready = previewState as? TilePreviewUiState.Ready
            if (expectedJob == null || job == null || ready == null ||
                ready.jobId != expectedJob || activeJobId != expectedJob
            ) {
                pureFloatDngStatus =
                    "D.RAW PURE Float32-export geblokkeerd: actieve finalized preview veranderde."
                render()
                return
            }

            val operationKey = backgroundOperationKey("pure-float32", expectedJob)
            if (!startBackgroundOperation(operationKey, "PURE Float32 DNG opbouwen")) {
                pureFloatDngStatus = "PURE Float32 achtergrondverwerking kon niet veilig starten."
                render()
                return
            }
            pureFloatDngStatus =
                "D.RAW PURE · 32-bit Float DNG wordt opgebouwd… exact Master replay + digest gate."
            render()

            startGuardedBackgroundThread(
                name = "truthraw-pure-f32-${job.id.take(8)}",
                operationKey = operationKey,
                onUnexpected = { pureFloatDngStatus = it },
            ) {
                val dir = File(filesDir, "pure_float32/$expectedJob").apply { mkdirs() }
                val previewResult = FullResJpegExporter.renderToPrivateJpeg(
                    contentResolver,
                    job,
                    0,
                    quarterTurns,
                    dir,
                )
                val preview = previewResult as? FullResJpegResult.Success
                val exportResult = try {
                    PureFloat32DngExporter.export(
                        contentResolver,
                        job,
                        destination,
                        quarterTurns,
                        Float32DngExportFlavor.PURE,
                        0,
                        preview?.file,
                        preview?.metrics?.width ?: 0,
                        preview?.metrics?.height ?: 0,
                        unifiedOutputPreviewFile =
                            File(dir, "unified_output_preview.uop1"),
                        unifiedOutputPreviewMaxEdge = 384,
                    )
                } finally {
                    preview?.file?.delete()
                }
                finishBackgroundOperation(
                    operationKey,
                    exportResult is PureFloat32DngExportResult.Success,
                    when (exportResult) {
                        is PureFloat32DngExportResult.Success -> "PURE Float32 DNG gereed."
                        is PureFloat32DngExportResult.Failed -> exportResult.reason
                    },
                )
                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    if (exportResult is PureFloat32DngExportResult.Success) {
                        unifiedOutputPreviewState?.bitmap?.recycle()
                        unifiedOutputPreviewState = exportResult.unifiedOutputPreview
                    }
                    pureFloatDngStatus = when (exportResult) {
                        is PureFloat32DngExportResult.Failed -> exportResult.reason
                        is PureFloat32DngExportResult.Success -> {
                            val m = exportResult.metrics
                            val previewText =
                                if (preview != null) "embedded neutral JPEG preview" else "zonder preview"
                            "D.RAW PURE opgeslagen + teruggelezen · ${m.width}×${m.height} · " +
                                "${formatBytes(m.outputBytes)} · 32-bit IEEE Float · $previewText · " +
                                "negatief/>1=${m.negativeComponentCount}/${m.overOneComponentCount} · " +
                                "Master digest verified=${m.scientificMasterIdentityVerified} · " +
                                "self-binding verified=${m.postWriteSelfBindingVerified} · " +
                                "frame/evidence=${m.physicalFrameCount}/${m.independentEvidenceCount} · " +
                                "oriëntatie-tag +${quarterTurns * 90}° · geen clipping, appearance of counterfactual."
                        }
                    }
                    render()
                }
            }
            return
        }


        if (requestCode == REQUEST_SAVE_TRUTHNEGATIVE_NATIVE_CONTAINER) {
            val expectedJob = pendingTruthNegativeNativeContainerJobId
            pendingTruthNegativeNativeContainerJobId = null
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                truthNegativeNativeContainerStatus =
                    "Native TruthNegative container-export geannuleerd."
                render()
                return
            }

            val job = session.jobs.firstOrNull { it.id == expectedJob }
            val ready = previewState as? TilePreviewUiState.Ready
            if (expectedJob == null ||
                job == null ||
                ready == null ||
                ready.jobId != expectedJob ||
                activeJobId != expectedJob
            ) {
                truthNegativeNativeContainerStatus =
                    "Native container geblokkeerd: actieve Scientific Master-route veranderde."
                render()
                return
            }

            val operationKey =
                backgroundOperationKey(
                    "truthnegative-native-container",
                    expectedJob,
                )
            if (truthNegativeHeavyOperationActive(expectedJob, operationKey)) {
                truthNegativeNativeContainerStatus =
                    "Wacht op de andere TruthNegative/Camera-5 analysetaak. " +
                        "Native export start daarna opnieuw handmatig."
                render()
                return
            }
            if (!startBackgroundOperation(
                    operationKey,
                    "TruthNegative Native v0.1 export + import verify",
                )
            ) {
                truthNegativeNativeContainerStatus =
                    "Native container achtergrondverwerking kon niet veilig starten."
                render()
                return
            }

            truthNegativeNativeContainerStatus =
                "TruthNegative Native v0.1 schrijft Float32 Scientific Master-values + " +
                    "Open Scene authority en opent het resultaat daarna opnieuw voor identity round-trip…"
            render()

            startGuardedBackgroundThread(
                name = "draw-tn-native-" + job.id.take(8),
                operationKey = operationKey,
                onUnexpected = {
                    truthNegativeNativeContainerStatus = it
                },
            ) {
                val exportResult =
                    TruthNegativeNativeContainerExporter.export(
                        contentResolver,
                        job,
                        destination,
                    )
                finishBackgroundOperation(
                    operationKey,
                    exportResult is TruthNegativeNativeContainerResult.Success,
                    when (exportResult) {
                        is TruthNegativeNativeContainerResult.Success ->
                            "TruthNegative Native v0.1 export/import geverifieerd."
                        is TruthNegativeNativeContainerResult.Failed ->
                            exportResult.reason
                    },
                )
                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    truthNegativeNativeContainerStatus =
                        when (exportResult) {
                            is TruthNegativeNativeContainerResult.Failed ->
                                exportResult.reason
                            is TruthNegativeNativeContainerResult.Success -> {
                                val m = exportResult.metrics
                                "TruthNegative Native v0.1 opgeslagen + native teruggelezen · " +
                                    m.width + "×" + m.height +
                                    " · " + formatBytes(m.fileBytes) +
                                    " · tiles=" + m.tileCount +
                                    " · records=" + m.recordCount +
                                    " · import=" + m.nativeImportVerified +
                                    " · authority-roundtrip=" +
                                    m.authorityRoundtripVerified +
                                    " · state-roundtrip=" +
                                    m.stateRoundtripVerified +
                                    " · TN=" +
                                    m.truthNegativeStateSha256.take(16) +
                                    "… · container=" +
                                    m.containerSha256.take(16) +
                                    "… · frame/evidence=1/1 · writeback=false."
                            }
                        }
                    render()
                }
            }
            return
        }

        if (requestCode == REQUEST_SAVE_TRUTHNEGATIVE) {
            val expectedJob = pendingTruthNegativeJobId
            pendingTruthNegativeJobId = null
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                truthNegativeStatus = "TRUTHNEGATIVE-export geannuleerd."
                render()
                return
            }
            val job = session.jobs.firstOrNull { it.id == expectedJob }
            val ready = previewState as? TilePreviewUiState.Ready
            if (expectedJob == null || job == null || ready == null ||
                ready.jobId != expectedJob || activeJobId != expectedJob
            ) {
                truthNegativeStatus =
                    "TRUTHNEGATIVE-export geblokkeerd: actieve Scientific Master-route veranderde."
                render()
                return
            }

            val operationKey = backgroundOperationKey("truthnegative", expectedJob)
            if (!startBackgroundOperation(operationKey, "TRUTHNEGATIVE TN-4 opbouwen")) {
                truthNegativeStatus = "TRUTHNEGATIVE achtergrondverwerking kon niet veilig starten."
                render()
                return
            }
            truthNegativeStatus =
                "TRUTHNEGATIVE TN-4 wordt opgebouwd… exact Master replay + Dynamic Authority + canonical Open Scene v0.70."
            render()

            startGuardedBackgroundThread(
                name = "truthnegative-tn4-${job.id.take(8)}",
                operationKey = operationKey,
                onUnexpected = { truthNegativeStatus = it },
            ) {
                val dir = File(
                    filesDir,
                    "truthnegative_tn4/$expectedJob",
                ).apply { mkdirs() }
                val exportResult = TruthNegativeExporter.export(
                    contentResolver,
                    job,
                    destination,
                    unifiedOutputPreviewFile =
                        File(dir, "unified_output_preview.uop1"),
                    unifiedOutputPreviewMaxEdge = 384,
                )
                finishBackgroundOperation(
                    operationKey,
                    exportResult is TruthNegativeExportResult.Success,
                    when (exportResult) {
                        is TruthNegativeExportResult.Success -> "TRUTHNEGATIVE TN-4 gereed."
                        is TruthNegativeExportResult.Failed -> exportResult.reason
                    },
                )
                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    if (exportResult is TruthNegativeExportResult.Success) {
                        unifiedOutputPreviewState?.bitmap?.recycle()
                        unifiedOutputPreviewState = exportResult.unifiedOutputPreview
                    }
                    truthNegativeStatus = when (exportResult) {
                        is TruthNegativeExportResult.Failed -> exportResult.reason
                        is TruthNegativeExportResult.Success -> {
                            val m = exportResult.metrics
                            "TRUTHNEGATIVE TN-4 opgeslagen + teruggelezen · ${m.width}×${m.height} · " +
                                "${formatBytes(m.outputBytes)} · camera-native Float32 · " +
                                "authority CAL/REC/CENS/UNK=${m.calibratedEstimateSamples}/" +
                                "${m.reconstructedSamples}/${m.censoredSamples}/${m.unknownSamples} · " +
                                "Master replay=${m.scientificMasterReplayVerified} · " +
                                "OpenScene finite/censored=${m.openSceneFinitePixels}/${m.openSceneCensoredPixels} · " +
                                "fullOpenScene=${m.fullOpenSceneStateBound} · post-write=${m.postWriteVerified} · " +
                                "frame/evidence=${m.physicalFrameCount}/${m.independentEvidenceCount}."
                        }
                    }
                    render()
                }
            }
            return
        }

        if (requestCode == REQUEST_SAVE_FULLRES_RESTORATION) {
            val expectedJob = pendingFullResRestorationJobId
            pendingFullResRestorationJobId = null
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                fullResRestorationStatus = "Full-resolution Restoration-export geannuleerd."
                render()
                return
            }
            val job = session.jobs.firstOrNull { it.id == expectedJob }
            val ready = previewState as? TilePreviewUiState.Ready
            if (expectedJob == null || job == null || ready == null ||
                ready.jobId != expectedJob || activeJobId != expectedJob
            ) {
                fullResRestorationStatus =
                    "Full-resolution Restoration geblokkeerd: actieve Scientific Master-route veranderde."
                render()
                return
            }

            val started = FullResRestorationForegroundService.start(
                this,
                job,
                destination,
                data?.flags ?: 0,
            )
            fullResRestorationStatus = if (started) {
                "Full-resolution Restoration v0.68 foreground transactie gestart · " +
                    "private staging → verify → body commit → header-last commit → whole-file SHA verify."
            } else {
                FullResRestorationJobStore.read(this)?.message
                    ?: "Full-resolution Restoration foreground job kon niet worden gestart."
            }
            restorationStatusHandler.removeCallbacks(restorationStatusPoll)
            restorationStatusHandler.post(restorationStatusPoll)
            render()
            return
        }

        if (requestCode == REQUEST_SAVE_RESTORATION_PROJECTION) {
            val format = consumePendingProjectionFormat()
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null || format == null) {
                projectionStatus = "Restoration-projectie geannuleerd."
                render()
                return
            }
            val restoration = FullResRestorationJobStore.read(this)
            if (restoration == null || restoration.phase != FullResRestorationJobPhase.SUCCESS) {
                projectionStatus = "Restoration-projectie geblokkeerd: complete .trr ontbreekt."
                render()
                return
            }
            val started = RestorationProjectionForegroundService.start(
                this,
                android.net.Uri.parse(restoration.sourceUri),
                android.net.Uri.parse(restoration.destinationUri),
                destination,
                format,
                data?.flags ?: 0,
            )
            projectionStatus = if (started) {
                "${format.label} v0.72 foreground projectie gestart · private full-resolution staging → Open Scene/role-mask verify → commit → whole-file SHA verify. Gereedmelding volgt in app én notificatie."
            } else {
                RestorationProjectionJobStore.read(this)?.message
                    ?: "${format.label}-projectie kon niet worden gestart."
            }
            restorationStatusHandler.removeCallbacks(restorationStatusPoll)
            restorationStatusHandler.post(restorationStatusPoll)
            render()
            return
        }

        if (requestCode == REQUEST_SAVE_LINEAR_DNG) {
            val expectedJob = pendingLinearDngJobId
            pendingPureFloatDngJobId = null
        pendingTruthNegativeJobId = null
        pendingFullResRestorationJobId = null
        pendingLinearDngJobId = null
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                linearDngStatus = "Linear DNG-export geannuleerd."
                render()
                return
            }
            val job = session.jobs.firstOrNull { it.id == expectedJob }
            val ready = previewState as? TilePreviewUiState.Ready
            if (expectedJob == null || job == null || ready == null || ready.jobId != expectedJob || activeJobId != expectedJob) {
                linearDngStatus = "Linear DNG-export geblokkeerd: actieve finalized preview veranderde tijdens de bestandsdialoog."
                render()
                return
            }
            val operationKey = backgroundOperationKey("linear-dng", expectedJob)
            if (!startBackgroundOperation(operationKey, "Linear DNG opbouwen")) {
                linearDngStatus = "Linear DNG achtergrondverwerking kon niet veilig starten."
                render()
                return
            }
            linearDngStatus = "Linear DNG wordt opgebouwd… finalized gate + camera-native RGB-projectie."
            render()
            startGuardedBackgroundThread(
                name = "truthraw-linear-dng-${job.id.take(8)}",
                operationKey = operationKey,
                onUnexpected = { linearDngStatus = it },
            ) {
                val dir = File(
                    filesDir,
                    "linear_dng/$expectedJob",
                ).apply { mkdirs() }
                val exportResult = LinearDngExporter.export(
                    contentResolver,
                    job,
                    destination,
                    unifiedOutputPreviewFile =
                        File(dir, "unified_output_preview.uop1"),
                    unifiedOutputPreviewMaxEdge = 384,
                )
                finishBackgroundOperation(
                    operationKey,
                    exportResult is LinearDngExportResult.Success,
                    when (exportResult) {
                        is LinearDngExportResult.Success -> "Linear DNG gereed."
                        is LinearDngExportResult.Failed -> exportResult.reason
                    },
                )
                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    if (exportResult is LinearDngExportResult.Success) {
                        unifiedOutputPreviewState?.bitmap?.recycle()
                        unifiedOutputPreviewState = exportResult.unifiedOutputPreview
                    }
                    linearDngStatus = when (exportResult) {
                        is LinearDngExportResult.Failed -> exportResult.reason
                        is LinearDngExportResult.Success -> {
                            val m = exportResult.metrics
                            "Linear DNG opgeslagen · ${m.width}×${m.height} · ${formatBytes(m.outputBytes)} · " +
                                "tiles=${m.tilesWritten} · clipped low/high=${m.samplesClippedLow}/${m.samplesClippedHigh} · " +
                                "frame/evidence=${m.physicalFrameCount}/${m.independentEvidenceCount} · " +
                                "bounded compatibility projection, geen nieuwe evidence/authority."
                        }
                    }
                    render()
                }
            }
            return
        }

        if (requestCode == REQUEST_SAVE_EMPIRICAL_JSON) {
            val expectedJob = pendingEmpiricalJobId
            val report = pendingEmpiricalJson
            pendingEmpiricalJobId = null
            pendingEmpiricalJson = null
            if (resultCode != RESULT_OK || data?.data == null) {
                empiricalStatus = "Empirical JSON-export geannuleerd."
                render()
                return
            }
            if (expectedJob == null || expectedJob != activeJobId || report == null) {
                empiricalStatus = "Empirical JSON-export geblokkeerd: actieve run veranderde tijdens de bestandsdialoog."
                render()
                return
            }
            empiricalStatus = try {
                val stream = contentResolver.openOutputStream(data.data!!, "w")
                    ?: throw IOException("Documentprovider gaf geen outputstream.")
                stream.bufferedWriter(Charsets.UTF_8).use { it.write(report) }
                "Empirical JSON opgeslagen · meetlaag only · verandert geen Scientific Master/authority."
            } catch (error: Exception) {
                "Empirical JSON-export faalde: ${error.message ?: error.javaClass.simpleName}"
            }
            render()
            return
        }

        if (requestCode == REQUEST_SAVE_NEF_MEASUREMENT_JSON) {
            val expectedJob = pendingNefMeasurementJobId
            val report = pendingNefMeasurementJson
            pendingNefMeasurementJobId = null
            pendingNefMeasurementJson = null
            if (resultCode != RESULT_OK || data?.data == null) {
                nefMeasurementExportStatus = "NEF measurement JSON-export geannuleerd."
                render()
                return
            }
            if (expectedJob == null || expectedJob != activeJobId || report == null) {
                nefMeasurementExportStatus = "NEF measurement JSON-export geblokkeerd: actieve bron veranderde."
                render()
                return
            }
            nefMeasurementExportStatus = try {
                val stream = contentResolver.openOutputStream(data.data!!, "w")
                    ?: throw IOException("Documentprovider gaf geen outputstream.")
                stream.bufferedWriter(Charsets.UTF_8).use { it.write(report) }
                "NEF measurement JSON opgeslagen · sealed source + sample-domain authority · geen Scientific Master."
            } catch (error: Exception) {
                "NEF measurement JSON-export faalde: ${error.message ?: error.javaClass.simpleName}"
            }
            render()
            return
        }

        if (requestCode != REQUEST_OPEN_RAW || resultCode != RESULT_OK || data == null) return

        val uris = buildList {
            data.data?.let(::add)
            val clip: ClipData? = data.clipData
            if (clip != null) {
                for (index in 0 until clip.itemCount) add(clip.getItemAt(index).uri)
            }
        }

        val jobs = RawIngress.readHandlesOnly(contentResolver, uris, data.flags)
        session = session.withJobs(jobs)
        val first = session.jobs.firstOrNull()
        jpegStatus = null
        renderEditStatus = null
        pendingRenderEditJobId = null
        pendingRenderEditFlags = 0
        pendingRenderEditQuarterTurns = 0
        pureFloatDngStatus = null
        truthNegativeContinuousStatus = null
        camera5ColorHighlightStatus = null
        pendingTruthNegativeNativeContainerJobId = null
        truthNegativeNativeContainerStatus = null
        linearDngStatus = null
        empiricalStatus = null
        empiricalAudit = null
        if (first == null) {
            activeJobId = null
            loadingStartedAtElapsedMs = null
            previewState = TilePreviewUiState.Idle
            render()
        } else {
            selectJob(first)
            if (first.source.format.nativeProcessingReady) {
                window.decorView.post {
                    if (activeJobId == first.id && previewState is TilePreviewUiState.Idle) {
                        requestPreview(first)
                    }
                }
            }
        }
    }

    private fun selectJob(job: RawJob) {
        (previewState as? TilePreviewUiState.Ready)?.bitmap?.recycle()
        unifiedOutputPreviewState?.bitmap?.recycle()
        unifiedOutputPreviewState = null
        restorationUnifiedPreviewKey = null
        ++previewGeneration
        activeJobId = job.id
        loadingStartedAtElapsedMs = null
        previewState = TilePreviewUiState.Idle
        jpegStatus = null
        fullColourMasterStatus = null
        truthNegative200MpStatus = null
        renderEditStatus = null
        pureFloatDngStatus = null
        truthNegativeContinuousStatus = null
        camera5ColorHighlightStatus = null
        pendingTruthNegativeNativeContainerJobId = null
        truthNegativeNativeContainerStatus = null
        linearDngStatus = null
        empiricalStatus = null
        empiricalAudit = null
        pendingJpegJobId = null
        pendingFullColourMasterJobId = null
        pendingTruthNegative200MpJobId = null
        pendingRenderEditJobId = null
        pendingPhotoRoute = null
        pendingPhotoFlags = 0
        pendingPhotoQuarterTurns = 0
        pendingPureFloatDngJobId = null
        pendingPureQuarterTurns = 0
        pendingLinearDngJobId = null
        pendingEmpiricalJobId = null
        pendingEmpiricalJson = null
        (nefMeasurementResult as? NefMeasurementResult.Ready)?.bitmap?.recycle()
        nefMeasurementResult = null
        nefMeasurementLoading = false
        pendingNefMeasurementJobId = null
        pendingNefMeasurementJson = null
        nefMeasurementExportStatus = null
        render()
    }

    private fun backgroundOperationKey(kind: String, jobId: String): String =
        "main:$kind:$jobId"

    private fun startBackgroundOperation(
        key: String,
        label: String,
    ): Boolean =
        TruthRawMediaProcessingForegroundService.start(
            applicationContext,
            key,
            label,
        )

    private fun finishBackgroundOperation(
        key: String,
        success: Boolean,
        message: String,
    ) {
        if (success) {
            TruthRawMediaProcessingForegroundService.success(applicationContext, key, message)
        } else {
            TruthRawMediaProcessingForegroundService.error(applicationContext, key, message)
        }
    }

    private fun startGuardedBackgroundThread(
        name: String,
        operationKey: String,
        onUnexpected: (String) -> Unit,
        block: () -> Unit,
    ) {
        Thread({
            try {
                block()
            } catch (error: Throwable) {
                val message =
                    "Onverwachte achtergrondfout: ${error.message ?: error.javaClass.simpleName}"
                finishBackgroundOperation(operationKey, false, message)
                runOnUiThread {
                    onUnexpected(message)
                    render()
                }
            }
        }, name).start()
    }

    private fun operationStatusVisual(
        message: String,
        phase: TruthRawOperationPhase,
        startedAtWallMs: Long,
        finishedAtWallMs: Long?,
    ): View = horizontal().apply {
        gravity = Gravity.CENTER_VERTICAL
        val dotColor = when (phase) {
            TruthRawOperationPhase.RUNNING,
            TruthRawOperationPhase.SUCCESS -> Color.rgb(65, 196, 106)
            TruthRawOperationPhase.ERROR -> Color.rgb(232, 73, 73)
            TruthRawOperationPhase.CANCELLED -> palette.textMuted
        }
        addView(View(this@MainActivity).apply {
            background = GradientDrawable().apply {
                shape = GradientDrawable.OVAL
                setColor(dotColor)
            }
            contentDescription = when (phase) {
                TruthRawOperationPhase.RUNNING -> "Proces loopt normaal"
                TruthRawOperationPhase.SUCCESS -> "Proces gereed"
                TruthRawOperationPhase.ERROR -> "Proces onverwacht gestopt"
                TruthRawOperationPhase.CANCELLED -> "Proces geannuleerd"
            }
        }, LinearLayout.LayoutParams(dp(10), dp(10)).apply { marginEnd = dp(8) })

        addView(vertical().apply {
            addView(label(message, 10.5f, muted = true))
            if (phase == TruthRawOperationPhase.RUNNING) {
                addView(Chronometer(this@MainActivity).apply {
                    val elapsedWall =
                        (System.currentTimeMillis() - startedAtWallMs).coerceAtLeast(0L)
                    base = SystemClock.elapsedRealtime() - elapsedWall
                    textSize = 10f
                    setTextColor(palette.textMuted)
                    format = "Looptijd %s"
                    start()
                })
            } else {
                val endWall = finishedAtWallMs ?: System.currentTimeMillis()
                val seconds = ((endWall - startedAtWallMs).coerceAtLeast(0L)) / 1000L
                val prefix = when (phase) {
                    TruthRawOperationPhase.SUCCESS -> "Gereed in"
                    TruthRawOperationPhase.ERROR -> "Gestopt na"
                    TruthRawOperationPhase.CANCELLED -> "Geannuleerd na"
                    TruthRawOperationPhase.RUNNING -> "Looptijd"
                }
                addView(label(
                    "%s %02d:%02d".format(prefix, seconds / 60L, seconds % 60L),
                    9.5f,
                    muted = true,
                ))
            }
        }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
    }

    private fun backgroundOperationStatusView(
        key: String,
        fallbackMessage: String,
    ): View? {
        val state = TruthRawOperationStore.recoverInterruptedIfNeeded(
            this,
            key,
            TruthRawMediaProcessingForegroundService.isActive(key),
        ) ?: return null
        return operationStatusVisual(
            message = state.message.ifBlank { fallbackMessage },
            phase = state.phase,
            startedAtWallMs = state.startedAtWallMs,
            finishedAtWallMs = state.finishedAtWallMs,
        )
    }

    private fun restorationStatusView(
        snapshot: FullResRestorationJobSnapshot,
    ): View = operationStatusVisual(
        message = snapshot.message,
        phase = when (snapshot.phase) {
            FullResRestorationJobPhase.SUCCESS -> TruthRawOperationPhase.SUCCESS
            FullResRestorationJobPhase.FAILED,
            FullResRestorationJobPhase.STALE_CLEANED -> TruthRawOperationPhase.ERROR
            else -> TruthRawOperationPhase.RUNNING
        },
        startedAtWallMs = snapshot.startedAtMs,
        finishedAtWallMs = if (snapshot.phase.terminal) snapshot.updatedAtMs else null,
    )

    private fun restorationProjectionStatusView(
        snapshot: RestorationProjectionJobSnapshot,
    ): View = operationStatusVisual(
        message = snapshot.message,
        phase = when (snapshot.phase) {
            RestorationProjectionJobPhase.SUCCESS -> TruthRawOperationPhase.SUCCESS
            RestorationProjectionJobPhase.FAILED,
            RestorationProjectionJobPhase.STALE_CLEANED -> TruthRawOperationPhase.ERROR
            else -> TruthRawOperationPhase.RUNNING
        },
        startedAtWallMs = snapshot.startedAtMs,
        finishedAtWallMs = if (snapshot.phase.terminal) snapshot.updatedAtMs else null,
    )

    private fun requestNefMeasurement(job: RawJob) {
        if (job.source.format.support != RawIngressSupport.NATIVE_SAMPLE_DECODE_CALIBRATION_PENDING ||
            job.source.format.decoderBackend != RawDecoderBackend.NIKON_NEF_UNCOMPRESSED16_CFA_V0_1
        ) {
            nefMeasurementResult = NefMeasurementResult.Failed(
                "Deze bron heeft geen toegelaten measurement-only NEF-adapter.",
            )
            render()
            return
        }

        (nefMeasurementResult as? NefMeasurementResult.Ready)?.bitmap?.recycle()
        nefMeasurementResult = null
        nefMeasurementLoading = true
        val generation = ++previewGeneration
        activeJobId = job.id
        val operationKey = backgroundOperationKey("nef-measurement", job.id)
        if (!startBackgroundOperation(operationKey, "NEF CFA-samples inspecteren")) {
            nefMeasurementLoading = false
            nefMeasurementResult = NefMeasurementResult.Failed(
                "Android achtergrondverwerking kon niet veilig worden gestart.",
            )
            render()
            return
        }
        render()

        startGuardedBackgroundThread(
            name = "truthraw-nef-measurement-${job.id.take(8)}",
            operationKey = operationKey,
            onUnexpected = { message ->
                nefMeasurementLoading = false
                nefMeasurementResult = NefMeasurementResult.Failed(message)
            },
        ) {
            val result = NefMeasurementLoader.load(contentResolver, job)
            finishBackgroundOperation(
                operationKey,
                result is NefMeasurementResult.Ready,
                when (result) {
                    is NefMeasurementResult.Ready -> "NEF CFA-sample-inspectie gereed."
                    is NefMeasurementResult.Failed -> result.reason
                },
            )
            runOnUiThread {
                if (generation != previewGeneration || activeJobId != job.id) {
                    (result as? NefMeasurementResult.Ready)?.bitmap?.recycle()
                    return@runOnUiThread
                }
                nefMeasurementLoading = false
                nefMeasurementResult = result
                render()
            }
        }
    }

    private fun requestPreview(job: RawJob) {
        if (!job.source.format.nativeProcessingReady) {
            previewState = TilePreviewUiState.Failed(
                job.id,
                "Bestand is veilig als bronhandle opgenomen (${job.source.format.displayLabel}), " +
                    "maar de volledige scientific-admission route is nog niet gekoppeld. " +
                    "De bronbytes blijven onaangeraakt; Scientific Master wordt niet aangemaakt.",
            )
            render()
            return
        }
        (previewState as? TilePreviewUiState.Ready)?.bitmap?.recycle()
        unifiedOutputPreviewState?.bitmap?.recycle()
        unifiedOutputPreviewState = null
        activeJobId = job.id
        jpegStatus = null
        pureFloatDngStatus = null
        linearDngStatus = null
        empiricalStatus = null
        empiricalAudit = null
        pendingJpegJobId = null
        pendingPureFloatDngJobId = null
        pendingLinearDngJobId = null
        pendingEmpiricalJobId = null
        pendingEmpiricalJson = null
        val generation = ++previewGeneration
        loadingStartedAtElapsedMs = SystemClock.elapsedRealtime()
        previewState = TilePreviewUiState.Loading(job.id)
        val operationKey = backgroundOperationKey("preview", job.id)
        if (!startBackgroundOperation(operationKey, "D.RAW foto verwerken")) {
            loadingStartedAtElapsedMs = null
            previewState = TilePreviewUiState.Failed(
                job.id,
                "Android mediaProcessing-service kon niet veilig worden gestart.",
            )
            render()
            return
        }
        val frameSampler = UiFramePacingSampler().also { it.start() }
        render()
        val preferredOutput = getSharedPreferences(
            TruthRawSuiteLauncherActivity.PREFS,
            MODE_PRIVATE,
        ).getString(
            TruthRawSuiteLauncherActivity.KEY_OUTPUT,
            TruthRawSuiteLauncherActivity.OUTPUT_PURE,
        ) ?: TruthRawSuiteLauncherActivity.OUTPUT_PURE

        startGuardedBackgroundThread(
            name = "truthraw-preview-${job.id.take(8)}",
            operationKey = operationKey,
            onUnexpected = { message ->
                loadingStartedAtElapsedMs = null
                previewState = TilePreviewUiState.Failed(job.id, message)
            },
        ) {
            if (preferredOutput == TruthRawSuiteLauncherActivity.OUTPUT_ADVANCED ||
                preferredOutput == TruthRawSuiteLauncherActivity.OUTPUT_PRO
            ) {
                val state = AdvancedTilePreviewLoader.load(
                    applicationContext,
                    contentResolver,
                    job,
                )
                finishBackgroundOperation(
                    operationKey,
                    state is TilePreviewUiState.Ready,
                    when (state) {
                        is TilePreviewUiState.Ready -> "D.RAW Advanced/PRO render gereed."
                        is TilePreviewUiState.Failed -> state.reason
                        is TilePreviewUiState.Loading -> "D.RAW render bleef onverwacht in loading."
                        TilePreviewUiState.Idle -> "D.RAW render gaf onverwacht Idle terug."
                    },
                )
                runOnUiThread {
                    frameSampler.stop()
                    if (generation != previewGeneration || activeJobId != job.id) {
                        (state as? TilePreviewUiState.Ready)?.bitmap?.recycle()
                        return@runOnUiThread
                    }
                    loadingStartedAtElapsedMs = null
                    previewState = state
                    empiricalAudit = null
                    render()
                }
            } else {
                val result = EmpiricalPreviewRunner.run(applicationContext, contentResolver, job)
                finishBackgroundOperation(
                    operationKey,
                    result.state is TilePreviewUiState.Ready,
                    when (val state = result.state) {
                        is TilePreviewUiState.Ready -> "D.RAW PURE render gereed."
                        is TilePreviewUiState.Failed -> state.reason
                        is TilePreviewUiState.Loading -> "D.RAW PURE render bleef onverwacht in loading."
                        TilePreviewUiState.Idle -> "D.RAW PURE render gaf onverwacht Idle terug."
                    },
                )
                runOnUiThread {
                    val pacing = frameSampler.stop()
                    if (generation != previewGeneration || activeJobId != job.id) {
                        (result.state as? TilePreviewUiState.Ready)?.bitmap?.recycle()
                        return@runOnUiThread
                    }
                    loadingStartedAtElapsedMs = null
                    previewState = result.state
                    empiricalAudit = result.audit.copy(framePacing = pacing)
                    render()
                }
            }
        }
    }

    private fun render() {
        val tier = currentLayoutTier()
        val root = vertical().apply {
            setBackgroundColor(palette.background)
            setPadding(dp(12), 0, dp(12), dp(12))
        }

        root.addView(topBar(tier))
        root.addView(
            when (tier) {
                LayoutTier.COMPACT -> compactLayout()
                LayoutTier.MEDIUM -> mediumLayout()
                LayoutTier.EXPANDED -> expandedLayout()
            },
            LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f),
        )

        root.setOnApplyWindowInsetsListener { view, insets ->
            val bars = insets.getInsets(WindowInsets.Type.systemBars())
            view.setPadding(dp(12) + bars.left, bars.top, dp(12) + bars.right, dp(12) + bars.bottom)
            insets
        }

        setContentView(root)
    }

    private fun topBar(tier: LayoutTier): View = horizontal().apply {
        gravity = Gravity.CENTER_VERTICAL
        setPadding(dp(4), dp(8), dp(4), dp(8))

        addView(TextView(this@MainActivity).apply {
            text = "‹"
            textSize = 34f
            setTextColor(palette.text)
            gravity = Gravity.CENTER
            contentDescription = "Terug"
            setOnClickListener { finish() }
        }, LinearLayout.LayoutParams(dp(46), dp(46)).apply { marginEnd = dp(6) })

        addView(vertical().apply {
            addView(label("D.RAW", 22f, bold = true))
            addView(label("${tier.name.lowercase().replaceFirstChar { it.uppercase() }} layout · ${session.selectedCount} RAW geselecteerd", 12f, muted = true))
        }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))

        addView(actionButton("RAW kiezen") { launchRawPicker() })
    }

    private fun compactLayout(): View = ScrollView(this).apply {
        isFillViewport = true
        addView(
            vertical().apply {
                addView(previewPane(), LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
                addView(space(8))
                addView(routePane(), LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
                if (session.jobs.isNotEmpty()) {
                    addView(space(8))
                    addView(jobStrip(), LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(132)))
                }
            },
            ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT),
        )
    }

    private fun mediumLayout(): View = horizontal().apply {
        addView(vertical().apply {
            addView(jobListPane(), LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))
            addView(routePane())
        }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 0.38f).apply { marginEnd = dp(8) })

        addView(previewPane(), LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 0.62f))
    }

    private fun expandedLayout(): View = horizontal().apply {
        addView(jobListPane(), LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 0.25f).apply { marginEnd = dp(8) })
        addView(previewPane(), LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 0.50f).apply { marginEnd = dp(8) })
        addView(toolsPane(), LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 0.25f))
    }

    private fun previewPane(): View = card().apply {
        val active = session.jobs.firstOrNull { it.id == activeJobId } ?: session.jobs.firstOrNull()
        if (active == null) {
            gravity = Gravity.CENTER
            addView(label("Selecteer RAW-bestanden van smartphone of camera", 20f, bold = true).apply { gravity = Gravity.CENTER })
            addView(space(10))
            addView(label(
                "DNG wordt nu native verwerkt. Canon/Nikon/Sony/Fujifilm/Panasonic/OM/Pentax/Leica/Hasselblad/Phase One e.a. " +
                    "mogen al als bronhandle binnenkomen en blijven fail-closed totdat hun decoder-adapter is gekoppeld. " +
                    "RAW-sensorwaarden worden nooit vooraf als volledige buffer gekopieerd.",
                13f,
                muted = true,
            ).apply { gravity = Gravity.CENTER })
            return@apply
        }

        addView(label(active.source.displayName, 16f, bold = true))
        addView(label(
            "${active.source.format.vendorLabel} · ${active.source.format.displayLabel} · " +
                "ingang=${active.source.sourceRoute.name.lowercase()} · decoder=${active.source.format.decoderBackend.name.lowercase()}",
            11f,
            muted = true,
        ))
        if (active.source.sourceRoute == SourceIngressRoute.CAMERA_CAPTURE &&
            active.source.upstreamSealedSourceSha256 != null
        ) {
            addView(label(
                "Acquisitie-ouder=${active.source.upstreamSourceRole ?: "SEALED_CAMERA_SOURCE"} · " +
                    "upstream SHA-256=${active.source.upstreamSealedSourceSha256.take(16)}… · " +
                    "DNG wordt opnieuw zelfstandig sealed/admitted; authority wordt niet geërfd.",
                10.5f,
                muted = true,
            ))
        }
        addView(label(
            "Finalized Scientific Preview · Scientific Master/TruthRange/Backplane-lineage vereist vóór vrijgave",
            11f,
            muted = true,
        ))
        addView(space(8))

        when (val state = previewState) {
            TilePreviewUiState.Idle -> {
                if (active.source.format.nativeProcessingReady) {
                    addView(label(
                        "RAW is geselecteerd en nog niet verwerkt. Start hieronder bewust de empirical + finalized TruthRaw-route.",
                        13f,
                        muted = true,
                    ))
                    addView(space(8))
                    addView(actionButton("Start D.RAW") { requestPreview(active) })
                } else if (
                    active.source.format.support == RawIngressSupport.NATIVE_SAMPLE_DECODE_CALIBRATION_PENDING &&
                    active.source.format.decoderBackend == RawDecoderBackend.NIKON_NEF_UNCOMPRESSED16_CFA_V0_1
                ) {
                    addView(label(
                        "Nikon NEF heeft nu een strikte measurement-only sampledecoder. Exacte CFA-samplecodes mogen " +
                            "worden geïnspecteerd, maar Scientific Master blijft geblokkeerd totdat black level, " +
                            "saturation/noise en kleur-authority zijn toegelaten.",
                        13f,
                        muted = true,
                    ))
                    addView(space(8))
                    if (nefMeasurementLoading) {
                        addView(horizontal().apply {
                            gravity = Gravity.CENTER_VERTICAL
                            addView(ProgressBar(this@MainActivity).apply { isIndeterminate = true },
                                LinearLayout.LayoutParams(dp(32), dp(32)).apply { marginEnd = dp(8) })
                            backgroundOperationStatusView(
                                backgroundOperationKey("nef-measurement", active.id),
                                "NEF CFA-samples worden read-only geïnspecteerd…",
                            )?.let(::addView)
                        })
                    } else {
                        addView(actionButton("Inspecteer NEF CFA-samples") { requestNefMeasurement(active) })
                    }

                    when (val measurement = nefMeasurementResult) {
                        null -> Unit
                        is NefMeasurementResult.Failed -> {
                            addView(space(6))
                            addView(label("NEF measurement fail-closed", 13f, bold = true))
                            addView(label(measurement.reason, 11f, muted = true))
                        }
                        is NefMeasurementResult.Ready -> {
                            addView(space(8))
                            addView(ImageView(this@MainActivity).apply {
                                setImageBitmap(measurement.bitmap)
                                adjustViewBounds = true
                                scaleType = ImageView.ScaleType.FIT_CENTER
                                contentDescription = "Measurement-only CFA sample preview voor ${active.source.displayName}"
                                if (currentLayoutTier() == LayoutTier.COMPACT) {
                                    minimumHeight = dp(180)
                                    maxHeight = dp(320)
                                }
                            })
                            val m = measurement.metrics
                            addView(label(
                                "MEASUREMENT_ONLY · bron ${m.sourceWidth}×${m.sourceHeight} · CFA=${cfaLabel(m.cfaCode)} · " +
                                    "samplecodes preview min/max=${m.minSampleCodeInPreview}/${m.maxSampleCodeInPreview}",
                                10f,
                                muted = true,
                            ))
                            addView(label(
                                "exactCFA=${m.exactCfaSamplesAvailable} · measurementReady=${m.measurementAdmissionReady} · " +
                                    "scientificReady=${m.scientificAdmissionReady} · directADCclaim=${m.directSensorAdcClaimAllowed} · " +
                                    "fullRawMaterialized=${m.fullRawFrameMaterialized}",
                                10f,
                                muted = true,
                            ))
                            addView(label(
                                "Dit beeld is alleen een zichtbaarheidproxy van broncodes; geen black subtraction, demosaic, " +
                                    "kleurcorrectie, Scientific Master of fotografische preview.",
                                10f,
                                muted = true,
                            ))
                            addView(label(
                                "source SHA-256=${m.sourceSha256.take(16)}… · sealed bytes=${formatBytes(m.sourceBytes)}",
                                10f,
                                muted = true,
                            ))
                            addView(space(6))
                            addView(actionButton("NEF measurement JSON opslaan") { launchNefMeasurementExport(active) })
                            nefMeasurementExportStatus?.let { addView(label(it, 10f, muted = true)) }
                        }
                    }
                } else {
                    addView(label(
                        "Bron geaccepteerd als immutable documenthandle. Voor ${active.source.format.displayLabel} " +
                            "is nog geen gevalideerde decoder-adapter gekoppeld; verwerken blijft fail-closed.",
                        13f,
                        muted = true,
                    ))
                    addView(space(6))
                    addView(label(
                        "DNG is de volledige native multi-vendor route. Nikon NEF heeft een eerste beperkte sampledecoder; " +
                            "andere proprietary RAW volgt adapter-voor-adapter zonder bron/provenance-regels te versoepelen.",
                        11f,
                        muted = true,
                    ))
                }
            }
            is TilePreviewUiState.Loading -> {
                addView(horizontal().apply {
                    gravity = Gravity.CENTER_VERTICAL
                    addView(ProgressBar(this@MainActivity).apply { isIndeterminate = true }, LinearLayout.LayoutParams(dp(36), dp(36)).apply {
                        marginEnd = dp(10)
                    })
                    backgroundOperationStatusView(
                        backgroundOperationKey("preview", active.id),
                        "D.RAW foto wordt verwerkt…",
                    )?.let(::addView)
                })
                addView(space(8))
                addView(label(
                    "Empirical pre-probe → onveranderde finalized route → empirical post-probe · SHA-256, DNG-profiel, RSS, latency, thermiek en framepacing worden gemeten.",
                    13f,
                    muted = true,
                ))
                addView(label(
                    "De route kan op een echte volledige RAW merkbaar rekenen. Zolang de looptijd doorloopt en Android de app niet als fout beëindigt, is de worker actief.",
                    11f,
                    muted = true,
                ))
            }
            is TilePreviewUiState.Failed -> {
                addView(label("Preview fail-closed geblokkeerd", 14f, bold = true))
                backgroundOperationStatusView(
                    backgroundOperationKey("preview", active.id),
                    state.reason,
                )?.let(::addView) ?: addView(label(state.reason, 12f, muted = true))
                addView(space(6))
                addView(actionButton("Opnieuw proberen") { requestPreview(active) })
            }
            is TilePreviewUiState.Ready -> {
                backgroundOperationStatusView(
                    backgroundOperationKey("preview", active.id),
                    "D.RAW render gereed.",
                )?.let(::addView)
                addView(space(5))
                val userQuarterTurns =
                    TruthRawOrientationOverride.quarterTurns(this@MainActivity, active.source)
                val image = ImageView(this@MainActivity).apply {
                    setImageBitmap(state.bitmap)
                    adjustViewBounds = true
                    scaleType = ImageView.ScaleType.FIT_CENTER
                    rotation = userQuarterTurns * 90f
                    if (userQuarterTurns % 2 != 0 && state.bitmap.width > 0 && state.bitmap.height > 0) {
                        val ratio = minOf(
                            state.bitmap.width.toFloat() / state.bitmap.height.toFloat(),
                            state.bitmap.height.toFloat() / state.bitmap.width.toFloat(),
                        )
                        scaleX = ratio
                        scaleY = ratio
                    }
                    contentDescription =
                        "Finalized D.RAW Scientific Preview voor ${active.source.displayName}, " +
                            "user rotation +${userQuarterTurns * 90} graden"
                    if (currentLayoutTier() == LayoutTier.COMPACT) {
                        minimumHeight = dp(180)
                        maxHeight = dp(320)
                    }
                }
                val imageLayout = if (currentLayoutTier() == LayoutTier.COMPACT) {
                    LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
                } else {
                    LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f)
                }
                addView(image, imageLayout)
                addView(space(6))
                addView(horizontal().apply {
                    gravity = Gravity.CENTER_VERTICAL
                    addView(actionButton("↻ 90°") {
                        TruthRawOrientationOverride.rotateClockwise(this@MainActivity, active.source)
                        render()
                    }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply {
                        marginEnd = dp(5)
                    })
                    addView(actionButton("Herstel origineel", enabled = userQuarterTurns != 0) {
                        TruthRawOrientationOverride.reset(this@MainActivity, active.source)
                        render()
                    }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply {
                        marginStart = dp(5)
                    })
                })
                addView(label(
                    if (userQuarterTurns == 0) {
                        "Oriëntatie: bronmetadata · geen override"
                    } else {
                        "Oriëntatie-override: +${userQuarterTurns * 90}° met de klok mee · " +
                            "alleen presentatie/projectie; sealed RAW en Scientific Master blijven ongewijzigd."
                    },
                    10f,
                    muted = true,
                ))
                addView(space(6))
                val m = state.metrics
                addView(label(
                    "${m.previewAuthority.name} · scientific release=${m.scientificPreviewReleaseAllowed} · stronger physical-color claim=${m.scientificClaimAllowed}",
                    10f,
                    muted = true,
                ))
                addView(label(
                    "bron ${m.sourceWidth}×${m.sourceHeight} · source resident ≤ ${formatBytes(m.sourceResidentUpperBoundBytes.toLong())} · logical resident ≤ ${formatBytes(m.logicalResidentUpperBoundBytes.toLong())}",
                    10f,
                    muted = true,
                ))
                addView(label(
                    "RAW gelezen ${formatBytes(m.rawPayloadBytesRead.toLong())} · tile passes ${m.tilesProcessedPass1}/${m.tilesProcessedPass2} · fullRawMaterialized=${m.fullRawMaterialized}",
                    10f,
                    muted = true,
                ))
                addView(label(
                    "frame/evidence=${m.physicalFrameCount}/${m.independentEvidenceCount} · ForwardMatrix=${m.usedForwardMatrix} · CameraCalibration toegepast=${m.cameraCalibrationApplied}",
                    10f,
                    muted = true,
                ))
                addView(space(6))
                val preferredOutput = preferredRoute()
                addView(label(
                    "Route: " + DrawRouteLogic.forMode(preferredOutput).displayLabel,
                    11f,
                    bold = true,
                ))

                if (m.advancedDerivative) {
                    addView(label(
                        "Open Scene · illuminationAuthority=${m.openWorldIlluminationAuthority} · " +
                            "outputAuthority=${m.openWorldOutputAuthority} · " +
                            "CAL/REC/CENS/UNK=${m.dynamicAuthorityCalibratedPreviewPixels}/" +
                            "${m.dynamicAuthorityReconstructedPreviewPixels}/" +
                            "${m.dynamicAuthorityCensoredPreviewPixels}/${m.dynamicAuthorityUnknownRgbSamples}",
                        10f,
                        muted = true,
                    ))
                }

                unifiedOutputPreviewState?.let { outputPreview ->
                    addView(space(8))
                    addView(label(
                        "Uitkomst-preview · ${outputPreview.outputLabel}",
                        12f,
                        bold = true,
                    ))
                    addView(label(
                        if (outputPreview.metrics.appearanceAddedByPreview) {
                            "TruthNegative/Free-World derivative · area-integrated scene resolve + " +
                                "expliciete Appearance/Display-laag · source scene en authority blijven immutable."
                        } else {
                            "Zelfde primary tile-source als het opgeslagen resultaat · " +
                                "alleen display-clamp + sRGB-transfer · geen extra HDR/detail/restoration."
                        },
                        10f,
                        muted = true,
                    ))
                    addView(ImageView(this@MainActivity).apply {
                        setImageBitmap(outputPreview.bitmap)
                        adjustViewBounds = true
                        scaleType = ImageView.ScaleType.FIT_CENTER
                        val outputQuarterTurns =
                            outputPreview.metrics.displayQuarterTurns
                        rotation = outputQuarterTurns * 90f
                        if (
                            outputQuarterTurns % 2 != 0 &&
                            outputPreview.bitmap.width > 0 &&
                            outputPreview.bitmap.height > 0
                        ) {
                            val ratio = minOf(
                                outputPreview.bitmap.width.toFloat() /
                                    outputPreview.bitmap.height.toFloat(),
                                outputPreview.bitmap.height.toFloat() /
                                    outputPreview.bitmap.width.toFloat(),
                            )
                            scaleX = ratio
                            scaleY = ratio
                        }
                        contentDescription =
                            "Exacte uitkomst-preview voor ${outputPreview.outputLabel}"
                        if (currentLayoutTier() == LayoutTier.COMPACT) {
                            minimumHeight = dp(160)
                            maxHeight = dp(300)
                        }
                    })
                    val om = outputPreview.metrics
                    val sourceSpace = when (om.sourceSpaceCode) {
                        1 -> "CAMERA_NATIVE"
                        2 -> "LINEAR_SRGB"
                        3 -> "XYZ_D50"
                        4 -> "DISPLAY_SRGB_JPEG"
                        else -> "UNKNOWN"
                    }
                    addView(label(
                        "${om.width}×${om.height} preview uit " +
                            "${om.sourceWidth}×${om.sourceHeight} primary · " +
                            "space=$sourceSpace · outputRotation=${om.displayQuarterTurns * 90}° · " +
                            "directPrimary=${om.primaryTileSourceUsedDirectly} · " +
                            "appearanceAdded=${om.appearanceAddedByPreview} · " +
                            "scientificWriteback=${om.scientificWritebackAllowed}",
                        9.5f,
                        muted = true,
                    ))
                }

                addView(space(8))

                when (preferredOutput) {
                    TruthRawSuiteLauncherActivity.OUTPUT_PURE -> {
                        addView(actionButton("Bewaar PURE · 32-bit Float DNG") {
                            launchPureFloatDngExport(active)
                        })
                        pureFloatDngStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey("pure-float32", active.id),
                                status,
                            )?.let(::addView) ?: addView(label(status, 10f, muted = true))
                        }
                        addView(label(
                            "Directe wetenschappelijke projectie: Float32, negatieve en >1 waarden behouden, " +
                                "Scientific Master digest + self-binding, geen appearance.",
                            10f,
                            muted = true,
                        ))
                        addView(space(7))
                        addView(actionButton("JPG · full resolution compatibility") {
                            launchJpegExport(active)
                        })
                        jpegStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey("jpeg", active.id),
                                status,
                            )?.let(::addView) ?: addView(label(status, 10f, muted = true))
                        }
                    }

                    TruthRawSuiteLauncherActivity.OUTPUT_ADVANCED -> {
                        addView(actionButton("JPG · full resolution") { launchJpegExport(active) })
                        jpegStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey("jpeg", active.id),
                                status,
                            )?.let(::addView) ?: addView(label(status, 10f, muted = true))
                        }
                        addView(space(5))
                        addView(actionButton("Float32 Full Colour Scientific Master · DNG · Lightroom") {
                            launchFullColourScientificMasterExport(active)
                        })
                        fullColourMasterStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey("full-colour-scientific-master", active.id),
                                status,
                            )?.let(::addView) ?: addView(label(status, 10f, muted = true))
                        }
                        addView(label(
                            "Camera-native full-colour Scientific Master RGB wordt rechtstreeks als IEEE Float32 LinearRaw-primary opgeslagen. " +
                                "Negatieve en >1 waarden blijven behouden; er wordt geen Advanced appearance in de primary gebakken. " +
                                "De ingebedde JPEG is uitsluitend een niet-autoritatieve preview.",
                            10f,
                            muted = true,
                        ))
                        addView(space(5))
                        addView(actionButton(
                            "TruthNegative 200MP · Float32 Full Colour · DNG",
                            enabled = active.source.verifiedCamera5TruthNegative200MpEnvelope,
                        ) {
                            launchTruthNegative200MpFullColourExport(active)
                        })
                        truthNegative200MpStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey(
                                    "truthnegative-200mp-full-colour",
                                    active.id,
                                ),
                                status,
                            )?.let(::addView) ?: addView(label(status, 10f, muted = true))
                        }
                        addView(label(
                            if (active.source.verifiedCamera5TruthNegative200MpEnvelope) {
                                "Camera-5 lineage VERIFIED · 4080×3072 Scientific Master → 16320×12288 dense projection. " +
                                    "Ongeveer 2,24 GiB primaire Float32-raster. Alle targetkanalen blijven fail-closed UNKNOWN/RECONSTRUCTED support; " +
                                    "dit claimt geen 200MP gemeten CFA-detail."
                            } else {
                                "Niet beschikbaar voor deze bron: vereist de exact geverifieerde physical Camera-5 " +
                                    "16320×12288 sealed-envelope → 4080×3072 admission-lineage."
                            },
                            10f,
                            muted = true,
                        ))
                        addView(space(5))
                        addView(actionButton("Render/Edit · Float32 DNG · Lightroom") {
                            launchAdvancedRenderEditExport(active)
                        })
                        renderEditStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey("advanced-render-edit", active.id),
                                status,
                            )?.let(::addView) ?: addView(label(status, 10f, muted = true))
                        }
                        addView(label(
                            "Ontwikkelde extended-linear Float32-master: Detail/Light/Restoration kunnen in de primary zitten; " +
                                "negatieve en >1 waarden blijven behouden. Natural HDR blijft recipe-only zolang output authority UNKNOWN bevat; " +
                                "Output Acutance blijft voor finale output.",
                            10f,
                            muted = true,
                        ))
                        addView(space(5))
                        addView(actionButton("Advanced instellingen") {
                            startActivity(Intent(this@MainActivity, TruthRawAdvancedActivity::class.java))
                        })
                        addView(space(5))
                        addView(actionButton("Full-res Restoration · retreatable .trr") {
                            launchFullResRestorationExport(active)
                        })
                        fullResRestorationStatus?.let { status ->
                            val snapshot = FullResRestorationJobStore.read(this@MainActivity)
                            if (snapshot != null && snapshot.jobId == active.id) {
                                addView(restorationStatusView(snapshot))
                            } else {
                                addView(label(status, 10f, muted = true))
                            }
                        }
                        addView(space(5))
                        addView(actionButton("Wetenschappelijke PURE-projectie") {
                            launchPureFloatDngExport(active)
                        })
                        pureFloatDngStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey("pure-float32", active.id),
                                status,
                            )?.let(::addView) ?: addView(label(status, 10f, muted = true))
                        }
                    }

                    TruthRawSuiteLauncherActivity.OUTPUT_PRO -> {
                        addView(actionButton("JPG · full resolution professional") {
                            launchJpegExport(active)
                        })
                        jpegStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey("jpeg", active.id),
                                status,
                            )?.let(::addView) ?: addView(label(status, 10f, muted = true))
                        }
                        addView(space(5))
                        addView(actionButton("Float32 Full Colour Scientific Master · DNG · Lightroom") {
                            launchFullColourScientificMasterExport(active)
                        })
                        fullColourMasterStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey("full-colour-scientific-master", active.id),
                                status,
                            )?.let(::addView) ?: addView(label(status, 10f, muted = true))
                        }
                        addView(space(5))
                        addView(actionButton(
                            "TruthNegative 200MP · Float32 Full Colour · DNG",
                            enabled = active.source.verifiedCamera5TruthNegative200MpEnvelope,
                        ) {
                            launchTruthNegative200MpFullColourExport(active)
                        })
                        truthNegative200MpStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey(
                                    "truthnegative-200mp-full-colour",
                                    active.id,
                                ),
                                status,
                            )?.let(::addView) ?: addView(label(status, 10f, muted = true))
                        }
                        addView(label(
                            if (active.source.verifiedCamera5TruthNegative200MpEnvelope) {
                                "Camera-5 lineage VERIFIED · target 16320×12288 · reconstructed full-colour Float32 · " +
                                    "geen 200MP measured-detail claim."
                            } else {
                                "200MP-projectie fail-closed: Camera-5 envelope/admission-lineage ontbreekt of is niet geverifieerd."
                            },
                            10f,
                            muted = true,
                        ))
                        addView(space(5))
                        addView(actionButton("Render/Edit · Float32 DNG · Lightroom") {
                            launchAdvancedRenderEditExport(active)
                        })
                        renderEditStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey("advanced-render-edit", active.id),
                                status,
                            )?.let(::addView) ?: addView(label(status, 10f, muted = true))
                        }
                        addView(space(5))
                        addView(actionButton("PURE · 32-bit Float DNG") {
                            launchPureFloatDngExport(active)
                        })
                        pureFloatDngStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey("pure-float32", active.id),
                                status,
                            )?.let(::addView) ?: addView(label(status, 10f, muted = true))
                        }
                        addView(space(5))
                        addView(actionButton(
                            "PRO · TruthNegative Continuous v0.5",
                            enabled =
                                active.source.format.nativeProcessingReady &&
                                    active.source.format.id == "DNG",
                        ) {
                            launchTruthNegativeContinuousPreview(active)
                        })
                        truthNegativeContinuousStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey(
                                    "truthnegative-continuous-preview",
                                    active.id,
                                ),
                                status,
                            )?.let(::addView) ?: addView(
                                label(status, 10f, muted = true),
                            )
                        }
                        addView(label(
                            "Raster-onafhankelijke Scientific Negative: bron + Scientific Master + " +
                                "Open Scene authority → continue area-resolve → neutrale v0.7 display-view. " +
                                "PURE en bestaande exports blijven ongewijzigd.",
                            10f,
                            muted = true,
                        ))

                        addView(space(5))
                        addView(actionButton(
                            "PRO · Camera-5 Color/Highlight Oracle",
                            enabled =
                                active.source.format.nativeProcessingReady &&
                                    active.source.format.id == "DNG" &&
                                    active.source.verifiedCamera5TruthNegative200MpEnvelope,
                        ) {
                            launchCamera5ColorHighlightOracle(active)
                        })
                        camera5ColorHighlightStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey(
                                    "camera5-color-highlight-oracle",
                                    active.id,
                                ),
                                status,
                            )?.let(::addView) ?: addView(
                                label(status, 10f, muted = true),
                            )
                        }
                        addView(label(
                            if (active.source.verifiedCamera5TruthNegative200MpEnvelope) {
                                "Physical Camera-5 lineage is sealed. De oracle zoekt de eerste " +
                                    "verdedigbare afwijkingslaag; remosaic blijft UNKNOWN zolang die status " +
                                    "niet apart door runtime evidence is verzegeld."
                            } else {
                                "Camera-5 Oracle is fail-closed: exact physical-5 acquisition/envelope " +
                                    "bewijs ontbreekt voor deze bron."
                            },
                            10f,
                            muted = true,
                        ))
                        addView(space(5))
                        addView(actionButton(
                            "Export TruthNegative Native · .tnc",
                            enabled =
                                active.source.format.nativeProcessingReady &&
                                    active.source.format.id == "DNG",
                        ) {
                            launchTruthNegativeNativeContainerExport(active)
                        })
                        truthNegativeNativeContainerStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey(
                                    "truthnegative-native-container",
                                    active.id,
                                ),
                                status,
                            )?.let(::addView) ?: addView(
                                label(status, 10f, muted = true),
                            )
                        }
                        addView(label(
                            "Nieuwe scientific-negative container: Float32 Scientific Master-valueplane + " +
                                "Open Scene Field v0.85 authority. Na schrijven volgt native import, " +
                                "authority-digest en TruthNegative-state round-trip verificatie.",
                            10f,
                            muted = true,
                        ))
                        addView(space(5))
                        addView(actionButton("Scientific Negative · TN-4") {
                            launchTruthNegativeExport(active)
                        })
                        truthNegativeStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey("truthnegative", active.id),
                                status,
                            )?.let(::addView) ?: addView(label(status, 10f, muted = true))
                        }
                        addView(space(5))
                        addView(actionButton("Full-res Restoration · .trr") {
                            launchFullResRestorationExport(active)
                        })
                        fullResRestorationStatus?.let { status ->
                            val snapshot = FullResRestorationJobStore.read(this@MainActivity)
                            if (snapshot != null && snapshot.jobId == active.id) {
                                addView(restorationStatusView(snapshot))
                            } else {
                                addView(label(status, 10f, muted = true))
                            }
                        }

                        val restoration = FullResRestorationJobStore.read(this@MainActivity)
                        if (restoration != null &&
                            restoration.jobId == active.id &&
                            restoration.phase == FullResRestorationJobPhase.SUCCESS
                        ) {
                            addView(space(6))
                            addView(label("Restoration projecties", 12f, bold = true))
                            val activeProjection = RestorationProjectionJobStore.read(this@MainActivity)
                            val projectionBusy = activeProjection != null && !activeProjection.phase.terminal
                            addView(actionButton("Float32 DNG", enabled = !projectionBusy) {
                                launchRestorationProjection(active, RestorationProjectionFormat.DNG)
                            })
                            addView(actionButton("Float32 TIFF", enabled = !projectionBusy) {
                                launchRestorationProjection(active, RestorationProjectionFormat.TIFF)
                            })
                            addView(actionButton("OpenEXR", enabled = !projectionBusy) {
                                launchRestorationProjection(active, RestorationProjectionFormat.EXR)
                            })
                            if (activeProjection != null) {
                                if (projectionBusy) {
                                    addView(ProgressBar(this@MainActivity).apply {
                                        isIndeterminate = true
                                    }, LinearLayout.LayoutParams(dp(30), dp(30)))
                                }
                                addView(restorationProjectionStatusView(activeProjection))
                            } else {
                                projectionStatus?.let {
                                    addView(label(it, 10f, muted = true))
                                }
                            }
                        }

                        addView(space(5))
                        addView(actionButton("16-bit Linear DNG · compatibility") {
                            launchLinearDngExport(active)
                        })
                        linearDngStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey("linear-dng", active.id),
                                status,
                            )?.let(::addView) ?: addView(label(status, 10f, muted = true))
                        }
                        addView(space(5))
                        addView(actionButton("Pro-instellingen") {
                            startActivity(Intent(this@MainActivity, TruthRawProActivity::class.java))
                        })
                    }
                }
            }
        }

        if (preferredRoute() == TruthRawSuiteLauncherActivity.OUTPUT_PRO) {
            empiricalAudit?.let { audit ->
            addView(space(8))
            addView(label("RAW ingress empirical v0.1", 13f, bold = true))
            val probe = audit.preProbe
            val shaShort = probe.sourceSha256?.let { if (it.length > 16) "${it.take(16)}…" else it } ?: "onbekend"
            addView(label(
                "source SHA=$shaShort · stabiel=${audit.sourceStableAcrossHarness} · DNG-kleur=${probe.metadataForm} · probe status=${probe.statusCode}",
                10f,
                muted = true,
            ))
            addView(label(
                "pipeline=${"%.1f".format(audit.runtime.pipelineWallMs)} ms · worker CPU=${"%.1f".format(audit.runtime.workerCpuMs)} ms · PSS piek=${formatBytes(audit.runtime.pssPeakKb.toLong() * 1024L)}",
                10f,
                muted = true,
            ))
            val pacing = audit.framePacing
            addView(label(
                "thermal ${thermalLabel(audit.runtime.thermalStart)}→${thermalLabel(audit.runtime.thermalEnd)} (piek ${thermalLabel(audit.runtime.thermalPeak)}) · UI p95=${pacing?.p95Ms?.let { "%.1f ms".format(it) } ?: "n/a"}",
                10f,
                muted = true,
            ))
            addView(label(
                "Meetlaag only: deze waarden sturen geen reconstructie, kleurmatrix, TruthRange, zero-line of scientific authority.",
                10f,
                muted = true,
            ))
            addView(space(5))
            addView(actionButton("Empirical JSON opslaan") { launchEmpiricalExport(active) })
            empiricalStatus?.let { addView(label(it, 10f, muted = true)) }
        }
        }
    }

    private fun preferredRoute(): String {
        val value = getSharedPreferences(
            TruthRawSuiteLauncherActivity.PREFS,
            MODE_PRIVATE,
        ).getString(
            TruthRawSuiteLauncherActivity.KEY_OUTPUT,
            TruthRawSuiteLauncherActivity.OUTPUT_PURE,
        ) ?: TruthRawSuiteLauncherActivity.OUTPUT_PURE
        return when (value) {
            TruthRawSuiteLauncherActivity.OUTPUT_ADVANCED,
            TruthRawSuiteLauncherActivity.OUTPUT_PRO -> value
            else -> TruthRawSuiteLauncherActivity.OUTPUT_PURE
        }
    }

    private fun photoFlagsForRoute(route: String): Int = when (route) {
        TruthRawSuiteLauncherActivity.OUTPUT_ADVANCED,
        TruthRawSuiteLauncherActivity.OUTPUT_PRO -> TruthRawAdvancedSettings.load(this).flags()
        else -> 0
    }

    private fun routePane(): View = card().apply {
        val route = preferredRoute()
        addView(label("Actieve route", 16f, bold = true))
        addView(space(6))
        addView(label(
            when (route) {
                TruthRawSuiteLauncherActivity.OUTPUT_ADVANCED -> "D.RAW ADVANCED · vrije fotografische ontwikkeling"
                TruthRawSuiteLauncherActivity.OUTPUT_PRO -> "D.RAW PRO · professionele werkbank"
                else -> "D.RAW PURE · directe wetenschappelijke route"
            },
            13f,
            bold = true,
        ))
        addView(space(4))
        addView(label(
            "Terug brengt je naar de routekeuze. Deze pagina opent nooit opnieuw vanzelf de RAW-kiezer.",
            11f,
            muted = true,
        ))
    }

    private fun toolsPane(): View = vertical().apply {
        addView(routePane())
        addView(space(8))
        addView(card().apply {
            addView(label("Kamers", 16f, bold = true))
            addView(label("Natural", 13f))
            addView(label("Detail", 13f))
            addView(label("Illumination", 13f))
            addView(label("Export", 13f))
            addView(space(8))
            addView(label("UI-keuzes veranderen geen scientific authority, zero-line of scene-ISO-contract.", 11f, muted = true))
        })
    }

    private fun jobListPane(): View = card().apply {
        addView(label("Ingang", 16f, bold = true))
        addView(label("${session.selectedCount} onafhankelijke bronhandle(s)", 12f, muted = true))
        addView(space(6))
        val scroll = ScrollView(this@MainActivity)
        scroll.addView(vertical().apply {
            if (session.jobs.isEmpty()) {
                addView(label("Nog geen RAW geselecteerd.", 13f, muted = true))
            } else {
                session.jobs.forEachIndexed { index, job -> addView(jobRow(index, job)) }
            }
        })
        addView(scroll, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))
    }

    private fun jobStrip(): View = ScrollView(this).apply {
        addView(vertical().apply {
            session.jobs.forEachIndexed { index, job -> addView(jobRow(index, job)) }
        })
    }

    private fun jobRow(index: Int, job: RawJob): View = vertical().apply {
        setPadding(dp(10), dp(8), dp(10), dp(8))
        background = rounded(palette.surfaceAlt, 12f)
        addView(label("${if (job.id == activeJobId) "▶ " else ""}${index + 1}. ${job.source.displayName}", 13f, bold = true))
        val size = job.source.declaredSizeBytes?.let { " · ${formatBytes(it)}" } ?: ""
        addView(label("${job.state.name.lowercase()}$size · ${job.source.format.vendorLabel} · ${job.source.format.displayLabel}", 11f, muted = true))
        addView(label(
            "decoder=${job.source.format.decoderBackend.name.lowercase()} · lineage: afzonderlijk totdat expliciete fusion-validatie bestaat",
            10f,
            muted = true,
        ))
        setOnClickListener { selectJob(job) }
    }.also {
        it.layoutParams = LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            bottomMargin = dp(6)
        }
    }

    private fun routeButton(text: String, route: InputRoute): View = actionButton(
        if (session.route == route) "✓ $text" else text,
    ) {
        session = session.copy(route = route)
        render()
    }.also {
        it.layoutParams = LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            bottomMargin = dp(5)
        }
    }

    private fun card(): LinearLayout = vertical().apply {
        setPadding(dp(16), dp(16), dp(16), dp(16))
        background = rounded(palette.surface, 18f)
    }

    private fun vertical(): LinearLayout = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
    private fun horizontal(): LinearLayout = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }

    private fun label(text: String, sizeSp: Float, bold: Boolean = false, muted: Boolean = false): TextView =
        TextView(this).apply {
            this.text = text
            textSize = sizeSp
            setTextColor(if (muted) palette.textMuted else palette.text)
            if (bold) setTypeface(typeface, Typeface.BOLD)
        }

    private fun actionButton(
        text: String,
        enabled: Boolean = true,
        action: () -> Unit,
    ): Button = Button(this).apply {
        this.text = text
        isAllCaps = false
        setTextColor(palette.text)
        background = rounded(palette.surfaceAlt, 14f)
        isEnabled = enabled
        alpha = if (enabled) 1f else 0.55f
        setOnClickListener { if (enabled) action() }
    }

    private fun rounded(color: Int, radiusDp: Float): GradientDrawable = GradientDrawable().apply {
        setColor(color)
        cornerRadius = dp(radiusDp).toFloat()
    }

    private fun space(heightDp: Int): View = Space(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(heightDp))
    }

    private fun currentLayoutTier(): LayoutTier {
        val widthPx = windowManager.currentWindowMetrics.bounds.width()
        val widthDp = widthPx / resources.displayMetrics.density
        return when {
            widthDp >= 840f -> LayoutTier.EXPANDED
            widthDp >= 600f -> LayoutTier.MEDIUM
            else -> LayoutTier.COMPACT
        }
    }

    private fun cfaLabel(code: Int): String = when (code) {
        0 -> "BGGR"
        1 -> "RGGB"
        2 -> "GRBG"
        3 -> "GBRG"
        else -> "UNKNOWN($code)"
    }

    private fun thermalLabel(status: Int): String = when (status) {
        0 -> "NONE"
        1 -> "LIGHT"
        2 -> "MODERATE"
        3 -> "SEVERE"
        4 -> "CRITICAL"
        5 -> "EMERGENCY"
        6 -> "SHUTDOWN"
        else -> "UNKNOWN($status)"
    }

    private fun formatBytes(bytes: Long): String = when {
        bytes >= 1024L * 1024L * 1024L -> "%.1f GB".format(bytes / (1024.0 * 1024.0 * 1024.0))
        bytes >= 1024L * 1024L -> "%.1f MB".format(bytes / (1024.0 * 1024.0))
        bytes >= 1024L -> "%.1f KB".format(bytes / 1024.0)
        else -> "$bytes B"
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density + 0.5f).toInt()
    private fun dp(value: Float): Int = (value * resources.displayMetrics.density + 0.5f).toInt()

    companion object {
        private const val PROJECTION_PICKER_PREFS = "truthraw_projection_picker_v072"
        private const val KEY_PENDING_PROJECTION_FORMAT = "pending_projection_format"
        const val EXTRA_AUTO_OPEN_RAW_PICKER = "truthraw.extra.AUTO_OPEN_RAW_PICKER"
        const val EXTRA_INTERNAL_CAMERA_SOURCE_PATH = "truthraw.extra.INTERNAL_CAMERA_SOURCE_PATH"
        const val EXTRA_INTERNAL_CAMERA_EVIDENCE_PATH = "truthraw.extra.INTERNAL_CAMERA_EVIDENCE_PATH"
        const val EXTRA_INTERNAL_CAMERA_UPSTREAM_SHA256 = "truthraw.extra.INTERNAL_CAMERA_UPSTREAM_SHA256"
        const val EXTRA_AUTO_START_TRUTHRAW = "truthraw.extra.AUTO_START_TRUTHRAW"

        private const val REQUEST_OPEN_RAW = 4101
        private const val REQUEST_SAVE_JPEG = 4102
        private const val REQUEST_SAVE_EMPIRICAL_JSON = 4103
        private const val REQUEST_SAVE_LINEAR_DNG = 4104
        private const val REQUEST_SAVE_NEF_MEASUREMENT_JSON = 4105
        private const val REQUEST_SAVE_PURE_FLOAT_DNG = 4106
        private const val REQUEST_SAVE_TRUTHNEGATIVE = 4107
        private const val REQUEST_SAVE_FULLRES_RESTORATION = 4108
        private const val REQUEST_SAVE_RESTORATION_PROJECTION = 4109
        private const val REQUEST_SAVE_FULL_COLOUR_MASTER = 4110
        private const val REQUEST_SAVE_ADVANCED_RENDER_EDIT = 4111
        private const val REQUEST_SAVE_TRUTHNEGATIVE_200MP_FULL_COLOUR = 4112
        private const val REQUEST_SAVE_TRUTHNEGATIVE_NATIVE_CONTAINER = 4113
    }
}