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
        val processingRoots =
            profiles
                .map { it.optString("source_sha256").trim().lowercase() }
                .filter { it.matches(Regex("[0-9a-f]{64}")) }
                .distinct()
                .sorted()

        val physicalRoots =
            profiles
                .mapNotNull { profile ->
                    val upstream =
                        profile.optString("upstream_sealed_source_sha256")
                            .trim()
                            .lowercase()
                    val processing =
                        profile.optString("source_sha256")
                            .trim()
                            .lowercase()
                    when {
                        upstream.matches(Regex("[0-9a-f]{64}")) -> upstream
                        processing.matches(Regex("[0-9a-f]{64}")) -> processing
                        else -> null
                    }
                }
                .distinct()
                .sorted()

        val processingRootsJson = JSONArray()
        processingRoots.forEach(processingRootsJson::put)
        val physicalRootsJson = JSONArray()
        physicalRoots.forEach(physicalRootsJson::put)

        val derivations = JSONArray()
        for (profile in profiles) {
            val processing =
                profile.optString("source_sha256").trim().lowercase()
            if (!processing.matches(Regex("[0-9a-f]{64}"))) continue
            val upstream =
                profile.optString("upstream_sealed_source_sha256")
                    .trim()
                    .lowercase()
            derivations.put(
                JSONObject()
                    .put("processing_source_sha256", processing)
                    .put(
                        "physical_evidence_root_sha256",
                        if (upstream.matches(Regex("[0-9a-f]{64}"))) {
                            upstream
                        } else {
                            processing
                        },
                    )
                    .put(
                        "processing_source_is_derived_container",
                        upstream.matches(Regex("[0-9a-f]{64}")),
                    )
                    .put(
                        "acquisition_evidence_sha256",
                        profile.optString("acquisition_evidence_sha256")
                            .takeIf { it.matches(Regex("[0-9a-fA-F]{64}")) }
                            ?.lowercase()
                            ?: JSONObject.NULL,
                    ),
            )
        }

        val graphIdentity =
            graph.optString("graph_identity_sha256")
        val manifestIdentity =
            sha256(
                "processing=" + processingRoots.joinToString("|") +
                    "|physical=" + physicalRoots.joinToString("|") +
                    "|derivations=" + derivations.toString() +
                    "|graph=" + graphIdentity +
                    "|schema=" + SCHEMA,
            )

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "DERIVED_LINEAGE_MANIFEST_AVAILABLE")
            // Compatibility field remains the processing-source roots used by
            // the existing observation graph.
            .put("source_sha256_roots", processingRootsJson)
            .put("source_root_count", processingRoots.size)
            .put("processing_source_sha256_roots", processingRootsJson)
            .put("processing_source_root_count", processingRoots.size)
            .put("physical_evidence_sha256_roots", physicalRootsJson)
            .put("physical_evidence_root_count", physicalRoots.size)
            .put("processing_to_physical_derivations", derivations)
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
                    .put("camera_rawsensor_root_precedes_derived_dng", true)
                    .put("derived_dng_may_replace_physical_rawsensor_root", false)
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
