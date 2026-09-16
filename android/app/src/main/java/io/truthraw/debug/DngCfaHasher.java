package io.truthraw.debug;

import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.HashMap;
import java.util.Map;

/** Minimal fail-closed classic-TIFF DNG CFA decoder for uncompressed 16-bit mosaic data.
 *
 * This is intentionally not a generic image decoder. It verifies the exact storage
 * contract used by the frozen TruthRaw source: classic TIFF, one 16-bit CFA sample
 * per pixel, Compression=1, PhotometricInterpretation=CFA, strips, no packing.
 * The decoded CFA digest is canonical little-endian uint16 in raster order.
 */
public final class DngCfaHasher {
    private static final int TAG_IMAGE_WIDTH = 256;
    private static final int TAG_IMAGE_LENGTH = 257;
    private static final int TAG_BITS_PER_SAMPLE = 258;
    private static final int TAG_COMPRESSION = 259;
    private static final int TAG_PHOTOMETRIC = 262;
    private static final int TAG_STRIP_OFFSETS = 273;
    private static final int TAG_SAMPLES_PER_PIXEL = 277;
    private static final int TAG_ROWS_PER_STRIP = 278;
    private static final int TAG_STRIP_BYTE_COUNTS = 279;
    private static final int PHOTOMETRIC_CFA = 32803;

    private DngCfaHasher() {}

    public static Result hash(File file) throws IOException {
        try (RandomAccessFile raf = new RandomAccessFile(file, "r")) {
            if (raf.length() < 8) {
                throw new IOException("TIFF file too short");
            }
            int b0 = raf.readUnsignedByte();
            int b1 = raf.readUnsignedByte();
            final boolean little;
            if (b0 == 'I' && b1 == 'I') {
                little = true;
            } else if (b0 == 'M' && b1 == 'M') {
                little = false;
            } else {
                throw new IOException("Unsupported TIFF byte order");
            }
            Reader r = new Reader(raf, little);
            if (r.u16() != 42) {
                throw new IOException("Not a classic TIFF/DNG container");
            }
            long ifdOffset = r.u32();
            if (ifdOffset <= 0 || ifdOffset >= raf.length()) {
                throw new IOException("Invalid first IFD offset");
            }
            raf.seek(ifdOffset);
            int entryCount = r.u16();
            Map<Integer, Entry> entries = new HashMap<>();
            for (int i = 0; i < entryCount; i++) {
                int tag = r.u16();
                int type = r.u16();
                long count = r.u32();
                long valueFieldPosition = raf.getFilePointer();
                long field = r.u32();
                int typeBytes = typeBytes(type);
                if (typeBytes == 0 || count <= 0 || count > Integer.MAX_VALUE) {
                    continue;
                }
                long dataBytes = count * (long) typeBytes;
                long dataOffset = dataBytes <= 4 ? valueFieldPosition : field;
                if (dataOffset < 0 || dataOffset + dataBytes > raf.length()) {
                    throw new IOException("TIFF entry outside file for tag " + tag);
                }
                entries.put(tag, new Entry(type, (int) count, dataOffset));
            }

            long width = scalar(entries, TAG_IMAGE_WIDTH, r);
            long height = scalar(entries, TAG_IMAGE_LENGTH, r);
            long bits = scalar(entries, TAG_BITS_PER_SAMPLE, r);
            long compression = scalar(entries, TAG_COMPRESSION, r);
            long photometric = scalar(entries, TAG_PHOTOMETRIC, r);
            long samplesPerPixel = scalar(entries, TAG_SAMPLES_PER_PIXEL, r);
            long rowsPerStrip = scalar(entries, TAG_ROWS_PER_STRIP, r);
            long[] offsets = array(entries, TAG_STRIP_OFFSETS, r);
            long[] byteCounts = array(entries, TAG_STRIP_BYTE_COUNTS, r);

            if (width <= 0 || height <= 0 || width > Integer.MAX_VALUE || height > Integer.MAX_VALUE) {
                throw new IOException("Invalid CFA dimensions");
            }
            if (bits != 16 || compression != 1 || photometric != PHOTOMETRIC_CFA || samplesPerPixel != 1) {
                throw new IOException("DNG is outside the supported uncompressed 16-bit CFA contract");
            }
            if (rowsPerStrip <= 0 || offsets.length == 0 || offsets.length != byteCounts.length) {
                throw new IOException("Invalid strip layout");
            }
            long expectedBytes = Math.multiplyExact(Math.multiplyExact(width, height), 2L);
            long declaredBytes = 0L;
            for (long n : byteCounts) {
                declaredBytes = Math.addExact(declaredBytes, n);
            }
            if (declaredBytes != expectedBytes) {
                throw new IOException("Strip payload does not equal width*height*2");
            }

            MessageDigest md = sha256();
            byte[] buffer = new byte[1024 * 1024];
            byte carry = 0;
            boolean hasCarry = false;
            for (int s = 0; s < offsets.length; s++) {
                long offset = offsets[s];
                long remaining = byteCounts[s];
                if (offset < 0 || remaining < 0 || offset + remaining > raf.length()) {
                    throw new IOException("Strip outside file");
                }
                raf.seek(offset);
                while (remaining > 0) {
                    int want = (int) Math.min(buffer.length, remaining);
                    raf.readFully(buffer, 0, want);
                    if (little) {
                        md.update(buffer, 0, want);
                    } else {
                        int start = 0;
                        if (hasCarry) {
                            if (want == 0) {
                                break;
                            }
                            md.update(buffer[0]);
                            md.update(carry);
                            start = 1;
                            hasCarry = false;
                        }
                        int pairedEnd = start + ((want - start) & ~1);
                        for (int i = start; i < pairedEnd; i += 2) {
                            md.update(buffer[i + 1]);
                            md.update(buffer[i]);
                        }
                        if (pairedEnd < want) {
                            carry = buffer[pairedEnd];
                            hasCarry = true;
                        }
                    }
                    remaining -= want;
                }
            }
            if (hasCarry) {
                throw new IOException("Odd number of bytes in 16-bit CFA payload");
            }
            return new Result((int) width, (int) height, offsets.length, expectedBytes, hex(md.digest()), little);
        } catch (ArithmeticException e) {
            throw new IOException("TIFF dimension overflow", e);
        }
    }

