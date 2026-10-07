package com.truthraw.adaptiveui

import android.content.Context
import android.graphics.BitmapFactory
import android.os.FileObserver
import android.os.SystemClock
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.security.MessageDigest

/**
 * Runtime handoff between the already-existing full-resolution JPEG renderer
 * and the downstream Free Raster presentation cable.
 *
 * This object never renders pixels. A validated JPEG_EXPORT request only arms a
 * read-only observer on MainActivity's private renderer directory. The emitted
 * file is staged after CLOSE_WRITE, but it is not promoted to Free Raster until
 * MainActivity has independently completed its normal post-render authority
 * checks, document commit and saved-JPEG preview publication.
 *
 * Therefore a stale/failed renderer result can never become the active Free
 * Raster source merely because a private file happened to be written.
 */
internal object UnifiedOutputFreeRasterRuntimeV01 {
    const val CONTRACT_VERSION = "UnifiedOutputFreeRasterRuntime/0.1"

    private const val CONFIRM_LABEL_PREFIX = "JPG full-resolution "
    private const val MAX_ARM_AGE_MS = 120_000L
    private const val WATCH_EVENTS = FileObserver.CLOSE_WRITE or FileObserver.MOVED_TO

    private data class FileSignature(
        val length: Long,
        val modified: Long,
    )

    private data class StagedArtifact(
        val file: File,
        val sha256: String,
        val bytes: Long,
    )

    private class ArmedState(
        val generation: Long,
        val request: DrawUnifiedOutputRasterRequestV01,
        val directory: File,
        val baseline: Map<String, FileSignature>,
        val startedAtElapsedMs: Long,
        val observer: FileObserver,
        var confirmed: Boolean = false,
        var staged: StagedArtifact? = null,
        var promoting: Boolean = false,
    )

    private val lock = Any()

    @Volatile
    private var appContext: Context? = null

    private var generation: Long = 0L
    private var armed: ArmedState? = null

    fun initialize(context: Context) {
        appContext = context.applicationContext
    }

    /**
     * Presentation-side hook invoked only after the outer raster request has
     * passed the existing scientific/evidence-law validator.
     *
     * Failure to arm is intentionally non-authoritative: JPEG export keeps its
     * original behaviour. Free Raster simply remains unavailable.
     */
    fun armValidatedJpegRequest(request: DrawUnifiedOutputRasterRequestV01) {
        if (request.purpose != DrawUnifiedOutputRasterRequestV01.Purpose.JPEG_EXPORT) return
        val context = appContext ?: return

        val directory = File(context.filesDir, "photo_export/${request.sourceJobId}")
        if (!directory.exists() && !directory.mkdirs()) return
        if (!directory.isDirectory) return

        val baseline = directory.listFiles()
            ?.filter { it.isFile }
            ?.associate { file ->
                file.absolutePath to FileSignature(file.length(), file.lastModified())
            }
            .orEmpty()

        val nextGeneration: Long
        synchronized(lock) {
            generation += 1L
            nextGeneration = generation
            cleanupLocked("rearm")
            UnifiedOutputFreeRasterBridge.clear("new_validated_full_resolution_jpeg_request")
        }

        lateinit var observer: FileObserver
        observer = object : FileObserver(directory.absolutePath, WATCH_EVENTS) {
            override fun onEvent(event: Int, path: String?) {
                if ((event and WATCH_EVENTS) == 0 || path.isNullOrBlank()) return
                stageCandidate(nextGeneration, File(directory, path))
            }
        }

        val state = ArmedState(
            generation = nextGeneration,
            request = request,
            directory = directory,
            baseline = baseline,
            startedAtElapsedMs = SystemClock.elapsedRealtime(),
            observer = observer,
        )

        synchronized(lock) {
            if (nextGeneration != generation) return
            armed = state
        }
        observer.startWatching()
    }

    /**
     * MainActivity publishes this preview only after the normal JPEG result path
     * has completed its source/route/orientation checks and successful document
     * commit. That publication is the promotion gate for the previously staged
     * private full-resolution artifact.
     */
    fun confirmFromSavedJpegPreview(
        sourceJobId: String,
        sourceUri: String,
        route: String,
        outputLabel: String,
    ) {
        if (outputLabel != CONFIRM_LABEL_PREFIX + route) return

        var promote: Pair<ArmedState, StagedArtifact>? = null
        synchronized(lock) {
            val state = armed ?: return
            if (isExpired(state)) {
                cleanupLocked("confirmation_timeout")
                return
            }
            val request = state.request
            if (
                request.sourceJobId != sourceJobId ||
                request.sourceUri != sourceUri ||
                request.route != route
            ) {
                return
            }
            state.confirmed = true
            val staged = state.staged
            if (staged != null && !state.promoting) {
                state.promoting = true
                promote = state to staged
            }
        }

        promote?.let { (state, staged) ->
            Thread(
                { promoteStaged(state, staged) },
                "draw-free-raster-promote-${sourceJobId.take(8)}",
            ).start()
        }
    }

