package com.draw.geometrycapture

import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.io.RandomAccessFile

/**
 * Universal, read-only source profiler.
 *
 * It never uses a device-specific map to decide scientific truth. It extracts what
 * the source container actually says, derives only conservative route hints, and
 * leaves everything else UNKNOWN.
 */
object UniversalSourceProfiler {

    fun profile(file: File, displayName: String, sourceSha256: String): JSONObject {
        val base = JSONObject()
            .put("schema", "D.RAW/UniversalSourceProfile/0.1")
            .put("status", "AUTO_PROFILED")
            .put("source_sha256", sourceSha256)
            .put("display_name", displayName)
            .put("byte_length", file.length())
            .put("device_specific_mapping_used", false)
            .put("unknown_is_valid", true)
            .put("extensions", JSONObject())

        val sniff = sniffContainer(file)
        base.put("container_sniff", sniff)

        if (sniff.optString("family") != "CLASSIC_TIFF") {
            return base
                .put("scientific_source_class", "OPAQUE_RAW_OR_IMAGE_CONTAINER")
                .put("metadata_parse_status", "NOT_CLASSIC_TIFF")
                .put("source_metadata", JSONObject())
                .put("route_hints", JSONArray().put("REQUIRE_FORMAT_ADAPTER_OR_GENERIC_DECODER_QUARANTINE"))
                .put("scene_analysis", FrontsideSceneInspector.inspect(file, null, sourceSha256))
                .put("authority", authorityBlock())
        }

        val parsed = try {
            RandomAccessFile(file, "r").use { raf ->
                DngContainerMetadataParser.parse(raf.channel, raf.length())
            }
        } catch (e: Exception) {
            return base
                .put("scientific_source_class", "TIFF_CONTAINER_METADATA_PARSE_FAILED")
                .put("metadata_parse_status", "FAILED")
                .put("metadata_error", e.message ?: e.javaClass.simpleName)
                .put("source_metadata", JSONObject())
                .put("route_hints", JSONArray().put("FAIL_CLOSED_OR_VERSIONED_COMPATIBILITY_ADAPTER"))
                .put("scene_analysis", FrontsideSceneInspector.inspect(file, null, sourceSha256))
                .put("authority", authorityBlock())
        }

        val ifds = parsed.optJSONArray("ifds") ?: JSONArray()
        val rawCandidates = parsed.optJSONArray("rawCfaIfdCandidates") ?: JSONArray()

        val make = findTagValue(ifds, "Make")
        val model = findTagValue(ifds, "Model")
        val uniqueCameraModel = findTagValue(ifds, "UniqueCameraModel")
        val software = findTagValue(ifds, "Software")
        val dateTime = findTagValue(ifds, "DateTime")
        val dateTimeOriginal = findTagValue(ifds, "DateTimeOriginal")
        val dateTimeDigitized = findTagValue(ifds, "DateTimeDigitized")
        val subSecTime = findTagValue(ifds, "SubSecTime")
        val subSecTimeOriginal = findTagValue(ifds, "SubSecTimeOriginal")
        val subSecTimeDigitized = findTagValue(ifds, "SubSecTimeDigitized")
        val offsetTime = findTagValue(ifds, "OffsetTime")
        val offsetTimeOriginal = findTagValue(ifds, "OffsetTimeOriginal")
        val offsetTimeDigitized = findTagValue(ifds, "OffsetTimeDigitized")
        val orientation = numberValue(findTagValue(ifds, "Orientation"))?.toInt()
        val exposureTime = rationalValue(findTagValue(ifds, "ExposureTime"))
        val fNumber = rationalValue(findTagValue(ifds, "FNumber"))
        val iso = numberValue(findTagValue(ifds, "ISOSpeedRatings"))
        val focalLength = rationalValue(findTagValue(ifds, "FocalLength"))
        val dngVersion = findTagValue(ifds, "DNGVersion")
        val dngBackward = findTagValue(ifds, "DNGBackwardVersion")
        val asShotNeutral = findTagValue(ifds, "AsShotNeutral")
        val colorMatrix1 = findTagValue(ifds, "ColorMatrix1")
        val colorMatrix2 = findTagValue(ifds, "ColorMatrix2")
        val forwardMatrix1 = findTagValue(ifds, "ForwardMatrix1")
        val forwardMatrix2 = findTagValue(ifds, "ForwardMatrix2")
        val noiseProfile = findTagValue(ifds, "NoiseProfile")

        val primaryRaw = largestRawCandidate(rawCandidates)
        val width = numberValue(primaryRaw?.opt("imageWidth"))?.toLong()
        val height = numberValue(primaryRaw?.opt("imageLength"))?.toLong()

        val metadata = JSONObject()
            .put("make", valueOrNull(make))
            .put("model", valueOrNull(model))
            .put("unique_camera_model", valueOrNull(uniqueCameraModel))
            .put("software", valueOrNull(software))
            .put("date_time", valueOrNull(dateTime))
            .put("date_time_original", valueOrNull(dateTimeOriginal))
            .put("date_time_digitized", valueOrNull(dateTimeDigitized))
            .put("subsec_time", valueOrNull(subSecTime))
            .put("subsec_time_original", valueOrNull(subSecTimeOriginal))
            .put("subsec_time_digitized", valueOrNull(subSecTimeDigitized))
            .put("offset_time", valueOrNull(offsetTime))
            .put("offset_time_original", valueOrNull(offsetTimeOriginal))
            .put("offset_time_digitized", valueOrNull(offsetTimeDigitized))
            .put("capture_time_preferred_text", valueOrNull(dateTimeOriginal ?: dateTime))
            .put("capture_subsec_preferred_text", valueOrNull(subSecTimeOriginal ?: subSecTime))
            .put("capture_offset_preferred_text", valueOrNull(offsetTimeOriginal ?: offsetTime))
            .put("orientation", orientation ?: JSONObject.NULL)
            .put("dng_version", valueOrNull(dngVersion))
            .put("dng_backward_version", valueOrNull(dngBackward))
            .put("exposure_time_seconds", exposureTime ?: JSONObject.NULL)
            .put("f_number", fNumber ?: JSONObject.NULL)
            .put("iso", iso ?: JSONObject.NULL)
            .put("focal_length_mm", focalLength ?: JSONObject.NULL)
            .put("as_shot_neutral", valueOrNull(asShotNeutral))
            .put("color_matrix_1_present", colorMatrix1 != null)
            .put("color_matrix_2_present", colorMatrix2 != null)
            .put("forward_matrix_1_present", forwardMatrix1 != null)
            .put("forward_matrix_2_present", forwardMatrix2 != null)
            .put("noise_profile_present", noiseProfile != null)

        val raster = JSONObject()
            .put("raw_cfa_candidate_count", rawCandidates.length())
            .put("width", width ?: JSONObject.NULL)
            .put("height", height ?: JSONObject.NULL)
            .put("bits_per_sample", valueOrNull(primaryRaw?.opt("bitsPerSample")))
            .put("compression", valueOrNull(primaryRaw?.opt("compression")))
            .put("samples_per_pixel", valueOrNull(primaryRaw?.opt("samplesPerPixel")))
            .put("cfa_repeat_pattern_dim", valueOrNull(primaryRaw?.opt("cfaRepeatPatternDim")))
            .put("cfa_pattern", valueOrNull(primaryRaw?.opt("cfaPattern")))
            .put("black_level", valueOrNull(primaryRaw?.opt("blackLevel")))
            .put("white_level", valueOrNull(primaryRaw?.opt("whiteLevel")))
            .put("active_area", valueOrNull(primaryRaw?.opt("activeArea")))
            .put("default_crop_size", valueOrNull(primaryRaw?.opt("defaultCropSize")))

        val sourceClass = when {
            rawCandidates.length() > 0 -> "DNG_CFA_RAW"
            dngVersion != null || uniqueCameraModel != null -> "DNG_NON_CFA_OR_UNSUPPORTED_RAW_LAYOUT"
            else -> "TIFF_IMAGE_OR_UNKNOWN"
        }

        val routeHints = JSONArray()
        if (rawCandidates.length() > 0) {
            routeHints.put("TRY_DIRECT_COMMON_DNG_SCIENTIFIC_INGRESS")
        }
        if (orientation != null && orientation !in 1..8) {
            routeHints.put("ORIENTATION_METADATA_INVALID_CHECK_VERSIONED_COMPATIBILITY_INGRESS")
        }
        if (rawCandidates.length() == 0) {
            routeHints.put("DO_NOT_ASSUME_CFA_SCIENTIFIC_SOURCE")
        }

        val sourceIdentityHint = JSONObject()
            .put("make", valueOrNull(make))
            .put("model", valueOrNull(model))
            .put("unique_camera_model", valueOrNull(uniqueCameraModel))
            .put("authority", "SOURCE_METADATA_BOUND_NOT_DEVICE_CALIBRATION")

        val optics = JSONObject()
            .put("focal_length_mm", focalLength ?: JSONObject.NULL)
            .put("f_number", fNumber ?: JSONObject.NULL)
            .put("lens_role", "UNKNOWN")
            .put("lens_role_authority", "UNKNOWN")
            .put("field_of_view", "UNKNOWN")
            .put("optical_support", "UNKNOWN")
            .put("note", "Focal length alone does not prove lens role or field of view across different sensor formats.")

        val frontside = FrontsideSceneInspector.inspect(
            file,
            parsed,
            sourceSha256
        )

        return base
            .put("scientific_source_class", sourceClass)
            .put("metadata_parse_status", "PASS_READ_ONLY")
            .put("container_metadata", parsed)
            .put("source_metadata", metadata)
            .put("primary_raw_raster", raster)
            .put("source_identity_hint", sourceIdentityHint)
            .put("optics", optics)
            .put("route_hints", routeHints)
            .put("scene_analysis", frontside)
            .put("authority", authorityBlock())
            .put(
                "open_world",
                JSONObject()
                    .put("source_self_describes_when_possible", true)
                    .put("unknown_fields_remain_unknown", true)
                    .put("future_format_adapters_allowed", true)
                    .put("future_scene_models_allowed", true)
                    .put("sealed_source_does_not_seal_interpretation", true)
            )
    }

