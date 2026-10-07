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
    const val CONTRACT_VERSION = "DrawUnifiedOutputRaster/0.3"

    /**
     * Outer evidence-law validation. A valid JPEG_EXPORT request also arms the
     * presentation-only Free Raster observer. Arming has no influence on the
     * validation result or JPEG exporter; inability to arm only means no
     * full-resolution Free Raster handoff will be available.
     */
    fun validate(
        request: DrawUnifiedOutputRasterRequestV01,
        activeJobId: String?,
    ): DrawUnifiedOutputRasterBindResultV01 {
        val result = validateCore(request, activeJobId)
        if (
            result is DrawUnifiedOutputRasterBindResultV01.Ready &&
            request.purpose == DrawUnifiedOutputRasterRequestV01.Purpose.JPEG_EXPORT
        ) {
            UnifiedOutputFreeRasterRuntimeV01.armValidatedJpegRequest(request)
        }
        return result
    }

    /**
     * Pure contract check used by sibling comparisons so revalidation during a
     * later presentation promotion can never re-arm the renderer observer.
     */
    private fun validateCore(
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

    /**
     * Existing JPEG helper retained for compatibility. JPEG and Free Raster are
     * siblings of one frozen full-resolution output binding; neither is derived
     * from TilePreviewUiState or a UI Bitmap.
     */
    fun fromFullResolutionJpegBinding(
        binding: DrawPhotoOutputBindingV01,
        targetWidth: Int,
        targetHeight: Int,
    ): DrawUnifiedOutputRasterRequestV01 =
        fromFullResolutionBinding(
            binding = binding,
            targetWidth = targetWidth,
            targetHeight = targetHeight,
            purpose = DrawUnifiedOutputRasterRequestV01.Purpose.JPEG_EXPORT,
        )

    fun fromFreeRasterBinding(
        binding: DrawPhotoOutputBindingV01,
        targetWidth: Int,
        targetHeight: Int,
    ): DrawUnifiedOutputRasterRequestV01 =
        fromFullResolutionBinding(
            binding = binding,
            targetWidth = targetWidth,
            targetHeight = targetHeight,
            purpose = DrawUnifiedOutputRasterRequestV01.Purpose.FREE_RASTER_VIEW,
        )

    private fun fromFullResolutionBinding(
        binding: DrawPhotoOutputBindingV01,
        targetWidth: Int,
        targetHeight: Int,
        purpose: DrawUnifiedOutputRasterRequestV01.Purpose,
    ): DrawUnifiedOutputRasterRequestV01 =
        DrawUnifiedOutputRasterRequestV01(
            sourceJobId = binding.sourceJobId,
            sourceUri = binding.sourceUri,
            route = binding.route,
            routeFlags = binding.routeFlags,
            targetWidth = targetWidth,
            targetHeight = targetHeight,
            purpose = purpose,
            userQuarterTurns = binding.userQuarterTurns,
            targetCoordinatesCreateMeasuredEvidence = false,
            scientificWritebackAllowed = binding.scientificWritebackAllowed,
            sourceMutationAllowed = binding.sourceMutationAllowed,
        )

    /**
     * Proves that JPEG export and Free Raster address the exact same frozen
     * downstream raster basis. This is deliberately a presentation/output
     * identity check, not a scientific promotion check.
     */
    fun validateJpegFreeRasterSiblingBasis(
        jpeg: DrawUnifiedOutputRasterRequestV01,
        freeRaster: DrawUnifiedOutputRasterRequestV01,
    ): String? {
        if (jpeg.purpose != DrawUnifiedOutputRasterRequestV01.Purpose.JPEG_EXPORT) {
            return "Unified output geblokkeerd: JPEG sibling heeft onjuist purpose."
        }
        if (freeRaster.purpose != DrawUnifiedOutputRasterRequestV01.Purpose.FREE_RASTER_VIEW) {
            return "Unified output geblokkeerd: Free Raster sibling heeft onjuist purpose."
        }

        val jpegValidation = validateCore(jpeg, jpeg.sourceJobId)
        if (jpegValidation is DrawUnifiedOutputRasterBindResultV01.Failed) {
            return jpegValidation.reason
        }
        val freeRasterValidation = validateCore(freeRaster, freeRaster.sourceJobId)
        if (freeRasterValidation is DrawUnifiedOutputRasterBindResultV01.Failed) {
            return freeRasterValidation.reason
        }

        if (jpeg.sourceJobId != freeRaster.sourceJobId ||
            jpeg.sourceUri != freeRaster.sourceUri
        ) {
            return "Unified output geblokkeerd: sibling source-observation verschilt."
        }
        if (jpeg.route != freeRaster.route ||
            jpeg.routeFlags != freeRaster.routeFlags
        ) {
            return "Unified output geblokkeerd: sibling outputroute/appearance verschilt."
        }
        if (jpeg.targetWidth != freeRaster.targetWidth ||
            jpeg.targetHeight != freeRaster.targetHeight
        ) {
            return "Unified output geblokkeerd: sibling rastergeometrie verschilt."
        }
        if (jpeg.userQuarterTurns != freeRaster.userQuarterTurns) {
            return "Unified output geblokkeerd: sibling oriëntatie verschilt."
        }
        if (jpeg.targetCoordinatesCreateMeasuredEvidence !=
                freeRaster.targetCoordinatesCreateMeasuredEvidence ||
            jpeg.scientificWritebackAllowed != freeRaster.scientificWritebackAllowed ||
            jpeg.sourceMutationAllowed != freeRaster.sourceMutationAllowed
        ) {
            return "Unified output geblokkeerd: sibling evidence/writeback-contract verschilt."
        }
        return null
    }
}
