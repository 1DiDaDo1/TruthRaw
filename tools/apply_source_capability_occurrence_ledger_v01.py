#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PARSER = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/DngContainerMetadataParser.kt"
ENVELOPE = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/SourceCapabilityEnvelopeV02.kt"
PROFILER = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/UniversalSourceProfiler.kt"


def replace_once(text: str, old: str, new: str, label: str) -> str:
    if new in text:
        return text
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one old block, found {count}")
    return text.replace(old, new, 1)


def patch_parser() -> None:
    text = PARSER.read_text(encoding="utf-8")
    text = replace_once(
        text,
        '            .put("parser", "truthraw.classic-tiff-dng-container-metadata.v0.54")',
        '            .put("parser", "truthraw.classic-tiff-dng-container-metadata.v0.55-occurrence-ledger")',
        "parser version",
    )
    text = replace_once(
        text,
        '''    private data class Entry(\n        val tag: Int,\n        val type: Int,\n        val count: Long,\n        val valueOffset: Long,\n        val inlineBytes: ByteArray,\n    )''',
        '''    private data class Entry(\n        val occurrenceIndex: Int,\n        val entryOffset: Long,\n        val valueFieldOffset: Long,\n        val tag: Int,\n        val type: Int,\n        val count: Long,\n        val valueOffset: Long,\n        val inlineBytes: ByteArray,\n    )''',
        "entry provenance fields",
    )
    text = replace_once(
        text,
        '''        repeat(count) {\n            val eb = ByteBuffer.wrap(block, p, 12).slice().order(order)\n            val tag = u16(eb.short)\n            val type = u16(eb.short)\n            val itemCount = u32(eb.int)\n            val valueField = ByteArray(4)\n            eb.get(valueField)\n            val valueOffset = u32(ByteBuffer.wrap(valueField).order(order).int)\n            entries += Entry(tag, type, itemCount, valueOffset, valueField)\n            p += 12\n        }''',
        '''        repeat(count) { occurrenceIndex ->\n            val entryOffset = offset + 2L + occurrenceIndex.toLong() * 12L\n            val valueFieldOffset = entryOffset + 8L\n            val eb = ByteBuffer.wrap(block, p, 12).slice().order(order)\n            val tag = u16(eb.short)\n            val type = u16(eb.short)\n            val itemCount = u32(eb.int)\n            val valueField = ByteArray(4)\n            eb.get(valueField)\n            val valueOffset = u32(ByteBuffer.wrap(valueField).order(order).int)\n            entries +=\n                Entry(\n                    occurrenceIndex = occurrenceIndex,\n                    entryOffset = entryOffset,\n                    valueFieldOffset = valueFieldOffset,\n                    tag = tag,\n                    type = type,\n                    count = itemCount,\n                    valueOffset = valueOffset,\n                    inlineBytes = valueField,\n                )\n            p += 12\n        }''',
        "physical entry offsets",
    )
    text = replace_once(
        text,
        '''        for (entry in entries) {\n            val decoded = decodeEntry(channel, fileSize, order, entry)\n            val tagJson = JSONObject()\n                .put("tag", entry.tag)\n                .put("name", tagNames[entry.tag] ?: "TAG_${entry.tag}")\n                .put("type", entry.type)\n                .put("typeName", typeNames[entry.type] ?: "TYPE_${entry.type}")\n                .put("count", entry.count)\n                .put("valueByteCount", safeMul(entry.count, typeSize(entry.type).toLong()))\n                .put("value", decoded)\n            entryJson.put(tagJson)''',
        '''        for (entry in entries) {\n            val decoded = decodeEntry(channel, fileSize, order, entry)\n            val unitSize = typeSize(entry.type)\n            val valueByteCount =\n                if (unitSize > 0) safeMul(entry.count, unitSize.toLong()) else -1L\n            val storageKind =\n                when {\n                    unitSize <= 0 || valueByteCount < 0L -> "UNKNOWN"\n                    valueByteCount <= 4L -> "INLINE"\n                    else -> "EXTERNAL"\n                }\n            val externalEndExclusive =\n                if (\n                    storageKind == "EXTERNAL" &&\n                    entry.valueOffset <= fileSize &&\n                    valueByteCount >= 0L &&\n                    valueByteCount <= fileSize - entry.valueOffset\n                ) {\n                    entry.valueOffset + valueByteCount\n                } else {\n                    null\n                }\n            val locatorStatus =\n                when {\n                    unitSize <= 0 -> "LOCATED_UNSUPPORTED_TYPE"\n                    valueByteCount < 0L -> "LOCATED_BYTE_COUNT_OVERFLOW"\n                    storageKind == "EXTERNAL" && externalEndExclusive == null ->\n                        "LOCATED_EXTERNAL_RANGE_OUT_OF_BOUNDS"\n                    else -> "LOCATED"\n                }\n            val decodeStatus =\n                when {\n                    unitSize <= 0 -> "UNSUPPORTED_TYPE"\n                    valueByteCount < 0L -> "BYTE_COUNT_OVERFLOW"\n                    storageKind == "EXTERNAL" && externalEndExclusive == null ->\n                        "EXTERNAL_RANGE_OUT_OF_BOUNDS"\n                    valueByteCount > MAX_METADATA_VALUE_BYTES -> "NOT_DECODED_TOO_LARGE"\n                    else -> "DECODED"\n                }\n            val tagJson = JSONObject()\n                .put("tag", entry.tag)\n                .put("name", tagNames[entry.tag] ?: "TAG_${entry.tag}")\n                .put("type", entry.type)\n                .put("typeName", typeNames[entry.type] ?: "TYPE_${entry.type}")\n                .put("count", entry.count)\n                .put("valueByteCount", valueByteCount)\n                .put("occurrenceIndex", entry.occurrenceIndex)\n                .put("entryOffset", entry.entryOffset)\n                .put("entryByteLength", 12)\n                .put("valueFieldOffset", entry.valueFieldOffset)\n                .put("valueFieldByteLength", 4)\n                .put(\n                    "originalValueOrOffsetBytesHex",\n                    entry.inlineBytes.joinToString("") {\n                        "%02x".format(it.toInt() and 0xff)\n                    },\n                )\n                .put("storageKind", storageKind)\n                .put("locatorStatus", locatorStatus)\n                .put("parseStatus", "PARSED_CLASSIC_TIFF_ENTRY")\n                .put("decodeStatus", decodeStatus)\n                .put(\n                    "externalValueOffset",\n                    if (storageKind == "EXTERNAL") entry.valueOffset else JSONObject.NULL,\n                )\n                .put(\n                    "externalValueEndExclusive",\n                    externalEndExclusive ?: JSONObject.NULL,\n                )\n                .put("value", decoded)\n            entryJson.put(tagJson)''',
        "entry evidence export",
    )
    PARSER.write_text(text, encoding="utf-8")