    fun pairProfile(a: JSONObject, b: JSONObject): JSONObject {
        val aMeta = a.optJSONObject("source_metadata") ?: JSONObject()
        val bMeta = b.optJSONObject("source_metadata") ?: JSONObject()
        val aOptics = a.optJSONObject("optics") ?: JSONObject()
        val bOptics = b.optJSONObject("optics") ?: JSONObject()

        val aDevice = normalizedDevice(aMeta)
        val bDevice = normalizedDevice(bMeta)
        val sameDeviceHint = aDevice != null && bDevice != null && aDevice == bDevice

        val fa = aOptics.optDouble("focal_length_mm", Double.NaN)
        val fb = bOptics.optDouble("focal_length_mm", Double.NaN)

        val focalOrdering = when {
            fa.isFinite() && fb.isFinite() && fa < fb -> "A_SHORTER_FOCAL_LENGTH_THAN_B"
            fa.isFinite() && fb.isFinite() && fb < fa -> "B_SHORTER_FOCAL_LENGTH_THAN_A"
            fa.isFinite() && fb.isFinite() -> "EQUAL_REPORTED_FOCAL_LENGTH"
            else -> "UNKNOWN"
        }

        val aScene = a.optJSONObject("scene_analysis") ?: JSONObject()
        val bScene = b.optJSONObject("scene_analysis") ?: JSONObject()
        val aReady = aScene.optJSONObject("geometry_readiness")
            ?.optBoolean("natural_feature_geometry_candidate", false) ?: false
        val bReady = bScene.optJSONObject("geometry_readiness")
            ?.optBoolean("natural_feature_geometry_candidate", false) ?: false

        val aAspect = aScene.optJSONObject("proportions")
            ?.optDouble("aspect_ratio_width_over_height", Double.NaN)
            ?: Double.NaN
        val bAspect = bScene.optJSONObject("proportions")
            ?.optDouble("aspect_ratio_width_over_height", Double.NaN)
            ?: Double.NaN

        val aspectAgreement = if (aAspect.isFinite() && bAspect.isFinite()) {
            kotlin.math.abs(aAspect - bAspect) /
                kotlin.math.max(aAspect, bAspect) <= 0.03
        } else {
            false
        }

        val geometryRoute = when {
            aReady && bReady ->
                "NATURAL_FEATURE_PAIR_MATCHING_CANDIDATE"
            else ->
                "SCIENTIFIC_RECONSTRUCTION_THEN_SCENE_INSPECTION"
        }

        return JSONObject()
            .put("schema", "D.RAW/UniversalSourcePairProfile/0.1")
            .put("same_device_metadata_hint", sameDeviceHint)
            .put("same_device_hint_authority", "SOURCE_METADATA_HINT_ONLY")
            .put("reported_focal_length_ordering", focalOrdering)
            .put("focal_ordering_is_field_of_view_authority", false)
            .put("frontside_a_available", aScene.optBoolean("decoded_preview_used", false))
            .put("frontside_b_available", bScene.optBoolean("decoded_preview_used", false))
            .put("frontside_natural_feature_candidate_a", aReady)
            .put("frontside_natural_feature_candidate_b", bReady)
            .put("frontside_aspect_ratio_compatible_hint", aspectAgreement)
            .put("frontside_hint_authority", "APPEARANCE_DERIVED_ONLY")
            .put("role_assignment", "DEFER_TO_SCENE_AND_SOURCE_EVIDENCE")
            .put("geometry_route", geometryRoute)
            .put("indexed_target_required", false)
            .put("metric_scale", "UNKNOWN_UNLESS_SOURCE_OR_SCENE_SUPPLIES_SCALE")
            .put("device_specific_mapping_used", false)
            .put("extensions", JSONObject())
    }

