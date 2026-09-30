package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Read-only planning layer for a future Free World continuous query.
 *
 * It can identify possible observation/component context, but it cannot map a
 * world coordinate to a measured source anchor until the bridge is admitted.
 */
object FreeWorldContinuousQueryPlannerV01 {
    const val SCHEMA = "D.RAW/FreeWorldContinuousQueryPlanner/0.1"

    fun plan(
        components: JSONObject,
        querySupportLedger: JSONObject,
    ): JSONObject {
        val plans = JSONArray()
        val componentArray =
            components.optJSONArray("components") ?: JSONArray()
        val supportByRoot =
            linkedMapOf<String, JSONObject>()
        val ledgerObservations =
            querySupportLedger.optJSONArray("observations")
                ?: JSONArray()

        for (i in 0 until ledgerObservations.length()) {
            val item =
                ledgerObservations.optJSONObject(i) ?: continue
            val sha = item.optString("source_sha256")
            if (sha.isNotBlank()) supportByRoot[sha] = item
        }

        for (i in 0 until componentArray.length()) {
            val component =
                componentArray.optJSONObject(i) ?: continue
            val roots =
                component.optJSONArray("source_sha256_roots")
                    ?: JSONArray()
            val candidateRoots = JSONArray()
            var exactAnchorResolverPotential = 0

            for (r in 0 until roots.length()) {
                val sha = roots.optString(r)
                val support = supportByRoot[sha]
                if (support != null) {
                    candidateRoots.put(sha)
                    if (
                        support.optString(
                            "sample_lattice_status",
                        ) == "AVAILABLE"
                    ) {
                        exactAnchorResolverPotential++
                    }
                }
            }

            plans.put(
                JSONObject()
                    .put(
                        "component_id_sha256",
                        component.optString(
                            "component_id_sha256",
                        ),
                    )
                    .put(
                        "candidate_source_roots",
                        candidateRoots,
                    )
                    .put(
                        "source_lattice_available_count",
                        exactAnchorResolverPotential,
                    )
                    .put(
                        "world_to_source_bridge_admitted",
                        false,
                    )
                    .put(
                        "exact_world_query_support_resolved",
                        false,
                    )
                    .put(
                        "query_result_authority_until_bridge",
                        "UNKNOWN",
                    ),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "QUERY_PLAN_CONTEXT_AVAILABLE_NO_WORLD_SOURCE_RESOLUTION",
            )
            .put("component_plan_count", plans.length())
            .put("components", plans)
            .put(
                "planning_policy",
                JSONObject()
                    .put(
                        "planner_may_choose_camera_or_lens_by_name",
                        false,
                    )
                    .put(
                        "planner_may_invent_cross_component_transform",
                        false,
                    )
                    .put(
                        "planner_may_treat_candidate_geometry_as_admitted_bridge",
                        false,
                    )
                    .put(
                        "planner_may_return_measured_without_exact_source_support",
                        false,
                    )
                    .put(
                        "unsupported_query_must_remain_unknown",
                        true,
                    ),
            )
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}