def patch_envelope() -> None:
    text = ENVELOPE.read_text(encoding="utf-8")
    text = replace_once(
        text,
        '''        val rejectionReason: String? = null,\n        val consumers: List<String> = emptyList(),\n    ) {''',
        '''        val rejectionReason: String? = null,\n        val consumers: List<String> = emptyList(),\n        val sourceByteLength: Long? = null,\n        val ifdPath: String? = null,\n        val ifdOffset: Long? = null,\n        val occurrenceIndex: Int? = null,\n        val entryOffset: Long? = null,\n        val valueFieldOffset: Long? = null,\n        val storageKind: String? = null,\n        val valueByteCount: Long? = null,\n        val externalValueOffset: Long? = null,\n        val externalValueEndExclusive: Long? = null,\n        val originalValueOrOffsetBytesHex: String? = null,\n        val locatorStatus: String? = null,\n        val parseStatus: String? = null,\n        val decodeStatus: String? = null,\n    ) {''',
        "record occurrence fields",
    )
    text = replace_once(
        text,
        '''            require(itemCount >= 0L) { "itemCount must be non-negative" }\n            if (lifecycle == Lifecycle.USED) {''',
        '''            require(itemCount >= 0L) { "itemCount must be non-negative" }\n            require(sourceByteLength == null || sourceByteLength >= 0L) {\n                "sourceByteLength must be non-negative when present"\n            }\n            require(occurrenceIndex == null || occurrenceIndex >= 0) {\n                "occurrenceIndex must be non-negative when present"\n            }\n            require(entryOffset == null || entryOffset >= 0L) {\n                "entryOffset must be non-negative when present"\n            }\n            require(valueFieldOffset == null || valueFieldOffset >= 0L) {\n                "valueFieldOffset must be non-negative when present"\n            }\n            require(\n                originalValueOrOffsetBytesHex == null ||\n                    originalValueOrOffsetBytesHex.matches(Regex("[0-9a-fA-F]{8}")),\n            ) { "classic TIFF value/offset field must be exactly four source bytes" }\n            if (lifecycle == Lifecycle.USED) {''',
        "record occurrence invariants",
    )
    text = replace_once(
        text,
        '''                rejectionReason ?: "",\n                consumers.sorted().joinToString(","),\n            ).joinToString("\\u001f")''',
        '''                rejectionReason ?: "",\n                consumers.sorted().joinToString(","),\n                sourceByteLength?.toString() ?: "",\n                ifdPath ?: "",\n                ifdOffset?.toString() ?: "",\n                occurrenceIndex?.toString() ?: "",\n                entryOffset?.toString() ?: "",\n                valueFieldOffset?.toString() ?: "",\n                storageKind ?: "",\n                valueByteCount?.toString() ?: "",\n                externalValueOffset?.toString() ?: "",\n                externalValueEndExclusive?.toString() ?: "",\n                originalValueOrOffsetBytesHex?.lowercase() ?: "",\n                locatorStatus ?: "",\n                parseStatus ?: "",\n                decodeStatus ?: "",\n            ).joinToString("\\u001f")''',
        "digest occurrence binding",
    )
    text = replace_once(
        text,
        '''                .put("consumers", JSONArray(consumers.sorted()))\n                .put("creates_new_evidence", false)''',
        '''                .put("consumers", JSONArray(consumers.sorted()))\n                .put("source_byte_length", sourceByteLength ?: JSONObject.NULL)\n                .put("ifd_path", ifdPath ?: JSONObject.NULL)\n                .put("ifd_offset", ifdOffset ?: JSONObject.NULL)\n                .put("occurrence_index", occurrenceIndex ?: JSONObject.NULL)\n                .put("entry_offset", entryOffset ?: JSONObject.NULL)\n                .put("value_field_offset", valueFieldOffset ?: JSONObject.NULL)\n                .put("storage_kind", storageKind ?: JSONObject.NULL)\n                .put("value_byte_count", valueByteCount ?: JSONObject.NULL)\n                .put("external_value_offset", externalValueOffset ?: JSONObject.NULL)\n                .put(\n                    "external_value_end_exclusive",\n                    externalValueEndExclusive ?: JSONObject.NULL,\n                )\n                .put(\n                    "original_value_or_offset_bytes_hex",\n                    originalValueOrOffsetBytesHex?.lowercase() ?: JSONObject.NULL,\n                )\n                .put("locator_status", locatorStatus ?: JSONObject.NULL)\n                .put("parse_status", parseStatus ?: JSONObject.NULL)\n                .put("decode_status", decodeStatus ?: JSONObject.NULL)\n                .put("creates_new_evidence", false)''',
        "record json occurrence export",
    )
    text = replace_once(
        text,
        '''        val parserId =\n            parsedContainerMetadata.optString("parser")\n                .ifBlank { "UNKNOWN_PARSER" }\n        val records = ArrayList<Record>()''',
        '''        val parserId =\n            parsedContainerMetadata.optString("parser")\n                .ifBlank { "UNKNOWN_PARSER" }\n        val sourceByteLength =\n            parsedContainerMetadata.optLong("fileSizeBytes", -1L).takeIf { it >= 0L }\n        val records = ArrayList<Record>()''',
        "source byte length binding",
    )
    text = replace_once(
        text,
        '''            val ifdPath = ifd.optString("path", "IFD[$i]")\n            val entries = ifd.optJSONArray("entries") ?: JSONArray()''',
        '''            val ifdPath = ifd.optString("path", "IFD[$i]")\n            val ifdOffset = ifd.optLong("offset", -1L).takeIf { it >= 0L }\n            val entries = ifd.optJSONArray("entries") ?: JSONArray()''',
        "ifd offset binding",
    )
    text = replace_once(
        text,
        '''                val value = entry.opt("value")\n                val classification = classify(name)\n                records +=\n                    Record(\n                        sourceSha256 = sourceSha256,\n                        sourceLocation = "$ifdPath/tag:${tag ?: -1}",''',
        '''                val value = entry.opt("value")\n                val occurrenceIndex =\n                    entry.optInt("occurrenceIndex", j).takeIf { it >= 0 } ?: j\n                val entryOffset =\n                    entry.optLong("entryOffset", -1L).takeIf { it >= 0L }\n                val valueFieldOffset =\n                    entry.optLong("valueFieldOffset", -1L).takeIf { it >= 0L }\n                val valueByteCount =\n                    entry.optLong("valueByteCount", -1L).takeIf { it >= 0L }\n                val externalValueOffset =\n                    entry.optLong("externalValueOffset", -1L).takeIf { it >= 0L }\n                val externalValueEndExclusive =\n                    entry.optLong("externalValueEndExclusive", -1L).takeIf { it >= 0L }\n                val originalValueOrOffsetBytesHex =\n                    entry.optString("originalValueOrOffsetBytesHex", "")\n                        .takeIf { it.matches(Regex("[0-9a-fA-F]{8}")) }\n                val location =\n                    if (entryOffset != null) {\n                        "$ifdPath/entry@$entryOffset/tag:${tag ?: -1}"\n                    } else {\n                        "$ifdPath/entry[$occurrenceIndex]/tag:${tag ?: -1}"\n                    }\n                val classification = classify(name)\n                records +=\n                    Record(\n                        sourceSha256 = sourceSha256,\n                        sourceLocation = location,''',
        "unique occurrence location",
    )
    text = replace_once(
        text,
        '''                        limitation = classification.limitation,\n                        rejectionReason = classification.rejectionReason,\n                    )''',
        '''                        limitation = classification.limitation,\n                        rejectionReason = classification.rejectionReason,\n                        sourceByteLength = sourceByteLength,\n                        ifdPath = ifdPath,\n                        ifdOffset = ifdOffset,\n                        occurrenceIndex = occurrenceIndex,\n                        entryOffset = entryOffset,\n                        valueFieldOffset = valueFieldOffset,\n                        storageKind =\n                            entry.optString("storageKind", "").takeIf { it.isNotBlank() },\n                        valueByteCount = valueByteCount,\n                        externalValueOffset = externalValueOffset,\n                        externalValueEndExclusive = externalValueEndExclusive,\n                        originalValueOrOffsetBytesHex = originalValueOrOffsetBytesHex,\n                        locatorStatus =\n                            entry.optString("locatorStatus", "").takeIf { it.isNotBlank() },\n                        parseStatus =\n                            entry.optString("parseStatus", "").takeIf { it.isNotBlank() },\n                        decodeStatus =\n                            entry.optString("decodeStatus", "").takeIf { it.isNotBlank() },\n                    )''',
        "record physical occurrence values",
    )
    text = replace_once(
        text,
        '''        return buildEnvelope(sourceSha256, parserId, records)''',
        '''        return buildEnvelope(sourceSha256, sourceByteLength, parserId, records)''',
        "envelope byte length call",
    )
    text = replace_once(
        text,
        '''    private fun buildEnvelope(\n        sourceSha256: String,\n        parserId: String,\n        records: List<Record>,\n    ): JSONObject {''',
        '''    private fun buildEnvelope(\n        sourceSha256: String,\n        sourceByteLength: Long?,\n        parserId: String,\n        records: List<Record>,\n    ): JSONObject {''',
        "envelope signature",
    )
    text = replace_once(
        text,
        '''            .put("parser_id", parserId)\n            .put("ledger_schema", LEDGER_SCHEMA)''',
        '''            .put("parser_id", parserId)\n            .put("source_byte_length", sourceByteLength ?: JSONObject.NULL)\n            .put("physical_occurrence_identity", "IFD_PATH_PLUS_EXACT_ENTRY_OFFSET")\n            .put("duplicate_tags_preserved_as_distinct_occurrences", true)\n            .put("ledger_schema", LEDGER_SCHEMA)''',
        "envelope occurrence declaration",
    )
    ENVELOPE.write_text(text, encoding="utf-8")


