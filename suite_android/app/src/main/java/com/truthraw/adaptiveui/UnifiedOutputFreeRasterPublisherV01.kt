package com.truthraw.adaptiveui

import java.io.File

/**
 * Small authority-preserving adapter between the already-tested full-resolution
 * JPEG renderer and the Free Raster file-backed handoff.
 *
 * It does not render. It only proves that JPEG export and Free Raster describe
 * the same frozen downstream raster basis and then publishes the exact rendered
 * artifact for the FREE_RASTER_VIEW sibling consumer.
 */
sealed interface UnifiedOutputFreeRasterPublishAdapterResultV01 {
    data class Ready(
        val jpegRequest: DrawUnifiedOutputRasterRequestV01,
        val freeRasterRequest: DrawUnifiedOutputRasterRequestV01,
        val snapshot: UnifiedOutputFreeRasterSnapshotV01,
    ) : UnifiedOutputFreeRasterPublishAdapterResultV01

    data class Failed(
        val reason: String,
    ) : UnifiedOutputFreeRasterPublishAdapterResultV01
}

object UnifiedOutputFreeRasterPublisherV01 {
    const val CONTRACT_VERSION = "UnifiedOutputFreeRasterPublisher/0.1"

    fun publishFromRenderedJpeg(
        binding: DrawPhotoOutputBindingV01,
        renderedFile: File,
        outputWidth: Int,
        outputHeight: Int,
        jpegSha256: String,
        artifactDirectory: File,
    ): UnifiedOutputFreeRasterPublishAdapterResultV01 {
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

        val mismatch = DrawUnifiedOutputRasterContractV01
            .validateJpegFreeRasterSiblingBasis(jpegRequest, freeRasterRequest)
        if (mismatch != null) {
            return UnifiedOutputFreeRasterPublishAdapterResultV01.Failed(mismatch)
        }

        return when (
            val publish = UnifiedOutputFreeRasterBridge.publishRenderedJpeg(
                renderedFile = renderedFile,
                freeRasterRequest = freeRasterRequest,
                expectedSha256 = jpegSha256,
                artifactDirectory = artifactDirectory,
            )
        ) {
            is UnifiedOutputFreeRasterPublishResultV01.Failed ->
                UnifiedOutputFreeRasterPublishAdapterResultV01.Failed(publish.reason)
            is UnifiedOutputFreeRasterPublishResultV01.Ready ->
                UnifiedOutputFreeRasterPublishAdapterResultV01.Ready(
                    jpegRequest = jpegRequest,
                    freeRasterRequest = freeRasterRequest,
                    snapshot = publish.snapshot,
                )
        }
    }
}
