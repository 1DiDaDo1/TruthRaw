package com.truthraw.adaptiveui

import android.content.Context
import android.os.SystemClock
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.security.MessageDigest

/**
 * Fail-closed runtime handoff from one full-resolution output render to the
 * downstream Free Raster presentation cable.
 *
 * v0.3 keeps the existing JPEG commit/current-output confirmation gate, but the
 * staged Free Raster artifact is no longer that JPEG. It is the exact RGB24
 * sibling emitted by the same native render before NV21/JPEG chroma reduction.
 * This remains DERIVED_PRESENTATION_OUTPUT / PRESENTATION_ONLY and never writes
 * back to Scientific Master.
 */
internal object UnifiedOutputFreeRasterRuntimeV01 {
    const val CONTRACT_VERSION = "UnifiedOutputFreeRasterRuntime/0.3"

    private const val CONFIRM_LABEL_PREFIX = "JPG full-resolution "
    private const val MAX_STAGE_AGE_MS = 120_000L

    sealed interface StageResult {
        data class Ready(
            val jpegRequest: DrawUnifiedOutputRasterRequestV01,
            val freeRasterRequest: DrawUnifiedOutputRasterRequestV01,
            val sha256: String,
            val bytes: Long,
        ) : StageResult

        data class Failed(
            val reason: String,
        ) : StageResult
    }

    private data class StagedArtifact(
        val file: File,
        val sha256: String,
        val bytes: Long,
    )

    private data class StagedState(
        val generation: Long,
        val jpegRequest: DrawUnifiedOutputRasterRequestV01,
        val freeRasterRequest: DrawUnifiedOutputRasterRequestV01,
        val artifact: StagedArtifact,
        val startedAtElapsedMs: Long,
        var promoting: Boolean = false,
    )

    private val lock = Any()

    @Volatile
    private var appContext: Context? = null

    private var generation: Long = 0L
    private var stagedState: StagedState? = null

    fun initialize(context: Context) {
        appContext = context.applicationContext
    }

