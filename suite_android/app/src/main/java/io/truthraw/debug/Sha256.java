package io.truthraw.debug;

import java.io.IOException;
import java.io.InputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public final class Sha256 {
    private static final int BUFFER_BYTES = 1024 * 1024;

    private Sha256() {}

    public static DigestResult digest(InputStream input) throws IOException {
        final MessageDigest md;
        try {
            md = MessageDigest.getInstance("SHA-256");
        } catch (NoSuchAlgorithmException e) {
            throw new IllegalStateException("SHA-256 unavailable", e);
        }
        byte[] buffer = new byte[BUFFER_BYTES];
        long count = 0L;
        int n;
        while ((n = input.read(buffer)) != -1) {
            md.update(buffer, 0, n);
            count += n;
        }
        return new DigestResult(count, toHex(md.digest()));
    }

    private static String toHex(byte[] data) {
        StringBuilder out = new StringBuilder(data.length * 2);
        for (byte b : data) {
            out.append(String.format("%02x", b & 0xff));
        }
        return out.toString();
    }

    public static final class DigestResult {
        public final long byteCount;
        public final String sha256;

        public DigestResult(long byteCount, String sha256) {
            this.byteCount = byteCount;
            this.sha256 = sha256;
        }
    }
}
