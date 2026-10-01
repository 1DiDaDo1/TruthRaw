package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.PI
import kotlin.math.abs
import kotlin.math.cos
import kotlin.math.floor
import kotlin.math.max
import kotlin.math.sin
import kotlin.math.sqrt

/**
 * Read-only decomposition audit for the coordinate bridge used by controlled
 * rotation FIELD_RESPONSE experiments.
 *
 * The explicit relation record already defines the nominal quarter-turn world
 * mapping. This audit therefore starts from that admitted nominal world cell
 * and tests the residual appearance-geometry terms separately:
 * residual rotation, uniform scale and translation.
 *
 * The purpose is to determine which appearance-derived term changes the
 * photometric decomposition on held-out data without silently treating any
 * term as a physical sensor-field transform. No variant is promoted or
 * selected automatically.
 */
object ControlledRotationFieldCoordinateBridgeAuditV01 {
    const val SCHEMA =
        "D.RAW/ControlledRotationFieldCoordinateBridgeAudit/0.1"

    private data class Bridge(
        val cornerRadiusIso: Double,
        val frontsideAspect: Double,
        val sourceFieldAspect: Double,
        val aspectRelativeDifference: Double,
    )

    private data class Geometry(
        val nominalRotationDegrees: Double,
        val residualRotationDegrees: Double,
        val uniformScale: Double,
        val translationXIso: Double,
        val translationYIso: Double,
        val bridge: Bridge,
    )

    private data class Projection(
        val radialBin: Int,
        val worldSector: Int,
        val rho: Double,
        val azimuthRadians: Double,
    )

    private data class Variant(
        val id: String,
        val useResidualRotation: Boolean,
        val useUniformScale: Boolean,
        val useTranslation: Boolean,
    )

    private data class VariantStats(
        var mappedTrain: Int = 0,
        var mappedHeld: Int = 0,
        var unmappedTrain: Int = 0,
        var unmappedHeld: Int = 0,
        var changedTrain: Int = 0,
        var changedHeld: Int = 0,
    )

    private val variants =
        listOf(
            Variant(
                id = "NOMINAL_RELATION_ONLY",
                useResidualRotation = false,
                useUniformScale = false,
                useTranslation = false,
            ),
            Variant(
                id = "RESIDUAL_ROTATION_ONLY",
                useResidualRotation = true,
                useUniformScale = false,
                useTranslation = false,
            ),
            Variant(
                id = "UNIFORM_SCALE_ONLY",
                useResidualRotation = false,
                useUniformScale = true,
                useTranslation = false,
            ),
            Variant(
                id = "TRANSLATION_ONLY",
                useResidualRotation = false,
                useUniformScale = false,
                useTranslation = true,
            ),
            Variant(
                id = "RESIDUAL_ROTATION_PLUS_SCALE",
                useResidualRotation = true,
                useUniformScale = true,
                useTranslation = false,
            ),
            Variant(
                id = "RESIDUAL_ROTATION_PLUS_TRANSLATION",
                useResidualRotation = true,
                useUniformScale = false,
                useTranslation = true,
            ),
            Variant(
                id = "FULL_RESIDUAL_SIMILARITY",
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
                "CONTROLLED_ROTATION_FIELD_COORDINATE_BRIDGE_AUDIT_AVAILABLE"
            ) {
                available++
            } else {
                unavailable++
            }
        }

