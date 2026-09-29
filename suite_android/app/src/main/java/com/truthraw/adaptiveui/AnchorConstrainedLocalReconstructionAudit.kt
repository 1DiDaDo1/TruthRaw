package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import android.os.ParcelFileDescriptor
import org.json.JSONArray
import org.json.JSONObject
import java.io.File

object AnchorConstrainedLocalReconstructionBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportAndVerify(
        sourceFd: Int,
        destinationFd: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
        analysisWidth: Int,
        analysisHeight: Int,
        frontsideTiles: IntArray,
    ): String
}

/**
 * Private holdout validation of a local raster-independent lattice solver.
 *
 * The held-out source sample remains an immutable measured anchor in the
 * source. The solver simply does not read its value while estimating it from
 * neighbouring measured anchors. The true value is revealed only afterwards
 * for validation.
 */
object AnchorConstrainedLocalReconstructionAudit {
    private const val MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
    private const val MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

    fun unavailable(
        sourceSha256: String,
        reason: String,
    ): JSONObject =
        JSONObject()
            .put(
                "schema",
                "D.RAW/Frontside/AnchorConstrainedLocalReconstruction/0.1",
            )
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("source_sha256", sourceSha256)
            .put("authority", "PRIVATE_RECONSTRUCTION_AUDIT_ONLY")
            .put("holdout_validation_available", false)
            .put("target_value_used_by_solver", false)
            .put("measured_anchors_modified", false)
            .put("unanchored_values_promoted_to_measured", false)
            .put("reconstructed_authority_only", true)
            .put("uncertainty_diagnostic_only", true)
            .put("noise_independence_admitted", false)
            .put("solver_applied_to_scientific_master", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)

    fun skipped(
        sourceSha256: String,
        reason: String,
    ): JSONObject =
        unavailable(sourceSha256, reason)
            .put("status", "NOT_REQUIRED_BY_CURRENT_FRONT_SIDE_STATE")

