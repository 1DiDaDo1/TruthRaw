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
        '            .put("parser", "truthraw.classic-tiff-dng-container-metadata.v0.55-occurrence-ledger")',
        '            .put("parser", "truthraw.classic-tiff-dng-container-metadata.v0.56-traversal-status")',
        "parser version",
    )
    text = replace_once(
        text,
        '''        val ifds = JSONArray()\n        val visited = mutableSetOf<Long>()\n        parseIfdRecursive(channel, fileSize, order, firstIfd, "IFD0", 0, visited, ifds)''',
        '''        val ifds = JSONArray()\n        val traversalAnomalies = JSONArray()\n        val visited = mutableSetOf<Long>()\n        parseIfdRecursive(\n            channel,\n            fileSize,\n            order,\n            firstIfd,\n            "IFD0",\n            0,\n            visited,\n            ifds,\n            traversalAnomalies,\n        )\n        val containerTraversalStatus =\n            if (traversalAnomalies.length() == 0) {\n                "COMPLETE_NO_REPORTED_TRAVERSAL_ANOMALY"\n            } else {\n                "ANOMALIES_FAIL_CLOSED"\n            }''',
        "top-level traversal state",
    )
    text = replace_once(
        text,
        '''            .put("firstIfdOffset", firstIfd)\n            .put("ifds", ifds)\n            .put("rawCfaIfdCandidates", rawCandidates)''',
        '''            .put("firstIfdOffset", firstIfd)\n            .put("containerTraversalStatus", containerTraversalStatus)\n            .put("containerTraversalAnomalyCount", traversalAnomalies.length())\n            .put("containerTraversalAnomalies", traversalAnomalies)\n            .put("ifds", ifds)\n            .put("rawCfaIfdCandidates", rawCandidates)''',
        "parser traversal export",
    )
    text = replace_once(
        text,
        '''        depth: Int,\n        visited: MutableSet<Long>,\n        out: JSONArray,\n    ) {\n        if (offset <= 0L || offset >= fileSize || depth > MAX_RECURSION || !visited.add(offset)) return''',
        '''        depth: Int,\n        visited: MutableSet<Long>,\n        out: JSONArray,\n        traversalAnomalies: JSONArray,\n    ) {\n        when {\n            depth > MAX_RECURSION -> {\n                recordTraversalAnomaly(\n                    traversalAnomalies,\n                    path,\n                    offset,\n                    depth,\n                    fileSize,\n                    "RECURSION_LIMIT_EXCEEDED_FAIL_CLOSED",\n                )\n                return\n            }\n            offset <= 0L -> {\n                recordTraversalAnomaly(\n                    traversalAnomalies,\n                    path,\n                    offset,\n                    depth,\n                    fileSize,\n                    "NON_POSITIVE_IFD_OFFSET_FAIL_CLOSED",\n                )\n                return\n            }\n            offset >= fileSize -> {\n                recordTraversalAnomaly(\n                    traversalAnomalies,\n                    path,\n                    offset,\n                    depth,\n                    fileSize,\n                    "IFD_OFFSET_OUT_OF_BOUNDS_FAIL_CLOSED",\n                )\n                return\n            }\n            !visited.add(offset) -> {\n                recordTraversalAnomaly(\n                    traversalAnomalies,\n                    path,\n                    offset,\n                    depth,\n                    fileSize,\n                    "REVISITED_IFD_OFFSET_FAIL_CLOSED",\n                )\n                return\n            }\n        }''',
        "explicit traversal anomalies",
    )
    text = replace_once(
        text,
        '''        subIfdOffsets.forEachIndexed { index, child ->\n            parseIfdRecursive(channel, fileSize, order, child, "$path/SubIFD[$index]", depth + 1, visited, out)\n        }\n        exifIfdOffset?.let { child ->\n            parseIfdRecursive(channel, fileSize, order, child, "$path/ExifIFD", depth + 1, visited, out)\n        }\n        if (nextIfdOffset > 0L) {\n            parseIfdRecursive(channel, fileSize, order, nextIfdOffset, "$path/NextIFD", depth + 1, visited, out)\n        }\n    }''',
        '''        subIfdOffsets.forEachIndexed { index, child ->\n            parseIfdRecursive(\n                channel, fileSize, order, child, "$path/SubIFD[$index]", depth + 1,\n                visited, out, traversalAnomalies,\n            )\n        }\n        exifIfdOffset?.let { child ->\n            parseIfdRecursive(\n                channel, fileSize, order, child, "$path/ExifIFD", depth + 1,\n                visited, out, traversalAnomalies,\n            )\n        }\n        if (nextIfdOffset > 0L) {\n            parseIfdRecursive(\n                channel, fileSize, order, nextIfdOffset, "$path/NextIFD", depth + 1,\n                visited, out, traversalAnomalies,\n            )\n        }\n    }\n\n    private fun recordTraversalAnomaly(\n        out: JSONArray,\n        path: String,\n        offset: Long,\n        depth: Int,\n        fileSize: Long,\n        status: String,\n    ) {\n        out.put(\n            JSONObject()\n                .put("path", path)\n                .put("attemptedOffset", offset)\n                .put("depth", depth)\n                .put("fileSizeBytes", fileSize)\n                .put("status", status)\n                .put("repairAttempted", false)\n                .put("sourceBytesModified", false),\n        )\n    }''',
        "recursive anomaly threading",
    )
    PARSER.write_text(text, encoding="utf-8")


