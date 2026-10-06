package com.truthraw.adaptiveui

/**
 * Resolution-independent request contract for the downstream D.RAW output cable.
 *
 * The target raster is a representation grid. A denser grid never promotes a
 * target sample to MEASURED and never changes the authority of the sealed CFA
 * samples that support the result.
 *
 * This is deliberately independent from TilePreviewUiState and Bitmap: UI
 * preview, Free Raster and file exporters are sibling consumers of the same
 * upstream output state.
 */
data class DrawUnifiedOutputRasterRequestV01(
    val sourceJobId: String,
    val sourceUri: String,
    val route: String,
    val routeFlags: Int,
    val targetWidth: Int,
    val targetHeight: Int,
    val purpose: Purpose,
    val userQuarterTurns: Int,
    val targetCoordinatesCreateMeasuredEvidence: Boolean = false,
    val scientificWritebackAllowed: Boolean = false,
    val sourceMutationAllowed: Boolean = false,
) {
    enum class Purpose {
        UI_PREVIEW,
        FREE_RASTER_VIEW,
        JPEG_EXPORT,
        PNG_EXPORT,
        OTHER_DERIVED_EXPORT,
    }

    val targetPixelCount: Long
        get() = targetWidth.toLong() * targetHeight.toLong()
}

sealed interface DrawUnifiedOutputRasterBindResultV01 {
    data class Ready(
        val request: DrawUnifiedOutputRasterRequestV01,
    ) : DrawUnifiedOutputRasterBindResultV01

    data class Failed(
        val reason: String,
    ) : DrawUnifiedOutputRasterBindResultV01
}

object DrawUnifiedOutputRasterContractV01 {
    const val CONTRACT_VERSION = "DrawUnifiedOutputRaster/0.1"

    /**
     * Outer evidence-law validation only. It intentionally imposes no fixed
     * product resolution ceiling: memory/codec/export adapters may apply their
     * own capability limits without turning those limits into scientific law.
     */
    fun validate(
        request: DrawUnifiedOutputRasterRequestV01,
        activeJobId: String?,
    ): DrawUnifiedOutputRasterBindResultV01 {
        if (activeJobId == null || request.sourceJobId != activeJobId) {
            return DrawUnifiedOutputRasterBindResultV01.Failed(
                "Output-raster geblokkeerd: actieve sealed observation veranderde.",
            )
        }
        if (request.sourceUri.isBlank()) {
            return DrawUnifiedOutputRasterBindResultV01.Failed(
                "Output-raster geblokkeerd: source-URI ontbreekt.",
            )
        }
        if (request.targetWidth <= 0 || request.targetHeight <= 0) {
            return DrawUnifiedOutputRasterBindResultV01.Failed(
                "Output-raster geblokkeerd: targetresolutie moet positief zijn.",
            )
        }
        if (request.targetPixelCount <= 0L) {
            return DrawUnifiedOutputRasterBindResultV01.Failed(
                "Output-raster geblokkeerd: targetraster overflow/ongeldig.",
            )
        }
        if (request.userQuarterTurns !in 0..3) {
            return DrawUnifiedOutputRasterBindResultV01.Failed(
                "Output-raster geblokkeerd: ongeldige downstream oriëntatie.",
            )
        }
        if (request.route !in setOf(
                TruthRawSuiteLauncherActivity.OUTPUT_PURE,
                TruthRawSuiteLauncherActivity.OUTPUT_ADVANCED,
                TruthRawSuiteLauncherActivity.OUTPUT_PRO,
            )
        ) {
            return DrawUnifiedOutputRasterBindResultV01.Failed(
                "Output-raster geblokkeerd: onbekende outputroute.",
            )
        }
        if (request.route == TruthRawSuiteLauncherActivity.OUTPUT_PURE &&
            request.routeFlags != 0
        ) {
            return DrawUnifiedOutputRasterBindResultV01.Failed(
                "PURE output-raster mag geen ADVANCED/PRO appearance-flags dragen.",
            )
        }
        if (request.targetCoordinatesCreateMeasuredEvidence ||
            request.scientificWritebackAllowed ||
            request.sourceMutationAllowed
        ) {
            return DrawUnifiedOutputRasterBindResultV01.Failed(
                "Output-raster geblokkeerd: evidence/writeback safety-contract mismatch.",
            )
        }

        return DrawUnifiedOutputRasterBindResultV01.Ready(request)
    }

    fun fromFullResolutionJpegBinding(
        binding: DrawPhotoOutputBindingV01,
        targetWidth: Int,
        targetHeight: Int,
    ): DrawUnifiedOutputRasterRequestV01 =
        DrawUnifiedOutputRasterRequestV01(
            sourceJobId = binding.sourceJobId,
            sourceUri = binding.sourceUri,
            route = binding.route,
            routeFlags = binding.routeFlags,
            targetWidth = targetWidth,
            targetHeight = targetHeight,
            purpose = DrawUnifiedOutputRasterRequestV01.Purpose.JPEG_EXPORT,
            userQuarterTurns = binding.userQuarterTurns,
            targetCoordinatesCreateMeasuredEvidence = false,
            scientificWritebackAllowed = binding.scientificWritebackAllowed,
            sourceMutationAllowed = binding.sourceMutationAllowed,
        )
}