def patch_profiler() -> None:
    text = PROFILER.read_text(encoding="utf-8")
    text = replace_once(
        text,
        '''        val ifds = parsed.optJSONArray("ifds") ?: JSONArray()''',
        '''        progress?.invoke("SOURCE_CAPABILITY_EVIDENCE_LEDGER_V02")\n        val sourceCapabilityEvidenceLedger =\n            SourceCapabilityEnvelopeV02.describe(\n                sourceSha256 = sourceSha256,\n                parsedContainerMetadata = parsed,\n            )\n\n        val ifds = parsed.optJSONArray("ifds") ?: JSONArray()''',
        "profiler exact occurrence ledger",
    )
    text = replace_once(
        text,
        '''            .put("container_metadata", parsed)\n            .put("source_metadata", metadata)''',
        '''            .put("container_metadata", parsed)\n            .put("source_capability_envelope_v0_2", sourceCapabilityEvidenceLedger)\n            .put("source_metadata", metadata)''',
        "profiler shadow ledger output",
    )
    PROFILER.write_text(text, encoding="utf-8")


def main() -> None:
    patch_parser()
    patch_envelope()
    patch_profiler()
    print("SOURCE_CAPABILITY_OCCURRENCE_LEDGER_V01_PATCH_APPLIED")


if __name__ == "__main__":
    main()
