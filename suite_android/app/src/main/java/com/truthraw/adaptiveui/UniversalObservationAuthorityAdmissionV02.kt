package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Prospective multidimensional admission boundary for future deterministic
 * local reconstruction model selection.
 *
 * V0.1 model-bank geometry remains untouched. This contract defines which
 * evidence axes a future selector must keep separate before it may claim that
 * a local model is scientifically admissible. It does not select or apply a
 * model and deliberately allows NO_SUITABLE_MODEL.
 */
object UniversalObservationAuthorityAdmissionV02 {
    const val SCHEMA = "D.RAW/UniversalObservationAuthorityAdmission/0.2"

    fun describe(): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "PROSPECTIVE_AUTHORITY_ADMISSION_CONTRACT_AVAILABLE")
            .put(
                "required_authority_axes",
                JSONArray()
                    .put("CFA_PHASE")
                    .put("STRUCTURE_SUPPORT")
                    .put("DIRECTION_SUPPORT")
                    .put("CENSOR_STATE")
                    .put("RADIOMETRIC_RELIABILITY")
                    .put("RECONSTRUCTION_UNCERTAINTY")
                    .put("OPTICAL_SUPPORT")
                    .put("TEMPORAL_SUPPORT"),
            )
            .put(
                "admission_laws",
                JSONObject()
                    .put("missing_axis_may_upgrade_authority", false)
                    .put("unknown_axis_is_valid_state", true)
                    .put("finite_neighbor_value_implies_reliable_support", false)
                    .put("raster_density_implies_optical_support", false)
                    .put("camera_or_lens_identity_may_fill_missing_axis", false)
                    .put("appearance_quality_may_fill_missing_axis", false)
                    .put("holdout_target_may_drive_pre_reveal_admission", false)
                    .put("no_suitable_model_is_valid_result", true),
            )
            .put("v01_model_bank_replaced", false)
            .put("v01_selection_outcome_changed", false)
            .put("full_authority_admission_performed", false)
            .put("winner_declared", false)
            .put("candidate_applied", false)
            .put("measured_anchors_modified", false)
            .put("unanchored_values_promoted_to_measured", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}