    fun analyze(
        resolver: ContentResolver,
        sourceUri: Uri,
        sourceSha256: String,
        cacheDir: File,
        frontsideV01: JSONObject?,
        sampleLattice: JSONObject?,
        supportDistance: JSONObject?,
    ): JSONObject {
        val contractError =
            validateInputContract(
                sourceSha256,
                frontsideV01,
                sampleLattice,
                supportDistance,
            )
        if (contractError != null) {
            return unavailable(sourceSha256, contractError)
        }

        val front = frontsideV01!!
        val candidateData = buildCandidateData(front)
        if (candidateData.isEmpty()) {
            return skipped(
                sourceSha256,
                "NO_VISIBLE_DARK_CHROMA_CANDIDATES",
            )
        }

        val analysisWidth = front.optInt("analysis_width", 0)
        val analysisHeight = front.optInt("analysis_height", 0)

        val temp = runCatching {
            File.createTempFile(
                "draw_anchor_holdout_",
                ".json",
                cacheDir,
            )
        }.getOrNull()
            ?: return unavailable(sourceSha256, "TEMP_FILE_CREATE_FAILED")

        try {
            val statusText = try {
                resolver.openFileDescriptor(sourceUri, "r")?.use { src ->
                    ParcelFileDescriptor.open(
                        temp,
                        ParcelFileDescriptor.MODE_CREATE or
                            ParcelFileDescriptor.MODE_TRUNCATE or
                            ParcelFileDescriptor.MODE_READ_WRITE,
                    ).use { dst ->
                        AnchorConstrainedLocalReconstructionBridge
                            .exportAndVerify(
                                src.fd,
                                dst.fd,
                                MAX_SOURCE_RESIDENT_BYTES,
                                MAX_LOGICAL_RESIDENT_BYTES,
                                analysisWidth,
                                analysisHeight,
                                candidateData,
                            )
                    }
                }
            } catch (error: Throwable) {
                null
            } ?: return unavailable(
                sourceSha256,
                "ANCHOR_HOLDOUT_BRIDGE_FAILED",
            )

            val status = runCatching { JSONObject(statusText) }.getOrNull()
                ?: return unavailable(
                    sourceSha256,
                    "ANCHOR_HOLDOUT_STATUS_INVALID",
                )

            val statusError = validateStatus(sourceSha256, status)
            if (statusError != null) {
                return unavailable(sourceSha256, statusError)
            }

            val rawText = runCatching { temp.readText() }.getOrNull()
                ?: return unavailable(
                    sourceSha256,
                    "ANCHOR_HOLDOUT_FILE_READ_FAILED",
                )
            val sidecar = runCatching { JSONObject(rawText) }.getOrNull()
                ?: return unavailable(
                    sourceSha256,
                    "ANCHOR_HOLDOUT_FILE_INVALID",
                )

            val sidecarError =
                validateSidecar(sourceSha256, sidecar)
            if (sidecarError != null) {
                return unavailable(sourceSha256, sidecarError)
            }

            val global = sidecar.optJSONObject("global") ?: JSONObject()
            val supportQueries =
                supportDistance?.optJSONArray("queries") ?: JSONArray()
            val supportById = HashMap<Int, JSONObject>()
            for (i in 0 until supportQueries.length()) {
                val q = supportQueries.optJSONObject(i) ?: continue
                supportById[q.optInt("id")] = q
            }

            val compactQueries = JSONArray()
            val auditQueries =
                sidecar.optJSONArray("queries") ?: JSONArray()
            for (i in 0 until auditQueries.length()) {
                val q = auditQueries.optJSONObject(i) ?: continue
                val id = q.optInt("id")
                val support = supportById[id]
                compactQueries.put(
                    JSONObject()
                        .put("id", id)
                        .put("frontside_x", q.optInt("frontside_x"))
                        .put("frontside_y", q.optInt("frontside_y"))
                        .put(
                            "source_rect",
                            q.optJSONArray("source_rect") ?: JSONArray(),
                        )
                        .put("holdouts", q.optLong("holdouts", 0L))
                        .put("solver_valid", q.optLong("solver_valid", 0L))
                        .put(
                            "baseline_valid",
                            q.optLong("baseline_valid", 0L),
                        )
                        .put(
                            "solver_lower_abs_error",
                            q.optLong("solver_lower_abs_error", 0L),
                        )
                        .put(
                            "baseline_lower_abs_error",
                            q.optLong("baseline_lower_abs_error", 0L),
                        )
                        .put(
                            "solver_mae",
                            q.optDouble("solver_mae", Double.NaN),
                        )
                        .put(
                            "baseline_mae",
                            q.optDouble("baseline_mae", Double.NaN),
                        )
                        .put(
                            "solver_rmse",
                            q.optDouble("solver_rmse", Double.NaN),
                        )
                        .put(
                            "baseline_rmse",
                            q.optDouble("baseline_rmse", Double.NaN),
                        )
                        .put(
                            "solver_coverage_1sigma",
                            q.optDouble(
                                "solver_coverage_1sigma",
                                Double.NaN,
                            ),
                        )
                        .put(
                            "solver_coverage_2sigma",
                            q.optDouble(
                                "solver_coverage_2sigma",
                                Double.NaN,
                            ),
                        )
                        .put(
                            "solver_coverage_3sigma",
                            q.optDouble(
                                "solver_coverage_3sigma",
                                Double.NaN,
                            ),
                        )
                        .put(
                            "nearest_structure_from_center",
                            support?.opt(
                                "nearest_structure_from_center",
                            ) ?: JSONObject.NULL,
                        )
                        .put(
                            "nearest_structure_to_rect",
                            support?.opt(
                                "nearest_structure_to_rect",
                            ) ?: JSONObject.NULL,
                        )
                        .put(
                            "center_structure_fraction",
                            support?.optJSONArray(
                                "center_structure_fraction",
                            ) ?: JSONArray(),
                        )
                        .put(
                            "rect_margin_structure_fraction",
                            support?.optJSONArray(
                                "rect_margin_structure_fraction",
                            ) ?: JSONArray(),
                        )
                        .put(
                            "holdout_target_is_noisy_measurement",
                            true,
                        )
                        .put(
                            "lower_holdout_error_is_scene_truth_proof",
                            false,
                        )
                        .put("promotion_allowed", false),
                )
            }

            val solverValid = global.optLong("solver_valid", 0L)
            val baselineValid = global.optLong("baseline_valid", 0L)
            val bothValid = global.optLong("both_valid", 0L)
            val solverWins =
                global.optLong("solver_lower_abs_error", 0L)
            val baselineWins =
                global.optLong("baseline_lower_abs_error", 0L)
            val ties = global.optLong("equal_abs_error", 0L)

            return JSONObject()
                .put(
                    "schema",
                    "D.RAW/Frontside/AnchorConstrainedLocalReconstruction/0.1",
                )
                .put(
                    "status",
                    "AUDIT_ONLY_HOLDOUT_VALIDATION_AVAILABLE",
                )
                .put("source_sha256", sourceSha256)
                .put(
                    "authority",
                    "PRIVATE_RECONSTRUCTION_AUDIT_ONLY",
                )
                .put(
                    "solver_name",
                    sidecar.optString("solver_name"),
                )
                .put(
                    "baseline_name",
                    sidecar.optString("baseline_name"),
                )
                .put(
                    "lattice_units_per_source_pixel",
                    sidecar.optLong(
                        "lattice_units_per_source_pixel",
                        0L,
                    ),
                )
                .put(
                    "holdout_period",
                    sidecar.optInt("holdout_period", 0),
                )
                .put(
                    "solver_support_radius_source_px",
                    sidecar.optInt(
                        "solver_support_radius_source_px",
                        0,
                    ),
                )
                .put(
                    "holdout_stream_sha256",
                    sidecar.optString("holdout_stream_sha256"),
                )
                .put(
                    "sidecar_json_sha256",
                    status.optString("jsonSha256"),
                )
                .put(
                    "truthnegative_state_sha256",
                    sidecar.optString("truthnegative_state_sha256"),
                )
                .put(
                    "global",
                    JSONObject()
                        .put(
                            "holdouts",
                            global.optLong("holdouts", 0L),
                        )
                        .put("solver_valid", solverValid)
                        .put("baseline_valid", baselineValid)
                        .put("both_valid", bothValid)
                        .put(
                            "solver_lower_abs_error",
                            solverWins,
                        )
                        .put(
                            "baseline_lower_abs_error",
                            baselineWins,
                        )
                        .put("equal_abs_error", ties)
                        .put(
                            "solver_lower_abs_error_fraction_among_both",
                            if (bothValid > 0L) {
                                solverWins.toDouble() /
                                    bothValid.toDouble()
                            } else {
                                0.0
                            },
                        )
                        .put(
                            "baseline_lower_abs_error_fraction_among_both",
                            if (bothValid > 0L) {
                                baselineWins.toDouble() /
                                    bothValid.toDouble()
                            } else {
                                0.0
                            },
                        )
                        .put(
                            "solver_mae",
                            global.optDouble("solver_mae", Double.NaN),
                        )
                        .put(
                            "baseline_mae",
                            global.optDouble(
                                "baseline_mae",
                                Double.NaN,
                            ),
                        )
                        .put(
                            "solver_rmse",
                            global.optDouble(
                                "solver_rmse",
                                Double.NaN,
                            ),
                        )
                        .put(
                            "baseline_rmse",
                            global.optDouble(
                                "baseline_rmse",
                                Double.NaN,
                            ),
                        )
                        .put(
                            "solver_bias",
                            global.optDouble(
                                "solver_bias",
                                Double.NaN,
                            ),
                        )
                        .put(
                            "solver_coverage_1sigma",
                            global.optDouble(
                                "solver_coverage_1sigma",
                                Double.NaN,
                            ),
                        )
                        .put(
                            "solver_coverage_2sigma",
                            global.optDouble(
                                "solver_coverage_2sigma",
                                Double.NaN,
                            ),
                        )
                        .put(
                            "solver_coverage_3sigma",
                            global.optDouble(
                                "solver_coverage_3sigma",
                                Double.NaN,
                            ),
                        )
                        .put(
                            "solver_mean_variance_inflation",
                            global.optDouble(
                                "solver_mean_variance_inflation",
                                Double.NaN,
                            ),
                        ),
                )
                .put("queries", compactQueries)
                .put("holdout_validation_available", true)
                .put("holdout_target_is_noisy_measurement", true)
                .put(
                    "holdout_error_is_scene_truth_metric",
                    false,
                )
                .put(
                    "lower_holdout_error_proves_denoising",
                    false,
                )
                .put(
                    "target_value_used_by_solver",
                    false,
                )
                .put("measured_anchors_modified", false)
                .put(
                    "unanchored_values_promoted_to_measured",
                    false,
                )
                .put("reconstructed_authority_only", true)
                .put("uncertainty_diagnostic_only", true)
                .put("noise_independence_admitted", false)
                .put("solver_applied_to_scientific_master", false)
                .put("distance_threshold_admitted", false)
                .put("can_reduce_protection", false)
                .put("can_enable_correction", false)
                .put("chroma_correction_supported", false)
                .put("private_ab_delta_allowed", false)
                .put("candidate_applied", false)
                .put("creates_new_evidence", false)
                .put("scientific_writeback_allowed", false)
        } finally {
            runCatching { temp.delete() }
        }
    }

