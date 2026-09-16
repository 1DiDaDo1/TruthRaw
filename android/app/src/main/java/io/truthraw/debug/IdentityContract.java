package io.truthraw.debug;

/** Frozen research identities shown by the non-canonical debug client.
 *
 * The app may admit an exact source by byte count + SHA-256, but it does not
 * independently recompute the decoded CFA, Scientific Master, Dynamic Authority
 * field, or HDR projection yet. Those identities remain research references.
 */
public final class IdentityContract {
    public static final String SOURCE_FILENAME = "IMG_BNC_TRUTHRAW20260907_094449_565.dng";
    public static final long SOURCE_BYTES = 25_106_120L;
    public static final String SOURCE_SHA256 = "7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67";
    public static final String DECODED_CFA_SHA256 = "883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c";
    public static final String SCIENTIFIC_MASTER_SHA256 = "a86034da7b6f9663640ee3b4478ccf4294d23b720f2b087d083dca38f5fb4640";
    public static final String DYNAMIC_AUTHORITY_SHA256 = "7678a0b145f8721347cdcc5177fedb720ba19b91bf34984a3f18bd8216408098";
    public static final String P3_TRANSFORM_SHA256 = "2105712a1be9950089976ba358afd4b062347680d5e6d8c500462c2e8d541533";
    public static final double REFERENCE_L0 = 0.12564234435558319;

    private IdentityContract() {}

    public static boolean isExactFrozenSource(long byteCount, String sha256) {
        return byteCount == SOURCE_BYTES && SOURCE_SHA256.equalsIgnoreCase(sha256);
    }

    public static String admissionLabel(long byteCount, String sha256) {
        return isExactFrozenSource(byteCount, sha256)
                ? "PASS — exact immutable frozen source"
                : "FAIL — not the exact frozen source; no scientific identity inheritance";
    }
}
