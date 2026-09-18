package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Downstream read-only resolver for v0.39.
 *
 * Uses Stage 3.6's already computed row/sample population. It does not read Image/HardwareBuffer,
 * does not write the sealed source, and does not infer optical/sensor-native resolution.
 */
object RawGeometryContextResolver {
    fun resolve(
        rasterAudit: JSONObject,
        requested: Camera2RawGeometryContextLadder.Profile,
    ): JSONObject {
        val rowPopulation = rasterAudit.optJSONObject("rowPopulation")
            ?: error("Stage 3.6 rowPopulation missing")
        val samplePopulation = rasterAudit.optJSONObject("samplePopulation")
            ?: error("Stage 3.6 samplePopulation missing")

        val firstNonZeroRow = rowPopulation.optInt("firstNonZeroRow", -1)
        val lastNonZeroRow = rowPopulation.optInt("lastNonZeroRow", -1)
        val nonZeroRows = rowPopulation.optInt("nonZeroRows", -1)
        val allZeroRows = rowPopulation.optInt("allZeroRows", -1)
        val firstNonZeroByte = samplePopulation.optLong("firstNonZeroByteOffset", -1L)
        val lastNonZeroByte = samplePopulation.optLong("lastNonZeroByteOffset", -1L)

        val rowBytes = requested.width.toLong() * 2L
        val prefixStartsAtZero = firstNonZeroRow == 0 && firstNonZeroByte == 0L
        val rowAlignedPopulatedExtentBytes =
            if (prefixStartsAtZero && lastNonZeroRow >= 0) (lastNonZeroRow.toLong() + 1L) * rowBytes else -1L
        val lastNonZeroSampleExtentBytes =
            if (firstNonZeroByte == 0L && lastNonZeroByte >= 0L) lastNonZeroByte + 2L else -1L

        val matches = JSONArray()
        Camera2RawGeometryContextLadder.allProfiles().forEach { p ->
            matches.put(
                JSONObject()
                    .put("contextId", p.id)
                    .put("width", p.width)
                    .put("height", p.height)
                    .put("nominalBytesU16", p.nominalBytesU16)
                    .put("rowAlignedExtentExactMatch", rowAlignedPopulatedExtentBytes == p.nominalBytesU16)
                    .put("lastNonZeroSampleExtentExactMatch", lastNonZeroSampleExtentBytes == p.nominalBytesU16),
            )
        }

        val exactRowMatches = Camera2RawGeometryContextLadder.allProfiles().filter {
            rowAlignedPopulatedExtentBytes == it.nominalBytesU16
        }

        val requestedEnvelopeBytes = requested.nominalBytesU16
        val allRequestedRowsContainSignal =
            nonZeroRows == requested.height && allZeroRows == 0 && firstNonZeroRow == 0 && lastNonZeroRow == requested.height - 1

        val classification = when {
            !prefixStartsAtZero ->
                "NONZERO_DOMAIN_NOT_PREFIX_ALIGNED__CONTEXT_GEOMETRY_UNRESOLVED"
            exactRowMatches.size == 1 && exactRowMatches.single().index == requested.index && allRequestedRowsContainSignal ->
                "REQUESTED_CONTEXT_FULL_ROW_DOMAIN_POPULATED__BYTE_EXTENT_MATCHES_REQUESTED_CONTEXT"
            exactRowMatches.size == 1 && exactRowMatches.single().index != requested.index ->
                "REQUESTED_CONTEXT_ENVELOPE_CONTAINS_SMALLER_CONTEXT_SIZED_POPULATED_ROW_PREFIX"
            exactRowMatches.size == 1 ->
                "POPULATED_ROW_PREFIX_EXACTLY_MATCHES_ONE_LADDER_CONTEXT"
            exactRowMatches.isEmpty() ->
                "POPULATED_ROW_PREFIX_MATCHES_NO_LADDER_CONTEXT"
            else ->
                "POPULATED_ROW_PREFIX_AMBIGUOUS_ACROSS_LADDER_CONTEXTS"
        }

        val selected = exactRowMatches.singleOrNull()

        return JSONObject()
            .put("schema", "truthraw.raw-geometry-context-resolver.v0.39")
            .put("readOnlyStage36Derived", true)
            .put("sourceModified", false)
            .put("requestedContext", Camera2RawGeometryContextLadder.profileEvidence(requested))
            .put("requestedEnvelopeBytesU16", requestedEnvelopeBytes)
            .put("rowBytes", rowBytes)
            .put("firstNonZeroRow", firstNonZeroRow)
            .put("lastNonZeroRow", lastNonZeroRow)
            .put("nonZeroRows", nonZeroRows)
            .put("allZeroRows", allZeroRows)
            .put("firstNonZeroByteOffset", firstNonZeroByte)
            .put("lastNonZeroByteOffset", lastNonZeroByte)
            .put("prefixStartsAtZero", prefixStartsAtZero)
            .put("rowAlignedPopulatedExtentBytes", rowAlignedPopulatedExtentBytes)
            .put("lastNonZeroSampleExtentBytes", lastNonZeroSampleExtentBytes)
            .put("allRequestedRowsContainSignal", allRequestedRowsContainSignal)
            .put("ladderByteMatches", matches)
            .put("rowAlignedExactMatchCount", exactRowMatches.size)
            .put(
                "selectedRowAlignedContext",
                selected?.let {
                    JSONObject()
                        .put("contextId", it.id)
                        .put("width", it.width)
                        .put("height", it.height)
                        .put("nominalBytesU16", it.nominalBytesU16)
                        .put("interpretationOnly", true)
                } ?: JSONObject.NULL,
            )
            .put("classification", classification)
            .put("sensorNativeGeometryClaimMade", false)
            .put("opticalResolutionClaimMade", false)
            .put("semanticPromotionAllowed", false)
            .put(
                "interpretationBoundary",
                "APP_VISIBLE_ROW_POPULATION_AND_EXACT_BYTE_EXTENT_ONLY__NO_NATIVE_ADC_OR_OPTICAL_RESOLUTION_PROOF",
            )
    }
}
