package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import java.security.MessageDigest

/**
 * Derived lineage manifest for the current selected observation set.
 *
 * It identifies source roots and derived research objects without turning any
 * derived hash into a new physical evidence root.
 */
object FreeWorldEvidenceLineageManifestV01 {
    const val SCHEMA = "D.RAW/FreeWorldEvidenceLineageManifest/0.1"

    fun build(
        profiles: List<JSONObject>,
        graph: JSONObject,
    ): JSONObject {
        val roots = profiles
            .map { it.optString("source_sha256") }
            .filter { it.isNotBlank() }
            .distinct()
            .sorted()

        val rootsJson = JSONArray()
        roots.forEach(rootsJson::put)

        val graphIdentity =
            graph.optString("graph_identity_sha256")
        val manifestIdentity =
            sha256(
                roots.joinToString("|") +
                    "|graph=" + graphIdentity +
                    "|schema=" + SCHEMA,
            )

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "DERIVED_LINEAGE_MANIFEST_AVAILABLE")
            .put("source_sha256_roots", rootsJson)
            .put("source_root_count", roots.size)
            .put(
                "derived_graph_identity_sha256",
                graphIdentity.ifBlank { JSONObject.NULL },
            )
            .put("manifest_identity_sha256", manifestIdentity)
            .put("manifest_identity_is_physical_evidence_root", false)
            .put("derived_graph_identity_is_physical_evidence_root", false)
            .put(
                "lineage_rules",
                JSONObject()
                    .put("source_roots_remain_independent", true)
                    .put("derived_objects_may_merge_source_authority", false)
                    .put("derived_objects_may_inflate_evidence_count", false)
                    .put("source_root_removal_must_be_explicit", true)
                    .put("reconstruction_must_remain_traceable_to_roots", true)
                    .put("appearance_must_remain_traceable_to_scientific_state", true),
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
