package com.truthraw.adaptiveui

import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.security.MessageDigest

/**
 * Process-local handoff for a full-resolution derived presentation artifact.
 *
 * Important authority boundary:
 * - the backing JPEG is DERIVED_PRESENTATION_OUTPUT;
 * - the Free Raster viewport itself remains PRESENTATION_ONLY;
 * - no Bitmap is stored here and no preview raster can become the pixel source;
 * - publishing never creates MEASURED evidence or writes to Scientific Master.
 *
 * The bridge owns a verified private copy so MainActivity may delete the
 * renderer's temporary file after JPEG export without invalidating Free Raster.
 */
data class UnifiedOutputFreeRasterSnapshotV01(
    val generation: Long,
    val request: DrawUnifiedOutputRasterRequestV01,
    val artifactPath: String,
    val artifactSha256: String,
    val artifactBytes: Long,
    val rasterWidth: Int,
    val rasterHeight: Int,
    val mediaType: String = "image/jpeg",
    val artifactAuthority: String = DrawPhotoOutputCableV01.OUTPUT_AUTHORITY,
    val viewportAuthority: String = "PRESENTATION_ONLY",
)

sealed interface UnifiedOutputFreeRasterPublishResultV01 {
    data class Ready(
        val snapshot: UnifiedOutputFreeRasterSnapshotV01,
    ) : UnifiedOutputFreeRasterPublishResultV01

    data class Failed(
        val reason: String,
    ) : UnifiedOutputFreeRasterPublishResultV01
}

object UnifiedOutputFreeRasterBridge {
    const val CONTRACT_VERSION = "UnifiedOutputFreeRasterBridge/0.1"

    @Volatile
    private var current: UnifiedOutputFreeRasterSnapshotV01? = null

    @Volatile
    private var generation: Long = 0L

    @Volatile
    var lastClearReason: String? = null
        private set

