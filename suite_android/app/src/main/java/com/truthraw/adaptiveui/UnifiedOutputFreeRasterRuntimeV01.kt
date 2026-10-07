package com.truthraw.adaptiveui

import android.content.Context
import android.graphics.BitmapFactory
import android.os.SystemClock
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.security.MessageDigest

/**
 * Fail-closed runtime handoff from the already-existing full-resolution JPEG
 * renderer to the downstream Free Raster presentation cable.
 *
 * It never renders pixels. MainActivity explicitly stages the exact private
 * renderer artifact before that temporary file is deleted. Staging alone does
 * not publish anything: promotion is allowed only after MainActivity later
 * publishes its normal saved-JPEG preview, which happens after successful
 * destination commit and on the then-active source/route.
 */
internal object UnifiedOutputFreeRasterRuntimeV01 {
    const val CONTRACT_VERSION = "UnifiedOutputFreeRasterRuntime/0.2"

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
     * Copy and SHA-verify the exact renderer artifact into runtime-owned staging.
     * No Free Raster state becomes visible here.
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

        val normalizedExpectedSha = jpegSha256.trim().lowercase()
        if (!normalizedExpectedSha.matches(Regex("[0-9a-f]{64}"))) {
            return StageResult.Failed("Free Raster staging: ongeldige JPEG SHA-256.")
        }
        if (!renderedFile.isFile || renderedFile.length() <= 0L) {
            return StageResult.Failed("Free Raster staging: renderer-artifact ontbreekt.")
        }

        val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
        runCatching { BitmapFactory.decodeFile(renderedFile.absolutePath, bounds) }
            .getOrElse {
                return StageResult.Failed("Free Raster staging: JPEG-bounds konden niet worden gelezen.")
            }
        if (bounds.outWidth != outputWidth || bounds.outHeight != outputHeight) {
            return StageResult.Failed(
                "Free Raster staging: renderer-geometrie ${bounds.outWidth}x${bounds.outHeight} != " +
                    "binding ${outputWidth}x${outputHeight}.",
            )
        }

        val stageDir = File(context.filesDir, "free_raster_staging/${binding.sourceJobId}")
        if (!stageDir.exists() && !stageDir.mkdirs()) {
            return StageResult.Failed("Free Raster staging: private stagingmap kon niet worden gemaakt.")
        }
        if (!stageDir.isDirectory) {
            return StageResult.Failed("Free Raster staging: private staginglocatie is ongeldig.")
        }

        val nextGeneration = synchronized(lock) {
            generation += 1L
            cleanupLocked()
            UnifiedOutputFreeRasterBridge.clear("new_full_resolution_candidate_staged")
            generation
        }
        val stageFile = File(
            stageDir,
            ".stage_${nextGeneration}_${System.nanoTime()}.jpg",
        )

        val digest = MessageDigest.getInstance("SHA-256")
        val bytes = try {
            FileInputStream(renderedFile).use { input ->
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
            return StageResult.Failed(
                "Free Raster staging-copy faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        }

        val copiedSha = digest.digest().joinToString("") { byte -> "%02x".format(byte) }
        if (
            bytes <= 0L ||
            !stageFile.isFile ||
            stageFile.length() != bytes ||
            copiedSha != normalizedExpectedSha
        ) {
            stageFile.delete()
            return StageResult.Failed("Free Raster staging: SHA-256/bytecontrole faalde.")
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
                return StageResult.Failed("Free Raster staging: nieuwere outputcandidate heeft voorrang.")
            }
            stagedState = state
        }

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
        val result = UnifiedOutputFreeRasterBridge.publishRenderedJpeg(
            renderedFile = state.artifact.file,
            freeRasterRequest = state.freeRasterRequest,
            expectedSha256 = state.artifact.sha256,
            artifactDirectory = outputDirectory,
        )
        if (result is UnifiedOutputFreeRasterPublishResultV01.Failed) {
            UnifiedOutputFreeRasterBridge.clear("full_resolution_publish_failed")
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