    private fun stageCandidate(expectedGeneration: Long, candidate: File) {
        val state = synchronized(lock) {
            val current = armed ?: return
            if (current.generation != expectedGeneration) return
            if (isExpired(current)) {
                cleanupLocked("observer_timeout")
                return
            }
            if (current.staged != null) return
            current
        }

        if (!candidate.isFile || candidate.length() <= 0L) return
        val previous = state.baseline[candidate.absolutePath]
        if (
            previous != null &&
            previous.length == candidate.length() &&
            previous.modified == candidate.lastModified()
        ) {
            return
        }

        val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
        runCatching { BitmapFactory.decodeFile(candidate.absolutePath, bounds) }.getOrNull()
        if (
            bounds.outWidth != state.request.targetWidth ||
            bounds.outHeight != state.request.targetHeight
        ) {
            return
        }

        val context = appContext ?: return
        val stageDir = File(context.filesDir, "free_raster_staging/${state.request.sourceJobId}")
        if (!stageDir.exists() && !stageDir.mkdirs()) return
        if (!stageDir.isDirectory) return

        val stageFile = File(
            stageDir,
            ".stage_${state.generation}_${System.nanoTime()}.jpg",
        )

        val digest = MessageDigest.getInstance("SHA-256")
        val bytes = try {
            FileInputStream(candidate).use { input ->
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
        } catch (_: Throwable) {
            stageFile.delete()
            return
        }

        if (bytes <= 0L || !stageFile.isFile || stageFile.length() != bytes) {
            stageFile.delete()
            return
        }
        val sha256 = digest.digest().joinToString("") { byte -> "%02x".format(byte) }
        val staged = StagedArtifact(stageFile, sha256, bytes)

        var promote: Pair<ArmedState, StagedArtifact>? = null
        synchronized(lock) {
            val current = armed
            if (
                current == null ||
                current.generation != state.generation ||
                isExpired(current)
            ) {
                stageFile.delete()
                return
            }
            if (current.staged != null) {
                stageFile.delete()
                return
            }
            current.staged = staged
            if (current.confirmed && !current.promoting) {
                current.promoting = true
                promote = current to staged
            }
        }

        promote?.let { (promoteState, promoteArtifact) ->
            promoteStaged(promoteState, promoteArtifact)
        }
    }

    private fun promoteStaged(state: ArmedState, staged: StagedArtifact) {
        val request = state.request
        val freeRasterRequest = request.copy(
            purpose = DrawUnifiedOutputRasterRequestV01.Purpose.FREE_RASTER_VIEW,
            targetCoordinatesCreateMeasuredEvidence = false,
            scientificWritebackAllowed = false,
            sourceMutationAllowed = false,
        )

        val siblingMismatch = DrawUnifiedOutputRasterContractV01
            .validateJpegFreeRasterSiblingBasis(request, freeRasterRequest)
        if (siblingMismatch != null) {
            UnifiedOutputFreeRasterBridge.clear("sibling_basis_rejected")
            finishPromotion(state, staged)
            return
        }

        val context = appContext
        if (context == null) {
            UnifiedOutputFreeRasterBridge.clear("runtime_context_missing")
            finishPromotion(state, staged)
            return
        }

        val outputDirectory = File(
            context.filesDir,
            "free_raster_output/${request.sourceJobId}",
        )
        val result = UnifiedOutputFreeRasterBridge.publishRenderedJpeg(
            renderedFile = staged.file,
            freeRasterRequest = freeRasterRequest,
            expectedSha256 = staged.sha256,
            artifactDirectory = outputDirectory,
        )
        if (result is UnifiedOutputFreeRasterPublishResultV01.Failed) {
            UnifiedOutputFreeRasterBridge.clear("full_resolution_publish_failed")
        }
        finishPromotion(state, staged)
    }

    private fun finishPromotion(state: ArmedState, staged: StagedArtifact) {
        staged.file.delete()
        synchronized(lock) {
            if (armed?.generation == state.generation) {
                cleanupLocked("promotion_complete")
            }
        }
    }

    private fun isExpired(state: ArmedState): Boolean =
        SystemClock.elapsedRealtime() - state.startedAtElapsedMs > MAX_ARM_AGE_MS

    private fun cleanupLocked(reason: String) {
        val previous = armed
        armed = null
        previous?.observer?.stopWatching()
        previous?.staged?.file?.delete()
        @Suppress("UNUSED_VARIABLE")
        val ignoredReason = reason
    }
}
