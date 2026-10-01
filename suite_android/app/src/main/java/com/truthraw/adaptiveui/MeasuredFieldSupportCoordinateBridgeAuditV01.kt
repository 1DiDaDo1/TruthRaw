package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.PI
import kotlin.math.abs
import kotlin.math.atan2
import kotlin.math.cos
import kotlin.math.floor
import kotlin.math.max
import kotlin.math.min
import kotlin.math.sin
import kotlin.math.sqrt

/**
 * Read-only FIELD_RESPONSE coordinate-bridge audit based on the actual
 * measured sparse source-grid positions that supported each field cell.
 *
 * Unlike the earlier bin-centre dry-runs, this module reconstructs the
 * geometric support of every (radial_bin, sensor_sector) cell from
 * BacksideSignalSupportAudit.sparse_measured_sample_grid. Only source_x and
 * source_y are used to build support centroids/footprints; photometric values
 * are not used to choose geometry or a variant.
 *
 * The constrained appearance similarity is still appearance-derived. Mapping
 * source-grid isotropic coordinates into the frontside-analysis isotropic
 * coordinate convention is therefore an explicitly unproven bridge. This is
 * diagnostic only and cannot promote registration, calibration or correction.
 */
object MeasuredFieldSupportCoordinateBridgeAuditV01 {
    const val SCHEMA =
        "D.RAW/MeasuredFieldSupportCoordinateBridgeAudit/0.1"

    private data class Geometry(
        val nominalRotationDegrees: Double,
        val residualRotationDegrees: Double,
        val uniformScale: Double,
        val translationXIso: Double,
        val translationYIso: Double,
    )

    private data class MutableSupportCell(
        var count: Int = 0,
        var sumX: Double = 0.0,
        var sumY: Double = 0.0,
        var sumX2: Double = 0.0,
        var sumY2: Double = 0.0,
        var minX: Double = Double.POSITIVE_INFINITY,
        var maxX: Double = Double.NEGATIVE_INFINITY,
        var minY: Double = Double.POSITIVE_INFINITY,
        var maxY: Double = Double.NEGATIVE_INFINITY,
    )

    private data class SupportCell(
        val count: Int,
        val centroidXSourcePx: Double,
        val centroidYSourcePx: Double,
        val centroidXIso: Double,
        val centroidYIso: Double,
        val centroidRho: Double,
        val centroidAngleRadians: Double,
        val footprintRmsRadiusSourcePx: Double,
        val footprintWidthSourcePx: Double,
        val footprintHeightSourcePx: Double,
    )

    private data class SourceSupportGrid(
        val cells: Map<String, SupportCell>,
        val sourceScalePx: Double,
        val cornerRadiusIso: Double,
        val sourceFieldAspect: Double,
        val frontsideAspect: Double,
        val aspectRelativeDifference: Double,
        val measuredPointCount: Int,
    )

    private data class Projection(
        val radialBin: Int,
        val worldSector: Int,
        val rho: Double,
        val azimuthRadians: Double,
    )

    private data class Variant(
        val id: String,
        val relationBaseline: Boolean = false,
        val useResidualRotation: Boolean = false,
        val useUniformScale: Boolean = false,
        val useTranslation: Boolean = false,
    )

    private data class VariantStats(
        var mappedTrain: Int = 0,
        var mappedHeld: Int = 0,
        var changedTrain: Int = 0,
        var changedHeld: Int = 0,
    )

    private val variants =
        listOf(
            Variant(
                id = "RELATION_NOMINAL_WORLD_CELL",
                relationBaseline = true,
            ),
            Variant(
                id = "MEASURED_SUPPORT_NOMINAL_ROTATION",
            ),
            Variant(
                id = "MEASURED_SUPPORT_RESIDUAL_ROTATION",
                useResidualRotation = true,
            ),
            Variant(
                id = "MEASURED_SUPPORT_UNIFORM_SCALE",
                useUniformScale = true,
            ),
            Variant(
                id = "MEASURED_SUPPORT_TRANSLATION",
                useTranslation = true,
            ),
            Variant(
                id = "MEASURED_SUPPORT_RESIDUAL_ROTATION_PLUS_SCALE",
                useResidualRotation = true,
                useUniformScale = true,
            ),
            Variant(
                id = "MEASURED_SUPPORT_RESIDUAL_ROTATION_PLUS_TRANSLATION",
                useResidualRotation = true,
                useTranslation = true,
            ),
            Variant(
                id = "MEASURED_SUPPORT_FULL_RESIDUAL_SIMILARITY",
                useResidualRotation = true,
                useUniformScale = true,
                useTranslation = true,
            ),
        )

