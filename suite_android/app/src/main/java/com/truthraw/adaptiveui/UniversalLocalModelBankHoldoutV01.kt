package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import android.os.ParcelFileDescriptor
import org.json.JSONObject

object UniversalLocalModelBankHoldoutBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportAndVerify(
        sourceFd: Int,
        destinationFd: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): String
}

/**
 * Full-resolution, source-agnostic hold-out audit for the universal local
 * model bank.
 *
 * It is deliberately broader than the earlier Dark-Chroma-local PR #90
 * experiment. A deterministic stratified subset of real CFA anchors is hidden
 * across the original full-resolution source. The surrounding same-phase
 * measured anchors are represented in the existing raster-independent lattice.
 *
 * The target value is not available to any model or to model selection until
 * after the selected model has been frozen for that hold-out.
 */
object UniversalLocalModelBankHoldoutV01 {
    const val SCHEMA =
        "D.RAW/UniversalLocalModelBankHoldout/0.1"

    private const val MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
    private const val MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

    fun describe(
        sourceSha256: String,
        nativeDngReady: Boolean,
        sampleLattice: JSONObject?,
        prospectivePolicy: JSONObject?,
    ): JSONObject {
        if (!nativeDngReady) {
            return unavailable(
                sourceSha256,
                "NATIVE_DNG_ROUTE_NOT_AVAILABLE",
            )
        }
        if (
            sampleLattice == null ||
            sampleLattice.optString("schema") !=
                RasterIndependentSampleLatticeV01.SCHEMA ||
            sampleLattice.optString("source_sha256") != sourceSha256 ||
            sampleLattice.optString("status") != "AVAILABLE"
        ) {
            return unavailable(
                sourceSha256,
                "RASTER_INDEPENDENT_SAMPLE_LATTICE_NOT_AVAILABLE",
            )
        }
        val prospectivePolicyBound =
            prospectivePolicy != null &&
                prospectivePolicy.optString("schema") ==
                    UniversalObservationModelSelectionV01.SCHEMA &&
                prospectivePolicy.optString("source_sha256") ==
                    sourceSha256 &&
                prospectivePolicy.optString("status") ==
                    "PROSPECTIVE_AUDIT_POLICY_AVAILABLE"

        return base(sourceSha256)
            .put("status", "READY_FOR_EXPLICIT_EXPORT_AUDIT")
            .put(
                "prospective_query_policy_binding_available",
                prospectivePolicyBound,
            )
            .put(
                "prospective_query_policy_required_for_full_resolution_audit",
                false,
            )
            .put(
                "dark_chroma_dependency_required",
                false,
            )
            .put(
                "authority",
                "PRIVATE_RECONSTRUCTION_MODEL_SELECTION_AUDIT_ONLY",
            )
            .put(
                "holdout_scope",
                "STRATIFIED_FULL_RESOLUTION_MEASURED_CFA_ANCHORS",
            )
            .put("holdout_period", 64)
            .put("support_radius_source_px", 8)
            .put(
                "source_raster_role",
                "EXACT_MEASURED_ANCHOR_GEOMETRY_AND_FULL_RESOLUTION_SOURCE_SUPPORT",
            )
            .put("source_raster_used_only_for_noise", false)
            .put(
                "scientific_solution_domain",
                "RASTER_INDEPENDENT_SPARSE_FIXED_POINT_LATTICE",
            )
            .put(
                "model_bank",
                org.json.JSONArray()
                    .put("ROBUST_MEDIAN_CONSTANT")
                    .put("DIRECTIONAL_LINE")
                    .put("AFFINE_PLANE")
                    .put("QUADRATIC_SURFACE")
                    .put("NO_RECONSTRUCTION"),
            )
            .put(
                "baseline_name",
                "CENTER_EXCLUDED_MULTISCALE_V0_2",
            )
            .put(
                "selection_score",
                "TARGET_BLIND_RESIDUAL_BIC_LIKE_COMPLEXITY_SCORE",
            )
            .put(
                "noise_profile_required_for_selection",
                false,
            )
            .put(
                "existing_2026_09_29_tele_holdout_is_independent_validation",
                false,
            )
            .put(
                "new_independent_capture_required_for_scientific_conclusion",
                true,
            )
    }

