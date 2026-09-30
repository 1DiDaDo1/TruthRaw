package com.truthraw.adaptiveui

/**
 * Runtime-safe default implementation of the Free World query ABI.
 *
 * Until a validated world-to-source relation and a scientific continuous
 * reconstruction operator are explicitly admitted, every world query returns
 * UNKNOWN rather than interpolating or inventing support.
 */
class FailClosedFreeWorldContinuousQuerySolverV01 :
    FreeWorldContinuousQuerySolverV01 {

    override fun query(
        request: FreeWorldContinuousQueryRequestV01,
    ): FreeWorldContinuousQueryResultV01 =
        FreeWorldContinuousQueryResultV01(
            values = null,
            valueDomain = "UNKNOWN",
            authority = FreeWorldAuthorityV01.UNKNOWN,
            sourceSupport = emptyList(),
            reconstructionSupport = emptyList(),
            uncertaintyAxes =
                listOf(
                    FreeWorldAxisUncertaintyV01(
                        axis = "WORLD_TO_SOURCE_RELATION",
                        sigmaOrBound = null,
                        units = null,
                        status =
                            "UNKNOWN_NO_ADMITTED_WORLD_TO_SOURCE_BRIDGE",
                    ),
                    FreeWorldAxisUncertaintyV01(
                        axis = "RADIOMETRIC_RESPONSE",
                        sigmaOrBound = null,
                        units = null,
                        status =
                            "UNKNOWN_NO_PROMOTED_RADIOMETRIC_RESPONSE",
                    ),
                    FreeWorldAxisUncertaintyV01(
                        axis = "NOISE_COMPONENT_AND_COVARIANCE",
                        sigmaOrBound = null,
                        units = null,
                        status =
                            "UNKNOWN_NO_PROMOTED_NOISE_COMPONENT_MODEL",
                    ),
                    FreeWorldAxisUncertaintyV01(
                        axis = "OPTICAL_FREQUENCY_SUPPORT",
                        sigmaOrBound = null,
                        units = null,
                        status =
                            "UNKNOWN_NO_PROMOTED_OPTICAL_SUPPORT",
                    ),
                    FreeWorldAxisUncertaintyV01(
                        axis = "GEOMETRY_DEPTH_VISIBILITY",
                        sigmaOrBound = null,
                        units = null,
                        status =
                            "UNKNOWN_NO_PROMOTED_3D_GEOMETRY",
                    ),
                    FreeWorldAxisUncertaintyV01(
                        axis = "CONTINUOUS_RECONSTRUCTION",
                        sigmaOrBound = null,
                        units = null,
                        status =
                            "UNKNOWN_NO_ADMITTED_CONTINUOUS_PIXEL_SOLVER",
                    ),
                ),
            censorLowerBound = null,
            censorUpperBound = null,
            provenanceSourceSha256 = emptyList(),
            spatialFootprintDescription =
                "UNKNOWN_NO_ADMITTED_WORLD_TO_SOURCE_BRIDGE",
            temporalFootprintDescription =
                if (request.timeSeconds == null) {
                    "UNSPECIFIED_QUERY_TIME"
                } else {
                    "REQUESTED_TIME_WITHOUT_ADMITTED_TEMPORAL_RELATION"
                },
        )
}
