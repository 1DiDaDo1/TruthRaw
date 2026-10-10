package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import java.security.MessageDigest

/**
 * Passive Source Capability Envelope v0.2 / Observation Evidence Ledger v0.1.
 *
 * This layer is intentionally read-only and shadow-only. It describes facts
 * that the existing DNG/TIFF parser already exposed. It does not parse image
 * samples, choose a scientific model, alter Scientific Master pixels, or feed
 * any current reconstruction/Appearance operator.
 *
 * Evidence lifecycle is explicit and monotonic:
 * PRESENT -> PARSED -> VALIDATED -> ADMITTED -> USED.
 *
 * v0.2 starts conservatively: parser facts are PARSED unless an independent
 * validator later upgrades them. Identity/private/unknown semantics remain
 * provenance-only or unsupported and can never become scientific USED merely
 * because a vendor/camera/lens name is recognizable.
 */
object SourceCapabilityEnvelopeV02 {
    const val SCHEMA = "D.RAW/SourceCapabilityEnvelope/0.2"
    const val LEDGER_SCHEMA = "D.RAW/ObservationEvidenceLedger/0.1"
    const val STATUS = "SHADOW_READ_ONLY_ZERO_PIXEL_CHANGE"

    enum class Lifecycle {
        PRESENT,
        PARSED,
        VALIDATED,
        ADMITTED,
        USED,
    }

    enum class Admission {
        PRESENT_UNVALIDATED,
        CONSISTENT_ADMITTED,
        CONTRADICTED_FAIL_CLOSED,
        PROVENANCE_ONLY,
        UNSUPPORTED_SEMANTICS,
        ABSENT,
    }

    enum class Authority {
        SOURCE_STRUCTURE,
        SOURCE_ACQUISITION_CONTEXT,
        SOURCE_CALIBRATION_CANDIDATE,
        SOURCE_NOISE_CONTEXT_CANDIDATE,
        SOURCE_OPERATION_PROVENANCE,
        SOURCE_IDENTITY_PROVENANCE,
        UNKNOWN_SEMANTICS,
    }

    data class Record(
        val sourceSha256: String,
        val sourceLocation: String,
        val tag: Int?,
        val semanticKey: String,
        val parserId: String,
        val parserType: String,
        val itemCount: Long,
        val canonicalValue: String,
        val lifecycle: Lifecycle,
        val admission: Admission,
        val authority: Authority,
        val unitsOrReferenceFrame: String = "UNSPECIFIED",
        val limitation: String = "NOT_INDEPENDENTLY_VALIDATED_IN_V0_2",
        val rejectionReason: String? = null,
        val consumers: List<String> = emptyList(),
    ) {
        init {
            require(isSha256(sourceSha256)) { "sourceSha256 must be 64 lowercase/uppercase hex chars" }
            require(sourceLocation.isNotBlank()) { "sourceLocation must not be blank" }
            require(semanticKey.isNotBlank()) { "semanticKey must not be blank" }
            require(parserId.isNotBlank()) { "parserId must not be blank" }
            require(itemCount >= 0L) { "itemCount must be non-negative" }
            if (lifecycle == Lifecycle.USED) {
                require(admission == Admission.CONSISTENT_ADMITTED) {
                    "USED evidence must already be CONSISTENT_ADMITTED"
                }
                require(consumers.isNotEmpty()) { "USED evidence must name at least one consumer" }
            }
            if (admission == Admission.PROVENANCE_ONLY || admission == Admission.UNSUPPORTED_SEMANTICS) {
                require(lifecycle.ordinal < Lifecycle.ADMITTED.ordinal) {
                    "Provenance/unsupported evidence may not be ADMITTED or USED"
                }
            }
            if (admission == Admission.CONTRADICTED_FAIL_CLOSED) {
                require(lifecycle.ordinal < Lifecycle.ADMITTED.ordinal) {
                    "Contradicted evidence may not be ADMITTED or USED"
                }
            }
        }

        fun canonicalLine(): String =
            listOf(
                sourceSha256.lowercase(),
                sourceLocation,
                tag?.toString() ?: "-",
                semanticKey,
                parserId,
                parserType,
                itemCount.toString(),
                canonicalValue,
                lifecycle.name,
                admission.name,
                authority.name,
                unitsOrReferenceFrame,
                limitation,
                rejectionReason ?: "",
                consumers.sorted().joinToString(","),
            ).joinToString("\u001f")

        fun toJson(): JSONObject =
            JSONObject()
                .put("source_sha256", sourceSha256.lowercase())
                .put("source_location", sourceLocation)
                .put("tag", tag ?: JSONObject.NULL)
                .put("semantic_key", semanticKey)
                .put("parser_id", parserId)
                .put("parser_type", parserType)
                .put("item_count", itemCount)
                .put("canonical_value", canonicalValue)
                .put("lifecycle", lifecycle.name)
                .put("admission", admission.name)
                .put("authority", authority.name)
                .put("units_or_reference_frame", unitsOrReferenceFrame)
                .put("limitation", limitation)
                .put("rejection_reason", rejectionReason ?: JSONObject.NULL)
                .put("consumers", JSONArray(consumers.sorted()))
                .put("creates_new_evidence", false)
                .put("scientific_writeback_allowed", false)
    }

