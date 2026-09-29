package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import android.os.ParcelFileDescriptor
import org.json.JSONObject

object UniversalLocalModelBankHoldoutV02Bridge {
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
object UniversalLocalModelBankHoldoutV02 {
    const val SCHEMA =
        "D.RAW/UniversalLocalModelBankHoldout/0.2"

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
                    .put("DIRECTIONAL_STRIP_LINE")
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
                "TARGET_BLIND_SUPPORT_CROSSFIT_PREDICTIVE_SCORE_V0_2",
            )
            .put(
                "noise_profile_required_for_selection",
                false,
            )
            .put("selector_uses_support_crossfit", true)
            .put("directional_support_conditioned", true)
            .put(
                "support_crossfit_partition",
                "VALIDATION_WHEN_OFFSET_LATTICE_INDEX_SUM_MOD_3_EQUALS_0",
            )
            .put("directional_strip_half_width_source_px", 2.0)
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
                    UniversalLocalModelBankHoldoutV02Bridge.exportAndVerify(
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
        require(status.optBoolean("selectorUsesSupportCrossfit", false)) {
            "Selector gebruikt de verplichte support-crossfit niet."
        }
        require(status.optBoolean("directionalSupportConditioned", false)) {
            "Directionele kandidaat gebruikt geen richtinggebonden support."
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

        val sidecarText =
            resolver.openInputStream(destinationUri)?.bufferedReader(
                Charsets.UTF_8,
            )?.use { it.readText() }
                ?: error("Universal model-bank sidecar kon niet worden teruggelezen.")
        val sidecar = JSONObject(sidecarText)
        require(
            sidecar.optString("schema") ==
                "D.RAW/UniversalLocalModelBankHoldoutAudit/0.2"
        ) {
            "Universal model-bank sidecar schema mismatch."
        }
        require(sidecar.optString("source_sha256") == expectedSourceSha256) {
            "Universal model-bank sidecar source-SHA mismatch."
        }
        require(sidecar.optBoolean("selector_uses_support_crossfit", false)) {
            "Sidecar mist support-crossfit selectorcontract."
        }
        require(sidecar.optBoolean("directional_support_conditioned", false)) {
            "Sidecar mist direction-conditioned supportcontract."
        }
        require(
            sidecar.optBoolean(
                "target_censor_state_used_for_holdout_admission",
                false,
            ),
        ) {
            "Sidecar mist de expliciete target-censor admission boundary."
        }
        require(
            !sidecar.optBoolean(
                "target_numeric_stage2_value_read_before_selection",
                true,
            ),
        ) {
            "Sidecar claimt dat de numerieke Stage-2 target vóór selector-freeze is gelezen."
        }
        require(!sidecar.optBoolean("target_value_used_by_models", true)) {
            "Sidecar claimt targetgebruik door model."
        }
        require(!sidecar.optBoolean("target_value_used_by_selector", true)) {
            "Sidecar claimt targetgebruik door selector."
        }
        require(!sidecar.optBoolean("holdout_error_used_by_selector", true)) {
            "Sidecar claimt holdout-errorgebruik door selector."
        }
        require(!sidecar.optBoolean("post_reveal_oracle_used_by_selector", true)) {
            "Sidecar claimt post-reveal oraclegebruik door selector."
        }
        require(!sidecar.optBoolean("lens_calibration_used", true)) {
            "Sidecar claimt lens-calibratiegebruik."
        }
        require(!sidecar.optBoolean("camera_model_used", true)) {
            "Sidecar claimt camera-modelgebruik."
        }
        require(!sidecar.optBoolean("vendor_mapping_used", true)) {
            "Sidecar claimt vendor-mapgebruik."
        }
        require(!sidecar.optBoolean("measured_anchors_modified", true)) {
            "Sidecar claimt gewijzigde measured anchors."
        }
        require(!sidecar.optBoolean("unanchored_values_promoted_to_measured", true)) {
            "Sidecar claimt promotion van unanchored naar MEASURED."
        }
        require(!sidecar.optBoolean("model_bank_applied_to_scientific_master", true)) {
            "Sidecar claimt Scientific-Master toepassing."
        }
        require(!sidecar.optBoolean("candidate_applied", true)) {
            "Sidecar claimt toegepaste researchkandidaat."
        }
        require(!sidecar.optBoolean("creates_new_evidence", true)) {
            "Sidecar claimt nieuwe evidence."
        }
        require(!sidecar.optBoolean("scientific_writeback_allowed", true)) {
            "Sidecar claimt scientific writeback."
        }
        val global = sidecar.optJSONObject("global")
            ?: error("Universal model-bank sidecar global ontbreekt.")
        require(global.optLong("holdouts", 0L) > 0L) {
            "Universal model-bank sidecar bevat geen holdouts."
        }
        require(
            sidecar.optJSONArray("holdout_records")?.length()?.toLong() ==
                global.optLong("holdouts", -1L)
        ) {
            "Universal model-bank holdout record-count mismatch."
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
            .put("target_censor_state_used_for_holdout_admission", true)
            .put("target_numeric_stage2_value_read_before_selection", false)
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
