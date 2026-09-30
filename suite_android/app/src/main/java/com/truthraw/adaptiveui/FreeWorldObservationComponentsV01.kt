package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import java.security.MessageDigest

/**
 * Partitions the candidate observation graph into deterministic connected
 * components without claiming that a component is one physical scene.
 *
 * This prevents unrelated selected files from being silently forced into a
 * single relative-world gauge.
 */
object FreeWorldObservationComponentsV01 {
    const val SCHEMA = "D.RAW/FreeWorldObservationComponents/0.1"

    fun build(graph: JSONObject): JSONObject {
        val roots = ArrayList<String>()
        val nodes = graph.optJSONArray("nodes") ?: JSONArray()
        for (i in 0 until nodes.length()) {
            val sha =
                nodes.optJSONObject(i)
                    ?.optString("source_sha256")
                    ?: continue
            if (sha.isNotBlank()) roots += sha
        }
        val sortedRoots = roots.distinct().sorted()

        val parent = linkedMapOf<String, String>()
        val rank = linkedMapOf<String, Int>()
        for (sha in sortedRoots) {
            parent[sha] = sha
            rank[sha] = 0
        }

        fun find(key: String): String {
            val p = parent[key] ?: key
            if (p == key) return key
            val root = find(p)
            parent[key] = root
            return root
        }

        fun union(a: String, b: String) {
            if (a !in parent || b !in parent) return
            var ra = find(a)
            var rb = find(b)
            if (ra == rb) return

            val rankA = rank[ra] ?: 0
            val rankB = rank[rb] ?: 0
            if (rankA < rankB || (rankA == rankB && ra > rb)) {
                val tmp = ra
                ra = rb
                rb = tmp
            }
            parent[rb] = ra
            if (rankA == rankB) {
                rank[ra] = rankA + 1
            }
        }

        val candidateEdges = ArrayList<Pair<String, String>>()
        val edges = graph.optJSONArray("edges") ?: JSONArray()
        for (i in 0 until edges.length()) {
            val edge = edges.optJSONObject(i) ?: continue
            val pair = edge.optJSONObject("pair_geometry") ?: continue
            if (
                pair.optString("status") !=
                "PAIR_GEOMETRY_CANDIDATE_AVAILABLE"
            ) {
                continue
            }
            val left = edge.optString("left_source_sha256")
            val right = edge.optString("right_source_sha256")
            if (
                left.isBlank() ||
                right.isBlank() ||
                left == right
            ) {
                continue
            }
            union(left, right)
            candidateEdges += left to right
        }

        val grouped =
            linkedMapOf<String, MutableList<String>>()
        for (sha in sortedRoots) {
            val root = find(sha)
            grouped.getOrPut(root) { ArrayList() }
                .add(sha)
        }

        val orderedGroups =
            grouped.values
                .map { it.sorted() }
                .sortedBy { it.firstOrNull().orEmpty() }

        val componentArray = JSONArray()
        val membership = JSONObject()
        var multiNodeComponentCount = 0
        var isolatedObservationCount = 0

        for (group in orderedGroups) {
            val edgeCount =
                candidateEdges.count { edge ->
                    edge.first in group &&
                        edge.second in group
                }
            if (group.size >= 2) {
                multiNodeComponentCount++
            } else {
                isolatedObservationCount++
            }

            val componentId =
                sha256(group.joinToString("|"))
            val rootsJson = JSONArray()
            group.forEach { sha ->
                rootsJson.put(sha)
                membership.put(sha, componentId)
            }

            componentArray.put(
                JSONObject()
                    .put(
                        "component_id_sha256",
                        componentId,
                    )
                    .put("observation_count", group.size)
                    .put(
                        "candidate_geometry_edge_count",
                        edgeCount,
                    )
                    .put(
                        "source_sha256_roots",
                        rootsJson,
                    )
                    .put(
                        "numeric_gauge_candidate_sha256",
                        group.firstOrNull()
                            ?: JSONObject.NULL,
                    )
                    .put(
                        "numeric_gauge_has_physical_origin_authority",
                        false,
                    )
                    .put(
                        "component_is_same_physical_scene_proven",
                        false,
                    )
                    .put(
                        "component_is_same_capture_system_proven",
                        false,
                    )
                    .put(
                        "component_role",
                        if (group.size >= 2) {
                            "CONNECTED_APPEARANCE_GEOMETRY_CANDIDATE_COMPONENT"
                        } else {
                            "ISOLATED_OBSERVATION"
                        },
                    ),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (sortedRoots.isNotEmpty()) {
                    "OBSERVATION_COMPONENTS_AVAILABLE"
                } else {
                    "UNKNOWN_FAIL_CLOSED"
                },
            )
            .put(
                "source_graph_identity_sha256",
                graph.opt(
                    "graph_identity_sha256",
                ) ?: JSONObject.NULL,
            )
            .put("component_count", componentArray.length())
            .put(
                "multi_observation_component_count",
                multiNodeComponentCount,
            )
            .put(
                "isolated_observation_count",
                isolatedObservationCount,
            )
            .put("components", componentArray)
            .put(
                "source_to_component_id",
                membership,
            )
            .put(
                "authority_boundary",
                JSONObject()
                    .put(
                        "connected_component_equals_same_world_proof",
                        false,
                    )
                    .put(
                        "selection_set_equals_one_world",
                        false,
                    )
                    .put(
                        "world_registration_promoted",
                        false,
                    )
                    .put(
                        "camera_or_lens_identity_used",
                        false,
                    ),
            )
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
