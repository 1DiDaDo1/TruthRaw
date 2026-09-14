package com.truthraw.adaptiveui

enum class TruthRawOutputMode(
    val wireName: String,
    val titleRes: Int,
    val subtitleRes: Int,
    val supportsAppearanceOptions: Boolean,
) {
    JPG("JPG", R.string.mode_jpg, R.string.mode_jpg_subtitle, true),
    JPG_XL("JPG_XL", R.string.mode_jxl, R.string.mode_jxl_subtitle, true),
    TRUTHRAW_PURE("TRUTHRAW_PURE", R.string.mode_pure, R.string.mode_pure_subtitle, false),
    TRUTHRAW_ADVANCED("TRUTHRAW_ADVANCED", R.string.mode_advanced, R.string.mode_advanced_subtitle, true),
}

data class AppearanceIntent(
    val colourful: Boolean = false,
    val detailed: Boolean = false,
    val soft: Boolean = false,
    val hdr: Boolean = false,
) {
    fun isNeutral(): Boolean = !colourful && !detailed && !soft && !hdr
}

data class OutputModeSelection(
    val mode: TruthRawOutputMode,
    val appearance: AppearanceIntent,
)

/**
 * Presentation/export policy only.
 *
 * This contract is deliberately downstream of sealed evidence, the Scientific
 * Master, TruthRange, zero-line, evidence counts and Technical Backplane state.
 * Selecting a user-facing mode can never create scientific authority.
 */
object OutputModePolicy {
    const val JPEG_XL_ENCODER_VALIDATED: Boolean = false

    fun selection(
        mode: TruthRawOutputMode,
        colourful: Boolean,
        detailed: Boolean,
        soft: Boolean,
        hdr: Boolean,
    ): OutputModeSelection {
        val requested = AppearanceIntent(colourful, detailed, soft, hdr)
        return OutputModeSelection(
            mode = mode,
            appearance = if (mode.supportsAppearanceOptions) requested else AppearanceIntent(),
        )
    }

    fun fromWireName(
        wireName: String?,
        colourful: Boolean = false,
        detailed: Boolean = false,
        soft: Boolean = false,
        hdr: Boolean = false,
    ): OutputModeSelection {
        val mode = TruthRawOutputMode.entries.firstOrNull { it.wireName == wireName }
            ?: TruthRawOutputMode.TRUTHRAW_PURE
        return selection(mode, colourful, detailed, soft, hdr)
    }

    fun allowsJpeg(selection: OutputModeSelection): Boolean =
        selection.mode == TruthRawOutputMode.JPG ||
            selection.mode == TruthRawOutputMode.TRUTHRAW_ADVANCED

    fun allowsJpegXl(selection: OutputModeSelection): Boolean =
        JPEG_XL_ENCODER_VALIDATED &&
            (selection.mode == TruthRawOutputMode.JPG_XL ||
                selection.mode == TruthRawOutputMode.TRUTHRAW_ADVANCED)

    fun allowedRawProjectionKinds(selection: OutputModeSelection): Set<RawProjectionKind> = when (selection.mode) {
        TruthRawOutputMode.JPG,
        TruthRawOutputMode.JPG_XL -> emptySet()
        TruthRawOutputMode.TRUTHRAW_PURE -> setOf(RawProjectionKind.TRUTHRAW_PURE_FLOAT32_DNG)
        TruthRawOutputMode.TRUTHRAW_ADVANCED -> RawProjectionKind.entries.toSet()
    }

    fun scientificAuthorityMayChange(@Suppress("UNUSED_PARAMETER") selection: OutputModeSelection): Boolean = false
}
