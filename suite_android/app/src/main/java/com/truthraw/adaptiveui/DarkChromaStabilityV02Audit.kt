package com.truthraw.adaptiveui

import android.graphics.Bitmap
import android.graphics.Color
import org.json.JSONArray
import org.json.JSONObject
import java.security.MessageDigest
import java.util.Locale
import kotlin.math.abs
import kotlin.math.ln
import kotlin.math.max
import kotlin.math.min
import kotlin.math.sqrt

/**
 * Versioned successor above immutable Dark Chroma Stability v0.1.
 *
 * v0.2 does not redefine the v0.1 frontside candidate. It binds to the exact
 * v0.1 result and adds an information-support gate so an almost black,
 * structure-poor/noise-dominant frontside cannot be interpreted as if the
 * hidden scene colour were known.
 *
 * A v0.2 frontside result may say:
 * - CHROMA_INSTABILITY_VISIBLE
 * - DARK_UNINFORMATIVE
 * - STRUCTURE_PROTECTED
 * - NO_CHROMA_INSTABILITY
 *
 * It can never, by itself, say CHROMA_CORRECTION_SUPPORTED. That state requires
 * later local backside/noise evidence bound to the same source observation.
 *
 * No source/Scientific-Master/D.RAWnegative pixels are changed.
 * No AI/ML/learned model, burst, temporal frame or other physical lens is used.
 */
object DarkChromaStabilityV02Audit {
    private const val GLOBAL_DARK_TILE_FRACTION_MIN = 0.95
    private const val GLOBAL_LOW_INFORMATION_ENTROPY_MAX = 1.50
    private const val GLOBAL_LOW_INFORMATION_EDGE_DENSITY_MAX = 0.005
    private const val LOCAL_MEAN_LUMA_INFORMATION_MIN = 8.0
    private const val LOCAL_LUMA_STDDEV_INFORMATION_MIN = 6.0

    fun unavailable(
        sourceSha256: String,
        v01: JSONObject?,
        backsideSupport: JSONObject?,
    ): JSONObject =
        baseResult(
            sourceSha256 = sourceSha256,
            status = "FRONTSIDE_PREVIEW_UNAVAILABLE",
            v01 = v01,
            backsideSupport = backsideSupport,
        )
            .put("global_information_state", "UNKNOWN")
            .put("global_dark_uninformative", false)
            .put("tiles", JSONArray())

