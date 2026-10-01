package com.truthraw.adaptiveui

import android.app.Activity
import android.content.ClipData
import android.content.Intent
import android.content.res.Configuration
import android.graphics.Bitmap
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
import org.json.JSONArray
import org.json.JSONObject

class MainActivity : Activity() {
    private var session = BatchSession()
    private var activeJobId: String? = null
    private var previewState: TilePreviewUiState = TilePreviewUiState.Idle
    private var unifiedOutputPreviewState: UnifiedOutputPreviewResult.Ready? = null
    private var n2AppearanceCandidateBitmap: Bitmap? = null
    private var n2AppearanceCandidateJobId: String? = null
    private var n2AppearanceCandidateMetrics: TruthNegativeContinuousPreviewMetrics? = null
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
    private var pendingN2SpatialSidecarJobId: String? = null
    private var n2SpatialSidecarStatus: String? = null
    private var pendingN2CenterExcludedSpatialJobId: String? = null
    private var n2CenterExcludedSpatialStatus: String? = null
    private var pendingN2ConfidenceFieldJobId: String? = null
    private var n2ConfidenceFieldStatus: String? = null
    private var pendingN2FactoredConfidenceJobId: String? = null
    private var n2FactoredConfidenceStatus: String? = null
    private var pendingN2SupportDistanceJobId: String? = null
    private var n2SupportDistanceStatus: String? = null
    private var pendingAnchorReconstructionJobId: String? = null
    private var anchorReconstructionStatus: String? = null
    private var pendingObservationModelSelectionJobId: String? = null
    private var pendingObservationModelSelectionJson: String? = null
    private var observationModelSelectionStatus: String? = null
    private var pendingUniversalModelBankHoldoutJobId: String? = null
    private var universalModelBankHoldoutStatus: String? = null
    private var pendingUniversalModelBankHoldoutV02JobId: String? = null
    private var universalModelBankHoldoutV02Status: String? = null
    private var pendingUniversalModelBankHoldoutV03JobId: String? = null
    private var universalModelBankHoldoutV03Status: String? = null
    private var pendingObservationOpticalFieldJobId: String? = null
    private var pendingObservationOpticalFieldJson: String? = null
    private var observationOpticalFieldStatus: String? = null
    private var pendingUniversalCalibrationAtlasJobId: String? = null
    private var pendingUniversalCalibrationAtlasJson: String? = null
    private var universalCalibrationAtlasStatus: String? = null
    private var pendingFieldResponseRepeatabilityJson: String? = null
    private var fieldResponseRepeatabilityStatus: String? = null
    private val fieldResponseBatchPendingJobIds = linkedSetOf<String>()
    private val fieldResponseBatchFailedJobIds = linkedSetOf<String>()
    private var pendingObservationWorldFieldSeparationJson: String? = null
    private var observationWorldFieldSeparationStatus: String? = null
    private var pendingFreeWorldFoundationJson: String? = null
    private var freeWorldFoundationStatus: String? = null
    // UI-only navigation state. Scientific/session authority is intentionally
    // not derived from or persisted through these scroll positions.
    private var compactScrollY: Int = 0
    private var mediumLeftScrollY: Int = 0
    private var mediumRightScrollY: Int = 0
    private var renderedLayoutTier: LayoutTier? = null
    private val calibrationObservationRecords = mutableListOf<JSONObject>()
    private var calibrationObservationRecordStatus: String? = null
    private var calibrationObservationSessionStoreId: String =
        java.util.UUID.randomUUID().toString()
    private var researchWorkbenchMode: Boolean = false
    private var pendingAppearanceHighlightDetailJobId: String? = null
    private var appearanceHighlightDetailStatus: String? = null
    private var pendingAppearanceHeadroomSweepJobId: String? = null
    private var appearanceHeadroomSweepStatus: String? = null
    private var n2CropAbResult: TruthNegativeN2CropAbResult.Ready? = null
    private var n2CropAbJobId: String? = null
    private var n2CropAbStatus: String? = null
    private var pendingFullResRestorationJobId: String? = null
    private var fullResRestorationStatus: String? = null
    private var pendingProjectionFormat: RestorationProjectionFormat? = null
    private var projectionStatus: String? = null
    private val researchStatusHandler = Handler(Looper.getMainLooper())
    private val researchStatusPoll = object : Runnable {
        override fun run() {
            val keepPolling =
                syncResearchBatchStatus()
            if (keepPolling) {
                researchStatusHandler.postDelayed(
                    this,
                    1000L,
                )
            }
        }
    }

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

    // Universal Intake knowledge lives beside, never inside, source evidence.
    // The back side (container/source facts) and front side (visible structure)
    // are inspected from the same immutable source handle.
    private val universalProfiles = mutableMapOf<String, JSONObject>()
    private val universalProfileErrors = mutableMapOf<String, String>()
    private val universalProfileLoading = mutableSetOf<String>()
    private val universalProfileCompletionWaiters =
        mutableMapOf<String, MutableList<(Boolean) -> Unit>>()
    private var researchWorkbenchSessionRestoreStatus: String? = null
    private var researchBatchLastHeartbeatWallMs: Long = 0L

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

    private fun clearN2AppearanceCandidate() {
        n2AppearanceCandidateBitmap?.recycle()
        n2AppearanceCandidateBitmap = null
        n2AppearanceCandidateJobId = null
        n2AppearanceCandidateMetrics = null
    }

    private fun clearN2CropAb() {
        TruthNegativeN2CropAbLoader.recycle(n2CropAbResult)
        n2CropAbResult = null
        n2CropAbJobId = null
        n2CropAbStatus = null
    }

    private fun calibrationObservationRecordStoreDir(): File =
        if (researchWorkbenchMode) {
            filesDir
        } else {
            cacheDir
        }

    private fun persistResearchCalibrationSessionPointer() {
        if (!researchWorkbenchMode) {
            return
        }
        ResearchCalibrationSessionPointerV01.save(
            filesDir = filesDir,
            sessionId =
                calibrationObservationSessionStoreId,
        )
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        DrawVisualTheme.applyWindow(this)
        researchWorkbenchMode =
            savedInstanceState?.getBoolean(
                STATE_RESEARCH_WORKBENCH_MODE,
            ) ?: intent.getBooleanExtra(
                EXTRA_OPEN_RESEARCH_WORKBENCH,
                false,
            )

        if (
            researchWorkbenchMode &&
            savedInstanceState == null
        ) {
            val persistentPointer =
                ResearchCalibrationSessionPointerV01.load(
                    filesDir,
                )
            val recoveredFromFiles =
                persistentPointer
                    ?: CalibrationObservationRecordSessionStoreV01
                        .latestValidSessionId(
                            filesDir,
                        )
            val recoveredFromLegacyCache =
                if (recoveredFromFiles == null) {
                    CalibrationObservationRecordSessionStoreV01
                        .latestValidSessionId(
                            cacheDir,
                        )
                } else {
                    null
                }
            val recoveredSessionId =
                recoveredFromFiles
                    ?: recoveredFromLegacyCache

            if (recoveredSessionId != null) {
                calibrationObservationSessionStoreId =
                    recoveredSessionId

                if (
                    recoveredFromLegacyCache !=
                    null
                ) {
                    val legacyRecords =
                        CalibrationObservationRecordSessionStoreV01.load(
                            cacheDir = cacheDir,
                            sessionId =
                                recoveredSessionId,
                        )
                    if (
                        legacyRecords.isNotEmpty()
                    ) {
                        CalibrationObservationRecordSessionStoreV01.save(
                            cacheDir = filesDir,
                            sessionId =
                                recoveredSessionId,
                            records =
                                legacyRecords,
                        )
                    }
                }

                persistResearchCalibrationSessionPointer()
            }
        }

        if (savedInstanceState != null) {
            calibrationObservationRecordStatus =
                savedInstanceState.getString(
                    STATE_CALIBRATION_RECORD_STATUS,
                )
            calibrationObservationSessionStoreId =
                savedInstanceState.getString(
                    STATE_CALIBRATION_SESSION_STORE_ID,
                ) ?: calibrationObservationSessionStoreId
            calibrationObservationRecords.clear()
            calibrationObservationRecords +=
                CalibrationObservationRecordSessionStoreV01.load(
                    cacheDir =
                        calibrationObservationRecordStoreDir(),
                    sessionId =
                        calibrationObservationSessionStoreId,
                )
        } else if (researchWorkbenchMode) {
            calibrationObservationRecords.clear()
            calibrationObservationRecords +=
                CalibrationObservationRecordSessionStoreV01.load(
                    cacheDir =
                        calibrationObservationRecordStoreDir(),
                    sessionId =
                        calibrationObservationSessionStoreId,
                )
        }

        if (researchWorkbenchMode) {
            persistResearchCalibrationSessionPointer()
        }

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

        restoreResearchWorkbenchSessionIfNeeded()

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

        if (
            savedInstanceState == null &&
            intent.getBooleanExtra(EXTRA_AUTO_OPEN_RAW_PICKER, false)
        ) {
            window.decorView.post { launchRawPicker() }
        }

        if (
            savedInstanceState == null &&
            intent.getBooleanExtra(
                EXTRA_AUTO_OPEN_CALIBRATION_RECORD_PICKER,
                false,
            )
        ) {
            window.decorView.post {
                launchCalibrationObservationRecordPicker()
            }
        }
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        setIntent(intent)
        researchWorkbenchMode =
            intent.getBooleanExtra(
                EXTRA_OPEN_RESEARCH_WORKBENCH,
                false,
            )

        if (
            researchWorkbenchMode &&
            calibrationObservationRecords.isEmpty()
        ) {
            ResearchCalibrationSessionPointerV01.load(
                filesDir,
            )?.let {
                calibrationObservationSessionStoreId =
                    it
            }
            calibrationObservationRecords +=
                CalibrationObservationRecordSessionStoreV01.load(
                    cacheDir =
                        calibrationObservationRecordStoreDir(),
                    sessionId =
                        calibrationObservationSessionStoreId,
                )
        }

        val cameraJob = readInternalCameraJob(intent)
        if (cameraJob != null) {
            installInternalCameraJob(cameraJob)
            render()
            if (intent.getBooleanExtra(EXTRA_AUTO_START_TRUTHRAW, false)) {
                window.decorView.post {
                    if (
                        activeJobId == cameraJob.id &&
                        previewState is TilePreviewUiState.Idle
                    ) {
                        requestPreview(cameraJob)
                    }
                }
            }
        } else {
            restoreResearchWorkbenchSessionIfNeeded()
            render()
        }

        if (
            intent.getBooleanExtra(EXTRA_AUTO_OPEN_RAW_PICKER, false)
        ) {
            window.decorView.post { launchRawPicker() }
        }

        if (
            intent.getBooleanExtra(
                EXTRA_AUTO_OPEN_CALIBRATION_RECORD_PICKER,
                false,
            )
        ) {
            window.decorView.post {
                launchCalibrationObservationRecordPicker()
            }
        }
    }

    private fun restoreResearchWorkbenchSessionIfNeeded() {
        if (!researchWorkbenchMode || session.jobs.isNotEmpty()) {
            return
        }
        val restored =
            ResearchWorkbenchSessionStoreV01.load(
                filesDir,
            ) ?: return
        if (restored.jobs.isEmpty()) {
            return
        }
        session = restored
        restoreResearchUniversalProfilesForCurrentSession()
        activeJobId =
            restored.jobs.firstOrNull()?.id
        previewState = TilePreviewUiState.Idle
        loadingStartedAtElapsedMs = null
        researchWorkbenchSessionRestoreStatus =
            "Onderzoeksselectie hersteld · " +
                restored.jobs.size +
                " RAW-bron(nen)."
    }

    private fun persistResearchWorkbenchSession() {
        if (!researchWorkbenchMode) {
            return
        }
        if (session.jobs.isEmpty()) {
            ResearchWorkbenchSessionStoreV01.clear(
                filesDir,
            )
        } else {
            ResearchWorkbenchSessionStoreV01.save(
                cacheDir = filesDir,
                session = session,
            )
        }
    }