    fun exportSidecar(
        resolver: ContentResolver,
        sourceUri: Uri,
        destinationUri: Uri,
        expectedSourceSha256: String,
        frontsideV01: JSONObject?,
        sampleLattice: JSONObject?,
        supportDistance: JSONObject?,
    ): JSONObject {
        val contractError =
            validateInputContract(
                expectedSourceSha256,
                frontsideV01,
                sampleLattice,
                supportDistance,
            )
        require(contractError == null) {
            contractError ?: "Onbekende inputfout."
        }

        val front = frontsideV01!!
        val analysisWidth = front.optInt("analysis_width", 0)
        val analysisHeight = front.optInt("analysis_height", 0)
        val candidateData = buildCandidateData(front)
        require(candidateData.isNotEmpty()) {
            "Geen zichtbare Dark-Chroma-kandidaten voor holdout-validatie."
        }

        val statusText =
            resolver.openFileDescriptor(sourceUri, "r")?.use { src ->
                resolver.openFileDescriptor(destinationUri, "rw")?.use { dst ->
                    AnchorConstrainedLocalReconstructionBridge.exportAndVerify(
                        src.fd,
                        dst.fd,
                        MAX_SOURCE_RESIDENT_BYTES,
                        MAX_LOGICAL_RESIDENT_BYTES,
                        analysisWidth,
                        analysisHeight,
                        candidateData,
                    )
                }
            } ?: error("Bron of bestemming kon niet worden geopend.")

        val status = JSONObject(statusText)
        val statusError =
            validateStatus(expectedSourceSha256, status)
        require(statusError == null) {
            statusError ?: "Onbekende native exportfout."
        }
        return status
    }

