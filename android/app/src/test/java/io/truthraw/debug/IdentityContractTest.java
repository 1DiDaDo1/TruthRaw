package io.truthraw.debug;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertEquals;

import java.io.ByteArrayInputStream;
import java.nio.charset.StandardCharsets;

import org.junit.Test;

public final class IdentityContractTest {
    @Test
    public void exactFrozenSourceRequiresBothSizeAndHash() {
        assertTrue(IdentityContract.isExactFrozenSource(
                IdentityContract.SOURCE_BYTES,
                IdentityContract.SOURCE_SHA256));
        assertFalse(IdentityContract.isExactFrozenSource(
                IdentityContract.SOURCE_BYTES + 1,
                IdentityContract.SOURCE_SHA256));
        assertFalse(IdentityContract.isExactFrozenSource(
                IdentityContract.SOURCE_BYTES,
                "0".repeat(64)));
    }

    @Test
    public void sha256StreamsWithoutChangingBytes() throws Exception {
        Sha256.DigestResult result = Sha256.digest(
                new ByteArrayInputStream("abc".getBytes(StandardCharsets.US_ASCII)));
        assertEquals(3L, result.byteCount);
        assertEquals(
                "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
                result.sha256);
    }

    @Test
    public void frozenV19IdentityIsExplicit() {
        assertEquals(
                "7678a0b145f8721347cdcc5177fedb720ba19b91bf34984a3f18bd8216408098",
                IdentityContract.DYNAMIC_AUTHORITY_SHA256);
    }
}