    private fun restoreResearchUniversalProfilesForCurrentSession() {
        if (session.jobs.isEmpty()) {
            return
        }
        // Completed per-RAW profiles are immutable derived diagnostics and may
        // be loaded while the service continues with later RAWs. Keeping this
        // live prevents the UI from reporting 0/3 after one or more profiles
        // have already been durably committed.
        val restoredProfiles =
            ResearchUniversalProfileStoreV01.load(
                filesDir = filesDir,
                session = session,
            )
        if (restoredProfiles.isEmpty()) {
            return
        }
        universalProfiles.putAll(
            restoredProfiles,
        )
        for (jobId in restoredProfiles.keys) {
            universalProfileErrors.remove(
                jobId,
            )
            universalProfileLoading.remove(
                jobId,
            )
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
        unifiedOutputPreviewState?.bitmap?.recycle()
        unifiedOutputPreviewState = null
        clearN2AppearanceCandidate()
        clearN2CropAb()
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
        pendingN2SpatialSidecarJobId = null
        n2SpatialSidecarStatus = null
        pendingN2CenterExcludedSpatialJobId = null
        n2CenterExcludedSpatialStatus = null
        pendingN2ConfidenceFieldJobId = null
        n2ConfidenceFieldStatus = null
        pendingN2FactoredConfidenceJobId = null
        n2FactoredConfidenceStatus = null
        pendingN2SupportDistanceJobId = null
        n2SupportDistanceStatus = null
        pendingAnchorReconstructionJobId = null
        anchorReconstructionStatus = null
        pendingAppearanceHighlightDetailJobId = null
        appearanceHighlightDetailStatus = null
        pendingAppearanceHeadroomSweepJobId = null
        appearanceHeadroomSweepStatus = null
        fullResRestorationStatus = null
        projectionStatus = null
        requestUniversalProfile(cameraJob)
    }

    override fun onResume() {
        super.onResume()
        restoreResearchWorkbenchSessionIfNeeded()
        restoreResearchUniversalProfilesForCurrentSession()
        if (researchWorkbenchMode && session.jobs.isNotEmpty()) {
            TruthRawOperationStore.read(
                this,
                fieldResponseRepeatabilityAnalysisOperationKey(),
            )?.let { operation ->
                fieldResponseRepeatabilityStatus =
                    operation.message
                if (operation.terminal) {
                    restoreResearchUniversalProfilesForCurrentSession()
                    fieldResponseBatchPendingJobIds.clear()
                }
            }
        }
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
        researchStatusHandler.removeCallbacks(
            researchStatusPoll,
        )
        if (syncResearchBatchStatus()) {
            researchStatusHandler.postDelayed(
                researchStatusPoll,
                1000L,
            )
        }
        render()
    }

    override fun onPause() {
        researchStatusHandler.removeCallbacks(
            researchStatusPoll,
        )
        restorationStatusHandler.removeCallbacks(restorationStatusPoll)
        super.onPause()
    }

    override fun onDestroy() {
        researchStatusHandler.removeCallbacks(
            researchStatusPoll,
        )
        restorationStatusHandler.removeCallbacks(restorationStatusPoll)
        (previewState as? TilePreviewUiState.Ready)?.bitmap?.recycle()
        unifiedOutputPreviewState?.bitmap?.recycle()
        unifiedOutputPreviewState = null
        clearN2AppearanceCandidate()
        clearN2CropAb()
        (nefMeasurementResult as? NefMeasurementResult.Ready)?.bitmap?.recycle()
        if (
            isFinishing &&
            !researchWorkbenchMode
        ) {
            CalibrationObservationRecordSessionStoreV01.clear(
                cacheDir =
                    calibrationObservationRecordStoreDir(),
                sessionId =
                    calibrationObservationSessionStoreId,
            )
        }
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
        recover("truthnegative-n2-spatial-sidecar")?.let {
            n2SpatialSidecarStatus = it.message
        }
        recover("truthnegative-n2-crop-ab")?.let {
            n2CropAbStatus = it.message
        }
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

    private fun syncResearchBatchStatus(): Boolean {
        if (
            !researchWorkbenchMode ||
            session.jobs.isEmpty()
        ) {
            return false
        }

        val key =
            fieldResponseRepeatabilityAnalysisOperationKey()
        val operation =
            TruthRawOperationStore.read(
                this,
                key,
            ) ?: return false

        var changed = false
        if (
            fieldResponseRepeatabilityStatus !=
            operation.message
        ) {
            fieldResponseRepeatabilityStatus =
                operation.message
            changed = true
        }

        val profilesBefore =
            universalProfiles.size
        restoreResearchUniversalProfilesForCurrentSession()
        if (universalProfiles.size != profilesBefore) {
            changed = true
        }

        val journal =
            ResearchBatchJournalV02.read(
                this,
                key,
            )
        val completed =
            linkedSetOf<String>().apply {
                addAll(universalProfiles.keys)
                addAll(
                    ResearchBatchJournalV02.completedJobIds(
                        this@MainActivity,
                        key,
                    ),
                )
            }
        val failed =
            ResearchBatchJournalV02.failedJobIds(
                this,
                key,
            )
        val pending =
            session.jobs
                .map { it.id }
                .filterNot {
                    it in completed ||
                        it in failed
                }
                .toSet()

        if (
            fieldResponseBatchPendingJobIds.toSet() !=
            pending
        ) {
            fieldResponseBatchPendingJobIds.clear()
            fieldResponseBatchPendingJobIds.addAll(
                pending,
            )
            changed = true
        }
        if (
            fieldResponseBatchFailedJobIds.toSet() !=
            failed
        ) {
            fieldResponseBatchFailedJobIds.clear()
            fieldResponseBatchFailedJobIds.addAll(
                failed,
            )
            changed = true
        }

        val heartbeat =
            journal?.optLong(
                "service_heartbeat_wall_ms",
                0L,
            ) ?: 0L
        if (
            heartbeat > 0L &&
            heartbeat !=
            researchBatchLastHeartbeatWallMs
        ) {
            researchBatchLastHeartbeatWallMs =
                heartbeat
            changed = true
        }

        if (operation.terminal) {
            restoreResearchUniversalProfilesForCurrentSession()
            if (
                fieldResponseBatchPendingJobIds.isNotEmpty()
            ) {
                fieldResponseBatchPendingJobIds.clear()
                changed = true
            }
        }

        if (changed) {
            render()
        }

        return !operation.terminal
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

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        outState.putBoolean(
            STATE_RESEARCH_WORKBENCH_MODE,
            researchWorkbenchMode,
        )
        outState.putString(
            STATE_CALIBRATION_RECORD_STATUS,
            calibrationObservationRecordStatus,
        )
        CalibrationObservationRecordSessionStoreV01.save(
            cacheDir =
                calibrationObservationRecordStoreDir(),
            sessionId =
                calibrationObservationSessionStoreId,
            records =
                calibrationObservationRecords,
        )
        persistResearchCalibrationSessionPointer()
        outState.putString(
            STATE_CALIBRATION_SESSION_STORE_ID,
            calibrationObservationSessionStoreId,
        )
        persistResearchWorkbenchSession()
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
    private fun launchCalibrationObservationRecordPicker() {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true)
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_OPEN_CALIBRATION_OBSERVATION_RECORDS,
        )
    }

    private fun currentCalibrationObservationRecords(): List<JSONObject> =
        calibrationObservationRecords.map { JSONObject(it.toString()) }

    private fun importCalibrationObservationRecords(data: Intent): String {
        val uris = buildList {
            data.data?.let(::add)
            val clip: ClipData? = data.clipData
            if (clip != null) {
                for (index in 0 until clip.itemCount) {
                    add(clip.getItemAt(index).uri)
                }
            }
        }.distinct()

        if (uris.isEmpty()) {
            return "Calibration Observation Records: geen JSON-bron ontvangen."
        }

        var validImported = 0
        var invalidRejected = 0
        var filesFailed = 0

        for (uri in uris) {
            val text =
                runCatching {
                    contentResolver.openInputStream(uri)
                        ?.bufferedReader(Charsets.UTF_8)
                        ?.use { it.readText() }
                }.getOrNull()
            if (text.isNullOrBlank()) {
                filesFailed++
                continue
            }

            val parsed =
                CalibrationObservationRecordBundleV01.parse(text)
            invalidRejected +=
                parsed.optInt("invalid_record_count", 0)

            val records =
                parsed.optJSONArray("records") ?: JSONArray()
            for (index in 0 until records.length()) {
                val record = records.optJSONObject(index) ?: continue
                val normalized =
                    CalibrationObservationRecordIdentityV01.normalize(record)
                val identity =
                    normalized.optString("record_identity_sha256")
                val duplicate =
                    calibrationObservationRecords.any {
                        it.optString("record_identity_sha256") == identity
                    }
                if (!duplicate) {
                    calibrationObservationRecords += normalized
                    validImported++
                }
            }
        }

        CalibrationObservationRecordSessionStoreV01.save(
            cacheDir =
                calibrationObservationRecordStoreDir(),
            sessionId =
                calibrationObservationSessionStoreId,
            records =
                calibrationObservationRecords,
        )
        persistResearchCalibrationSessionPointer()

        return "Calibration Observation Records · nieuw=" +
            validImported +
            " · totaal=" +
            calibrationObservationRecords.size +
            " · rejected=" +
            invalidRejected +
            " · file-fail=" +
            filesFailed +
            " · relation-based only · promotion=false."
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
                "D.RAWnegative 200MP geblokkeerd: physical Camera-5 envelope/admission-lineage is niet exact geverifieerd."
            render()
            return
        }
        if (!job.source.format.nativeProcessingReady || job.source.format.id != "DNG") {
            truthNegative200MpStatus =
                "D.RAWnegative 200MP vereist de volledig admitted DNG/Camera-5-route."
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
            "truthnegative-n2-spatial-sidecar",
            "truthnegative-n2-support-distance",
            "anchor-constrained-local-reconstruction",
            "universal-local-model-bank-holdout",
            "truthnegative-n2-crop-ab",
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
                "D.RAWnegative v0.1 vereist de volledig admitted DNG-route."
            render()
            return
        }

        val operationKey =
            backgroundOperationKey("truthnegative-continuous-preview", job.id)
        if (truthNegativeHeavyOperationActive(job.id, operationKey)) {
            truthNegativeContinuousStatus =
                "Wacht op de andere D.RAWnegative/Camera-5 analysetaak. " +
                    "Deze zware Scientific Master-routes draaien bewust niet meer tegelijk."
            render()
            return
        }
        if (!startBackgroundOperation(
                operationKey,
                "D.RAWnegative v0.1 · Scientific Negative → Free-World preview",
            )
        ) {
            truthNegativeContinuousStatus =
                "D.RAWnegative preview kon niet veilig starten."
            render()
            return
        }

        truthNegativeContinuousStatus =
            "D.RAWnegative v0.1 bindt de raster-onafhankelijke scientific-negative state aan Observation + source-local Zero-Line gauge, " +
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
                    is TruthNegativeContinuousPreviewResult.Ready -> {
                        val m = result.metrics
                        "D.RAWnegative v0.1 preview gereed · DN=" +
                            m.drawNegativeStateSha256.take(16) + "… · " +
                            "N2 audit-only sampled=${m.n2Sampled}, " +
                            "candidate=${m.n2CorrectedCandidates}, " +
                            "preserved=${m.n2Preserved}, structure=" +
                            "${m.n2StructureProtected}, censored/boundary=" +
                            "${m.n2CensoredProtected}/${m.n2CensorBoundaryProtected}, " +
                            "noiseProfile=${m.n2NoiseProfileAvailable}, " +
                            "primary-candidate-applied=false · " +
                            "A/B-B=${m.n2AppearanceChangedPixels} changed px."
                    }
                    is TruthNegativeContinuousPreviewResult.Failed ->
                        result.reason
                },
            )

            runOnUiThread {
                if (activeJobId != job.id) {
                    (result as? TruthNegativeContinuousPreviewResult.Ready)
                        ?.let { ready ->
                            ready.bitmap.recycle()
                            ready.n2CandidateBitmap.recycle()
                        }
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
                        clearN2AppearanceCandidate()
                        n2AppearanceCandidateBitmap = result.n2CandidateBitmap
                        n2AppearanceCandidateJobId = job.id
                        n2AppearanceCandidateMetrics = result.metrics
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
                                "${m.independentEvidenceCount} · writeback=false · " +
                                "N2 audit-only: sampled=${m.n2Sampled}, " +
                                "candidate-corrected=${m.n2CorrectedCandidates}, " +
                                "preserved=${m.n2Preserved}, structure-protected=" +
                                "${m.n2StructureProtected}, censored/boundary=" +
                                "${m.n2CensoredProtected}/${m.n2CensorBoundaryProtected}, " +
                                "noiseProfile=${m.n2NoiseProfileAvailable}, removed-energy=" +
                                "%.3f%%".format(m.n2RemovedResidualEnergyFraction * 100.0) +
                                ", max|Δ|stage2=" +
                                "%.8f".format(m.n2MaxAbsCorrectionStage2) +
                                ", audit=${m.n2AuditSha256.take(16)}… · " +
                                "primary-candidate-applied=false · " +
                                "A/B candidate=${m.n2AppearanceChangedPixels} px/" +
                                "${m.n2AppearanceAdjustedChannels} ch, clamp=" +
                                "${m.n2AppearanceDisplayClampPixels} px, grid=" +
                                "${m.n2AppearanceGridSha256.take(16)}… · " +
                                "appearance-only=true."
                    }
                }
                render()
            }
        }
    }


    private fun launchN2CropAbDiagnostic(job: RawJob) {
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            n2CropAbStatus =
                "N2 1:1 cropdiagnose vereist de admitted DNG-route."
            render()
            return
        }

        val operationKey =
            backgroundOperationKey("truthnegative-n2-crop-ab", job.id)
        if (truthNegativeHeavyOperationActive(job.id, operationKey)) {
            n2CropAbStatus =
                "Wacht op de andere D.RAWnegative/N2 analysetaak. " +
                    "De 1:1 cropdiagnose draait bewust niet parallel."
            render()
            return
        }
        if (!startBackgroundOperation(
                operationKey,
                "N2 1:1 A/B/Δ cropdiagnose",
            )
        ) {
            n2CropAbStatus =
                "N2 1:1 cropdiagnose kon niet veilig starten."
            render()
            return
        }

        clearN2CropAb()
        n2CropAbJobId = job.id
        n2CropAbStatus =
            "N2 1:1 cropdiagnose selecteert automatisch rustige/noise-, " +
                "structuur- en censorzones, audit het volledige CFA-raster en " +
                "propagereert alleen toegelaten correcties door dezelfde full-colour reconstructie…"
        render()

        startGuardedBackgroundThread(
            name = "draw-n2-crop-ab-" + job.id.take(8),
            operationKey = operationKey,
            onUnexpected = { message ->
                n2CropAbStatus = message
            },
        ) {
            val result =
                TruthNegativeN2CropAbLoader.load(contentResolver, job)
            finishBackgroundOperation(
                operationKey,
                result is TruthNegativeN2CropAbResult.Ready,
                when (result) {
                    is TruthNegativeN2CropAbResult.Ready ->
                        "N2 1:1 full-colour A/B/Δ cropdiagnose gereed."
                    is TruthNegativeN2CropAbResult.Failed ->
                        result.reason
                },
            )
            runOnUiThread {
                if (activeJobId != job.id) {
                    TruthNegativeN2CropAbLoader.recycle(
                        result as? TruthNegativeN2CropAbResult.Ready,
                    )
                    return@runOnUiThread
                }
                when (result) {
                    is TruthNegativeN2CropAbResult.Failed -> {
                        n2CropAbStatus = result.reason
                    }
                    is TruthNegativeN2CropAbResult.Ready -> {
                        TruthNegativeN2CropAbLoader.recycle(n2CropAbResult)
                        n2CropAbResult = result
                        n2CropAbJobId = job.id
                        val r = result.report
                        n2CropAbStatus =
                            "N2 1:1 full-colour + Risk/Quality gereed · " +
                                r.crops.joinToString(" · ") { panel ->
                                    val m = panel.metrics
                                    m.kind.name + "=" +
                                        m.changedPixels + "/" +
                                        (m.width * m.height) +
                                        " Δp99=" +
                                        "%.6f".format(m.pixelDelta.p99) +
                                        " edge=" +
                                        "%.4f".format(m.edgeEnergyRatio)
                                } +
                                " · Δ×" + r.deltaGain +
                                " · writeback=false."
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
                "Wacht op de andere D.RAWnegative/Camera-5 analysetaak. " +
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
                "D.RAWnegative legacy .tnc-compatibility vereist de admitted DNG-route."
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
                stem + "_drawnegative_legacy_tnc_v0_1.tnc",
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
    private fun launchN2SpatialSidecarExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            n2SpatialSidecarStatus =
                "N2 Spatial Audit vereist de admitted DNG-route."
            render()
            return
        }
        pendingN2SpatialSidecarJobId = job.id
        n2SpatialSidecarStatus = null
        val stem =
            job.source.displayName.substringBeforeLast(
                '.',
                job.source.displayName,
            )
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                stem + "_draw_n2_spatial_audit_v0_1.json",
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_SAVE_N2_SPATIAL_SIDECAR,
        )
    }

    @Suppress("DEPRECATION")
    private fun launchN2CenterExcludedSpatialExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            n2CenterExcludedSpatialStatus =
                "N2 v0.2.1 Spatial Audit vereist de admitted DNG-route."
            render()
            return
        }
        pendingN2CenterExcludedSpatialJobId = job.id
        n2CenterExcludedSpatialStatus = null
        val stem =
            job.source.displayName.substringBeforeLast(
                '.',
                job.source.displayName,
            )
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                stem + "_draw_n2_center_excluded_spatial_audit_v0_2_1.json",
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_SAVE_N2_CENTER_EXCLUDED_SPATIAL,
        )
    }

    @Suppress("DEPRECATION")
    private fun launchN2ConfidenceFieldExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            n2ConfidenceFieldStatus =
                "N2 Confidence Field v0.3 vereist de admitted DNG-route."
            render()
            return
        }
        pendingN2ConfidenceFieldJobId = job.id
        n2ConfidenceFieldStatus = null
        val stem =
            job.source.displayName.substringBeforeLast(
                '.',
                job.source.displayName,
            )
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                stem + "_draw_n2_confidence_field_v0_3.json",
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_SAVE_N2_CONFIDENCE_FIELD,
        )
    }

    @Suppress("DEPRECATION")
    private fun launchN2FactoredConfidenceExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            n2FactoredConfidenceStatus =
                "N2 Factored Confidence v0.3.1 vereist de admitted DNG-route."
            render()
            return
        }
        pendingN2FactoredConfidenceJobId = job.id
        n2FactoredConfidenceStatus = null
        val stem =
            job.source.displayName.substringBeforeLast(
                '.',
                job.source.displayName,
            )
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                stem + "_draw_n2_factored_confidence_state_v0_3_1.json",
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_SAVE_N2_FACTORED_CONFIDENCE,
        )
    }

    @Suppress("DEPRECATION")
    private fun launchN2SupportDistanceExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            n2SupportDistanceStatus =
                "N2 Sample Support Distance v0.1 vereist de admitted DNG-route."
            render()
            return
        }
        val profile = universalProfiles[job.id]
        val support =
            profile?.optJSONObject("n2_sample_support_distance")
        if (
            support?.optString("status") !=
            "AUDIT_ONLY_DISTANCE_BINDING_AVAILABLE"
        ) {
            n2SupportDistanceStatus =
                "N2 Sample Support Distance v0.1 export vereist eerst een succesvolle Universele Ingang-analyse met zichtbare Dark-Chroma-kandidaten."
            render()
            return
        }

        pendingN2SupportDistanceJobId = job.id
        n2SupportDistanceStatus = null
        val stem =
            job.source.displayName.substringBeforeLast(
                '.',
                job.source.displayName,
            )
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                stem + "_draw_n2_sample_support_distance_v0_1.json",
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_SAVE_N2_SUPPORT_DISTANCE,
        )
    }

    @Suppress("DEPRECATION")
    private fun launchAnchorConstrainedReconstructionExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            anchorReconstructionStatus =
                "Anchor-Constrained Local Reconstruction v0.1 vereist de admitted DNG-route."
            render()
            return
        }

        val profile = universalProfiles[job.id]
        val audit =
            profile?.optJSONObject(
                "anchor_constrained_local_reconstruction",
            )
        if (
            audit?.optString("status") !=
            "AUDIT_ONLY_HOLDOUT_VALIDATION_AVAILABLE"
        ) {
            anchorReconstructionStatus =
                "Anchor-Constrained Local Reconstruction v0.1 export vereist eerst een succesvolle Universele Ingang-analyse met zichtbare Dark-Chroma-kandidaten en exacte support-geometrie."
            render()
            return
        }

        pendingAnchorReconstructionJobId = job.id
        anchorReconstructionStatus = null
        val stem =
            job.source.displayName.substringBeforeLast(
                '.',
                job.source.displayName,
            )
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                stem + "_draw_anchor_constrained_local_reconstruction_v0_1.json",
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_SAVE_ANCHOR_RECONSTRUCTION,
        )
    }

    @Suppress("DEPRECATION")
    private fun launchObservationModelSelectionExport(job: RawJob) {
        if (job.id != activeJobId) return
        val policy =
            universalProfiles[job.id]?.optJSONObject(
                "universal_observation_model_selection",
            )
        if (
            policy?.optString("status") !=
            "PROSPECTIVE_AUDIT_POLICY_AVAILABLE"
        ) {
            observationModelSelectionStatus =
                "Universal Observation Model Selection v0.1 export vereist eerst een succesvolle Universele Ingang-analyse met raster-onafhankelijke support-geometrie."
            render()
            return
        }
        if (
            policy.optBoolean("heldout_target_used_for_selection", true) ||
            policy.optBoolean("holdout_error_used_for_selection", true) ||
            policy.optBoolean("lens_calibration_used", true) ||
            policy.optBoolean("camera_model_used", true) ||
            policy.optBoolean("vendor_mapping_used", true) ||
            policy.optBoolean("candidate_applied", true) ||
            policy.optBoolean("scientific_writeback_allowed", true)
        ) {
            observationModelSelectionStatus =
                "Universal Observation Model Selection v0.1 export geblokkeerd: target-blind/universele safety-contract mismatch."
            render()
            return
        }

        pendingObservationModelSelectionJobId = job.id
        pendingObservationModelSelectionJson = policy.toString(2) + "\n"
        observationModelSelectionStatus = null
        val stem =
            job.source.displayName.substringBeforeLast(
                '.',
                job.source.displayName,
            )
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                stem + "_draw_universal_observation_model_selection_v0_1.json",
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_SAVE_OBSERVATION_MODEL_SELECTION,
        )
    }

    @Suppress("DEPRECATION")
    private fun launchUniversalModelBankHoldoutExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id || job.id != activeJobId) return
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            universalModelBankHoldoutStatus =
                "Universal Local Model Bank Holdout v0.1 vereist momenteel de admitted native DNG-route."
            render()
            return
        }

        val audit =
            universalProfiles[job.id]?.optJSONObject(
                "universal_local_model_bank_holdout",
            )
        if (
            audit?.optString("status") !=
            "READY_FOR_EXPLICIT_EXPORT_AUDIT"
        ) {
            universalModelBankHoldoutStatus =
                "Universal Local Model Bank Holdout v0.1 vereist eerst een succesvolle Universele Ingang-analyse met een geldige raster-onafhankelijke sample-lattice; Dark-Chroma/prospective query-policy is hiervoor niet vereist."
            render()
            return
        }

        pendingUniversalModelBankHoldoutJobId = job.id
        universalModelBankHoldoutStatus = null
        val stem =
            job.source.displayName.substringBeforeLast(
                '.',
                job.source.displayName,
            )
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                stem + "_draw_universal_local_model_bank_holdout_v0_1.json",
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_SAVE_UNIVERSAL_MODEL_BANK_HOLDOUT,
        )
    }

    @Suppress("DEPRECATION")
    private fun launchUniversalModelBankHoldoutV02Export(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id || job.id != activeJobId) return
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            universalModelBankHoldoutV02Status =
                "Universal Local Model Bank Holdout v0.2 vereist momenteel de admitted native DNG-route."
            render()
            return
        }

        val audit =
            universalProfiles[job.id]?.optJSONObject(
                "universal_local_model_bank_holdout_v0_2",
            )
        if (
            audit?.optString("status") !=
            "READY_FOR_EXPLICIT_EXPORT_AUDIT"
        ) {
            universalModelBankHoldoutV02Status =
                "Universal Local Model Bank Holdout v0.2 vereist eerst een succesvolle Universele Ingang-analyse met een geldige raster-onafhankelijke sample-lattice; Dark-Chroma/prospective query-policy is hiervoor niet vereist."
            render()
            return
        }

        pendingUniversalModelBankHoldoutV02JobId = job.id
        universalModelBankHoldoutV02Status = null
        val stem =
            job.source.displayName.substringBeforeLast(
                '.',
                job.source.displayName,
            )
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                stem + "_draw_universal_local_model_bank_holdout_v0_2.json",
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_SAVE_UNIVERSAL_MODEL_BANK_HOLDOUT_V02,
        )
    }

    @Suppress("DEPRECATION")
    private fun launchUniversalModelBankHoldoutV03Export(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id || job.id != activeJobId) return
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            universalModelBankHoldoutV03Status =
                "Universal Local Model Bank Holdout v0.3 vereist momenteel de admitted native DNG-route."
            render()
            return
        }

        val audit =
            universalProfiles[job.id]?.optJSONObject(
                "universal_local_model_bank_holdout_v0_3",
            )
        if (
            audit?.optString("status") !=
            "READY_FOR_EXPLICIT_EXPORT_AUDIT"
        ) {
            universalModelBankHoldoutV03Status =
                "Universal Local Model Bank Holdout v0.3 vereist eerst een succesvolle Universele Ingang-analyse met een geldige raster-onafhankelijke sample-lattice; Dark-Chroma/prospective query-policy is hiervoor niet vereist."
            render()
            return
        }

        pendingUniversalModelBankHoldoutV03JobId = job.id
        universalModelBankHoldoutV03Status = null
        val stem =
            job.source.displayName.substringBeforeLast(
                '.',
                job.source.displayName,
            )
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                stem + "_draw_universal_local_model_bank_holdout_v0_3.json",
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_SAVE_UNIVERSAL_MODEL_BANK_HOLDOUT_V03,
        )
    }

    @Suppress("DEPRECATION")
    private fun launchObservationOpticalFieldExport(job: RawJob) {
        if (job.id != activeJobId) return

        val chart =
            universalProfiles[job.id]?.optJSONObject(
                "observation_optical_field_chart",
            )
        if (chart?.optString("status") != "FIELD_CHART_AVAILABLE") {
            observationOpticalFieldStatus =
                "Observation Optical Field Chart v0.1 export vereist eerst een succesvolle Universele Ingang-analyse met geldige bron-/ActiveArea-geometrie."
            render()
            return
        }
        if (
            chart.optJSONObject("vignetting_interpretation")
                ?.optBoolean("correction_gain_allowed", true) != false ||
            chart.optBoolean("source_sample_values_modified", true) ||
            chart.optBoolean("source_sample_positions_modified", true) ||
            chart.optBoolean("new_measured_samples_created", true) ||
            chart.optBoolean("scientific_writeback_allowed", true)
        ) {
            observationOpticalFieldStatus =
                "Observation Optical Field Chart v0.1 export geblokkeerd: read-only optical-field safety-contract mismatch."
            render()
            return
        }

        pendingObservationOpticalFieldJobId = job.id
        pendingObservationOpticalFieldJson =
            chart.toString(2) + "\n"
        observationOpticalFieldStatus = null

        val stem =
            job.source.displayName.substringBeforeLast(
                '.',
                job.source.displayName,
            )
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                stem + "_draw_observation_optical_field_chart_v0_1.json",
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_SAVE_OBSERVATION_OPTICAL_FIELD,
        )
    }

    @Suppress("DEPRECATION")
    private fun launchUniversalCalibrationAtlasExport(job: RawJob) {
        if (job.id != activeJobId) return

        val atlas =
            universalProfiles[job.id]?.optJSONObject(
                "universal_observation_calibration_atlas",
            )
        if (atlas?.optString("status") != "OBSERVATION_ATLAS_AVAILABLE") {
            universalCalibrationAtlasStatus =
                "Universal Observation & Calibration Atlas v0.1 vereist eerst een Universele Ingang-analyse van de sealed observation."
            render()
            return
        }

        val identity = atlas.optJSONObject("universal_identity_policy")
        val colour = atlas.optJSONObject("colour_state")
        val illumination = atlas.optJSONObject("illumination_state")
        val optical = atlas.optJSONObject("optical_support")
        if (
            identity?.optBoolean("camera_identity_required", true) != false ||
            identity?.optBoolean("lens_identity_required", true) != false ||
            identity?.optBoolean("prior_user_calibration_required", true) != false ||
            colour?.optBoolean("automatic_colour_correction_from_atlas_allowed", true) != false ||
            illumination?.optBoolean("automatic_light_falloff_correction_allowed", true) != false ||
            optical?.optBoolean("deconvolution_authorized", true) != false ||
            atlas.optBoolean("source_sample_values_modified", true) ||
            atlas.optBoolean("source_sample_positions_modified", true) ||
            atlas.optBoolean("new_measured_samples_created", true) ||
            atlas.optBoolean("scientific_writeback_allowed", true)
        ) {
            universalCalibrationAtlasStatus =
                "Universal Observation & Calibration Atlas v0.1 export geblokkeerd: universal/read-only safety-contract mismatch."
            render()
            return
        }

        pendingUniversalCalibrationAtlasJobId = job.id
        pendingUniversalCalibrationAtlasJson = atlas.toString(2) + "\n"
        universalCalibrationAtlasStatus = null

        val stem =
            job.source.displayName.substringBeforeLast(
                '.',
                job.source.displayName,
            )
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                stem + "_draw_universal_observation_calibration_atlas_v0_1.json",
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_SAVE_UNIVERSAL_CALIBRATION_ATLAS,
        )
    }

    private fun currentMeasuredFieldCharts(): List<JSONObject> =
        session.jobs.mapNotNull { job ->
            universalProfiles[job.id]
                ?.optJSONObject("observation_optical_field_chart")
                ?.takeIf {
                    it.optString("status") == "FIELD_CHART_AVAILABLE" &&
                        it.optJSONObject("measured_composite_field_signal")
                            ?.optString("status") ==
                        "MEASURED_COMPOSITE_FIELD_SIGNAL_AVAILABLE"
                }
        }

    private fun fieldResponseRepeatabilityAnalysisOperationKey(
        jobs: List<RawJob> = session.jobs,
    ): String {
        val setKey =
            jobs
                .map { it.id }
                .sorted()
                .joinToString("|")
                .hashCode()
                .toString()
        return "main:field-response-repeatability-analysis:" + setKey
    }

    private fun requestUniversalProfilesForSelectedSources() {
        val selected = session.jobs.toList()
        if (selected.isEmpty()) {
            fieldResponseRepeatabilityStatus =
                "Geen geselecteerde bronnen voor Field Response Repeatability v0.1."
            render()
            return
        }

        persistResearchWorkbenchSession()
        fieldResponseBatchPendingJobIds.clear()
        fieldResponseBatchFailedJobIds.clear()
        fieldResponseBatchPendingJobIds.addAll(
            selected.map {
                it.id
            },
        )

        val operationKey =
            fieldResponseRepeatabilityAnalysisOperationKey(
                selected,
            )

        val started =
            TruthRawMediaProcessingForegroundService
                .startResearchBatch(
                    context = applicationContext,
                    key = operationKey,
                    label =
                        "Field Response Repeatability v0.1 · universele bronanalyse",
                )

        if (!started) {
            fieldResponseRepeatabilityStatus =
                "Field Response Repeatability v0.1 analyse kon niet starten: er loopt al een gelijknamige achtergrondanalyse."
            render()
            return
        }

        fieldResponseRepeatabilityStatus =
            "Universele bronanalyse draait nu in de Android foreground media-processing service · " +
                selected.size +
                " bronnen · app-focus niet vereist · resultaten worden per RAW persistent opgeslagen."
        render()
    }

    @Suppress("DEPRECATION")
    private fun launchFieldResponseRepeatabilityExport() {
        val charts = currentMeasuredFieldCharts()
        val report = FieldResponseRepeatabilityV01.evaluate(charts)
        if (
            report.optString("status") !=
            "READ_ONLY_REPEATABILITY_AUDIT_AVAILABLE"
        ) {
            fieldResponseRepeatabilityStatus =
                "Field Response Repeatability v0.1 blijft fail-closed: " +
                    report.optString("reason", "onvoldoende geschikte observaties") +
                    " · measured charts=" + charts.size + "/3."
            render()
            return
        }

        if (
            report.optJSONObject("interpretation")
                ?.optBoolean("calibration_promoted", true) != false ||
            report.optJSONObject("interpretation")
                ?.optBoolean("correction_gain_allowed", true) != false ||
            report.optBoolean("source_sample_values_modified", true) ||
            report.optBoolean("source_sample_positions_modified", true) ||
            report.optBoolean("new_measured_samples_created", true) ||
            report.optBoolean("scientific_writeback_allowed", true)
        ) {
            fieldResponseRepeatabilityStatus =
                "Field Response Repeatability v0.1 export geblokkeerd: read-only safety-contract mismatch."
            render()
            return
        }

        pendingFieldResponseRepeatabilityJson = report.toString(2) + "\n"
        ResearchPendingJsonExportStoreV01.save(
            filesDir = filesDir,
            key =
                ResearchPendingJsonExportStoreV01.FIELD_RESPONSE_REPEATABILITY,
            reportText =
                pendingFieldResponseRepeatabilityJson!!,
        )
        pendingFieldResponseRepeatabilityJson = null
        fieldResponseRepeatabilityStatus = null

        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                "draw_field_response_repeatability_v0_1_" +
                    report.optInt("observation_count", charts.size) +
                    "_observations.json",
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_SAVE_FIELD_RESPONSE_REPEATABILITY,
        )
    }

    private fun currentObservationWorldProfiles(): List<JSONObject> =
        session.jobs.mapNotNull { job ->
            universalProfiles[job.id]
        }

    @Suppress("DEPRECATION")
    private fun launchFreeWorldFoundationExport() {
        val profiles = currentObservationWorldProfiles()
        val repeatability =
            FieldResponseRepeatabilityV01.evaluate(
                currentMeasuredFieldCharts(),
            ).takeIf {
                it.optString("status") ==
                    "READ_ONLY_REPEATABILITY_AUDIT_AVAILABLE"
            }

        val report =
            FreeWorldObservationGeometryFoundationV01.build(
                profiles = profiles,
                fieldRepeatability = repeatability,
                calibrationRecords = currentCalibrationObservationRecords(),
            )

        if (
            report.optString("status") !=
            "FREE_WORLD_FOUNDATION_AVAILABLE"
        ) {
            freeWorldFoundationStatus =
                "Free World Foundation v0.1 blijft fail-closed: minimaal twee geprofileerde observations zijn nodig."
            render()
            return
        }

        val boundary =
            report.optJSONObject("promotion_boundary") ?: JSONObject()
        val firewall =
            report.optJSONObject("promotion_firewall") ?: JSONObject()
        if (
            firewall.optString("status") !=
            "RESEARCH_PROMOTION_FIREWALL_PASS" ||
            !firewall.optBoolean(
                "export_safe_under_current_research_contract",
                false,
            ) ||
            boundary.optBoolean("world_registration_promoted", true) ||
            boundary.optBoolean("camera_system_response_proven", true) ||
            boundary.optBoolean("lens_only_vignetting_proven", true) ||
            boundary.optBoolean("calibration_promoted", true) ||
            boundary.optBoolean("correction_authorized", true) ||
            boundary.optBoolean("deconvolution_authorized", true) ||
            boundary.optBoolean("multi_frame_scientific_fusion_applied", true) ||
            boundary.optBoolean("scientific_writeback_allowed", true) ||
            report.optBoolean("creates_new_evidence", true) ||
            report.optBoolean("scientific_writeback_allowed", true)
        ) {
            freeWorldFoundationStatus =
                "Free World Foundation v0.1 export geblokkeerd: promotion firewall mismatch."
            render()
            return
        }

        pendingFreeWorldFoundationJson = report.toString(2) + "\n"
        ResearchPendingJsonExportStoreV01.save(
            filesDir = filesDir,
            key =
                ResearchPendingJsonExportStoreV01.FREE_WORLD_FOUNDATION,
            reportText =
                pendingFreeWorldFoundationJson!!,
        )
        pendingFreeWorldFoundationJson = null
        freeWorldFoundationStatus = null

        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                "draw_free_world_observation_geometry_foundation_v0_1_" +
                    profiles.size +
                    "_observations.json",
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_SAVE_FREE_WORLD_FOUNDATION,
        )
    }

    @Suppress("DEPRECATION")
    private fun launchObservationWorldFieldSeparationExport() {
        val profiles = currentObservationWorldProfiles()
        val report = ObservationWorldFieldSeparationV01.evaluate(profiles)

        if (
            report.optString("status") !=
            "COORDINATE_AUTHORITY_SEPARATION_AVAILABLE"
        ) {
            observationWorldFieldSeparationStatus =
                "Observation-World Field Separation v0.1 blijft fail-closed: minimaal twee " +
                    "onderscheiden universele observations zijn nodig."
            render()
            return
        }

        val promotion =
            report.optJSONObject("promotion_boundary") ?: JSONObject()
        if (
            promotion.optBoolean("camera_system_response_proven", true) ||
            promotion.optBoolean("lens_only_vignetting_proven", true) ||
            promotion.optBoolean("calibration_promoted", true) ||
            promotion.optBoolean("correction_authorized", true) ||
            promotion.optBoolean("scientific_writeback_allowed", true) ||
            report.optBoolean("source_sample_values_modified", true) ||
            report.optBoolean("source_sample_positions_modified", true) ||
            report.optBoolean("new_measured_samples_created", true) ||
            report.optBoolean("scientific_writeback_allowed", true)
        ) {
            observationWorldFieldSeparationStatus =
                "Observation-World Field Separation v0.1 export geblokkeerd: safety-contract mismatch."
            render()
            return
        }

        pendingObservationWorldFieldSeparationJson =
            report.toString(2) + "\n"
        ResearchPendingJsonExportStoreV01.save(
            filesDir = filesDir,
            key =
                ResearchPendingJsonExportStoreV01.OBSERVATION_WORLD_FIELD_SEPARATION,
            reportText =
                pendingObservationWorldFieldSeparationJson!!,
        )
        pendingObservationWorldFieldSeparationJson = null
        observationWorldFieldSeparationStatus = null

        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                "draw_observation_world_field_separation_v0_1_" +
                    report.optInt("observation_count", profiles.size) +
                    "_observations.json",
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_SAVE_OBSERVATION_WORLD_FIELD_SEPARATION,
        )
    }

    @Suppress("DEPRECATION")
    private fun launchAppearanceHighlightDetailExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            appearanceHighlightDetailStatus =
                "Appearance Highlight Detail v0.1 vereist de admitted DNG-route."
            render()
            return
        }
        pendingAppearanceHighlightDetailJobId = job.id
        appearanceHighlightDetailStatus = null
        val stem =
            job.source.displayName.substringBeforeLast(
                '.',
                job.source.displayName,
            )
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                stem + "_draw_appearance_highlight_detail_audit_v0_1.json",
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_SAVE_APPEARANCE_HIGHLIGHT_DETAIL,
        )
    }

    @Suppress("DEPRECATION")
    private fun launchAppearanceHeadroomSweepExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            appearanceHeadroomSweepStatus =
                "Appearance Headroom Sweep v0.2 vereist de admitted DNG-route."
            render()
            return
        }
        pendingAppearanceHeadroomSweepJobId = job.id
        appearanceHeadroomSweepStatus = null
        val stem =
            job.source.displayName.substringBeforeLast(
                '.',
                job.source.displayName,
            )
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                stem + "_draw_appearance_highlight_headroom_sweep_v0_2.json",
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(
            intent,
            REQUEST_SAVE_APPEARANCE_HEADROOM_SWEEP,
        )
    }

    @Suppress("DEPRECATION")
    private fun launchTruthNegativeExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        if (!job.source.format.nativeProcessingReady || job.source.format.id != "DNG") {
            truthNegativeStatus =
                "Legacy TruthNegative TN-4 is momenteel alleen beschikbaar voor de volledig admitted DNG-route."
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
                truthNegative200MpStatus = "D.RAWnegative 200MP-export geannuleerd."
                render()
                return
            }
            val job = session.jobs.firstOrNull { it.id == expectedJob }
            val ready = previewState as? TilePreviewUiState.Ready
            if (expectedJob == null || job == null || ready == null ||
                ready.jobId != expectedJob || activeJobId != expectedJob
            ) {
                truthNegative200MpStatus =
                    "D.RAWnegative 200MP geblokkeerd: actieve D.RAW-route veranderde."
                render()
                return
            }

            val operationKey =
                backgroundOperationKey("truthnegative-200mp-full-colour", expectedJob)
            if (!startBackgroundOperation(
                    operationKey,
                    "D.RAWnegative 200MP Full Colour Float32 DNG opbouwen",
                )
            ) {
                truthNegative200MpStatus =
                    "D.RAWnegative 200MP kon niet veilig in de achtergrond starten."
                render()
                return
            }
            truthNegative200MpStatus =
                "D.RAWnegative 200MP · 16320×12288 camera-native Float32 wordt opgebouwd… " +
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
                            "D.RAWnegative 200MP preview faalde: ${previewResult.reason}",
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
                            "D.RAWnegative 200MP Full Colour gereed."
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
                            "D.RAWnegative 200MP v0.1 gereed · ${m.width}×${m.height} · " +
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


        if (requestCode == REQUEST_SAVE_N2_SPATIAL_SIDECAR) {
            val expectedJob = pendingN2SpatialSidecarJobId
            pendingN2SpatialSidecarJobId = null
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                n2SpatialSidecarStatus =
                    "N2 Spatial Audit-export geannuleerd."
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
                n2SpatialSidecarStatus =
                    "N2 Spatial Audit geblokkeerd: actieve Scientific Master-route veranderde."
                render()
                return
            }

            val operationKey =
                backgroundOperationKey(
                    "truthnegative-n2-spatial-sidecar",
                    expectedJob,
                )
            if (truthNegativeHeavyOperationActive(expectedJob, operationKey)) {
                n2SpatialSidecarStatus =
                    "Wacht op de andere D.RAWnegative/Camera-5 analysetaak. " +
                        "N2 Spatial Audit start daarna opnieuw handmatig."
                render()
                return
            }
            if (!startBackgroundOperation(
                    operationKey,
                    "N2 Spatial Audit sidecar opbouwen",
                )
            ) {
                n2SpatialSidecarStatus =
                    "N2 Spatial Audit achtergrondverwerking kon niet veilig starten."
                render()
                return
            }

            n2SpatialSidecarStatus =
                "N2 Spatial Audit · per 64×64 source-tile wordt een audit-only " +
                    "candidate/protection-kaart opgebouwd; zichtbare afbeelding blijft ongewijzigd…"
            render()

            startGuardedBackgroundThread(
                name = "draw-n2-spatial-" + job.id.take(8),
                operationKey = operationKey,
                onUnexpected = {
                    n2SpatialSidecarStatus = it
                },
            ) {
                val exportResult =
                    TruthNegativeN2SpatialSidecarExporter.export(
                        contentResolver,
                        job,
                        destination,
                    )
                finishBackgroundOperation(
                    operationKey,
                    exportResult is TruthNegativeN2SpatialSidecarResult.Success,
                    when (exportResult) {
                        is TruthNegativeN2SpatialSidecarResult.Success ->
                            "N2 Spatial Audit sidecar opgeslagen + SHA geverifieerd."
                        is TruthNegativeN2SpatialSidecarResult.Failed ->
                            exportResult.reason
                    },
                )
                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    n2SpatialSidecarStatus =
                        when (exportResult) {
                            is TruthNegativeN2SpatialSidecarResult.Failed ->
                                exportResult.reason
                            is TruthNegativeN2SpatialSidecarResult.Success -> {
                                val m = exportResult.metrics
                                "N2 Spatial Audit opgeslagen · " +
                                    m.width + "×" + m.height +
                                    " · " + formatBytes(m.fileBytes) +
                                    " · tiles=" + m.tileCount +
                                    " · sampled=" + m.sampled +
                                    " · candidate=" + m.candidateCorrected +
                                    " · preserved=" + m.preserved +
                                    " · structure=" + m.structureProtected +
                                    " · censored/boundary=" +
                                    m.censoredProtected + "/" +
                                    m.censorBoundaryProtected +
                                    " · removed-energy=" +
                                    "%.3f%%".format(
                                        m.removedResidualEnergyFraction * 100.0,
                                    ) +
                                    " · max|Δ|stage2=" +
                                    "%.8f".format(m.maxAbsCorrectionStage2) +
                                    " · spatial=" +
                                    m.spatialSha256.take(16) +
                                    "… · JSON=" +
                                    m.jsonSha256.take(16) +
                                    "… · candidate-applied=false."
                            }
                        }
                    render()
                }
            }
            return
        }

        if (requestCode == REQUEST_SAVE_N2_CENTER_EXCLUDED_SPATIAL) {
            val expectedJob = pendingN2CenterExcludedSpatialJobId
            pendingN2CenterExcludedSpatialJobId = null
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                n2CenterExcludedSpatialStatus =
                    "N2 v0.2.1 Spatial Audit-export geannuleerd."
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
                n2CenterExcludedSpatialStatus =
                    "N2 v0.2.1 Spatial Audit geblokkeerd: actieve Scientific Master-route veranderde."
                render()
                return
            }

            val operationKey =
                backgroundOperationKey(
                    "truthnegative-n2-center-excluded-spatial",
                    expectedJob,
                )
            if (truthNegativeHeavyOperationActive(expectedJob, operationKey)) {
                n2CenterExcludedSpatialStatus =
                    "Wacht op de andere D.RAWnegative/Camera-5 analysetaak. " +
                        "N2 v0.2.1 Spatial Audit start daarna opnieuw handmatig."
                render()
                return
            }
            if (!startBackgroundOperation(
                    operationKey,
                    "N2 v0.2.1 center-excluded spatial audit",
                )
            ) {
                n2CenterExcludedSpatialStatus =
                    "N2 v0.2.1 Spatial Audit achtergrondverwerking kon niet veilig starten."
                render()
                return
            }

            n2CenterExcludedSpatialStatus =
                "N2 v0.2.1 · v0.1 tile-parity + center-excluded predictorstatistiek " +
                    "per 64×64 source-tile; B en Scientific Master blijven ongewijzigd…"
            render()

            startGuardedBackgroundThread(
                name = "draw-n2-ce-spatial-" + job.id.take(8),
                operationKey = operationKey,
                onUnexpected = {
                    n2CenterExcludedSpatialStatus = it
                },
            ) {
                val exportResult =
                    TruthNegativeN2CenterExcludedSpatialExporter.export(
                        contentResolver,
                        job,
                        destination,
                    )
                finishBackgroundOperation(
                    operationKey,
                    exportResult is
                        TruthNegativeN2CenterExcludedSpatialResult.Success,
                    when (exportResult) {
                        is TruthNegativeN2CenterExcludedSpatialResult.Success ->
                            "N2 v0.2.1 Spatial Audit opgeslagen + SHA geverifieerd."
                        is TruthNegativeN2CenterExcludedSpatialResult.Failed ->
                            exportResult.reason
                    },
                )
                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    n2CenterExcludedSpatialStatus =
                        when (exportResult) {
                            is TruthNegativeN2CenterExcludedSpatialResult.Failed ->
                                exportResult.reason
                            is TruthNegativeN2CenterExcludedSpatialResult.Success -> {
                                val m = exportResult.metrics
                                val validPct =
                                    if (m.v01CandidateCenters > 0L) {
                                        100.0 * m.predictorValid.toDouble() /
                                            m.v01CandidateCenters.toDouble()
                                    } else 0.0
                                val pairPct =
                                    if (m.pairsConsidered > 0L) {
                                        100.0 * m.pairsAccepted.toDouble() /
                                            m.pairsConsidered.toDouble()
                                    } else 0.0
                                val scalePct =
                                    if (m.scalesConsidered > 0L) {
                                        100.0 * m.scalesAccepted.toDouble() /
                                            m.scalesConsidered.toDouble()
                                    } else 0.0
                                val centerGt2Pct =
                                    if (m.predictorValid > 0L) {
                                        100.0 * m.centerZGt2.toDouble() /
                                            m.predictorValid.toDouble()
                                    } else 0.0
                                "N2 v0.2.1 Spatial Audit opgeslagen · " +
                                    m.width + "×" + m.height +
                                    " · " + formatBytes(m.fileBytes) +
                                    " · tiles=" + m.tileCount +
                                    " · v0.1 candidates=" + m.v01CandidateCenters +
                                    " · predictor-valid=" +
                                    "%.2f%%".format(validPct) +
                                    " · pair-accept=" +
                                    "%.2f%%".format(pairPct) +
                                    " · scale-accept=" +
                                    "%.2f%%".format(scalePct) +
                                    " · center >2σ=" +
                                    "%.2f%%".format(centerGt2Pct) +
                                    " · mean|max |r|=" +
                                    "%.8f".format(m.meanAbsResidual) + "/" +
                                    "%.8f".format(m.maxAbsResidual) +
                                    " · CE=" +
                                    m.centerExcludedAuditSha256.take(16) +
                                    "… · JSON=" +
                                    m.jsonSha256.take(16) +
                                    "… · audit-only."
                            }
                        }
                    render()
                }
            }
            return
        }

        if (requestCode == REQUEST_SAVE_N2_CONFIDENCE_FIELD) {
            val expectedJob = pendingN2ConfidenceFieldJobId
            pendingN2ConfidenceFieldJobId = null
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                n2ConfidenceFieldStatus =
                    "N2 Confidence Field v0.3-export geannuleerd."
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
                n2ConfidenceFieldStatus =
                    "N2 Confidence Field v0.3 geblokkeerd: actieve Scientific Master-route veranderde."
                render()
                return
            }

            val operationKey =
                backgroundOperationKey(
                    "truthnegative-n2-confidence-field",
                    expectedJob,
                )
            if (truthNegativeHeavyOperationActive(expectedJob, operationKey)) {
                n2ConfidenceFieldStatus =
                    "Wacht op de andere D.RAWnegative/Camera-5 analysetaak. " +
                        "N2 Confidence Field v0.3 start daarna opnieuw handmatig."
                render()
                return
            }
            if (!startBackgroundOperation(
                    operationKey,
                    "N2 Confidence Field v0.3",
                )
            ) {
                n2ConfidenceFieldStatus =
                    "N2 Confidence Field v0.3 achtergrondverwerking kon niet veilig starten."
                render()
                return
            }

            n2ConfidenceFieldStatus =
                "N2 Confidence Field v0.3 · vector-valued audit uit v0.1 + v0.2.1; " +
                    "geen scalar probability, geen correctie en geen writeback…"
            render()

            startGuardedBackgroundThread(
                name = "draw-n2-confidence-" + job.id.take(8),
                operationKey = operationKey,
                onUnexpected = {
                    n2ConfidenceFieldStatus = it
                },
            ) {
                val exportResult =
                    TruthNegativeN2ConfidenceFieldExporter.export(
                        contentResolver,
                        job,
                        destination,
                    )
                finishBackgroundOperation(
                    operationKey,
                    exportResult is
                        TruthNegativeN2ConfidenceFieldResult.Success,
                    when (exportResult) {
                        is TruthNegativeN2ConfidenceFieldResult.Success ->
                            "N2 Confidence Field v0.3 opgeslagen + SHA geverifieerd."
                        is TruthNegativeN2ConfidenceFieldResult.Failed ->
                            exportResult.reason
                    },
                )
                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    n2ConfidenceFieldStatus =
                        when (exportResult) {
                            is TruthNegativeN2ConfidenceFieldResult.Failed ->
                                exportResult.reason
                            is TruthNegativeN2ConfidenceFieldResult.Success -> {
                                val m = exportResult.metrics
                                "N2 Confidence Field v0.3 opgeslagen · " +
                                    m.width + "×" + m.height +
                                    " · " + formatBytes(m.fileBytes) +
                                    " · tiles=" + m.tileCount +
                                    " · candidate=" +
                                    "%.2f%%".format(100.0 * m.candidateFraction) +
                                    " · predictor=" +
                                    "%.2f%%".format(100.0 * m.predictorCoverage) +
                                    " · pair=" +
                                    "%.2f%%".format(100.0 * m.pairAcceptance) +
                                    " · scale=" +
                                    "%.2f%%".format(100.0 * m.scaleAcceptance) +
                                    " · center >2σ=" +
                                    "%.2f%%".format(100.0 * m.centerZGt2Fraction) +
                                    " · classes N/U/M/F=" +
                                    m.noCandidateTiles + "/" +
                                    m.unresolvedTiles + "/" +
                                    m.mixedTiles + "/" +
                                    m.fullyCoherentTiles +
                                    " · CF=" +
                                    m.confidenceFieldSha256.take(16) +
                                    "… · promotion=false."
                            }
                        }
                    render()
                }
            }
            return
        }

        if (requestCode == REQUEST_SAVE_N2_FACTORED_CONFIDENCE) {
            val expectedJob = pendingN2FactoredConfidenceJobId
            pendingN2FactoredConfidenceJobId = null
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                n2FactoredConfidenceStatus =
                    "N2 Factored Confidence v0.3.1-export geannuleerd."
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
                n2FactoredConfidenceStatus =
                    "N2 Factored Confidence v0.3.1 geblokkeerd: actieve Scientific Master-route veranderde."
                render()
                return
            }

            val operationKey =
                backgroundOperationKey(
                    "truthnegative-n2-factored-confidence",
                    expectedJob,
                )
            if (truthNegativeHeavyOperationActive(expectedJob, operationKey)) {
                n2FactoredConfidenceStatus =
                    "Wacht op de andere D.RAWnegative/Camera-5 analysetaak. " +
                        "N2 Factored Confidence v0.3.1 start daarna opnieuw handmatig."
                render()
                return
            }
            if (!startBackgroundOperation(
                    operationKey,
                    "N2 Factored Confidence v0.3.1",
                )
            ) {
                n2FactoredConfidenceStatus =
                    "N2 Factored Confidence v0.3.1 achtergrondverwerking kon niet veilig starten."
                render()
                return
            }

            n2FactoredConfidenceStatus =
                "N2 v0.3.1 · afzonderlijke confidence-feiten uit exact gebonden v0.3; " +
                    "geen gewichten, geen scalar probability en promotion=false…"
            render()

            startGuardedBackgroundThread(
                name = "draw-n2-factored-" + job.id.take(8),
                operationKey = operationKey,
                onUnexpected = {
                    n2FactoredConfidenceStatus = it
                },
            ) {
                val exportResult =
                    TruthNegativeN2FactoredConfidenceExporter.export(
                        contentResolver,
                        job,
                        destination,
                    )
                finishBackgroundOperation(
                    operationKey,
                    exportResult is
                        TruthNegativeN2FactoredConfidenceResult.Success,
                    when (exportResult) {
                        is TruthNegativeN2FactoredConfidenceResult.Success ->
                            "N2 Factored Confidence v0.3.1 opgeslagen + SHA geverifieerd."
                        is TruthNegativeN2FactoredConfidenceResult.Failed ->
                            exportResult.reason
                    },
                )
                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    n2FactoredConfidenceStatus =
                        when (exportResult) {
                            is TruthNegativeN2FactoredConfidenceResult.Failed ->
                                exportResult.reason
                            is TruthNegativeN2FactoredConfidenceResult.Success -> {
                                val m = exportResult.metrics
                                "N2 Factored Confidence v0.3.1 opgeslagen · " +
                                    m.width + "×" + m.height +
                                    " · " + formatBytes(m.fileBytes) +
                                    " · tiles=" + m.tileCount +
                                    " · candidates=" + m.hasCandidateTiles +
                                    " · all-predictable=" +
                                    m.allCandidatesPredictableTiles +
                                    " · outlier-free=" +
                                    m.centerOutlierFreeTiles +
                                    " · both=" +
                                    m.predictableAndCenterOutlierFreeTiles +
                                    " · pair-free=" +
                                    m.pairRejectionFreeTiles +
                                    " · scale-free=" +
                                    m.scaleRejectionFreeTiles +
                                    " · variance≤center=" +
                                    m.maxPredictorVarianceLeCenterVarianceTiles +
                                    " · FS=" +
                                    m.factoredStateSha256.take(16) +
                                    "… · promotion=false."
                            }
                        }
                    render()
                }
            }
            return
        }

        if (requestCode == REQUEST_SAVE_N2_SUPPORT_DISTANCE) {
            val expectedJob = pendingN2SupportDistanceJobId
            pendingN2SupportDistanceJobId = null
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                n2SupportDistanceStatus =
                    "N2 Sample Support Distance v0.1-export geannuleerd."
                render()
                return
            }

            val job = session.jobs.firstOrNull { it.id == expectedJob }
            val ready = previewState as? TilePreviewUiState.Ready
            val profile =
                expectedJob?.let { universalProfiles[it] }
            val support =
                profile?.optJSONObject("n2_sample_support_distance")
            val frontsideV01 =
                profile?.optJSONObject("scene_analysis")
                    ?.optJSONObject("dark_chroma_stability_v0_1")
            val expectedSourceSha =
                support?.optString("source_sha256", "") ?: ""

            if (
                expectedJob == null ||
                job == null ||
                ready == null ||
                ready.jobId != expectedJob ||
                activeJobId != expectedJob ||
                support?.optString("status") !=
                    "AUDIT_ONLY_DISTANCE_BINDING_AVAILABLE" ||
                frontsideV01 == null ||
                expectedSourceSha.isBlank()
            ) {
                n2SupportDistanceStatus =
                    "N2 Sample Support Distance v0.1 geblokkeerd: actieve bron/binding veranderde of de v0.6-audit is niet beschikbaar."
                render()
                return
            }

            val operationKey =
                backgroundOperationKey(
                    "truthnegative-n2-support-distance",
                    expectedJob,
                )
            if (truthNegativeHeavyOperationActive(expectedJob, operationKey)) {
                n2SupportDistanceStatus =
                    "Wacht op de andere D.RAWnegative/Camera-5 analysetaak. " +
                        "N2 Sample Support Distance v0.1 start daarna opnieuw handmatig."
                render()
                return
            }
            if (!startBackgroundOperation(
                    operationKey,
                    "N2 Sample Support Distance v0.1",
                )
            ) {
                n2SupportDistanceStatus =
                    "N2 Sample Support Distance v0.1 achtergrondverwerking kon niet veilig starten."
                render()
                return
            }

            n2SupportDistanceStatus =
                "N2 v0.6 support-distance sidecar · exacte sampled structure/censor-coördinaten " +
                    "+ SHA-binding · geen drempel, geen correction-enable…"
            render()

            startGuardedBackgroundThread(
                name = "draw-n2-support-distance-" + job.id.take(8),
                operationKey = operationKey,
                onUnexpected = {
                    n2SupportDistanceStatus = it
                },
            ) {
                val exportResult = runCatching {
                    N2SampleSupportDistanceAudit.exportSidecar(
                        resolver = contentResolver,
                        sourceUri = job.source.uri,
                        destinationUri = destination,
                        expectedSourceSha256 = expectedSourceSha,
                        frontsideV01 = frontsideV01,
                    )
                }

                val success = exportResult.isSuccess
                val finishMessage =
                    exportResult.fold(
                        onSuccess = {
                            "N2 Sample Support Distance v0.1 opgeslagen + SHA geverifieerd."
                        },
                        onFailure = {
                            "N2 Sample Support Distance v0.1 export faalde: " +
                                (it.message ?: it.javaClass.simpleName)
                        },
                    )
                finishBackgroundOperation(
                    operationKey,
                    success,
                    finishMessage,
                )

                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    n2SupportDistanceStatus =
                        exportResult.fold(
                            onSuccess = { status ->
                                "N2 Sample Support Distance v0.1 opgeslagen · " +
                                    status.optInt("width", 0) + "×" +
                                    status.optInt("height", 0) +
                                    " · " +
                                    formatBytes(status.optLong("fileBytes", 0L)) +
                                    " · queries=" +
                                    status.optInt("queryCount", 0) +
                                    " · structure=" +
                                    status.optLong("structureProtected", 0L) +
                                    " · support=" +
                                    status.optString(
                                        "supportPointStreamSha256",
                                        "",
                                    ).take(16) +
                                    "… · JSON=" +
                                    status.optString("jsonSha256", "").take(16) +
                                    "… · correction=false."
                            },
                            onFailure = {
                                "N2 Sample Support Distance v0.1 export faalde: " +
                                    (it.message ?: it.javaClass.simpleName)
                            },
                        )
                    render()
                }
            }
            return
        }

        if (requestCode == REQUEST_SAVE_ANCHOR_RECONSTRUCTION) {
            val expectedJob = pendingAnchorReconstructionJobId
            pendingAnchorReconstructionJobId = null
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                anchorReconstructionStatus =
                    "Anchor-Constrained Local Reconstruction v0.1-export geannuleerd."
                render()
                return
            }

            val job = session.jobs.firstOrNull { it.id == expectedJob }
            val ready = previewState as? TilePreviewUiState.Ready
            val profile =
                expectedJob?.let { universalProfiles[it] }
            val audit =
                profile?.optJSONObject(
                    "anchor_constrained_local_reconstruction",
                )
            val sampleLattice =
                profile?.optJSONObject(
                    "raster_independent_sample_lattice",
                )
            val support =
                profile?.optJSONObject("n2_sample_support_distance")
            val frontsideV01 =
                profile?.optJSONObject("scene_analysis")
                    ?.optJSONObject("dark_chroma_stability_v0_1")
            val expectedSourceSha =
                audit?.optString("source_sha256", "") ?: ""

            if (
                expectedJob == null ||
                job == null ||
                ready == null ||
                ready.jobId != expectedJob ||
                activeJobId != expectedJob ||
                audit?.optString("status") !=
                    "AUDIT_ONLY_HOLDOUT_VALIDATION_AVAILABLE" ||
                frontsideV01 == null ||
                sampleLattice == null ||
                support == null ||
                expectedSourceSha.isBlank()
            ) {
                anchorReconstructionStatus =
                    "Anchor-Constrained Local Reconstruction v0.1 geblokkeerd: actieve bron/lattice/support-binding veranderde of de holdout-audit is niet beschikbaar."
                render()
                return
            }

            val operationKey =
                backgroundOperationKey(
                    "anchor-constrained-local-reconstruction",
                    expectedJob,
                )
            if (truthNegativeHeavyOperationActive(expectedJob, operationKey)) {
                anchorReconstructionStatus =
                    "Wacht op de andere D.RAWnegative analysetaak. Anchor-Constrained Local Reconstruction v0.1 start daarna opnieuw handmatig."
                render()
                return
            }
            if (!startBackgroundOperation(
                    operationKey,
                    "Anchor-Constrained Local Reconstruction v0.1",
                )
            ) {
                anchorReconstructionStatus =
                    "Anchor-Constrained Local Reconstruction v0.1 achtergrondverwerking kon niet veilig starten."
                render()
                return
            }

            anchorReconstructionStatus =
                "Anchor holdout-sidecar · echte CFA-ankers tijdelijk verborgen voor predictor · " +
                    "private RECONSTRUCTED schatting + onzekerheid · geen writeback…"
            render()

            startGuardedBackgroundThread(
                name = "draw-anchor-holdout-" + job.id.take(8),
                operationKey = operationKey,
                onUnexpected = {
                    anchorReconstructionStatus = it
                },
            ) {
                val exportResult = runCatching {
                    AnchorConstrainedLocalReconstructionAudit.exportSidecar(
                        resolver = contentResolver,
                        sourceUri = job.source.uri,
                        destinationUri = destination,
                        expectedSourceSha256 = expectedSourceSha,
                        frontsideV01 = frontsideV01,
                        sampleLattice = sampleLattice,
                        supportDistance = support,
                    )
                }

                finishBackgroundOperation(
                    operationKey,
                    exportResult.isSuccess,
                    exportResult.fold(
                        onSuccess = {
                            "Anchor-Constrained Local Reconstruction v0.1 opgeslagen + SHA geverifieerd."
                        },
                        onFailure = {
                            "Anchor-Constrained Local Reconstruction v0.1 export faalde: " +
                                (it.message ?: it.javaClass.simpleName)
                        },
                    ),
                )

                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    anchorReconstructionStatus =
                        exportResult.fold(
                            onSuccess = { status ->
                                "Anchor-Constrained Local Reconstruction v0.1 opgeslagen · " +
                                    status.optInt("width", 0) + "×" +
                                    status.optInt("height", 0) +
                                    " · " +
                                    formatBytes(status.optLong("fileBytes", 0L)) +
                                    " · queries=" +
                                    status.optInt("queryCount", 0) +
                                    " · holdouts=" +
                                    status.optLong("holdouts", 0L) +
                                    " · valid solver/baseline=" +
                                    status.optLong("solverValid", 0L) + "/" +
                                    status.optLong("baselineValid", 0L) +
                                    " · lower-|error| solver/baseline=" +
                                    status.optLong("solverLowerAbsError", 0L) + "/" +
                                    status.optLong("baselineLowerAbsError", 0L) +
                                    " · holdout=" +
                                    status.optString(
                                        "holdoutStreamSha256",
                                        "",
                                    ).take(16) +
                                    "… · JSON=" +
                                    status.optString("jsonSha256", "").take(16) +
                                    "… · writeback=false."
                            },
                            onFailure = {
                                "Anchor-Constrained Local Reconstruction v0.1 export faalde: " +
                                    (it.message ?: it.javaClass.simpleName)
                            },
                        )
                    render()
                }
            }
            return
        }

        if (requestCode == REQUEST_SAVE_OBSERVATION_MODEL_SELECTION) {
            val expectedJob = pendingObservationModelSelectionJobId
            val report = pendingObservationModelSelectionJson
            pendingObservationModelSelectionJobId = null
            pendingObservationModelSelectionJson = null
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                observationModelSelectionStatus =
                    "Universal Observation Model Selection v0.1-export geannuleerd."
                render()
                return
            }

            val policy =
                expectedJob?.let { universalProfiles[it] }
                    ?.optJSONObject(
                        "universal_observation_model_selection",
                    )
            if (
                expectedJob == null ||
                expectedJob != activeJobId ||
                report == null ||
                policy == null
            ) {
                observationModelSelectionStatus =
                    "Universal Observation Model Selection v0.1 geblokkeerd: actieve bron/policy veranderde."
                render()
                return
            }
            if (
                policy.optString("status") !=
                    "PROSPECTIVE_AUDIT_POLICY_AVAILABLE" ||
                policy.optBoolean("heldout_target_used_for_selection", true) ||
                policy.optBoolean("holdout_error_used_for_selection", true) ||
                policy.optBoolean("lens_calibration_used", true) ||
                policy.optBoolean("camera_model_used", true) ||
                policy.optBoolean("vendor_mapping_used", true) ||
                policy.optBoolean("scientific_writeback_allowed", true)
            ) {
                observationModelSelectionStatus =
                    "Universal Observation Model Selection v0.1 geblokkeerd: target-blind/universele safety-contract mismatch."
                render()
                return
            }

            observationModelSelectionStatus = try {
                val stream =
                    contentResolver.openOutputStream(destination, "w")
                        ?: throw IOException(
                            "Documentprovider gaf geen outputstream.",
                        )
                stream.bufferedWriter(Charsets.UTF_8).use {
                    it.write(report)
                }
                "Universal Observation Model Selection v0.1 JSON opgeslagen · target-blind modelbank · queries=" +
                    policy.optInt("query_count", 0) +
                    " · lens/device calibration=false · writeback=false."
            } catch (error: Exception) {
                "Universal Observation Model Selection v0.1 export faalde: " +
                    (error.message ?: error.javaClass.simpleName)
            }
            render()
            return
        }

        if (requestCode == REQUEST_SAVE_OBSERVATION_OPTICAL_FIELD) {
            val expectedJob = pendingObservationOpticalFieldJobId
            val report = pendingObservationOpticalFieldJson
            pendingObservationOpticalFieldJobId = null
            pendingObservationOpticalFieldJson = null
            val destination = data?.data

            if (resultCode != RESULT_OK || destination == null) {
                observationOpticalFieldStatus =
                    "Observation Optical Field Chart v0.1-export geannuleerd."
                render()
                return
            }

            val chart =
                expectedJob?.let { universalProfiles[it] }
                    ?.optJSONObject("observation_optical_field_chart")
            if (
                expectedJob == null ||
                expectedJob != activeJobId ||
                report == null ||
                chart == null
            ) {
                observationOpticalFieldStatus =
                    "Observation Optical Field Chart v0.1 geblokkeerd: actieve sealed observation veranderde."
                render()
                return
            }

            if (
                chart.optString("status") != "FIELD_CHART_AVAILABLE" ||
                chart.optJSONObject("vignetting_interpretation")
                    ?.optBoolean("correction_gain_allowed", true) != false ||
                chart.optBoolean("source_sample_values_modified", true) ||
                chart.optBoolean("source_sample_positions_modified", true) ||
                chart.optBoolean("new_measured_samples_created", true) ||
                chart.optBoolean("scientific_writeback_allowed", true)
            ) {
                observationOpticalFieldStatus =
                    "Observation Optical Field Chart v0.1 geblokkeerd: read-only optical-field safety-contract mismatch."
                render()
                return
            }

            observationOpticalFieldStatus = try {
                val stream =
                    contentResolver.openOutputStream(destination, "w")
                        ?: throw IOException(
                            "Documentprovider gaf geen outputstream.",
                        )
                stream.bufferedWriter(Charsets.UTF_8).use {
                    it.write(report)
                }
                val signal =
                    chart.optJSONObject("measured_composite_field_signal")
                val opcode =
                    chart.optJSONObject("source_opcode_provenance_hint")
                        ?.optJSONObject("opcode_list_2")
                "Observation Optical Field Chart v0.1 JSON opgeslagen · field=" +
                    chart.optString("status") +
                    " · measured-signal=" +
                    (signal?.optString("status") ?: "UNKNOWN") +
                    " · GainMaps=" +
                    (opcode?.optInt("gain_map_opcode_count", 0) ?: 0) +
                    " · correction=false · writeback=false."
            } catch (error: Exception) {
                "Observation Optical Field Chart v0.1 export faalde: " +
                    (error.message ?: error.javaClass.simpleName)
            }
            render()
            return
        }

        if (requestCode == REQUEST_SAVE_UNIVERSAL_CALIBRATION_ATLAS) {
            val expectedJob = pendingUniversalCalibrationAtlasJobId
            val report = pendingUniversalCalibrationAtlasJson
            pendingUniversalCalibrationAtlasJobId = null
            pendingUniversalCalibrationAtlasJson = null
            val destination = data?.data

            if (resultCode != RESULT_OK || destination == null) {
                universalCalibrationAtlasStatus =
                    "Universal Observation & Calibration Atlas v0.1-export geannuleerd."
                render()
                return
            }

            val atlas =
                expectedJob?.let { universalProfiles[it] }
                    ?.optJSONObject("universal_observation_calibration_atlas")
            if (
                expectedJob == null ||
                expectedJob != activeJobId ||
                report == null ||
                atlas == null
            ) {
                universalCalibrationAtlasStatus =
                    "Universal Observation & Calibration Atlas v0.1 geblokkeerd: actieve sealed observation veranderde."
                render()
                return
            }

            val identity = atlas.optJSONObject("universal_identity_policy")
            val colour = atlas.optJSONObject("colour_state")
            val illumination = atlas.optJSONObject("illumination_state")
            val optical = atlas.optJSONObject("optical_support")
            if (
                atlas.optString("status") != "OBSERVATION_ATLAS_AVAILABLE" ||
                identity?.optBoolean("camera_identity_required", true) != false ||
                identity?.optBoolean("lens_identity_required", true) != false ||
                identity?.optBoolean("prior_user_calibration_required", true) != false ||
                colour?.optBoolean("automatic_colour_correction_from_atlas_allowed", true) != false ||
                illumination?.optBoolean("automatic_light_falloff_correction_allowed", true) != false ||
                optical?.optBoolean("deconvolution_authorized", true) != false ||
                atlas.optBoolean("source_sample_values_modified", true) ||
                atlas.optBoolean("source_sample_positions_modified", true) ||
                atlas.optBoolean("new_measured_samples_created", true) ||
                atlas.optBoolean("scientific_writeback_allowed", true)
            ) {
                universalCalibrationAtlasStatus =
                    "Universal Observation & Calibration Atlas v0.1 geblokkeerd: universal/read-only safety-contract mismatch."
                render()
                return
            }

            universalCalibrationAtlasStatus = try {
                val stream =
                    contentResolver.openOutputStream(destination, "w")
                        ?: throw IOException(
                            "Documentprovider gaf geen outputstream.",
                        )
                stream.bufferedWriter(Charsets.UTF_8).use {
                    it.write(report)
                }
                val front = atlas.optJSONObject("frontside")
                val back = atlas.optJSONObject("backside")
                val field = atlas.optJSONObject("field_response")
                "Universal Observation & Calibration Atlas v0.1 JSON opgeslagen · front=" +
                    (front?.optString("status") ?: "UNKNOWN") +
                    " · backside-measured=" +
                    (back?.optBoolean("measured_signal_available", false) ?: false) +
                    " · field=" +
                    (field?.optString("coordinate_chart_status") ?: "UNKNOWN") +
                    " · camera/lens-identiteit vereist=false · correctie=false · writeback=false."
            } catch (error: Exception) {
                "Universal Observation & Calibration Atlas v0.1 export faalde: " +
                    (error.message ?: error.javaClass.simpleName)
            }
            render()
            return
        }

        if (requestCode == REQUEST_SAVE_FREE_WORLD_FOUNDATION) {
            pendingFreeWorldFoundationJson = null
            val destination = data?.data

            if (resultCode != RESULT_OK || destination == null) {
                ResearchPendingJsonExportStoreV01.clear(
                    filesDir = filesDir,
                    key =
                        ResearchPendingJsonExportStoreV01.FREE_WORLD_FOUNDATION,
                )
                freeWorldFoundationStatus =
                    "Free World Foundation v0.1-export geannuleerd."
                render()
                return
            }

            val copied =
                runCatching {
                    ResearchPendingJsonExportStoreV01.copyFrozenTo(
                        filesDir = filesDir,
                        key =
                            ResearchPendingJsonExportStoreV01.FREE_WORLD_FOUNDATION,
                        resolver = contentResolver,
                        destination = destination,
                    )
                }

            freeWorldFoundationStatus =
                copied.fold(
                    onSuccess = { result ->
                        "Free World Foundation v0.1 JSON opgeslagen · bevroren snapshot exact gekopieerd · bytes=" +
                            result.byteLength +
                            " · SHA-256=" +
                            result.sha256.take(16) +
                            "… · geen herberekening na bestandskiezer."
                    },
                    onFailure = { error ->
                        "Free World Foundation v0.1 export faalde: " +
                            (
                                error.message
                                    ?: error.javaClass.simpleName
                                )
                    },
                )

            ResearchPendingJsonExportStoreV01.clear(
                filesDir = filesDir,
                key =
                    ResearchPendingJsonExportStoreV01.FREE_WORLD_FOUNDATION,
            )
            render()
            return
        }

        if (requestCode == REQUEST_SAVE_OBSERVATION_WORLD_FIELD_SEPARATION) {
            pendingObservationWorldFieldSeparationJson = null
            val destination = data?.data

            if (resultCode != RESULT_OK || destination == null) {
                ResearchPendingJsonExportStoreV01.clear(
                    filesDir = filesDir,
                    key =
                        ResearchPendingJsonExportStoreV01.OBSERVATION_WORLD_FIELD_SEPARATION,
                )
                observationWorldFieldSeparationStatus =
                    "Observation-World Field Separation v0.1-export geannuleerd."
                render()
                return
            }

            val copied =
                runCatching {
                    ResearchPendingJsonExportStoreV01.copyFrozenTo(
                        filesDir = filesDir,
                        key =
                            ResearchPendingJsonExportStoreV01.OBSERVATION_WORLD_FIELD_SEPARATION,
                        resolver = contentResolver,
                        destination = destination,
                    )
                }

            observationWorldFieldSeparationStatus =
                copied.fold(
                    onSuccess = { result ->
                        "Observation-World Field Separation v0.1 JSON opgeslagen · bevroren snapshot exact gekopieerd · bytes=" +
                            result.byteLength +
                            " · SHA-256=" +
                            result.sha256.take(16) +
                            "…"
                    },
                    onFailure = { error ->
                        "Observation-World Field Separation v0.1 export faalde: " +
                            (
                                error.message
                                    ?: error.javaClass.simpleName
                                )
                    },
                )

            ResearchPendingJsonExportStoreV01.clear(
                filesDir = filesDir,
                key =
                    ResearchPendingJsonExportStoreV01.OBSERVATION_WORLD_FIELD_SEPARATION,
            )
            render()
            return
        }

        if (requestCode == REQUEST_SAVE_FIELD_RESPONSE_REPEATABILITY) {
            pendingFieldResponseRepeatabilityJson = null
            val destination = data?.data

            if (resultCode != RESULT_OK || destination == null) {
                ResearchPendingJsonExportStoreV01.clear(
                    filesDir = filesDir,
                    key =
                        ResearchPendingJsonExportStoreV01.FIELD_RESPONSE_REPEATABILITY,
                )
                fieldResponseRepeatabilityStatus =
                    "Field Response Repeatability v0.1-export geannuleerd."
                render()
                return
            }

            val copied =
                runCatching {
                    ResearchPendingJsonExportStoreV01.copyFrozenTo(
                        filesDir = filesDir,
                        key =
                            ResearchPendingJsonExportStoreV01.FIELD_RESPONSE_REPEATABILITY,
                        resolver = contentResolver,
                        destination = destination,
                    )
                }

            fieldResponseRepeatabilityStatus =
                copied.fold(
                    onSuccess = { result ->
                        "Field Response Repeatability v0.1 JSON opgeslagen · bevroren snapshot exact gekopieerd · bytes=" +
                            result.byteLength +
                            " · SHA-256=" +
                            result.sha256.take(16) +
                            "…"
                    },
                    onFailure = { error ->
                        "Field Response Repeatability v0.1 export faalde: " +
                            (
                                error.message
                                    ?: error.javaClass.simpleName
                                )
                    },
                )

            ResearchPendingJsonExportStoreV01.clear(
                filesDir = filesDir,
                key =
                    ResearchPendingJsonExportStoreV01.FIELD_RESPONSE_REPEATABILITY,
            )
            render()
            return
        }

        if (requestCode == REQUEST_SAVE_UNIVERSAL_MODEL_BANK_HOLDOUT) {
            val expectedJob = pendingUniversalModelBankHoldoutJobId
            pendingUniversalModelBankHoldoutJobId = null
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                universalModelBankHoldoutStatus =
                    "Universal Local Model Bank Holdout v0.1-export geannuleerd."
                render()
                return
            }

            val job = session.jobs.firstOrNull { it.id == expectedJob }
            val ready = previewState as? TilePreviewUiState.Ready
            if (
                expectedJob == null ||
                job == null ||
                ready == null ||
                ready.jobId != expectedJob ||
                activeJobId != expectedJob
            ) {
                universalModelBankHoldoutStatus =
                    "Universal Local Model Bank Holdout v0.1 geblokkeerd: actieve sealed observation veranderde."
                render()
                return
            }

            val operationKey =
                backgroundOperationKey(
                    "universal-local-model-bank-holdout",
                    expectedJob,
                )
            if (truthNegativeHeavyOperationActive(expectedJob, operationKey)) {
                universalModelBankHoldoutStatus =
                    "Wacht op de andere zware D.RAW-analysetaak; Universal Local Model Bank Holdout v0.1 start daarna opnieuw handmatig."
                render()
                return
            }
            if (!startBackgroundOperation(
                    operationKey,
                    "Universal Local Model Bank Holdout v0.1",
                )
            ) {
                universalModelBankHoldoutStatus =
                    "Universal Local Model Bank Holdout v0.1 achtergrondverwerking kon niet veilig starten."
                render()
                return
            }

            universalModelBankHoldoutStatus =
                "Universal Local Model Bank Holdout v0.1 · full-resolution CFA holdouts + target-blinde modelselectie worden doorgerekend…"
            render()

            startGuardedBackgroundThread(
                name = "draw-universal-model-bank-" + job.id.take(8),
                operationKey = operationKey,
                onUnexpected = {
                    universalModelBankHoldoutStatus = it
                },
            ) {
                val exportResult = runCatching {
                    UniversalLocalModelBankHoldoutV01.exportSidecar(
                        resolver = contentResolver,
                        sourceUri = job.source.uri,
                        destinationUri = destination,
                        expectedSourceSha256 =
                            universalProfiles[expectedJob]
                                ?.optString("source_sha256", "")
                                .orEmpty(),
                    )
                }

                finishBackgroundOperation(
                    operationKey,
                    exportResult.isSuccess,
                    exportResult.fold(
                        onSuccess = {
                            "Universal Local Model Bank Holdout v0.1 gereed."
                        },
                        onFailure = {
                            "Universal Local Model Bank Holdout v0.1 faalde: " +
                                (it.message ?: it.javaClass.simpleName)
                        },
                    ),
                )

                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    universalModelBankHoldoutStatus =
                        exportResult.fold(
                            onSuccess = { status ->
                                "Universal Local Model Bank Holdout v0.1 opgeslagen · " +
                                    status.optInt("width", 0) + "×" +
                                    status.optInt("height", 0) +
                                    " · " +
                                    formatBytes(status.optLong("fileBytes", 0L)) +
                                    " · holdouts=" +
                                    status.optLong("holdouts", 0L) +
                                    " · selected/baseline valid=" +
                                    status.optLong("selectedValid", 0L) + "/" +
                                    status.optLong("baselineValid", 0L) +
                                    " · lower-|error| selected/baseline=" +
                                    status.optLong(
                                        "selectedLowerAbsErrorThanBaseline",
                                        0L,
                                    ) + "/" +
                                    status.optLong(
                                        "baselineLowerAbsErrorThanSelected",
                                        0L,
                                    ) +
                                    " · holdout=" +
                                    status.optString(
                                        "holdoutStreamSha256",
                                        "",
                                    ).take(16) +
                                    "… · JSON=" +
                                    status.optString(
                                        "jsonSha256",
                                        "",
                                    ).take(16) +
                                    "… · targetLeak=false · lens/device=false · writeback=false."
                            },
                            onFailure = {
                                "Universal Local Model Bank Holdout v0.1 export faalde: " +
                                    (it.message ?: it.javaClass.simpleName)
                            },
                        )
                    render()
                }
            }
            return
        }

        if (requestCode == REQUEST_SAVE_UNIVERSAL_MODEL_BANK_HOLDOUT_V02) {
            val expectedJob = pendingUniversalModelBankHoldoutV02JobId
            pendingUniversalModelBankHoldoutV02JobId = null
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                universalModelBankHoldoutV02Status =
                    "Universal Local Model Bank Holdout v0.2-export geannuleerd."
                render()
                return
            }

            val job = session.jobs.firstOrNull { it.id == expectedJob }
            val ready = previewState as? TilePreviewUiState.Ready
            if (
                expectedJob == null ||
                job == null ||
                ready == null ||
                ready.jobId != expectedJob ||
                activeJobId != expectedJob
            ) {
                universalModelBankHoldoutV02Status =
                    "Universal Local Model Bank Holdout v0.2 geblokkeerd: actieve sealed observation veranderde."
                render()
                return
            }

            val operationKey =
                backgroundOperationKey(
                    "universal-local-model-bank-holdout-v02",
                    expectedJob,
                )
            if (truthNegativeHeavyOperationActive(expectedJob, operationKey)) {
                universalModelBankHoldoutV02Status =
                    "Wacht op de andere zware D.RAW-analysetaak; Universal Local Model Bank Holdout v0.2 start daarna opnieuw handmatig."
                render()
                return
            }
            if (!startBackgroundOperation(
                    operationKey,
                    "Universal Local Model Bank Holdout v0.2",
                )
            ) {
                universalModelBankHoldoutV02Status =
                    "Universal Local Model Bank Holdout v0.2 achtergrondverwerking kon niet veilig starten."
                render()
                return
            }

            universalModelBankHoldoutV02Status =
                "Universal Local Model Bank Holdout v0.2 · full-resolution CFA holdouts + target-blinde modelselectie worden doorgerekend…"
            render()

            startGuardedBackgroundThread(
                name = "draw-universal-model-bank-v02-" + job.id.take(8),
                operationKey = operationKey,
                onUnexpected = {
                    universalModelBankHoldoutV02Status = it
                },
            ) {
                val exportResult = runCatching {
                    UniversalLocalModelBankHoldoutV02.exportSidecar(
                        resolver = contentResolver,
                        sourceUri = job.source.uri,
                        destinationUri = destination,
                        expectedSourceSha256 =
                            universalProfiles[expectedJob]
                                ?.optString("source_sha256", "")
                                .orEmpty(),
                    )
                }

                finishBackgroundOperation(
                    operationKey,
                    exportResult.isSuccess,
                    exportResult.fold(
                        onSuccess = {
                            "Universal Local Model Bank Holdout v0.2 gereed."
                        },
                        onFailure = {
                            "Universal Local Model Bank Holdout v0.2 faalde: " +
                                (it.message ?: it.javaClass.simpleName)
                        },
                    ),
                )

                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    universalModelBankHoldoutV02Status =
                        exportResult.fold(
                            onSuccess = { status ->
                                "Universal Local Model Bank Holdout v0.2 opgeslagen · " +
                                    status.optInt("width", 0) + "×" +
                                    status.optInt("height", 0) +
                                    " · " +
                                    formatBytes(status.optLong("fileBytes", 0L)) +
                                    " · holdouts=" +
                                    status.optLong("holdouts", 0L) +
                                    " · selected/baseline valid=" +
                                    status.optLong("selectedValid", 0L) + "/" +
                                    status.optLong("baselineValid", 0L) +
                                    " · lower-|error| selected/baseline=" +
                                    status.optLong(
                                        "selectedLowerAbsErrorThanBaseline",
                                        0L,
                                    ) + "/" +
                                    status.optLong(
                                        "baselineLowerAbsErrorThanSelected",
                                        0L,
                                    ) +
                                    " · holdout=" +
                                    status.optString(
                                        "holdoutStreamSha256",
                                        "",
                                    ).take(16) +
                                    "… · JSON=" +
                                    status.optString(
                                        "jsonSha256",
                                        "",
                                    ).take(16) +
                                    "… · targetLeak=false · lens/device=false · writeback=false."
                            },
                            onFailure = {
                                "Universal Local Model Bank Holdout v0.2 export faalde: " +
                                    (it.message ?: it.javaClass.simpleName)
                            },
                        )
                    render()
                }
            }
            return
        }

        if (requestCode == REQUEST_SAVE_UNIVERSAL_MODEL_BANK_HOLDOUT_V03) {
            val expectedJob = pendingUniversalModelBankHoldoutV03JobId
            pendingUniversalModelBankHoldoutV03JobId = null
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                universalModelBankHoldoutV03Status =
                    "Universal Local Model Bank Holdout v0.3-export geannuleerd."
                render()
                return
            }

            val job = session.jobs.firstOrNull { it.id == expectedJob }
            val ready = previewState as? TilePreviewUiState.Ready
            if (
                expectedJob == null ||
                job == null ||
                ready == null ||
                ready.jobId != expectedJob ||
                activeJobId != expectedJob
            ) {
                universalModelBankHoldoutV03Status =
                    "Universal Local Model Bank Holdout v0.3 geblokkeerd: actieve sealed observation veranderde."
                render()
                return
            }

            val operationKey =
                backgroundOperationKey(
                    "universal-local-model-bank-holdout-v03",
                    expectedJob,
                )
            if (truthNegativeHeavyOperationActive(expectedJob, operationKey)) {
                universalModelBankHoldoutV03Status =
                    "Wacht op de andere zware D.RAW-analysetaak; Universal Local Model Bank Holdout v0.3 start daarna opnieuw handmatig."
                render()
                return
            }
            if (!startBackgroundOperation(
                    operationKey,
                    "Universal Local Model Bank Holdout v0.3",
                )
            ) {
                universalModelBankHoldoutV03Status =
                    "Universal Local Model Bank Holdout v0.3 achtergrondverwerking kon niet veilig starten."
                render()
                return
            }

            universalModelBankHoldoutV03Status =
                "Universal Local Model Bank Holdout v0.3 · full-resolution CFA holdouts + target-blinde modelselectie worden doorgerekend…"
            render()

            startGuardedBackgroundThread(
                name = "draw-universal-model-bank-v03-" + job.id.take(8),
                operationKey = operationKey,
                onUnexpected = {
                    universalModelBankHoldoutV03Status = it
                },
            ) {
                val exportResult = runCatching {
                    UniversalLocalModelBankHoldoutV03.exportSidecar(
                        resolver = contentResolver,
                        sourceUri = job.source.uri,
                        destinationUri = destination,
                        expectedSourceSha256 =
                            universalProfiles[expectedJob]
                                ?.optString("source_sha256", "")
                                .orEmpty(),
                    )
                }

                finishBackgroundOperation(
                    operationKey,
                    exportResult.isSuccess,
                    exportResult.fold(
                        onSuccess = {
                            "Universal Local Model Bank Holdout v0.3 gereed."
                        },
                        onFailure = {
                            "Universal Local Model Bank Holdout v0.3 faalde: " +
                                (it.message ?: it.javaClass.simpleName)
                        },
                    ),
                )

                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    universalModelBankHoldoutV03Status =
                        exportResult.fold(
                            onSuccess = { status ->
                                "Universal Local Model Bank Holdout v0.3 opgeslagen · " +
                                    status.optInt("width", 0) + "×" +
                                    status.optInt("height", 0) +
                                    " · " +
                                    formatBytes(status.optLong("fileBytes", 0L)) +
                                    " · holdouts=" +
                                    status.optLong("holdouts", 0L) +
                                    " · selected/baseline valid=" +
                                    status.optLong("selectedValid", 0L) + "/" +
                                    status.optLong("baselineValid", 0L) +
                                    " · lower-|error| selected/baseline=" +
                                    status.optLong(
                                        "selectedLowerAbsErrorThanBaseline",
                                        0L,
                                    ) + "/" +
                                    status.optLong(
                                        "baselineLowerAbsErrorThanSelected",
                                        0L,
                                    ) +
                                    " · holdout=" +
                                    status.optString(
                                        "holdoutStreamSha256",
                                        "",
                                    ).take(16) +
                                    "… · JSON=" +
                                    status.optString(
                                        "jsonSha256",
                                        "",
                                    ).take(16) +
                                    "… · targetLeak=false · lens/device=false · writeback=false."
                            },
                            onFailure = {
                                "Universal Local Model Bank Holdout v0.3 export faalde: " +
                                    (it.message ?: it.javaClass.simpleName)
                            },
                        )
                    render()
                }
            }
            return
        }

        if (requestCode == REQUEST_SAVE_APPEARANCE_HIGHLIGHT_DETAIL) {
            val expectedJob = pendingAppearanceHighlightDetailJobId
            pendingAppearanceHighlightDetailJobId = null
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                appearanceHighlightDetailStatus =
                    "Appearance Highlight Detail v0.1-export geannuleerd."
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
                appearanceHighlightDetailStatus =
                    "Appearance Highlight Detail v0.1 geblokkeerd: actieve Scientific Master-route veranderde."
                render()
                return
            }

            val operationKey =
                backgroundOperationKey(
                    "truthnegative-appearance-highlight-detail",
                    expectedJob,
                )
            if (truthNegativeHeavyOperationActive(expectedJob, operationKey)) {
                appearanceHighlightDetailStatus =
                    "Wacht op de andere D.RAWnegative/Camera-5 analysetaak. " +
                        "Appearance Highlight Detail v0.1 start daarna opnieuw handmatig."
                render()
                return
            }
            if (!startBackgroundOperation(
                    operationKey,
                    "Appearance Highlight Detail v0.1",
                )
            ) {
                appearanceHighlightDetailStatus =
                    "Appearance Highlight Detail v0.1 achtergrondverwerking kon niet veilig starten."
                render()
                return
            }

            appearanceHighlightDetailStatus =
                "Appearance Highlight Detail v0.1 · meet scene→display peak-collapse " +
                    "op exact dezelfde PRO Appearance v0.7-route; geen writeback…"
            render()

            startGuardedBackgroundThread(
                name = "draw-appearance-highlight-" + job.id.take(8),
                operationKey = operationKey,
                onUnexpected = {
                    appearanceHighlightDetailStatus = it
                },
            ) {
                val exportResult =
                    TruthNegativeAppearanceHighlightDetailExporter.export(
                        contentResolver,
                        job,
                        destination,
                    )
                finishBackgroundOperation(
                    operationKey,
                    exportResult is
                        TruthNegativeAppearanceHighlightDetailResult.Success,
                    when (exportResult) {
                        is TruthNegativeAppearanceHighlightDetailResult.Success ->
                            "Appearance Highlight Detail v0.1 opgeslagen + SHA geverifieerd."
                        is TruthNegativeAppearanceHighlightDetailResult.Failed ->
                            exportResult.reason
                    },
                )
                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    appearanceHighlightDetailStatus =
                        when (exportResult) {
                            is TruthNegativeAppearanceHighlightDetailResult.Failed ->
                                exportResult.reason
                            is TruthNegativeAppearanceHighlightDetailResult.Success -> {
                                val m = exportResult.metrics
                                "Appearance Highlight Detail v0.1 opgeslagen · " +
                                    m.width + "×" + m.height +
                                    " · " + formatBytes(m.fileBytes) +
                                    " · >refWhite=" +
                                    m.sourceAboveReferenceWhite + "/" +
                                    m.sampleCount +
                                    " · atPeak=" + m.mappedAtPeak +
                                    " · collapsed-pairs=" +
                                    m.peakCollapsedDistinctAdjacentPairs +
                                    "/" + m.sourceDistinctAdjacentPairs +
                                    " · bright-collapsed=" +
                                    m.brightPeakCollapsedDistinctAdjacentPairs +
                                    " · source-censored=" +
                                    m.sourceCensored +
                                    " · headroom=" +
                                    (if (m.noHighlightHeadroom) "0 nit" else "aanwezig") +
                                    " · AH=" +
                                    m.auditSha256.take(16) + "…"
                            }
                        }
                    render()
                }
            }
            return
        }

        if (requestCode == REQUEST_SAVE_APPEARANCE_HEADROOM_SWEEP) {
            val expectedJob = pendingAppearanceHeadroomSweepJobId
            pendingAppearanceHeadroomSweepJobId = null
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                appearanceHeadroomSweepStatus =
                    "Appearance Headroom Sweep v0.2-export geannuleerd."
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
                appearanceHeadroomSweepStatus =
                    "Appearance Headroom Sweep v0.2 geblokkeerd: actieve Scientific Master-route veranderde."
                render()
                return
            }

            val operationKey =
                backgroundOperationKey(
                    "truthnegative-appearance-headroom-sweep",
                    expectedJob,
                )
            if (truthNegativeHeavyOperationActive(expectedJob, operationKey)) {
                appearanceHeadroomSweepStatus =
                    "Wacht op de andere D.RAWnegative/Camera-5 analysetaak. " +
                        "Appearance Headroom Sweep v0.2 start daarna opnieuw handmatig."
                render()
                return
            }
            if (!startBackgroundOperation(
                    operationKey,
                    "Appearance Headroom Sweep v0.2",
                )
            ) {
                appearanceHeadroomSweepStatus =
                    "Appearance Headroom Sweep v0.2 achtergrondverwerking kon niet veilig starten."
                render()
                return
            }

            appearanceHeadroomSweepStatus =
                "Appearance Headroom Sweep v0.2 · 100/100, 90/100, 80/100 en 70/100 " +
                    "door exact dezelfde PRO Appearance v0.7-route; geen writeback…"
            render()

            startGuardedBackgroundThread(
                name = "draw-appearance-headroom-" + job.id.take(8),
                operationKey = operationKey,
                onUnexpected = {
                    appearanceHeadroomSweepStatus = it
                },
            ) {
                val exportResult =
                    TruthNegativeAppearanceHeadroomSweepExporter.export(
                        contentResolver,
                        job,
                        destination,
                    )
                finishBackgroundOperation(
                    operationKey,
                    exportResult is
                        TruthNegativeAppearanceHeadroomSweepResult.Success,
                    when (exportResult) {
                        is TruthNegativeAppearanceHeadroomSweepResult.Success ->
                            "Appearance Headroom Sweep v0.2 opgeslagen + SHA geverifieerd."
                        is TruthNegativeAppearanceHeadroomSweepResult.Failed ->
                            exportResult.reason
                    },
                )
                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    appearanceHeadroomSweepStatus =
                        when (exportResult) {
                            is TruthNegativeAppearanceHeadroomSweepResult.Failed ->
                                exportResult.reason
                            is TruthNegativeAppearanceHeadroomSweepResult.Success -> {
                                val m = exportResult.metrics
                                val summary = m.variants.joinToString(" · ") { v ->
                                    v.id.removePrefix("baseline_")
                                        .removePrefix("shoulder_") +
                                        ": collapse=" + v.collapsedPairs +
                                        ", grad=" +
                                        String.format("%.4f", v.gradientRetention) +
                                        ", below-knee-changed=" +
                                        v.belowKneeChanged
                                }
                                "Appearance Headroom Sweep v0.2 opgeslagen · " +
                                    m.width + "×" + m.height +
                                    " · " + formatBytes(m.fileBytes) +
                                    " · " + summary +
                                    " · winner=geen · HS=" +
                                    m.sweepSha256.take(16) + "…"
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
                    "D.RAWnegative legacy .tnc-export geannuleerd."
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
                    "Wacht op de andere D.RAWnegative/Camera-5 analysetaak. " +
                        "Native export start daarna opnieuw handmatig."
                render()
                return
            }
            if (!startBackgroundOperation(
                    operationKey,
                    "D.RAWnegative · legacy .tnc export + import verify",
                )
            ) {
                truthNegativeNativeContainerStatus =
                    "Native container achtergrondverwerking kon niet veilig starten."
                render()
                return
            }

            truthNegativeNativeContainerStatus =
                "D.RAWnegative bindt de nieuwe state; de legacy .tnc schrijft Float32 Scientific Master-values + " +
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
                            "D.RAWnegative legacy .tnc export/import geverifieerd."
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
                                "D.RAWnegative legacy .tnc opgeslagen + teruggelezen · " +
                                    m.width + "×" + m.height +
                                    " · " + formatBytes(m.fileBytes) +
                                    " · tiles=" + m.tileCount +
                                    " · records=" + m.recordCount +
                                    " · D.RAWnegative=" +
                                    m.drawNegativeStateSha256.take(16) + "…" +
                                    " · authority C/R/X/U=" +
                                    m.authorityCalibratedEstimate + "/" +
                                    m.authorityReconstructed + "/" +
                                    m.authorityCensored + "/" +
                                    m.authorityUnknown +
                                    " · censored RGB=" +
                                    m.censoredR + "/" + m.censoredG + "/" + m.censoredB +
                                    " · censored >1/≤1=" +
                                    m.censoredValueAboveOne + "/" +
                                    m.censoredValueAtOrBelowOne +
                                    " · censor tiles=" + m.censoredTileCount +
                                    " · censor bbox=" + m.censoredMinX + "," + m.censoredMinY +
                                    "–" + m.censoredMaxX + "," + m.censoredMaxY +
                                    " · RAW bound=" + m.censoredRawCodeBoundMin +
                                    "…" + m.censoredRawCodeBoundMax +
                                    " · bound mismatch=" + m.censoredRawCodeBoundMismatchCount +
                                    " · CFA 00/10/01/11=" +
                                    m.censoredParity00 + "/" + m.censoredParity10 + "/" +
                                    m.censoredParity01 + "/" + m.censoredParity11 +
                                    " · RAW10=1023=" + m.censoredRaw10MaxCount +
                                    " (" + m.censoredRaw10Parity00 + "/" +
                                    m.censoredRaw10Parity10 + "/" +
                                    m.censoredRaw10Parity01 + "/" +
                                    m.censoredRaw10Parity11 + ")" +
                                    " · values <0/>1=" +
                                    m.valueNegativeCount + "/" +
                                    m.valueAboveOneCount +
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
                truthNegativeStatus = "Legacy TruthNegative TN-4-export geannuleerd."
                render()
                return
            }
            val job = session.jobs.firstOrNull { it.id == expectedJob }
            val ready = previewState as? TilePreviewUiState.Ready
            if (expectedJob == null || job == null || ready == null ||
                ready.jobId != expectedJob || activeJobId != expectedJob
            ) {
                truthNegativeStatus =
                    "Legacy TruthNegative TN-4-export geblokkeerd: actieve Scientific Master-route veranderde."
                render()
                return
            }

            val operationKey = backgroundOperationKey("truthnegative", expectedJob)
            if (!startBackgroundOperation(operationKey, "Legacy TruthNegative TN-4 opbouwen")) {
                truthNegativeStatus = "Legacy TruthNegative TN-4 achtergrondverwerking kon niet veilig starten."
                render()
                return
            }
            truthNegativeStatus =
                "Legacy TruthNegative TN-4 wordt opgebouwd… exact Master replay + Dynamic Authority + canonical Open Scene v0.70."
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
                        is TruthNegativeExportResult.Success -> "Legacy TruthNegative TN-4 gereed."
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
                            "Legacy TruthNegative TN-4 opgeslagen + teruggelezen · ${m.width}×${m.height} · " +
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

        if (requestCode == REQUEST_OPEN_CALIBRATION_OBSERVATION_RECORDS) {
            if (resultCode != RESULT_OK || data == null) {
                calibrationObservationRecordStatus =
                    "Calibration Observation Record-import geannuleerd."
                render()
                return
            }
            calibrationObservationRecordStatus =
                importCalibrationObservationRecords(data)
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
        ResearchUniversalProfileStoreV01.clear(
            filesDir,
        )
        universalProfiles.clear()
        universalProfileErrors.clear()
        universalProfileLoading.clear()
        universalProfileCompletionWaiters.clear()
        session = session.withJobs(jobs)
        researchWorkbenchSessionRestoreStatus = null
        persistResearchWorkbenchSession()
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
            if (
                first.source.format.nativeProcessingReady &&
                (!researchWorkbenchMode || session.jobs.size <= 1)
            ) {
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
        clearN2AppearanceCandidate()
        clearN2CropAb()
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
        if (!researchWorkbenchMode || session.jobs.size <= 1) {
            requestUniversalProfile(job)
        }
        render()
    }

    private fun requestUniversalProfile(
        job: RawJob,
        force: Boolean = false,
        onComplete: ((Boolean) -> Unit)? = null,
    ) {
        val jobId = job.id

        if (
            !force &&
            universalProfiles.containsKey(
                jobId,
            )
        ) {
            onComplete?.invoke(true)
            return
        }

        if (
            universalProfileLoading.contains(
                jobId,
            )
        ) {
            onComplete?.let { callback ->
                universalProfileCompletionWaiters
                    .getOrPut(jobId) {
                        mutableListOf()
                    }
                    .add(callback)
            }
            return
        }

        onComplete?.let { callback ->
            universalProfileCompletionWaiters
                .getOrPut(jobId) {
                    mutableListOf()
                }
                .add(callback)
        }

        universalProfiles.remove(jobId)
        universalProfileErrors.remove(jobId)
        universalProfileLoading.add(jobId)
        render()

        Thread({
            val result =
                runCatching {
                    UniversalSourceProfiler.profile(
                        contentResolver,
                        job.source,
                        cacheDir,
                    )
                }
            runOnUiThread {
                universalProfileLoading.remove(
                    jobId,
                )
                var success = false
                result.onSuccess { profile ->
                    universalProfiles[jobId] =
                        profile
                    ResearchUniversalProfileStoreV01.save(
                        filesDir = filesDir,
                        job = job,
                        profile = profile,
                    )
                    universalProfileErrors.remove(
                        jobId,
                    )
                    success = true
                }.onFailure { error ->
                    universalProfiles.remove(
                        jobId,
                    )
                    ResearchUniversalProfileStoreV01.remove(
                        filesDir = filesDir,
                        jobId = jobId,
                    )
                    universalProfileErrors[jobId] =
                        error.message
                            ?: error.javaClass.simpleName
                }

                val waiters =
                    universalProfileCompletionWaiters
                        .remove(jobId)
                        .orEmpty()
                for (callback in waiters) {
                    callback(success)
                }
                render()
            }
        }, "draw-universal-intake-" + jobId.take(8)).start()
    }

    private fun universalIntakePane(job: RawJob): View = card().apply {
        addView(label("Universele ingang · achterkant + voorkant", 16f, bold = true))
        addView(label(
            "D.RAW leest dezelfde verzegelde bron parallel als technisch bestand en als zichtbaar beeld. " +
                "De voorkant gebruikt alleen deterministische, inspecteerbare computer vision; geen AI/ML.",
            11f,
            muted = true,
        ))
        addView(space(6))

        when {
            universalProfileLoading.contains(job.id) -> {
                addView(horizontal().apply {
                    gravity = Gravity.CENTER_VERTICAL
                    addView(
                        ProgressBar(this@MainActivity).apply { isIndeterminate = true },
                        LinearLayout.LayoutParams(dp(28), dp(28)).apply { marginEnd = dp(8) },
                    )
                    addView(label("Universele bronkennis wordt opgebouwd…", 11f, muted = true))
                })
            }
            universalProfileErrors[job.id] != null -> {
                addView(label(
                    "Universele intake kon deze bron nog niet volledig lezen: " +
                        universalProfileErrors[job.id],
                    11f,
                    muted = true,
                ))
                addView(space(5))
                addView(actionButton("Opnieuw analyseren") { requestUniversalProfile(job, force = true) })
            }
            universalProfiles[job.id] != null -> {
                val profile = universalProfiles.getValue(job.id)
                val meta = profile.optJSONObject("source_metadata") ?: JSONObject()
                val raster = profile.optJSONObject("primary_raw_raster") ?: JSONObject()
                val scene = profile.optJSONObject("scene_analysis") ?: JSONObject()
                val stats = scene.optJSONObject("appearance_statistics") ?: JSONObject()
                val readiness = scene.optJSONObject("geometry_readiness") ?: JSONObject()
                val darkChroma =
                    scene.optJSONObject("dark_chroma_stability_v0_1") ?: JSONObject()
                val darkChromaGlobal =
                    darkChroma.optJSONObject("global") ?: JSONObject()
                val singleObservation =
                    darkChroma.optJSONObject("single_observation_contract") ?: JSONObject()
                val darkChromaV02 =
                    scene.optJSONObject("dark_chroma_stability_v0_2") ?: JSONObject()
                val darkChromaV02Global =
                    darkChromaV02.optJSONObject("global") ?: JSONObject()
                val darkChromaV02Backside =
                    darkChromaV02.optJSONObject("backside_support") ?: JSONObject()
                val backsideSignal =
                    profile.optJSONObject("backside_signal_support") ?: JSONObject()
                val backsideSignalGlobal =
                    backsideSignal.optJSONObject("global") ?: JSONObject()
                val darkChromaV03 =
                    scene.optJSONObject("dark_chroma_stability_v0_3") ?: JSONObject()
                val darkChromaV03Global =
                    darkChromaV03.optJSONObject("global") ?: JSONObject()
                val darkChromaV03Factors =
                    darkChromaV03.optJSONObject("degeneracy_factors") ?: JSONObject()
                val n2LocalBinding =
                    profile.optJSONObject("n2_local_spatial_binding") ?: JSONObject()
                val n2LocalGlobal =
                    n2LocalBinding.optJSONObject("global") ?: JSONObject()
                val darkChromaV04 =
                    scene.optJSONObject("dark_chroma_stability_v0_4") ?: JSONObject()
                val darkChromaV04Global =
                    darkChromaV04.optJSONObject("global") ?: JSONObject()
                val n2StructureSupport =
                    profile.optJSONObject("n2_structure_support_binding") ?: JSONObject()
                val n2StructureGlobal =
                    n2StructureSupport.optJSONObject("global") ?: JSONObject()
                val darkChromaV05 =
                    scene.optJSONObject("dark_chroma_stability_v0_5") ?: JSONObject()
                val darkChromaV05Global =
                    darkChromaV05.optJSONObject("global") ?: JSONObject()
                val n2SupportDistance =
                    profile.optJSONObject("n2_sample_support_distance") ?: JSONObject()
                val n2SupportDistanceGlobal =
                    n2SupportDistance.optJSONObject("global") ?: JSONObject()
                val darkChromaV06 =
                    scene.optJSONObject("dark_chroma_stability_v0_6") ?: JSONObject()
                val darkChromaV06Global =
                    darkChromaV06.optJSONObject("global") ?: JSONObject()
                val sampleLattice =
                    profile.optJSONObject("raster_independent_sample_lattice") ?: JSONObject()
                val n2LatticeGeometry =
                    profile.optJSONObject("n2_raster_independent_sample_geometry") ?: JSONObject()
                val darkChromaV07 =
                    scene.optJSONObject("dark_chroma_stability_v0_7") ?: JSONObject()
                val anchorReconstruction =
                    profile.optJSONObject(
                        "anchor_constrained_local_reconstruction",
                    ) ?: JSONObject()
                val anchorReconstructionGlobal =
                    anchorReconstruction.optJSONObject("global") ?: JSONObject()
                val observationModelSelection =
                    profile.optJSONObject(
                        "universal_observation_model_selection",
                    ) ?: JSONObject()

                val sourceClass = profile.optString("scientific_source_class", "UNKNOWN")
                val width = raster.opt("width")?.toString() ?: "?"
                val height = raster.opt("height")?.toString() ?: "?"
                val focal = meta.opt("focal_length_mm")?.toString() ?: "?"
                val iso = meta.opt("iso")?.toString() ?: "?"
                addView(label(
                    "Achterkant · class=" + sourceClass + " · raster=" + width + "×" + height +
                        " · focal=" + focal + "mm · ISO=" + iso,
                    10.5f,
                    muted = true,
                ))

                val observationOpticalFieldChart =
                    profile.optJSONObject(
                        "observation_optical_field_chart",
                    )
                if (
                    observationOpticalFieldChart?.optString("status") ==
                    "FIELD_CHART_AVAILABLE"
                ) {
                    addView(space(5))
                    addView(actionButton(
                        "Export Observation Optical Field Chart v0.1 · JSON",
                    ) {
                        launchObservationOpticalFieldExport(job)
                    })
                    observationOpticalFieldStatus?.let { status ->
                        addView(label(status, 10f, muted = true))
                    }
                    addView(label(
                        "Universele read-only veldkaart van deze sealed observation: bron/ActiveArea → rho + " +
                            "azimut + radiale/tangentiële basis, plus gemeten CFA-signaal per ring/sector. " +
                            "Beschikbaar vanuit Research ongeacht PURE/ADVANCED/PRO en zonder Finalized Preview. " +
                            "DNG GainMap blijft provenance-hint; geen lensprofiel, correctiegain of writeback.",
                        10f,
                        muted = true,
                    ))
                }

                val universalCalibrationAtlas =
                    profile.optJSONObject(
                        "universal_observation_calibration_atlas",
                    )
                if (
                    universalCalibrationAtlas?.optString("status") ==
                    "OBSERVATION_ATLAS_AVAILABLE"
                ) {
                    addView(space(5))
                    addView(actionButton(
                        "Export Universal Observation & Calibration Atlas v0.1 · JSON",
                    ) {
                        launchUniversalCalibrationAtlasExport(job)
                    })
                    universalCalibrationAtlasStatus?.let { status ->
                        addView(label(status, 10f, muted = true))
                    }
                    addView(label(
                        "Deze atlas-export hoort bij de Universele Ingang zelf en blijft beschikbaar als de " +
                            "wetenschappelijke RAW-decoder fail-closed stopt. Geen Scientific Preview, DNG-route, " +
                            "camera-/lensidentiteit of voorafgaande gebruikerskalibratie is vereist.",
                        10f,
                        muted = true,
                    ))
                }

                val bits = raster.opt("bits_per_sample")?.toString() ?: "?"
                val compression = raster.opt("compression")?.toString() ?: "?"
                val sampleFormat = raster.opt("sample_format")?.toString() ?: "?"
                val spp = raster.opt("samples_per_pixel")?.toString() ?: "?"
                val storageKind = raster.optString("storage_kind", "UNKNOWN")
                val cfa = raster.opt("cfa_pattern")?.toString() ?: "?"
                val opcodeList2 = raster.optBoolean("opcode_list_2_present", false)
                val black = raster.opt("black_level")?.toString() ?: "?"
                val white = raster.opt("white_level")?.toString() ?: "?"
                addView(label(
                    "RAW-opslag · bits=" + bits +
                        " · compression=" + compression +
                        " · sampleFormat=" + sampleFormat +
                        " · spp=" + spp +
                        " · storage=" + storageKind +
                        " · CFA=" + cfa +
                        " · OpcodeList2=" + opcodeList2,
                    10f,
                    muted = true,
                ))
                addView(label(
                    "RAW-niveaus · black=" + black + " · white=" + white,
                    10f,
                    muted = true,
                ))

                if (sampleLattice.optString("status") == "AVAILABLE") {
                    addView(space(4))
                    addView(label(
                        "D.RAW Sample Lattice v0.1 · RASTER-INDEPENDENT · source=" +
                            sampleLattice.opt("source_width") + "×" +
                            sampleLattice.opt("source_height") +
                            " · units/source-pixel=" +
                            sampleLattice.optLong(
                                "coordinate_units_per_source_pixel",
                                0L,
                            ) +
                            " · measured anchors=" +
                            sampleLattice.opt("measured_anchor_count") +
                            " · dense=false · upscaling=false",
                        10.5f,
                        muted = true,
                    ))
                    addView(label(
                        "De verzegelde bronmetingen blijven exact op hun eigen ankerposities. " +
                            "De fijnere lattice is alleen een vrije wetenschappelijke coördinatenwereld: " +
                            "tussenposities starten UNKNOWN, krijgen geen verzonnen pixelwaarde en verhogen " +
                            "de optische bronresolutie niet. Het bronraster bepaalt waar gemeten is, niet waar D.RAW mag rekenen.",
                        10f,
                        muted = true,
                    ))
                    addView(label(
                        "Originele resolutie is NIET alleen een noise-raster: zij blijft de full-resolution " +
                            "MEASURED steun voor CFA-waarden, detail, structuur, geometrie en authority. " +
                            "De oplossingsruimte en uiteindelijke projectieresolutie mogen daarvan losstaan.",
                        10f,
                        muted = true,
                    ))
                }

                if (scene.optBoolean("decoded_preview_used", false)) {
                    val aw = scene.optInt("analysis_width", 0)
                    val ah = scene.optInt("analysis_height", 0)
                    val edge = stats.optDouble("edge_density", Double.NaN)
                    val entropy = stats.optDouble("luma_entropy_bits_32_bin", Double.NaN)
                    addView(label(
                        "Voorkant · " + aw + "×" + ah + " inspectie · edgeDensity=" +
                            (if (edge.isFinite()) "%.4f".format(edge) else "?") +
                            " · entropy=" +
                            (if (entropy.isFinite()) "%.2f".format(entropy) else "?") +
                            " · naturalGeometryCandidate=" +
                            readiness.optBoolean("natural_feature_geometry_candidate", false),
                        10.5f,
                        muted = true,
                    ))
                } else {
                    addView(label(
                        "Voorkant · preview niet direct beschikbaar; bron blijft geldig en UNKNOWN is toegestaan.",
                        10.5f,
                        muted = true,
                    ))
                }

                if (darkChroma.optString("status") == "AUDIT_ONLY_AVAILABLE") {
                    val tileCount = darkChromaGlobal.optLong("tile_count", 0L)
                    val darkTiles = darkChromaGlobal.optLong("dark_tile_count", 0L)
                    val flatDark = darkChromaGlobal.optLong("flat_dark_tile_count", 0L)
                    val candidates =
                        darkChromaGlobal.optLong(
                            "frontside_chroma_instability_candidate_tiles",
                            0L,
                        )
                    val structure =
                        darkChromaGlobal.optLong("structure_protected_tiles", 0L)
                    val auditSha = darkChroma.optString("audit_sha256", "")
                    addView(space(4))
                    addView(label(
                        "Dark Chroma Stability v0.1 · AUDIT ONLY · tiles=" + tileCount +
                            " · dark=" + darkTiles +
                            " · flat-dark=" + flatDark +
                            " · chroma-candidates=" + candidates +
                            " · structure-veto=" + structure +
                            " · audit=" +
                            (if (auditSha.length >= 16) auditSha.take(16) + "…" else auditSha),
                        10.5f,
                        muted = true,
                    ))
                    addView(label(
                        "Single observation · sourceCount=" +
                            singleObservation.optInt("source_observation_count", 0) +
                            " · otherLenses=" +
                            singleObservation.optBoolean("other_physical_lenses_used", true) +
                            " · temporalFrames=" +
                            singleObservation.optBoolean("temporal_frames_used", true) +
                            " · one D.RAWnegative diagnostic binding · candidateApplied=false.",
                        10f,
                        muted = true,
                    ))
                    addView(label(
                        "Frontside mag alleen structuur beschermen en chroma-instabiliteit aanwijzen. " +
                            "Geen vervangkleur, geen sensor-noise claim en geen Scientific-Master-writeback; " +
                            "backside/noise-evidence blijft vereist voor iedere latere correctie.",
                        10f,
                        muted = true,
                    ))
                }

                if (
                    backsideSignal.optString("status") ==
                    "MEASURED_SOURCE_PAYLOAD_SAMPLE_AVAILABLE"
                ) {
                    val p50 =
                        backsideSignalGlobal.optDouble(
                            "p50_normalized_above_black",
                            Double.NaN,
                        )
                    val p90 =
                        backsideSignalGlobal.optDouble(
                            "p90_normalized_above_black",
                            Double.NaN,
                        )
                    val p99 =
                        backsideSignalGlobal.optDouble(
                            "p99_normalized_above_black",
                            Double.NaN,
                        )
                    val nearBlack =
                        backsideSignalGlobal.optDouble("fraction_le_0_01", Double.NaN)
                    addView(space(4))
                    addView(label(
                        "Backside Signal Support v0.1 · SOURCE PAYLOAD · state=" +
                            backsideSignal.optString("signal_support_state", "UNKNOWN") +
                            " · samples=" + backsideSignal.optInt("sample_count", 0) +
                            " · p50/p90/p99=" +
                            (if (p50.isFinite()) "%.5f".format(p50) else "?") + "/" +
                            (if (p90.isFinite()) "%.5f".format(p90) else "?") + "/" +
                            (if (p99.isFinite()) "%.5f".format(p99) else "?") +
                            " · frac≤0.01=" +
                            (if (nearBlack.isFinite()) "%.3f".format(nearBlack) else "?"),
                        10f,
                        muted = true,
                    ))
                    addView(label(
                        "Meet direct uit geselecteerde DNG-CFA payload t.o.v. Black/White. " +
                            "Geen clamp, geen ADC-claim, geen correctie-enable; alleen een conservatieve blocker.",
                        10f,
                        muted = true,
                    ))
                } else {
                    addView(space(4))
                    addView(label(
                        "Backside Signal Support v0.1 · UNKNOWN/fail-closed · reason=" +
                            backsideSignal.optString("reason", "niet beschikbaar") +
                            ". Geen signaalclaim uit onbekende topology.",
                        10f,
                        muted = true,
                    ))
                }

                if (darkChromaV02.optString("status") == "AUDIT_ONLY_AVAILABLE") {
                    val infoState =
                        darkChromaV02.optString("global_information_state", "UNKNOWN")
                    val visible =
                        darkChromaV02Global.optLong("visible_chroma_instability_tiles", 0L)
                    val uninformative =
                        darkChromaV02Global.optLong("dark_uninformative_tiles", 0L)
                    val pending =
                        darkChromaV02Global.optLong(
                            "backside_confirmation_pending_tiles",
                            0L,
                        )
                    val supported =
                        darkChromaV02Global.optLong(
                            "chroma_correction_supported_tiles",
                            0L,
                        )
                    val v02Audit = darkChromaV02.optString("audit_sha256", "")
                    addView(space(4))
                    addView(label(
                        "Dark Chroma Stability v0.2 · INFORMATION GATE · state=" +
                            infoState +
                            " · visible-instability=" + visible +
                            " · dark-uninformative=" + uninformative +
                            " · backside-pending=" + pending +
                            " · correction-supported=" + supported +
                            " · audit=" +
                            (if (v02Audit.length >= 16) v02Audit.take(16) + "…" else v02Audit),
                        10.5f,
                        muted = true,
                    ))
                    addView(label(
                        "Backside hint · NoiseProfile=" +
                            darkChromaV02Backside.optBoolean("noise_profile_present", false) +
                            " · localNoiseBound=" +
                            darkChromaV02Backside.optBoolean(
                                "local_noise_confirmation_available",
                                false,
                            ) +
                            ". Metadata alleen promoveert geen correctie.",
                        10f,
                        muted = true,
                    ))
                    addView(label(
                        "v0.2 wet: DARK_UNINFORMATIVE = geen verborgen kleur reconstrueren. " +
                            "CHROMA_CORRECTION_SUPPORTED blijft onmogelijk totdat dezelfde observation " +
                            "lokale backside/N2-support heeft; candidateApplied=false.",
                        10f,
                        muted = true,
                    ))
                }

                if (darkChromaV03.optString("status") == "AUDIT_ONLY_AVAILABLE") {
                    val state =
                        darkChromaV03.optString("global_information_state", "UNKNOWN")
                    val visible =
                        darkChromaV03Global.optLong("visible_chroma_instability_tiles", 0L)
                    val uninformative =
                        darkChromaV03Global.optLong("dark_uninformative_tiles", 0L)
                    val pending =
                        darkChromaV03Global.optLong(
                            "backside_confirmation_pending_tiles",
                            0L,
                        )
                    val supported =
                        darkChromaV03Global.optLong(
                            "chroma_correction_supported_tiles",
                            0L,
                        )
                    val darkFraction =
                        darkChromaV03Factors.optDouble("dark_tile_fraction", Double.NaN)
                    val candidateFraction =
                        darkChromaV03Factors.optDouble(
                            "visible_candidate_fraction",
                            Double.NaN,
                        )
                    val structureFraction =
                        darkChromaV03Factors.optDouble(
                            "structure_protected_fraction",
                            Double.NaN,
                        )
                    val edge =
                        darkChromaV03Factors.optDouble("edge_density", Double.NaN)
                    addView(space(4))
                    addView(label(
                        "Dark Chroma Stability v0.3 · DEGENERACY + BACKSIDE GATE · state=" +
                            state +
                            " · degenerate=" +
                            darkChromaV03.optBoolean("frontside_degenerate", false) +
                            " · backsideNearBlack=" +
                            darkChromaV03.optBoolean("backside_near_black_dominated", false) +
                            " · visible=" + visible +
                            " · dark-uninformative=" + uninformative +
                            " · backside-pending=" + pending +
                            " · correction-supported=" + supported,
                        10.5f,
                        muted = true,
                    ))
                    addView(label(
                        "Degeneracy factors · dark=" +
                            (if (darkFraction.isFinite()) "%.3f".format(darkFraction) else "?") +
                            " · candidate=" +
                            (if (candidateFraction.isFinite()) "%.3f".format(candidateFraction) else "?") +
                            " · structure=" +
                            (if (structureFraction.isFinite()) "%.3f".format(structureFraction) else "?") +
                            " · edgeDensity=" +
                            (if (edge.isFinite()) "%.4f".format(edge) else "?") +
                            " · entropy is alleen diagnostiek, geen harde poort.",
                        10f,
                        muted = true,
                    ))
                    addView(label(
                        "v0.3 wet: een gedegenereerde bijna-zwarte frontside of gemeten near-black backside " +
                            "mag alleen blokkeren. Geen verborgen kleur, geen private A/B/Δ en geen " +
                            "CHROMA_CORRECTION_SUPPORTED tot lokale N2/backside-binding bestaat.",
                        10f,
                        muted = true,
                    ))
                }

                if (
                    n2LocalBinding.optString("status") ==
                    "AUDIT_ONLY_BINDING_AVAILABLE"
                ) {
                    addView(space(4))
                    addView(label(
                        "N2 Local Spatial Binding v0.1 · SAME OBSERVATION · frontside-bound=" +
                            n2LocalGlobal.optLong("bound_frontside_tiles", 0L) + "/" +
                            n2LocalGlobal.optLong("frontside_tile_count", 0L) +
                            " · visible-bound=" +
                            n2LocalGlobal.optLong("visible_candidate_bound_tiles", 0L) +
                            " · structure-blocked=" +
                            n2LocalGlobal.optLong(
                                "visible_candidate_structure_blocked_tiles",
                                0L,
                            ) +
                            " · censor-blocked=" +
                            n2LocalGlobal.optLong(
                                "visible_candidate_censor_blocked_tiles",
                                0L,
                            ) +
                            " · all-predictable=" +
                            n2LocalGlobal.optLong(
                                "visible_candidate_all_predictable_tiles",
                                0L,
                            ) +
                            " · center-outlier-free=" +
                            n2LocalGlobal.optLong(
                                "visible_candidate_center_outlier_free_tiles",
                                0L,
                            ) +
                            " · pair-free=" +
                            n2LocalGlobal.optLong(
                                "visible_candidate_pair_rejection_free_tiles",
                                0L,
                            ) +
                            " · scale-free=" +
                            n2LocalGlobal.optLong(
                                "visible_candidate_scale_rejection_free_tiles",
                                0L,
                            ) +
                            " · strict-vector=" +
                            n2LocalGlobal.optLong(
                                "visible_candidate_strict_local_support_vector_tiles",
                                0L,
                            ),
                        10f,
                        muted = true,
                    ))
                    addView(label(
                        "Frontside 16×16 analyse-regio's zijn proportioneel aan de 64×64 N2 Factored " +
                            "Confidence v0.3.1 bron-tiles gebonden. Dit is alleen lokale provenance/support-binding: " +
                            "N2 promotion=false, correction-supported=false.",
                        10f,
                        muted = true,
                    ))
                } else {
                    addView(space(4))
                    addView(label(
                        "N2 Local Spatial Binding v0.1 · UNKNOWN/fail-closed · reason=" +
                            n2LocalBinding.optString("reason", "niet beschikbaar") +
                            ". Geen lokale N2-claim zonder exacte source-binding.",
                        10f,
                        muted = true,
                    ))
                }

                if (
                    darkChromaV04.optString("status") ==
                    "AUDIT_ONLY_LOCAL_BINDING_AVAILABLE"
                ) {
                    addView(space(4))
                    addView(label(
                        "Dark Chroma Stability v0.4 · LOCAL N2 BINDING · global-v0.3=" +
                            darkChromaV04.optString(
                                "v0_3_global_information_state",
                                "UNKNOWN",
                            ) +
                            " · localBinding=" +
                            darkChromaV04.optBoolean("local_n2_binding_available", false) +
                            " · visible=" +
                            darkChromaV04Global.optLong("visible_candidate_tiles", 0L) +
                            " · locally-bound=" +
                            darkChromaV04Global.optLong(
                                "locally_bound_visible_candidate_tiles",
                                0L,
                            ) +
                            " · dark-blocked=" +
                            darkChromaV04Global.optLong(
                                "dark_uninformative_blocked_tiles",
                                0L,
                            ) +
                            " · protection-blocked=" +
                            darkChromaV04Global.optLong(
                                "structure_or_censor_blocked_tiles",
                                0L,
                            ) +
                            " · strict-vector=" +
                            darkChromaV04Global.optLong(
                                "local_strict_vector_present_tiles",
                                0L,
                            ) +
                            " · correction-supported=" +
                            darkChromaV04Global.optLong(
                                "chroma_correction_supported_tiles",
                                0L,
                            ),
                        10.5f,
                        muted = true,
                    ))
                    addView(label(
                        "v0.4 wet: lokale N2-binding mag DARK_UNINFORMATIVE nooit overrulen. " +
                            "Factored N2-assen blijven vector-valued diagnostiek en zijn geen kansscore. " +
                            "Private chroma A/B/Δ blijft uit tot deze binding op echte toesteldata is gevalideerd.",
                        10f,
                        muted = true,
                    ))
                }

                when (n2StructureSupport.optString("status")) {
                    "AUDIT_ONLY_FINE_BINDING_AVAILABLE" -> {
                        val overlapFraction =
                            n2StructureGlobal.optDouble(
                                "visible_candidate_overlap_structure_fraction",
                                Double.NaN,
                            )
                        val interiorFraction =
                            n2StructureGlobal.optDouble(
                                "visible_candidate_interior_structure_fraction",
                                Double.NaN,
                            )
                        val maxFineFraction =
                            n2StructureGlobal.optDouble(
                                "visible_candidate_max_fine_tile_structure_fraction",
                                Double.NaN,
                            )
                        addView(space(4))
                        addView(label(
                            "N2 Structure Support v0.1 · FINE 32×32 SOURCE GRID · visible=" +
                                n2StructureGlobal.optLong("visible_candidate_tiles", 0L) +
                                " · fine-bound=" +
                                n2StructureGlobal.optLong(
                                    "visible_candidate_bound_tiles",
                                    0L,
                                ) +
                                " · overlap-structure=" +
                                (if (overlapFraction.isFinite()) {
                                    "%.4f".format(overlapFraction)
                                } else {
                                    "?"
                                }) +
                                " · interior-structure=" +
                                (if (interiorFraction.isFinite()) {
                                    "%.4f".format(interiorFraction)
                                } else {
                                    "?"
                                }) +
                                " · fine tiles structure/free=" +
                                n2StructureGlobal.optLong(
                                    "visible_candidate_fine_tiles_with_structure",
                                    0L,
                                ) + "/" +
                                n2StructureGlobal.optLong(
                                    "visible_candidate_fine_tiles_without_structure",
                                    0L,
                                ) +
                                " · max-fine=" +
                                (if (maxFineFraction.isFinite()) {
                                    "%.4f".format(maxFineFraction)
                                } else {
                                    "?"
                                }),
                            10f,
                            muted = true,
                        ))
                        addView(label(
                            "Zelfde N2 structure-preservation gate en period=8 sample-grid, " +
                                "maar gerapporteerd op 32×32 bron-tiles. Alleen gemeten sample-support: " +
                                "onbemeten pixels worden niet ingevuld. Deze laag mag bescherming niet verminderen " +
                                "en kan geen correctie inschakelen.",
                            10f,
                            muted = true,
                        ))
                    }
                    "NOT_REQUIRED_BY_CURRENT_FRONT_SIDE_STATE" -> {
                        addView(space(4))
                        addView(label(
                            "N2 Structure Support v0.1 · niet nodig voor deze bronstate · reason=" +
                                n2StructureSupport.optString("reason", "UNKNOWN") +
                                ". Geen extra fine audit uitgevoerd.",
                            10f,
                            muted = true,
                        ))
                    }
                    else -> {
                        addView(space(4))
                        addView(label(
                            "N2 Structure Support v0.1 · UNKNOWN/fail-closed · reason=" +
                                n2StructureSupport.optString("reason", "niet beschikbaar") +
                                ". Geen fijnere structure-claim zonder bewezen source-binding.",
                            10f,
                            muted = true,
                        ))
                    }
                }

                if (
                    darkChromaV05.optString("status") ==
                    "AUDIT_ONLY_STRUCTURE_REFINEMENT_AVAILABLE"
                ) {
                    val overlapFraction =
                        darkChromaV05Global.optDouble(
                            "visible_candidate_overlap_structure_fraction",
                            Double.NaN,
                        )
                    val interiorFraction =
                        darkChromaV05Global.optDouble(
                            "visible_candidate_interior_structure_fraction",
                            Double.NaN,
                        )
                    addView(space(4))
                    addView(label(
                        "Dark Chroma Stability v0.5 · STRUCTURE RESOLUTION REFINEMENT · visible=" +
                            darkChromaV05Global.optLong("visible_candidate_tiles", 0L) +
                            " · fine-bound=" +
                            darkChromaV05Global.optLong(
                                "fine_bound_visible_candidate_tiles",
                                0L,
                            ) +
                            " · legacy-v0.4-blocked=" +
                            darkChromaV05Global.optLong(
                                "legacy_coarse_protection_blocked_visible_tiles",
                                0L,
                            ) +
                            " · zero-interior-structure=" +
                            darkChromaV05Global.optLong(
                                "fine_zero_interior_structure_visible_candidate_tiles",
                                0L,
                            ) +
                            " · overlap/interior=" +
                            (if (overlapFraction.isFinite()) {
                                "%.4f".format(overlapFraction)
                            } else {
                                "?"
                            }) + "/" +
                            (if (interiorFraction.isFinite()) {
                                "%.4f".format(interiorFraction)
                            } else {
                                "?"
                            }) +
                            " · correction-supported=" +
                            darkChromaV05Global.optLong(
                                "chroma_correction_supported_tiles",
                                0L,
                            ),
                        10.5f,
                        muted = true,
                    ))
                    addView(label(
                        "v0.5 verandert het v0.4-veto nog niet. Het meet alleen hoe dicht structure-protection " +
                            "werkelijk binnen/om de selectieve frontside-regio ligt. Geen kansscore, geen verborgen " +
                            "kleur, geen private A/B/Δ en geen Scientific-Master-writeback.",
                        10f,
                        muted = true,
                    ))
                }

                when (n2SupportDistance.optString("status")) {
                    "AUDIT_ONLY_DISTANCE_BINDING_AVAILABLE" -> {
                        val zero =
                            n2SupportDistanceGlobal.optJSONArray(
                                "center_zero_structure_candidates_r8_r16_r32_r64",
                            ) ?: JSONArray()
                        val nearestCenter =
                            n2SupportDistanceGlobal.optJSONObject(
                                "nearest_center_structure_distance_px",
                            ) ?: JSONObject()
                        val nearestRect =
                            n2SupportDistanceGlobal.optJSONObject(
                                "nearest_rect_structure_distance_px",
                            ) ?: JSONObject()
                        fun fmtDistance(o: JSONObject, key: String): String {
                            val v = o.optDouble(key, Double.NaN)
                            return if (v.isFinite()) "%.2f".format(v) else "?"
                        }
                        addView(space(4))
                        addView(label(
                            "N2 Sample Support Distance v0.1 · EXACT SAMPLED GEOMETRY · queries=" +
                                n2SupportDistanceGlobal.optLong("query_count", 0L) +
                                " · structure-inside=" +
                                n2SupportDistanceGlobal.optLong(
                                    "structure_inside_rect_candidates",
                                    0L,
                                ) +
                                " · center-zero r8/16/32/64=" +
                                zero.optLong(0, 0L) + "/" +
                                zero.optLong(1, 0L) + "/" +
                                zero.optLong(2, 0L) + "/" +
                                zero.optLong(3, 0L) +
                                " · nearest-center min/med/max=" +
                                fmtDistance(nearestCenter, "min") + "/" +
                                fmtDistance(nearestCenter, "median") + "/" +
                                fmtDistance(nearestCenter, "max") +
                                " px · nearest-rect min/med/max=" +
                                fmtDistance(nearestRect, "min") + "/" +
                                fmtDistance(nearestRect, "median") + "/" +
                                fmtDistance(nearestRect, "max") + " px",
                            10f,
                            muted = true,
                        ))
                        addView(label(
                            "Exacte N2 sample-coördinaten voor structure/censor/boundary zijn in de sidecar " +
                                "opgenomen en gehasht. Afstanden en radius-dichtheden zijn alleen diagnostiek: " +
                                "geen interpolatie van onbemeten pixels, geen kansscore en geen correctie-enable. " +
                                "v0.5 aggregate parity=" +
                                n2SupportDistance.optBoolean(
                                    "v0_5_aggregate_parity_verified",
                                    false,
                                ) +
                                ".",
                            10f,
                            muted = true,
                        ))
                    }
                    "NOT_REQUIRED_BY_CURRENT_FRONT_SIDE_STATE" -> {
                        addView(space(4))
                        addView(label(
                            "N2 Sample Support Distance v0.1 · niet nodig voor deze bronstate · reason=" +
                                n2SupportDistance.optString("reason", "UNKNOWN") +
                                ". Geen extra afstandsaudit uitgevoerd.",
                            10f,
                            muted = true,
                        ))
                    }
                    else -> {
                        addView(space(4))
                        addView(label(
                            "N2 Sample Support Distance v0.1 · UNKNOWN/fail-closed · reason=" +
                                n2SupportDistance.optString("reason", "niet beschikbaar") +
                                ". Geen afstandsclaim zonder exact sampled support.",
                            10f,
                            muted = true,
                        ))
                    }
                }

                if (
                    darkChromaV06.optString("status") ==
                    "AUDIT_ONLY_SAMPLE_SUPPORT_DISTANCE_AVAILABLE"
                ) {
                    val zero =
                        darkChromaV06Global.optJSONArray(
                            "center_zero_structure_candidates_r8_r16_r32_r64",
                        ) ?: JSONArray()
                    addView(space(4))
                    addView(label(
                        "Dark Chroma Stability v0.6 · SAMPLE-LEVEL SUPPORT DISTANCE · visible=" +
                            darkChromaV06Global.optLong("visible_candidate_tiles", 0L) +
                            " · distance-bound=" +
                            darkChromaV06Global.optLong(
                                "distance_bound_visible_candidate_tiles",
                                0L,
                            ) +
                            " · structure-inside=" +
                            darkChromaV06Global.optLong(
                                "structure_inside_rect_candidates",
                                0L,
                            ) +
                            " · center-zero r8/16/32/64=" +
                            zero.optLong(0, 0L) + "/" +
                            zero.optLong(1, 0L) + "/" +
                            zero.optLong(2, 0L) + "/" +
                            zero.optLong(3, 0L) +
                            " · correction-supported=" +
                            darkChromaV06Global.optLong(
                                "chroma_correction_supported_tiles",
                                0L,
                            ),
                        10.5f,
                        muted = true,
                    ))
                    addView(label(
                        "v0.6 voert nog geen afstandsdrempel in. Exact sampled structure/censor-support blijft " +
                            "een vector van meetfeiten; het mag bestaande bescherming niet verminderen en " +
                            "private chroma A/B/Δ blijft uit.",
                        10f,
                        muted = true,
                    ))
                }

                if (
                    n2LatticeGeometry.optString("status") ==
                    "AUDIT_ONLY_LATTICE_BINDING_AVAILABLE"
                ) {
                    addView(space(4))
                    addView(label(
                        "N2 Raster-Independent Geometry v0.1 · queries=" +
                            n2LatticeGeometry.optInt("query_count", 0) +
                            " · units/source-pixel=" +
                            n2LatticeGeometry.optLong(
                                "coordinate_units_per_source_pixel",
                                0L,
                            ) +
                            " · source samples unchanged · unanchored UNKNOWN · correction=false",
                        10f,
                        muted = true,
                    ))
                }

                if (
                    darkChromaV07.optString("status") ==
                    "AUDIT_ONLY_RASTER_INDEPENDENT_GEOMETRY_AVAILABLE"
                ) {
                    addView(label(
                        "Dark Chroma Stability v0.7 · SAMPLE-LATTICE BINDING · distance-bound=" +
                            darkChromaV07.optInt(
                                "distance_bound_candidate_count",
                                0,
                            ) +
                            " · noiseCorrection=false · private A/B/Δ=false · writeback=false",
                        10.5f,
                        muted = true,
                    ))
                    addView(label(
                        "v0.7 gebruikt het fijnere raster niet als nieuwe foto. Het is de oplossingsruimte " +
                            "waarin gemeten ankers, structure-support en latere reconstructies met eigen provenance " +
                            "kunnen bestaan zonder het camerarooster tot wereldgrens te maken.",
                        10f,
                        muted = true,
                    ))
                }

                when (anchorReconstruction.optString("status")) {
                    "AUDIT_ONLY_HOLDOUT_VALIDATION_AVAILABLE" -> {
                        val holdouts =
                            anchorReconstructionGlobal.optLong(
                                "holdouts",
                                0L,
                            )
                        val solverValid =
                            anchorReconstructionGlobal.optLong(
                                "solver_valid",
                                0L,
                            )
                        val baselineValid =
                            anchorReconstructionGlobal.optLong(
                                "baseline_valid",
                                0L,
                            )
                        val bothValid =
                            anchorReconstructionGlobal.optLong(
                                "both_valid",
                                0L,
                            )
                        val solverWins =
                            anchorReconstructionGlobal.optLong(
                                "solver_lower_abs_error",
                                0L,
                            )
                        val baselineWins =
                            anchorReconstructionGlobal.optLong(
                                "baseline_lower_abs_error",
                                0L,
                            )
                        val solverMae =
                            anchorReconstructionGlobal.optDouble(
                                "solver_mae",
                                Double.NaN,
                            )
                        val baselineMae =
                            anchorReconstructionGlobal.optDouble(
                                "baseline_mae",
                                Double.NaN,
                            )
                        val cov2 =
                            anchorReconstructionGlobal.optDouble(
                                "solver_coverage_2sigma",
                                Double.NaN,
                            )
                        addView(space(4))
                        addView(label(
                            "Anchor-Constrained Local Reconstruction v0.1 · HOLDOUT AUDIT · holdouts=" +
                                holdouts +
                                " · solver/baseline valid=" +
                                solverValid + "/" + baselineValid +
                                " · both=" + bothValid +
                                " · lower-|error| solver/baseline=" +
                                solverWins + "/" + baselineWins +
                                " · MAE solver/baseline=" +
                                (if (solverMae.isFinite()) {
                                    "%.7f".format(solverMae)
                                } else {
                                    "?"
                                }) + "/" +
                                (if (baselineMae.isFinite()) {
                                    "%.7f".format(baselineMae)
                                } else {
                                    "?"
                                }) +
                                " · 2σ coverage=" +
                                (if (cov2.isFinite()) {
                                    "%.3f".format(cov2)
                                } else {
                                    "?"
                                }),
                            10.5f,
                            muted = true,
                        ))
                        addView(label(
                            "Echte CFA-ankers worden tijdelijk alleen voor de predictor verborgen; hun waarde wordt " +
                                "pas daarna als holdout-truth gelezen. De nieuwe lokale affine lattice-solver gebruikt " +
                                "alleen andere MEASURED ankers. Uitvoer blijft RECONSTRUCTED/audit-only. Een lagere " +
                                "holdoutfout voorspelt de noisy meting beter, maar bewijst nog geen scene-truth of denoise-winst.",
                            10f,
                            muted = true,
                        ))
                    }
                    "NOT_REQUIRED_BY_CURRENT_FRONT_SIDE_STATE" -> {
                        addView(space(4))
                        addView(label(
                            "Anchor-Constrained Local Reconstruction v0.1 · niet nodig voor deze bronstate · reason=" +
                                anchorReconstruction.optString(
                                    "reason",
                                    "UNKNOWN",
                                ),
                            10f,
                            muted = true,
                        ))
                    }
                    else -> {
                        addView(space(4))
                        addView(label(
                            "Anchor-Constrained Local Reconstruction v0.1 · UNKNOWN/fail-closed · reason=" +
                                anchorReconstruction.optString(
                                    "reason",
                                    "niet beschikbaar",
                                ),
                            10f,
                            muted = true,
                        ))
                    }
                }

                when (observationModelSelection.optString("status")) {
                    "PROSPECTIVE_AUDIT_POLICY_AVAILABLE" -> {
                        addView(space(4))
                        addView(label(
                            "Universal Observation Model Selection v0.1 · PROSPECTIVE · queries=" +
                                observationModelSelection.optInt("query_count", 0) +
                                " · lensCalibration=false · cameraModel=false · vendorMap=false",
                            10.5f,
                            muted = true,
                        ))
                        addView(label(
                            "Modelbank wordt alleen uit deze verzegelde observation begrensd: exacte CFA-ankers, " +
                                "vrije lattice, Structure/Censored/CensorBoundary-support en frontside-geometrie. " +
                                "Geen held-out target/error wordt gebruikt voor selectie. De bestaande 29-09 tele-holdout " +
                                "is development evidence; een nieuwe onafhankelijke capture is vereist voor validatie.",
                            10f,
                            muted = true,
                        ))
                    }
                    else -> {
                        addView(space(4))
                        addView(label(
                            "Universal Observation Model Selection v0.1 · UNKNOWN/fail-closed · reason=" +
                                observationModelSelection.optString(
                                    "reason",
                                    "niet beschikbaar",
                                ),
                            10f,
                            muted = true,
                        ))
                    }
                }

                addView(label(
                    "Authority · bron-SHA=MEASURED · container=SOURCE_METADATA_BOUND · " +
                        "frontside=APPEARANCE_DERIVED_ONLY · scientificWriteback=false",
                    10f,
                    muted = true,
                ))
            }
            else -> {
                addView(actionButton("Lees bron universeel") { requestUniversalProfile(job) })
            }
        }
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
        clearN2AppearanceCandidate()
        clearN2CropAb()
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

                    // PRO is a scene/output route, not the source-bound diagnostic
                    // Scientific Preview. Once the immutable Scientific Master gate
                    // has completed, continue automatically through the raster-
                    // independent TruthNegative Continuous -> Open Scene -> Free-
                    // World area resolve -> Appearance/Display path. The diagnostic
                    // preview remains visible and unchanged for scientific inspection.
                    if (
                        preferredOutput == TruthRawSuiteLauncherActivity.OUTPUT_PRO &&
                        state is TilePreviewUiState.Ready
                    ) {
                        launchTruthNegativeContinuousPreview(job)
                    }
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
        val previousTier = renderedLayoutTier
        if (previousTier != null && previousTier != tier) {
            when {
                previousTier == LayoutTier.COMPACT &&
                    tier == LayoutTier.MEDIUM -> {
                    // Preserve the user's vertical working position in the main
                    // content column after a portrait -> landscape transition.
                    mediumRightScrollY = compactScrollY
                    // The source list is deliberately kept at its top so all
                    // selected RAW rows remain immediately reachable.
                    mediumLeftScrollY = 0
                }
                previousTier == LayoutTier.MEDIUM &&
                    tier == LayoutTier.COMPACT -> {
                    compactScrollY = mediumRightScrollY
                }
            }
        }
        renderedLayoutTier = tier
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
            addView(
                label(
                    "${tier.name.lowercase().replaceFirstChar { it.uppercase() }} layout · " +
                        "${session.selectedCount} RAW geselecteerd" +
                        if (researchWorkbenchMode) " · RESEARCH" else "",
                    12f,
                    muted = true,
                ),
            )
        }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))

        addView(actionButton("RAW kiezen") { launchRawPicker() })
    }

    private fun compactLayout(): View =
        rememberedScrollView(
            initialY = compactScrollY,
            onScrollYChanged = { compactScrollY = it },
        ).apply {
            addView(
                vertical().apply {
                    addView(
                        previewPane(),
                        LinearLayout.LayoutParams(
                            ViewGroup.LayoutParams.MATCH_PARENT,
                            ViewGroup.LayoutParams.WRAP_CONTENT,
                        ),
                    )
                    addView(space(8))
                    addView(
                        routePane(),
                        LinearLayout.LayoutParams(
                            ViewGroup.LayoutParams.MATCH_PARENT,
                            ViewGroup.LayoutParams.WRAP_CONTENT,
                        ),
                    )
                    if (session.jobs.isNotEmpty()) {
                        addView(space(8))
                        // Keep the selected-source rows in the page's single scroll
                        // surface. A fixed-height nested ScrollView caused touches to
                        // be consumed by the outer page on compact phones.
                        addView(
                            jobRowsPane(),
                            LinearLayout.LayoutParams(
                                ViewGroup.LayoutParams.MATCH_PARENT,
                                ViewGroup.LayoutParams.WRAP_CONTENT,
                            ),
                        )
                    }
                    addView(space(8))
                    addView(
                        if (researchWorkbenchMode) {
                            multiObservationPane()
                        } else {
                            researchEntryPane()
                        },
                        LinearLayout.LayoutParams(
                            ViewGroup.LayoutParams.MATCH_PARENT,
                            ViewGroup.LayoutParams.WRAP_CONTENT,
                        ),
                    )
                },
                ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                ),
            )
        }

    private fun mediumLayout(): View = horizontal().apply {
        // Medium used to place a weighted, internally scrolling job list above
        // wrap-content research controls. On phone landscape those controls could
        // collapse the job list to zero height. Make each column one independent
        // scroll surface instead.
        addView(
            rememberedScrollView(
                initialY = mediumLeftScrollY,
                onScrollYChanged = { mediumLeftScrollY = it },
            ).apply {
                addView(
                    vertical().apply {
                        addView(jobRowsPane())
                        addView(space(8))
                        addView(routePane())
                        addView(space(8))
                        addView(
                            if (researchWorkbenchMode) {
                                multiObservationPane()
                            } else {
                                researchEntryPane()
                            },
                        )
                    },
                    ViewGroup.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                    ),
                )
            },
            LinearLayout.LayoutParams(
                0,
                ViewGroup.LayoutParams.MATCH_PARENT,
                0.38f,
            ).apply { marginEnd = dp(8) },
        )

        addView(
            rememberedScrollView(
                initialY = mediumRightScrollY,
                onScrollYChanged = { mediumRightScrollY = it },
            ).apply {
                addView(
                    previewPane(),
                    ViewGroup.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                    ),
                )
            },
            LinearLayout.LayoutParams(
                0,
                ViewGroup.LayoutParams.MATCH_PARENT,
                0.62f,
            ),
        )
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
        addView(universalIntakePane(active))
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
                        "Diagnostische finalized D.RAW Scientific Preview voor ${active.source.displayName}; " +
                            "niet de PRO D.RAWnegative/Free-World eindweergave; " +
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
                if (
            preferredRoute() == TruthRawSuiteLauncherActivity.OUTPUT_PRO &&
            researchWorkbenchMode
        ) {
                    addView(label(
                        "Diagnostische Scientific Preview · source-bound controlebeeld; " +
                            "PRO bouwt automatisch daaronder de raster-onafhankelijke " +
                            "D.RAWnegative / Free-World Appearance-weergave.",
                        10f,
                        muted = true,
                    ))
                }
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
                            "D.RAWnegative/Free-World derivative · area-integrated scene resolve + " +
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

                    val candidateBitmap = n2AppearanceCandidateBitmap
                    val candidateMetrics = n2AppearanceCandidateMetrics
                    if (
                        preferredOutput == TruthRawSuiteLauncherActivity.OUTPUT_PRO &&
                        n2AppearanceCandidateJobId == active.id &&
                        candidateBitmap != null &&
                        candidateMetrics != null &&
                        (
                            outputPreview.outputLabel.contains("D.RAWnegative") ||
                                outputPreview.outputLabel.contains("TruthNegative Continuous")
                            )
                    ) {
                        addView(space(8))
                        addView(label(
                            "N2 A/B · appearance-only kandidaat",
                            12f,
                            bold = true,
                        ))
                        addView(label(
                            "A = ongewijzigde D.RAWnegative/Appearance. B = sampled-CFA N2-correcties " +
                                "alleen voor deze display-proef area-gemiddeld naar het previewraster. " +
                                "Geen kandidaat-reconstructie, geen wijziging van Scientific Master/D.RAWnegative, " +
                                "geen nieuwe evidence en geen export-writeback.",
                            9.5f,
                            muted = true,
                        ))
                        val turns = outputPreview.metrics.displayQuarterTurns
                        fun abImage(bitmap: Bitmap, description: String): ImageView =
                            ImageView(this@MainActivity).apply {
                                setImageBitmap(bitmap)
                                adjustViewBounds = true
                                scaleType = ImageView.ScaleType.FIT_CENTER
                                rotation = turns * 90f
                                if (
                                    turns % 2 != 0 &&
                                    bitmap.width > 0 &&
                                    bitmap.height > 0
                                ) {
                                    val ratio = minOf(
                                        bitmap.width.toFloat() / bitmap.height.toFloat(),
                                        bitmap.height.toFloat() / bitmap.width.toFloat(),
                                    )
                                    scaleX = ratio
                                    scaleY = ratio
                                }
                                contentDescription = description
                                minimumHeight = dp(120)
                                maxHeight = dp(240)
                            }
                        addView(horizontal().apply {
                            gravity = Gravity.TOP
                            addView(vertical().apply {
                                addView(label("A · huidig", 10.5f, bold = true))
                                addView(abImage(
                                    outputPreview.bitmap,
                                    "A: ongewijzigde D.RAWnegative Appearance",
                                ))
                            }, LinearLayout.LayoutParams(
                                0,
                                ViewGroup.LayoutParams.WRAP_CONTENT,
                                1f,
                            ).apply { marginEnd = dp(4) })
                            addView(vertical().apply {
                                addView(label("B · N2 kandidaat", 10.5f, bold = true))
                                addView(abImage(
                                    candidateBitmap,
                                    "B: N2 appearance-only kandidaat",
                                ))
                            }, LinearLayout.LayoutParams(
                                0,
                                ViewGroup.LayoutParams.WRAP_CONTENT,
                                1f,
                            ).apply { marginStart = dp(4) })
                        })
                        addView(label(
                            "B changed=${candidateMetrics.n2AppearanceChangedPixels}/" +
                                "${candidateMetrics.targetPixels} px · adjusted-ch=" +
                                "${candidateMetrics.n2AppearanceAdjustedChannels} · grid=" +
                                "${candidateMetrics.n2AppearanceGridWidth}×" +
                                "${candidateMetrics.n2AppearanceGridHeight} · grid SHA=" +
                                candidateMetrics.n2AppearanceGridSha256.take(16) +
                                "… · primary candidate-applied=false.",
                            9f,
                            muted = true,
                        ))
                        addView(space(7))
                        addView(actionButton("N2 · 1:1 Full-colour SAFE A/B/Δ") {
                            launchN2CropAbDiagnostic(active)
                        })
                        n2CropAbStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey(
                                    "truthnegative-n2-crop-ab",
                                    active.id,
                                ),
                                status,
                            )?.let(::addView) ?: addView(
                                label(status, 9.5f, muted = true),
                            )
                        }

                        val cropReady =
                            n2CropAbResult?.takeIf {
                                n2CropAbJobId == active.id
                            }
                        cropReady?.let { ready ->
                            val cropTurns = outputPreview.metrics.displayQuarterTurns
                            fun cropImage(
                                bitmap: Bitmap,
                                description: String,
                            ): ImageView =
                                ImageView(this@MainActivity).apply {
                                    setImageBitmap(bitmap)
                                    adjustViewBounds = true
                                    scaleType = ImageView.ScaleType.FIT_CENTER
                                    rotation = cropTurns * 90f
                                    contentDescription = description
                                    minimumHeight = dp(82)
                                    maxHeight = dp(180)
                                }

                            addView(space(8))
                            addView(label(
                                "N2 1:1 broncrops · full-colour support-safe candidate",
                                11.5f,
                                bold = true,
                            ))
                            addView(label(
                                "Iedere crop is 1 bronpixel → 1 previewpixel. A blijft de " +
                                    "ongewijzigde D.RAWnegative/Appearance. Voor B worden uitsluitend " +
                                    "N2-toegelaten CFA-correcties gebruikt. Correcties binnen de reconstructie-support " +
                                    "van een beschermd outputpixel worden eerst conservatief onderdrukt; daarna gaat de " +
                                    "private Stage-2-kopie door exact dezelfde measured-preserving Float64 full-colour " +
                                    "reconstructie als de Scientific Master. Beschermde core-pixels moeten daardoor " +
                                    "bit-identiek blijven. Δ toont |B−A| ×${ready.report.deltaGain}; zwart = geen zichtbaar verschil. " +
                                    "Bron, Scientific Master en D.RAWnegative blijven ongewijzigd.",
                                9.2f,
                                muted = true,
                            ))

                            ready.report.crops.forEach { panel ->
                                val m = panel.metrics
                                val title = when (m.kind) {
                                    TruthNegativeN2CropKind.QUIET_CANDIDATE ->
                                        "Rustige/noise-kandidaatzone"
                                    TruthNegativeN2CropKind.STRUCTURE ->
                                        "Structuurzone"
                                    TruthNegativeN2CropKind.CENSOR ->
                                        "Censor/highlight-zone"
                                }
                                addView(space(7))
                                addView(label(
                                    "$title · bron x=${m.sourceX}, y=${m.sourceY} · " +
                                        "${m.width}×${m.height}",
                                    10.5f,
                                    bold = true,
                                ))
                                addView(horizontal().apply {
                                    gravity = Gravity.TOP
                                    fun column(
                                        titleText: String,
                                        bitmap: Bitmap,
                                        description: String,
                                    ): LinearLayout =
                                        vertical().apply {
                                            addView(label(
                                                titleText,
                                                9.5f,
                                                bold = true,
                                            ))
                                            addView(cropImage(
                                                bitmap,
                                                description,
                                            ))
                                        }
                                    addView(
                                        column(
                                            "A",
                                            panel.aBitmap,
                                            "$title A referentie",
                                        ),
                                        LinearLayout.LayoutParams(
                                            0,
                                            ViewGroup.LayoutParams.WRAP_CONTENT,
                                            1f,
                                        ).apply { marginEnd = dp(3) },
                                    )
                                    addView(
                                        column(
                                            "B",
                                            panel.bBitmap,
                                            "$title B N2-kandidaat",
                                        ),
                                        LinearLayout.LayoutParams(
                                            0,
                                            ViewGroup.LayoutParams.WRAP_CONTENT,
                                            1f,
                                        ).apply {
                                            marginStart = dp(2)
                                            marginEnd = dp(2)
                                        },
                                    )
                                    addView(
                                        column(
                                            "Δ",
                                            panel.deltaBitmap,
                                            "$title absolute delta",
                                        ),
                                        LinearLayout.LayoutParams(
                                            0,
                                            ViewGroup.LayoutParams.WRAP_CONTENT,
                                            1f,
                                        ).apply { marginStart = dp(3) },
                                    )
                                })
                                addView(label(
                                    "sampled=${m.sampled} · candidate=${m.corrected} · " +
                                        "preserved=${m.preserved} · structure=${m.structureProtected} · " +
                                        "censored/boundary=${m.censoredProtected}/${m.censorBoundaryProtected} · " +
                                        "changed=${m.changedPixels}/${m.width * m.height} px · " +
                                        "Δmean=${"%.7f".format(m.meanAbsEncodedDelta)} · " +
                                        "Δmax=${"%.7f".format(m.maxAbsEncodedDelta)} · " +
                                        "removed-energy=${"%.3f".format(m.removedResidualEnergyFraction * 100.0)}% · " +
                                        "max|Δ|stage2=${"%.8f".format(m.maxAbsCorrectionStage2)} · " +
                                        "candidate-stage2=${m.candidateStage2Sites} · " +
                                        "rgb-changed=${m.adjustedChannels} · baseline-mismatch=" +
                                        "${m.baselineRgbMismatches} · candidate=" +
                                        "${m.candidateIdentitySha256.take(12)}…",
                                    8.7f,
                                    muted = true,
                                ))
                                addView(label(
                                    "Risk/Quality · pixel Δ p50/p95/p99/max=" +
                                        "${"%.7f".format(m.pixelDelta.p50)}/" +
                                        "${"%.7f".format(m.pixelDelta.p95)}/" +
                                        "${"%.7f".format(m.pixelDelta.p99)}/" +
                                        "${"%.7f".format(m.pixelDelta.max)} · " +
                                        "RGB p99=" +
                                        "${"%.7f".format(m.redDelta.p99)}/" +
                                        "${"%.7f".format(m.greenDelta.p99)}/" +
                                        "${"%.7f".format(m.blueDelta.p99)} · " +
                                        "Y′ p99=${"%.7f".format(m.lumaDelta.p99)} · " +
                                        "chroma p99=${"%.7f".format(m.chromaDelta.p99)}",
                                    8.7f,
                                    muted = true,
                                ))
                                addView(label(
                                    "max Δ @ bron x=${m.maxDeltaSourceX}, y=${m.maxDeltaSourceY} · " +
                                        "afstand structure=${if (m.distanceToStructurePx < 0.0) "n.v.t." else "%.2f px".format(m.distanceToStructurePx)} · " +
                                        "afstand censor/boundary=${if (m.distanceToCensorBoundaryPx < 0.0) "n.v.t." else "%.2f px".format(m.distanceToCensorBoundaryPx)} · " +
                                        "edge A/B=${"%.5f".format(m.edgeEnergyA)}/${"%.5f".format(m.edgeEnergyB)} · " +
                                        "B/A=${"%.6f".format(m.edgeEnergyRatio)} · " +
                                        "mean |Δgrad|=${"%.7f".format(m.meanAbsGradientDelta)} · " +
                                        "support-guard suppressed=${m.supportGuardSuppressedStage2Sites} · " +
                                        "protected-core=${m.protectedCorePixels} · protected-changed=" +
                                        "${m.protectedCoreChangedRgbChannels} · radius=${m.reconstructionInfluenceRadius}px · " +
                                        "QA=${m.qualitySha256.take(12)}…",
                                    8.7f,
                                    muted = true,
                                ))
                                addView(label(
                                    "N2 v0.2 center-excluded audit · v0.1-candidates=" +
                                        "${m.centerExcludedV01CandidateCenters} · predictor valid/invalid=" +
                                        "${m.centerExcludedPredictorValid}/${m.centerExcludedPredictorInvalid} · " +
                                        "pairs considered/accepted/rejected=" +
                                        "${m.centerExcludedPairsConsidered}/${m.centerExcludedPairsAccepted}/" +
                                        "${m.centerExcludedPairsRejected} · scales considered/accepted/rejected=" +
                                        "${m.centerExcludedScalesConsidered}/${m.centerExcludedScalesAccepted}/" +
                                        "${m.centerExcludedScalesRejected} · residual ≤1σ / 1–2σ / >2σ=" +
                                        "${m.centerExcludedResidualWithin1Sigma}/" +
                                        "${m.centerExcludedResidualBetween1And2Sigma}/" +
                                        "${m.centerExcludedResidualAbove2Sigma} · |r| mean/max=" +
                                        "${"%.8f".format(m.centerExcludedMeanAbsResidual)}/" +
                                        "${"%.8f".format(m.centerExcludedMaxAbsResidual)} · max dir/cross=" +
                                        "${"%.3f".format(m.centerExcludedMaxDirectionalSigma)}σ/" +
                                        "${"%.3f".format(m.centerExcludedMaxCrossScaleSigma)}σ · " +
                                        "audit-only=true · candidate-applied=false",
                                    8.7f,
                                    muted = true,
                                ))
                            }

                            addView(label(
                                "1:1 full-colour diagnose: baseline-reconstructie moet Float32-bit-identiek " +
                                    "zijn aan de exacte Scientific Master bronpixel; anders faalt de route gesloten. " +
                                    "Reconstruction-support closure vereist protected-core-changed=0. " +
                                    "Center-excluded v0.2 blijft parallel audit-only en wijzigt B niet. " +
                                    "Risk/Quality meet encoded-display Δ en gradientenergie alleen als diagnose " +
                                    "(geen MTF-claim). source/Scientific Master/D.RAWnegative blijven immutable · " +
                                    "creates-new-evidence=false · scientific-writeback=false.",
                                9f,
                                muted = true,
                            ))
                        }
                    }
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
                            "D.RAWnegative 200MP · Float32 Full Colour · DNG",
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
                            "D.RAWnegative 200MP · Float32 Full Colour · DNG",
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
                            "PRO · D.RAWnegative v0.1",
                            enabled =
                                active.source.format.nativeProcessingReady &&
                                    active.source.format.id == "DNG",
                        ) {
                            launchTruthNegativeContinuousPreview(active)
                        })
                        truthNegativeContinuousStatus?.let { status ->
                            val operationView = backgroundOperationStatusView(
                                backgroundOperationKey(
                                    "truthnegative-continuous-preview",
                                    active.id,
                                ),
                                status,
                            )
                            if (operationView != null) {
                                addView(operationView)
                                if (status.startsWith("TN Continuous v0.5 ·") &&
                                    status.contains("N2 audit-only:")
                                ) {
                                    addView(label(status, 9.5f, muted = true))
                                }
                            } else {
                                addView(label(status, 10f, muted = true))
                            }
                        }
                        addView(label(
                            "D.RAWnegative: één admitted observation → Scientific Master + Open Scene authority + " +
                                "legacy TruthNegative Continuous parent → observation-bound D.RAWnegative state → " +
                                "continue area-resolve. Source-local gauge; cross-lens radiometrische fusie blijft geblokkeerd.",
                            10f,
                            muted = true,
                        ))

                        if (researchWorkbenchMode) {
                            addView(space(8))
                            addView(
                                label(
                                    "Research tests & audits · groen/rood statuspunt + timer blijft actief",
                                    12f,
                                    bold = true,
                                ),
                            )
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
                            "D.RAWnegative · legacy parent .tnc",
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
                            "Legacy .tnc blijft byte-/schema-compatibiliteitsparent: Float32 Scientific Master + Open Scene authority. " +
                                "D.RAWnegative heeft een aparte state-SHA bovenop die parent; de v0.1 D.RAWnegative-state wordt niet stilletjes in .tnc geschreven.",
                            10f,
                            muted = true,
                        ))
                        addView(space(5))
                        addView(actionButton(
                            "Export N2 Spatial Audit · JSON",
                            enabled =
                                active.source.format.nativeProcessingReady &&
                                    active.source.format.id == "DNG",
                        ) {
                            launchN2SpatialSidecarExport(active)
                        })
                        n2SpatialSidecarStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey(
                                    "truthnegative-n2-spatial-sidecar",
                                    active.id,
                                ),
                                status,
                            )?.let(::addView) ?: addView(
                                label(status, 10f, muted = true),
                            )
                        }
                        addView(label(
                            "Audit-only spatial sidecar: per 64×64 source-tile candidate/preserve/" +
                                "structure/censor-statistiek, cryptografisch gebonden aan bron, " +
                                "Scientific Master, authority field en D.RAWnegative-state. " +
                                "candidate-applied=false.",
                            10f,
                            muted = true,
                        ))

                        addView(space(5))
                        addView(actionButton(
                            "Export N2 v0.2.1 Center-Excluded Spatial · JSON",
                            enabled =
                                active.source.format.nativeProcessingReady &&
                                    active.source.format.id == "DNG",
                        ) {
                            launchN2CenterExcludedSpatialExport(active)
                        })
                        n2CenterExcludedSpatialStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey(
                                    "truthnegative-n2-center-excluded-spatial",
                                    active.id,
                                ),
                                status,
                            )?.let(::addView) ?: addView(
                                label(status, 10f, muted = true),
                            )
                        }
                        addView(label(
                            "Parallel audit-only laag: verifieert eerst exacte v0.1 tile-parity en " +
                                "meet daarna per 64×64 tile center-excluded H/V/diagonaal multiscale " +
                                "predictor-support. center-only σ is primair; gecombineerde σ blijft " +
                                "diagnostisch omdat noise-independence niet is bewezen. B blijft ongewijzigd.",
                            10f,
                            muted = true,
                        ))

                        addView(space(5))
                        addView(actionButton(
                            "Export N2 v0.3 Confidence Field · JSON",
                            enabled =
                                active.source.format.nativeProcessingReady &&
                                    active.source.format.id == "DNG",
                        ) {
                            launchN2ConfidenceFieldExport(active)
                        })
                        n2ConfidenceFieldStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey(
                                    "truthnegative-n2-confidence-field",
                                    active.id,
                                ),
                                status,
                            )?.let(::addView) ?: addView(
                                label(status, 10f, muted = true),
                            )
                        }
                        addView(label(
                            "Vector-valued audit-only field boven v0.1 + v0.2.1: candidate-dichtheid, " +
                                "predictor coverage, pair/scale-coherentie, center-z, protection load, " +
                                "variance-ratio en CFA-fase blijven afzonderlijke assen. Geen gewogen " +
                                "probability, support-distance nog niet admitted en promotion=false.",
                            10f,
                            muted = true,
                        ))

                        addView(space(5))
                        addView(actionButton(
                            "Export N2 v0.3.1 Factored Confidence · JSON",
                            enabled =
                                active.source.format.nativeProcessingReady &&
                                    active.source.format.id == "DNG",
                        ) {
                            launchN2FactoredConfidenceExport(active)
                        })
                        n2FactoredConfidenceStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey(
                                    "truthnegative-n2-factored-confidence",
                                    active.id,
                                ),
                                status,
                            )?.let(::addView) ?: addView(
                                label(status, 10f, muted = true),
                            )
                        }
                        addView(label(
                            "Audit-only feitenlaag boven exact dezelfde v0.3 Confidence Field: " +
                                "all-candidates-predictable, center-outlier-free, pair/scale rejection, " +
                                "structure/censor/boundary en variance-relatie blijven apart. " +
                                "De oude support-class blijft alleen legacy; geen nieuwe drempels en promotion=false.",
                            10f,
                            muted = true,
                        ))

                        addView(space(5))
                        addView(actionButton(
                            "Export N2 Sample Support Distance v0.1 · JSON",
                            enabled =
                                active.source.format.nativeProcessingReady &&
                                    active.source.format.id == "DNG" &&
                                    universalProfiles[active.id]
                                        ?.optJSONObject("n2_sample_support_distance")
                                        ?.optString("status") ==
                                    "AUDIT_ONLY_DISTANCE_BINDING_AVAILABLE",
                        ) {
                            launchN2SupportDistanceExport(active)
                        })
                        n2SupportDistanceStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey(
                                    "truthnegative-n2-support-distance",
                                    active.id,
                                ),
                                status,
                            )?.let(::addView) ?: addView(
                                label(status, 10f, muted = true),
                            )
                        }
                        addView(label(
                            "v0.6 audit-sidecar: exact sampled Structure/Censored/CensorBoundary broncoördinaten " +
                                "+ center/radius en rect-margin afstanden. Geen interpolatie van onbemeten pixels, " +
                                "geen afstandsdrempel, geen promotion en geen Scientific-Master-writeback.",
                            10f,
                            muted = true,
                        ))

                        addView(space(5))
                        addView(actionButton(
                            "Export Anchor-Constrained Reconstruction v0.1 · JSON",
                            enabled =
                                active.source.format.nativeProcessingReady &&
                                    active.source.format.id == "DNG" &&
                                    universalProfiles[active.id]
                                        ?.optJSONObject(
                                            "anchor_constrained_local_reconstruction",
                                        )
                                        ?.optString("status") ==
                                    "AUDIT_ONLY_HOLDOUT_VALIDATION_AVAILABLE",
                        ) {
                            launchAnchorConstrainedReconstructionExport(active)
                        })
                        anchorReconstructionStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey(
                                    "anchor-constrained-local-reconstruction",
                                    active.id,
                                ),
                                status,
                            )?.let(::addView) ?: addView(
                                label(status, 10f, muted = true),
                            )
                        }
                        addView(label(
                            "Holdout-audit: echte CFA-ankers blijven verzegeld en worden alleen tijdelijk voor de " +
                                "predictor verborgen. De private lattice-solver voorspelt ze uit andere gemeten " +
                                "ankers; daarna wordt pas met de echte waarde vergeleken. RECONSTRUCTED authority, " +
                                "onzekerheid diagnostisch, geen correction-enable en geen writeback.",
                            10f,
                            muted = true,
                        ))

                        addView(space(5))
                        addView(actionButton(
                            "Export Universal Observation Model Selection v0.1 · JSON",
                            enabled =
                                universalProfiles[active.id]
                                    ?.optJSONObject(
                                        "universal_observation_model_selection",
                                    )
                                    ?.optString("status") ==
                                "PROSPECTIVE_AUDIT_POLICY_AVAILABLE",
                        ) {
                            launchObservationModelSelectionExport(active)
                        })
                        observationModelSelectionStatus?.let { status ->
                            addView(label(status, 10f, muted = true))
                        }
                        addView(label(
                            "Prospective selector-sidecar: legt alleen observation-derived supportregime en " +
                                "eligible modelbank vast. Geen held-out target, holdout-error, lensprofiel, " +
                                "camera-ID of vendor-map. Exporteer deze vóór beoordeling van nieuwe holdoutresultaten.",
                            10f,
                            muted = true,
                        ))

                        addView(space(5))
                        addView(actionButton(
                            "Export Universal Local Model Bank Holdout v0.1 · JSON",
                            enabled =
                                active.source.format.nativeProcessingReady &&
                                    active.source.format.id == "DNG" &&
                                    universalProfiles[active.id]
                                        ?.optJSONObject(
                                            "universal_local_model_bank_holdout",
                                        )
                                        ?.optString("status") ==
                                    "READY_FOR_EXPLICIT_EXPORT_AUDIT",
                        ) {
                            launchUniversalModelBankHoldoutExport(active)
                        })
                        universalModelBankHoldoutStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey(
                                    "universal-local-model-bank-holdout",
                                    active.id,
                                ),
                                status,
                            )?.let(::addView) ?: addView(
                                label(status, 10f, muted = true),
                            )
                        }
                        addView(label(
                            "Full-resolution research-audit: houdt een gestratificeerde set echte CFA-ankers " +
                                "verborgen over de originele bron en kiest vóór target-reveal uit median/constant, " +
                                "directional line, affine, quadratic of NO_RECONSTRUCTION. Geen lens/camera/vendor-profiel " +
                                "nodig voor selectie; de post-reveal oracle is alleen diagnose en mag de selector niet sturen.",
                            10f,
                            muted = true,
                        ))

                        addView(space(5))
                        addView(actionButton(
                            "Export Universal Local Model Bank Holdout v0.2 · JSON",
                            enabled =
                                active.source.format.nativeProcessingReady &&
                                    active.source.format.id == "DNG" &&
                                    universalProfiles[active.id]
                                        ?.optJSONObject(
                                            "universal_local_model_bank_holdout_v0_2",
                                        )
                                        ?.optString("status") ==
                                    "READY_FOR_EXPLICIT_EXPORT_AUDIT",
                        ) {
                            launchUniversalModelBankHoldoutV02Export(active)
                        })
                        universalModelBankHoldoutV02Status?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey(
                                    "universal-local-model-bank-holdout-v02",
                                    active.id,
                                ),
                                status,
                            )?.let(::addView) ?: addView(
                                label(status, 10f, muted = true),
                            )
                        }
                        addView(label(
                            "v0.2 successor: modelkeuze gebruikt een target-blinde support-crossfit. " +
                                "De directionele kandidaat fit alleen richtinggebonden CFA-supportstroken, zodat hij " +
                                "niet meer algebraïsch dezelfde centrumvoorspelling als affine hoeft te geven. " +
                                "Tele/main/ultra-wide identiteit blijft buiten de selector.",
                            10f,
                            muted = true,
                        ))

                        addView(space(5))
                        addView(actionButton(
                            "Export Universal Local Model Bank Holdout v0.3 · JSON",
                            enabled =
                                active.source.format.nativeProcessingReady &&
                                    active.source.format.id == "DNG" &&
                                    universalProfiles[active.id]
                                        ?.optJSONObject(
                                            "universal_local_model_bank_holdout_v0_3",
                                        )
                                        ?.optString("status") ==
                                    "READY_FOR_EXPLICIT_EXPORT_AUDIT",
                        ) {
                            launchUniversalModelBankHoldoutV03Export(active)
                        })
                        universalModelBankHoldoutV03Status?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey(
                                    "universal-local-model-bank-holdout-v03",
                                    active.id,
                                ),
                                status,
                            )?.let(::addView) ?: addView(
                                label(status, 10f, muted = true),
                            )
                        }
                        addView(label(
                            "v0.3 selector-only successor: gebruikt exact dezelfde v0.2 kandidaatfits en " +
                                "directionele support, maar vergelijkt model families puur op dezelfde common " +
                                "validation-RMS. Geen tweede BIC/complexiteitsstraf; modelcomplexiteit is alleen " +
                                "deterministische tie-break. Geen lens/camera/vendor-profiel en geen writeback.",
                            10f,
                            muted = true,
                        ))

                        addView(space(5))
                        addView(actionButton(
                            "Export Universal Observation & Calibration Atlas v0.1 · JSON",
                            enabled =
                                universalProfiles[active.id]
                                    ?.optJSONObject(
                                        "universal_observation_calibration_atlas",
                                    )
                                    ?.optString("status") ==
                                "OBSERVATION_ATLAS_AVAILABLE",
                        ) {
                            launchUniversalCalibrationAtlasExport(active)
                        })
                        universalCalibrationAtlasStatus?.let { status ->
                            addView(label(status, 10f, muted = true))
                        }
                        addView(label(
                            "Universele observatiekaart van dezelfde sealed bron: voor- en achterkant gekoppeld, " +
                                "kleur/lichtval/optiek/tijd/restauratie als losse authority-assen. Geen camera- of " +
                                "lensprofiel vereist; optionele kalibratie is extra evidence en nooit een ingangseis.",
                            10f,
                            muted = true,
                        ))

                        addView(space(5))
                        addView(actionButton(
                            "Export Appearance Highlight Detail v0.1 · JSON",
                            enabled =
                                active.source.format.nativeProcessingReady &&
                                    active.source.format.id == "DNG",
                        ) {
                            launchAppearanceHighlightDetailExport(active)
                        })
                        appearanceHighlightDetailStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey(
                                    "truthnegative-appearance-highlight-detail",
                                    active.id,
                                ),
                                status,
                            )?.let(::addView) ?: addView(
                                label(status, 10f, muted = true),
                            )
                        }
                        addView(label(
                            "Display-diagnose boven dezelfde PRO Appearance v0.7: vergelijkt scene-luminantie " +
                                "met mapped luminance vóór sRGB-encoding en telt naburige bronverschillen die " +
                                "op exact dezelfde display-peak eindigen. Geen Honor-reference als evidence, " +
                                "geen MTF-claim en geen wijziging van Scientific Master/D.RAWnegative.",
                            10f,
                            muted = true,
                        ))

                        addView(space(5))
                        addView(actionButton(
                            "Export Appearance Headroom Sweep v0.2 · JSON",
                            enabled =
                                active.source.format.nativeProcessingReady &&
                                    active.source.format.id == "DNG",
                        ) {
                            launchAppearanceHeadroomSweepExport(active)
                        })
                        appearanceHeadroomSweepStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey(
                                    "truthnegative-appearance-headroom-sweep",
                                    active.id,
                                ),
                                status,
                            )?.let(::addView) ?: addView(
                                label(status, 10f, muted = true),
                            )
                        }
                        addView(label(
                            "Parallel appearance-only sweep op dezelfde 100-nit SDR-peak. De shoulder begint " +
                                "bij 100, 90, 80 of 70 nit; de route meet peak-collapse, gradientbehoud, " +
                                "gamut-clamp en lower-range verandering. Er wordt geen winnaar gekozen en " +
                                "de normale PRO-preview blijft ongewijzigd.",
                            10f,
                            muted = true,
                        ))
                        }

                        addView(space(5))
                        addView(actionButton("Legacy Scientific Negative · TN-4") {
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

    private fun researchEntryPane(): View = card().apply {
        addView(label("Research & JSON", 16f, bold = true))
        addView(space(5))
        addView(
            label(
                "Normale fotoverwerking blijft compact. Open deze werkbank alleen voor Multi-observation, Calibration Observation Records, field/atlas-exports, holdout-audits en de Free World Foundation.",
                10.5f,
                muted = true,
            ),
        )
        addView(space(8))
        addView(actionButton("Open onderzoekswerkbank") {
            researchWorkbenchMode = true
            render()
        })
        addView(space(6))
        addView(actionButton("Research-overzicht / Global JSON") {
            startActivity(
                Intent(
                    this@MainActivity,
                    TruthRawResearchHubActivity::class.java,
                ),
            )
        })
    }

    private fun multiObservationPane(): View = card().apply {
        val selected = session.jobs.size
        val profiled =
            session.jobs.count { universalProfiles[it.id] != null }
        val measuredCharts = currentMeasuredFieldCharts().size
        val calibrationRecordCount =
            calibrationObservationRecords.size

        addView(label("Multi-observation · Research & JSON", 16f, bold = true))
        addView(label(
            "selected=" + selected +
                " · universal-profile=" + profiled +
                " · measured-field-chart=" + measuredCharts +
                " · minimum=3",
            11f,
            muted = true,
        ))
        researchWorkbenchSessionRestoreStatus?.let {
            addView(
                label(
                    it,
                    10f,
                    muted = true,
                ),
            )
        }
        addView(space(6))
        addView(actionButton(
            "Voeg relation-based Calibration Observation Record(s) · JSON toe",
            enabled = true,
        ) {
            launchCalibrationObservationRecordPicker()
        })
        calibrationObservationRecordStatus?.let {
            addView(label(it, 10f, muted = true))
        }
        addView(label(
            "Optioneel · records=" +
                calibrationRecordCount +
                " · normale RAW-intake vereist geen calibratie. Camera/lens/vendor/RAW-identiteit mag geen scientific key zijn.",
            10f,
            muted = true,
        ))
        if (calibrationRecordCount > 0) {
            addView(actionButton(
                "Wis gekoppelde Calibration Observation Records",
                enabled = true,
            ) {
                calibrationObservationRecords.clear()
                calibrationObservationRecordStatus =
                    "Calibration Observation Records gewist · RAW-observaties blijven onaangeraakt."
                render()
            })
        }
        addView(space(6))
        addView(actionButton(
            "Analyseer alle geselecteerde bronnen universeel",
            enabled = selected > 0,
        ) {
            requestUniversalProfilesForSelectedSources()
        })
        addView(space(5))
        addView(actionButton(
            "Export Field Response Repeatability v0.1 · JSON",
            enabled = measuredCharts >= 3,
        ) {
            launchFieldResponseRepeatabilityExport()
        })
        addView(space(5))
        addView(actionButton(
            "Export Observation-World Field Separation v0.1 · JSON",
            enabled = profiled >= 2,
        ) {
            launchObservationWorldFieldSeparationExport()
        })
        observationWorldFieldSeparationStatus?.let {
            addView(label(it, 10f, muted = true))
        }
        addView(space(5))
        addView(actionButton(
            "Export Free World Observation Geometry Foundation v0.1 · JSON",
            enabled = profiled >= 2,
        ) {
            launchFreeWorldFoundationExport()
        })
        freeWorldFoundationStatus?.let {
            addView(label(it, 10f, muted = true))
        }
        addView(label(
            "Foundation-export bundelt deterministische lokale features, pair-geometry hypotheses, " +
                "Free World observation graph, multi-lens/360 campaign policy, Natural Self-Calibration Atlas, " +
                "radiometric/noise/optics/colour/temporal/3D/world-space candidate-runtimes, " +
                "gescheiden uncertainty, continuous-query contract en restoration authority. " +
                "Optionele relation-records voeden alleen kandidaten; alles blijft fail-closed tot afzonderlijke validatie.",
            10f,
            muted = true,
        ))
        addView(label(
            "Nieuwe vrije-wereld fundering: SOURCE/SENSOR SPACE, WORLD/SCENE SPACE en VIEW/OUTPUT SPACE " +
                "blijven strikt gescheiden. Gewone overlappende of 360°-observaties mogen later een " +
                "deterministische wereldregistratie leveren; de gebruiker of panoramamiddenpunt wordt nooit " +
                "het calibratiemiddelpunt. v0.1 registreert nog niet automatisch en corrigeert niets.",
            10f,
            muted = true,
        ))
        backgroundOperationStatusView(
            fieldResponseRepeatabilityAnalysisOperationKey(),
            fieldResponseRepeatabilityStatus
                ?: "Field Response Repeatability v0.1 bronanalyse",
        )?.let(::addView)
            ?: fieldResponseRepeatabilityStatus?.let {
                addView(label(it, 10f, muted = true))
            }

        if (fieldResponseBatchPendingJobIds.isNotEmpty()) {
            addView(label(
                "Analyse actief · nog " +
                    fieldResponseBatchPendingJobIds.size +
                    " bron(nen) bezig.",
                10f,
                muted = true,
            ))

            val journal =
                ResearchBatchJournalV02.read(
                    this@MainActivity,
                    fieldResponseRepeatabilityAnalysisOperationKey(),
                )
            if (journal != null) {
                val heartbeat =
                    journal.optLong(
                        "service_heartbeat_wall_ms",
                        0L,
                    )
                val ageSeconds =
                    if (heartbeat > 0L) {
                        (
                            (
                                System.currentTimeMillis() -
                                    heartbeat
                                ).coerceAtLeast(0L) /
                                1000L
                            )
                    } else {
                        -1L
                    }
                val system =
                    journal.optJSONObject(
                        "system",
                    )
                val pssKb =
                    system?.optLong(
                        "process_pss_kb",
                        -1L,
                    ) ?: -1L
                val stage =
                    journal.optString(
                        "current_stage",
                        "UNKNOWN",
                    )
                val attempt =
                    journal.optInt(
                        "attempt_count",
                        0,
                    )
                val redeliveries =
                    journal.optInt(
                        "redelivery_count",
                        0,
                    )
                addView(
                    label(
                        "Service-checkpoint · stage=" +
                            stage +
                            " · heartbeat=" +
                            if (ageSeconds >= 0L) {
                                ageSeconds.toString() + " s"
                            } else {
                                "onbekend"
                            } +
                            " · PSS=" +
                            if (pssKb >= 0L) {
                                (pssKb / 1024L).toString() + " MiB"
                            } else {
                                "onbekend"
                            } +
                            " · poging=" +
                            attempt +
                            " · redelivery=" +
                            redeliveries,
                        10f,
                        muted = true,
                    ),
                )

                journal.optJSONObject(
                    "previous_process_exit",
                )?.takeIf {
                    it.optBoolean(
                        "android17_memory_limiter_anon_swap",
                        false,
                    )
                }?.let {
                    addView(
                        label(
                            "Vorige process-exit: Android 17 MemoryLimiter:AnonSwap gedetecteerd.",
                            10f,
                            muted = true,
                        ),
                    )
                }
            }
        }

        if (measuredCharts < 3) {
            addView(label(
                "Repeatability-gate nog niet open: measured-field-chart=" +
                    measuredCharts + " · minimum=3. Alleen DNG-observaties met een werkelijk gemeten PR96 " +
                    "CFA-field chart tellen mee; JPEG en decoder-pending/ongeschikte RAW-topologie tellen niet mee.",
                10f,
                muted = true,
            ))
        }

        addView(label(
            "Read-only vergelijking van ≥3 onafhankelijke PR96-field charts. Per observation wordt alleen een " +
                "scalar niveau verwijderd; radiale, azimutale en CFA-fasevormen worden in EV vergeleken. " +
                "Geen camera-/lensidentiteit, geen lens-only vignettering, geen kalibratiepromotie, geen correctie.",
            10f,
            muted = true,
        ))
        addView(space(8))
        addView(actionButton("Research-overzicht / Global JSON") {
            startActivity(
                Intent(
                    this@MainActivity,
                    TruthRawResearchHubActivity::class.java,
                ),
            )
        })
        addView(space(6))
        addView(actionButton("Sluit onderzoekswerkbank") {
            researchWorkbenchMode = false
            render()
        })
    }

    private fun toolsPane(): View = vertical().apply {
        addView(routePane())
        addView(space(8))
        addView(
            if (researchWorkbenchMode) {
                multiObservationPane()
            } else {
                researchEntryPane()
            },
        )
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

    private fun jobRowsPane(): View = card().apply {
        addView(label("Ingang", 16f, bold = true))
        addView(
            label(
                "${session.selectedCount} onafhankelijke bronhandle(s)",
                12f,
                muted = true,
            ),
        )
        addView(space(6))
        if (session.jobs.isEmpty()) {
            addView(label("Nog geen RAW geselecteerd.", 13f, muted = true))
        } else {
            session.jobs.forEachIndexed { index, job ->
                addView(jobRow(index, job))
            }
        }
    }

    private fun rememberedScrollView(
        initialY: Int,
        onScrollYChanged: (Int) -> Unit,
    ): ScrollView =
        ScrollView(this).apply {
            isFillViewport = true
            setOnScrollChangeListener { _, _, scrollY, _, _ ->
                onScrollYChanged(scrollY)
            }
            post {
                scrollTo(0, initialY.coerceAtLeast(0))
            }
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
        background = rounded(
            if (enabled) palette.surfaceAlt else palette.surface,
            14f,
        )
        isEnabled = enabled
        // Disabled actions stay visibly disabled by their surface, not by
        // washing out the label. User-facing text remains dark/readable.
        alpha = 1f
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
        private const val STATE_RESEARCH_WORKBENCH_MODE =
            "truthraw.state.RESEARCH_WORKBENCH_MODE"
        private const val STATE_CALIBRATION_RECORD_STATUS =
            "truthraw.state.CALIBRATION_RECORD_STATUS"
        private const val STATE_CALIBRATION_SESSION_STORE_ID =
            "truthraw.state.CALIBRATION_SESSION_STORE_ID"
        const val EXTRA_AUTO_OPEN_RAW_PICKER = "truthraw.extra.AUTO_OPEN_RAW_PICKER"
        const val EXTRA_OPEN_RESEARCH_WORKBENCH =
            "truthraw.extra.OPEN_RESEARCH_WORKBENCH"
        const val EXTRA_AUTO_OPEN_CALIBRATION_RECORD_PICKER =
            "truthraw.extra.AUTO_OPEN_CALIBRATION_RECORD_PICKER"
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
        private const val REQUEST_SAVE_N2_SPATIAL_SIDECAR = 4114
        private const val REQUEST_SAVE_N2_CENTER_EXCLUDED_SPATIAL = 4115
        private const val REQUEST_SAVE_N2_CONFIDENCE_FIELD = 4116
        private const val REQUEST_SAVE_N2_FACTORED_CONFIDENCE = 4117
        private const val REQUEST_SAVE_APPEARANCE_HIGHLIGHT_DETAIL = 4118
        private const val REQUEST_SAVE_APPEARANCE_HEADROOM_SWEEP = 4119
        private const val REQUEST_SAVE_N2_SUPPORT_DISTANCE = 4120
        private const val REQUEST_SAVE_ANCHOR_RECONSTRUCTION = 4121
        private const val REQUEST_SAVE_OBSERVATION_MODEL_SELECTION = 4122
        private const val REQUEST_SAVE_UNIVERSAL_MODEL_BANK_HOLDOUT = 4123
        private const val REQUEST_SAVE_UNIVERSAL_MODEL_BANK_HOLDOUT_V02 = 4124
        private const val REQUEST_SAVE_UNIVERSAL_MODEL_BANK_HOLDOUT_V03 = 4125
        private const val REQUEST_SAVE_OBSERVATION_OPTICAL_FIELD = 4126
        private const val REQUEST_SAVE_UNIVERSAL_CALIBRATION_ATLAS = 4127
        private const val REQUEST_SAVE_FIELD_RESPONSE_REPEATABILITY = 4128
        private const val REQUEST_SAVE_OBSERVATION_WORLD_FIELD_SEPARATION = 4129
        private const val REQUEST_SAVE_FREE_WORLD_FOUNDATION = 4130
        private const val REQUEST_OPEN_CALIBRATION_OBSERVATION_RECORDS = 4131
    }
}