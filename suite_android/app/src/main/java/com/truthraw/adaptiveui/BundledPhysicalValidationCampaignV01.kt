package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Preserves the later physical validation plan as a machine-readable,
 * identity-independent campaign contract.
 *
 * Nothing here executes a correction or promotes a result. The purpose is to
 * avoid losing the intended validation design while the implementation-first
 * architecture wave is completed.
 */
object BundledPhysicalValidationCampaignV01 {
    const val SCHEMA = "D.RAW/BundledPhysicalValidationCampaign/0.1"

    fun describe(): JSONObject {
        val gates = JSONArray()

        fun gate(
            id: String,
            purpose: String,
            minimum: String,
            promotionTarget: String,
        ) {
            gates.put(
                JSONObject()
                    .put("id", id)
                    .put("purpose", purpose)
                    .put("minimum_evidence", minimum)
                    .put("promotion_target", promotionTarget)
                    .put("performed", false)
                    .put("passed", false)
                    .put("promoted", false),
            )
        }

        gate(
            "NATURAL_OVERLAP_SAME_ROUTE",
            "Validate deterministic local features, pair geometry, tracks and loop consistency when world structure moves through one physical capture route.",
            "At least 3 independently sealed overlapping RAW observations with meaningful sensor-position change and one held-out observation.",
            "WORLD_RELATION_GEOMETRY",
        )
        gate(
            "CROSS_OPTICAL_ROUTE_OVERLAP",
            "Validate that ultra-wide/main/tele or other distinct optical routes may share world structure without sharing one field calibration.",
            "At least 3 sealed overlapping RAW observations from distinct optical routes; camera/lens names are not relation keys.",
            "CROSS_ROUTE_WORLD_RELATION",
        )
        gate(
            "ROTATION_SEQUENCE",
            "Validate world-fixed versus source/sensor-fixed behavior under camera rotation.",
            "Original sealed RAW sequence with overlap and known sequence membership; orientation metadata is optional evidence, not identity.",
            "WORLD_SENSOR_FIELD_SEPARATION",
        )
        gate(
            "RAW_360_SEQUENCE",
            "Validate component/graph behavior across a broad rotational sweep while preserving each original RAW as the evidence root.",
            "Original individual RAW observations with overlap; a stitched panorama alone is insufficient.",
            "LARGE_SCALE_RELATIVE_WORLD_GRAPH",
        )
        gate(
            "CONTROLLED_FIELD_ROTATION",
            "Method-validation experiment for source/sensor-fixed field response.",
            "Prefer 0/90/180/270 degree controlled orientations of a sufficiently uniform field, with setup uncertainty recorded.",
            "FIELD_RESPONSE_CALIBRATION_CANDIDATE",
        )
        gate(
            "COLOUR_MULTI_ILLUMINANT",
            "Separate empirical colour relation from illuminant dependence.",
            "Reference target under at least 2 characterized illuminants plus held-out validation.",
            "COLOUR_CALIBRATION_CANDIDATE",
        )
        gate(
            "OPTICAL_SUPPORT",
            "Admit measured optical support rather than inferred sharpness.",
            "Explicit SFR/MTF/PSF or equivalent controlled observations with field coordinates, focus state and uncertainty.",
            "OPTICAL_SUPPORT_CALIBRATION_CANDIDATE",
        )
        gate(
            "DARK_NOISE_OFFSET",
            "Admit dark/noise/offset behavior from physical observations rather than metadata alone.",
            "Independent dark/noise observation set with exposure/gain context and held-out validation.",
            "DARK_NOISE_CALIBRATION_CANDIDATE",
        )
        gate(
            "TEMPORAL_FOOTPRINT",
            "Admit physical sequence timing, motion or rolling-shutter relations.",
            "Source-bound timing evidence and repeated observations whose temporal relation can be independently checked.",
            "TEMPORAL_RELATION_CANDIDATE",
        )

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "VALIDATION_CAMPAIGN_CONTRACT_AVAILABLE")
            .put("normal_user_calibration_required", false)
            .put("camera_or_lens_identity_required", false)
            .put("gates", gates)
            .put(
                "global_rules",
                JSONObject()
                    .put("source_sha256_roots_remain_independent", true)
                    .put("held_out_validation_required_for_promotion", true)
                    .put("automatic_model_winner_used", false)
                    .put("failed_gate_may_be_hidden", false)
                    .put("unknown_is_valid_state", true)
                    .put("one_device_result_is_universal_truth", false)
                    .put("one_scene_result_is_lens_only_truth", false),
            )
            .put("world_registration_promoted", false)
            .put("calibration_promoted", false)
            .put("correction_authorized", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }
}
