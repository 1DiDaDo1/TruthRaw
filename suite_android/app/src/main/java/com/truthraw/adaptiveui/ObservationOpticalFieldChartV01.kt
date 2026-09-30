package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.atan2
import kotlin.math.max
import kotlin.math.sqrt

/**
 * Observation-derived optical field chart v0.1.
 *
 * This does not identify a physical lens model and does not claim that the
 * raster center is the optical axis. It only unfolds the sealed observation
 * into a deterministic flat source-field coordinate chart:
 *
 * source position -> radial/tangential coordinates around a declared geometric
 * origin.
 *
 * Lens, sensor and scene contributions remain explicitly entangled until a
 * later versioned experiment can separate them.
 */
object ObservationOpticalFieldChartV01 {
    const val SCHEMA = "D.RAW/ObservationOpticalFieldChart/0.1"
    private const val RADIAL_BINS = 12

    fun describe(
        sourceSha256: String,
        sampleLattice: JSONObject,
        primaryRawRaster: JSONObject?,
        frontside: JSONObject?,
        optics: JSONObject?,
        measuredSignalProfile: JSONObject?,
    ): JSONObject {
        val width = sampleLattice.optInt("source_width", -1)
        val height = sampleLattice.optInt("source_height", -1)
        if (
            sampleLattice.optString("status") != "AVAILABLE" ||
            width <= 0 ||
            height <= 0
        ) {
            return unavailable(
                sourceSha256,
                "SOURCE_RASTER_GEOMETRY_UNAVAILABLE",
            )
        }

        val area = activeArea(primaryRawRaster, width, height)
        val top = area[0]
        val left = area[1]
        val bottom = area[2]
        val right = area[3]

        val centerX = (left.toDouble() + right.toDouble() - 1.0) * 0.5
        val centerY = (top.toDouble() + bottom.toDouble() - 1.0) * 0.5

        val units = sampleLattice.optLong(
            "coordinate_units_per_source_pixel",
            RasterIndependentSampleLatticeV01.UNITS_PER_SOURCE_PIXEL,
        )
        val centerU =
            ((left.toLong() + right.toLong() - 1L) * units) / 2L
        val centerV =
            ((top.toLong() + bottom.toLong() - 1L) * units) / 2L

        val corners = arrayOf(
            doubleArrayOf(left.toDouble(), top.toDouble()),
            doubleArrayOf((right - 1).toDouble(), top.toDouble()),
            doubleArrayOf(left.toDouble(), (bottom - 1).toDouble()),
            doubleArrayOf((right - 1).toDouble(), (bottom - 1).toDouble()),
        )
        var maxRadius = 0.0
        for (corner in corners) {
            maxRadius = max(
                maxRadius,
                hypot(
                    corner[0] - centerX,
                    corner[1] - centerY,
                ),
            )
        }

        val probes = JSONArray()
        addProbe(probes, "CENTER", centerX, centerY, centerX, centerY, maxRadius)
        addProbe(
            probes,
            "LEFT_MID",
            left.toDouble(),
            centerY,
            centerX,
            centerY,
            maxRadius,
        )
        addProbe(
            probes,
            "RIGHT_MID",
            (right - 1).toDouble(),
            centerY,
            centerX,
            centerY,
            maxRadius,
        )
        addProbe(
            probes,
            "TOP_MID",
            centerX,
            top.toDouble(),
            centerX,
            centerY,
            maxRadius,
        )
        addProbe(
            probes,
            "BOTTOM_MID",
            centerX,
            (bottom - 1).toDouble(),
            centerX,
            centerY,
            maxRadius,
        )
        addProbe(
            probes,
            "TOP_LEFT",
            left.toDouble(),
            top.toDouble(),
            centerX,
            centerY,
            maxRadius,
        )
        addProbe(
            probes,
            "TOP_RIGHT",
            (right - 1).toDouble(),
            top.toDouble(),
            centerX,
            centerY,
            maxRadius,
        )
        addProbe(
            probes,
            "BOTTOM_LEFT",
            left.toDouble(),
            (bottom - 1).toDouble(),
            centerX,
            centerY,
            maxRadius,
        )
        addProbe(
            probes,
            "BOTTOM_RIGHT",
            (right - 1).toDouble(),
            (bottom - 1).toDouble(),
            centerX,
            centerY,
            maxRadius,
        )

        val annuli = JSONArray()
        for (i in 0 until RADIAL_BINS) {
            annuli.put(
                JSONObject()
                    .put("bin", i)
                    .put("rho_min", i.toDouble() / RADIAL_BINS.toDouble())
                    .put(
                        "rho_max",
                        (i + 1).toDouble() / RADIAL_BINS.toDouble(),
                    ),
            )
        }

        val frontsideCentroid =
            frontside
                ?.optJSONObject("proportions")
                ?.opt("structural_centroid_normalized")
                ?: JSONObject.NULL

        val signal =
            if (
                measuredSignalProfile != null &&
                measuredSignalProfile.optString("status") ==
                "MEASURED_COMPOSITE_FIELD_SIGNAL_AVAILABLE"
            ) {
                measuredSignalProfile
            } else {
                JSONObject()
                    .put("status", "UNKNOWN")
                    .put(
                        "reason",
                        "MEASURED_FIELD_SIGNAL_PROFILE_UNAVAILABLE",
                    )
            }

        val opcodeList2Hint =
            primaryRawRaster
                ?.optJSONObject("opcode_list_2_metadata")
                ?: JSONObject()
                    .put("opcode_header_parse_status", "UNAVAILABLE")
                    .put("gain_map_present", false)
                    .put("gain_map_applied", false)

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "FIELD_CHART_AVAILABLE")
            .put("source_sha256", sourceSha256)
            .put(
                "interpretation",
                "FLAT_SOURCE_FIELD_COORDINATE_CHART_NOT_PHYSICAL_LENS_FLATTENING",
            )
            .put(
                "field_origin",
                JSONObject()
                    .put("source_x", centerX)
                    .put("source_y", centerY)
                    .put("lattice_u", centerU)
                    .put("lattice_v", centerV)
                    .put(
                        "authority",
                        "ACTIVE_AREA_GEOMETRIC_CENTER_ONLY",
                    )
                    .put("optical_axis_proven", false),
            )
            .put(
                "active_area",
                JSONArray()
                    .put(top)
                    .put(left)
                    .put(bottom)
                    .put(right),
            )
            .put("max_active_corner_radius_source_px", maxRadius)
            .put(
                "coordinate_definition",
                JSONObject()
                    .put("dx_source_px", "x-field_origin_x")
                    .put("dy_source_px", "y-field_origin_y")
                    .put(
                        "rho",
                        "sqrt(dx^2+dy^2)/max_active_corner_radius",
                    )
                    .put("azimuth", "atan2(dy,dx)")
                    .put(
                        "radial_unit",
                        "(dx/r,dy/r) for r>0",
                    )
                    .put(
                        "tangential_unit",
                        "(-dy/r,dx/r) for r>0",
                    )
                    .put(
                        "field_angle_from_rho_proven",
                        false,
                    ),
            )
            .put("field_probes", probes)
            .put("radial_annuli", annuli)
            .put(
                "reported_optics_context",
                JSONObject()
                    .put(
                        "focal_length_mm",
                        optics?.opt("focal_length_mm")
                            ?: JSONObject.NULL,
                    )
                    .put(
                        "f_number",
                        optics?.opt("f_number")
                            ?: JSONObject.NULL,
                    )
                    .put(
                        "metadata_used_to_define_field_coordinates",
                        false,
                    ),
            )
            .put(
                "frontside_context",
                JSONObject()
                    .put(
                        "structural_centroid_normalized",
                        frontsideCentroid,
                    )
                    .put(
                        "structural_centroid_used_as_optical_axis",
                        false,
                    )
                    .put("authority", "APPEARANCE_DERIVED_ONLY"),
            )
            .put("measured_composite_field_signal", signal)
            .put(
                "source_opcode_provenance_hint",
                JSONObject()
                    .put("opcode_list_2", opcodeList2Hint)
                    .put(
                        "authority",
                        "SOURCE_METADATA_PROVENANCE_HINT_ONLY",
                    )
                    .put("used_as_scientific_calibration", false)
                    .put("used_to_define_field_coordinates", false)
                    .put("used_to_modify_source_samples", false),
            )
            .put(
                "vignetting_interpretation",
                JSONObject()
                    .put(
                        "relative_illumination_candidate_observable",
                        true,
                    )
                    .put("lens_only_vignetting_proven", false)
                    .put("scene_illumination_separated", false)
                    .put("sensor_angular_response_separated", false)
                    .put("cos4_model_assumed", false)
                    .put("correction_gain_allowed", false)
                    .put(
                        "authority",
                        "COMPOSITE_SCENE_LENS_SENSOR_OBSERVATION_ONLY",
                    ),
            )
            .put("source_sample_values_modified", false)
            .put("source_sample_positions_modified", false)
            .put("new_measured_samples_created", false)
            .put("dense_lattice_materialized", false)
            .put("lens_profile_lookup_used", false)
            .put("camera_model_routing_used", false)
            .put("vendor_mapping_used", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun activeArea(
        raster: JSONObject?,
        width: Int,
        height: Int,
    ): IntArray {
        val raw = raster?.opt("active_area")
        val a = raw as? JSONArray
        if (a != null && a.length() >= 4) {
            val top = a.optInt(0, -1)
            val left = a.optInt(1, -1)
            val bottom = a.optInt(2, -1)
            val right = a.optInt(3, -1)
            if (
                top >= 0 &&
                left >= 0 &&
                bottom > top &&
                right > left &&
                bottom <= height &&
                right <= width
            ) {
                return intArrayOf(top, left, bottom, right)
            }
        }
        return intArrayOf(0, 0, height, width)
    }

    private fun addProbe(
        out: JSONArray,
        id: String,
        x: Double,
        y: Double,
        centerX: Double,
        centerY: Double,
        maxRadius: Double,
    ) {
        val dx = x - centerX
        val dy = y - centerY
        val r = hypot(dx, dy)
        val rho =
            if (maxRadius > 0.0) {
                (r / maxRadius).coerceIn(0.0, 1.0)
            } else {
                0.0
            }
        val azimuth = if (r > 0.0) atan2(dy, dx) else 0.0

        val radialX = if (r > 0.0) dx / r else 0.0
        val radialY = if (r > 0.0) dy / r else 0.0

        out.put(
            JSONObject()
                .put("id", id)
                .put("source_x", x)
                .put("source_y", y)
                .put("rho", rho)
                .put("azimuth_radians", azimuth)
                .put(
                    "radial_unit",
                    JSONArray().put(radialX).put(radialY),
                )
                .put(
                    "tangential_unit",
                    JSONArray().put(-radialY).put(radialX),
                ),
        )
    }

    private fun hypot(x: Double, y: Double): Double =
        sqrt(x * x + y * y)

    private fun unavailable(
        sourceSha256: String,
        reason: String,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("source_sha256", sourceSha256)
            .put("optical_axis_proven", false)
            .put("lens_only_vignetting_proven", false)
            .put("correction_gain_allowed", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}