    private fun validateInputContract(
        sourceSha256: String,
        frontsideV01: JSONObject?,
        sampleLattice: JSONObject?,
        supportDistance: JSONObject?,
    ): String? {
        if (
            frontsideV01 == null ||
            frontsideV01.optString("schema") !=
                "D.RAW/Frontside/DarkChromaStability/0.1" ||
            frontsideV01.optString("source_sha256") != sourceSha256
        ) {
            return "FRONTSIDE_V0_1_BINDING_MISMATCH"
        }
        if (
            sampleLattice == null ||
            sampleLattice.optString("schema") !=
                "D.RAW/RasterIndependentSampleLattice/0.1" ||
            sampleLattice.optString("source_sha256") != sourceSha256 ||
            sampleLattice.optString("status") != "AVAILABLE" ||
            sampleLattice.optBoolean("upscaling_performed", true) ||
            sampleLattice.optBoolean("source_values_modified", true)
        ) {
            return "SAMPLE_LATTICE_BINDING_MISMATCH"
        }
        if (
            supportDistance == null ||
            supportDistance.optString("schema") !=
                "D.RAW/Frontside/N2SampleSupportDistance/0.1" ||
            supportDistance.optString("source_sha256") != sourceSha256 ||
            supportDistance.optString("status") !=
                "AUDIT_ONLY_DISTANCE_BINDING_AVAILABLE" ||
            !supportDistance.optBoolean(
                "exact_sample_coordinates_recorded",
                false,
            ) ||
            supportDistance.optBoolean("can_reduce_protection", true) ||
            supportDistance.optBoolean("can_enable_correction", true)
        ) {
            return "EXACT_SUPPORT_GEOMETRY_REQUIRED"
        }
        if (
            frontsideV01.optInt("analysis_width", 0) <= 0 ||
            frontsideV01.optInt("analysis_height", 0) <= 0
        ) {
            return "FRONTSIDE_DIMENSIONS_INVALID"
        }
        return null
    }

