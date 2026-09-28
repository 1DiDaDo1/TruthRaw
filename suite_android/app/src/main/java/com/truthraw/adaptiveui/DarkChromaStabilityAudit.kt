package com.truthraw.adaptiveui

import android.graphics.Bitmap
import android.graphics.Color
import org.json.JSONArray
import org.json.JSONObject
import java.security.MessageDigest
import java.util.Locale
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.min
import kotlin.math.sqrt

/**
 * Audit-only frontside analysis for one immutable source observation.
 *
 * The purpose is deliberately narrow: identify dark rendered regions where
 * visible chroma varies more strongly than the locally visible structure
 * supports, while protecting edges/textures from frontside-driven correction.
 *
 * This module never changes source bytes, CFA samples, Scientific Master,
 * D.RAWnegative or appearance pixels. It does not claim that a visible colour
 * fluctuation is sensor noise; it only creates APPEARANCE_DERIVED_ONLY
 * constraints that may later be checked against backside/noise evidence.
 *
 * No AI/ML/learned model and no temporal/multi-lens input are used.
 */
object DarkChromaStabilityAudit {
    private const val TILE_EDGE = 16

    // Research-v0.1 frontside heuristics. They are deliberately labelled as
    // appearance-only thresholds and have no calibration/scientific authority.
    private const val DARK_LUMA_MEAN_MAX = 72.0
    private const val DARK_PIXEL_LUMA_MAX = 80
    private const val DARK_PIXEL_FRACTION_MIN = 0.60
    private const val STRUCTURE_EDGE_DENSITY_VETO = 0.080
    private const val STRUCTURE_LUMA_STDDEV_VETO = 24.0
    private const val FLAT_EDGE_DENSITY_MAX = 0.040
    private const val FLAT_LUMA_STDDEV_MAX = 16.0
    private const val CHROMA_STDDEV_CANDIDATE_MIN = 6.0
    private const val CHROMA_TO_LUMA_RATIO_MIN = 0.85
    private const val EDGE_DELTA_THRESHOLD = 16

    fun unavailable(sourceSha256: String): JSONObject =
        JSONObject()
            .put("schema", "D.RAW/Frontside/DarkChromaStability/0.1")
            .put("status", "FRONTSIDE_PREVIEW_UNAVAILABLE")
            .put("source_sha256", sourceSha256)
            .put("authority", "APPEARANCE_DERIVED_ONLY")
            .put("audit_only", true)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
            .put("requires_backside_confirmation", true)
            .put("single_observation_contract", singleObservationContract(sourceSha256))

