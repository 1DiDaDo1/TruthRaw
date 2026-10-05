package com.truthraw.adaptiveui

import java.util.concurrent.ConcurrentHashMap
import org.json.JSONObject

/**
 * Process-local transport for an already-computed T5 corridor audit.
 *
 * This is diagnostic plumbing only. It never invokes the T5 corridor, never
 * changes Scientific Master/source samples, and never promotes runtime state.
 * Snapshots are keyed only by exact source SHA-256 so observations cannot share
 * T5 telemetry accidentally.
 */
object ResearchPerformanceT5CorridorBindingV01 {
    const val SCHEMA = "D.RAW/ResearchPerformanceT5CorridorBinding/0.1"
    private const val T5_AUDIT_SCHEMA = "D.RAW/Runtime/T5CorridorAudit/0.1"
    private val SHA256 = Regex("[0-9a-f]{64}")
    private val snapshotsBySource = ConcurrentHashMap<String, String>()

    fun publish(audit: JSONObject): Boolean {
        val sourceSha = normalizeSha(audit.optString("source_sha256"))
            ?: return false
        if (!diagnosticContractValid(audit, sourceSha)) {
            return false
        }
        snapshotsBySource[sourceSha] = audit.toString()
        return true
    }

    fun forProfile(profile: JSONObject): JSONObject {
        val sourceSha = normalizeSha(profile.optString("source_sha256"))
            ?: return unavailable(
                sourceSha256 = profile.optString("source_sha256", "UNKNOWN"),
                reason = "PROFILE_SOURCE_SHA256_INVALID",
            )
        val encoded = snapshotsBySource[sourceSha]
            ?: return unavailable(
                sourceSha256 = sourceSha,
                reason = "NO_PRECOMPUTED_RUNTIME_T5_AUDIT_FOR_SOURCE",
            )
        val audit = runCatching { JSONObject(encoded) }.getOrNull()
            ?: return unavailable(
                sourceSha256 = sourceSha,
                reason = "RUNTIME_T5_AUDIT_SNAPSHOT_INVALID",
            )
        if (!diagnosticContractValid(audit, sourceSha)) {
            return unavailable(
                sourceSha256 = sourceSha,
                reason = "RUNTIME_T5_AUDIT_BINDING_CONTRADICTION",
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "SOURCE_BOUND_PRECOMPUTED_T5_AUDIT_AVAILABLE")
            .put("source_sha256", sourceSha)
            .put("source_binding_verified", true)
            .put("profile_run_binding_verified", false)
            .put(
                "telemetry_origin",
                "LATEST_PROCESS_LOCAL_PRECOMPUTED_TRUTHNEGATIVE_CONTINUOUS_PREVIEW",
            )
            .put("t5_corridor_recomputed_by_binding", false)
            .put("t5_corridor_audit", JSONObject(audit.toString()))
            .put("cross_observation_reuse_allowed", false)
            .put("authority", "DIAGNOSTIC_RUNTIME_ONLY")
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun unavailable(
        sourceSha256: String,
        reason: String,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("source_sha256", sourceSha256)
            .put("source_binding_verified", false)
            .put("profile_run_binding_verified", false)
            .put("telemetry_origin", "UNAVAILABLE")
            .put("t5_corridor_recomputed_by_binding", false)
            .put("t5_corridor_audit", JSONObject.NULL)
            .put("cross_observation_reuse_allowed", false)
            .put("authority", "DIAGNOSTIC_RUNTIME_ONLY")
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)

    private fun diagnosticContractValid(
        audit: JSONObject,
        expectedSourceSha: String,
    ): Boolean {
        if (audit.optString("schema") != T5_AUDIT_SCHEMA) return false
        if (audit.optString("audit_authority") != "DIAGNOSTIC_RUNTIME_ONLY") return false
        if (audit.optString("mutation_authority") != "NONE") return false
        if (normalizeSha(audit.optString("source_sha256")) != expectedSourceSha) return false
        if (audit.optBoolean("creates_new_evidence", true)) return false
        if (audit.optBoolean("scientific_writeback_allowed", true)) return false
        if (audit.optBoolean("sealed_cfa_modified_by_audit", true)) return false
        if (audit.optBoolean("measured_anchors_modified_by_audit", true)) return false
        if (audit.optBoolean("scientific_master_modified_by_audit", true)) return false
        if (audit.optBoolean("exact_gauge_v0_3_modified_by_audit", true)) return false
        if (audit.optBoolean("reconstruction_behavior_modified_by_audit", true)) return false

        val firewalls = audit.optJSONObject("mutation_writeback_firewalls")
            ?: return false
        if (firewalls.optBoolean("candidate_applied", true)) return false
        if (firewalls.optBoolean("scientific_writeback_allowed", true)) return false
        return true
    }

    private fun normalizeSha(value: String): String? =
        value.trim().lowercase().takeIf { SHA256.matches(it) }
}