    private fun validateStatus(
        sourceSha256: String,
        status: JSONObject,
    ): String? {
        if (status.optInt("status", -999) != 0) {
            return "ANCHOR_HOLDOUT_STATUS_" +
                status.optInt("status", -999)
        }
        if (
            !status.optBoolean("postWriteVerified", false) ||
            status.optString("sourceSha256") != sourceSha256 ||
            status.optBoolean("targetValueUsedBySolver", true) ||
            status.optBoolean("measuredAnchorsModified", true) ||
            status.optBoolean(
                "unanchoredValuesPromotedToMeasured",
                true,
            ) ||
            !status.optBoolean("reconstructedAuthorityOnly", false) ||
            !status.optBoolean("uncertaintyDiagnosticOnly", false) ||
            status.optBoolean("noiseIndependenceAdmitted", true) ||
            status.optBoolean("solverAppliedToScientificMaster", true) ||
            status.optBoolean("candidateApplied", true) ||
            status.optBoolean("scientificWritebackAllowed", true)
        ) {
            return "ANCHOR_HOLDOUT_STATUS_CONTRACT_MISMATCH"
        }
        return null
    }

    private fun validateSidecar(
        sourceSha256: String,
        sidecar: JSONObject,
    ): String? {
        if (
            sidecar.optString("schema") !=
                "D.RAW/AnchorConstrainedLocalReconstructionAudit/0.1" ||
            sidecar.optString("source_sha256") != sourceSha256 ||
            sidecar.optBoolean("target_value_used_by_solver", true) ||
            sidecar.optBoolean("measured_anchors_modified", true) ||
            sidecar.optBoolean(
                "unanchored_values_promoted_to_measured",
                true,
            ) ||
            !sidecar.optBoolean("reconstructed_authority_only", false) ||
            !sidecar.optBoolean("uncertainty_diagnostic_only", false) ||
            sidecar.optBoolean("noise_independence_admitted", true) ||
            sidecar.optBoolean("solver_applied_to_scientific_master", true) ||
            sidecar.optBoolean("candidate_applied", true) ||
            sidecar.optBoolean("scientific_writeback_allowed", true)
        ) {
            return "ANCHOR_HOLDOUT_FILE_CONTRACT_MISMATCH"
        }
        val global = sidecar.optJSONObject("global") ?: return
            "ANCHOR_HOLDOUT_GLOBAL_MISSING"
        if (
            global.optLong("holdouts", 0L) <= 0L ||
            sidecar.optJSONArray("queries")?.length() ?: 0 <= 0 ||
            sidecar.optJSONArray("holdout_records")?.length() ?: 0 <= 0
        ) {
            return "ANCHOR_HOLDOUT_EMPTY"
        }
        return null
    }

    private fun buildCandidateData(frontsideV01: JSONObject): IntArray {
        val tiles = frontsideV01.optJSONArray("tiles") ?: JSONArray()
        val data = ArrayList<Int>()
        for (i in 0 until tiles.length()) {
            val t = tiles.optJSONObject(i) ?: continue
            if (
                !t.optBoolean(
                    "frontside_chroma_instability_candidate",
                    false,
                )
            ) {
                continue
            }
            data += t.optInt("x")
            data += t.optInt("y")
            data += t.optInt("width")
            data += t.optInt("height")
        }
        return data.toIntArray()
    }
}
