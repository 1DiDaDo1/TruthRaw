package com.truthraw.adaptiveui

import java.util.Locale

/**
 * Vendor-neutral RAW ingress classification.
 *
 * This registry only classifies a user-selected document from filename/MIME.
 * It does not infer camera/sensor identity, CFA semantics, bit depth, calibration
 * authority, or native-ADC provenance.
 *
 * v0.56 processing policy:
 * - strict DNG is wired to the existing Tile-Native DNG source;
 * - proprietary RAW formats may enter the workspace as immutable handles;
 * - unsupported decoder routes fail closed before Scientific Master creation.
 */
enum class RawIngressSupport {
    NATIVE_TRUTHRAW_DNG,
    ACCEPTED_HANDLE_DECODER_PENDING,
    NOT_RECOGNIZED_AS_RAW,
}

enum class RawDecoderBackend {
    MULTIVENDOR_RAW_SOURCE_ADAPTER_V0_1_DNG,
    DECODER_PENDING,
    NONE,
}

data class RawFormatProfile(
    val id: String,
    val displayLabel: String,
    val vendorLabel: String,
    val extensions: Set<String>,
    val mimeTypes: Set<String>,
    val support: RawIngressSupport,
    val decoderBackend: RawDecoderBackend,
) {
    val nativeProcessingReady: Boolean
        get() = support == RawIngressSupport.NATIVE_TRUTHRAW_DNG &&
            decoderBackend == RawDecoderBackend.MULTIVENDOR_RAW_SOURCE_ADAPTER_V0_1_DNG
}

object RawFormatRegistry {
    private fun extSet(vararg values: String): Set<String> =
        values.map { it.lowercase(Locale.ROOT) }.toSet()

    private fun mimeSet(vararg values: String): Set<String> =
        values.map { it.lowercase(Locale.ROOT) }.toSet()

    private fun pending(
        id: String,
        label: String,
        vendor: String,
        vararg extensions: String,
    ) = RawFormatProfile(
        id = id,
        displayLabel = label,
        vendorLabel = vendor,
        extensions = extSet(*extensions),
        mimeTypes = emptySet(),
        support = RawIngressSupport.ACCEPTED_HANDLE_DECODER_PENDING,
        decoderBackend = RawDecoderBackend.DECODER_PENDING,
    )

    private val profiles = listOf(
        RawFormatProfile(
            id = "DNG",
            displayLabel = "DNG / LinearRaw / ProRAW",
            vendorLabel = "Multi-vendor",
            extensions = extSet("dng"),
            mimeTypes = mimeSet("image/x-adobe-dng", "image/dng"),
            support = RawIngressSupport.NATIVE_TRUTHRAW_DNG,
            decoderBackend = RawDecoderBackend.MULTIVENDOR_RAW_SOURCE_ADAPTER_V0_1_DNG,
        ),
        pending("CANON_CR3", "Canon CR3", "Canon", "cr3"),
        pending("CANON_CR2", "Canon CR2", "Canon", "cr2"),
        pending("NIKON_NEF", "Nikon NEF", "Nikon", "nef"),
        pending("NIKON_NRW", "Nikon NRW", "Nikon", "nrw"),
        pending("SONY_ARW", "Sony ARW", "Sony", "arw"),
        pending("FUJIFILM_RAF", "Fujifilm RAF", "Fujifilm", "raf"),
        pending("PANASONIC_RW2", "Panasonic RW2", "Panasonic / Lumix", "rw2"),
        pending("OLYMPUS_ORF", "Olympus / OM System ORF", "Olympus / OM System", "orf"),
        pending("PENTAX_PEF", "Pentax PEF", "Pentax / Ricoh", "pef"),
        pending("LEICA_RWL", "Leica RWL", "Leica", "rwl"),
        pending("HASSELBLAD_3FR", "Hasselblad 3FR", "Hasselblad", "3fr"),
        pending("HASSELBLAD_FFF", "Hasselblad FFF", "Hasselblad", "fff"),
        pending("PHASE_ONE_IIQ", "Phase One IIQ", "Phase One", "iiq"),
        pending("SIGMA_X3F", "Sigma X3F", "Sigma", "x3f"),
        pending("SAMSUNG_SRW", "Samsung SRW", "Samsung", "srw"),
        pending("EPSON_ERF", "Epson ERF", "Epson", "erf"),
        pending("KODAK_DCR_KDC", "Kodak RAW", "Kodak", "dcr", "kdc"),
        pending("MINOLTA_MRW", "Minolta MRW", "Minolta", "mrw"),
        pending("MAMIYA_MEF", "Mamiya MEF", "Mamiya", "mef"),
        pending("RAW_GENERIC", "Generic RAW", "Unknown", "raw"),
    )

    val nativeExtensions: Set<String> =
        profiles.filter { it.nativeProcessingReady }.flatMap { it.extensions }.toSet()

    val acceptedExtensions: Set<String> =
        profiles.flatMap { it.extensions }.toSet()

    fun classify(displayName: String, mimeType: String?): RawFormatProfile {
        val extension = displayName.substringAfterLast('.', "")
            .lowercase(Locale.ROOT)
            .trim()
        val normalizedMime = mimeType?.lowercase(Locale.ROOT)?.trim()

        profiles.firstOrNull { extension.isNotEmpty() && extension in it.extensions }?.let { return it }
        profiles.firstOrNull { normalizedMime != null && normalizedMime in it.mimeTypes }?.let { return it }

        return RawFormatProfile(
            id = "UNRECOGNIZED",
            displayLabel = "Onbekend bestandstype",
            vendorLabel = "Onbekend",
            extensions = emptySet(),
            mimeTypes = emptySet(),
            support = RawIngressSupport.NOT_RECOGNIZED_AS_RAW,
            decoderBackend = RawDecoderBackend.NONE,
        )
    }
}
