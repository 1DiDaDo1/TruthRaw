package com.truthraw.adaptiveui

/**
 * Typed ABI for the future raster-independent Free World solver.
 *
 * No implementation is provided here. The interface exists so future
 * reconstruction code cannot return a bare RGB value without authority,
 * uncertainty, support and provenance.
 */
enum class FreeWorldAuthorityV01 {
    MEASURED,
    CALIBRATED_ESTIMATE,
    RECONSTRUCTED,
    CENSORED,
    UNKNOWN,
    APPEARANCE_ONLY,
}

data class FreeWorldContinuousQueryRequestV01(
    val worldX: Double,
    val worldY: Double,
    val timeSeconds: Double? = null,
    val viewDirectionX: Double? = null,
    val viewDirectionY: Double? = null,
    val viewDirectionZ: Double? = null,
    val requestedFootprintWidth: Double? = null,
    val requestedFootprintHeight: Double? = null,
)

data class FreeWorldSupportReferenceV01(
    val sourceSha256: String,
    val sourceX: Double?,
    val sourceY: Double?,
    val supportWeight: Double?,
    val measured: Boolean,
)

data class FreeWorldAxisUncertaintyV01(
    val axis: String,
    val sigmaOrBound: Double?,
    val units: String?,
    val status: String,
)

data class FreeWorldContinuousQueryResultV01(
    val values: DoubleArray?,
    val valueDomain: String,
    val authority: FreeWorldAuthorityV01,
    val sourceSupport: List<FreeWorldSupportReferenceV01>,
    val reconstructionSupport: List<FreeWorldSupportReferenceV01>,
    val uncertaintyAxes: List<FreeWorldAxisUncertaintyV01>,
    val censorLowerBound: DoubleArray?,
    val censorUpperBound: DoubleArray?,
    val provenanceSourceSha256: List<String>,
    val spatialFootprintDescription: String,
    val temporalFootprintDescription: String,
)

/**
 * Implementations must preserve the contract described by
 * FreeWorldContinuousQueryContractV01.
 */
interface FreeWorldContinuousQuerySolverV01 {
    fun query(
        request: FreeWorldContinuousQueryRequestV01,
    ): FreeWorldContinuousQueryResultV01
}
