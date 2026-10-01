package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.abs
import kotlin.math.cos
import kotlin.math.sin
import kotlin.math.sqrt

/**
 * Read-only optical-field topography audit.
 *
 * The field is rendered conceptually as X/Y plus selectable Z ("height")
 * channels. Height never means physical scene depth or literal lens-surface
 * sag unless a future experiment proves such an interpretation. Current
 * layers are measured or derived diagnostic observables only.
 *
 * X/Y are derived from the measured sparse source-grid support centroid of
 * each FIELD_RESPONSE cell. The individual sparse source positions are
 * measured source evidence; the centroid and every topographic height derived
 * from them are not new measurements.
 */
object OpticalFieldTopographyAuditV01 {
    const val SCHEMA =
        "D.RAW/OpticalFieldTopographyAudit/0.1"

    private data class Support(
        val xUnit: Double,
        val yUnit: Double,
        val rho: Double,
        val azimuthRadians: Double,
        val pointCount: Int,
        val centroidOffsetNormalized: Double,
        val footprintRmsSourcePx: Double,
    )

    private data class CorrPair(
        val x: Double,
        val y: Double,
    )

    fun evaluate(
        records: List<JSONObject>,
        fieldResponse: JSONObject,
        measuredSupportAudit: JSONObject,
    ): JSONObject {
        if (
            fieldResponse.optString("status") !=
            "WORLD_SENSOR_FIELD_SEPARATION_CANDIDATE_AVAILABLE"
        ) {
            return unavailable(
                "FIELD_RESPONSE_SEPARATION_CANDIDATE_UNAVAILABLE",
            )
        }
        if (
            measuredSupportAudit.optString("status") !=
            "MEASURED_FIELD_SUPPORT_COORDINATE_BRIDGE_AUDIT_SET_AVAILABLE"
        ) {
            return unavailable(
                "MEASURED_FIELD_SUPPORT_AUDIT_UNAVAILABLE",
            )
        }

        val supportBySource =
            readSupportLookup(
                measuredSupportAudit,
            )
        if (supportBySource.isEmpty()) {
            return unavailable(
                "MEASURED_SUPPORT_CELLS_UNAVAILABLE",
            )
        }

        val world =
            fieldResponse.optJSONObject(
                "world_component_candidates_ev",
            ) ?: JSONObject()
        val sensor =
            fieldResponse.optJSONObject(
                "sensor_component_candidates_ev",
            ) ?: JSONObject()

        val reports = JSONArray()
        var admittedRecords = 0
        var availableRecords = 0

        for ((recordIndex, record) in records.withIndex()) {
            if (
                record.optString("axis_scope") !=
                "FIELD_RESPONSE"
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
            admittedRecords++

            val samples =
                payload.optJSONArray("samples")
                    ?: JSONArray()
            val perSource =
                linkedMapOf<String, JSONArray>()
            val allPoints = JSONArray()

            val trainOffsetResidual =
                ArrayList<CorrPair>()
            val heldOffsetResidual =
                ArrayList<CorrPair>()
            val trainFootprintResidual =
                ArrayList<CorrPair>()
            val heldFootprintResidual =
                ArrayList<CorrPair>()
            val trainSupportResidual =
                ArrayList<CorrPair>()
            val heldSupportResidual =
                ArrayList<CorrPair>()

            var predictableTrain = 0
            var predictableHeld = 0
            var supportMissing = 0

            for (i in 0 until samples.length()) {
                val sample =
                    samples.optJSONObject(i)
                        ?: continue
                val source =
                    sample.optString("source_sha256")
                        .trim()
                        .lowercase()
                val radial =
                    sample.optInt("radial_bin", -1)
                val sector =
                    sample.optInt("sensor_sector", -1)
                val support =
                    supportBySource[source]
                        ?.get(key(radial, sector))
                if (support == null) {
                    supportMissing++
                    continue
                }

                val role =
                    sample.optString(
                        "role",
                        "TRAIN",
                    )
                val actual =
                    sample.optDouble(
                        "relative_signal_ev",
                        Double.NaN,
                    )
                val worldId =
                    sample.optString(
                        "world_cell_id",
                    )
                val sensorId =
                    sample.optString(
                        "sensor_cell_id",
                    )
                val w =
                    if (world.has(worldId)) {
                        world.optDouble(
                            worldId,
                            Double.NaN,
                        )
                    } else {
                        Double.NaN
                    }
                val q =
                    if (sensor.has(sensorId)) {
                        sensor.optDouble(
                            sensorId,
                            Double.NaN,
                        )
                    } else {
                        Double.NaN
                    }
                val predicted =
                    if (
                        actual.isFinite() &&
                        w.isFinite() &&
                        q.isFinite()
                    ) {
                        w + q
                    } else {
                        Double.NaN
                    }
                val signedResidual =
                    if (predicted.isFinite()) {
                        actual - predicted
                    } else {
                        Double.NaN
                    }
                val absResidual =
                    if (signedResidual.isFinite()) {
                        abs(signedResidual)
                    } else {
                        Double.NaN
                    }

                if (absResidual.isFinite()) {
                    if (role == "HELD_OUT") {
                        predictableHeld++
                        heldOffsetResidual +=
                            CorrPair(
                                support.centroidOffsetNormalized,
                                absResidual,
                            )
                        heldFootprintResidual +=
                            CorrPair(
                                support.footprintRmsSourcePx,
                                absResidual,
                            )
                        heldSupportResidual +=
                            CorrPair(
                                support.pointCount.toDouble(),
                                absResidual,
                            )
                    } else {
                        predictableTrain++
                        trainOffsetResidual +=
                            CorrPair(
                                support.centroidOffsetNormalized,
                                absResidual,
                            )
                        trainFootprintResidual +=
                            CorrPair(
                                support.footprintRmsSourcePx,
                                absResidual,
                            )
                        trainSupportResidual +=
                            CorrPair(
                                support.pointCount.toDouble(),
                                absResidual,
                            )
                    }
                }

                val point =
                    JSONObject()
                        .put("source_sha256", source)
                        .put(
                            "observation_role",
                            sample.optString(
                                "observation_role",
                            ),
                        )
                        .put("role", role)
                        .put(
                            "radial_bin",
                            radial,
                        )
                        .put(
                            "sensor_sector",
                            sector,
                        )
                        .put(
                            "world_sector",
                            sample.optInt(
                                "world_sector",
                                -1,
                            ),
                        )
                        .put(
                            "world_cell_id",
                            worldId,
                        )
                        .put(
                            "sensor_cell_id",
                            sensorId,
                        )
                        .put(
                            "x_unit_disk",
                            support.xUnit,
                        )
                        .put(
                            "y_unit_disk",
                            support.yUnit,
                        )
                        .put(
                            "rho",
                            support.rho,
                        )
                        .put(
                            "azimuth_radians",
                            support.azimuthRadians,
                        )
                        .put(
                            "z_relative_signal_ev",
                            if (actual.isFinite()) {
                                actual
                            } else {
                                JSONObject.NULL
                            },
                        )
                        .put(
                            "z_nominal_model_signed_residual_ev",
                            if (signedResidual.isFinite()) {
                                signedResidual
                            } else {
                                JSONObject.NULL
                            },
                        )
                        .put(
                            "z_nominal_model_abs_residual_ev",
                            if (absResidual.isFinite()) {
                                absResidual
                            } else {
                                JSONObject.NULL
                            },
                        )
                        .put(
                            "z_support_point_count",
                            support.pointCount,
                        )
                        .put(
                            "z_support_centroid_offset_normalized",
                            support.centroidOffsetNormalized,
                        )
                        .put(
                            "z_support_footprint_rms_source_px",
                            support.footprintRmsSourcePx,
                        )
                        .put(
                            "height_is_physical_depth",
                            false,
                        )
                        .put(
                            "height_is_literal_lens_surface_sag",
                            false,
                        )

                allPoints.put(point)
                perSource
                    .getOrPut(source) {
                        JSONArray()
                    }
                    .put(point)
            }

            val sourceSurfaces =
                JSONArray()
            for ((source, points) in perSource) {
                sourceSurfaces.put(
                    JSONObject()
                        .put(
                            "source_sha256",
                            source,
                        )
                        .put(
                            "point_count",
                            points.length(),
                        )
                        .put(
                            "points",
                            points,
                        ),
                )
            }

            reports.put(
                JSONObject()
                    .put("schema", SCHEMA)
                    .put(
                        "status",
                        "OPTICAL_FIELD_TOPOGRAPHY_AUDIT_AVAILABLE",
                    )
                    .put(
                        "record_index",
                        recordIndex,
                    )
                    .put(
                        "coordinate_plane",
                        "MEASURED_SUPPORT_CENTROID_UNIT_DISK",
                    )
                    .put(
                        "height_layers",
                        heightLayers(),
                    )
                    .put(
                        "view_presets",
                        viewPresets(),
                    )
                    .put(
                        "source_surfaces",
                        sourceSurfaces,
                    )
                    .put(
                        "all_points",
                        allPoints,
                    )
                    .put(
                        "nominal_model_predictable_training_point_count",
                        predictableTrain,
                    )
                    .put(
                        "nominal_model_predictable_held_out_point_count",
                        predictableHeld,
                    )
                    .put(
                        "support_missing_sample_count",
                        supportMissing,
                    )
                    .put(
                        "association_diagnostics",
                        JSONObject()
                            .put(
                                "training_abs_residual_vs_centroid_offset_pearson",
                                pearsonJson(
                                    trainOffsetResidual,
                                ),
                            )
                            .put(
                                "held_out_abs_residual_vs_centroid_offset_pearson",
                                pearsonJson(
                                    heldOffsetResidual,
                                ),
                            )
                            .put(
                                "training_abs_residual_vs_support_footprint_pearson",
                                pearsonJson(
                                    trainFootprintResidual,
                                ),
                            )
                            .put(
                                "held_out_abs_residual_vs_support_footprint_pearson",
                                pearsonJson(
                                    heldFootprintResidual,
                                ),
                            )
                            .put(
                                "training_abs_residual_vs_support_count_pearson",
                                pearsonJson(
                                    trainSupportResidual,
                                ),
                            )
                            .put(
                                "held_out_abs_residual_vs_support_count_pearson",
                                pearsonJson(
                                    heldSupportResidual,
                                ),
                            )
                            .put(
                                "association_is_causation_proof",
                                false,
                            )
                            .put(
                                "correlations_are_promotion_thresholds",
                                false,
                            ),
                    )
                    .put(
                        "interpretation_contract",
                        JSONObject()
                            .put(
                                "topographic_height_is_physical_scene_depth",
                                false,
                            )
                            .put(
                                "topographic_height_is_literal_lens_surface_sag",
                                false,
                            )
                            .put(
                                "topographic_height_is_selected_observable_only",
                                true,
                            )
                            .put(
                                "measured_sparse_positions_are_source_evidence",
                                true,
                            )
                            .put(
                                "support_centroids_are_derived",
                                true,
                            )
                            .put(
                                "field_signal_is_scene_lens_sensor_composite",
                                true,
                            )
                            .put(
                                "nominal_residual_is_lens_only_error",
                                false,
                            )
                            .put(
                                "side_view_may_reveal_spatial_ridges_and_basins",
                                true,
                            )
                            .put(
                                "ridge_alignment_proves_common_physical_cause",
                                false,
                            ),
                    )
                    .put(
                        "automatic_problem_cause_selected",
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
                    .put(
                        "candidate_applied",
                        false,
                    )
                    .put(
                        "source_sample_values_modified",
                        false,
                    )
                    .put(
                        "source_sample_positions_modified",
                        false,
                    )
                    .put(
                        "creates_new_evidence",
                        false,
                    )
                    .put(
                        "scientific_writeback_allowed",
                        false,
                    ),
            )
            availableRecords++
        }

        if (admittedRecords == 0) {
            return unavailable(
                "NO_ADMITTED_CONTROLLED_ROTATION_FIELD_RECORD",
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (availableRecords > 0) {
                    "OPTICAL_FIELD_TOPOGRAPHY_AUDIT_SET_AVAILABLE"
                } else {
                    "UNKNOWN_FAIL_CLOSED"
                },
            )
            .put(
                "admitted_record_count",
                admittedRecords,
            )
            .put(
                "available_record_count",
                availableRecords,
            )
            .put(
                "record_reports",
                reports,
            )
            .put(
                "height_is_physical_scene_depth",
                false,
            )
            .put(
                "height_is_literal_lens_surface_sag",
                false,
            )
            .put(
                "automatic_problem_cause_selected",
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
            .put(
                "source_sample_values_modified",
                false,
            )
            .put(
                "source_sample_positions_modified",
                false,
            )
            .put("creates_new_evidence", false)
            .put(
                "scientific_writeback_allowed",
                false,
            )
    }

    private fun readSupportLookup(
        audit: JSONObject,
    ): Map<String, Map<String, Support>> {
        val reports =
            audit.optJSONArray("record_reports")
                ?: return emptyMap()
        val out =
            linkedMapOf<String, MutableMap<String, Support>>()

        for (ri in 0 until reports.length()) {
            val report =
                reports.optJSONObject(ri)
                    ?: continue
            if (
                report.optString("status") !=
                "MEASURED_FIELD_SUPPORT_COORDINATE_BRIDGE_AUDIT_AVAILABLE"
            ) {
                continue
            }
            val diagnostics =
                report.optJSONArray(
                    "source_diagnostics",
                ) ?: continue
            for (di in 0 until diagnostics.length()) {
                val d =
                    diagnostics.optJSONObject(di)
                        ?: continue
                val source =
                    d.optString("source_sha256")
                        .trim()
                        .lowercase()
                if (source.isBlank()) continue
                val cells =
                    d.optJSONArray("support_cells")
                        ?: continue
                val target =
                    out.getOrPut(source) {
                        linkedMapOf()
                    }
                for (ci in 0 until cells.length()) {
                    val c =
                        cells.optJSONObject(ci)
                            ?: continue
                    val radial =
                        c.optInt(
                            "radial_bin",
                            -1,
                        )
                    val sector =
                        c.optInt(
                            "sensor_sector",
                            -1,
                        )
                    val rho =
                        c.optDouble(
                            "centroid_rho",
                            Double.NaN,
                        )
                    val angle =
                        c.optDouble(
                            "centroid_azimuth_radians",
                            Double.NaN,
                        )
                    val offset =
                        c.optDouble(
                            "centroid_offset_from_theoretical_bin_center_normalized",
                            Double.NaN,
                        )
                    val footprint =
                        c.optDouble(
                            "footprint_rms_radius_source_px",
                            Double.NaN,
                        )
                    val pointCount =
                        c.optInt(
                            "support_point_count",
                            0,
                        )
                    if (
                        radial < 0 ||
                        sector < 0 ||
                        !rho.isFinite() ||
                        !angle.isFinite() ||
                        !offset.isFinite() ||
                        !footprint.isFinite() ||
                        pointCount <= 0
                    ) {
                        continue
                    }
                    target[key(radial, sector)] =
                        Support(
                            xUnit =
                                rho * cos(angle),
                            yUnit =
                                rho * sin(angle),
                            rho = rho,
                            azimuthRadians = angle,
                            pointCount = pointCount,
                            centroidOffsetNormalized =
                                offset,
                            footprintRmsSourcePx =
                                footprint,
                        )
                }
            }
        }
        return out
    }

    private fun heightLayers(): JSONArray =
        JSONArray()
            .put(
                layer(
                    "RELATIVE_SIGNAL_EV",
                    "z_relative_signal_ev",
                    "EV",
                    "MEASURED_RELATIVE_FIELD_SIGNAL_FROM_ADMITTED_RELATION_RECORD",
                    false,
                ),
            )
            .put(
                layer(
                    "NOMINAL_MODEL_SIGNED_RESIDUAL_EV",
                    "z_nominal_model_signed_residual_ev",
                    "EV",
                    "DERIVED_DIAGNOSTIC_RESIDUAL",
                    false,
                ),
            )
            .put(
                layer(
                    "NOMINAL_MODEL_ABS_RESIDUAL_EV",
                    "z_nominal_model_abs_residual_ev",
                    "EV",
                    "DERIVED_DIAGNOSTIC_ERROR_MAGNITUDE",
                    false,
                ),
            )
            .put(
                layer(
                    "SUPPORT_POINT_COUNT",
                    "z_support_point_count",
                    "COUNT",
                    "DERIVED_COUNT_OF_MEASURED_SOURCE_POSITIONS",
                    false,
                ),
            )
            .put(
                layer(
                    "SUPPORT_CENTROID_OFFSET_NORMALIZED",
                    "z_support_centroid_offset_normalized",
                    "UNIT_DISK_DISTANCE",
                    "DERIVED_GEOMETRIC_OFFSET_FROM_THEORETICAL_BIN_CENTER",
                    false,
                ),
            )
            .put(
                layer(
                    "SUPPORT_FOOTPRINT_RMS_SOURCE_PX",
                    "z_support_footprint_rms_source_px",
                    "SOURCE_PIXEL",
                    "DERIVED_SPREAD_OF_MEASURED_SOURCE_POSITIONS",
                    false,
                ),
            )

    private fun layer(
        id: String,
        field: String,
        unit: String,
        authority: String,
        physicalHeight: Boolean,
    ): JSONObject =
        JSONObject()
            .put("layer_id", id)
            .put("point_field", field)
            .put("unit", unit)
            .put("authority", authority)
            .put(
                "height_is_physical_depth_or_lens_sag",
                physicalHeight,
            )

    private fun viewPresets(): JSONArray =
        JSONArray()
            .put(
                JSONObject()
                    .put("id", "TOP_DOWN")
                    .put(
                        "purpose",
                        "SPATIAL_FIELD_LOCATION",
                    )
                    .put(
                        "camera_pitch_degrees",
                        -90,
                    )
                    .put("camera_yaw_degrees", 0),
            )
            .put(
                JSONObject()
                    .put("id", "SIDE_X")
                    .put(
                        "purpose",
                        "HEIGHT_PROFILE_ALONG_FIELD_X",
                    )
                    .put("camera_pitch_degrees", 0)
                    .put("camera_yaw_degrees", 0),
            )
            .put(
                JSONObject()
                    .put("id", "SIDE_Y")
                    .put(
                        "purpose",
                        "HEIGHT_PROFILE_ALONG_FIELD_Y",
                    )
                    .put("camera_pitch_degrees", 0)
                    .put("camera_yaw_degrees", 90),
            )
            .put(
                JSONObject()
                    .put("id", "OBLIQUE")
                    .put(
                        "purpose",
                        "RIDGE_BASIN_INSPECTION",
                    )
                    .put(
                        "camera_pitch_degrees",
                        -35,
                    )
                    .put(
                        "camera_yaw_degrees",
                        45,
                    ),
            )

    private fun pearsonJson(
        pairs: List<CorrPair>,
    ): JSONObject {
        val finite =
            pairs.filter {
                it.x.isFinite() &&
                    it.y.isFinite()
            }
        if (finite.size < 3) {
            return JSONObject()
                .put("status", "UNKNOWN")
                .put(
                    "reason",
                    "INSUFFICIENT_PAIRED_SUPPORT",
                )
                .put(
                    "sample_count",
                    finite.size,
                )
                .put(
                    "pearson_r",
                    JSONObject.NULL,
                )
                .put(
                    "causation_proven",
                    false,
                )
        }

        val meanX =
            finite.sumOf { it.x } /
                finite.size.toDouble()
        val meanY =
            finite.sumOf { it.y } /
                finite.size.toDouble()
        var cov = 0.0
        var sx = 0.0
        var sy = 0.0
        for (p in finite) {
            val dx = p.x - meanX
            val dy = p.y - meanY
            cov += dx * dy
            sx += dx * dx
            sy += dy * dy
        }
        val denom =
            sqrt(sx * sy)
        val r =
            if (
                denom.isFinite() &&
                denom > 1.0e-15
            ) {
                cov / denom
            } else {
                Double.NaN
            }

        return JSONObject()
            .put(
                "status",
                if (r.isFinite()) {
                    "DESCRIPTIVE_ASSOCIATION_AVAILABLE"
                } else {
                    "UNKNOWN"
                },
            )
            .put(
                "sample_count",
                finite.size,
            )
            .put(
                "pearson_r",
                if (r.isFinite()) {
                    r
                } else {
                    JSONObject.NULL
                },
            )
            .put(
                "causation_proven",
                false,
            )
            .put(
                "used_for_automatic_selection",
                false,
            )
    }

    private fun key(
        radialBin: Int,
        sector: Int,
    ): String =
        "$radialBin:$sector"

    private fun unavailable(
        reason: String,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put(
                "height_is_physical_scene_depth",
                false,
            )
            .put(
                "height_is_literal_lens_surface_sag",
                false,
            )
            .put(
                "automatic_problem_cause_selected",
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
            .put("creates_new_evidence", false)
            .put(
                "scientific_writeback_allowed",
                false,
            )
}
