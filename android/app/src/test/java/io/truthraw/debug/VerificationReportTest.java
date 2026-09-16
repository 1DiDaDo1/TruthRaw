package io.truthraw.debug;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class VerificationReportTest {
    @Test
    public void exactSourceAndCfaProduceScopedPassReport() {
        Sha256.DigestResult source = new Sha256.DigestResult(
                IdentityContract.SOURCE_BYTES, IdentityContract.SOURCE_SHA256);
        DngCfaHasher.Result cfa = new DngCfaHasher.Result(
                IdentityContract.DECODED_CFA_WIDTH,
                IdentityContract.DECODED_CFA_HEIGHT,
                IdentityContract.DECODED_CFA_STRIPS,
                IdentityContract.DECODED_CFA_BYTES,
                IdentityContract.DECODED_CFA_SHA256,
                true);

        String json = VerificationReport.build(
                "2026-09-16T07:53:00Z",
                "HONOR BKQ-N49",
                "16 (API 36)",
                IdentityContract.SOURCE_FILENAME,
                source,
                cfa);

        assertTrue(json.contains("\"result\": \"PASS_EXACT_SOURCE_AND_DECODED_CFA\""));
        assertTrue(json.contains("\"source_and_cfa_identity_match\": true"));
        assertTrue(json.contains("\"scientific_master_recomputed_on_device\": false"));
        assertTrue(json.contains("\"dynamic_authority_recomputed_on_device\": false"));
        assertTrue(json.contains("\"hdr_projection_run_on_device\": false"));
        assertTrue(json.contains("\"scientific_writeback_allowed\": false"));
        assertTrue(json.contains(IdentityContract.SCIENTIFIC_MASTER_SHA256));
        assertTrue(json.contains(IdentityContract.DYNAMIC_AUTHORITY_SHA256));
    }

    @Test
    public void mismatchingSourceNeverInheritsPass() {
        Sha256.DigestResult source = new Sha256.DigestResult(123L, "deadbeef");
        String json = VerificationReport.build(
                "2026-09-16T07:53:00Z", "device", "android", "other.dng", source, null);

        assertTrue(json.contains("\"result\": \"FAIL_IDENTITY_NOT_COMPLETE\""));
        assertTrue(json.contains("\"source_and_cfa_identity_match\": false"));
        assertTrue(json.contains("\"decoded\": false"));
        assertFalse(json.contains("\"identity_match\": true"));
    }
}