def patch_envelope() -> None:
    text = ENVELOPE.read_text(encoding="utf-8")
    text = replace_once(
        text,
        '''        val records = ArrayList<Record>()\n        val ifds = parsedContainerMetadata.optJSONArray("ifds") ?: JSONArray()''',
        '''        val records = ArrayList<Record>()\n        val traversalStatus =\n            parsedContainerMetadata.optString(\n                "containerTraversalStatus",\n                "UNKNOWN_NOT_REPORTED_BY_PARSER",\n            )\n        val traversalAnomalies =\n            bindTraversalAnomalies(\n                sourceSha256 = sourceSha256,\n                sourceByteLength = sourceByteLength,\n                parsedContainerMetadata = parsedContainerMetadata,\n            )\n        val ifds = parsedContainerMetadata.optJSONArray("ifds") ?: JSONArray()''',
        "read traversal evidence",
    )
    text = replace_once(
        text,
        '''        return buildEnvelope(sourceSha256, sourceByteLength, parserId, records)''',
        '''        return buildEnvelope(\n            sourceSha256,\n            sourceByteLength,\n            parserId,\n            records,\n            traversalStatus,\n            traversalAnomalies,\n        )''',
        "build envelope traversal args",
    )
    text = replace_once(
        text,
        '''            .put("record_count", 0)\n            .put("records", JSONArray())''',
        '''            .put("record_count", 0)\n            .put("records", JSONArray())\n            .put("container_traversal_status", "PARSE_FAILED_FAIL_CLOSED")\n            .put("container_structure_complete", false)\n            .put("container_traversal_anomaly_count", 0)\n            .put("container_traversal_anomalies", JSONArray())\n            .put(\n                "container_parse_failure",\n                JSONObject()\n                    .put("source_sha256", sourceSha256.lowercase())\n                    .put("reason", reason)\n                    .put("admission", Admission.CONTRADICTED_FAIL_CLOSED.name)\n                    .put("creates_new_evidence", false)\n                    .put("scientific_writeback_allowed", false),\n            )''',
        "unavailable parse-failure evidence",
    )
    text = replace_once(
        text,
        '''    private fun buildEnvelope(\n        sourceSha256: String,\n        sourceByteLength: Long?,\n        parserId: String,\n        records: List<Record>,\n    ): JSONObject {''',
        '''    private fun buildEnvelope(\n        sourceSha256: String,\n        sourceByteLength: Long?,\n        parserId: String,\n        records: List<Record>,\n        traversalStatus: String,\n        traversalAnomalies: JSONArray,\n    ): JSONObject {''',
        "build envelope signature",
    )
    text = replace_once(
        text,
        '''        val recordsJson = JSONArray()\n        sorted.forEach { recordsJson.put(it.toJson()) }\n\n        return base(sourceSha256)''',
        '''        val recordsJson = JSONArray()\n        sorted.forEach { recordsJson.put(it.toJson()) }\n        val traversalAnomalyDigest = traversalAnomalyDigest(traversalAnomalies)\n        val ledgerDigest = ledgerDigest(sorted)\n        val observationEvidenceDigest =\n            sha256(\n                "$LEDGER_SCHEMA\\nentries=$ledgerDigest\\ntraversal=$traversalAnomalyDigest",\n            )\n\n        return base(sourceSha256)''',
        "combined evidence digest",
    )
    text = replace_once(
        text,
        '''            .put("duplicate_tags_preserved_as_distinct_occurrences", true)\n            .put("ledger_schema", LEDGER_SCHEMA)\n            .put("ledger_digest_sha256", ledgerDigest(sorted))\n            .put("record_count", sorted.size)''',
        '''            .put("duplicate_tags_preserved_as_distinct_occurrences", true)\n            .put("ledger_schema", LEDGER_SCHEMA)\n            .put("ledger_digest_sha256", ledgerDigest)\n            .put("container_traversal_status", traversalStatus)\n            .put(\n                "container_structure_complete",\n                traversalStatus == "COMPLETE_NO_REPORTED_TRAVERSAL_ANOMALY",\n            )\n            .put("container_traversal_anomaly_count", traversalAnomalies.length())\n            .put("container_traversal_anomaly_digest_sha256", traversalAnomalyDigest)\n            .put("container_traversal_anomalies", traversalAnomalies)\n            .put("observation_evidence_digest_sha256", observationEvidenceDigest)\n            .put("record_count", sorted.size)''',
        "envelope traversal output",
    )
    marker = '''    private fun base(sourceSha256: String): JSONObject =\n'''
    helper = '''    private fun bindTraversalAnomalies(\n        sourceSha256: String,\n        sourceByteLength: Long?,\n        parsedContainerMetadata: JSONObject,\n    ): JSONArray {\n        val source =\n            parsedContainerMetadata.optJSONArray("containerTraversalAnomalies") ?: JSONArray()\n        val canonical = mutableListOf<String>()\n        for (i in 0 until source.length()) {\n            val anomaly = source.optJSONObject(i) ?: continue\n            val bound =\n                JSONObject()\n                    .put("source_sha256", sourceSha256.lowercase())\n                    .put("source_byte_length", sourceByteLength ?: JSONObject.NULL)\n                    .put("path", anomaly.optString("path", "UNKNOWN_IFD_PATH"))\n                    .put("attempted_offset", anomaly.optLong("attemptedOffset", -1L))\n                    .put("depth", anomaly.optInt("depth", -1))\n                    .put("status", anomaly.optString("status", "UNKNOWN_TRAVERSAL_ANOMALY"))\n                    .put("lifecycle", Lifecycle.PARSED.name)\n                    .put("admission", Admission.CONTRADICTED_FAIL_CLOSED.name)\n                    .put("authority", Authority.SOURCE_STRUCTURE.name)\n                    .put("repair_attempted", false)\n                    .put("source_values_modified", false)\n                    .put("creates_new_evidence", false)\n                    .put("scientific_writeback_allowed", false)\n            canonical += canonicalJson(bound)\n        }\n        val out = JSONArray()\n        canonical.sorted().forEach { encoded -> out.put(JSONObject(encoded)) }\n        return out\n    }\n\n    private fun traversalAnomalyDigest(anomalies: JSONArray): String {\n        val lines = mutableListOf<String>()\n        for (i in 0 until anomalies.length()) {\n            lines += canonicalJson(anomalies.opt(i))\n        }\n        return sha256(lines.sorted().joinToString("\\n"))\n    }\n\n'''
    if helper not in text:
        if text.count(marker) != 1:
            raise SystemExit("traversal helpers: base marker not unique")
        text = text.replace(marker, helper + marker, 1)
    ENVELOPE.write_text(text, encoding="utf-8")


def patch_profiler() -> None:
    text = PROFILER.read_text(encoding="utf-8")
    text = replace_once(
        text,
        '''                .put("metadata_parse_status", "FAILED")\n                .put("metadata_error", e.message ?: e.javaClass.simpleName)\n                .put("source_metadata", JSONObject())''',
        '''                .put("metadata_parse_status", "FAILED")\n                .put("metadata_error", e.message ?: e.javaClass.simpleName)\n                .put(\n                    "source_capability_envelope_v0_2",\n                    SourceCapabilityEnvelopeV02.unavailable(\n                        sourceSha256 = sourceSha256,\n                        reason = e.message ?: e.javaClass.simpleName,\n                    ),\n                )\n                .put("source_metadata", JSONObject())''',
        "profiler hard parse fail-closed envelope",
    )
    PROFILER.write_text(text, encoding="utf-8")


def main() -> None:
    patch_parser()
    patch_envelope()
    patch_profiler()
    print("SOURCE_CAPABILITY_TRAVERSAL_STATUS_V01_PATCH_APPLIED")


if __name__ == "__main__":
    main()