    /**
     * Build a passive ledger from the already parsed classic-TIFF/DNG metadata.
     * No new file reads occur here.
     */
    fun describe(
        sourceSha256: String,
        parsedContainerMetadata: JSONObject,
    ): JSONObject {
        require(isSha256(sourceSha256)) { "Invalid source SHA-256 binding" }

        val parserId =
            parsedContainerMetadata.optString("parser")
                .ifBlank { "UNKNOWN_PARSER" }
        val records = ArrayList<Record>()
        val ifds = parsedContainerMetadata.optJSONArray("ifds") ?: JSONArray()

        for (i in 0 until ifds.length()) {
            val ifd = ifds.optJSONObject(i) ?: continue
            val ifdPath = ifd.optString("path", "IFD[$i]")
            val entries = ifd.optJSONArray("entries") ?: JSONArray()
            for (j in 0 until entries.length()) {
                val entry = entries.optJSONObject(j) ?: continue
                val tag = entry.optInt("tag", -1).takeIf { it >= 0 }
                val name = entry.optString("name", tag?.let { "TAG_$it" } ?: "UNKNOWN_TAG")
                val typeName = entry.optString("typeName", "UNKNOWN_TYPE")
                val count = entry.optLong("count", 0L).coerceAtLeast(0L)
                val value = entry.opt("value")
                val classification = classify(name)
                records +=
                    Record(
                        sourceSha256 = sourceSha256,
                        sourceLocation = "$ifdPath/tag:${tag ?: -1}",
                        tag = tag,
                        semanticKey = name,
                        parserId = parserId,
                        parserType = typeName,
                        itemCount = count,
                        canonicalValue = canonicalJson(value),
                        lifecycle = Lifecycle.PARSED,
                        admission = classification.admission,
                        authority = classification.authority,
                        unitsOrReferenceFrame = classification.units,
                        limitation = classification.limitation,
                        rejectionReason = classification.rejectionReason,
                    )
            }
        }

        return buildEnvelope(sourceSha256, parserId, records)
    }

    fun unavailable(
        sourceSha256: String,
        reason: String,
    ): JSONObject {
        require(isSha256(sourceSha256)) { "Invalid source SHA-256 binding" }
        return base(sourceSha256)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("ledger_digest_sha256", sha256("$LEDGER_SCHEMA|${sourceSha256.lowercase()}|UNAVAILABLE|$reason"))
            .put("record_count", 0)
            .put("records", JSONArray())
            .put("parsed_capabilities", JSONArray())
            .put("admitted_capabilities", JSONArray())
            .put("used_capabilities", JSONArray())
            .put("contradictions", JSONArray())
    }

    internal fun ledgerDigest(records: List<Record>): String {
        val body =
            records
                .map(Record::canonicalLine)
                .sorted()
                .joinToString("\n")
        return sha256("$LEDGER_SCHEMA\n$body")
    }

    internal fun isSha256(value: String): Boolean =
        value.matches(Regex("[0-9a-fA-F]{64}"))

    private data class Classification(
        val admission: Admission,
        val authority: Authority,
        val units: String = "UNSPECIFIED",
        val limitation: String = "PARSED_SOURCE_FACT_NOT_YET_INDEPENDENTLY_VALIDATED",
        val rejectionReason: String? = null,
    )

