package com.truthraw.adaptiveui

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class SourceCapabilityEnvelopeV02Test {
    private val shaA = "a".repeat(64)

    @Test
    fun ledgerDigestIsIndependentOfInsertionOrder() {
        val a = record("IFD0/tag:256", 256, "ImageWidth", "4080")
        val b = record("IFD0/tag:257", 257, "ImageLength", "3072")
        assertEquals(
            SourceCapabilityEnvelopeV02.ledgerDigest(listOf(a, b)),
            SourceCapabilityEnvelopeV02.ledgerDigest(listOf(b, a)),
        )
    }

    @Test
    fun sourceBindingRequiresExactSha256Shape() {
        assertTrue(SourceCapabilityEnvelopeV02.isSha256(shaA))
        assertTrue(SourceCapabilityEnvelopeV02.isSha256("ABCDEF" + "0".repeat(58)))
        assertFalse(SourceCapabilityEnvelopeV02.isSha256("a".repeat(63)))
        assertFalse(SourceCapabilityEnvelopeV02.isSha256("z".repeat(64)))
    }

    @Test
    fun provenanceOnlyCannotBecomeScientificUsed() {
        var rejected = false
        try {
            SourceCapabilityEnvelopeV02.Record(
                sourceSha256 = shaA,
                sourceLocation = "IFD0/tag:271",
                tag = 271,
                semanticKey = "Make",
                parserId = "test",
                parserType = "ASCII",
                itemCount = 1,
                canonicalValue = "\"Vendor\"",
                lifecycle = SourceCapabilityEnvelopeV02.Lifecycle.USED,
                admission = SourceCapabilityEnvelopeV02.Admission.PROVENANCE_ONLY,
                authority = SourceCapabilityEnvelopeV02.Authority.SOURCE_IDENTITY_PROVENANCE,
                consumers = listOf("SCIENTIFIC_MODEL"),
            )
        } catch (_: IllegalArgumentException) {
            rejected = true
        }
        assertTrue(rejected)
    }

    @Test
    fun usedEvidenceRequiresAdmissionAndNamedConsumer() {
        var rejected = false
        try {
            SourceCapabilityEnvelopeV02.Record(
                sourceSha256 = shaA,
                sourceLocation = "IFD0/tag:50714",
                tag = 50714,
                semanticKey = "BlackLevel",
                parserId = "test",
                parserType = "RATIONAL",
                itemCount = 1,
                canonicalValue = "64",
                lifecycle = SourceCapabilityEnvelopeV02.Lifecycle.USED,
                admission = SourceCapabilityEnvelopeV02.Admission.CONSISTENT_ADMITTED,
                authority = SourceCapabilityEnvelopeV02.Authority.SOURCE_STRUCTURE,
                consumers = emptyList(),
            )
        } catch (_: IllegalArgumentException) {
            rejected = true
        }
        assertTrue(rejected)
    }

    @Test
    fun digestChangesWhenSourceBindingChanges() {
        val a = record("IFD0/tag:256", 256, "ImageWidth", "4080")
        val b = a.copy(sourceSha256 = "b".repeat(64))
        assertFalse(
            SourceCapabilityEnvelopeV02.ledgerDigest(listOf(a)) ==
                SourceCapabilityEnvelopeV02.ledgerDigest(listOf(b)),
        )
    }

    private fun record(
        location: String,
        tag: Int,
        semantic: String,
        value: String,
    ) =
        SourceCapabilityEnvelopeV02.Record(
            sourceSha256 = shaA,
            sourceLocation = location,
            tag = tag,
            semanticKey = semantic,
            parserId = "truthraw.test.parser",
            parserType = "LONG",
            itemCount = 1,
            canonicalValue = value,
            lifecycle = SourceCapabilityEnvelopeV02.Lifecycle.PARSED,
            admission = SourceCapabilityEnvelopeV02.Admission.PRESENT_UNVALIDATED,
            authority = SourceCapabilityEnvelopeV02.Authority.SOURCE_STRUCTURE,
        )
}