    /**
     * Copies one already-rendered full-resolution JPEG into bridge-owned app
     * storage, verifies its SHA-256, then publishes it atomically at the
     * process-state level. The renderer is never invoked here.
     */
    @Synchronized
    fun publishRenderedJpeg(
        renderedFile: File,
        freeRasterRequest: DrawUnifiedOutputRasterRequestV01,
        expectedSha256: String,
        artifactDirectory: File,
    ): UnifiedOutputFreeRasterPublishResultV01 {
        if (freeRasterRequest.purpose !=
            DrawUnifiedOutputRasterRequestV01.Purpose.FREE_RASTER_VIEW
        ) {
            return UnifiedOutputFreeRasterPublishResultV01.Failed(
                "Free Raster publish geblokkeerd: request is geen FREE_RASTER_VIEW sibling.",
            )
        }

        when (
            val validation = DrawUnifiedOutputRasterContractV01.validate(
                freeRasterRequest,
                freeRasterRequest.sourceJobId,
            )
        ) {
            is DrawUnifiedOutputRasterBindResultV01.Failed ->
                return UnifiedOutputFreeRasterPublishResultV01.Failed(validation.reason)
            is DrawUnifiedOutputRasterBindResultV01.Ready -> Unit
        }

        val normalizedExpected = expectedSha256.trim().lowercase()
        if (!normalizedExpected.matches(Regex("[0-9a-f]{64}"))) {
            return UnifiedOutputFreeRasterPublishResultV01.Failed(
                "Free Raster publish geblokkeerd: ongeldige artifact-SHA-256.",
            )
        }
        if (!renderedFile.isFile || renderedFile.length() <= 0L) {
            return UnifiedOutputFreeRasterPublishResultV01.Failed(
                "Free Raster publish geblokkeerd: full-resolution artifact ontbreekt.",
            )
        }
        if (!artifactDirectory.exists() && !artifactDirectory.mkdirs()) {
            return UnifiedOutputFreeRasterPublishResultV01.Failed(
                "Free Raster publish geblokkeerd: private artifactmap kon niet worden gemaakt.",
            )
        }
        if (!artifactDirectory.isDirectory) {
            return UnifiedOutputFreeRasterPublishResultV01.Failed(
                "Free Raster publish geblokkeerd: private artifactlocatie is ongeldig.",
            )
        }

        val temp = File(
            artifactDirectory,
            ".free_raster_${freeRasterRequest.sourceJobId.take(16)}_${System.nanoTime()}.tmp",
        )

        val digest = MessageDigest.getInstance("SHA-256")
        val copiedBytes = try {
            FileInputStream(renderedFile).use { input ->
                FileOutputStream(temp).use { output ->
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
        } catch (t: Throwable) {
            temp.delete()
            return UnifiedOutputFreeRasterPublishResultV01.Failed(
                "Free Raster publish faalde tijdens private artifactcopy: ${t.message ?: t.javaClass.simpleName}",
            )
        }

        val copiedSha = digest.digest().joinToString("") { byte -> "%02x".format(byte) }
        if (copiedBytes <= 0L || copiedSha != normalizedExpected) {
            temp.delete()
            return UnifiedOutputFreeRasterPublishResultV01.Failed(
                "Free Raster publish geblokkeerd: artifactcopy SHA-256 mismatch.",
            )
        }

        val finalFile = File(
            artifactDirectory,
            "free_raster_${freeRasterRequest.sourceJobId.take(16)}_${copiedSha.take(16)}.jpg",
        )

        if (finalFile.exists()) {
            if (finalFile.length() != copiedBytes || sha256(finalFile) != copiedSha) {
                temp.delete()
                return UnifiedOutputFreeRasterPublishResultV01.Failed(
                    "Free Raster publish geblokkeerd: bestaand bridge-artifact heeft afwijkende inhoud.",
                )
            }
            temp.delete()
        } else if (!temp.renameTo(finalFile)) {
            temp.delete()
            return UnifiedOutputFreeRasterPublishResultV01.Failed(
                "Free Raster publish faalde: private artifact kon niet atomair worden vastgelegd.",
            )
        }

        generation += 1L
        val snapshot = UnifiedOutputFreeRasterSnapshotV01(
            generation = generation,
            request = freeRasterRequest,
            artifactPath = finalFile.absolutePath,
            artifactSha256 = copiedSha,
            artifactBytes = copiedBytes,
            rasterWidth = freeRasterRequest.targetWidth,
            rasterHeight = freeRasterRequest.targetHeight,
        )
        current = snapshot
        lastClearReason = null
        return UnifiedOutputFreeRasterPublishResultV01.Ready(snapshot)
    }

    /**
     * Acquire never decodes the raster. It only returns the immutable binding
     * when the private artifact still exists with the published byte length.
     */
    @Synchronized
    fun acquire(): UnifiedOutputFreeRasterSnapshotV01? {
        val snapshot = current ?: return null
        val file = File(snapshot.artifactPath)
        if (!file.isFile || file.length() != snapshot.artifactBytes) {
            current = null
            lastClearReason = "free_raster_artifact_missing_or_length_changed"
            return null
        }
        return snapshot
    }

    /**
     * Clears authority/state only. Existing immutable derived files are not
     * deleted here because a Workspace decode may still hold a read handle.
     * Lifecycle cleanup can remove superseded files after consumers release.
     */
    @Synchronized
    fun clear(reason: String) {
        current = null
        generation += 1L
        lastClearReason = reason
    }

    private fun sha256(file: File): String {
        val digest = MessageDigest.getInstance("SHA-256")
        FileInputStream(file).use { input ->
            val buffer = ByteArray(256 * 1024)
            while (true) {
                val count = input.read(buffer)
                if (count < 0) break
                if (count == 0) continue
                digest.update(buffer, 0, count)
            }
        }
        return digest.digest().joinToString("") { byte -> "%02x".format(byte) }
    }
}
