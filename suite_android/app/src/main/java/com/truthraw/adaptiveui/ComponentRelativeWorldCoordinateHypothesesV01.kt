package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Builds one independent relative numeric gauge hypothesis per connected
 * appearance-geometry component.
 *
 * No transform is allowed to leak between disconnected components.
 */
object ComponentRelativeWorldCoordinateHypothesesV01 {
    const val SCHEMA =
        "D.RAW/ComponentRelativeWorldCoordinateHypotheses/0.1"

    fun build(
        graph: JSONObject,
        components: JSONObject,
    ): JSONObject {
        val componentArray =
            components.optJSONArray("components")
                ?: JSONArray()
        val graphNodes =
            graph.optJSONArray("nodes") ?: JSONArray()
        val graphEdges =
            graph.optJSONArray("edges") ?: JSONArray()

        val results = JSONArray()
        val flattenedObservations = JSONArray()
        var connectedHypothesisCount = 0

        for (i in 0 until componentArray.length()) {
            val component =
                componentArray.optJSONObject(i) ?: continue
            val rootsArray =
                component.optJSONArray("source_sha256_roots")
                    ?: continue
            val rootSet = linkedSetOf<String>()
            for (r in 0 until rootsArray.length()) {
                val sha = rootsArray.optString(r)
                if (sha.isNotBlank()) rootSet += sha
            }
            if (rootSet.isEmpty()) continue

            val subNodes = JSONArray()
            for (n in 0 until graphNodes.length()) {
                val node = graphNodes.optJSONObject(n) ?: continue
                if (node.optString("source_sha256") in rootSet) {
                    subNodes.put(JSONObject(node.toString()))
                }
            }

            val subEdges = JSONArray()
            for (e in 0 until graphEdges.length()) {
                val edge = graphEdges.optJSONObject(e) ?: continue
                val left = edge.optString("left_source_sha256")
                val right = edge.optString("right_source_sha256")
                if (left in rootSet && right in rootSet) {
                    subEdges.put(JSONObject(edge.toString()))
                }
            }

            val subGraph =
                JSONObject()
                    .put(
                        "schema",
                        FreeWorldObservationGraphV01.SCHEMA,
                    )
                    .put(
                        "status",
                        "READ_ONLY_OBSERVATION_GRAPH_AVAILABLE",
                    )
                    .put(
                        "graph_identity_sha256",
                        graph.opt(
                            "graph_identity_sha256",
                        ) ?: JSONObject.NULL,
                    )
                    .put("nodes", subNodes)
                    .put("edges", subEdges)

            val hypothesis =
                RelativeWorldCoordinateHypothesisV01.build(
                    subGraph,
                )
            if (
                hypothesis.optString("status") ==
                "RELATIVE_WORLD_COORDINATE_HYPOTHESIS_AVAILABLE"
            ) {
                connectedHypothesisCount++
            }

            val observations =
                hypothesis.optJSONArray("observations")
                    ?: JSONArray()
            for (o in 0 until observations.length()) {
                val copy =
                    JSONObject(
                        observations.optJSONObject(o)
                            ?.toString()
                            ?: continue,
                    )
                copy.put(
                    "component_id_sha256",
                    component.optString(
                        "component_id_sha256",
                    ),
                )
                flattenedObservations.put(copy)
            }

            results.put(
                JSONObject()
                    .put(
                        "component_id_sha256",
                        component.optString(
                            "component_id_sha256",
                        ),
                    )
                    .put(
                        "component_same_physical_scene_proven",
                        false,
                    )
                    .put(
                        "relative_world_hypothesis",
                        hypothesis,
                    ),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (results.length() > 0) {
                    "COMPONENT_RELATIVE_WORLD_HYPOTHESES_AVAILABLE"
                } else {
                    "UNKNOWN_FAIL_CLOSED"
                },
            )
            .put("component_count", results.length())
            .put(
                "connected_relative_world_hypothesis_count",
                connectedHypothesisCount,
            )
            .put("components", results)
            .put(
                "observations",
                flattenedObservations,
            )
            .put(
                "coordinate_policy",
                JSONObject()
                    .put(
                        "cross_component_transform_exists",
                        false,
                    )
                    .put(
                        "disconnected_components_may_share_numeric_gauge",
                        false,
                    )
                    .put(
                        "component_gauge_has_physical_origin_authority",
                        false,
                    )
                    .put(
                        "component_connectivity_is_same_world_proof",
                        false,
                    ),
            )
            .put("world_registration_promoted", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }
}
