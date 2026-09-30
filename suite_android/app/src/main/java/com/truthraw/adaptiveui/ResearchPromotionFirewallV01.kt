package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Mechanical promotion firewall for research foundation exports.
 */
object ResearchPromotionFirewallV01 {
    const val SCHEMA = "D.RAW/ResearchPromotionFirewall/0.1"

    private val forbiddenTrueKeys =
        listOf(
            "world_registration_promoted",
            "world_registration_proven",
            "registration_promoted",
            "world_point_estimate_promoted",
            "same_world_structure_proven",
            "same_physical_world_point_proven",
            "component_is_same_physical_scene_proven",
            "scientific_model_promoted",
            "camera_system_response_proven",
            "lens_only_vignetting_proven",
            "calibration_promoted",
            "radiometric_calibration_promoted",
            "noise_component_calibration_promoted",
            "geometry_promoted",
            "correction_authorized",
            "deconvolution_authorized",
            "deconvolution_applied",
            "inverse_optics_authorized",
            "automatic_colour_correction_applied",
            "automatic_radiometric_correction_applied",
            "noise_reduction_applied",
            "world_space_denoise_applied",
            "light_transport_applied_to_scientific_master",
            "temporal_fusion_applied",
            "multi_frame_scientific_fusion_applied",
            "restoration_applied",
            "scientific_writeback_allowed",
            "creates_new_evidence",
        )

    fun audit(report: JSONObject): JSONObject {
        val violations = JSONArray()
        scanObject(report, "$", violations)

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (violations.length() == 0) {
                    "RESEARCH_PROMOTION_FIREWALL_PASS"
                } else {
                    "RESEARCH_PROMOTION_FIREWALL_BLOCK"
                },
            )
            .put("forbidden_true_keys", JSONArray(forbiddenTrueKeys))
            .put("violations", violations)
            .put(
                "promotion_allowed",
                false,
            )
            .put(
                "export_safe_under_current_research_contract",
                violations.length() == 0,
            )
            .put("scientific_writeback_allowed", false)
    }

    private fun scanObject(
        obj: JSONObject,
        path: String,
        violations: JSONArray,
    ) {
        val keys = obj.keys()
        while (keys.hasNext()) {
            val key = keys.next()
            val value = obj.opt(key)
            val childPath = "$path.$key"

            if (
                key in forbiddenTrueKeys &&
                value is Boolean &&
                value
            ) {
                violations.put(
                    JSONObject()
                        .put("path", childPath)
                        .put("reason", "FORBIDDEN_RESEARCH_PROMOTION_TRUE"),
                )
            }

            when (value) {
                is JSONObject ->
                    scanObject(value, childPath, violations)
                is JSONArray ->
                    scanArray(value, childPath, violations)
            }
        }
    }

    private fun scanArray(
        array: JSONArray,
        path: String,
        violations: JSONArray,
    ) {
        for (i in 0 until array.length()) {
            when (val value = array.opt(i)) {
                is JSONObject ->
                    scanObject(value, "$path[$i]", violations)
                is JSONArray ->
                    scanArray(value, "$path[$i]", violations)
            }
        }
    }
}