    private static MessageDigest sha256() {
        try {
            return MessageDigest.getInstance("SHA-256");
        } catch (NoSuchAlgorithmException e) {
            throw new IllegalStateException("SHA-256 unavailable", e);
        }
    }

    private static long scalar(Map<Integer, Entry> entries, int tag, Reader r) throws IOException {
        long[] values = array(entries, tag, r);
        if (values.length != 1) {
            throw new IOException("Expected one value for TIFF tag " + tag);
        }
        return values[0];
    }

    private static long[] array(Map<Integer, Entry> entries, int tag, Reader r) throws IOException {
        Entry e = entries.get(tag);
        if (e == null) {
            throw new IOException("Missing required TIFF tag " + tag);
        }
        if (e.type != 3 && e.type != 4) {
            throw new IOException("Unsupported TIFF type for tag " + tag);
        }
        long old = r.raf.getFilePointer();
        try {
            r.raf.seek(e.offset);
            long[] out = new long[e.count];
            for (int i = 0; i < out.length; i++) {
                out[i] = e.type == 3 ? r.u16() : r.u32();
            }
            return out;
        } finally {
            r.raf.seek(old);
        }
    }

    private static int typeBytes(int type) {
        if (type == 3) return 2; // SHORT
        if (type == 4) return 4; // LONG
        return 0;
    }

    private static String hex(byte[] data) {
        StringBuilder out = new StringBuilder(data.length * 2);
        for (byte b : data) {
            out.append(String.format("%02x", b & 0xff));
        }
        return out.toString();
    }

    private static final class Entry {
        final int type;
        final int count;
        final long offset;

        Entry(int type, int count, long offset) {
            this.type = type;
            this.count = count;
            this.offset = offset;
        }
    }

    private static final class Reader {
        final RandomAccessFile raf;
        final boolean little;

        Reader(RandomAccessFile raf, boolean little) {
            this.raf = raf;
            this.little = little;
        }

        int u16() throws IOException {
            int a = raf.readUnsignedByte();
            int b = raf.readUnsignedByte();
            return little ? (a | (b << 8)) : ((a << 8) | b);
        }

        long u32() throws IOException {
            long a = raf.readUnsignedByte();
            long b = raf.readUnsignedByte();
            long c = raf.readUnsignedByte();
            long d = raf.readUnsignedByte();
            return little
                    ? (a | (b << 8) | (c << 16) | (d << 24))
                    : ((a << 24) | (b << 16) | (c << 8) | d);
        }
    }

    public static final class Result {
        public final int width;
        public final int height;
        public final int stripCount;
        public final long decodedBytes;
        public final String decodedCfaSha256;
        public final boolean sourceLittleEndian;

        Result(int width, int height, int stripCount, long decodedBytes, String decodedCfaSha256, boolean sourceLittleEndian) {
            this.width = width;
            this.height = height;
            this.stripCount = stripCount;
            this.decodedBytes = decodedBytes;
            this.decodedCfaSha256 = decodedCfaSha256;
            this.sourceLittleEndian = sourceLittleEndian;
        }
    }
}
