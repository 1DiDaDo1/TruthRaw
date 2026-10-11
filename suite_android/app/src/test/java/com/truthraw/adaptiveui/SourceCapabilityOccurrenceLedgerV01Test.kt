package com.truthraw.adaptiveui

import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.channels.FileChannel
import java.nio.file.Files
import java.nio.file.StandardOpenOption
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class SourceCapabilityOccurrenceLedgerV01Test {
    private val sha = "c".repeat(64)

    @Test
    fun classicTiffEntriesRetainExactPhysicalOccurrencesAndDuplicateTags() {
        val bytes = ByteArray(58)
        val b = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN)
        b.put(0, 'I'.code.toByte())
        b.put(1, 'I'.code.toByte())
        b.putShort(2, 42.toShort())
        b.putInt(4, 8)
        b.putShort(8, 3.toShort())

        // IFD0 entry 0: BlackLevel, SHORT[1], inline value 64.
        b.position(10)
        b.putShort(50714.toShort())
        b.putShort(3.toShort())
        b.putInt(1)
        b.putShort(64.toShort())
        b.putShort(0.toShort())

        // IFD0 entry 1: the same BlackLevel tag, LONG[2], external at [50,58).
        b.position(22)
        b.putShort(50714.toShort())
        b.putShort(4.toShort())
        b.putInt(2)
        b.putInt(50)

        // IFD0 entry 2: physically present unsupported type. It must not disappear.
        b.position(34)
        b.putShort(65000.toShort())
        b.putShort(99.toShort())
        b.putInt(1)
        b.putInt(0)

        b.putInt(46, 0)
        b.putInt(50, 10)
        b.putInt(54, 20)

        val parsed = parse(bytes)
        assertEquals(
            "truthraw.classic-tiff-dng-container-metadata.v0.55-occurrence-ledger",
            parsed.getString("parser"),
        )
        val entries = parsed.getJSONArray("ifds").getJSONObject(0).getJSONArray("entries")
        assertEquals(3, entries.length())

        val inline = entries.getJSONObject(0)
        assertEquals(0, inline.getInt("occurrenceIndex"))
        assertEquals(10L, inline.getLong("entryOffset"))
        assertEquals(18L, inline.getLong("valueFieldOffset"))
        assertEquals("40000000", inline.getString("originalValueOrOffsetBytesHex"))
        assertEquals("INLINE", inline.getString("storageKind"))
        assertEquals("LOCATED", inline.getString("locatorStatus"))
        assertEquals("PARSED_CLASSIC_TIFF_ENTRY", inline.getString("parseStatus"))
        assertEquals("DECODED", inline.getString("decodeStatus"))
        assertTrue(inline.isNull("externalValueOffset"))

        val external = entries.getJSONObject(1)
        assertEquals(1, external.getInt("occurrenceIndex"))
        assertEquals(22L, external.getLong("entryOffset"))
        assertEquals(30L, external.getLong("valueFieldOffset"))
        assertEquals("32000000", external.getString("originalValueOrOffsetBytesHex"))
        assertEquals("EXTERNAL", external.getString("storageKind"))
        assertEquals(8L, external.getLong("valueByteCount"))
        assertEquals(50L, external.getLong("externalValueOffset"))
        assertEquals(58L, external.getLong("externalValueEndExclusive"))
        assertEquals("DECODED", external.getString("decodeStatus"))

        val unsupported = entries.getJSONObject(2)
        assertEquals(2, unsupported.getInt("occurrenceIndex"))
        assertEquals(34L, unsupported.getLong("entryOffset"))
        assertEquals("UNKNOWN", unsupported.getString("storageKind"))
        assertEquals("LOCATED_UNSUPPORTED_TYPE", unsupported.getString("locatorStatus"))
        assertEquals("UNSUPPORTED_TYPE", unsupported.getString("decodeStatus"))

        val envelope = SourceCapabilityEnvelopeV02.describe(sha, parsed)
        assertEquals(58L, envelope.getLong("source_byte_length"))
        assertTrue(envelope.getBoolean("duplicate_tags_preserved_as_distinct_occurrences"))
        assertEquals("IFD_PATH_PLUS_EXACT_ENTRY_OFFSET", envelope.getString("physical_occurrence_identity"))
        assertEquals(3, envelope.getInt("record_count"))
        assertEquals(0, envelope.getJSONArray("admitted_capabilities").length())
        assertEquals(0, envelope.getJSONArray("used_capabilities").length())
        assertFalse(envelope.getBoolean("pixel_route_effect"))
        assertFalse(envelope.getBoolean("scientific_master_pixel_effect"))
        assertFalse(envelope.getBoolean("appearance_pixel_effect"))

        val records = envelope.getJSONArray("records")
        val first = records.getJSONObject(0)
        val second = records.getJSONObject(1)
        assertNotEquals(first.getString("source_location"), second.getString("source_location"))
        assertTrue(
            (0 until records.length())
                .map { records.getJSONObject(it) }
                .count { it.optInt("tag", -1) == 50714 } == 2,
        )
        for (i in 0 until records.length()) {
            val record = records.getJSONObject(i)
            assertEquals(sha, record.getString("source_sha256"))
            assertEquals(58L, record.getLong("source_byte_length"))
            assertFalse(record.getBoolean("creates_new_evidence"))
            assertFalse(record.getBoolean("scientific_writeback_allowed"))
        }
    }

    @Test
    fun externalOutOfBoundsOccurrenceIsRetainedButFailsClosed() {
        val bytes = ByteArray(26)
        val b = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN)
        b.put(0, 'I'.code.toByte())
        b.put(1, 'I'.code.toByte())
        b.putShort(2, 42.toShort())
        b.putInt(4, 8)
        b.putShort(8, 1.toShort())
        b.position(10)
        b.putShort(50714.toShort())
        b.putShort(5.toShort()) // RATIONAL, 8 bytes => external.
        b.putInt(1)
        b.putInt(1000)
        b.putInt(22, 0)

        val parsed = parse(bytes)
        val entry =
            parsed.getJSONArray("ifds")
                .getJSONObject(0)
                .getJSONArray("entries")
                .getJSONObject(0)

        assertEquals("EXTERNAL", entry.getString("storageKind"))
        assertEquals("LOCATED_EXTERNAL_RANGE_OUT_OF_BOUNDS", entry.getString("locatorStatus"))
        assertEquals("EXTERNAL_RANGE_OUT_OF_BOUNDS", entry.getString("decodeStatus"))
        assertEquals(1000L, entry.getLong("externalValueOffset"))
        assertTrue(entry.isNull("externalValueEndExclusive"))

        val envelope = SourceCapabilityEnvelopeV02.describe(sha, parsed)
        assertEquals(1, envelope.getInt("record_count"))
        val record = envelope.getJSONArray("records").getJSONObject(0)
        assertEquals("LOCATED_EXTERNAL_RANGE_OUT_OF_BOUNDS", record.getString("locator_status"))
        assertEquals("EXTERNAL_RANGE_OUT_OF_BOUNDS", record.getString("decode_status"))
        assertEquals("PRESENT_UNVALIDATED", record.getString("admission"))
        assertFalse(record.getBoolean("creates_new_evidence"))
    }

    @Test
    fun bigTiffMagicRemainsFailClosedInsteadOfUsingClassicEntryLayout() {
        val bytes = ByteArray(16)
        val b = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN)
        b.put(0, 'I'.code.toByte())
        b.put(1, 'I'.code.toByte())
        b.putShort(2, 43.toShort())

        var rejected = false
        try {
            parse(bytes)
        } catch (_: IllegalArgumentException) {
            rejected = true
        }
        assertTrue(rejected)
    }

    private fun parse(bytes: ByteArray) =
        Files.createTempFile("draw-occurrence-ledger", ".dng").let { path ->
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
