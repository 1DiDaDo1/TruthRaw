package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Keeps 2D correspondence, parallax/depth, visibility and 3D authority
 * separate. Existing appearance geometry may seed candidates but cannot prove
 * a metric scene.
 */
object GeometryDepthSupportAtlasV01 {
    const val SCHEMA = "D.RAW/GeometryDepthSupportAtlas/0.1"

    fun describe(
        graph: JSONObject,
        tracks: JSONObject,
        cycles: JSONObject,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "GEOMETRY_DEPTH_SUPPORT_CONTRACT_AVAILABLE")
            .put(
                "current_2d_support",
                JSONObject()
                    .put("candidate_edge_count", graph.optInt("geometry_candidate_edge_count", 0))
                    .put("track_hypothesis_count", tracks.optInt("track_count", 0))
                    .put("closed_cycle_count", cycles.optInt("closed_triangle_count", 0))
                    .put("authority", "APPEARANCE_DERIVED_ONLY"),
            )
            .put(
                "future_geometry_axes",
                JSONArray()
                    .put("INTRINSIC_RAY_MODEL")
                    .put("CAMERA_POSE")
                    .put("PARALLAX")
                    .put("DEPTH")
                    .put("SURFACE_NORMAL")
                    .put("VISIBILITY")
                    .put("OCCLUSION")
                    .put("NON_RIGID_STATE"),
            )
            .put("intrinsic_ray_model_validated", false)
            .put("metric_pose_validated", false)
            .put("parallax_depth_estimated", false)
            .put("visibility_graph_estimated", false)
            .put("same_physical_world_point_proven", false)
            .put("photogrammetric_geometry_upgrades_radiometric_authority", false)
            .put("counterfactual_animation_state_is_measured_evidence", false)
            .put("geometry_promoted", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}