    fun exportSidecar(
        resolver: ContentResolver,
        sourceUri: Uri,
        destinationUri: Uri,
        expectedSourceSha256: String,
    ): JSONObject {
        val statusText =
            resolver.openFileDescriptor(sourceUri, "r")?.use { src ->
                resolver.openFileDescriptor(destinationUri, "rw")?.use { dst ->
                    UniversalLocalModelBankHoldoutBridge.exportAndVerify(
                        src.fd,
                        dst.fd,
                        MAX_SOURCE_RESIDENT_BYTES,
                        MAX_LOGICAL_RESIDENT_BYTES,
                    )
                }
            } ?: error("Bron of bestemming kon niet worden geopend.")

        val status = JSONObject(statusText)
        require(status.optInt("status", -999) == 0) {
            "Universal model-bank holdout export faalde: " +
                status.optString(
                    "message",
                    "status=" + status.optInt("status"),
                )
        }
        require(status.optBoolean("postWriteVerified", false)) {
            "Universal model-bank holdout is niet post-write geverifieerd."
        }
        require(status.optString("sourceSha256") == expectedSourceSha256) {
            "Universal model-bank holdout source-SHA mismatch."
        }
        require(!status.optBoolean("targetValueUsedByModels", true)) {
            "Hold-out target is door een model gebruikt."
        }
        require(!status.optBoolean("targetValueUsedBySelector", true)) {
            "Hold-out target is door de selector gebruikt."
        }
        require(!status.optBoolean("holdoutErrorUsedBySelector", true)) {
            "Hold-out fout is door de selector gebruikt."
        }
        require(!status.optBoolean("postRevealOracleUsedBySelector", true)) {
            "Post-reveal oracle is door de selector gebruikt."
        }
        require(!status.optBoolean("lensCalibrationUsed", true)) {
            "Selector is lens-calibratieafhankelijk geworden."
        }
        require(!status.optBoolean("cameraModelUsed", true)) {
            "Selector is camera-modelafhankelijk geworden."
        }
        require(!status.optBoolean("vendorMappingUsed", true)) {
            "Selector is vendor-mapafhankelijk geworden."
        }
        require(!status.optBoolean("measuredAnchorsModified", true)) {
            "Measured anchors zijn gewijzigd."
        }
        require(!status.optBoolean("unanchoredValuesPromotedToMeasured", true)) {
            "Unanchored waarden zijn naar MEASURED gepromoveerd."
        }
        require(!status.optBoolean("modelBankAppliedToScientificMaster", true)) {
            "Modelbank is op Scientific Master toegepast."
        }
        require(!status.optBoolean("candidateApplied", true)) {
            "Researchkandidaat is toegepast."
        }
        require(!status.optBoolean("createsNewEvidence", true)) {
            "Audit claimt nieuwe evidence."
        }
        require(!status.optBoolean("scientificWritebackAllowed", true)) {
            "Audit staat scientific writeback toe."
        }
        return status
    }

    private fun unavailable(
        sourceSha256: String,
        reason: String,
    ): JSONObject =
        base(sourceSha256)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)

    private fun base(sourceSha256: String): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("source_sha256", sourceSha256)
            .put("target_value_used_by_models", false)
            .put("target_value_used_by_selector", false)
            .put("holdout_error_used_by_selector", false)
            .put("post_reveal_oracle_used_by_selector", false)
            .put("lens_calibration_used", false)
            .put("camera_model_used", false)
            .put("vendor_mapping_used", false)
            .put("device_specific_mapping_used", false)
            .put("ai_or_learned_models_used", false)
            .put("measured_anchors_modified", false)
            .put("unanchored_values_promoted_to_measured", false)
            .put("model_bank_applied_to_scientific_master", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}
