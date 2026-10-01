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
 * Read-only geometry-adjusted FIELD_RESPONSE mapping dry-run.
 *
 * The measured field values are never changed. For an admitted controlled
 * rotation record, each aggregate sensor-field cell is represented by its
 * deterministic polar bin centre. The rotation-constrained appearance
 * similarity maps that representative point back into the 0-degree anchor
 * frame. The projected point is then re-binned to a candidate world cell.
 *
 * This is deliberately only a dry-run because the bridge between the
 * frontside appearance-analysis raster and the measured source-field chart is
 * not yet independently calibrated. The transform is therefore not world
 * registration proof and can never authorize correction or writeback.
 */
object GeometryAdjustedFieldMappingDryRunV01 {
    const val SCHEMA =
        "D.RAW/GeometryAdjustedFieldMappingDryRun/0.1"

    private data class Geometry(
        val totalRotationDegrees: Double,
        val scale: Double,
        val txIso: Double,
        val tyIso: Double,
        val cornerRadiusIso: Double,
        val supportCount: Int,
        val status: String,
    )

    private data class Projection(
        val worldRadialBin: Int,
        val worldSector: Int,
        val rho: Double,
        val azimuthRadians: Double,
    )

    fun evaluate(
        profiles: List<JSONObject>,
        records: List<JSONObject>,
        constrainedAudit: JSONObject,
    ): JSONObject {
        val reports = JSONArray()
        var admittedCount = 0
        var availableCount = 0
        var unavailableCount = 0

        val auditReports =
            constrainedAudit.optJSONArray("record_reports")
                ?: JSONArray()

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

            admittedCount++
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
                "GEOMETRY_ADJUSTED_FIELD_MAPPING_DRY_RUN_AVAILABLE"
            ) {
                availableCount++
            } else {
                unavailableCount++
            }
        }

        if (admittedCount == 0) {
            return unavailableSet(
                "NO_ADMITTED_CONTROLLED_ROTATION_FIELD_RECORD",
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (availableCount > 0) {
                    "GEOMETRY_ADJUSTED_FIELD_MAPPING_DRY_RUN_SET_AVAILABLE"
                } else {
                    "UNKNOWN_FAIL_CLOSED"
                },
            )
            .put("admitted_record_count", admittedCount)
            .put("available_record_count", availableCount)
            .put(
                "unavailable_record_count",
                unavailableCount,
            )
            .put("record_reports", reports)
            .put(
                "authority_boundary",
                authorityBoundary(),
            )
            .put("candidate_applied", false)
            .put("image_transform_applied", false)
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
        if (
            radialBins < 2 ||
            azimuthBins < 2
        ) {
            return unavailableRecord(
                recordIndex,
                "FIELD_BIN_GEOMETRY_INVALID",
            )
        }

        val sourceProfiles =
            profiles
                .mapNotNull { profile ->
                    val sha =
                        profile.optString(
                            "source_sha256",
                        ).trim().lowercase()
                    if (sha.isBlank()) {
                        null
                    } else {
                        sha to profile
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

        val anchorSource =
            constrainedReport.optString(
                "anchor_source_sha256",
            ).trim().lowercase()
        val geometryBySource =
            linkedMapOf<String, Geometry>()
        val geometrySummary = JSONArray()

        for (i in 0 until diagnostics.length()) {
            val d =
                diagnostics.optJSONObject(i)
                    ?: continue
            val source =
                d.optString(
                    "source_sha256",
                ).trim().lowercase()
            if (source.isBlank()) continue

            val status = d.optString("status")
            val candidateAvailable =
                status ==
                "ROTATION_CONSTRAINED_SIMILARITY_CANDIDATE_AVAILABLE"
            val anchorIdentity =
                status ==
                "ROTATION_CONSTRAINED_ANCHOR_IDENTITY"
            if (
                !anchorIdentity &&
                !candidateAvailable
            ) {
                continue
            }
            if (
                candidateAvailable &&
                !d.optBoolean(
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

            val rotation =
                d.optDouble(
                    "fitted_total_rotation_degrees",
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
                !rotation.isFinite() ||
                !scale.isFinite() ||
                scale <= 0.0 ||
                !tx.isFinite() ||
                !ty.isFinite()
            ) {
                continue
            }

            val supportCount =
                when {
                    d.has("right_feature_support_count") ->
                        d.optInt(
                            "right_feature_support_count",
                            0,
                        )
                    d.has("feature_support_count") ->
                        d.optInt(
                            "feature_support_count",
                            0,
                        )
                    else ->
                        d.optInt(
                            "robust_match_count",
                            0,
                        )
                }

            geometryBySource[source] =
                Geometry(
                    totalRotationDegrees = rotation,
                    scale = scale,
                    txIso = tx,
                    tyIso = ty,
                    cornerRadiusIso =
                        bridge.optDouble(
                            "frontside_corner_radius_isotropic",
                            Double.NaN,
                        ),
                    supportCount = supportCount,
                    status = status,
                )
            geometrySummary.put(
                JSONObject()
                    .put("source_sha256", source)
                    .put("geometry_status", status)
                    .put(
                        "total_rotation_degrees",
                        rotation,
                    )
                    .put("uniform_scale", scale)
                    .put("translation_x_isotropic", tx)
                    .put("translation_y_isotropic", ty)
                    .put(
                        "frontside_corner_radius_isotropic",
                        bridge.optDouble(
                            "frontside_corner_radius_isotropic",
                        ),
                    )
                    .put(
                        "frontside_aspect_ratio",
                        bridge.optDouble(
                            "frontside_aspect_ratio",
                        ),
                    )
                    .put(
                        "source_field_aspect_ratio",
                        bridge.optDouble(
                            "source_field_aspect_ratio",
                        ),
                    )
                    .put(
                        "aspect_ratio_relative_difference",
                        bridge.optDouble(
                            "aspect_ratio_relative_difference",
                        ),
                    )
                    .put(
                        "feature_support_count",
                        supportCount,
                    )
                    .put(
                        "coordinate_bridge_proven",
                        false,
                    ),
            )
        }

        if (
            anchorSource.isBlank() ||
            anchorSource !in geometryBySource
        ) {
            return unavailableRecord(
                recordIndex,
                "ANCHOR_GEOMETRY_OR_COORDINATE_BRIDGE_MISSING",
            )
        }

        val rootArray =
            record.optJSONArray(
                "source_sha256_roots",
            ) ?: JSONArray()
        for (i in 0 until rootArray.length()) {
            val root =
                rootArray.optString(i)
                    .trim().lowercase()
            if (
                root.isNotBlank() &&
                root !in geometryBySource
            ) {
                return unavailableRecord(
                    recordIndex,
                    "COMPLETE_CONSTRAINED_GEOMETRY_REQUIRED_FOR_ALL_RECORD_ROOTS",
                )
                    .put(
                        "missing_source_sha256",
                        root,
                    )
            }
        }

        val anchorGeometry =
            geometryBySource[anchorSource]
                ?: return unavailableRecord(
                    recordIndex,
                    "ANCHOR_GEOMETRY_MISSING",
                )
        val samples =
            payload.optJSONArray("samples")
                ?: return unavailableRecord(
                    recordIndex,
                    "FIELD_SAMPLES_MISSING",
                )

        val nominalComparable =
            JSONObject(record.toString())
        val adjusted =
            JSONObject(record.toString())
        val nominalSamples = JSONArray()
        val adjustedSamples = JSONArray()
        val mappingStats =
            linkedMapOf<String, IntArray>()

        var mappedTrain = 0
        var mappedHeld = 0
        var changedTrain = 0
        var changedHeld = 0
        var unmappedTrain = 0
        var unmappedHeld = 0

        for (i in 0 until samples.length()) {
            val sample =
                samples.optJSONObject(i)
                    ?: continue
            val source =
                sample.optString(
                    "source_sha256",
                ).trim().lowercase()
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
            if (
                radialBin !in 0 until radialBins ||
                sensorSector !in 0 until azimuthBins
            ) {
                if (role == "HELD_OUT") {
                    unmappedHeld++
                } else {
                    unmappedTrain++
                }
                continue
            }

            val geometry =
                geometryBySource[source]
            if (geometry == null) {
                if (role == "HELD_OUT") {
                    unmappedHeld++
                } else {
                    unmappedTrain++
                }
                continue
            }

            val projection =
                projectCellCentreToAnchor(
                    radialBin = radialBin,
                    sensorSector = sensorSector,
                    radialBins = radialBins,
                    azimuthBins = azimuthBins,
                    sourceGeometry = geometry,
                    anchorGeometry = anchorGeometry,
                )
            if (projection == null) {
                if (role == "HELD_OUT") {
                    unmappedHeld++
                } else {
                    unmappedTrain++
                }
                continue
            }

            val nominal =
                JSONObject(sample.toString())
            val adjustedSample =
                JSONObject(sample.toString())
            val adjustedWorldId =
                worldCellId(
                    projection.worldRadialBin,
                    projection.worldSector,
                )
            adjustedSample
                .put(
                    "world_cell_id",
                    adjustedWorldId,
                )
                .put(
                    "geometry_adjusted_world_radial_bin",
                    projection.worldRadialBin,
                )
                .put(
                    "geometry_adjusted_world_sector",
                    projection.worldSector,
                )
                .put(
                    "geometry_adjusted_world_rho_bin_center_projection",
                    projection.rho,
                )
                .put(
                    "geometry_adjusted_world_azimuth_radians_bin_center_projection",
                    projection.azimuthRadians,
                )

            nominalSamples.put(nominal)
            adjustedSamples.put(adjustedSample)

            val nominalWorld =
                sample.optString(
                    "world_cell_id",
                )
            val changed =
                nominalWorld != adjustedWorldId
            if (role == "HELD_OUT") {
                mappedHeld++
                if (changed) changedHeld++
            } else {
                mappedTrain++
                if (changed) changedTrain++
            }

            val stats =
                mappingStats.getOrPut(source) {
                    IntArray(4)
                }
            if (role == "HELD_OUT") {
                stats[2]++
                if (changed) stats[3]++
            } else {
                stats[0]++
                if (changed) stats[1]++
            }
        }

        nominalComparable
            .getJSONObject("axis_payload")
            .put("samples", nominalSamples)
        adjusted
            .getJSONObject("axis_payload")
            .put("samples", adjustedSamples)

        val nominalComparableResult =
            FieldResponseRotationSeparationCandidateV01
                .evaluate(
                    listOf(nominalComparable),
                )
        val adjustedResult =
            FieldResponseRotationSeparationCandidateV01
                .evaluate(
                    listOf(adjusted),
                )

        if (
            nominalComparableResult.optString("status") !=
            "WORLD_SENSOR_FIELD_SEPARATION_CANDIDATE_AVAILABLE" ||
            adjustedResult.optString("status") !=
            "WORLD_SENSOR_FIELD_SEPARATION_CANDIDATE_AVAILABLE"
        ) {
            return unavailableRecord(
                recordIndex,
                "COMPARABLE_FIELD_SOLVER_DRY_RUN_UNAVAILABLE",
            )
                .put(
                    "nominal_comparable_status",
                    nominalComparableResult.optString(
                        "status",
                    ),
                )
                .put(
                    "adjusted_status",
                    adjustedResult.optString(
                        "status",
                    ),
                )
        }

        val sameSampleComparison =
            sameSampleComparison(
                nominalSamples = nominalSamples,
                adjustedSamples = adjustedSamples,
                nominalResult = nominalComparableResult,
                adjustedResult = adjustedResult,
            )

        val perObservation = JSONArray()
        for ((source, stats) in mappingStats) {
            perObservation.put(
                JSONObject()
                    .put(
                        "source_sha256",
                        source,
                    )
                    .put(
                        "train_mapped_sample_count",
                        stats[0],
                    )
                    .put(
                        "train_world_cell_changed_count",
                        stats[1],
                    )
                    .put(
                        "held_out_mapped_sample_count",
                        stats[2],
                    )
                    .put(
                        "held_out_world_cell_changed_count",
                        stats[3],
                    ),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "GEOMETRY_ADJUSTED_FIELD_MAPPING_DRY_RUN_AVAILABLE",
            )
            .put("record_index", recordIndex)
            .put(
                "anchor_source_sha256",
                anchorSource,
            )
            .put(
                "projection_model",
                "FIELD_BIN_CENTRE_INVERSE_ROTATION_CONSTRAINED_SIMILARITY_TO_ANCHOR",
            )
            .put(
                "field_bin_center_projection_only",
                true,
            )
            .put(
                "exact_measured_sample_positions_used",
                false,
            )
            .put(
                "frontside_to_source_field_coordinate_bridge_proven",
                false,
            )
            .put(
                "geometry_by_observation",
                geometrySummary,
            )
            .put(
                "mapping_comparison",
                JSONObject()
                    .put(
                        "mapped_training_sample_count",
                        mappedTrain,
                    )
                    .put(
                        "mapped_held_out_sample_count",
                        mappedHeld,
                    )
                    .put(
                        "unmapped_training_sample_count",
                        unmappedTrain,
                    )
                    .put(
                        "unmapped_held_out_sample_count",
                        unmappedHeld,
                    )
                    .put(
                        "training_world_cell_changed_count",
                        changedTrain,
                    )
                    .put(
                        "held_out_world_cell_changed_count",
                        changedHeld,
                    )
                    .put(
                        "per_observation",
                        perObservation,
                    ),
            )
            .put(
                "nominal_comparable_solver",
                compactSolverResult(
                    nominalComparableResult,
                ),
            )
            .put(
                "geometry_adjusted_solver",
                compactSolverResult(
                    adjustedResult,
                ),
            )
            .put(
                "same_sample_rmse_comparison",
                sameSampleComparison,
            )
            .put(
                "comparison_interpretation",
                JSONObject()
                    .put(
                        "lower_rmse_is_reported_descriptively_only",
                        true,
                    )
                    .put(
                        "automatic_geometry_winner_selected",
                        false,
                    )
                    .put(
                        "held_out_photometric_values_used_to_fit_world_or_sensor_components",
                        false,
                    )
                    .put(
                        "held_out_photometric_values_used_to_select_geometry",
                        false,
                    )
                    .put(
                        "geometry_adjusted_result_is_calibration_proof",
                        false,
                    )
                    .put(
                        "geometry_adjusted_result_can_authorize_correction",
                        false,
                    ),
            )
            .put(
                "authority_boundary",
                authorityBoundary(),
            )
            .put("candidate_applied", false)
            .put("image_transform_applied", false)
            .put("source_sample_values_modified", false)
            .put("source_sample_positions_modified", false)
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun projectCellCentreToAnchor(
        radialBin: Int,
        sensorSector: Int,
        radialBins: Int,
        azimuthBins: Int,
        sourceGeometry: Geometry,
        anchorGeometry: Geometry,
    ): Projection? {
        if (
            !sourceGeometry.cornerRadiusIso.isFinite() ||
            sourceGeometry.cornerRadiusIso <= 0.0 ||
            !anchorGeometry.cornerRadiusIso.isFinite() ||
            anchorGeometry.cornerRadiusIso <= 0.0
        ) {
            return null
        }

        val rho =
            (radialBin.toDouble() + 0.5) /
                radialBins.toDouble()
        val theta =
            (sensorSector.toDouble() + 0.5) *
                (2.0 * PI / azimuthBins.toDouble())

        val qx =
            rho *
                sourceGeometry.cornerRadiusIso *
                cos(theta)
        val qy =
            rho *
                sourceGeometry.cornerRadiusIso *
                sin(theta)

        val dx = qx - sourceGeometry.txIso
        val dy = qy - sourceGeometry.tyIso
        val angle =
            Math.toRadians(
                -sourceGeometry.totalRotationDegrees,
            )
        val c = cos(angle)
        val s = sin(angle)
        val px =
            (c * dx - s * dy) /
                sourceGeometry.scale
        val py =
            (s * dx + c * dy) /
                sourceGeometry.scale

        val worldX =
            px /
                anchorGeometry.cornerRadiusIso
        val worldY =
            py /
                anchorGeometry.cornerRadiusIso
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
            worldRadialBin = worldRadialBin,
            worldSector = worldSector,
            rho = worldRho,
            azimuthRadians = worldAngle,
        )
    }

    private fun coordinateBridge(
        profile: JSONObject,
    ): JSONObject? {
        val scene =
            profile.optJSONObject(
                "scene_analysis",
            ) ?: return null
        val geometry =
            scene.optJSONObject(
                "deterministic_local_feature_geometry_v0_1",
            ) ?: return null
        if (
            geometry.optString("status") !=
            "DETERMINISTIC_LOCAL_FEATURES_AVAILABLE"
        ) {
            return null
        }

        val analysisWidth =
            geometry.optInt(
                "analysis_width",
                0,
            )
        val analysisHeight =
            geometry.optInt(
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
            abs(frontAspect - fieldAspect) /
                max(
                    abs(fieldAspect),
                    1.0e-12,
                )

        return JSONObject()
            .put(
                "frontside_corner_radius_isotropic",
                cornerRadiusIso,
            )
            .put(
                "frontside_aspect_ratio",
                frontAspect,
            )
            .put(
                "source_field_aspect_ratio",
                fieldAspect,
            )
            .put(
                "aspect_ratio_relative_difference",
                aspectDelta,
            )
            .put(
                "coordinate_bridge_proven",
                false,
            )
    }

    private fun sameSampleComparison(
        nominalSamples: JSONArray,
        adjustedSamples: JSONArray,
        nominalResult: JSONObject,
        adjustedResult: JSONObject,
    ): JSONObject {
        val nominalWorld =
            nominalResult.optJSONObject(
                "world_component_candidates_ev",
            ) ?: JSONObject()
        val nominalSensor =
            nominalResult.optJSONObject(
                "sensor_component_candidates_ev",
            ) ?: JSONObject()
        val adjustedWorld =
            adjustedResult.optJSONObject(
                "world_component_candidates_ev",
            ) ?: JSONObject()
        val adjustedSensor =
            adjustedResult.optJSONObject(
                "sensor_component_candidates_ev",
            ) ?: JSONObject()

        val trainActual = ArrayList<Double>()
        val trainNominal = ArrayList<Double>()
        val trainAdjusted = ArrayList<Double>()
        val heldActual = ArrayList<Double>()
        val heldNominal = ArrayList<Double>()
        val heldAdjusted = ArrayList<Double>()

        val n =
            minOf(
                nominalSamples.length(),
                adjustedSamples.length(),
            )
        for (i in 0 until n) {
            val nominal =
                nominalSamples.optJSONObject(i)
                    ?: continue
            val adjusted =
                adjustedSamples.optJSONObject(i)
                    ?: continue
            val value =
                nominal.optDouble(
                    "relative_signal_ev",
                    Double.NaN,
                )
            if (!value.isFinite()) continue

            val sensorId =
                nominal.optString(
                    "sensor_cell_id",
                )
            val nominalWorldId =
                nominal.optString(
                    "world_cell_id",
                )
            val adjustedWorldId =
                adjusted.optString(
                    "world_cell_id",
                )
            if (
                sensorId.isBlank() ||
                nominalWorldId.isBlank() ||
                adjustedWorldId.isBlank()
            ) {
                continue
            }

            val nw =
                if (nominalWorld.has(nominalWorldId)) {
                    nominalWorld.optDouble(
                        nominalWorldId,
                        Double.NaN,
                    )
                } else {
                    Double.NaN
                }
            val ns =
                if (nominalSensor.has(sensorId)) {
                    nominalSensor.optDouble(
                        sensorId,
                        Double.NaN,
                    )
                } else {
                    Double.NaN
                }
            val aw =
                if (adjustedWorld.has(adjustedWorldId)) {
                    adjustedWorld.optDouble(
                        adjustedWorldId,
                        Double.NaN,
                    )
                } else {
                    Double.NaN
                }
            val asv =
                if (adjustedSensor.has(sensorId)) {
                    adjustedSensor.optDouble(
                        sensorId,
                        Double.NaN,
                    )
                } else {
                    Double.NaN
                }
            if (
                !nw.isFinite() ||
                !ns.isFinite() ||
                !aw.isFinite() ||
                !asv.isFinite()
            ) {
                continue
            }

            val role =
                nominal.optString(
                    "role",
                    "TRAIN",
                )
            if (role == "HELD_OUT") {
                heldActual += value
                heldNominal += nw + ns
                heldAdjusted += aw + asv
            } else {
                trainActual += value
                trainNominal += nw + ns
                trainAdjusted += aw + asv
            }
        }

        val nominalTrain =
            ResearchMathV01.rmse(
                trainActual,
                trainNominal,
            )
        val adjustedTrain =
            ResearchMathV01.rmse(
                trainActual,
                trainAdjusted,
            )
        val nominalHeld =
            ResearchMathV01.rmse(
                heldActual,
                heldNominal,
            )
        val adjustedHeld =
            ResearchMathV01.rmse(
                heldActual,
                heldAdjusted,
            )

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
                "nominal_training_rmse_ev",
                nominalTrain ?: JSONObject.NULL,
            )
            .put(
                "geometry_adjusted_training_rmse_ev",
                adjustedTrain ?: JSONObject.NULL,
            )
            .put(
                "training_delta_adjusted_minus_nominal_ev",
                finiteDifference(
                    adjustedTrain,
                    nominalTrain,
                ),
            )
            .put(
                "nominal_held_out_rmse_ev",
                nominalHeld ?: JSONObject.NULL,
            )
            .put(
                "geometry_adjusted_held_out_rmse_ev",
                adjustedHeld ?: JSONObject.NULL,
            )
            .put(
                "held_out_delta_adjusted_minus_nominal_ev",
                finiteDifference(
                    adjustedHeld,
                    nominalHeld,
                ),
            )
            .put(
                "identical_sample_set_used_for_nominal_and_adjusted_rmse",
                true,
            )
            .put(
                "held_out_values_used_for_training",
                false,
            )
            .put(
                "held_out_values_used_for_geometry_selection",
                false,
            )
            .put(
                "automatic_winner_selected",
                false,
            )
    }

    private fun compactSolverResult(
        result: JSONObject,
    ): JSONObject =
        JSONObject()
            .put(
                "status",
                result.optString("status"),
            )
            .put(
                "training_sample_count",
                result.optInt(
                    "training_sample_count",
                    0,
                ),
            )
            .put(
                "held_out_sample_count",
                result.optInt(
                    "held_out_sample_count",
                    0,
                ),
            )
            .put(
                "training_rmse_ev",
                result.opt(
                    "training_rmse_ev",
                ) ?: JSONObject.NULL,
            )
            .put(
                "held_out_rmse_ev",
                result.opt(
                    "held_out_rmse_ev",
                ) ?: JSONObject.NULL,
            )
            .put("candidate_applied", false)
            .put(
                "scientific_writeback_allowed",
                false,
            )

    private fun finiteDifference(
        a: Double?,
        b: Double?,
    ): Any =
        if (
            a != null &&
            b != null &&
            a.isFinite() &&
            b.isFinite()
        ) {
            a - b
        } else {
            JSONObject.NULL
        }

    private fun worldCellId(
        radialBin: Int,
        worldSector: Int,
    ): String =
        "R" +
            radialBin.toString().padStart(2, '0') +
            "_W" +
            worldSector.toString().padStart(2, '0')

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

    private fun authorityBoundary(): JSONObject =
        JSONObject()
            .put(
                "frontside_appearance_geometry_is_world_registration_proof",
                false,
            )
            .put(
                "frontside_to_source_field_coordinate_bridge_proven",
                false,
            )
            .put(
                "field_bin_center_is_exact_measured_sample_position",
                false,
            )
            .put(
                "registration_adjusted_field_solver_executed_for_dry_run",
                true,
            )
            .put(
                "registration_adjusted_field_solver_promoted",
                false,
            )
            .put(
                "geometry_adjusted_result_is_field_calibration",
                false,
            )
            .put(
                "world_registration_promoted",
                false,
            )
            .put(
                "field_response_calibration_promoted",
                false,
            )
            .put(
                "correction_authorized",
                false,
            )

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
                "registration_adjusted_field_solver_executed_for_dry_run",
                false,
            )
            .put(
                "world_registration_promoted",
                false,
            )
            .put(
                "field_response_calibration_promoted",
                false,
            )
            .put(
                "correction_authorized",
                false,
            )
            .put("candidate_applied", false)
            .put("image_transform_applied", false)
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
            .put(
                "registration_adjusted_field_solver_executed_for_dry_run",
                false,
            )
            .put(
                "world_registration_promoted",
                false,
            )
            .put(
                "field_response_calibration_promoted",
                false,
            )
            .put(
                "correction_authorized",
                false,
            )
            .put("candidate_applied", false)
            .put("image_transform_applied", false)
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}
