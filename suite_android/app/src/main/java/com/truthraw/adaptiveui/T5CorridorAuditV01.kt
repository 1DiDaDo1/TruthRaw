package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Read-only runtime audit for the existing v0.4 -> v0.5 -> v0.6 ->
 * Room Capsule -> v0.7 corridor.
 *
 * This object has no mutation or promotion authority. It only serializes
 * telemetry emitted after the existing corridor has already executed. Missing
 * telemetry is UNKNOWN; explicit native corridor rejection is BLOCKED where
 * the failing stage is known.
 */
object T5CorridorAuditV01 {
    private const val SCHEMA = "D.RAW/Runtime/T5CorridorAudit/0.1"
    private const val EXPECTED_TELEMETRY_SCHEMA = 1

    fun from(metrics: TruthNegativeContinuousPreviewMetrics): JSONObject {
        val target = metrics.targetPixels
        val telemetryKnown =
            metrics.t5TelemetrySchemaVersion == EXPECTED_TELEMETRY_SCHEMA &&
                target > 0

        val v04Observed = telemetryKnown && metrics.t5V04ObservedCount == target
        val v05Observed = telemetryKnown && metrics.t5V05ObservedCount == target
        val v06Observed = telemetryKnown && metrics.t5V06ObservedCount == target
        val roomObserved = telemetryKnown &&
            metrics.t5RoomCapsuleAppliedCount +
                metrics.t5RoomCapsuleExactBypassCount == target
        val v07Observed = telemetryKnown && metrics.t5V07ObservedCount == target

        val v06State = when {
            !telemetryKnown -> "UNKNOWN"
            v06Observed &&
                metrics.lightTransportSeedBuilt &&
                metrics.lightTransportParentBound &&
                metrics.t5V06InferredAuthority -> "READY"
            else -> "BLOCKED"
        }

        val roomCapsuleState = when {
            !telemetryKnown || !roomObserved -> "UNKNOWN"
            metrics.t5RoomCapsuleAppliedCount == target &&
                metrics.t5RoomCapsuleExactBypassCount == 0 -> "APPLIED"
            metrics.t5RoomCapsuleAppliedCount == 0 &&
                metrics.t5RoomCapsuleExactBypassCount == target &&
                metrics.roomCapsuleExactBypass -> "EXACT_PRESERVING_BYPASS"
            else -> "UNKNOWN"
        }

        val firewallBits = metrics.t5FirewallBits
        val sourcePreVerified = telemetryKnown && firewallBits and 0x01 != 0
        val sourceMasterBound = telemetryKnown && firewallBits and 0x02 != 0
        val deepFirewallsClosed = telemetryKnown && firewallBits and 0x04 != 0
        val v06FirewallsClosed = telemetryKnown && firewallBits and 0x08 != 0
        val roomFirewallsClosed = telemetryKnown && firewallBits and 0x10 != 0
        val v07FirewallsClosed = telemetryKnown && firewallBits and 0x20 != 0
        val exposureExactlyOnce = telemetryKnown &&
            firewallBits and 0x40 != 0 &&
            metrics.t5ExposureApplicationCount == 1
        val sourcePostVerified = telemetryKnown && firewallBits and 0x80 != 0

        val stages = JSONArray()
            .put(
                stage(
                    id = "v0.4",
                    state = observedState(v04Observed, telemetryKnown),
                    authority = "RADIOMETRY_PRESERVED_FROM_TRUTHNEGATIVE",
                    provenanceSha256 = knownDigest(
                        metrics.t5V04LineageSha256,
                        telemetryKnown && v04Observed,
                    ),
                    firewallsClosed = deepFirewallsClosed,
                ),
            )
            .put(
                stage(
                    id = "v0.5",
                    state = observedState(v05Observed, telemetryKnown),
                    authority = if (metrics.t5V05ImagePlaneBound) {
                        "IMAGE_PLANE_BOUND"
                    } else {
                        "UNKNOWN"
                    },
                    provenanceSha256 = knownDigest(
                        metrics.t5V05LineageSha256,
                        telemetryKnown && v05Observed,
                    ),
                    firewallsClosed = deepFirewallsClosed,
                ),
            )
            .put(
                stage(
                    id = "v0.6",
                    state = v06State,
                    authority = if (metrics.t5V06InferredAuthority) {
                        "GEOMETRY_MATERIAL_ILLUMINATION_INFERRED"
                    } else {
                        "UNKNOWN"
                    },
                    provenanceSha256 = knownDigest(
                        metrics.t5V06LineageSha256,
                        telemetryKnown && v06Observed,
                    ),
                    firewallsClosed = v06FirewallsClosed,
                ).put(
                    "scientific_radiometry_inherited_as_measurement",
                    if (v06FirewallsClosed) false else JSONObject.NULL,
                ),
            )
            .put(
                stage(
                    id = "ROOM_CAPSULE",
                    state = roomCapsuleState,
                    authority = if (
                        roomCapsuleState == "EXACT_PRESERVING_BYPASS"
                    ) {
                        "INFERRED_CONTEXT_NO_ADMITTED_ROOM_EVIDENCE"
                    } else {
                        "UNKNOWN"
                    },
                    provenanceSha256 = knownDigest(
                        metrics.t5RoomCapsuleLineageSha256,
                        telemetryKnown && roomObserved,
                    ),
                    firewallsClosed = roomFirewallsClosed,
                ),
            )
            .put(
                stage(
                    id = "v0.7",
                    state = observedState(v07Observed, telemetryKnown),
                    authority = "APPEARANCE_ONLY_DOWNSTREAM",
                    provenanceSha256 = knownDigest(
                        metrics.t5V07LineageSha256,
                        telemetryKnown && v07Observed,
                    ),
                    firewallsClosed = v07FirewallsClosed,
                ),
            )

        val allRuntimeEvidenceComplete =
            telemetryKnown &&
                v04Observed &&
                v05Observed &&
                v06State == "READY" &&
                roomCapsuleState == "EXACT_PRESERVING_BYPASS" &&
                v07Observed &&
                sourcePreVerified &&
                sourceMasterBound &&
                sourcePostVerified &&
                deepFirewallsClosed &&
                v06FirewallsClosed &&
                roomFirewallsClosed &&
                v07FirewallsClosed &&
                exposureExactlyOnce &&
                metrics.t5PhysicalFrameCount == 1 &&
                metrics.t5IndependentEvidenceCount == 1 &&
                !metrics.t5CandidateApplied

        return JSONObject()
            .put("schema", SCHEMA)
            .put("audit_authority", "DIAGNOSTIC_RUNTIME_ONLY")
            .put("mutation_authority", "NONE")
            .put(
                "audit_state",
                if (allRuntimeEvidenceComplete) "READY" else "UNKNOWN_FAIL_CLOSED",
            )
            .put(
                "source_sha256",
                knownDigest(metrics.sourceSha256, telemetryKnown),
            )
            .put(
                "scientific_master_sha256",
                knownDigest(metrics.scientificMasterSha256, telemetryKnown),
            )
            .put(
                "source_scientific_master_binding_verified",
                if (sourceMasterBound &&
                    metrics.t5SourceScientificMasterBindingVerified
                ) {
                    true
                } else if (telemetryKnown) {
                    false
                } else {
                    JSONObject.NULL
                },
            )
            .put("v0_6_state", v06State)
            .put("room_capsule_state", roomCapsuleState)
            .put(
                "exposure_application_count",
                if (telemetryKnown) {
                    metrics.t5ExposureApplicationCount
                } else {
                    JSONObject.NULL
                },
            )
            .put("exposure_application_count_verified", exposureExactlyOnce)
            .put("physical_frame_count", metrics.t5PhysicalFrameCount)
            .put(
                "independent_evidence_count",
                metrics.t5IndependentEvidenceCount,
            )
            .put(
                "source_integrity_pre_and_post_verified",
                sourcePreVerified && sourcePostVerified,
            )
            .put(
                "mutation_writeback_firewalls",
                JSONObject()
                    .put("deep_scene", firewallState(deepFirewallsClosed, telemetryKnown))
                    .put("v0_6", firewallState(v06FirewallsClosed, telemetryKnown))
                    .put("room_capsule", firewallState(roomFirewallsClosed, telemetryKnown))
                    .put("v0_7", firewallState(v07FirewallsClosed, telemetryKnown))
                    .put("candidate_applied", metrics.t5CandidateApplied)
                    .put("scientific_writeback_allowed", false),
            )
            .put("stages", stages)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
            .put("sealed_cfa_modified_by_audit", false)
            .put("measured_anchors_modified_by_audit", false)
            .put("scientific_master_modified_by_audit", false)
            .put("exact_gauge_v0_3_modified_by_audit", false)
            .put("reconstruction_behavior_modified_by_audit", false)
    }

