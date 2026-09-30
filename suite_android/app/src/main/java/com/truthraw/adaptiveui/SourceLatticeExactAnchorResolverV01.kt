package com.truthraw.adaptiveui

/**
 * Exact measured-anchor resolver for the raster-independent source lattice.
 *
 * This is deliberately source-lattice only. It does not claim that world
 * registration exists. Unanchored positions remain UNKNOWN.
 */
data class ExactMeasuredAnchorV01(
    val sourceSha256: String,
    val sourceX: Long,
    val sourceY: Long,
    val values: DoubleArray,
    val valueDomain: String,
    val cfaPhase: String?,
    val uncertaintyStatus: String,
)

interface MeasuredAnchorProviderV01 {
    fun sampleAt(
        sourceX: Long,
        sourceY: Long,
    ): ExactMeasuredAnchorV01?
}

enum class SourceLatticeQueryAuthorityV01 {
    MEASURED,
    UNKNOWN,
}

data class SourceLatticeExactAnchorQueryResultV01(
    val latticeU: Long,
    val latticeV: Long,
    val sourceX: Long?,
    val sourceY: Long?,
    val values: DoubleArray?,
    val valueDomain: String?,
    val cfaPhase: String?,
    val sourceSha256: String?,
    val uncertaintyStatus: String,
    val authority: SourceLatticeQueryAuthorityV01,
    val exactMeasuredAnchor: Boolean,
    val interpolationPerformed: Boolean,
    val createsNewEvidence: Boolean,
    val scientificWritebackAllowed: Boolean,
)

object SourceLatticeExactAnchorResolverV01 {
    const val SCHEMA = "D.RAW/SourceLatticeExactAnchorResolver/0.1"

    fun resolve(
        latticeU: Long,
        latticeV: Long,
        provider: MeasuredAnchorProviderV01,
    ): SourceLatticeExactAnchorQueryResultV01 {
        val units =
            RasterIndependentSampleLatticeV01.UNITS_PER_SOURCE_PIXEL

        if (
            latticeU % units != 0L ||
            latticeV % units != 0L
        ) {
            return unknown(
                latticeU = latticeU,
                latticeV = latticeV,
                sourceX = null,
                sourceY = null,
                uncertaintyStatus =
                    "UNANCHORED_LATTICE_POSITION_UNKNOWN",
            )
        }

        val sourceX = latticeU / units
        val sourceY = latticeV / units
        val anchor =
            provider.sampleAt(
                sourceX = sourceX,
                sourceY = sourceY,
            ) ?: return unknown(
                latticeU = latticeU,
                latticeV = latticeV,
                sourceX = sourceX,
                sourceY = sourceY,
                uncertaintyStatus =
                    "NO_ADMITTED_MEASURED_ANCHOR_AT_EXACT_SOURCE_COORDINATE",
            )

        if (
            anchor.sourceX != sourceX ||
            anchor.sourceY != sourceY ||
            anchor.sourceSha256.isBlank()
        ) {
            return unknown(
                latticeU = latticeU,
                latticeV = latticeV,
                sourceX = sourceX,
                sourceY = sourceY,
                uncertaintyStatus =
                    "MEASURED_ANCHOR_PROVIDER_BINDING_MISMATCH",
            )
        }

        return SourceLatticeExactAnchorQueryResultV01(
            latticeU = latticeU,
            latticeV = latticeV,
            sourceX = sourceX,
            sourceY = sourceY,
            values = anchor.values.copyOf(),
            valueDomain = anchor.valueDomain,
            cfaPhase = anchor.cfaPhase,
            sourceSha256 = anchor.sourceSha256,
            uncertaintyStatus = anchor.uncertaintyStatus,
            authority = SourceLatticeQueryAuthorityV01.MEASURED,
            exactMeasuredAnchor = true,
            interpolationPerformed = false,
            createsNewEvidence = false,
            scientificWritebackAllowed = false,
        )
    }

    private fun unknown(
        latticeU: Long,
        latticeV: Long,
        sourceX: Long?,
        sourceY: Long?,
        uncertaintyStatus: String,
    ): SourceLatticeExactAnchorQueryResultV01 =
        SourceLatticeExactAnchorQueryResultV01(
            latticeU = latticeU,
            latticeV = latticeV,
            sourceX = sourceX,
            sourceY = sourceY,
            values = null,
            valueDomain = null,
            cfaPhase = null,
            sourceSha256 = null,
            uncertaintyStatus = uncertaintyStatus,
            authority = SourceLatticeQueryAuthorityV01.UNKNOWN,
            exactMeasuredAnchor = false,
            interpolationPerformed = false,
            createsNewEvidence = false,
            scientificWritebackAllowed = false,
        )
}