    fun analyze(
        bitmap: Bitmap,
        sourceSha256: String,
        v01: JSONObject,
        backsideSupport: JSONObject?,
    ): JSONObject {
        val v01Source = v01.optString("source_sha256")
        val v01Schema = v01.optString("schema")
        val v01Global = v01.optJSONObject("global") ?: JSONObject()
        val v01Tiles = v01.optJSONArray("tiles") ?: JSONArray()

        val exactV01Binding =
            v01Source == sourceSha256 &&
                v01Schema == "D.RAW/Frontside/DarkChromaStability/0.1"

        val globalFrontside = measureGlobalFrontside(bitmap)
        val tileCount = v01Global.optLong("tile_count", 0L)
        val darkTileCount = v01Global.optLong("dark_tile_count", 0L)
        val darkTileFraction =
            if (tileCount > 0L) darkTileCount.toDouble() / tileCount.toDouble() else 0.0

        val globalDarkUninformative =
            exactV01Binding &&
                darkTileFraction >= GLOBAL_DARK_TILE_FRACTION_MIN &&
                globalFrontside.entropyBits <= GLOBAL_LOW_INFORMATION_ENTROPY_MAX &&
                globalFrontside.edgeDensity <= GLOBAL_LOW_INFORMATION_EDGE_DENSITY_MAX

        var structureProtectedTiles = 0L
        var visibleInstabilityTiles = 0L
        var darkUninformativeTiles = 0L
        var backsidePendingTiles = 0L
        var correctionSupportedTiles = 0L
        var noInstabilityTiles = 0L

        val outTiles = JSONArray()
        val digest = MessageDigest.getInstance("SHA-256")

        for (i in 0 until v01Tiles.length()) {
            val t = v01Tiles.optJSONObject(i) ?: continue
            val structureProtected = t.optBoolean("structure_protected", false)
            val darkEligible = t.optBoolean("dark_eligible", false)
            val visibleCandidate =
                t.optBoolean("frontside_chroma_instability_candidate", false)
            val meanLuma = t.optDouble("mean_luma_0_255", Double.NaN)
            val lumaStddev = t.optDouble("luma_stddev", Double.NaN)

            val localSignalSupport =
                meanLuma.isFinite() &&
                    lumaStddev.isFinite() &&
                    (
                        meanLuma >= LOCAL_MEAN_LUMA_INFORMATION_MIN ||
                            lumaStddev >= LOCAL_LUMA_STDDEV_INFORMATION_MIN
                        )

            val darkUninformative =
                darkEligible &&
                    !structureProtected &&
                    (
                        globalDarkUninformative ||
                            !localSignalSupport
                        )

            val informationState = when {
                structureProtected -> "STRUCTURE_PROTECTED"
                darkUninformative -> "DARK_UNINFORMATIVE"
                visibleCandidate -> "CHROMA_INSTABILITY_VISIBLE"
                else -> "NO_CHROMA_INSTABILITY"
            }

            val backsideLocalSupportBound = false
            val chromaCorrectionSupported =
                visibleCandidate &&
                    !darkUninformative &&
                    !structureProtected &&
                    localSignalSupport &&
                    backsideLocalSupportBound

            if (structureProtected) structureProtectedTiles++
            if (visibleCandidate) visibleInstabilityTiles++
            if (darkUninformative) darkUninformativeTiles++
            if (
                visibleCandidate &&
                !darkUninformative &&
                !structureProtected &&
                !backsideLocalSupportBound
            ) {
                backsidePendingTiles++
            }
            if (chromaCorrectionSupported) correctionSupportedTiles++
            if (!visibleCandidate) noInstabilityTiles++

            val out = JSONObject()
                .put("x", t.optInt("x"))
                .put("y", t.optInt("y"))
                .put("width", t.optInt("width"))
                .put("height", t.optInt("height"))
                .put("v0_1_candidate", visibleCandidate)
                .put("structure_protected", structureProtected)
                .put("dark_eligible", darkEligible)
                .put("local_signal_support", localSignalSupport)
                .put("information_state", informationState)
                .put("dark_uninformative", darkUninformative)
                .put("backside_local_support_bound", backsideLocalSupportBound)
                .put(
                    "correction_support_state",
                    when {
                        structureProtected -> "BLOCKED_BY_STRUCTURE"
                        darkUninformative -> "BLOCKED_DARK_UNINFORMATIVE"
                        !visibleCandidate -> "NOT_REQUESTED"
                        else -> "BACKSIDE_CONFIRMATION_PENDING"
                    },
                )
                .put("chroma_correction_supported", chromaCorrectionSupported)
                .put("candidate_applied", false)
            outTiles.put(out)

            val line = buildString {
                append(out.optInt("x")).append(',')
                append(out.optInt("y")).append(',')
                append(if (visibleCandidate) 1 else 0).append(',')
                append(if (structureProtected) 1 else 0).append(',')
                append(if (localSignalSupport) 1 else 0).append(',')
                append(if (darkUninformative) 1 else 0).append(',')
                append(informationState).append(',')
                append(if (chromaCorrectionSupported) 1 else 0).append('\n')
            }
            digest.update(line.toByteArray(Charsets.UTF_8))
        }

        val auditSha256 =
            digest.digest().joinToString("") {
                "%02x".format(Locale.US, it.toInt() and 0xff)
            }

        return baseResult(
            sourceSha256 = sourceSha256,
            status =
                if (exactV01Binding) {
                    "AUDIT_ONLY_AVAILABLE"
                } else {
                    "FAIL_CLOSED_V0_1_BINDING_MISMATCH"
                },
            v01 = v01,
            backsideSupport = backsideSupport,
        )
            .put("exact_v0_1_binding_verified", exactV01Binding)
            .put("analysis_width", bitmap.width)
            .put("analysis_height", bitmap.height)
            .put("audit_sha256", auditSha256)
            .put("global_dark_tile_fraction", darkTileFraction)
            .put("global_luma_entropy_bits_32_bin", globalFrontside.entropyBits)
            .put("global_edge_density", globalFrontside.edgeDensity)
            .put(
                "global_information_state",
                if (globalDarkUninformative) {
                    "DARK_UNINFORMATIVE"
                } else {
                    "FRONTSIDE_INFORMATION_PRESENT"
                },
            )
            .put("global_dark_uninformative", globalDarkUninformative)
            .put(
                "global",
                JSONObject()
                    .put("tile_count", tileCount)
                    .put("structure_protected_tiles", structureProtectedTiles)
                    .put("visible_chroma_instability_tiles", visibleInstabilityTiles)
                    .put("dark_uninformative_tiles", darkUninformativeTiles)
                    .put("backside_confirmation_pending_tiles", backsidePendingTiles)
                    .put("chroma_correction_supported_tiles", correctionSupportedTiles)
                    .put("no_chroma_instability_tiles", noInstabilityTiles),
            )
            .put(
                "information_support_thresholds",
                JSONObject()
                    .put("authority", "RESEARCH_APPEARANCE_HEURISTIC_ONLY")
                    .put("global_dark_tile_fraction_min", GLOBAL_DARK_TILE_FRACTION_MIN)
                    .put(
                        "global_low_information_entropy_max",
                        GLOBAL_LOW_INFORMATION_ENTROPY_MAX,
                    )
                    .put(
                        "global_low_information_edge_density_max",
                        GLOBAL_LOW_INFORMATION_EDGE_DENSITY_MAX,
                    )
                    .put(
                        "local_mean_luma_information_min",
                        LOCAL_MEAN_LUMA_INFORMATION_MIN,
                    )
                    .put(
                        "local_luma_stddev_information_min",
                        LOCAL_LUMA_STDDEV_INFORMATION_MIN,
                    ),
            )
            .put(
                "correction_contract",
                JSONObject()
                    .put(
                        "required_intersection",
                        "V0_1_VISIBLE_CHROMA_INSTABILITY_AND_INFORMATION_SUPPORT_AND_NO_STRUCTURE_VETO_AND_LOCAL_BACKSIDE_NOISE_SUPPORT",
                    )
                    .put("frontside_can_prove_sensor_noise", false)
                    .put("frontside_can_choose_replacement_colour", false)
                    .put("backside_metadata_hint_is_local_noise_confirmation", false)
                    .put("local_backside_support_bound", false)
                    .put("chroma_correction_supported", false)
                    .put("private_ab_delta_allowed_only_after_local_backside_binding", true),
            )
            .put("tiles", outTiles)
    }