    private fun classify(name: String): Classification {
        if (name.startsWith("TAG_")) {
            return Classification(
                admission = Admission.UNSUPPORTED_SEMANTICS,
                authority = Authority.UNKNOWN_SEMANTICS,
                limitation = "TAG_PRESENT_BUT_SEMANTIC_MEANING_NOT_ADMITTED",
                rejectionReason = "UNSUPPORTED_OR_UNNAMED_TAG_SEMANTICS",
            )
        }

        return when (name) {
            "Make", "Model", "UniqueCameraModel", "Software" ->
                Classification(
                    admission = Admission.PROVENANCE_ONLY,
                    authority = Authority.SOURCE_IDENTITY_PROVENANCE,
                    limitation = "IDENTITY_MAY_DESCRIBE_PROVENANCE_BUT_MAY_NOT_SELECT_SCIENTIFIC_TRUTH",
                    rejectionReason = "IDENTITY_NOT_A_SCIENTIFIC_CALIBRATION_KEY",
                )

            "StripOffsets", "StripByteCounts", "TileOffsets", "TileByteCounts",
            "JPEGInterchangeFormat", "JPEGInterchangeFormatLength" ->
                Classification(
                    admission = Admission.PROVENANCE_ONLY,
                    authority = Authority.SOURCE_STRUCTURE,
                    limitation = "STORAGE_LOCATION_METADATA_ONLY_NOT_PIXEL_AUTHORITY",
                    rejectionReason = "STORAGE_OFFSETS_NOT_SCIENTIFIC_PIXEL_SEMANTICS",
                )

            "OpcodeList2", "OpcodeList3" ->
                Classification(
                    admission = Admission.PROVENANCE_ONLY,
                    authority = Authority.SOURCE_OPERATION_PROVENANCE,
                    limitation = "OPAQUE_OR_PARTIALLY_PARSED_DNG_OPCODE_PROVENANCE_ONLY",
                    rejectionReason = "OPCODE_NOT_SCIENTIFICALLY_ADMITTED_BY_THIS_LEDGER",
                )

            "ExposureTime" ->
                Classification(
                    admission = Admission.PRESENT_UNVALIDATED,
                    authority = Authority.SOURCE_ACQUISITION_CONTEXT,
                    units = "SECONDS_WHEN_DNG_EXIF_SEMANTICS_VALIDATE",
                )
            "FNumber" ->
                Classification(
                    admission = Admission.PRESENT_UNVALIDATED,
                    authority = Authority.SOURCE_ACQUISITION_CONTEXT,
                    units = "F_NUMBER_WHEN_DNG_EXIF_SEMANTICS_VALIDATE",
                )
            "ISOSpeedRatings" ->
                Classification(
                    admission = Admission.PRESENT_UNVALIDATED,
                    authority = Authority.SOURCE_ACQUISITION_CONTEXT,
                    units = "ISO_METADATA_CONTEXT_NOT_CALIBRATED_GAIN",
                )
            "FocalLength" ->
                Classification(
                    admission = Admission.PRESENT_UNVALIDATED,
                    authority = Authority.SOURCE_ACQUISITION_CONTEXT,
                    units = "MILLIMETRES_WHEN_EXIF_SEMANTICS_VALIDATE",
                    limitation = "FOCAL_LENGTH_ALONE_DOES_NOT_PROVE_FOV_LENS_ROLE_OR_OPTICAL_SUPPORT",
                )

            "ColorMatrix1", "ColorMatrix2", "CameraCalibration1", "CameraCalibration2",
            "ForwardMatrix1", "ForwardMatrix2", "AsShotNeutral",
            "CalibrationIlluminant1", "CalibrationIlluminant2" ->
                Classification(
                    admission = Admission.PRESENT_UNVALIDATED,
                    authority = Authority.SOURCE_CALIBRATION_CANDIDATE,
                    limitation = "CALIBRATION_DOMAIN_AND_DIMENSIONS_MUST_VALIDATE_BEFORE_ADMISSION",
                )

            "NoiseProfile" ->
                Classification(
                    admission = Admission.PRESENT_UNVALIDATED,
                    authority = Authority.SOURCE_NOISE_CONTEXT_CANDIDATE,
                    limitation = "SOURCE_NOISE_METADATA_IS_NOT_YET_A_VALIDATED_NOISE_LIKELIHOOD",
                )

            else ->
                Classification(
                    admission = Admission.PRESENT_UNVALIDATED,
                    authority = Authority.SOURCE_STRUCTURE,
                )
        }
    }

    private fun buildEnvelope(
        sourceSha256: String,
        parserId: String,
        records: List<Record>,
    ): JSONObject {
        val sorted = records.sortedWith(compareBy<Record> { it.sourceLocation }.thenBy { it.semanticKey })
        val parsedCapabilities = sorted.map { it.semanticKey }.distinct().sorted()
        val admitted = sorted.filter { it.lifecycle.ordinal >= Lifecycle.ADMITTED.ordinal }.map { it.semanticKey }.distinct().sorted()
        val used = sorted.filter { it.lifecycle == Lifecycle.USED }.map { it.semanticKey }.distinct().sorted()
        val recordsJson = JSONArray()
        sorted.forEach { recordsJson.put(it.toJson()) }

        return base(sourceSha256)
            .put("status", STATUS)
            .put("parser_id", parserId)
            .put("ledger_schema", LEDGER_SCHEMA)
            .put("ledger_digest_sha256", ledgerDigest(sorted))
            .put("record_count", sorted.size)
            .put("parsed_capabilities", JSONArray(parsedCapabilities))
            .put("admitted_capabilities", JSONArray(admitted))
            .put("used_capabilities", JSONArray(used))
            .put("records", recordsJson)
            .put("contradictions", JSONArray())
            .put("contradiction_detection_status", "NOT_YET_SEMANTICALLY_BOUND_IN_V0_2")
            .put("downstream_consumers_enabled", false)
            .put("pixel_route_effect", false)
            .put("scientific_master_pixel_effect", false)
            .put("appearance_pixel_effect", false)
    }

    private fun base(sourceSha256: String): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("source_sha256", sourceSha256.lowercase())
            .put("evidence_lifecycle", "PRESENT->PARSED->VALIDATED->ADMITTED->USED")
            .put("shadow_read_only", true)
            .put("image_samples_decoded_by_this_layer", false)
            .put("source_values_modified", false)
            .put("measured_authority_created", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
            .put("vendor_identity_may_promote_scientific_authority", false)
            .put("camera_identity_may_promote_scientific_authority", false)
            .put("lens_identity_may_promote_scientific_authority", false)

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

    private fun sha256(value: String): String {
        val digest = MessageDigest.getInstance("SHA-256").digest(value.toByteArray(Charsets.UTF_8))
        return digest.joinToString("") { "%02x".format(it.toInt() and 0xff) }
    }
}
