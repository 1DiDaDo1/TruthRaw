package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import java.security.MessageDigest

/**
 * Multi-observation feature-track hypotheses assembled from robust inlier
 * pair matches.
 *
 * Tracks are appearance-derived relation candidates only. They are useful for
 * later graph-consistency and natural self-calibration validation, but they do
 * not prove that all members are the same physical world point.
 */
object FreeWorldFeatureTrackHypothesesV01 {
    const val SCHEMA = "D.RAW/FreeWorldFeatureTrackHypotheses/0.1"

    private data class Member(
        val sourceSha256: String,
        val featureIndex: Int,
        val x: Double,
        val y: Double,
    ) {
        val key: String
            get() = "$sourceSha256:$featureIndex"
    }

    fun build(graph: JSONObject): JSONObject {
        val parent = linkedMapOf<String, String>()
        val rank = linkedMapOf<String, Int>()
        val members = linkedMapOf<String, Member>()
        val supportEdges = linkedSetOf<String>()

        fun add(member: Member) {
            members.putIfAbsent(member.key, member)
            parent.putIfAbsent(member.key, member.key)
            rank.putIfAbsent(member.key, 0)
        }

        fun find(key: String): String {
            val p = parent[key] ?: key
            if (p == key) return key
            val root = find(p)
            parent[key] = root
            return root
        }

        fun union(a: String, b: String) {
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

        val edges = graph.optJSONArray("edges") ?: JSONArray()
        var robustPairMatchCount = 0

        for (i in 0 until edges.length()) {
            val edge = edges.optJSONObject(i) ?: continue
            val pair = edge.optJSONObject("pair_geometry") ?: continue
            if (
                pair.optString("status") !=
                "PAIR_GEOMETRY_CANDIDATE_AVAILABLE"
            ) {
                continue
            }

            val leftSha = edge.optString("left_source_sha256")
            val rightSha = edge.optString("right_source_sha256")
            if (
                leftSha.isBlank() ||
                rightSha.isBlank() ||
                leftSha == rightSha
            ) {
                continue
            }

            val matches = pair.optJSONArray("matches") ?: continue
            for (m in 0 until matches.length()) {
                val match = matches.optJSONObject(m) ?: continue
                if (!match.optBoolean("robust_inlier", false)) continue

                val leftIndex = match.optInt("left_index", -1)
                val rightIndex = match.optInt("right_index", -1)
                val leftX =
                    match.optDouble(
                        "left_x_normalized",
                        Double.NaN,
                    )
                val leftY =
                    match.optDouble(
                        "left_y_normalized",
                        Double.NaN,
                    )
                val rightX =
                    match.optDouble(
                        "right_x_normalized",
                        Double.NaN,
                    )
                val rightY =
                    match.optDouble(
                        "right_y_normalized",
                        Double.NaN,
                    )
                if (
                    leftIndex < 0 ||
                    rightIndex < 0 ||
                    !leftX.isFinite() ||
                    !leftY.isFinite() ||
                    !rightX.isFinite() ||
                    !rightY.isFinite()
                ) {
                    continue
                }

                val left =
                    Member(
                        sourceSha256 = leftSha,
                        featureIndex = leftIndex,
                        x = leftX,
                        y = leftY,
                    )
                val right =
                    Member(
                        sourceSha256 = rightSha,
                        featureIndex = rightIndex,
                        x = rightX,
                        y = rightY,
                    )
                add(left)
                add(right)
                union(left.key, right.key)
                supportEdges += "$leftSha>$rightSha"
                robustPairMatchCount++
            }
        }

        val grouped =
            linkedMapOf<String, MutableList<Member>>()
        for ((key, member) in members) {
            val root = find(key)
            grouped.getOrPut(root) { ArrayList() }
                .add(member)
        }

        val tracks = JSONArray()
        var conflictCount = 0
        var multiObservationTrackCount = 0
        var threePlusObservationTrackCount = 0

        val orderedGroups =
            grouped.values
                .map { group ->
                    group.sortedWith(
                        compareBy<Member> { it.sourceSha256 }
                            .thenBy { it.featureIndex },
                    )
                }
                .filter { it.size >= 2 }
                .sortedBy { group ->
                    group.joinToString("|") { it.key }
                }

        for (group in orderedGroups) {
            val observationCount =
                group.map { it.sourceSha256 }
                    .distinct()
                    .size
            val conflict =
                observationCount != group.size

            if (conflict) conflictCount++
            if (observationCount >= 2) {
                multiObservationTrackCount++
            }
            if (observationCount >= 3) {
                threePlusObservationTrackCount++
            }

            val memberArray = JSONArray()
            for (member in group) {
                memberArray.put(
                    JSONObject()
                        .put(
                            "source_sha256",
                            member.sourceSha256,
                        )
                        .put(
                            "feature_index",
                            member.featureIndex,
                        )
                        .put("x_normalized", member.x)
                        .put("y_normalized", member.y),
                )
            }

            val trackId =
                sha256(
                    group.joinToString("|") { it.key },
                )

            tracks.put(
                JSONObject()
                    .put("track_id_sha256", trackId)
                    .put(
                        "member_count",
                        group.size,
                    )
                    .put(
                        "distinct_observation_count",
                        observationCount,
                    )
                    .put(
                        "contains_multiple_features_from_same_observation",
                        conflict,
                    )
                    .put(
                        "authority",
                        "APPEARANCE_DERIVED_TRACK_HYPOTHESIS_ONLY",
                    )
                    .put(
                        "same_physical_world_point_proven",
                        false,
                    )
                    .put("members", memberArray),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (tracks.length() > 0) {
                    "FEATURE_TRACK_HYPOTHESES_AVAILABLE"
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
            .put(
                "robust_pair_match_count",
                robustPairMatchCount,
            )
            .put(
                "pair_edge_support_count",
                supportEdges.size,
            )
            .put(
                "track_count",
                tracks.length(),
            )
            .put(
                "multi_observation_track_count",
                multiObservationTrackCount,
            )
            .put(
                "three_plus_observation_track_count",
                threePlusObservationTrackCount,
            )
            .put(
                "same_observation_feature_conflict_track_count",
                conflictCount,
            )
            .put("tracks", tracks)
            .put(
                "authority_boundary",
                JSONObject()
                    .put(
                        "same_world_structure_proven",
                        false,
                    )
                    .put(
                        "world_registration_promoted",
                        false,
                    )
                    .put(
                        "camera_or_lens_identity_used",
                        false,
                    )
                    .put(
                        "track_conflict_may_be_silently_discarded",
                        false,
                    ),
            )
            .put("creates_sensor_evidence", false)
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