        if (admitted == 0) {
            return JSONObject()
                .put("schema", SCHEMA)
                .put("status", "UNKNOWN_FAIL_CLOSED")
                .put(
                    "reason",
                    "NO_ADMITTED_CONTROLLED_ROTATION_FIELD_RECORD",
                )
                .put("record_reports", reports)
                .put("automatic_variant_winner_selected", false)
                .put("field_response_calibration_promoted", false)
                .put("correction_authorized", false)
                .put("candidate_applied", false)
                .put("creates_new_evidence", false)
                .put("scientific_writeback_allowed", false)
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (available > 0) {
                    "CONTROLLED_ROTATION_FIELD_COORDINATE_BRIDGE_AUDIT_SET_AVAILABLE"
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

        val sourceProfiles =
            profiles
                .mapNotNull { profile ->
                    profile.optString("source_sha256")
                        .trim()
                        .lowercase()
                        .takeIf { it.isNotBlank() }
                        ?.let { it to profile }
                }
                .toMap()

        val anchorSource =
            constrainedReport.optString(
                "anchor_source_sha256",
            ).trim().lowercase()
        if (anchorSource.isBlank()) {
            return unavailableRecord(
                recordIndex,
                "ANCHOR_SOURCE_MISSING",
            )
        }

        val diagnostics =
            constrainedReport.optJSONArray(
                "observation_diagnostics",
            ) ?: return unavailableRecord(
                recordIndex,
                "CONSTRAINED_OBSERVATION_DIAGNOSTICS_MISSING",
            )

        val geometryBySource =
            linkedMapOf<String, Geometry>()
        val bridgeDiagnostics = JSONArray()
        var maximumAspectDelta = 0.0

        for (i in 0 until diagnostics.length()) {
            val diagnostic =
                diagnostics.optJSONObject(i)
                    ?: continue
            val source =
                diagnostic.optString(
                    "source_sha256",
                ).trim().lowercase()
            if (source.isBlank()) continue

            val status =
                diagnostic.optString("status")
            val isAnchor =
                status ==
                    "ROTATION_CONSTRAINED_ANCHOR_IDENTITY"
            val isCandidate =
                status ==
                    "ROTATION_CONSTRAINED_SIMILARITY_CANDIDATE_AVAILABLE"
            if (!isAnchor && !isCandidate) continue
            if (
                isCandidate &&
                !diagnostic.optBoolean(
                    "diagnostic_bounds_pass",
                    false,
                )
            ) {
                continue
            }

            val profile =
                sourceProfiles[source]
                    ?: continue
            val bridge =
                coordinateBridge(profile)
                    ?: continue

            val nominalRotation =
                diagnostic.optDouble(
                    "nominal_rotation_degrees",
                    Double.NaN,
                )
            val residualRotation =
                diagnostic.optDouble(
                    "residual_rotation_degrees",
                    Double.NaN,
                )
            val scale =
                diagnostic.optDouble(
                    "uniform_scale",
                    Double.NaN,
                )
            val tx =
                diagnostic.optDouble(
                    "translation_x_isotropic",
                    Double.NaN,
                )
            val ty =
                diagnostic.optDouble(
                    "translation_y_isotropic",
                    Double.NaN,
                )
            if (
                !nominalRotation.isFinite() ||
                !residualRotation.isFinite() ||
                !scale.isFinite() ||
                scale <= 0.0 ||
                !tx.isFinite() ||
                !ty.isFinite()
            ) {
                continue
            }

            geometryBySource[source] =
                Geometry(
                    nominalRotationDegrees =
                        nominalRotation,
                    residualRotationDegrees =
                        residualRotation,
                    uniformScale = scale,
                    translationXIso = tx,
                    translationYIso = ty,
                    bridge = bridge,
                )

            maximumAspectDelta =
                max(
                    maximumAspectDelta,
                    bridge.aspectRelativeDifference,
                )
            bridgeDiagnostics.put(
                JSONObject()
                    .put("source_sha256", source)
                    .put(
                        "frontside_aspect_ratio",
                        bridge.frontsideAspect,
                    )
                    .put(
                        "source_field_aspect_ratio",
                        bridge.sourceFieldAspect,
                    )
                    .put(
                        "aspect_ratio_relative_difference",
                        bridge.aspectRelativeDifference,
                    )
                    .put(
                        "frontside_corner_radius_isotropic",
                        bridge.cornerRadiusIso,
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
                    ),
            )
        }

        val anchorGeometry =
            geometryBySource[anchorSource]
                ?: return unavailableRecord(
                    recordIndex,
                    "ANCHOR_GEOMETRY_OR_COORDINATE_BRIDGE_MISSING",
                )

        val roots =
            record.optJSONArray(
                "source_sha256_roots",
            ) ?: JSONArray()
        for (i in 0 until roots.length()) {
            val root =
                roots.optString(i)
                    .trim()
                    .lowercase()
            if (
                root.isNotBlank() &&
                root !in geometryBySource
            ) {
                return unavailableRecord(
                    recordIndex,
                    "COMPLETE_GEOMETRY_AND_BRIDGE_REQUIRED_FOR_ALL_ROOTS",
                ).put(
                    "missing_source_sha256",
                    root,
                )
            }
        }

        val sourceSamples =
            payload.optJSONArray("samples")
                ?: return unavailableRecord(
                    recordIndex,
                    "FIELD_SAMPLES_MISSING",
                )

        val variantArrays =
            linkedMapOf<String, JSONArray>()
        val variantStats =
            linkedMapOf<String, VariantStats>()
        for (variant in variants) {
            variantArrays[variant.id] = JSONArray()
            variantStats[variant.id] = VariantStats()
        }

        var commonMappedTrain = 0
        var commonMappedHeld = 0
        var excludedByAnyVariantTrain = 0
        var excludedByAnyVariantHeld = 0

        for (i in 0 until sourceSamples.length()) {
            val sample =
                sourceSamples.optJSONObject(i)
                    ?: continue
            val role =
                sample.optString(
                    "role",
                    "TRAIN",
                )
            val source =
                sample.optString(
                    "source_sha256",
                ).trim().lowercase()
            val radialBin =
                sample.optInt(
                    "radial_bin",
                    -1,
                )
            val nominalWorldSector =
                sample.optInt(
                    "world_sector",
                    -1,
                )
            val geometry =
                geometryBySource[source]

            if (
                geometry == null ||
                radialBin !in 0 until radialBins ||
                nominalWorldSector !in 0 until azimuthBins
            ) {
                if (role == "HELD_OUT") {
                    excludedByAnyVariantHeld++
                } else {
                    excludedByAnyVariantTrain++
                }
                continue
            }

            val projections =
                linkedMapOf<String, Projection?>()
            var allMapped = true

            for (variant in variants) {
                val projection =
                    if (
                        variant.id ==
                        "NOMINAL_RELATION_ONLY"
                    ) {
                        Projection(
                            radialBin = radialBin,
                            worldSector =
                                nominalWorldSector,
                            rho =
                                (
                                    radialBin.toDouble() +
                                        0.5
                                    ) /
                                    radialBins.toDouble(),
                            azimuthRadians =
                                (
                                    nominalWorldSector.toDouble() +
                                        0.5
                                    ) *
                                    (
                                        2.0 *
                                            PI /
                                            azimuthBins.toDouble()
                                        ),
                        )
                    } else {
                        projectResidualFromNominalWorldCell(
                            radialBin = radialBin,
                            nominalWorldSector =
                                nominalWorldSector,
                            radialBins = radialBins,
                            azimuthBins = azimuthBins,
                            geometry = geometry,
                            anchorGeometry =
                                anchorGeometry,
                            variant = variant,
                        )
                    }
                projections[variant.id] =
                    projection
                val stats =
                    variantStats[variant.id]
                        ?: continue
                if (projection == null) {
                    allMapped = false
                    if (role == "HELD_OUT") {
                        stats.unmappedHeld++
                    } else {
                        stats.unmappedTrain++
                    }
                } else {
                    if (role == "HELD_OUT") {
                        stats.mappedHeld++
                        if (
                            projection.radialBin !=
                                radialBin ||
                            projection.worldSector !=
                                nominalWorldSector
                        ) {
                            stats.changedHeld++
                        }
                    } else {
                        stats.mappedTrain++
                        if (
                            projection.radialBin !=
                                radialBin ||
                            projection.worldSector !=
                                nominalWorldSector
                        ) {
                            stats.changedTrain++
                        }
                    }
                }
            }

            if (!allMapped) {
                if (role == "HELD_OUT") {
                    excludedByAnyVariantHeld++
                } else {
                    excludedByAnyVariantTrain++
                }
                continue
            }

            if (role == "HELD_OUT") {
                commonMappedHeld++
            } else {
                commonMappedTrain++
            }

            for (variant in variants) {
                val projection =
                    projections[variant.id]
                        ?: continue
                val derived =
                    JSONObject(sample.toString())
                        .put(
                            "world_cell_id",
                            worldCellId(
                                projection.radialBin,
                                projection.worldSector,
                            ),
                        )
                        .put(
                            "coordinate_bridge_variant",
                            variant.id,
                        )
                        .put(
                            "coordinate_bridge_projected_world_radial_bin",
                            projection.radialBin,
                        )
                        .put(
                            "coordinate_bridge_projected_world_sector",
                            projection.worldSector,
                        )
                variantArrays[variant.id]
                    ?.put(derived)
            }
        }

        if (
            commonMappedTrain < 6 ||
            commonMappedHeld < 1
        ) {
            return unavailableRecord(
                recordIndex,
                "INSUFFICIENT_COMMON_VARIANT_SAMPLE_SUPPORT",
            )
                .put(
                    "common_mapped_training_sample_count",
                    commonMappedTrain,
                )
                .put(
                    "common_mapped_held_out_sample_count",
                    commonMappedHeld,
                )
        }

        val solverByVariant =
            linkedMapOf<String, JSONObject>()
        for (variant in variants) {
            val result =
                FieldResponseRotationSeparationCandidateV01
                    .evaluateDerivedDiagnosticSamples(
                        samples =
                            variantArrays[variant.id]
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
                    "COORDINATE_BRIDGE_VARIANT_SOLVER_UNAVAILABLE",
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

        val exactCommonComparison =
            exactCommonPredictableComparison(
                variantArrays =
                    variantArrays,
                solverByVariant =
                    solverByVariant,
            )

        val variantReports = JSONArray()
        for (variant in variants) {
            val stats =
                variantStats[variant.id]
                    ?: VariantStats()
            val solver =
                solverByVariant[variant.id]
                    ?: JSONObject()
            variantReports.put(
                JSONObject()
                    .put("variant_id", variant.id)
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
                        "mapped_training_sample_count_before_common_intersection",
                        stats.mappedTrain,
                    )
                    .put(
                        "mapped_held_out_sample_count_before_common_intersection",
                        stats.mappedHeld,
                    )
                    .put(
                        "unmapped_training_sample_count",
                        stats.unmappedTrain,
                    )
                    .put(
                        "unmapped_held_out_sample_count",
                        stats.unmappedHeld,
                    )
                    .put(
                        "training_world_cell_changed_count",
                        stats.changedTrain,
                    )
                    .put(
                        "held_out_world_cell_changed_count",
                        stats.changedHeld,
                    )
                    .put(
                        "solver_training_sample_count",
                        solver.optInt(
                            "training_sample_count",
                            0,
                        ),
                    )
                    .put(
                        "solver_held_out_sample_count",
                        solver.optInt(
                            "held_out_sample_count",
                            0,
                        ),
                    )
                    .put(
                        "solver_training_rmse_ev",
                        solver.opt(
                            "training_rmse_ev",
                        ) ?: JSONObject.NULL,
                    )
                    .put(
                        "solver_held_out_rmse_ev",
                        solver.opt(
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

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "CONTROLLED_ROTATION_FIELD_COORDINATE_BRIDGE_AUDIT_AVAILABLE",
            )
            .put("record_index", recordIndex)
            .put(
                "anchor_source_sha256",
                anchorSource,
            )
            .put(
                "nominal_world_mapping_source",
                "EXPLICIT_ADMITTED_ROTATION_RELATION_RECORD",
            )
            .put(
                "residual_geometry_source",
                "CONTROLLED_ROTATION_CONSTRAINED_GEOMETRY_AUDIT",
            )
            .put(
                "bridge_method",
                "START_FROM_NOMINAL_WORLD_BIN_CENTRE_THEN_APPLY_SELECTED_RESIDUAL_COMPONENTS",
            )
            .put(
                "bridge_diagnostics",
                bridgeDiagnostics,
            )
            .put(
                "maximum_aspect_ratio_relative_difference",
                maximumAspectDelta,
            )
            .put(
                "common_source_sample_support",
                JSONObject()
                    .put(
                        "training_sample_count",
                        commonMappedTrain,
                    )
                    .put(
                        "held_out_sample_count",
                        commonMappedHeld,
                    )
                    .put(
                        "excluded_by_any_variant_training_sample_count",
                        excludedByAnyVariantTrain,
                    )
                    .put(
                        "excluded_by_any_variant_held_out_sample_count",
                        excludedByAnyVariantHeld,
                    )
                    .put(
                        "same_source_sample_set_used_for_all_variant_solvers",
                        true,
                    ),
            )
            .put("variant_reports", variantReports)
            .put(
                "exact_common_predictable_comparison",
                exactCommonComparison,
            )
            .put(
                "interpretation_contract",
                JSONObject()
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
                        "field_bin_center_is_exact_measured_sample_position",
                        false,
                    )
                    .put(
                        "frontside_to_source_field_coordinate_bridge_proven",
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

    private fun projectResidualFromNominalWorldCell(
        radialBin: Int,
        nominalWorldSector: Int,
        radialBins: Int,
        azimuthBins: Int,
        geometry: Geometry,
        anchorGeometry: Geometry,
        variant: Variant,
    ): Projection? {
        val sourceCorner =
            geometry.bridge.cornerRadiusIso
        val anchorCorner =
            anchorGeometry.bridge.cornerRadiusIso
        if (
            !sourceCorner.isFinite() ||
            sourceCorner <= 0.0 ||
            !anchorCorner.isFinite() ||
            anchorCorner <= 0.0
        ) {
            return null
        }

        val rho =
            (
                radialBin.toDouble() +
                    0.5
                ) /
                radialBins.toDouble()
        val theta =
            (
                nominalWorldSector.toDouble() +
                    0.5
                ) *
                (
                    2.0 *
                        PI /
                        azimuthBins.toDouble()
                    )

        var x =
            rho *
                sourceCorner *
                cos(theta)
        var y =
            rho *
                sourceCorner *
                sin(theta)

        if (variant.useTranslation) {
            val nominalBackAngle =
                Math.toRadians(
                    -geometry.nominalRotationDegrees,
                )
            val nc = cos(nominalBackAngle)
            val ns = sin(nominalBackAngle)
            val txNominal =
                nc * geometry.translationXIso -
                    ns * geometry.translationYIso
            val tyNominal =
                ns * geometry.translationXIso +
                    nc * geometry.translationYIso
            x -= txNominal
            y -= tyNominal
        }

        if (variant.useResidualRotation) {
            val residualBackAngle =
                Math.toRadians(
                    -geometry.residualRotationDegrees,
                )
            val rc = cos(residualBackAngle)
            val rs = sin(residualBackAngle)
            val rx = rc * x - rs * y
            val ry = rs * x + rc * y
            x = rx
            y = ry
        }

        if (variant.useUniformScale) {
            if (
                !geometry.uniformScale.isFinite() ||
                geometry.uniformScale <= 0.0
            ) {
                return null
            }
            x /= geometry.uniformScale
            y /= geometry.uniformScale
        }

        val worldX = x / anchorCorner
        val worldY = y / anchorCorner
        val worldRho =
            sqrt(
                worldX * worldX +
                    worldY * worldY,
            )
        if (
            !worldRho.isFinite() ||
            worldRho < 0.0 ||
            worldRho > 1.0
        ) {
            return null
        }

        var worldAngle =
            kotlin.math.atan2(
                worldY,
                worldX,
            )
        if (worldAngle < 0.0) {
            worldAngle += 2.0 * PI
        }

        val worldRadialBin =
            minOf(
                radialBins - 1,
                floor(
                    worldRho *
                        radialBins.toDouble(),
                ).toInt(),
            )
        val worldSector =
            minOf(
                azimuthBins - 1,
                floor(
                    worldAngle /
                        (2.0 * PI) *
                        azimuthBins.toDouble(),
                ).toInt(),
            )

        return Projection(
            radialBin = worldRadialBin,
            worldSector = worldSector,
            rho = worldRho,
            azimuthRadians = worldAngle,
        )
    }

    private fun exactCommonPredictableComparison(
        variantArrays:
            Map<String, JSONArray>,
        solverByVariant:
            Map<String, JSONObject>,
    ): JSONObject {
        val first =
            variantArrays[variants.first().id]
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
                    variantArrays[variant.id]
                        ?.optJSONObject(i)
                val solver =
                    solverByVariant[variant.id]
                if (sample == null || solver == null) {
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

    private fun coordinateBridge(
        profile: JSONObject,
    ): Bridge? {
        val scene =
            profile.optJSONObject(
                "scene_analysis",
            ) ?: return null
        val featureGeometry =
            scene.optJSONObject(
                "deterministic_local_feature_geometry_v0_1",
            ) ?: return null
        if (
            featureGeometry.optString("status") !=
            "DETERMINISTIC_LOCAL_FEATURES_AVAILABLE"
        ) {
            return null
        }

        val analysisWidth =
            featureGeometry.optInt(
                "analysis_width",
                0,
            )
        val analysisHeight =
            featureGeometry.optInt(
                "analysis_height",
                0,
            )
        if (
            analysisWidth < 2 ||
            analysisHeight < 2
        ) {
            return null
        }

        val analysisMax =
            max(
                analysisWidth - 1,
                analysisHeight - 1,
            ).toDouble()
        val halfW =
            (analysisWidth - 1) * 0.5
        val halfH =
            (analysisHeight - 1) * 0.5
        val cornerRadiusIso =
            sqrt(
                halfW * halfW +
                    halfH * halfH,
            ) / analysisMax

        val field =
            profile.optJSONObject(
                "observation_optical_field_chart",
            ) ?: return null
        val activeArea =
            field.optJSONArray(
                "active_area",
            ) ?: return null
        if (activeArea.length() < 4) {
            return null
        }

        val top = activeArea.optInt(0, -1)
        val left = activeArea.optInt(1, -1)
        val bottom = activeArea.optInt(2, -1)
        val right = activeArea.optInt(3, -1)
        if (
            top < 0 ||
            left < 0 ||
            bottom <= top ||
            right <= left
        ) {
            return null
        }

        val frontAspect =
            analysisWidth.toDouble() /
                analysisHeight.toDouble()
        val fieldAspect =
            (right - left).toDouble() /
                (bottom - top).toDouble()
        val aspectDelta =
            abs(
                frontAspect -
                    fieldAspect,
            ) /
                max(
                    abs(fieldAspect),
                    1.0e-12,
                )

        return Bridge(
            cornerRadiusIso =
                cornerRadiusIso,
            frontsideAspect =
                frontAspect,
            sourceFieldAspect =
                fieldAspect,
            aspectRelativeDifference =
                aspectDelta,
        )
    }

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
}
