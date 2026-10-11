package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Compatibility shadow projection from fields that UniversalSourceProfiler has
 * already parsed into source_metadata / primary_raw_raster.
 *
 * This is deliberately weaker than the exact IFD/tag ledger produced by
 * SourceCapabilityEnvelopeV02.describe(...). It performs no new source reads,
 * creates no admission, and has zero downstream scientific consumers.
 */
object SourceCapabilityProfileProjectionV01 {
    const val SCHEMA = "D.RAW/SourceCapabilityProfileProjection/0.1"
    private const val PARSER_ID = "D.RAW/UniversalSourceProfile/0.2-PROJECTION"

    fun describe(
        sourceSha256: String,
        metadata: JSONObject?,
        raster: JSONObject?,
    ): JSONObject {
        require(SourceCapabilityEnvelopeV02.isSha256(sourceSha256)) {
            "Invalid source SHA-256 binding"
        }

        val records = ArrayList<SourceCapabilityEnvelopeV02.Record>()
        val m = metadata ?: JSONObject()
        val r = raster ?: JSONObject()

        addSection(
            records = records,
            sourceSha256 = sourceSha256,
            section = "source_metadata",
            obj = m,
        )
        addSection(
            records = records,
            sourceSha256 = sourceSha256,
            section = "primary_raw_raster",
            obj = r,
        )

        val sorted =
            records.sortedWith(
                compareBy<SourceCapabilityEnvelopeV02.Record> { it.sourceLocation }
                    .thenBy { it.semanticKey },
            )
        val jsonRecords = JSONArray()
        sorted.forEach { jsonRecords.put(it.toJson()) }
        val parsedCapabilities =
            sorted.map { it.semanticKey }.distinct().sorted()

        return JSONObject()
            .put("schema", SCHEMA)
            .put("envelope_schema", SourceCapabilityEnvelopeV02.SCHEMA)
            .put("ledger_schema", SourceCapabilityEnvelopeV02.LEDGER_SCHEMA)
            .put("status", SourceCapabilityEnvelopeV02.STATUS)
            .put("source_sha256", sourceSha256.lowercase())
            .put("projection_only", true)
            .put("projection_source", "ALREADY_PARSED_UNIVERSAL_SOURCE_PROFILE_FIELDS")
            .put("exact_ifd_tag_locations_available", false)
            .put("exact_ifd_tag_ledger_required_for_promotion", true)
            .put("parser_id", PARSER_ID)
            .put("record_count", sorted.size)
            .put("ledger_digest_sha256", SourceCapabilityEnvelopeV02.ledgerDigest(sorted))
            .put("parsed_capabilities", JSONArray(parsedCapabilities))
            .put("admitted_capabilities", JSONArray())
            .put("used_capabilities", JSONArray())
            .put("records", jsonRecords)
            .put("contradictions", JSONArray())
            .put("contradiction_detection_status", "NOT_YET_SEMANTICALLY_BOUND_IN_PROFILE_PROJECTION")
            .put("downstream_consumers_enabled", false)
            .put("image_samples_decoded_by_this_layer", false)
            .put("source_values_modified", false)
            .put("measured_authority_created", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
            .put("pixel_route_effect", false)
            .put("scientific_master_pixel_effect", false)
            .put("appearance_pixel_effect", false)
    }

    private fun addSection(
        records: MutableList<SourceCapabilityEnvelopeV02.Record>,
        sourceSha256: String,
        section: String,
        obj: JSONObject,
    ) {
        val keys = mutableListOf<String>()
        val iterator = obj.keys()
        while (iterator.hasNext()) keys += iterator.next()

        for (key in keys.sorted()) {
            val value = obj.opt(key)
            if (value == null || value === JSONObject.NULL) continue
            val classification = classify(section, key)
            records +=
                SourceCapabilityEnvelopeV02.Record(
                    sourceSha256 = sourceSha256,
                    sourceLocation = "UniversalSourceProfile/$section/$key",
                    tag = null,
                    semanticKey = key,
                    parserId = PARSER_ID,
                    parserType = jsonType(value),
                    itemCount = itemCount(value),
                    canonicalValue = canonicalJson(value),
                    lifecycle = SourceCapabilityEnvelopeV02.Lifecycle.PARSED,
                    admission = classification.admission,
                    authority = classification.authority,
                    unitsOrReferenceFrame = classification.units,
                    limitation = classification.limitation,
                    rejectionReason = classification.rejectionReason,
                )
        }
    }

    private data class Classification(
        val admission: SourceCapabilityEnvelopeV02.Admission,
        val authority: SourceCapabilityEnvelopeV02.Authority,
        val units: String = "UNSPECIFIED",
        val limitation: String = "PROFILE_PROJECTION_NOT_INDEPENDENTLY_VALIDATED",
        val rejectionReason: String? = null,
    )

    private fun classify(section: String, key: String): Classification {
        if (section == "source_metadata") {
            return when (key) {
                "make", "model", "unique_camera_model", "software" ->
                    Classification(
                        admission = SourceCapabilityEnvelopeV02.Admission.PROVENANCE_ONLY,
                        authority = SourceCapabilityEnvelopeV02.Authority.SOURCE_IDENTITY_PROVENANCE,
                        limitation = "IDENTITY_MAY_DESCRIBE_PROVENANCE_BUT_MAY_NOT_SELECT_SCIENTIFIC_TRUTH",
                        rejectionReason = "IDENTITY_NOT_A_SCIENTIFIC_CALIBRATION_KEY",
                    )

                "exposure_time_seconds" -> acquisition("SECONDS_WHEN_SOURCE_SEMANTICS_VALIDATE")
                "f_number" -> acquisition("F_NUMBER_WHEN_SOURCE_SEMANTICS_VALIDATE")
                "iso" -> acquisition("ISO_METADATA_CONTEXT_NOT_CALIBRATED_GAIN")
                "focal_length_mm" ->
                    Classification(
                        admission = SourceCapabilityEnvelopeV02.Admission.PRESENT_UNVALIDATED,
                        authority = SourceCapabilityEnvelopeV02.Authority.SOURCE_ACQUISITION_CONTEXT,
                        units = "MILLIMETRES_WHEN_SOURCE_SEMANTICS_VALIDATE",
                        limitation = "FOCAL_LENGTH_ALONE_DOES_NOT_PROVE_FOV_LENS_ROLE_OR_OPTICAL_SUPPORT",
                    )

                "as_shot_neutral",
                "color_matrix_1_present", "color_matrix_2_present",
                "camera_calibration_1_present", "camera_calibration_2_present",
                "forward_matrix_1_present", "forward_matrix_2_present" ->
                    Classification(
                        admission = SourceCapabilityEnvelopeV02.Admission.PRESENT_UNVALIDATED,
                        authority = SourceCapabilityEnvelopeV02.Authority.SOURCE_CALIBRATION_CANDIDATE,
                        limitation = "PRESENCE_OR_VALUE_ONLY_CALIBRATION_DOMAIN_NOT_YET_VALIDATED",
                    )

                "noise_profile_present" ->
                    Classification(
                        admission = SourceCapabilityEnvelopeV02.Admission.PRESENT_UNVALIDATED,
                        authority = SourceCapabilityEnvelopeV02.Authority.SOURCE_NOISE_CONTEXT_CANDIDATE,
                        limitation = "NOISE_PROFILE_PRESENCE_IS_NOT_A_VALIDATED_NOISE_LIKELIHOOD",
                    )

                else ->
                    Classification(
                        admission = SourceCapabilityEnvelopeV02.Admission.PRESENT_UNVALIDATED,
                        authority = SourceCapabilityEnvelopeV02.Authority.SOURCE_ACQUISITION_CONTEXT,
                    )
            }
        }

        return when (key) {
            "opcode_list_2_present", "opcode_list_2_metadata" ->
                Classification(
                    admission = SourceCapabilityEnvelopeV02.Admission.PROVENANCE_ONLY,
                    authority = SourceCapabilityEnvelopeV02.Authority.SOURCE_OPERATION_PROVENANCE,
                    limitation = "DNG_OPCODE_PROFILE_PROJECTION_PROVENANCE_ONLY",
                    rejectionReason = "OPCODE_NOT_SCIENTIFICALLY_ADMITTED_BY_PROFILE_PROJECTION",
                )
            else ->
                Classification(
                    admission = SourceCapabilityEnvelopeV02.Admission.PRESENT_UNVALIDATED,
                    authority = SourceCapabilityEnvelopeV02.Authority.SOURCE_STRUCTURE,
                )
        }
    }

    private fun acquisition(units: String) =
        Classification(
            admission = SourceCapabilityEnvelopeV02.Admission.PRESENT_UNVALIDATED,
            authority = SourceCapabilityEnvelopeV02.Authority.SOURCE_ACQUISITION_CONTEXT,
            units = units,
        )

    private fun itemCount(value: Any): Long =
        when (value) {
            is JSONArray -> value.length().toLong()
            is JSONObject -> {
                var count = 0L
                val iterator = value.keys()
                while (iterator.hasNext()) {
                    iterator.next()
                    count++
                }
                count
            }
            else -> 1L
        }

    private fun jsonType(value: Any): String =
        when (value) {
            is JSONObject -> "JSON_OBJECT"
            is JSONArray -> "JSON_ARRAY"
            is Boolean -> "BOOLEAN"
            is Number -> "NUMBER"
            is String -> "STRING"
            else -> value.javaClass.simpleName.uppercase()
        }

    private fun canonicalJson(value: Any?): String =
        when (value) {
            null, JSONObject.NULL -> "null"
            is JSONObject -> {
                val keys = mutableListOf<String>()
                val iterator = value.keys()
                while (iterator.hasNext()) keys += iterator.next()
                keys.sorted().joinToString(prefix = "{", postfix = "}", separator = ",") { key ->
                    JSONObject.quote(key) + ":" + canonicalJson(value.opt(key))
                }
            }
            is JSONArray ->
                (0 until value.length()).joinToString(prefix = "[", postfix = "]", separator = ",") { index ->
                    canonicalJson(value.opt(index))
                }
            is String -> JSONObject.quote(value)
            is Boolean -> value.toString()
            is Number -> value.toString()
            else -> JSONObject.quote(value.toString())
        }
}