    /**
     * Resolve and stage the exact pre-encode RGB24 sibling associated with this
     * JPEG candidate. No Free Raster state becomes visible here.
     */
    fun stageRenderedJpeg(
        binding: DrawPhotoOutputBindingV01,
        renderedFile: File,
        outputWidth: Int,
        outputHeight: Int,
        jpegSha256: String,
    ): StageResult {
        val context = appContext
            ?: return StageResult.Failed("Free Raster staging: runtime-context ontbreekt.")

        val jpegRequest = DrawUnifiedOutputRasterContractV01.fromFullResolutionJpegBinding(
            binding = binding,
            targetWidth = outputWidth,
            targetHeight = outputHeight,
        )
        val freeRasterRequest = DrawUnifiedOutputRasterContractV01.fromFreeRasterBinding(
            binding = binding,
            targetWidth = outputWidth,
            targetHeight = outputHeight,
        )
        val siblingMismatch = DrawUnifiedOutputRasterContractV01
            .validateJpegFreeRasterSiblingBasis(jpegRequest, freeRasterRequest)
        if (siblingMismatch != null) {
            return StageResult.Failed(siblingMismatch)
        }

        val normalizedJpegSha = jpegSha256.trim().lowercase()
        if (!normalizedJpegSha.matches(Regex("[0-9a-f]{64}"))) {
            return StageResult.Failed("Free Raster staging: ongeldige JPEG SHA-256.")
        }
        if (!renderedFile.isFile || renderedFile.length() <= 0L) {
            return StageResult.Failed("Free Raster staging: JPEG-candidate ontbreekt.")
        }
        if (FullResJpegExporter.sha256(renderedFile) != normalizedJpegSha) {
            return StageResult.Failed("Free Raster staging: JPEG-candidate SHA-256 mismatch.")
        }

        val raster = FullResPresentationRasterRegistryV01.resolveForJpeg(
            jpegFile = renderedFile,
            expectedWidth = outputWidth,
            expectedHeight = outputHeight,
        ) ?: return StageResult.Failed(
            "Free Raster staging: exact pre-encode RGB24 sibling ontbreekt.",
        )

        fun fail(reason: String): StageResult.Failed {
            FullResPresentationRasterRegistryV01.releaseForJpeg(renderedFile, deleteRaster = true)
            return StageResult.Failed(reason)
        }

        val expectedRgbBytes = outputWidth.toLong() * outputHeight.toLong() * 3L
        if (
            raster.pixelFormat != FullResPresentationRasterV01.PIXEL_FORMAT ||
            raster.bytes != expectedRgbBytes ||
            raster.file.length() != expectedRgbBytes
        ) {
            return fail("Free Raster staging: RGB24-rastergeometrie mismatch.")
        }

        val stageDir = File(context.filesDir, "free_raster_staging/${binding.sourceJobId}")
        if (!stageDir.exists() && !stageDir.mkdirs()) {
            return fail("Free Raster staging: private stagingmap kon niet worden gemaakt.")
        }
        if (!stageDir.isDirectory) {
            return fail("Free Raster staging: private staginglocatie is ongeldig.")
        }

        val nextGeneration = synchronized(lock) {
            generation += 1L
            cleanupLocked()
            UnifiedOutputFreeRasterBridge.clear("new_full_resolution_candidate_staged")
            generation
        }
        val stageFile = File(
            stageDir,
            ".stage_${nextGeneration}_${System.nanoTime()}.rgb24",
        )

        val digest = MessageDigest.getInstance("SHA-256")
        val bytes = try {
            FileInputStream(raster.file).use { input ->
                FileOutputStream(stageFile).use { output ->
                    val buffer = ByteArray(256 * 1024)
                    var total = 0L
                    while (true) {
                        val count = input.read(buffer)
                        if (count < 0) break
                        if (count == 0) continue
                        output.write(buffer, 0, count)
                        digest.update(buffer, 0, count)
                        total += count.toLong()
                    }
                    output.flush()
                    output.fd.sync()
                    total
                }
            }
        } catch (error: Throwable) {
            stageFile.delete()
            return fail(
                "Free Raster RGB24 staging-copy faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        }

        val copiedSha = digest.digest().joinToString("") { byte -> "%02x".format(byte) }
        if (
            bytes != expectedRgbBytes ||
            !stageFile.isFile ||
            stageFile.length() != bytes ||
            copiedSha != raster.sha256
        ) {
            stageFile.delete()
            return fail("Free Raster RGB24 staging: SHA-256/bytecontrole faalde.")
        }

        val artifact = StagedArtifact(stageFile, copiedSha, bytes)
        val state = StagedState(
            generation = nextGeneration,
            jpegRequest = jpegRequest,
            freeRasterRequest = freeRasterRequest,
            artifact = artifact,
            startedAtElapsedMs = SystemClock.elapsedRealtime(),
        )
        synchronized(lock) {
            if (generation != nextGeneration) {
                stageFile.delete()
                return fail("Free Raster staging: nieuwere outputcandidate heeft voorrang.")
            }
            stagedState = state
        }
        FullResPresentationRasterRegistryV01.releaseForJpeg(renderedFile, deleteRaster = true)

        return StageResult.Ready(
            jpegRequest = jpegRequest,
            freeRasterRequest = freeRasterRequest,
            sha256 = copiedSha,
            bytes = bytes,
        )
    }

    /**
     * Promotion gate. The exact output label is emitted only by the saved-JPEG
     * result preview, after successful destination commit. The current route is
     * supplied by MainActivity at publication time, so a route change during the
     * render also prevents promotion.
     */
    fun confirmFromSavedJpegPreview(
        sourceJobId: String,
        sourceUri: String,
        route: String,
        outputLabel: String,
    ) {
        if (outputLabel != CONFIRM_LABEL_PREFIX + route) return

        val state = synchronized(lock) {
            val current = stagedState ?: return
            if (isExpired(current)) {
                cleanupLocked()
                return
            }
            val request = current.jpegRequest
            if (
                request.sourceJobId != sourceJobId ||
                request.sourceUri != sourceUri ||
                request.route != route ||
                current.promoting
            ) {
                return
            }
            current.promoting = true
            current
        }

        Thread(
            { promoteStaged(state) },
            "draw-free-raster-promote-${sourceJobId.take(8)}",
        ).start()
    }

    fun discardStaged(sourceJobId: String, reason: String) {
        synchronized(lock) {
            val current = stagedState ?: return
            if (current.jpegRequest.sourceJobId != sourceJobId) return
            cleanupLocked()
            UnifiedOutputFreeRasterBridge.clear(
                if (reason.isBlank()) "staged_candidate_discarded" else reason,
            )
        }
    }

    private fun promoteStaged(state: StagedState) {
        if (isExpired(state)) {
            discardStaged(state.jpegRequest.sourceJobId, "full_resolution_promotion_timeout")
            return
        }
        val context = appContext
        if (context == null) {
            discardStaged(state.jpegRequest.sourceJobId, "runtime_context_missing")
            return
        }

        val outputDirectory = File(
            context.filesDir,
            "free_raster_output/${state.jpegRequest.sourceJobId}",
        )
        val result = UnifiedOutputFreeRasterBridge.publishRenderedRgb24(
            renderedFile = state.artifact.file,
            freeRasterRequest = state.freeRasterRequest,
            expectedSha256 = state.artifact.sha256,
            artifactDirectory = outputDirectory,
        )
        if (result is UnifiedOutputFreeRasterPublishResultV01.Failed) {
            UnifiedOutputFreeRasterBridge.clear("full_resolution_rgb24_publish_failed")
        }

        synchronized(lock) {
            if (stagedState?.generation == state.generation) {
                cleanupLocked()
            }
        }
    }

    private fun isExpired(state: StagedState): Boolean =
        SystemClock.elapsedRealtime() - state.startedAtElapsedMs > MAX_STAGE_AGE_MS

    private fun cleanupLocked() {
        stagedState?.artifact?.file?.delete()
        stagedState = null
    }
}
