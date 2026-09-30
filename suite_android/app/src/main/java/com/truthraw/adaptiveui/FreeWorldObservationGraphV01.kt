package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import java.security.MessageDigest

/**
 * Read-only graph over independently sealed observations.
 *
 * Nodes are keyed only by source SHA-256. Pair edges are appearance-derived
 * geometry candidates. No edge merges source evidence or proves camera/lens
 * identity.
 */
object FreeWorldObservationGraphV01 {
    const val SCHEMA = "D.RAW/FreeWorldObservationGraph/0.1"

    fun build(profiles: List<JSONObject>): JSONObject {
        val unique = linkedMapOf<String, JSONObject>()
        for (profile in profiles) {
            val sha = profile.optString("source_sha256")
            if (sha.isNotBlank()) unique.putIfAbsent(sha, profile)
        }

        val nodes = JSONArray()
        for ((sha, profile) in unique) {
            val front =
                profile.optJSONObject("scene_analysis") ?: JSONObject()
            val local =
                front.optJSONObject(
                    "deterministic_local_feature_geometry_v0_1",
                ) ?: JSONObject()
            val field =
                profile.optJSONObject(
                    "observation_optical_field_chart",
                ) ?: JSONObject()
            val measuredField =
                field.optJSONObject(
                    "measured_composite_field_signal",
                ) ?: JSONObject()
            val metadata =
                profile.optJSONObject("source_metadata") ?: JSONObject()

            nodes.put(
                JSONObject()
                    .put("source_sha256", sha)
                    .put(
                        "source_class",
                        profile.optString(
                            "scientific_source_class",
                            "UNKNOWN",
                        ),
                    )
                    .put(
                        "source_route",
                        profile.optString("source_route", "UNKNOWN"),
                    )
                    .put(
                        "frontside_authority",
                        front.optString(
                            "authority",
                            "UNKNOWN",
                        ),
                    )
                    .put(
                        "local_feature_status",
                        local.optString("status", "UNKNOWN"),
                    )
                    .put(
                        "local_feature_count",
                        local.optInt("keypoint_count", 0),
                    )
                    .put(
                        "measured_sensor_field_available",
                        field.optString("status") ==
                            "FIELD_CHART_AVAILABLE" &&
                            measuredField.optString("status") ==
                            "MEASURED_COMPOSITE_FIELD_SIGNAL_AVAILABLE",
                    )
                    .put(
                        "capture_time_text_provenance_hint",
                        metadata.opt(
                            "capture_time_preferred_text",
                        ) ?: JSONObject.NULL,
                    )
                    .put(
                        "focal_length_metadata_hint_mm",
                        metadata.opt("focal_length_mm")
                            ?: JSONObject.NULL,
                    )
                    .put(
                        "camera_or_lens_identity_is_graph_key",
                        false,
                    ),
            )
        }

        val profileList = unique.values.toList()
        val edges = JSONArray()
        var candidateCount = 0
        for (i in profileList.indices) {
            for (j in i + 1 until profileList.size) {
                val left = profileList[i]
                val right = profileList[j]
                val pair =
                    DeterministicLocalFeaturePairGeometryV01.evaluate(
                        left.optJSONObject("scene_analysis")
                            ?: JSONObject()
                                .put(
                                    "source_sha256",
                                    left.optString("source_sha256"),
                                ),
                        right.optJSONObject("scene_analysis")
                            ?: JSONObject()
                                .put(
                                    "source_sha256",
                                    right.optString("source_sha256"),
                                ),
                    )
                if (
                    pair.optString("status") ==
                    "PAIR_GEOMETRY_CANDIDATE_AVAILABLE"
                ) {
                    candidateCount++
                }
                edges.put(
                    JSONObject()
                        .put(
                            "edge_role",
                            "FRONTSIDE_PAIR_GEOMETRY_CANDIDATE",
                        )
                        .put(
                            "left_source_sha256",
                            left.optString("source_sha256"),
                        )
                        .put(
                            "right_source_sha256",
                            right.optString("source_sha256"),
                        )
                        .put(
                            "relation_authority",
                            "APPEARANCE_DERIVED_CANDIDATE_ONLY",
                        )
                        .put("pair_geometry", pair)
                        .put("source_evidence_merged", false)
                        .put(
                            "independent_evidence_count_upgraded",
                            false,
                        )
                        .put(
                            "same_world_structure_proven",
                            false,
                        )
                        .put(
                            "same_physical_camera_proven",
                            false,
                        )
                        .put(
                            "same_physical_lens_proven",
                            false,
                        ),
                )
            }
        }

        val roots = unique.keys.sorted()
        val graphIdentity =
            sha256(
                roots.joinToString("|") +
                    "|candidateEdges=" + candidateCount,
            )

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (unique.size >= 2) {
                    "READ_ONLY_OBSERVATION_GRAPH_AVAILABLE"
                } else {
                    "UNKNOWN_FAIL_CLOSED"
                },
            )
            .put("graph_identity_sha256", graphIdentity)
            .put("graph_identity_is_evidence_root", false)
            .put("observation_count", unique.size)
            .put("pair_edge_count", edges.length())
            .put(
                "geometry_candidate_edge_count",
                candidateCount,
            )
            .put("nodes", nodes)
            .put("edges", edges)
            .put(
                "campaign_semantics",
                JSONObject()
                    .put(
                        "selection_order_is_capture_time_order",
                        false,
                    )
                    .put(
                        "natural_overlap_sequence_allowed",
                        true,
                    )
                    .put("raw_360_sequence_allowed", true)
                    .put(
                        "stitched_panorama_is_evidence_root",
                        false,
                    )
                    .put(
                        "camera_holder_is_world_origin",
                        false,
                    )
                    .put(
                        "panorama_center_is_world_origin",
                        false,
                    ),
            )
            .put(
                "authority_boundary",
                JSONObject()
                    .put("world_registration_promoted", false)
                    .put("calibration_promoted", false)
                    .put("correction_authorized", false)
                    .put(
                        "scientific_writeback_allowed",
                        false,
                    ),
            )
            .put("camera_model_routing_used", false)
            .put("lens_profile_lookup_used", false)
            .put("vendor_mapping_used", false)
            .put("ai_ml_neural_generative_used", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun sha256(value: String): String {
        val digest =
            MessageDigest.getInstance("SHA-256")
                .digest(value.toByteArray(Charsets.UTF_8))
        return digest.joinToString("") {
            "%02x".format(it.toInt() and 0xff)
        }
    }
}