    fun evaluate(
        profiles: List<JSONObject>,
        records: List<JSONObject>,
        constrainedAudit: JSONObject,
    ): JSONObject {
        val auditReports =
            constrainedAudit.optJSONArray("record_reports")
                ?: JSONArray()
        val reports = JSONArray()
        var admitted = 0
        var available = 0
        var unavailable = 0

        for ((recordIndex, record) in records.withIndex()) {
            if (
                record.optString("axis_scope") !=
                "FIELD_RESPONSE"
            ) {
                continue
            }
            val payload =
                record.optJSONObject("axis_payload")
                    ?: continue
            if (
                !payload.optBoolean(
                    "controlled_rotation_relation",
                    false,
                )
            ) {
                continue
            }
            val admission =
                CalibrationObservationAdmissionV01
                    .admitForNumericCandidate(
                        record = record,
                        axis = "FIELD_RESPONSE",
                    )
            if (
                admission.optString("status") !=
                "NUMERIC_CANDIDATE_RELATION_ADMITTED"
            ) {
                continue
            }

            admitted++
            val constrainedReport =
                findRecordReport(
                    reports = auditReports,
                    recordIndex = recordIndex,
                )
            val report =
                if (constrainedReport == null) {
                    unavailableRecord(
                        recordIndex,
                        "CONSTRAINED_GEOMETRY_RECORD_REPORT_MISSING",
                    )
                } else {
                    evaluateRecord(
                        profiles = profiles,
                        record = record,
                        recordIndex = recordIndex,
                        constrainedReport =
                            constrainedReport,
                    )
                }
            reports.put(report)
            if (
                report.optString("status") ==
                "MEASURED_FIELD_SUPPORT_COORDINATE_BRIDGE_AUDIT_AVAILABLE"
            ) {
                available++
            } else {
                unavailable++
            }
        }

        if (admitted == 0) {
            return unavailableSet(
                "NO_ADMITTED_CONTROLLED_ROTATION_FIELD_RECORD",
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (available > 0) {
                    "MEASURED_FIELD_SUPPORT_COORDINATE_BRIDGE_AUDIT_SET_AVAILABLE"
                } else {
                    "UNKNOWN_FAIL_CLOSED"
                },
            )
            .put("admitted_record_count", admitted)
            .put("available_record_count", available)
            .put("unavailable_record_count", unavailable)
            .put("record_reports", reports)
            .put("automatic_variant_winner_selected", false)
            .put("field_response_calibration_promoted", false)
            .put("world_registration_promoted", false)
            .put("correction_authorized", false)
            .put("candidate_applied", false)
            .put("source_sample_values_modified", false)
            .put("source_sample_positions_modified", false)
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun evaluateRecord(
        profiles: List<JSONObject>,
        record: JSONObject,
        recordIndex: Int,
        constrainedReport: JSONObject,
    ): JSONObject {
        if (
            constrainedReport.optString("status") !=
            "CONTROLLED_ROTATION_CONSTRAINED_GEOMETRY_AUDIT_AVAILABLE"
        ) {
            return unavailableRecord(
                recordIndex,
                "CONSTRAINED_GEOMETRY_AUDIT_UNAVAILABLE",
            )
        }

        val payload =
            record.optJSONObject("axis_payload")
                ?: return unavailableRecord(
                    recordIndex,
                    "AXIS_PAYLOAD_MISSING",
                )
        val radialBins =
            payload.optInt("radial_bin_count", 0)
        val azimuthBins =
            payload.optInt("azimuth_bin_count", 0)
        if (radialBins < 2 || azimuthBins < 2) {
            return unavailableRecord(
                recordIndex,
                "FIELD_BIN_GEOMETRY_INVALID",
            )
        }

        val bySource =
            profiles
                .mapNotNull { profile ->
                    val source =
                        profile.optString("source_sha256")
                            .trim()
                            .lowercase()
                    if (source.isBlank()) {
                        null
                    } else {
                        source to profile
                    }
                }
                .toMap()

        val diagnostics =
            constrainedReport.optJSONArray(
                "observation_diagnostics",
            ) ?: return unavailableRecord(
                recordIndex,
                "CONSTRAINED_OBSERVATION_DIAGNOSTICS_MISSING",
            )
        val geometryBySource =
            linkedMapOf<String, Geometry>()
        for (i in 0 until diagnostics.length()) {
            val d =
                diagnostics.optJSONObject(i)
                    ?: continue
            val source =
                d.optString("source_sha256")
                    .trim()
                    .lowercase()
            if (source.isBlank()) continue
            val status = d.optString("status")
            val accepted =
                status ==
                    "ROTATION_CONSTRAINED_ANCHOR_IDENTITY" ||
                    (
                        status ==
                            "ROTATION_CONSTRAINED_SIMILARITY_CANDIDATE_AVAILABLE" &&
                            d.optBoolean(
                                "diagnostic_bounds_pass",
                                false,
                            )
                        )
            if (!accepted) continue

            val nominal =
                d.optDouble(
                    "nominal_rotation_degrees",
                    Double.NaN,
                )
            val residual =
                d.optDouble(
                    "residual_rotation_degrees",
                    Double.NaN,
                )
            val scale =
                d.optDouble(
                    "uniform_scale",
                    Double.NaN,
                )
            val tx =
                d.optDouble(
                    "translation_x_isotropic",
                    Double.NaN,
                )
            val ty =
                d.optDouble(
                    "translation_y_isotropic",
                    Double.NaN,
                )
            if (
                !nominal.isFinite() ||
                !residual.isFinite() ||
                !scale.isFinite() ||
                scale <= 0.0 ||
                !tx.isFinite() ||
                !ty.isFinite()
            ) {
                continue
            }
            geometryBySource[source] =
                Geometry(
                    nominalRotationDegrees = nominal,
                    residualRotationDegrees = residual,
                    uniformScale = scale,
                    translationXIso = tx,
                    translationYIso = ty,
                )
        }

        val anchorSource =
            constrainedReport.optString(
                "anchor_source_sha256",
            ).trim().lowercase()
        val anchorProfile =
            bySource[anchorSource]
                ?: return unavailableRecord(
                    recordIndex,
                    "ANCHOR_PROFILE_MISSING",
                )
        val anchorGrid =
            buildSupportGrid(
                profile = anchorProfile,
                radialBins = radialBins,
                azimuthBins = azimuthBins,
            ) ?: return unavailableRecord(
                recordIndex,
                "ANCHOR_MEASURED_SPARSE_FIELD_SUPPORT_UNAVAILABLE",
            )

        val roots =
            record.optJSONArray(
                "source_sha256_roots",
            ) ?: JSONArray()
        val supportBySource =
            linkedMapOf<String, SourceSupportGrid>()
        val sourceDiagnostics = JSONArray()

        for (i in 0 until roots.length()) {
            val source =
                roots.optString(i)
                    .trim()
                    .lowercase()
            if (source.isBlank()) continue
            if (source !in geometryBySource) {
                return unavailableRecord(
                    recordIndex,
                    "COMPLETE_CONSTRAINED_GEOMETRY_REQUIRED_FOR_ALL_ROOTS",
                ).put("missing_source_sha256", source)
            }
            val profile =
                bySource[source]
                    ?: return unavailableRecord(
                        recordIndex,
                        "ACTIVE_PROFILE_MISSING",
                    ).put(
                        "missing_source_sha256",
                        source,
                    )
            val grid =
                buildSupportGrid(
                    profile = profile,
                    radialBins = radialBins,
                    azimuthBins = azimuthBins,
                ) ?: return unavailableRecord(
                    recordIndex,
                    "MEASURED_SPARSE_FIELD_SUPPORT_UNAVAILABLE",
                ).put(
                    "missing_source_sha256",
                    source,
                )
            supportBySource[source] = grid

            val deviation =
                supportCentroidDeviationSummary(
                    grid = grid,
                    radialBins = radialBins,
                    azimuthBins = azimuthBins,
                )
            sourceDiagnostics.put(
                JSONObject()
                    .put("source_sha256", source)
                    .put(
                        "measured_sparse_point_count",
                        grid.measuredPointCount,
                    )
                    .put(
                        "populated_field_support_cell_count",
                        grid.cells.size,
                    )
                    .put(
                        "source_field_aspect_ratio",
                        grid.sourceFieldAspect,
                    )
                    .put(
                        "frontside_aspect_ratio",
                        grid.frontsideAspect,
                    )
                    .put(
                        "aspect_ratio_relative_difference",
                        grid.aspectRelativeDifference,
                    )
                    .put(
                        "source_corner_radius_isotropic",
                        grid.cornerRadiusIso,
                    )
                    .put(
                        "support_centroid_deviation_from_theoretical_bin_center",
                        deviation,
                    )
                    .put(
                        "support_coordinates_authority",
                        "MEASURED_SOURCE_SAMPLE_POSITIONS",
                    )
                    .put(
                        "support_centroid_is_measured_sample_position",
                        false,
                    )
                    .put(
                        "source_grid_to_frontside_isotropic_bridge_proven",
                        false,
                    ),
            )
        }

        val samples =
            payload.optJSONArray("samples")
                ?: return unavailableRecord(
                    recordIndex,
                    "FIELD_SAMPLES_MISSING",
                )

        val arrays =
            linkedMapOf<String, JSONArray>()
        val stats =
            linkedMapOf<String, VariantStats>()
        for (variant in variants) {
            arrays[variant.id] = JSONArray()
            stats[variant.id] = VariantStats()
        }

        var commonTrain = 0
        var commonHeld = 0
        var missingSupportTrain = 0
        var missingSupportHeld = 0
        var outsideAnyVariantTrain = 0
        var outsideAnyVariantHeld = 0

        for (i in 0 until samples.length()) {
            val sample =
                samples.optJSONObject(i)
                    ?: continue
            val source =
                sample.optString("source_sha256")
                    .trim()
                    .lowercase()
            val role =
                sample.optString(
                    "role",
                    "TRAIN",
                )
            val radialBin =
                sample.optInt(
                    "radial_bin",
                    -1,
                )
            val sensorSector =
                sample.optInt(
                    "sensor_sector",
                    -1,
                )
            val nominalWorldSector =
                sample.optInt(
                    "world_sector",
                    -1,
                )
            if (
                radialBin !in 0 until radialBins ||
                sensorSector !in 0 until azimuthBins ||
                nominalWorldSector !in 0 until azimuthBins
            ) {
                if (role == "HELD_OUT") {
                    missingSupportHeld++
                } else {
                    missingSupportTrain++
                }
                continue
            }

            val grid =
                supportBySource[source]
            val geometry =
                geometryBySource[source]
            val support =
                grid?.cells?.get(
                    supportCellKey(
                        radialBin,
                        sensorSector,
                    ),
                )
            if (
                grid == null ||
                geometry == null ||
                support == null
            ) {
                if (role == "HELD_OUT") {
                    missingSupportHeld++
                } else {
                    missingSupportTrain++
                }
                continue
            }

            val projections =
                linkedMapOf<String, Projection?>()
            var allMapped = true
            for (variant in variants) {
                val projection =
                    if (variant.relationBaseline) {
                        Projection(
                            radialBin = radialBin,
                            worldSector =
                                nominalWorldSector,
                            rho =
                                support.centroidRho,
                            azimuthRadians =
                                support.centroidAngleRadians,
                        )
                    } else {
                        projectMeasuredSupportCentroid(
                            support = support,
                            geometry = geometry,
                            anchorGrid = anchorGrid,
                            radialBins = radialBins,
                            azimuthBins = azimuthBins,
                            variant = variant,
                        )
                    }
                projections[variant.id] =
                    projection
                if (projection == null) {
                    allMapped = false
                }
            }

            if (!allMapped) {
                if (role == "HELD_OUT") {
                    outsideAnyVariantHeld++
                } else {
                    outsideAnyVariantTrain++
                }
                continue
            }

            if (role == "HELD_OUT") {
                commonHeld++
            } else {
                commonTrain++
            }

            val nominalWorldId =
                sample.optString("world_cell_id")
            for (variant in variants) {
                val projection =
                    projections[variant.id]
                        ?: continue
                val derivedWorldId =
                    worldCellId(
                        projection.radialBin,
                        projection.worldSector,
                    )
                val derived =
                    JSONObject(sample.toString())
                        .put(
                            "world_cell_id",
                            derivedWorldId,
                        )
                        .put(
                            "measured_support_point_count",
                            support.count,
                        )
                        .put(
                            "measured_support_centroid_x_source_px",
                            support.centroidXSourcePx,
                        )
                        .put(
                            "measured_support_centroid_y_source_px",
                            support.centroidYSourcePx,
                        )
                        .put(
                            "measured_support_footprint_rms_radius_source_px",
                            support.footprintRmsRadiusSourcePx,
                        )
                        .put(
                            "measured_support_coordinate_variant",
                            variant.id,
                        )
                        .put(
                            "measured_support_projected_world_radial_bin",
                            projection.radialBin,
                        )
                        .put(
                            "measured_support_projected_world_sector",
                            projection.worldSector,
                        )
                arrays[variant.id]
                    ?.put(derived)

                val st =
                    stats[variant.id]
                        ?: continue
                val changed =
                    nominalWorldId !=
                        derivedWorldId
                if (role == "HELD_OUT") {
                    st.mappedHeld++
                    if (changed) st.changedHeld++
                } else {
                    st.mappedTrain++
                    if (changed) st.changedTrain++
                }
            }
        }

        if (commonTrain < 6 || commonHeld < 1) {
            return unavailableRecord(
                recordIndex,
                "INSUFFICIENT_COMMON_MEASURED_SUPPORT",
            )
                .put(
                    "common_training_sample_count",
                    commonTrain,
                )
                .put(
                    "common_held_out_sample_count",
                    commonHeld,
                )
        }

        val solverByVariant =
            linkedMapOf<String, JSONObject>()
        for (variant in variants) {
            val result =
                FieldResponseRotationSeparationCandidateV01
                    .evaluateDerivedDiagnosticSamples(
                        samples =
                            arrays[variant.id]
                                ?: JSONArray(),
                        sourceSha256Roots =
                            roots,
                    )
            solverByVariant[variant.id] =
                result
            if (
                result.optString("status") !=
                "WORLD_SENSOR_FIELD_SEPARATION_CANDIDATE_AVAILABLE"
            ) {
                return unavailableRecord(
                    recordIndex,
                    "MEASURED_SUPPORT_VARIANT_SOLVER_UNAVAILABLE",
                )
                    .put(
                        "failed_variant_id",
                        variant.id,
                    )
                    .put(
                        "failed_variant_status",
                        result.optString(
                            "status",
                        ),
                    )
                    .put(
                        "failed_variant_reason",
                        result.optString(
                            "reason",
                        ),
                    )
            }
        }

        val variantReports = JSONArray()
        for (variant in variants) {
            val result =
                solverByVariant[variant.id]
                    ?: JSONObject()
            val st =
                stats[variant.id]
                    ?: VariantStats()
            variantReports.put(
                JSONObject()
                    .put("variant_id", variant.id)
                    .put(
                        "relation_baseline",
                        variant.relationBaseline,
                    )
                    .put(
                        "uses_residual_rotation",
                        variant.useResidualRotation,
                    )
                    .put(
                        "uses_uniform_scale",
                        variant.useUniformScale,
                    )
                    .put(
                        "uses_translation",
                        variant.useTranslation,
                    )
                    .put(
                        "mapped_training_sample_count",
                        st.mappedTrain,
                    )
                    .put(
                        "mapped_held_out_sample_count",
                        st.mappedHeld,
                    )
                    .put(
                        "training_world_cell_changed_count",
                        st.changedTrain,
                    )
                    .put(
                        "held_out_world_cell_changed_count",
                        st.changedHeld,
                    )
                    .put(
                        "solver_training_sample_count",
                        result.optInt(
                            "training_sample_count",
                            0,
                        ),
                    )
                    .put(
                        "solver_held_out_sample_count",
                        result.optInt(
                            "held_out_sample_count",
                            0,
                        ),
                    )
                    .put(
                        "solver_training_rmse_ev",
                        result.opt(
                            "training_rmse_ev",
                        ) ?: JSONObject.NULL,
                    )
                    .put(
                        "solver_held_out_rmse_ev",
                        result.opt(
                            "held_out_rmse_ev",
                        ) ?: JSONObject.NULL,
                    )
                    .put("candidate_applied", false)
                    .put(
                        "scientific_writeback_allowed",
                        false,
                    ),
            )
        }

        val exactComparison =
            exactCommonPredictableComparison(
                arrays = arrays,
                solverByVariant = solverByVariant,
            )

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "MEASURED_FIELD_SUPPORT_COORDINATE_BRIDGE_AUDIT_AVAILABLE",
            )
            .put("record_index", recordIndex)
            .put(
                "anchor_source_sha256",
                anchorSource,
            )
            .put(
                "support_source",
                "BACKSIDE_SPARSE_MEASURED_SOURCE_GRID_POSITIONS",
            )
            .put(
                "support_cell_definition",
                "EXACT_SAME_12x12_RADIAL_AZIMUTH_BINNING_AS_MEASURED_FIELD_SIGNAL",
            )
            .put(
                "photometric_values_used_to_construct_support_geometry",
                false,
            )
            .put(
                "support_centroid_is_exact_measured_sample_position",
                false,
            )
            .put(
                "source_diagnostics",
                sourceDiagnostics,
            )
            .put(
                "common_source_sample_support",
                JSONObject()
                    .put(
                        "training_sample_count",
                        commonTrain,
                    )
                    .put(
                        "held_out_sample_count",
                        commonHeld,
                    )
                    .put(
                        "missing_support_training_sample_count",
                        missingSupportTrain,
                    )
                    .put(
                        "missing_support_held_out_sample_count",
                        missingSupportHeld,
                    )
                    .put(
                        "outside_any_variant_training_sample_count",
                        outsideAnyVariantTrain,
                    )
                    .put(
                        "outside_any_variant_held_out_sample_count",
                        outsideAnyVariantHeld,
                    )
                    .put(
                        "same_source_sample_set_used_for_all_variant_solvers",
                        true,
                    ),
            )
            .put("variant_reports", variantReports)
            .put(
                "exact_common_predictable_comparison",
                exactComparison,
            )
            .put(
                "interpretation_contract",
                JSONObject()
                    .put(
                        "measured_support_positions_are_source_evidence",
                        true,
                    )
                    .put(
                        "derived_support_centroid_is_source_evidence",
                        false,
                    )
                    .put(
                        "source_grid_to_frontside_isotropic_bridge_proven",
                        false,
                    )
                    .put(
                        "frontside_decoder_crop_binding_proven",
                        false,
                    )
                    .put(
                        "frontside_decoder_orientation_binding_proven",
                        false,
                    )
                    .put(
                        "appearance_residual_rotation_is_sensor_field_rotation_proven",
                        false,
                    )
                    .put(
                        "appearance_uniform_scale_is_sensor_field_scale_proven",
                        false,
                    )
                    .put(
                        "appearance_translation_is_sensor_field_translation_proven",
                        false,
                    )
                    .put(
                        "lower_rmse_is_physical_truth",
                        false,
                    )
                    .put(
                        "automatic_variant_winner_selected",
                        false,
                    )
                    .put(
                        "held_out_values_used_to_fit_components",
                        false,
                    )
                    .put(
                        "held_out_values_used_to_select_variant",
                        false,
                    ),
            )
            .put("field_response_calibration_promoted", false)
            .put("world_registration_promoted", false)
            .put("correction_authorized", false)
            .put("candidate_applied", false)
            .put("image_transform_applied", false)
            .put("source_sample_values_modified", false)
            .put("source_sample_positions_modified", false)
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun buildSupportGrid(
        profile: JSONObject,
        radialBins: Int,
        azimuthBins: Int,
    ): SourceSupportGrid? {
        val backside =
            profile.optJSONObject(
                "backside_signal_support",
            ) ?: return null
        val sparse =
            backside.optJSONObject(
                "sparse_measured_sample_grid",
            ) ?: return null
        if (
            sparse.optString("status") !=
            "MEASURED_SPARSE_SOURCE_GRID_AVAILABLE"
        ) {
            return null
        }
        val points =
            sparse.optJSONArray("points")
                ?: return null

        val field =
            profile.optJSONObject(
                "observation_optical_field_chart",
            ) ?: return null
        val active =
            field.optJSONArray("active_area")
                ?: return null
        if (active.length() < 4) return null
        val top = active.optInt(0, -1)
        val left = active.optInt(1, -1)
        val bottom = active.optInt(2, -1)
        val right = active.optInt(3, -1)
        if (
            top < 0 ||
            left < 0 ||
            bottom <= top ||
            right <= left
        ) {
            return null
        }

        val width = right - left
        val height = bottom - top
        val sourceScale =
            max(
                width - 1,
                height - 1,
            ).toDouble()
        if (
            !sourceScale.isFinite() ||
            sourceScale <= 0.0
        ) {
            return null
        }
        val cx =
            (left.toDouble() +
                right.toDouble() -
                1.0) *
                0.5
        val cy =
            (top.toDouble() +
                bottom.toDouble() -
                1.0) *
                0.5

        val corners =
            arrayOf(
                doubleArrayOf(
                    left.toDouble(),
                    top.toDouble(),
                ),
                doubleArrayOf(
                    (right - 1).toDouble(),
                    top.toDouble(),
                ),
                doubleArrayOf(
                    left.toDouble(),
                    (bottom - 1).toDouble(),
                ),
                doubleArrayOf(
                    (right - 1).toDouble(),
                    (bottom - 1).toDouble(),
                ),
            )
        var maxRadius = 0.0
        for (corner in corners) {
            val dx = corner[0] - cx
            val dy = corner[1] - cy
            maxRadius =
                max(
                    maxRadius,
                    sqrt(
                        dx * dx +
                            dy * dy,
                    ),
                )
        }
        if (!(maxRadius > 0.0)) return null

        val mutable =
            linkedMapOf<String, MutableSupportCell>()
        var measuredPointCount = 0
        for (i in 0 until points.length()) {
            val p =
                points.optJSONObject(i)
                    ?: continue
            val x =
                p.optDouble(
                    "source_x",
                    Double.NaN,
                )
            val y =
                p.optDouble(
                    "source_y",
                    Double.NaN,
                )
            if (
                !x.isFinite() ||
                !y.isFinite()
            ) {
                continue
            }
            val dx = x - cx
            val dy = y - cy
            val rho =
                (
                    sqrt(
                        dx * dx +
                            dy * dy,
                    ) /
                        maxRadius
                    ).coerceIn(
                    0.0,
                    1.0,
                )
            val radialBin =
                min(
                    radialBins - 1,
                    (
                        rho *
                            radialBins.toDouble()
                        ).toInt(),
                )
            var angle = atan2(dy, dx)
            if (angle < 0.0) {
                angle += 2.0 * PI
            }
            val sector =
                min(
                    azimuthBins - 1,
                    (
                        angle /
                            (2.0 * PI) *
                            azimuthBins.toDouble()
                        ).toInt(),
                )

            val cell =
                mutable.getOrPut(
                    supportCellKey(
                        radialBin,
                        sector,
                    ),
                ) {
                    MutableSupportCell()
                }
            cell.count++
            cell.sumX += x
            cell.sumY += y
            cell.sumX2 += x * x
            cell.sumY2 += y * y
            cell.minX = min(cell.minX, x)
            cell.maxX = max(cell.maxX, x)
            cell.minY = min(cell.minY, y)
            cell.maxY = max(cell.maxY, y)
            measuredPointCount++
        }

        if (mutable.isEmpty()) return null

        val cells =
            linkedMapOf<String, SupportCell>()
        for ((key, cell) in mutable) {
            if (cell.count <= 0) continue
            val n = cell.count.toDouble()
            val centroidX = cell.sumX / n
            val centroidY = cell.sumY / n
            val dx = centroidX - cx
            val dy = centroidY - cy
            val r =
                sqrt(
                    dx * dx +
                        dy * dy,
                )
            val rho =
                (r / maxRadius)
                    .coerceIn(
                        0.0,
                        1.0,
                    )
            var angle =
                if (r > 0.0) {
                    atan2(dy, dx)
                } else {
                    0.0
                }
            if (angle < 0.0) {
                angle += 2.0 * PI
            }
            val variance =
                max(
                    0.0,
                    (
                        cell.sumX2 +
                            cell.sumY2
                        ) /
                        n -
                        centroidX *
                            centroidX -
                        centroidY *
                            centroidY,
                )
            cells[key] =
                SupportCell(
                    count = cell.count,
                    centroidXSourcePx = centroidX,
                    centroidYSourcePx = centroidY,
                    centroidXIso =
                        (centroidX - cx) /
                            sourceScale,
                    centroidYIso =
                        (centroidY - cy) /
                            sourceScale,
                    centroidRho = rho,
                    centroidAngleRadians = angle,
                    footprintRmsRadiusSourcePx =
                        sqrt(variance),
                    footprintWidthSourcePx =
                        cell.maxX -
                            cell.minX,
                    footprintHeightSourcePx =
                        cell.maxY -
                            cell.minY,
                )
        }

        val scene =
            profile.optJSONObject(
                "scene_analysis",
            ) ?: return null
        val front =
            scene.optJSONObject(
                "deterministic_local_feature_geometry_v0_1",
            ) ?: return null
        val frontWidth =
            front.optInt(
                "analysis_width",
                0,
            )
        val frontHeight =
            front.optInt(
                "analysis_height",
                0,
            )
        if (
            frontWidth < 2 ||
            frontHeight < 2
        ) {
            return null
        }
        val sourceAspect =
            width.toDouble() /
                height.toDouble()
        val frontAspect =
            frontWidth.toDouble() /
                frontHeight.toDouble()
        val aspectDelta =
            abs(
                frontAspect -
                    sourceAspect,
            ) /
                max(
                    abs(sourceAspect),
                    1.0e-12,
                )

        return SourceSupportGrid(
            cells = cells,
            sourceScalePx = sourceScale,
            cornerRadiusIso =
                maxRadius /
                    sourceScale,
            sourceFieldAspect = sourceAspect,
            frontsideAspect = frontAspect,
            aspectRelativeDifference =
                aspectDelta,
            measuredPointCount =
                measuredPointCount,
        )
    }

    private fun projectMeasuredSupportCentroid(
        support: SupportCell,
        geometry: Geometry,
        anchorGrid: SourceSupportGrid,
        radialBins: Int,
        azimuthBins: Int,
        variant: Variant,
    ): Projection? {
        var x =
            support.centroidXIso
        var y =
            support.centroidYIso

        if (variant.useTranslation) {
            x -=
                geometry.translationXIso
            y -=
                geometry.translationYIso
        }

        val inverseAngle =
            Math.toRadians(
                -(
                    geometry.nominalRotationDegrees +
                        if (
                            variant.useResidualRotation
                        ) {
                            geometry.residualRotationDegrees
                        } else {
                            0.0
                        }
                    ),
            )
        val c = cos(inverseAngle)
        val s = sin(inverseAngle)
        val rx =
            c * x -
                s * y
        val ry =
            s * x +
                c * y
        x = rx
        y = ry

        if (variant.useUniformScale) {
            if (
                !geometry.uniformScale.isFinite() ||
                geometry.uniformScale <= 0.0
            ) {
                return null
            }
            x /=
                geometry.uniformScale
            y /=
                geometry.uniformScale
        }

        val anchorCorner =
            anchorGrid.cornerRadiusIso
        if (
            !anchorCorner.isFinite() ||
            anchorCorner <= 0.0
        ) {
            return null
        }
        val worldX =
            x /
                anchorCorner
        val worldY =
            y /
                anchorCorner
        val rho =
            sqrt(
                worldX * worldX +
                    worldY * worldY,
            )
        if (
            !rho.isFinite() ||
            rho < 0.0 ||
            rho > 1.0
        ) {
            return null
        }

        var angle =
            atan2(
                worldY,
                worldX,
            )
        if (angle < 0.0) {
            angle += 2.0 * PI
        }
        val radialBin =
            minOf(
                radialBins - 1,
                floor(
                    rho *
                        radialBins.toDouble(),
                ).toInt(),
            )
        val sector =
            minOf(
                azimuthBins - 1,
                floor(
                    angle /
                        (2.0 * PI) *
                        azimuthBins.toDouble(),
                ).toInt(),
            )

        return Projection(
            radialBin = radialBin,
            worldSector = sector,
            rho = rho,
            azimuthRadians = angle,
        )
    }

    private fun supportCentroidDeviationSummary(
        grid: SourceSupportGrid,
        radialBins: Int,
        azimuthBins: Int,
    ): JSONObject {
        val radialAbs =
            ArrayList<Double>()
        val angularAbsDegrees =
            ArrayList<Double>()
        val counts =
            ArrayList<Double>()
        val footprintRms =
            ArrayList<Double>()

        for ((key, cell) in grid.cells) {
            val parts =
                key.split(':')
            if (parts.size != 2) continue
            val radialBin =
                parts[0].toIntOrNull()
                    ?: continue
            val sector =
                parts[1].toIntOrNull()
                    ?: continue
            val rhoCenter =
                (
                    radialBin.toDouble() +
                        0.5
                    ) /
                    radialBins.toDouble()
            val angleCenter =
                (
                    sector.toDouble() +
                        0.5
                    ) *
                    (
                        2.0 *
                            PI /
                            azimuthBins.toDouble()
                        )
            radialAbs +=
                abs(
                    cell.centroidRho -
                        rhoCenter,
                )
            val delta =
                wrapPi(
                    cell.centroidAngleRadians -
                        angleCenter,
                )
            angularAbsDegrees +=
                Math.toDegrees(
                    abs(delta),
                )
            counts +=
                cell.count.toDouble()
            footprintRms +=
                cell.footprintRmsRadiusSourcePx
        }

        return JSONObject()
            .put(
                "cell_count",
                radialAbs.size,
            )
            .put(
                "mean_abs_rho_deviation",
                meanOrNull(radialAbs),
            )
            .put(
                "max_abs_rho_deviation",
                maxOrNull(radialAbs),
            )
            .put(
                "mean_abs_azimuth_deviation_degrees",
                meanOrNull(
                    angularAbsDegrees,
                ),
            )
            .put(
                "max_abs_azimuth_deviation_degrees",
                maxOrNull(
                    angularAbsDegrees,
                ),
            )
            .put(
                "mean_support_point_count",
                meanOrNull(counts),
            )
            .put(
                "minimum_support_point_count",
                minOrNull(counts),
            )
            .put(
                "mean_footprint_rms_radius_source_px",
                meanOrNull(
                    footprintRms,
                ),
            )
    }

    private fun exactCommonPredictableComparison(
        arrays: Map<String, JSONArray>,
        solverByVariant: Map<String, JSONObject>,
    ): JSONObject {
        val first =
            arrays[variants.first().id]
                ?: JSONArray()
        val trainActual =
            ArrayList<Double>()
        val heldActual =
            ArrayList<Double>()
        val trainPredicted =
            linkedMapOf<String, ArrayList<Double>>()
        val heldPredicted =
            linkedMapOf<String, ArrayList<Double>>()
        for (variant in variants) {
            trainPredicted[variant.id] =
                ArrayList()
            heldPredicted[variant.id] =
                ArrayList()
        }

        for (i in 0 until first.length()) {
            val reference =
                first.optJSONObject(i)
                    ?: continue
            val actual =
                reference.optDouble(
                    "relative_signal_ev",
                    Double.NaN,
                )
            val sensorId =
                reference.optString(
                    "sensor_cell_id",
                )
            if (
                !actual.isFinite() ||
                sensorId.isBlank()
            ) {
                continue
            }

            val predictions =
                linkedMapOf<String, Double>()
            var predictableByAll = true
            for (variant in variants) {
                val sample =
                    arrays[variant.id]
                        ?.optJSONObject(i)
                val solver =
                    solverByVariant[variant.id]
                if (
                    sample == null ||
                    solver == null
                ) {
                    predictableByAll = false
                    break
                }
                val worldId =
                    sample.optString(
                        "world_cell_id",
                    )
                val world =
                    solver.optJSONObject(
                        "world_component_candidates_ev",
                    )
                val sensor =
                    solver.optJSONObject(
                        "sensor_component_candidates_ev",
                    )
                if (
                    world == null ||
                    sensor == null ||
                    !world.has(worldId) ||
                    !sensor.has(sensorId)
                ) {
                    predictableByAll = false
                    break
                }
                val w =
                    world.optDouble(
                        worldId,
                        Double.NaN,
                    )
                val q =
                    sensor.optDouble(
                        sensorId,
                        Double.NaN,
                    )
                if (
                    !w.isFinite() ||
                    !q.isFinite()
                ) {
                    predictableByAll = false
                    break
                }
                predictions[variant.id] =
                    w + q
            }
            if (!predictableByAll) continue

            val role =
                reference.optString(
                    "role",
                    "TRAIN",
                )
            if (role == "HELD_OUT") {
                heldActual += actual
                for (variant in variants) {
                    heldPredicted[variant.id]
                        ?.add(
                            predictions[variant.id]
                                ?: continue,
                        )
                }
            } else {
                trainActual += actual
                for (variant in variants) {
                    trainPredicted[variant.id]
                        ?.add(
                            predictions[variant.id]
                                ?: continue,
                        )
                }
            }
        }

        val reports = JSONArray()
        for (variant in variants) {
            val trainRmse =
                ResearchMathV01.rmse(
                    trainActual,
                    trainPredicted[variant.id]
                        ?: emptyList(),
                )
            val heldRmse =
                ResearchMathV01.rmse(
                    heldActual,
                    heldPredicted[variant.id]
                        ?: emptyList(),
                )
            reports.put(
                JSONObject()
                    .put(
                        "variant_id",
                        variant.id,
                    )
                    .put(
                        "training_rmse_ev",
                        trainRmse
                            ?: JSONObject.NULL,
                    )
                    .put(
                        "held_out_rmse_ev",
                        heldRmse
                            ?: JSONObject.NULL,
                    ),
            )
        }

        return JSONObject()
            .put(
                "training_sample_count",
                trainActual.size,
            )
            .put(
                "held_out_sample_count",
                heldActual.size,
            )
            .put(
                "identical_predictable_sample_set_used_for_all_variants",
                true,
            )
            .put(
                "variant_rmse",
                reports,
            )
            .put(
                "held_out_values_used_for_training",
                false,
            )
            .put(
                "held_out_values_used_for_variant_selection",
                false,
            )
            .put(
                "automatic_variant_winner_selected",
                false,
            )
    }

    private fun supportCellKey(
        radialBin: Int,
        sector: Int,
    ): String =
        "$radialBin:$sector"

    private fun worldCellId(
        radialBin: Int,
        worldSector: Int,
    ): String =
        "R" +
            radialBin.toString()
                .padStart(2, '0') +
            "_W" +
            worldSector.toString()
                .padStart(2, '0')

    private fun wrapPi(
        value: Double,
    ): Double {
        var x =
            value %
                (2.0 * PI)
        if (x > PI) x -= 2.0 * PI
        if (x <= -PI) x += 2.0 * PI
        return x
    }

    private fun meanOrNull(
        values: List<Double>,
    ): Any {
        val finite =
            values.filter {
                it.isFinite()
            }
        return if (finite.isEmpty()) {
            JSONObject.NULL
        } else {
            finite.average()
        }
    }

    private fun maxOrNull(
        values: List<Double>,
    ): Any {
        val finite =
            values.filter {
                it.isFinite()
            }
        return if (finite.isEmpty()) {
            JSONObject.NULL
        } else {
            finite.maxOrNull()
                ?: JSONObject.NULL
        }
    }

    private fun minOrNull(
        values: List<Double>,
    ): Any {
        val finite =
            values.filter {
                it.isFinite()
            }
        return if (finite.isEmpty()) {
            JSONObject.NULL
        } else {
            finite.minOrNull()
                ?: JSONObject.NULL
        }
    }

    private fun findRecordReport(
        reports: JSONArray,
        recordIndex: Int,
    ): JSONObject? {
        for (i in 0 until reports.length()) {
            val report =
                reports.optJSONObject(i)
                    ?: continue
            if (
                report.optInt(
                    "record_index",
                    -1,
                ) == recordIndex
            ) {
                return report
            }
        }
        return null
    }

    private fun unavailableRecord(
        recordIndex: Int,
        reason: String,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("record_index", recordIndex)
            .put("reason", reason)
            .put(
                "automatic_variant_winner_selected",
                false,
            )
            .put(
                "field_response_calibration_promoted",
                false,
            )
            .put("world_registration_promoted", false)
            .put("correction_authorized", false)
            .put("candidate_applied", false)
            .put("image_transform_applied", false)
            .put("source_sample_values_modified", false)
            .put("source_sample_positions_modified", false)
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)

    private fun unavailableSet(
        reason: String,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("automatic_variant_winner_selected", false)
            .put("field_response_calibration_promoted", false)
            .put("world_registration_promoted", false)
            .put("correction_authorized", false)
            .put("candidate_applied", false)
            .put("image_transform_applied", false)
            .put("source_sample_values_modified", false)
            .put("source_sample_positions_modified", false)
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}