    fun fromFailure(
        nativeStatusCode: Int?,
        reason: String,
    ): JSONObject {
        val v06State = if (nativeStatusCode == -20) {
            "BLOCKED"
        } else {
            "UNKNOWN"
        }
        return JSONObject()
            .put("schema", SCHEMA)
            .put("audit_authority", "DIAGNOSTIC_RUNTIME_ONLY")
            .put("mutation_authority", "NONE")
            .put("audit_state", "UNKNOWN_FAIL_CLOSED")
            .put("source_sha256", "UNKNOWN")
            .put("scientific_master_sha256", "UNKNOWN")
            .put("v0_6_state", v06State)
            .put("room_capsule_state", "UNKNOWN")
            .put("exposure_application_count", JSONObject.NULL)
            .put("native_status_code", nativeStatusCode ?: JSONObject.NULL)
            .put("reason", reason)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun stage(
        id: String,
        state: String,
        authority: String,
        provenanceSha256: Any,
        firewallsClosed: Boolean,
    ): JSONObject = JSONObject()
        .put("stage", id)
        .put("runtime_state", state)
        .put("authority", authority)
        .put("provenance_sha256", provenanceSha256)
        .put(
            "mutation_writeback_firewall",
            if (firewallsClosed) "CLOSED" else "UNKNOWN",
        )

    private fun observedState(
        observed: Boolean,
        telemetryKnown: Boolean,
    ): String = when {
        observed -> "OBSERVED"
        telemetryKnown -> "BLOCKED"
        else -> "UNKNOWN"
    }

    private fun firewallState(
        closed: Boolean,
        telemetryKnown: Boolean,
    ): String = when {
        closed -> "CLOSED"
        telemetryKnown -> "BLOCKED"
        else -> "UNKNOWN"
    }

    private fun knownDigest(
        digest: String,
        known: Boolean,
    ): Any = if (
        known && digest.length == 64 && digest.any { it != '0' }
    ) {
        digest
    } else {
        "UNKNOWN"
    }
}
