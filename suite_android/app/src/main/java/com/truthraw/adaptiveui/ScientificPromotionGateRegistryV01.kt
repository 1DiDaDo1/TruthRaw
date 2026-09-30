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
