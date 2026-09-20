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
    private var previewGeneration: Long = 0
    private var loadingStartedAtElapsedMs: Long? = null
    private var pendingJpegJobId: String? = null
    private var jpegStatus: String? = null
    private var pendingJpgLJobId: String? = null
    private var jpgLStatus: String? = null
    private var pendingPhotoRoute: String? = null
    private var pendingPhotoFlags: Int = 0
    private var pendingPhotoQuarterTurns: Int = 0
    private var pendingPureFloatDngJobId: String? = null
    private var pendingPureQuarterTurns: Int = 0
    private var pureFloatDngStatus: String? = null
    private var pendingTruthNegativeJobId: String? = null
    private var truthNegativeStatus: String? = null
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
        get() {
            val dark = resources.configuration.uiMode and Configuration.UI_MODE_NIGHT_MASK ==
                Configuration.UI_MODE_NIGHT_YES
            return if (dark) {
                Palette(
                    background = Color.rgb(15, 17, 20),
                    surface = Color.rgb(27, 30, 35),
                    surfaceAlt = Color.rgb(36, 40, 46),
                    text = Color.rgb(245, 246, 248),
                    textMuted = Color.rgb(172, 178, 187),
                    accent = Color.rgb(128, 157, 255),
                )
            } else {
                Palette(
                    background = Color.rgb(246, 247, 249),
                    surface = Color.WHITE,
                    surfaceAlt = Color.rgb(235, 238, 243),
                    text = Color.rgb(24, 27, 31),
                    textMuted = Color.rgb(92, 99, 109),
                    accent = Color.rgb(54, 88, 200),
                )
            }
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)

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
        jpgLStatus = null
        pureFloatDngStatus = null
        truthNegativeStatus = null
        fullResRestorationStatus = null
        projectionStatus = null
    }

    override fun onResume() {
        super.onResume()
        FullResRestorationJobStore.recoverInterruptedIfNeeded(this)
        RestorationProjectionJobStore.recoverInterruptedIfNeeded(this)
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
        (nefMeasurementResult as? NefMeasurementResult.Ready)?.bitmap?.recycle()
        super.onDestroy()
    }

    private fun syncFullResRestorationStatus() {
        val snapshot = FullResRestorationJobStore.read(this) ?: return
        if (snapshot.message != fullResRestorationStatus) {
            fullResRestorationStatus = snapshot.message
            render()
        }
    }

    private fun syncRestorationProjectionStatus() {
        val snapshot = RestorationProjectionJobStore.read(this) ?: return
        if (snapshot.message != projectionStatus) {
            projectionStatus = snapshot.message
            render()
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
            putExtra(Intent.EXTRA_TITLE, "${stem}_truthraw_${route.lowercase()}_fullres.jpg")
        }
        startActivityForResult(intent, REQUEST_SAVE_JPEG)
    }

    @Suppress("DEPRECATION")
    private fun launchJpgLExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        pendingJpgLJobId = job.id
        jpgLStatus = null
        val route = preferredRoute()
        pendingPhotoRoute = route
        pendingPhotoFlags = photoFlagsForRoute(route)
        pendingPhotoQuarterTurns = TruthRawOrientationOverride.quarterTurns(this, job.source)
        val stem = job.source.displayName.substringBeforeLast('.', job.source.displayName)
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            // JPG-L v0.3 RAW/Edit: Float32 DNG is the primary Lightroom-editable
            // image. JPEG remains preview/delivery only and is never the authority.
            type = "image/x-adobe-dng"
            putExtra(Intent.EXTRA_TITLE, "${stem}_truthraw_jpgl_raw_edit_v0_3.dng")
        }
        startActivityForResult(intent, REQUEST_SAVE_JPG_L)
    }

    @Suppress("DEPRECATION")
    private fun launchPureFloatDngExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        if (!job.source.format.nativeProcessingReady || job.source.format.id != "DNG") {
            pureFloatDngStatus =
                "TRUTHRAW PURE Float32 is momenteel alleen beschikbaar voor de volledig admitted DNG-route."
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
            putExtra(Intent.EXTRA_TITLE, "${stem}_truthraw_pure_float32_v0_63.dng")
        }
        startActivityForResult(intent, REQUEST_SAVE_PURE_FLOAT_DNG)
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
            putExtra(Intent.EXTRA_TITLE, "${stem}_truthraw_fullres_restoration_v0_67.trr")
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
            putExtra(Intent.EXTRA_TITLE, "${stem}_truthraw_restoration_v0_72.${format.extension}")
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
            putExtra(Intent.EXTRA_TITLE, "${stem}_truthraw_linear_v0_1.dng")
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
            putExtra(Intent.EXTRA_TITLE, "${stem}_truthraw_raw_ingress_empirical_v0_2.json")
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
            putExtra(Intent.EXTRA_TITLE, "${stem}_truthraw_nef_measurement_v0_58.json")
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
                jpegStatus = "JPG-export geblokkeerd: actieve TruthRaw-route veranderde."
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
            Thread({
                val dir = File(filesDir, "photo_export/$expectedJob").apply { mkdirs() }
                val rendered = FullResJpegExporter.renderToPrivateJpeg(
                    contentResolver, job, flags, quarterTurns, dir,
                )
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
                        jpegStatus = status
                        render()
                    }
                }
            }, "truthraw-fullres-jpg-${job.id.take(8)}").start()
            return
        }

        if (requestCode == REQUEST_SAVE_JPG_L) {
            val expectedJob = pendingJpgLJobId
            pendingJpgLJobId = null
            val route = pendingPhotoRoute ?: preferredRoute()
            val flags = pendingPhotoFlags
            val quarterTurns = pendingPhotoQuarterTurns
            pendingPhotoRoute = null
            pendingPhotoFlags = 0
            pendingPhotoQuarterTurns = 0
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                jpgLStatus = "JPG-L RAW/Edit-export geannuleerd."
                render()
                return
            }
            val job = session.jobs.firstOrNull { it.id == expectedJob }
            val ready = previewState as? TilePreviewUiState.Ready
            if (expectedJob == null || job == null || ready == null ||
                ready.jobId != expectedJob || activeJobId != expectedJob
            ) {
                jpgLStatus = "JPG-L RAW/Edit geblokkeerd: actieve TruthRaw-route veranderde."
                render()
                return
            }

            val operationKey = backgroundOperationKey("jpgl-raw-edit", expectedJob)
            if (!startBackgroundOperation(operationKey, "JPG-L RAW/Edit Float32 DNG opbouwen")) {
                jpgLStatus = "JPG-L achtergrondverwerking kon niet veilig starten."
                render()
                return
            }
            jpgLStatus =
                "JPG-L RAW/Edit · 32-bit Float DNG wordt opgebouwd… " +
                    "Float32 is primaire editlaag; Advanced blijft non-destructief recipe."
            render()
            Thread({
                val dir = File(filesDir, "jpgl_raw_edit/$expectedJob").apply { mkdirs() }
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
                            "JPG-L RAW/Edit preview faalde: ${previewResult.reason}",
                        )
                    is FullResJpegResult.Success -> {
                        try {
                            PureFloat32DngExporter.export(
                                contentResolver,
                                job,
                                destination,
                                quarterTurns,
                                Float32DngExportFlavor.JPGL_RAW_EDIT,
                                flags,
                                previewResult.file,
                                previewResult.metrics.width,
                                previewResult.metrics.height,
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
                        is PureFloat32DngExportResult.Success -> "JPG-L RAW/Edit gereed."
                        is PureFloat32DngExportResult.Failed -> exportResult.reason
                    },
                )
                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    jpgLStatus = when (exportResult) {
                        is PureFloat32DngExportResult.Failed -> exportResult.reason
                        is PureFloat32DngExportResult.Success -> {
                            val m = exportResult.metrics
                            "JPG-L RAW/Edit v0.3 gereed · ${m.width}×${m.height} · " +
                                "${formatBytes(m.outputBytes)} · IEEE Float32 primary + embedded JPEG preview · " +
                                "negatief/>1=${m.negativeComponentCount}/${m.overOneComponentCount} · " +
                                "Advanced recipe flags=$flags · route=$route · " +
                                "Scientific Master replay=${m.scientificMasterIdentityVerified} · " +
                                "self/edit-binding=${m.postWriteSelfBindingVerified} · " +
                                "rotatie=${quarterTurns * 90}°."
                        }
                    }
                    render()
                }
            }, "truthraw-jpgl-raw-edit-${job.id.take(8)}").start()
            return
        }

        if (requestCode == REQUEST_SAVE_PURE_FLOAT_DNG) {
            val expectedJob = pendingPureFloatDngJobId
            pendingPureFloatDngJobId = null
            val quarterTurns = pendingPureQuarterTurns
            pendingPureQuarterTurns = 0
            val destination = data?.data
            if (resultCode != RESULT_OK || destination == null) {
                pureFloatDngStatus = "TRUTHRAW PURE Float32-export geannuleerd."
                render()
                return
            }
            val job = session.jobs.firstOrNull { it.id == expectedJob }
            val ready = previewState as? TilePreviewUiState.Ready
            if (expectedJob == null || job == null || ready == null ||
                ready.jobId != expectedJob || activeJobId != expectedJob
            ) {
                pureFloatDngStatus =
                    "TRUTHRAW PURE Float32-export geblokkeerd: actieve finalized preview veranderde."
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
                "TRUTHRAW PURE · 32-bit Float DNG wordt opgebouwd… exact Master replay + digest gate."
            render()

            Thread({
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
                    pureFloatDngStatus = when (exportResult) {
                        is PureFloat32DngExportResult.Failed -> exportResult.reason
                        is PureFloat32DngExportResult.Success -> {
                            val m = exportResult.metrics
                            val previewText =
                                if (preview != null) "embedded neutral JPEG preview" else "zonder preview"
                            "TRUTHRAW PURE opgeslagen + teruggelezen · ${m.width}×${m.height} · " +
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
            }, "truthraw-pure-f32-${job.id.take(8)}").start()
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
            if (!startBackgroundOperation(operationKey, "TRUTHNEGATIVE TN-3 opbouwen")) {
                truthNegativeStatus = "TRUTHNEGATIVE achtergrondverwerking kon niet veilig starten."
                render()
                return
            }
            truthNegativeStatus =
                "TRUTHNEGATIVE TN-3 wordt opgebouwd… exact Master replay + Dynamic Authority + canonical Open Scene v0.70."
            render()

            Thread({
                val exportResult =
                    TruthNegativeExporter.export(contentResolver, job, destination)
                finishBackgroundOperation(
                    operationKey,
                    exportResult is TruthNegativeExportResult.Success,
                    when (exportResult) {
                        is TruthNegativeExportResult.Success -> "TRUTHNEGATIVE TN-3 gereed."
                        is TruthNegativeExportResult.Failed -> exportResult.reason
                    },
                )
                runOnUiThread {
                    if (activeJobId != expectedJob) return@runOnUiThread
                    truthNegativeStatus = when (exportResult) {
                        is TruthNegativeExportResult.Failed -> exportResult.reason
                        is TruthNegativeExportResult.Success -> {
                            val m = exportResult.metrics
                            "TRUTHNEGATIVE TN-3 opgeslagen + teruggelezen · ${m.width}×${m.height} · " +
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
            }, "truthnegative-tn3-${job.id.take(8)}").start()
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
            Thread({
                val exportResult = LinearDngExporter.export(contentResolver, job, destination)
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
            }, "truthraw-linear-dng-${job.id.take(8)}").start()
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
        pureFloatDngStatus = null
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
        ++previewGeneration
        activeJobId = job.id
        loadingStartedAtElapsedMs = null
        previewState = TilePreviewUiState.Idle
        jpegStatus = null
        pureFloatDngStatus = null
        linearDngStatus = null
        empiricalStatus = null
        empiricalAudit = null
        pendingJpegJobId = null
        pendingJpgLJobId = null
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
        val state = TruthRawOperationStore.read(this, key) ?: return null
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

        Thread({
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
        }, "truthraw-nef-measurement-${job.id.take(8)}").start()
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
        if (!startBackgroundOperation(operationKey, "TruthRaw foto verwerken")) {
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

        Thread({
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
                        is TilePreviewUiState.Ready -> "TruthRaw Advanced/PRO render gereed."
                        is TilePreviewUiState.Failed -> state.reason
                        is TilePreviewUiState.Loading -> "TruthRaw render bleef onverwacht in loading."
                        TilePreviewUiState.Idle -> "TruthRaw render gaf onverwacht Idle terug."
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
                        is TilePreviewUiState.Ready -> "TruthRaw PURE render gereed."
                        is TilePreviewUiState.Failed -> state.reason
                        is TilePreviewUiState.Loading -> "TruthRaw PURE render bleef onverwacht in loading."
                        TilePreviewUiState.Idle -> "TruthRaw PURE render gaf onverwacht Idle terug."
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
        }, "truthraw-preview-${job.id.take(8)}").start()
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
            addView(label("TruthRaw", 22f, bold = true))
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
                    addView(actionButton("Start TruthRaw") { requestPreview(active) })
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
                        "TruthRaw foto wordt verwerkt…",
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
                    "TruthRaw render gereed.",
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
                        "Finalized TruthRaw Scientific Preview voor ${active.source.displayName}, " +
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
                    "Route: " + when (preferredOutput) {
                        TruthRawSuiteLauncherActivity.OUTPUT_ADVANCED -> "TRUTHRAW ADVANCED"
                        TruthRawSuiteLauncherActivity.OUTPUT_PRO -> "TRUTHRAW PRO"
                        else -> "TRUTHRAW PURE"
                    },
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
                        addView(actionButton("JPG-L RAW/Edit · Float32 DNG · Lightroom") {
                            launchJpgLExport(active)
                        })
                        jpgLStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey("jpgl-raw-edit", active.id),
                                status,
                            )?.let(::addView) ?: addView(label(status, 10f, muted = true))
                        }
                        addView(label(
                            "JPG-L RAW/Edit gebruikt Float32 Linear DNG als primaire bewerkbare afbeelding. " +
                                "Negatieve en >1 waarden blijven behouden; Advanced-instellingen worden als non-destructief recipe gebonden. " +
                                "De gewone JPG blijft uitsluitend preview/delivery.",
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
                        addView(actionButton("JPG-L RAW/Edit · Float32 DNG · Lightroom") {
                            launchJpgLExport(active)
                        })
                        jpgLStatus?.let { status ->
                            backgroundOperationStatusView(
                                backgroundOperationKey("jpgl-raw-edit", active.id),
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
                        addView(actionButton("Scientific Negative · TN-3") {
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
                TruthRawSuiteLauncherActivity.OUTPUT_ADVANCED -> "TRUTHRAW ADVANCED · vrije fotografische ontwikkeling"
                TruthRawSuiteLauncherActivity.OUTPUT_PRO -> "TRUTHRAW PRO · professionele werkbank"
                else -> "TRUTHRAW PURE · directe wetenschappelijke route"
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
        private const val REQUEST_SAVE_JPG_L = 4110
    }
}