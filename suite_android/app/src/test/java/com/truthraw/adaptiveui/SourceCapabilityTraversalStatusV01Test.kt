package com.truthraw.adaptiveui

import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.channels.FileChannel
import java.nio.file.Files
import java.nio.file.StandardOpenOption
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class SourceCapabilityTraversalStatusV01Test {
    private val sha = "d".repeat(64)

    @Test
    fun invalidSubIfdOffsetBecomesSourceBoundFailClosedAnomaly() {
        val bytes = ByteArray(26)
        val b = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN)
        b.put(0, 'I'.code.toByte())
        b.put(1, 'I'.code.toByte())
        b.putShort(2, 42.toShort())
        b.putInt(4, 8)
        b.putShort(8, 1.toShort())
        b.position(10)
        b.putShort(330.toShort()) // SubIFDs
        b.putShort(4.toShort()) // LONG
        b.putInt(1)
        b.putInt(1000) // Inline child offset, outside this file.
        b.putInt(22, 0)

        val parsed = parse(bytes)
        assertEquals("ANOMALIES_FAIL_CLOSED", parsed.getString("containerTraversalStatus"))
        assertEquals(1, parsed.getInt("containerTraversalAnomalyCount"))
        assertEquals(1, parsed.getJSONArray("ifds").length())

        val parserAnomaly = parsed.getJSONArray("containerTraversalAnomalies").getJSONObject(0)
        assertEquals("IFD0/SubIFD[0]", parserAnomaly.getString("path"))
        assertEquals(1000L, parserAnomaly.getLong("attemptedOffset"))
        assertEquals("IFD_OFFSET_OUT_OF_BOUNDS_FAIL_CLOSED", parserAnomaly.getString("status"))
        assertFalse(parserAnomaly.getBoolean("repairAttempted"))
        assertFalse(parserAnomaly.getBoolean("sourceBytesModified"))

        val envelope = SourceCapabilityEnvelopeV02.describe(sha, parsed)
        assertEquals("ANOMALIES_FAIL_CLOSED", envelope.getString("container_traversal_status"))
        assertFalse(envelope.getBoolean("container_structure_complete"))
        assertEquals(1, envelope.getInt("container_traversal_anomaly_count"))
        assertEquals(1, envelope.getInt("record_count"))
        assertEquals(64, envelope.getString("container_traversal_anomaly_digest_sha256").length)
        assertEquals(64, envelope.getString("observation_evidence_digest_sha256").length)

        val bound = envelope.getJSONArray("container_traversal_anomalies").getJSONObject(0)
        assertEquals(sha, bound.getString("source_sha256"))
        assertEquals(bytes.size.toLong(), bound.getLong("source_byte_length"))
        assertEquals("IFD0/SubIFD[0]", bound.getString("path"))
        assertEquals(1000L, bound.getLong("attempted_offset"))
        assertEquals("PARSED", bound.getString("lifecycle"))
        assertEquals("CONTRADICTED_FAIL_CLOSED", bound.getString("admission"))
        assertEquals("SOURCE_STRUCTURE", bound.getString("authority"))
        assertFalse(bound.getBoolean("repair_attempted"))
        assertFalse(bound.getBoolean("source_values_modified"))
        assertFalse(bound.getBoolean("creates_new_evidence"))
        assertFalse(bound.getBoolean("scientific_writeback_allowed"))

        assertEquals(0, envelope.getJSONArray("admitted_capabilities").length())
        assertEquals(0, envelope.getJSONArray("used_capabilities").length())
        assertFalse(envelope.getBoolean("pixel_route_effect"))
        assertFalse(envelope.getBoolean("scientific_master_pixel_effect"))
        assertFalse(envelope.getBoolean("appearance_pixel_effect"))
    }

    @Test
    fun revisitedIfdOffsetIsReportedWithoutCallingItNecessarilyACycle() {
        val bytes = ByteArray(14)
        val b = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN)
        b.put(0, 'I'.code.toByte())
        b.put(1, 'I'.code.toByte())
        b.putShort(2, 42.toShort())
        b.putInt(4, 8)
        b.putShort(8, 0.toShort())
        b.putInt(10, 8) // NextIFD points back to the already parsed IFD.

        val parsed = parse(bytes)
        assertEquals(1, parsed.getJSONArray("ifds").length())
        assertEquals("ANOMALIES_FAIL_CLOSED", parsed.getString("containerTraversalStatus"))
        val anomaly = parsed.getJSONArray("containerTraversalAnomalies").getJSONObject(0)
        assertEquals("IFD0/NextIFD", anomaly.getString("path"))
        assertEquals(8L, anomaly.getLong("attemptedOffset"))
        assertEquals(1, anomaly.getInt("depth"))
        assertEquals("REVISITED_IFD_OFFSET_FAIL_CLOSED", anomaly.getString("status"))
    }

    @Test
    fun recursionLimitIsExplicitAndDoesNotInventFurtherIfds() {
        val bytes = ByteArray(44)
        val b = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN)
        b.put(0, 'I'.code.toByte())
        b.put(1, 'I'.code.toByte())
        b.putShort(2, 42.toShort())
        b.putInt(4, 8)

        val offsets = intArrayOf(8, 14, 20, 26, 32, 38)
        for (i in offsets.indices) {
            val offset = offsets[i]
            b.putShort(offset, 0.toShort())
            b.putInt(offset + 2, if (i + 1 < offsets.size) offsets[i + 1] else 0)
        }

        val parsed = parse(bytes)
        assertEquals(5, parsed.getJSONArray("ifds").length())
        assertEquals("ANOMALIES_FAIL_CLOSED", parsed.getString("containerTraversalStatus"))
        assertEquals(1, parsed.getInt("containerTraversalAnomalyCount"))
        val anomaly = parsed.getJSONArray("containerTraversalAnomalies").getJSONObject(0)
        assertEquals(38L, anomaly.getLong("attemptedOffset"))
        assertEquals(5, anomaly.getInt("depth"))
        assertEquals("RECURSION_LIMIT_EXCEEDED_FAIL_CLOSED", anomaly.getString("status"))
    }

    @Test
    fun cleanTraversalReportsCompleteWithDeterministicEmptyAnomalyDigest() {
        val bytes = ByteArray(14)
        val b = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN)
        b.put(0, 'I'.code.toByte())
        b.put(1, 'I'.code.toByte())
        b.putShort(2, 42.toShort())
        b.putInt(4, 8)
        b.putShort(8, 0.toShort())
        b.putInt(10, 0)

        val parsed = parse(bytes)
        assertEquals(
            "COMPLETE_NO_REPORTED_TRAVERSAL_ANOMALY",
            parsed.getString("containerTraversalStatus"),
        )
        assertEquals(0, parsed.getInt("containerTraversalAnomalyCount"))

        val first = SourceCapabilityEnvelopeV02.describe(sha, parsed)
        val second = SourceCapabilityEnvelopeV02.describe(sha, parsed)
        assertTrue(first.getBoolean("container_structure_complete"))
        assertEquals(0, first.getInt("container_traversal_anomaly_count"))
        assertEquals(
            first.getString("container_traversal_anomaly_digest_sha256"),
            second.getString("container_traversal_anomaly_digest_sha256"),
        )
        assertEquals(
            first.getString("observation_evidence_digest_sha256"),
            second.getString("observation_evidence_digest_sha256"),
        )
    }

    @Test
    fun hardStructuralTruncationStillThrowsInsteadOfBecomingPartialSuccess() {
        val bytes = ByteArray(9)
        val b = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN)
        b.put(0, 'I'.code.toByte())
        b.put(1, 'I'.code.toByte())
        b.putShort(2, 42.toShort())
        b.putInt(4, 8)
        b.put(8, 1)

        var rejected = false
        try {
            parse(bytes)
        } catch (_: IllegalArgumentException) {
            rejected = true
        }
        assertTrue(rejected)
    }

    @Test
    fun unavailableEnvelopeBindsHardParseFailureToSourceWithoutAuthority() {
        val envelope = SourceCapabilityEnvelopeV02.unavailable(sha, "IFD block onvolledig")
        assertEquals("UNKNOWN_FAIL_CLOSED", envelope.getString("status"))
        assertEquals("PARSE_FAILED_FAIL_CLOSED", envelope.getString("container_traversal_status"))
        assertFalse(envelope.getBoolean("container_structure_complete"))
        assertEquals(0, envelope.getInt("record_count"))
        assertEquals(0, envelope.getJSONArray("records").length())
        val failure = envelope.getJSONObject("container_parse_failure")
        assertEquals(sha, failure.getString("source_sha256"))
        assertEquals("CONTRADICTED_FAIL_CLOSED", failure.getString("admission"))
        assertFalse(failure.getBoolean("creates_new_evidence"))
        assertFalse(failure.getBoolean("scientific_writeback_allowed"))
        assertFalse(envelope.getBoolean("creates_new_evidence"))
        assertFalse(envelope.getBoolean("scientific_writeback_allowed"))
    }

    private fun parse(bytes: ByteArray) =
        Files.createTempFile("draw-traversal-status", ".dng").let { path ->
            try {
                Files.write(path, bytes)
                FileChannel.open(path, StandardOpenOption.READ).use { channel ->
                    DngContainerMetadataParser.parse(channel, bytes.size.toLong())
                }
            } finally {
                Files.deleteIfExists(path)
            }
        }
}
