package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.roundToLong

/**
 * D.RAW raster-independent scientific coordinate lattice v0.1.
 *
 * This is NOT an upscaler and NOT a dense replacement image.
 *
 * The sealed source raster contributes exact measured anchor locations:
 *     u = x * 2^20
 *     v = y * 2^20
 *
 * Positions between anchors are coordinate locations only. They start with
 * UNKNOWN authority and acquire no value unless a later versioned scientific
 * reconstruction supplies one with explicit provenance/uncertainty.
 */
object RasterIndependentSampleLatticeV01 {
    const val FRACTION_BITS: Int = 20
    const val UNITS_PER_SOURCE_PIXEL: Long = 1L shl FRACTION_BITS
    const val SCHEMA: String = "D.RAW/RasterIndependentSampleLattice/0.1"

    fun describe(
        sourceSha256: String,
        sourceWidth: Int?,
        sourceHeight: Int?,
        sourceClass: String,
        cfaPattern: Any?,
    ): JSONObject {
        val validRaster =
            sourceWidth != null && sourceHeight != null &&
                sourceWidth > 0 && sourceHeight > 0
        val cfaAnchors =
            validRaster && sourceClass == "DNG_CFA_RAW"
        val anchorCount =
            if (cfaAnchors) {
                sourceWidth!!.toLong() * sourceHeight!!.toLong()
            } else {
                null
            }

        val coordinateExtent =
            if (validRaster) {
                JSONArray()
                    .put(sourceToLattice(sourceWidth!! - 1))
                    .put(sourceToLattice(sourceHeight!! - 1))
            } else {
                JSONObject.NULL
            }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", if (validRaster) "AVAILABLE" else "UNKNOWN_FAIL_CLOSED")
            .put("source_sha256", sourceSha256)
            .put("source_class", sourceClass)
            .put("source_width", sourceWidth ?: JSONObject.NULL)
            .put("source_height", sourceHeight ?: JSONObject.NULL)
            .put("coordinate_fraction_bits", FRACTION_BITS)
            .put("coordinate_units_per_source_pixel", UNITS_PER_SOURCE_PIXEL)
            .put("coordinate_extent_last_anchor_units", coordinateExtent)
            .put(
                "representation",
                "SPARSE_IMPLICIT_FIXED_POINT_SAMPLE_LATTICE",
            )
            .put(
                "measured_anchor_mapping",
                "u=x*1048576; v=y*1048576",
            )
            .put(
                "measured_anchor_count",
                anchorCount ?: JSONObject.NULL,
            )
            .put(
                "measured_anchor_count_authority",
                if (cfaAnchors) {
                    "SOURCE_RASTER_LAYOUT_BOUND"
                } else {
                    "UNKNOWN_FOR_THIS_SOURCE_CLASS"
                },
            )
            .put("cfa_pattern", cfaPattern ?: JSONObject.NULL)
            .put(
                "source_samples_are_only_measured_anchors",
                cfaAnchors,
            )
            .put(
                "unanchored_lattice_positions_authority",
                "UNKNOWN",
            )
            .put(
                "unanchored_lattice_positions_have_values",
                false,
            )
            .put("dense_lattice_materialized", false)
            .put("dense_cell_count_claimed", false)
            .put("source_values_resampled", false)
            .put("source_values_interpolated", false)
            .put("source_values_modified", false)
            .put("source_bytes_modified", false)
            .put("new_sensor_measurements_created", false)
            .put("optical_resolution_increased", false)
            .put("upscaling_performed", false)
            .put("output_resolution_claimed", false)
            .put(
                "coordinate_precision_is_sensor_resolution_claim",
                false,
            )
            .put(
                "finite_source_raster_is_world_boundary",
                false,
            )
            .put(
                "finite_source_raster_is_measurement_sampling",
                true,
            )
            .put(
                "scientific_role",
                "RASTER_INDEPENDENT_COORDINATE_DOMAIN_FOR_MEASURED_ANCHORS_AND_VERSIONED_RECONSTRUCTION",
            )
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    fun bindSupportGeometry(
        sourceSha256: String,
        lattice: JSONObject,
        supportDistance: JSONObject?,
    ): JSONObject {
        if (
            lattice.optString("schema") != SCHEMA ||
            lattice.optString("source_sha256") != sourceSha256 ||
            lattice.optString("status") != "AVAILABLE"
        ) {
            return unavailableGeometry(
                sourceSha256,
                "SAMPLE_LATTICE_NOT_AVAILABLE",
            )
        }

        if (
            supportDistance == null ||
            supportDistance.optString("schema") !=
            "D.RAW/Frontside/N2SampleSupportDistance/0.1" ||
            supportDistance.optString("source_sha256") != sourceSha256
        ) {
            return unavailableGeometry(
                sourceSha256,
                "N2_SUPPORT_DISTANCE_BINDING_MISMATCH",
            )
        }

        val status = supportDistance.optString("status")
        if (status == "NOT_REQUIRED_BY_CURRENT_FRONT_SIDE_STATE") {
            return unavailableGeometry(
                sourceSha256,
                supportDistance.optString(
                    "reason",
                    "N2_SUPPORT_DISTANCE_NOT_REQUIRED",
                ),
            ).put("status", "NOT_REQUIRED_BY_CURRENT_FRONT_SIDE_STATE")
        }
        if (status != "AUDIT_ONLY_DISTANCE_BINDING_AVAILABLE") {
            return unavailableGeometry(
                sourceSha256,
                supportDistance.optString(
                    "reason",
                    "N2_SUPPORT_DISTANCE_NOT_AVAILABLE",
                ),
            )
        }

        val sourceQueries =
            supportDistance.optJSONArray("queries") ?: JSONArray()
        val latticeQueries = JSONArray()

        for (i in 0 until sourceQueries.length()) {
            val q = sourceQueries.optJSONObject(i) ?: continue
            val sourceRect = q.optJSONArray("source_rect")
            if (sourceRect == null || sourceRect.length() < 4) continue

            val left = sourceRect.optLong(0)
            val top = sourceRect.optLong(1)
            val right = sourceRect.optLong(2)
            val bottom = sourceRect.optLong(3)
            if (right <= left || bottom <= top) continue

            val latticeRect =
                JSONArray()
                    .put(sourceToLattice(left))
                    .put(sourceToLattice(top))
                    .put(sourceToLattice(right))
                    .put(sourceToLattice(bottom))

            // Source support samples use integer source-coordinate points.
            // Candidate center is derived exactly from its source rectangle;
            // because the fixed-point scale is even, half-pixel centers remain
            // integer lattice coordinates.
            val centerU =
                ((left + right - 1L) * UNITS_PER_SOURCE_PIXEL) / 2L
            val centerV =
                ((top + bottom - 1L) * UNITS_PER_SOURCE_PIXEL) / 2L

            latticeQueries.put(
                JSONObject()
                    .put("id", q.optInt("id"))
                    .put("frontside_x", q.optInt("frontside_x"))
                    .put("frontside_y", q.optInt("frontside_y"))
                    .put("source_rect", sourceRect)
                    .put("lattice_rect_units", latticeRect)
                    .put(
                        "lattice_center_units",
                        JSONArray().put(centerU).put(centerV),
                    )
                    .put(
                        "nearest_structure_from_center",
                        mapNearest(q.opt("nearest_structure_from_center")),
                    )
                    .put(
                        "nearest_structure_to_rect",
                        mapNearest(q.opt("nearest_structure_to_rect")),
                    )
                    .put(
                        "nearest_censored_from_center",
                        mapNearest(q.opt("nearest_censored_from_center")),
                    )
                    .put(
                        "nearest_censored_to_rect",
                        mapNearest(q.opt("nearest_censored_to_rect")),
                    )
                    .put(
                        "nearest_censor_boundary_from_center",
                        mapNearest(
                            q.opt("nearest_censor_boundary_from_center"),
                        ),
                    )
                    .put(
                        "nearest_censor_boundary_to_rect",
                        mapNearest(
                            q.opt("nearest_censor_boundary_to_rect"),
                        ),
                    )
                    .put(
                        "center_radii_lattice_units",
                        scaleArray(
                            supportDistance.optJSONArray(
                                "center_radii_px",
                            ),
                        ),
                    )
                    .put(
                        "rect_margin_radii_lattice_units",
                        scaleArray(
                            supportDistance.optJSONArray(
                                "rect_margin_radii_px",
                            ),
                        ),
                    )
                    .put(
                        "center_structure_fraction",
                        q.optJSONArray("center_structure_fraction")
                            ?: JSONArray(),
                    )
                    .put(
                        "rect_margin_structure_fraction",
                        q.optJSONArray(
                            "rect_margin_structure_fraction",
                        ) ?: JSONArray(),
                    )
                    .put(
                        "unanchored_positions_inferred",
                        false,
                    )
                    .put(
                        "correction_supported",
                        false,
                    ),
            )
        }

        return JSONObject()
            .put(
                "schema",
                "D.RAW/N2RasterIndependentSampleGeometry/0.1",
            )
            .put("status", "AUDIT_ONLY_LATTICE_BINDING_AVAILABLE")
            .put("source_sha256", sourceSha256)
            .put("sample_lattice_schema", SCHEMA)
            .put(
                "coordinate_units_per_source_pixel",
                UNITS_PER_SOURCE_PIXEL,
            )
            .put("query_count", latticeQueries.length())
            .put("queries", latticeQueries)
            .put("source_sample_values_modified", false)
            .put("source_sample_positions_modified", false)
            .put("new_measured_samples_created", false)
            .put("unanchored_positions_inferred", false)
            .put("interpolation_performed", false)
            .put("upscaling_performed", false)
            .put("distance_threshold_admitted", false)
            .put("can_reduce_protection", false)
            .put("can_enable_correction", false)
            .put("chroma_correction_supported", false)
            .put("private_ab_delta_allowed", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun unavailableGeometry(
        sourceSha256: String,
        reason: String,
    ): JSONObject =
        JSONObject()
            .put(
                "schema",
                "D.RAW/N2RasterIndependentSampleGeometry/0.1",
            )
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("source_sha256", sourceSha256)
            .put(
                "coordinate_units_per_source_pixel",
                UNITS_PER_SOURCE_PIXEL,
            )
            .put("new_measured_samples_created", false)
            .put("unanchored_positions_inferred", false)
            .put("interpolation_performed", false)
            .put("upscaling_performed", false)
            .put("can_reduce_protection", false)
            .put("can_enable_correction", false)
            .put("chroma_correction_supported", false)
            .put("scientific_writeback_allowed", false)

    private fun mapNearest(value: Any?): Any {
        val source = value as? JSONObject ?: return JSONObject.NULL
        val x = source.optLong("x", Long.MIN_VALUE)
        val y = source.optLong("y", Long.MIN_VALUE)
        val distancePx =
            source.optDouble("distance_px", Double.NaN)
        if (x == Long.MIN_VALUE || y == Long.MIN_VALUE) {
            return JSONObject.NULL
        }

        return JSONObject()
            .put("source_x", x)
            .put("source_y", y)
            .put(
                "lattice_u",
                sourceToLattice(x),
            )
            .put(
                "lattice_v",
                sourceToLattice(y),
            )
            .put(
                "distance_source_pixels",
                if (distancePx.isFinite()) {
                    distancePx
                } else {
                    JSONObject.NULL
                },
            )
            .put(
                "distance_lattice_units",
                if (distancePx.isFinite()) {
                    (distancePx * UNITS_PER_SOURCE_PIXEL.toDouble())
                        .roundToLong()
                } else {
                    JSONObject.NULL
                },
            )
    }

    private fun scaleArray(source: JSONArray?): JSONArray {
        val out = JSONArray()
        if (source == null) return out
        for (i in 0 until source.length()) {
            val v = source.optLong(i, Long.MIN_VALUE)
            if (v == Long.MIN_VALUE) {
                out.put(JSONObject.NULL)
            } else {
                out.put(sourceToLattice(v))
            }
        }
        return out
    }

    private fun sourceToLattice(value: Int): Long =
        sourceToLattice(value.toLong())

    private fun sourceToLattice(value: Long): Long =
        Math.multiplyExact(value, UNITS_PER_SOURCE_PIXEL)
}
