package io.truthraw.debug;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import org.junit.Test;

public final class DngCfaHasherTest {
    @Test
    public void hashesCanonicalLittleEndianCfaRasterFromUncompressedStrip() throws Exception {
        File file = File.createTempFile("truthraw-cfa-test-", ".dng");
        try {
            byte[] tiff = syntheticTiff();
            try (FileOutputStream out = new FileOutputStream(file)) {
                out.write(tiff);
            }
            DngCfaHasher.Result r = DngCfaHasher.hash(file);
            assertEquals(2, r.width);
            assertEquals(2, r.height);
            assertEquals(1, r.stripCount);
            assertEquals(8L, r.decodedBytes);
            assertTrue(r.sourceLittleEndian);
            assertEquals("a6f329ab6727fb8193804b6e25dc194cf01eac57bc2af8d7b6f10e69a2ebb0c6", r.decodedCfaSha256);
        } finally {
            file.delete();
        }
    }

    private static byte[] syntheticTiff() {
        final int entries = 9;
        final int ifdOffset = 8;
        final int payloadOffset = ifdOffset + 2 + entries * 12 + 4;
        ByteBuffer b = ByteBuffer.allocate(payloadOffset + 8).order(ByteOrder.LITTLE_ENDIAN);
        b.put((byte) 'I').put((byte) 'I').putShort((short) 42).putInt(ifdOffset);
        b.position(ifdOffset);
        b.putShort((short) entries);
        entryLong(b, 256, 2);       // ImageWidth
        entryLong(b, 257, 2);       // ImageLength
        entryShort(b, 258, 16);     // BitsPerSample
        entryShort(b, 259, 1);      // Compression = none
        entryShort(b, 262, 32803);  // Photometric = CFA
        entryLong(b, 273, payloadOffset); // StripOffsets
        entryShort(b, 277, 1);      // SamplesPerPixel
        entryLong(b, 278, 2);       // RowsPerStrip
        entryLong(b, 279, 8);       // StripByteCounts
        b.putInt(0);                // next IFD
        b.putShort((short) 1).putShort((short) 2).putShort((short) 3).putShort((short) 1023);
        return b.array();
    }

    private static void entryShort(ByteBuffer b, int tag, int value) {
        b.putShort((short) tag).putShort((short) 3).putInt(1).putShort((short) value).putShort((short) 0);
    }

    private static void entryLong(ByteBuffer b, int tag, int value) {
        b.putShort((short) tag).putShort((short) 4).putInt(1).putInt(value);
    }
}