    fun analyze(
        bitmap: Bitmap,
        sourceSha256: String,
    ): JSONObject {
        val width = bitmap.width
        val height = bitmap.height
        val tiles = JSONArray()
        val digest = MessageDigest.getInstance("SHA-256")

        var tileCount = 0L
        var darkTileCount = 0L
        var flatDarkTileCount = 0L
        var chromaInstabilityCandidateTiles = 0L
        var structureProtectedTiles = 0L
        var darkStructureProtectedTiles = 0L
        var sampledPixels = 0L
        var darkPixels = 0L
        var chromaStddevSum = 0.0
        var lumaStddevSum = 0.0
        var maxChromaStddev = 0.0
        var maxChromaToLumaRatio = 0.0

        var ty = 0
        while (ty < height) {
            var tx = 0
            while (tx < width) {
                val tw = min(TILE_EDGE, width - tx)
                val th = min(TILE_EDGE, height - ty)
                val count = tw * th

                var sumY = 0.0
                var sumY2 = 0.0
                var sumRg = 0.0
                var sumRg2 = 0.0
                var sumBg = 0.0
                var sumBg2 = 0.0
                var tileDarkPixels = 0L
                var gradientPairs = 0L
                var strongGradientPairs = 0L
                var gradientAbsSum = 0.0

                for (y in ty until ty + th) {
                    for (x in tx until tx + tw) {
                        val c = bitmap.getPixel(x, y)
                        val r = Color.red(c)
                        val g = Color.green(c)
                        val b = Color.blue(c)
                        val yy = ((77 * r + 150 * g + 29 * b) shr 8)
                        val rg = (r - g).toDouble()
                        val bg = (b - g).toDouble()

                        sumY += yy
                        sumY2 += yy.toDouble() * yy.toDouble()
                        sumRg += rg
                        sumRg2 += rg * rg
                        sumBg += bg
                        sumBg2 += bg * bg
                        if (yy <= DARK_PIXEL_LUMA_MAX) tileDarkPixels++

                        if (x + 1 < tx + tw) {
                            val c2 = bitmap.getPixel(x + 1, y)
                            val y2 = (
                                77 * Color.red(c2) +
                                    150 * Color.green(c2) +
                                    29 * Color.blue(c2)
                                ) shr 8
                            val d = abs(y2 - yy)
                            gradientPairs++
                            gradientAbsSum += d.toDouble()
                            if (d >= EDGE_DELTA_THRESHOLD) strongGradientPairs++
                        }
                        if (y + 1 < ty + th) {
                            val c2 = bitmap.getPixel(x, y + 1)
                            val y2 = (
                                77 * Color.red(c2) +
                                    150 * Color.green(c2) +
                                    29 * Color.blue(c2)
                                ) shr 8
                            val d = abs(y2 - yy)
                            gradientPairs++
                            gradientAbsSum += d.toDouble()
                            if (d >= EDGE_DELTA_THRESHOLD) strongGradientPairs++
                        }
                    }
                }

                val meanY = if (count > 0) sumY / count else 0.0
                val varY = if (count > 0) max(0.0, sumY2 / count - meanY * meanY) else 0.0
                val lumaStddev = sqrt(varY)
                val meanRg = if (count > 0) sumRg / count else 0.0
                val meanBg = if (count > 0) sumBg / count else 0.0
                val varRg = if (count > 0) max(0.0, sumRg2 / count - meanRg * meanRg) else 0.0
                val varBg = if (count > 0) max(0.0, sumBg2 / count - meanBg * meanBg) else 0.0
                val chromaStddev = sqrt(varRg + varBg)
                val edgeDensity =
                    if (gradientPairs > 0L) strongGradientPairs.toDouble() / gradientPairs else 0.0
                val meanAbsLumaGradient =
                    if (gradientPairs > 0L) gradientAbsSum / gradientPairs else 0.0
                val darkFraction =
                    if (count > 0) tileDarkPixels.toDouble() / count.toDouble() else 0.0
                val chromaToLumaRatio = chromaStddev / max(1.0, lumaStddev)

                val darkEligible =
                    meanY <= DARK_LUMA_MEAN_MAX &&
                        darkFraction >= DARK_PIXEL_FRACTION_MIN
                val structureProtected =
                    edgeDensity >= STRUCTURE_EDGE_DENSITY_VETO ||
                        lumaStddev >= STRUCTURE_LUMA_STDDEV_VETO
                val flatStructure =
                    edgeDensity <= FLAT_EDGE_DENSITY_MAX &&
                        lumaStddev <= FLAT_LUMA_STDDEV_MAX
                val chromaInstabilityCandidate =
                    darkEligible &&
                        flatStructure &&
                        !structureProtected &&
                        chromaStddev >= CHROMA_STDDEV_CANDIDATE_MIN &&
                        chromaToLumaRatio >= CHROMA_TO_LUMA_RATIO_MIN

                tileCount++
                sampledPixels += count.toLong()
                darkPixels += tileDarkPixels
                if (darkEligible) darkTileCount++
                if (darkEligible && flatStructure) flatDarkTileCount++
                if (structureProtected) structureProtectedTiles++
                if (darkEligible && structureProtected) darkStructureProtectedTiles++
                if (chromaInstabilityCandidate) chromaInstabilityCandidateTiles++
                chromaStddevSum += chromaStddev
                lumaStddevSum += lumaStddev
                maxChromaStddev = max(maxChromaStddev, chromaStddev)
                maxChromaToLumaRatio = max(maxChromaToLumaRatio, chromaToLumaRatio)

                val tile = JSONObject()
                    .put("x", tx)
                    .put("y", ty)
                    .put("width", tw)
                    .put("height", th)
                    .put("sample_count", count)
                    .put("mean_luma_0_255", meanY)
                    .put("luma_stddev", lumaStddev)
                    .put("dark_pixel_fraction", darkFraction)
                    .put("mean_rg_opponent", meanRg)
                    .put("mean_bg_opponent", meanBg)
                    .put("chroma_opponent_stddev", chromaStddev)
                    .put("chroma_to_luma_stddev_ratio", chromaToLumaRatio)
                    .put("edge_density", edgeDensity)
                    .put("mean_abs_luma_gradient", meanAbsLumaGradient)
                    .put("dark_eligible", darkEligible)
                    .put("flat_structure", flatStructure)
                    .put("structure_protected", structureProtected)
                    .put("frontside_chroma_instability_candidate", chromaInstabilityCandidate)
                    .put("candidate_requires_backside_confirmation", chromaInstabilityCandidate)
                    .put("candidate_applied", false)
                tiles.put(tile)

                // Stable diagnostic digest. The quantized text deliberately
                // avoids locale-dependent decimal formatting.
                val digestLine = buildString {
                    append(tx).append(',')
                    append(ty).append(',')
                    append(tw).append(',')
                    append(th).append(',')
                    append(Math.round(meanY * 1000.0)).append(',')
                    append(Math.round(lumaStddev * 1000.0)).append(',')
                    append(Math.round(chromaStddev * 1000.0)).append(',')
                    append(Math.round(edgeDensity * 1_000_000.0)).append(',')
                    append(if (darkEligible) 1 else 0).append(',')
                    append(if (flatStructure) 1 else 0).append(',')
                    append(if (structureProtected) 1 else 0).append(',')
                    append(if (chromaInstabilityCandidate) 1 else 0)
                    append('\n')
                }
                digest.update(digestLine.toByteArray(Charsets.UTF_8))

                tx += TILE_EDGE
            }
            ty += TILE_EDGE
        }

        val auditSha256 = digest.digest().joinToString("") { "%02x".format(Locale.US, it.toInt() and 0xff) }

        return JSONObject()
            .put("schema", "D.RAW/Frontside/DarkChromaStability/0.1")
            .put("status", "AUDIT_ONLY_AVAILABLE")
            .put("source_sha256", sourceSha256)
            .put("authority", "APPEARANCE_DERIVED_ONLY")
            .put("analysis_width", width)
            .put("analysis_height", height)
            .put("tile_edge", TILE_EDGE)
            .put("audit_sha256", auditSha256)
            .put(
                "global",
                JSONObject()
                    .put("tile_count", tileCount)
                    .put("sampled_pixels", sampledPixels)
                    .put("dark_pixels", darkPixels)
                    .put("dark_tile_count", darkTileCount)
                    .put("flat_dark_tile_count", flatDarkTileCount)
                    .put("structure_protected_tiles", structureProtectedTiles)
                    .put("dark_structure_protected_tiles", darkStructureProtectedTiles)
                    .put("frontside_chroma_instability_candidate_tiles", chromaInstabilityCandidateTiles)
                    .put(
                        "mean_chroma_opponent_stddev",
                        if (tileCount > 0L) chromaStddevSum / tileCount.toDouble() else 0.0,
                    )
                    .put(
                        "mean_luma_stddev",
                        if (tileCount > 0L) lumaStddevSum / tileCount.toDouble() else 0.0,
                    )
                    .put("max_chroma_opponent_stddev", maxChromaStddev)
                    .put("max_chroma_to_luma_stddev_ratio", maxChromaToLumaRatio),
            )
            .put(
                "heuristic_thresholds",
                JSONObject()
                    .put("authority", "RESEARCH_APPEARANCE_HEURISTIC_ONLY")
                    .put("dark_luma_mean_max", DARK_LUMA_MEAN_MAX)
                    .put("dark_pixel_luma_max", DARK_PIXEL_LUMA_MAX)
                    .put("dark_pixel_fraction_min", DARK_PIXEL_FRACTION_MIN)
                    .put("structure_edge_density_veto", STRUCTURE_EDGE_DENSITY_VETO)
                    .put("structure_luma_stddev_veto", STRUCTURE_LUMA_STDDEV_VETO)
                    .put("flat_edge_density_max", FLAT_EDGE_DENSITY_MAX)
                    .put("flat_luma_stddev_max", FLAT_LUMA_STDDEV_MAX)
                    .put("chroma_stddev_candidate_min", CHROMA_STDDEV_CANDIDATE_MIN)
                    .put("chroma_to_luma_ratio_min", CHROMA_TO_LUMA_RATIO_MIN)
                    .put("edge_delta_threshold", EDGE_DELTA_THRESHOLD),
            )
            .put(
                "constraint_bundle",
                JSONObject()
                    .put("structure_constraint", "FRONTSIDE_VETO_ONLY")
                    .put("chroma_constraint", "VISIBLE_CHROMA_INSTABILITY_CANDIDATE_ONLY")
                    .put("noise_constraint", "NO_SENSOR_NOISE_CLAIM_FROM_FRONTSIDE")
                    .put("replacement_colour_estimated", false)
                    .put("pixel_value_replacement_proposed", false)
                    .put("requires_backside_noise_model_or_measurement", true)
                    .put("bindable_to_same_observation_drawnegative", true)
                    .put("binding_role", "D.RAWNEGATIVE_DIAGNOSTIC_CONSTRAINT_ONLY"),
            )
            .put("single_observation_contract", singleObservationContract(sourceSha256))
            .put("tiles", tiles)
            .put("uses_ai_or_learned_model", false)
            .put("audit_only", true)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun singleObservationContract(sourceSha256: String): JSONObject =
        JSONObject()
            .put("source_observation_count", 1)
            .put("selected_source_sha256", sourceSha256)
            .put("other_physical_lenses_used", false)
            .put("temporal_frames_used", false)
            .put("burst_used", false)
            .put("multi_observation_fusion_allowed", false)
            .put("frontside_views_derived_from_same_observation_only", true)
            .put("drawnegative_target_count", 1)
}
