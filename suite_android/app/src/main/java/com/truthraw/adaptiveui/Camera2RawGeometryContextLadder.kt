package com.truthraw.adaptiveui

import android.util.Size
import org.json.JSONArray
import org.json.JSONObject

/**
 * v0.39 geometry/context ladder.
 *
 * This is deliberately not a vendor-key experiment. It varies only the Camera2 RAW output
 * geometry/context already advertised by physical Camera 5.
 */
object Camera2RawGeometryContextLadder {
    const val TOTAL_CONTEXTS = 3

    data class Profile(
        val index: Int,
        val id: String,
        val width: Int,
        val height: Int,
        val routeClass: String,
        val useMaximumResolutionPixelMode: Boolean,
    ) {
        val nominalBytesU16: Long get() = width.toLong() * height.toLong() * 2L
        val label: String get() =
            "C${index + 1}/3 · ${id} · ${width}×${height} · " +
                if (useMaximumResolutionPixelMode) "MAX" else "STANDARD"
    }

    private val profiles = listOf(
        Profile(
            index = 0,
            id = "STANDARD_12P5MP",
            width = 4080,
            height = 3072,
            routeClass = "STANDARD_MAP_OUTPUT",
            useMaximumResolutionPixelMode = false,
        ),
        Profile(
            index = 1,
            id = "MAXIMUM_50MP",
            width = 8160,
            height = 6144,
            routeClass = "MAXIMUM_MAP_OUTPUT",
            useMaximumResolutionPixelMode = true,
        ),
        Profile(
            index = 2,
            id = "MAXIMUM_HIGH_200MP",
            width = 16320,
            height = 12288,
            routeClass = "MAXIMUM_MAP_HIGH_RESOLUTION",
            useMaximumResolutionPixelMode = true,
        ),
    )

    fun profile(index: Int): Profile {
        require(index in profiles.indices) { "context index $index outside 0..${profiles.lastIndex}" }
        return profiles[index]
    }

    fun allProfiles(): List<Profile> = profiles

    fun nextIndex(index: Int): Int = (index + 1) % profiles.size

    fun containsExact(sizes: List<Size>, profile: Profile): Boolean =
        sizes.any { it.width == profile.width && it.height == profile.height }

    fun validateAdvertisements(
        standardOutput: List<Size>,
        maximumOutput: List<Size>,
        maximumHigh: List<Size>,
    ): JSONObject {
        val checks = JSONArray()
        profiles.forEach { p ->
            val advertised = when (p.routeClass) {
                "STANDARD_MAP_OUTPUT" -> containsExact(standardOutput, p)
                "MAXIMUM_MAP_OUTPUT" -> containsExact(maximumOutput, p)
                "MAXIMUM_MAP_HIGH_RESOLUTION" -> containsExact(maximumHigh, p)
                else -> false
            }
            checks.put(
                JSONObject()
                    .put("index", p.index)
                    .put("id", p.id)
                    .put("width", p.width)
                    .put("height", p.height)
                    .put("routeClass", p.routeClass)
                    .put("useMaximumResolutionPixelMode", p.useMaximumResolutionPixelMode)
                    .put("nominalBytesU16", p.nominalBytesU16)
                    .put("advertisedOnExpectedRoute", advertised),
            )
        }

        val allPresent = (0 until checks.length()).all {
            checks.getJSONObject(it).optBoolean("advertisedOnExpectedRoute", false)
        }

        return JSONObject()
            .put("schema", "truthraw.camera2-raw-geometry-context-ladder.v0.39")
            .put("experiment", "ADVERTISED_RAW_OUTPUT_CONTEXT_GEOMETRY_LADDER")
            .put("vendorKeysWritten", 0)
            .put("vendorNameSemanticsUsed", false)
            .put("contexts", checks)
            .put("allRequiredContextsAdvertised", allPresent)
            .put("semanticPromotionAllowed", false)
            .put("authority", "CAMERA2_ADVERTISEMENT_AND_CAPTURE_CONTEXT_ONLY")
    }

    fun profileEvidence(profile: Profile): JSONObject =
        JSONObject()
            .put("schema", "truthraw.camera2-raw-geometry-context-profile.v0.39")
            .put("contextIndex", profile.index)
            .put("contextNumber", profile.index + 1)
            .put("contextId", profile.id)
            .put("requestedWidth", profile.width)
            .put("requestedHeight", profile.height)
            .put("routeClass", profile.routeClass)
            .put("useMaximumResolutionPixelMode", profile.useMaximumResolutionPixelMode)
            .put("nominalBytesU16", profile.nominalBytesU16)
            .put("vendorKeysWritten", 0)
            .put("vendorInterventionUsed", false)
            .put("selectionBasis", "EXACT_DEVICE_ADVERTISED_CAMERA2_RAW_SENSOR_CONTEXT")
            .put("semanticPromotionAllowed", false)
}
