package com.truthraw.adaptiveui

import org.json.JSONObject

/**
 * One non-promoting summary of what the current appearance-geometry machinery
 * can already compute, and what still requires physical validation.
 */
object GeometryValidationReadinessV01 {
    const val SCHEMA = "D.RAW/GeometryValidationReadiness/0.1"

    fun describe(
        graph: JSONObject,
        tracks: JSONObject,
        cycles: JSONObject,
        trackProjection: JSONObject,
    ): JSONObject {
        val candidateEdges =
            graph.optInt(
                "geometry_candidate_edge_count",
                0,
            )
        val multiTracks =
            tracks.optInt(
                "multi_observation_track_count",
                0,
            )
        val threePlusTracks =
            tracks.optInt(
                "three_plus_observation_track_count",
                0,
            )
        val conflicts =
            tracks.optInt(
                "same_observation_feature_conflict_track_count",
                0,
            )
        val cycleCount =
            cycles.optInt(
                "closed_triangle_count",
                0,
            )
        val projectedTracks =
            trackProjection.optInt(
                "projected_track_count",
                0,
            )

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "GEOMETRY_VALIDATION_MACHINERY_AVAILABLE",
            )
            .put(
                "available_candidate_evidence",
                JSONObject()
                    .put(
                        "pair_geometry_candidate_edges",
                        candidateEdges,
                    )
                    .put(
                        "multi_observation_feature_tracks",
                        multiTracks,
                    )
                    .put(
                        "three_plus_observation_feature_tracks",
                        threePlusTracks,
                    )
                    .put(
                        "closed_three_observation_cycles",
                        cycleCount,
                    )
                    .put(
                        "relative_world_projected_tracks",
                        projectedTracks,
                    )
                    .put(
                        "track_conflicts",
                        conflicts,
                    ),
            )
            .put(
                "descriptive_metrics",
                JSONObject()
                    .put(
                        "median_cycle_closure_rms_normalized",
                        cycles.opt(
                            "median_cycle_closure_rms_normalized",
                        ) ?: JSONObject.NULL,
                    )
                    .put(
                        "median_track_rms_dispersion",
                        trackProjection.opt(
                            "median_track_rms_dispersion",
                        ) ?: JSONObject.NULL,
                    ),
            )
            .put(
                "future_validation_gates",
                JSONObject()
                    .put(
                        "overlapping_scene_pair_validation_required",
                        true,
                    )
                    .put(
                        "cross_focal_length_validation_required",
                        true,
                    )
                    .put(
                        "rotation_sequence_validation_required",
                        true,
                    )
                    .put(
                        "graph_loop_consistency_validation_required",
                        true,
                    )
                    .put(
                        "held_out_observation_validation_required",
                        true,
                    )
                    .put(
                        "world_vs_sensor_field_separation_validation_required",
                        true,
                    ),
            )
            .put(
                "current_authority",
                JSONObject()
                    .put(
                        "automatic_numeric_threshold_used",
                        false,
                    )
                    .put(
                        "same_world_structure_proven",
                        false,
                    )
                    .put(
                        "camera_pose_proven",
                        false,
                    )
                    .put(
                        "absolute_world_scale_proven",
                        false,
                    )
                    .put(
                        "world_registration_promoted",
                        false,
                    )
                    .put(
                        "calibration_promoted",
                        false,
                    )
                    .put(
                        "correction_authorized",
                        false,
                    ),
            )
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}
