package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Registry separating candidate machinery from future scientific promotion.
 * Requirements are descriptive gates, not automatic pass criteria.
 */
object ScientificPromotionGateRegistryV01 {
    const val SCHEMA = "D.RAW/ScientificPromotionGateRegistry/0.1"

    fun describe(): JSONObject {
        val entries = JSONArray()

        fun entry(
            target: String,
            prerequisites: List<String>,
        ) {
            val req = JSONArray()
            prerequisites.forEach(req::put)
            entries.put(
                JSONObject()
                    .put("target", target)
                    .put("prerequisites", req)
                    .put("automatic_promotion_allowed", false)
                    .put("current_status", "NOT_PROMOTED"),
            )
        }

        entry(
            "WORLD_REGISTRATION",
            listOf(
                "VALIDATED_PAIR_GEOMETRY",
                "MULTI_OBSERVATION_TRACK_CONSISTENCY",
                "GRAPH_LOOP_CONSISTENCY",
                "HELD_OUT_OBSERVATION_VALIDATION",
                "AXIS_SEPARATED_UNCERTAINTY",
            ),
        )
        entry(
            "WORLD_TO_SOURCE_BRIDGE",
            listOf(
                "WORLD_REGISTRATION_VALIDATED",
                "EXPLICIT_SOURCE_COORDINATE_MAPPING",
                "MAPPING_UNCERTAINTY",
                "NO_APPEARANCE_ONLY_AUTHORITY_UPGRADE",
            ),
        )
        entry(
            "RADIOMETRIC_RESPONSE_CALIBRATION",
            listOf(
                "CONTROLLED_EXPOSURE_RELATION",
                "EFFECTIVE_GAIN_ESTIMATE",
                "BLACK_OFFSET_SEPARATED_FROM_ZERO_LINE",
                "SATURATION_CENSOR_BEHAVIOR",
                "HELD_OUT_VALIDATION",
                "UNCERTAINTY",
            ),
        )
        entry(
            "FIELD_RESPONSE_CALIBRATION",
            listOf(
                "MULTIPLE_MEASURED_FIELD_OBSERVATIONS",
                "WORLD_VS_SENSOR_SEPARATION",
                "SCENE_ILLUMINATION_NOT_CONFLATED",
                "HELD_OUT_VALIDATION",
            ),
        )
        entry(
            "COLOUR_CALIBRATION",
            listOf(
                "REFERENCE_TARGET",
                "AT_LEAST_TWO_CHARACTERIZED_ILLUMINANTS",
                "HELD_OUT_VALIDATION",
                "UNCERTAINTY",
            ),
        )
        entry(
            "OPTICAL_SUPPORT_CALIBRATION",
            listOf(
                "CONTROLLED_SFR_MTF_PSF_OR_EQUIVALENT",
                "FIELD_COORDINATES",
                "FOCUS_STATE",
                "UNCERTAINTY",
                "HELD_OUT_VALIDATION",
            ),
        )
        entry(
            "DARK_NOISE_CALIBRATION",
            listOf(
                "INDEPENDENT_DARK_NOISE_OBSERVATIONS",
                "EXPOSURE_GAIN_CONTEXT",
                "UNCERTAINTY",
                "HELD_OUT_VALIDATION",
            ),
        )
        entry(
            "NOISE_COMPONENT_CALIBRATION",
            listOf(
                "INDEPENDENT_REPEATED_OBSERVATIONS",
                "DARK_AND_SIGNAL_CONTEXT_WHERE_APPLICABLE",
                "TEMPORAL_VS_FIXED_PATTERN_SEPARATION",
                "CFA_PHASE_AND_SPATIAL_CORRELATION_ACCOUNTED_FOR",
                "RADIOMETRIC_CONTEXT",
                "HELD_OUT_VALIDATION",
                "UNCERTAINTY",
            ),
        )
        entry(
            "GEOMETRY_DEPTH_VISIBILITY",
            listOf(
                "VALIDATED_WORLD_RELATION_GEOMETRY",
                "PARALLAX_OR_EQUIVALENT_DEPTH_CONSTRAINT",
                "VISIBILITY_OR_OCCLUSION_UNCERTAINTY",
                "HELD_OUT_OBSERVATION_VALIDATION",
                "NO_RADIOMETRIC_AUTHORITY_UPGRADE",
            ),
        )
        entry(
            "WORLD_SPACE_NOISE_SEPARATION",
            listOf(
                "VALIDATED_WORLD_TO_SOURCE_BRIDGE",
                "NOISE_COMPONENT_CALIBRATION",
                "MULTIPLE_INDEPENDENT_OBSERVATIONS",
                "SENSOR_POSITION_DIVERSITY",
                "TEMPORAL_AND_VIEW_DEPENDENCE_ACCOUNTED_FOR",
                "UNKNOWN_RESIDUAL_PRESERVED",
                "HELD_OUT_VALIDATION",
            ),
        )
        entry(
            "TEMPORAL_RELATION",
            listOf(
                "SOURCE_BOUND_TIMING_EVIDENCE",
                "PHYSICAL_SEQUENCE_RELATION",
                "UNCERTAINTY",
                "HELD_OUT_VALIDATION",
            ),
        )
        entry(
            "CONTINUOUS_FREE_WORLD_SOLVER",
            listOf(
                "VALIDATED_WORLD_TO_SOURCE_BRIDGE",
                "EXACT_MEASURED_ANCHOR_PRESERVATION",
                "RECONSTRUCTION_AUTHORITY",
                "RADIOMETRIC_RESPONSE_CONTEXT",
                "NOISE_UNCERTAINTY_TRANSPORT",
                "SPATIAL_FOOTPRINT",
                "TEMPORAL_FOOTPRINT",
                "UNCERTAINTY",
                "CENSOR_BOUNDS",
            ),
        )
        entry(
            "SCIENTIFIC_CORRECTION_OR_WRITEBACK",
            listOf(
                "AXIS_SPECIFIC_PROMOTION_ALREADY_GRANTED",
                "REVERSIBLE_VERSIONED_TRANSFORM",
                "SOURCE_PROVENANCE_PRESERVED",
                "NO_MEASURED_SAMPLE_MUTATION",
                "SEPARATE_EXPLICIT_APPROVAL_GATE",
            ),
        )

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "PROMOTION_GATE_REGISTRY_AVAILABLE")
            .put("entries", entries)
            .put("registry_itself_grants_promotion", false)
            .put("automatic_promotion_allowed", false)
            .put("scientific_writeback_allowed", false)
            .put("creates_new_evidence", false)
    }
}
