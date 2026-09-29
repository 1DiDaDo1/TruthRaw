package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import android.os.ParcelFileDescriptor
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import kotlin.math.max

object TruthNegativeN2SupportDistanceBridge {
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
 * Exact sampled support-distance audit.
 *
 * The native sidecar records exact N2 sampled coordinates for Structure,
 * Censored and CensorBoundary preserve reasons using a compact base64
 * little-endian U32 x/y stream. The Android profile keeps only compact
 * query metrics and binding hashes; no unsampled pixel is inferred.
 */
object N2SampleSupportDistanceAudit {
    private const val MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
    private const val MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

    fun unavailable(
        sourceSha256: String,
        reason: String,
    ): JSONObject =
        JSONObject()
            .put("schema", "D.RAW/Frontside/N2SampleSupportDistance/0.1")
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("source_sha256", sourceSha256)
            .put("authority", "EXACT_SAMPLED_SUPPORT_GEOMETRY_DIAGNOSTIC_ONLY")
            .put("distance_binding_available", false)
            .put("exact_sample_coordinates_recorded", false)
            .put("unsampled_pixels_inferred", false)
            .put("scalar_probability_created", false)
            .put("can_reduce_protection", false)
            .put("can_enable_correction", false)
            .put("chroma_correction_supported", false)
            .put("private_ab_delta_allowed", false)
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
        fineStructure: JSONObject?,
    ): JSONObject {
        if (frontsideV01 == null) {
            return unavailable(sourceSha256, "FRONTSIDE_V0_1_MISSING")
        }
        if (
            frontsideV01.optString("schema") !=
            "D.RAW/Frontside/DarkChromaStability/0.1" ||
            frontsideV01.optString("source_sha256") != sourceSha256
        ) {
            return unavailable(sourceSha256, "FRONTSIDE_V0_1_BINDING_MISMATCH")
        }

        val analysisWidth = frontsideV01.optInt("analysis_width", 0)
        val analysisHeight = frontsideV01.optInt("analysis_height", 0)
        val tiles = frontsideV01.optJSONArray("tiles") ?: JSONArray()
        if (analysisWidth <= 0 || analysisHeight <= 0 || tiles.length() <= 0) {
            return unavailable(sourceSha256, "FRONTSIDE_DIMENSION_CONTRACT_MISMATCH")
        }

        val candidateData = buildCandidateData(frontsideV01)
        if (candidateData.isEmpty()) {
            return skipped(sourceSha256, "NO_VISIBLE_DARK_CHROMA_CANDIDATES")
        }

        val temp = runCatching {
            File.createTempFile(
                "draw_n2_support_distance_",
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
                        TruthNegativeN2SupportDistanceBridge.exportAndVerify(
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
                "N2_SUPPORT_DISTANCE_BRIDGE_FAILED",
            )

            val status = runCatching { JSONObject(statusText) }.getOrNull()
                ?: return unavailable(
                    sourceSha256,
                    "N2_SUPPORT_DISTANCE_STATUS_INVALID",
                )
            if (status.optInt("status", -999) != 0) {
                return unavailable(
                    sourceSha256,
                    "N2_SUPPORT_DISTANCE_STATUS_" +
                        status.optInt("status", -999),
                )
            }
            if (
                !status.optBoolean("postWriteVerified", false) ||
                status.optString("sourceSha256") != sourceSha256 ||
                !status.optBoolean("exactSampleCoordinatesRecorded", false) ||
                status.optBoolean("unsampledPixelsInferred", true) ||
                status.optBoolean("scalarProbabilityCreated", true) ||
                status.optBoolean("canReduceProtection", true) ||
                status.optBoolean("canEnableCorrection", true) ||
                status.optBoolean("candidateApplied", true) ||
                status.optBoolean("scientificWritebackAllowed", true)
            ) {
                return unavailable(
                    sourceSha256,
                    "N2_SUPPORT_DISTANCE_STATUS_CONTRACT_MISMATCH",
                )
            }

            val rawText = runCatching { temp.readText() }.getOrNull()
                ?: return unavailable(
                    sourceSha256,
                    "N2_SUPPORT_DISTANCE_FILE_READ_FAILED",
                )
            val sidecar = runCatching { JSONObject(rawText) }.getOrNull()
                ?: return unavailable(
                    sourceSha256,
                    "N2_SUPPORT_DISTANCE_FILE_INVALID",
                )

            if (
                sidecar.optString("schema") !=
                "D.RAW/TruthNegative/N2SampleSupportDistance/0.1" ||
                sidecar.optString("source_sha256") != sourceSha256 ||
                !sidecar.optBoolean("exact_sample_coordinates_recorded", false) ||
                sidecar.optBoolean("unsampled_pixels_inferred", true) ||
                sidecar.optBoolean("scalar_probability_created", true) ||
                sidecar.optBoolean("can_reduce_protection", true) ||
                sidecar.optBoolean("can_enable_correction", true) ||
                sidecar.optBoolean("candidate_applied", true) ||
                sidecar.optBoolean("scientific_writeback_allowed", true)
            ) {
                return unavailable(
                    sourceSha256,
                    "N2_SUPPORT_DISTANCE_FILE_CONTRACT_MISMATCH",
                )
            }

            val queries = sidecar.optJSONArray("queries") ?: JSONArray()
            if (
                queries.length() <= 0 ||
                queries.length() != candidateData.size / 4
            ) {
                return unavailable(
                    sourceSha256,
                    "N2_SUPPORT_DISTANCE_QUERY_COUNT_MISMATCH",
                )
            }

            val compactQueries = JSONArray()
            val nearestCenterStructure = ArrayList<Double>()
            val nearestRectStructure = ArrayList<Double>()
            val centerZeroCounts = LongArray(4)
            var structureInsideRectCandidates = 0L
            var censorInsideRectCandidates = 0L
            var boundaryInsideRectCandidates = 0L

            for (i in 0 until queries.length()) {
                val q = queries.optJSONObject(i) ?: continue
                val centerStructure =
                    q.optJSONArray("center_structure") ?: JSONArray()
                val centerFractions =
                    q.optJSONArray("center_structure_fraction") ?: JSONArray()
                val rectSampled =
                    q.optJSONArray("rect_margin_sampled") ?: JSONArray()
                val rectStructure =
                    q.optJSONArray("rect_margin_structure") ?: JSONArray()
                val rectFractions =
                    q.optJSONArray("rect_margin_structure_fraction") ?: JSONArray()
                val rectCensored =
                    q.optJSONArray("rect_margin_censored") ?: JSONArray()
                val rectBoundary =
                    q.optJSONArray("rect_margin_censor_boundary") ?: JSONArray()

                for (r in 0 until minOf(4, centerStructure.length())) {
                    if (centerStructure.optLong(r, 0L) == 0L) {
                        centerZeroCounts[r]++
                    }
                }
                if (rectStructure.optLong(0, 0L) > 0L) {
                    structureInsideRectCandidates++
                }
                if (rectCensored.optLong(0, 0L) > 0L) {
                    censorInsideRectCandidates++
                }
                if (rectBoundary.optLong(0, 0L) > 0L) {
                    boundaryInsideRectCandidates++
                }

                val nearestCenter =
                    q.optJSONObject("nearest_structure_from_center")
                val nearestRect =
                    q.optJSONObject("nearest_structure_to_rect")
                nearestCenter?.optDouble("distance_px", Double.NaN)
                    ?.takeIf { it.isFinite() }
                    ?.let(nearestCenterStructure::add)
                nearestRect?.optDouble("distance_px", Double.NaN)
                    ?.takeIf { it.isFinite() }
                    ?.let(nearestRectStructure::add)

                compactQueries.put(
                    JSONObject()
                        .put("id", q.optInt("id"))
                        .put("frontside_x", q.optInt("frontside_x"))
                        .put("frontside_y", q.optInt("frontside_y"))
                        .put(
                            "frontside_width",
                            q.optInt("frontside_width"),
                        )
                        .put(
                            "frontside_height",
                            q.optInt("frontside_height"),
                        )
                        .put("source_rect", q.optJSONArray("source_rect"))
                        .put("source_center", q.optJSONArray("source_center"))
                        .put(
                            "center_structure",
                            centerStructure,
                        )
                        .put(
                            "center_structure_fraction",
                            centerFractions,
                        )
                        .put("rect_margin_sampled", rectSampled)
                        .put("rect_margin_structure", rectStructure)
                        .put(
                            "rect_margin_structure_fraction",
                            rectFractions,
                        )
                        .put("rect_margin_censored", rectCensored)
                        .put(
                            "rect_margin_censor_boundary",
                            rectBoundary,
                        )
                        .put(
                            "nearest_structure_from_center",
                            nearestCenter ?: JSONObject.NULL,
                        )
                        .put(
                            "nearest_structure_to_rect",
                            nearestRect ?: JSONObject.NULL,
                        )
                        .put(
                            "nearest_censored_from_center",
                            q.opt("nearest_censored_from_center"),
                        )
                        .put(
                            "nearest_censored_to_rect",
                            q.opt("nearest_censored_to_rect"),
                        )
                        .put(
                            "nearest_censor_boundary_from_center",
                            q.opt("nearest_censor_boundary_from_center"),
                        )
                        .put(
                            "nearest_censor_boundary_to_rect",
                            q.opt("nearest_censor_boundary_to_rect"),
                        ),
                )
            }

            val sidecarGlobal =
                sidecar.optJSONObject("global") ?: JSONObject()
            val fineGlobal =
                fineStructure?.optJSONObject("field_global") ?: JSONObject()
            val parityAvailable =
                fineStructure?.optString("status") ==
                    "AUDIT_ONLY_FINE_BINDING_AVAILABLE"
            val aggregateParity =
                parityAvailable &&
                    fineGlobal.optLong("sampled", -1L) ==
                    sidecarGlobal.optLong("sampled", -2L) &&
                    fineGlobal.optLong("structure_protected", -1L) ==
                    sidecarGlobal.optLong("structure_protected", -2L) &&
                    fineGlobal.optLong("censored_protected", -1L) ==
                    sidecarGlobal.optLong("censored_protected", -2L) &&
                    fineGlobal.optLong(
                        "censor_boundary_protected",
                        -1L,
                    ) ==
                    sidecarGlobal.optLong(
                        "censor_boundary_protected",
                        -2L,
                    )

            nearestCenterStructure.sort()
            nearestRectStructure.sort()

            return JSONObject()
                .put(
                    "schema",
                    "D.RAW/Frontside/N2SampleSupportDistance/0.1",
                )
                .put("status", "AUDIT_ONLY_DISTANCE_BINDING_AVAILABLE")
                .put("source_sha256", sourceSha256)
                .put(
                    "authority",
                    "EXACT_SAMPLED_SUPPORT_GEOMETRY_DIAGNOSTIC_ONLY",
                )
                .put(
                    "candidate_sha256",
                    sidecar.optString("candidate_sha256"),
                )
                .put(
                    "n2_audit_sha256",
                    sidecar.optString("audit_sha256"),
                )
                .put(
                    "support_point_stream_sha256",
                    sidecar.optString("support_point_stream_sha256"),
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
                    "coordinate_encoding",
                    sidecar.optString("coordinate_encoding"),
                )
                .put(
                    "center_radii_px",
                    sidecar.optJSONArray("center_radii_px"),
                )
                .put(
                    "rect_margin_radii_px",
                    sidecar.optJSONArray("rect_margin_radii_px"),
                )
                .put("field_global", sidecarGlobal)
                .put("v0_5_aggregate_parity_available", parityAvailable)
                .put("v0_5_aggregate_parity_verified", aggregateParity)
                .put(
                    "global",
                    JSONObject()
                        .put("query_count", queries.length())
                        .put(
                            "structure_inside_rect_candidates",
                            structureInsideRectCandidates,
                        )
                        .put(
                            "censor_inside_rect_candidates",
                            censorInsideRectCandidates,
                        )
                        .put(
                            "censor_boundary_inside_rect_candidates",
                            boundaryInsideRectCandidates,
                        )
                        .put(
                            "center_zero_structure_candidates_r8_r16_r32_r64",
                            JSONArray()
                                .put(centerZeroCounts[0])
                                .put(centerZeroCounts[1])
                                .put(centerZeroCounts[2])
                                .put(centerZeroCounts[3]),
                        )
                        .put(
                            "nearest_center_structure_distance_px",
                            distanceSummary(nearestCenterStructure),
                        )
                        .put(
                            "nearest_rect_structure_distance_px",
                            distanceSummary(nearestRectStructure),
                        ),
                )
                .put("queries", compactQueries)
                .put("distance_binding_available", true)
                .put("exact_sample_coordinates_recorded", true)
                .put("sample_grid_evidence_only", true)
                .put("unsampled_pixels_inferred", false)
                .put("scalar_probability_created", false)
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
    ): JSONObject {
        require(frontsideV01 != null) {
            "Frontside v0.1 ontbreekt."
        }
        require(
            frontsideV01.optString("schema") ==
                "D.RAW/Frontside/DarkChromaStability/0.1"
        ) {
            "Frontside v0.1 schema mismatch."
        }
        require(
            frontsideV01.optString("source_sha256") == expectedSourceSha256
        ) {
            "Frontside/source SHA mismatch."
        }

        val analysisWidth = frontsideV01.optInt("analysis_width", 0)
        val analysisHeight = frontsideV01.optInt("analysis_height", 0)
        require(analysisWidth > 0 && analysisHeight > 0) {
            "Frontside analyse-afmetingen ontbreken."
        }

        val candidateData = buildCandidateData(frontsideV01)
        require(candidateData.isNotEmpty()) {
            "Geen zichtbare Dark-Chroma-kandidaten om te exporteren."
        }

        val statusText =
            resolver.openFileDescriptor(sourceUri, "r")?.use { src ->
                resolver.openFileDescriptor(destinationUri, "rw")?.use { dst ->
                    TruthNegativeN2SupportDistanceBridge.exportAndVerify(
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
        require(status.optInt("status", -999) == 0) {
            "Native support-distance export faalde: " +
                status.optString("message", "status=" + status.optInt("status"))
        }
        require(status.optBoolean("postWriteVerified", false)) {
            "Support-distance export is niet post-write geverifieerd."
        }
        require(status.optString("sourceSha256") == expectedSourceSha256) {
            "Support-distance export source-SHA mismatch."
        }
        require(status.optBoolean("exactSampleCoordinatesRecorded", false)) {
            "Exacte sampled coördinaten ontbreken in de sidecar."
        }
        require(!status.optBoolean("unsampledPixelsInferred", true)) {
            "Sidecar claimt onbemeten pixels."
        }
        require(!status.optBoolean("canReduceProtection", true)) {
            "Sidecar mag bescherming niet verminderen."
        }
        require(!status.optBoolean("canEnableCorrection", true)) {
            "Sidecar mag correctie niet inschakelen."
        }
        require(!status.optBoolean("scientificWritebackAllowed", true)) {
            "Sidecar mag geen Scientific-Master-writeback toestaan."
        }
        return status
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

    private fun distanceSummary(values: List<Double>): JSONObject {
        if (values.isEmpty()) {
            return JSONObject()
                .put("count", 0)
                .put("min", JSONObject.NULL)
                .put("median", JSONObject.NULL)
                .put("max", JSONObject.NULL)
        }
        val sorted = values.sorted()
        val median =
            if (sorted.size % 2 == 1) {
                sorted[sorted.size / 2]
            } else {
                0.5 * (
                    sorted[sorted.size / 2 - 1] +
                        sorted[sorted.size / 2]
                    )
            }
        return JSONObject()
            .put("count", sorted.size)
            .put("min", sorted.first())
            .put("median", median)
            .put("max", sorted.last())
    }
}