    private fun baseResult(
        sourceSha256: String,
        status: String,
        v01: JSONObject?,
        backsideSupport: JSONObject?,
    ): JSONObject {
        val noiseProfilePresent =
            backsideSupport?.optBoolean("noise_profile_present", false) ?: false
        val blackLevelPresent =
            backsideSupport?.optBoolean("black_level_present", false) ?: false
        val whiteLevelPresent =
            backsideSupport?.optBoolean("white_level_present", false) ?: false

        return JSONObject()
            .put("schema", "D.RAW/Frontside/DarkChromaStability/0.2")
            .put("status", status)
            .put("source_sha256", sourceSha256)
            .put("authority", "APPEARANCE_DERIVED_ONLY")
            .put(
                "v0_1_binding",
                JSONObject()
                    .put(
                        "schema",
                        v01?.optString("schema", "UNKNOWN") ?: "UNKNOWN",
                    )
                    .put(
                        "audit_sha256",
                        v01?.optString("audit_sha256", "") ?: "",
                    )
                    .put("source_sha256", v01?.optString("source_sha256", "") ?: ""),
            )
            .put(
                "backside_support",
                JSONObject()
                    .put("authority", "SOURCE_METADATA_BOUND_HINT_ONLY")
                    .put("noise_profile_present", noiseProfilePresent)
                    .put("black_level_present", blackLevelPresent)
                    .put("white_level_present", whiteLevelPresent)
                    .put("local_noise_confirmation_available", false)
                    .put("n2_local_support_bound", false)
                    .put("can_support_correction_by_itself", false),
            )
            .put(
                "single_observation_contract",
                JSONObject()
                    .put("source_observation_count", 1)
                    .put("selected_source_sha256", sourceSha256)
                    .put("other_physical_lenses_used", false)
                    .put("temporal_frames_used", false)
                    .put("burst_used", false)
                    .put("multi_observation_fusion_allowed", false)
                    .put("frontside_views_derived_from_same_observation_only", true)
                    .put("drawnegative_target_count", 1),
            )
            .put("uses_ai_or_learned_model", false)
            .put("audit_only", true)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
            .put("replacement_colour_estimated", false)
            .put("pixel_value_replacement_proposed", false)
    }

    private data class GlobalFrontsideMetrics(
        val entropyBits: Double,
        val edgeDensity: Double,
    )

    private fun measureGlobalFrontside(bitmap: Bitmap): GlobalFrontsideMetrics {
        val width = bitmap.width
        val height = bitmap.height
        val n = width * height
        if (n <= 0) return GlobalFrontsideMetrics(0.0, 0.0)

        val luma = IntArray(n)
        val histogram = IntArray(32)
        var sum = 0.0
        var sum2 = 0.0
        var idx = 0

        for (y in 0 until height) {
            for (x in 0 until width) {
                val c = bitmap.getPixel(x, y)
                val yy = (
                    77 * Color.red(c) +
                        150 * Color.green(c) +
                        29 * Color.blue(c)
                    ) shr 8
                luma[idx++] = yy
                histogram[min(31, yy / 8)]++
                sum += yy
                sum2 += yy.toDouble() * yy.toDouble()
            }
        }

        var entropy = 0.0
        for (count in histogram) {
            if (count <= 0) continue
            val p = count.toDouble() / n.toDouble()
            entropy -= p * (ln(p) / ln(2.0))
        }

        val mean = sum / n.toDouble()
        val variance = max(0.0, sum2 / n.toDouble() - mean * mean)
        val stddev = sqrt(variance)
        val edgeThreshold = max(18.0, stddev * 0.55)

        var edgeCount = 0L
        var interiorCount = 0L
        if (width >= 3 && height >= 3) {
            for (y in 1 until height - 1) {
                for (x in 1 until width - 1) {
                    val center = y * width + x
                    val gx = (luma[center + 1] - luma[center - 1]).toDouble()
                    val gy = (luma[center + width] - luma[center - width]).toDouble()
                    val mag = sqrt(gx * gx + gy * gy)
                    interiorCount++
                    if (mag >= edgeThreshold) edgeCount++
                }
            }
        }

        val edgeDensity =
            if (interiorCount > 0L) {
                edgeCount.toDouble() / interiorCount.toDouble()
            } else {
                0.0
            }

        return GlobalFrontsideMetrics(entropy, edgeDensity)
    }
}