    private fun sceneAnalysisPlaceholder(): JSONObject {
        return JSONObject()
            .put("status", "DEFERRED_UNTIL_IMAGE_CONTENT_AVAILABLE_IN_SCIENTIFIC_PIPELINE")
            .put("natural_feature_geometry_allowed", true)
            .put("indexed_target_required", false)
            .put("metric_target_required_for_relative_geometry", false)
            .put("absolute_metric_scale_requires_scale_evidence", true)
            .put("scene_model_not_hard_coded", true)
    }

    private fun authorityBlock(): JSONObject {
        return JSONObject()
            .put("container_fields", "SOURCE_METADATA_BOUND")
            .put("source_sha256", "MEASURED")
            .put("lens_role", "UNKNOWN_UNLESS_INDEPENDENTLY_PROVEN")
            .put("scene_geometry", "NOT_YET_MEASURED")
            .put("metric_scale", "UNKNOWN_UNLESS_EVIDENCE_EXISTS")
            .put("creates_new_sensor_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun sniffContainer(file: File): JSONObject {
        if (file.length() < 4L) {
            return JSONObject().put("family", "UNKNOWN").put("reason", "TOO_SHORT")
        }
        val bytes = ByteArray(4)
        RandomAccessFile(file, "r").use { raf ->
            raf.readFully(bytes)
        }
        val littleTiff = bytes[0] == 'I'.code.toByte() && bytes[1] == 'I'.code.toByte() &&
            (bytes[2].toInt() and 0xff) == 42 && (bytes[3].toInt() and 0xff) == 0
        val bigTiff = bytes[0] == 'M'.code.toByte() && bytes[1] == 'M'.code.toByte() &&
            (bytes[2].toInt() and 0xff) == 0 && (bytes[3].toInt() and 0xff) == 42
        return when {
            littleTiff -> JSONObject().put("family", "CLASSIC_TIFF").put("byte_order", "LITTLE_ENDIAN")
            bigTiff -> JSONObject().put("family", "CLASSIC_TIFF").put("byte_order", "BIG_ENDIAN")
            else -> JSONObject()
                .put("family", "UNKNOWN_OR_VENDOR_RAW")
                .put("magic_prefix_hex", bytes.joinToString("") { "%02x".format(it.toInt() and 0xff) })
        }
    }

    private fun largestRawCandidate(array: JSONArray): JSONObject? {
        var best: JSONObject? = null
        var bestArea = -1.0
        for (i in 0 until array.length()) {
            val c = array.optJSONObject(i) ?: continue
            val w = numberValue(c.opt("imageWidth")) ?: continue
            val h = numberValue(c.opt("imageLength")) ?: continue
            val area = w * h
            if (area > bestArea) {
                bestArea = area
                best = c
            }
        }
        return best
    }

    private fun findTagValue(ifds: JSONArray, name: String): Any? {
        for (i in 0 until ifds.length()) {
            val entries = ifds.optJSONObject(i)?.optJSONArray("entries") ?: continue
            for (j in 0 until entries.length()) {
                val e = entries.optJSONObject(j) ?: continue
                if (e.optString("name") == name) {
                    val value = e.opt("value")
                    if (value != null && value !== JSONObject.NULL) return value
                }
            }
        }
        return null
    }

    private fun numberValue(value: Any?): Double? {
        return when (value) {
            is Number -> value.toDouble()
            is JSONObject -> if (value.has("value") && !value.isNull("value")) value.optDouble("value") else null
            else -> null
        }
    }

    private fun rationalValue(value: Any?): Double? = numberValue(value)

    private fun normalizedDevice(metadata: JSONObject): String? {
        val unique = metadata.optString("unique_camera_model", "").trim()
        if (unique.isNotEmpty() && unique != "null") return unique.lowercase()
        val make = metadata.optString("make", "").trim()
        val model = metadata.optString("model", "").trim()
        if (make.isEmpty() && model.isEmpty()) return null
        return (make + "|" + model).lowercase()
    }

    private fun valueOrNull(value: Any?): Any = value ?: JSONObject.NULL
}
