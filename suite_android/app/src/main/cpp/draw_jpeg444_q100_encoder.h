#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace draw_jpeg444_q100 {

using WriteFn = void(void *context, const void *data, int size);

static constexpr std::uint8_t kZigZag[64] = {
    0,1,5,6,14,15,27,28,2,4,7,13,16,26,29,42,
    3,8,12,17,25,30,41,43,9,11,18,24,31,40,44,53,
    10,19,23,32,39,45,52,54,20,22,33,38,46,51,55,60,
    21,34,37,47,50,56,59,61,35,36,48,49,57,58,62,63,
};

struct HuffmanTable {
    std::uint16_t code[256]{};
    std::uint8_t bits[256]{};
};

inline bool buildHuffman(
    const std::uint8_t counts[17],
    const std::uint8_t *values,
    std::size_t valueCount,
    HuffmanTable &out) noexcept {
    std::memset(&out, 0, sizeof(out));
    std::uint32_t code = 0u;
    std::size_t k = 0u;
    for (int bitLength = 1; bitLength <= 16; ++bitLength) {
        for (int j = 0; j < counts[bitLength]; ++j) {
            if (k >= valueCount || code > 0xffffu) return false;
            const std::uint8_t symbol = values[k++];
            out.code[symbol] = static_cast<std::uint16_t>(code);
            out.bits[symbol] = static_cast<std::uint8_t>(bitLength);
            ++code;
        }
        code <<= 1u;
    }
    return k == valueCount;
}

inline void putByte(WriteFn *write, void *context, std::uint8_t value) {
    write(context, &value, 1);
}

inline void writeBits(
    WriteFn *write,
    void *context,
    std::uint32_t &bitBuffer,
    int &bitCount,
    std::uint16_t code,
    std::uint8_t bits) {
    if (bits == 0u) return;
    bitCount += static_cast<int>(bits);
    bitBuffer |= static_cast<std::uint32_t>(code) << (24 - bitCount);
    while (bitCount >= 8) {
        const std::uint8_t value = static_cast<std::uint8_t>((bitBuffer >> 16) & 0xffu);
        putByte(write, context, value);
        if (value == 0xffu) putByte(write, context, 0u);
        bitBuffer <<= 8u;
        bitCount -= 8;
    }
}

inline void dct(
    float &d0, float &d1, float &d2, float &d3,
    float &d4, float &d5, float &d6, float &d7) noexcept {
    float tmp0 = d0 + d7;
    float tmp7 = d0 - d7;
    float tmp1 = d1 + d6;
    float tmp6 = d1 - d6;
    float tmp2 = d2 + d5;
    float tmp5 = d2 - d5;
    float tmp3 = d3 + d4;
    float tmp4 = d3 - d4;
    float tmp10 = tmp0 + tmp3;
    float tmp13 = tmp0 - tmp3;
    float tmp11 = tmp1 + tmp2;
    float tmp12 = tmp1 - tmp2;
    d0 = tmp10 + tmp11;
    d4 = tmp10 - tmp11;
    const float z1 = (tmp12 + tmp13) * 0.707106781f;
    d2 = tmp13 + z1;
    d6 = tmp13 - z1;
    tmp10 = tmp4 + tmp5;
    tmp11 = tmp5 + tmp6;
    tmp12 = tmp6 + tmp7;
    const float z5 = (tmp10 - tmp12) * 0.382683433f;
    const float z2 = tmp10 * 0.541196100f + z5;
    const float z4 = tmp12 * 1.306562965f + z5;
    const float z3 = tmp11 * 0.707106781f;
    const float z11 = tmp7 + z3;
    const float z13 = tmp7 - z3;
    d5 = z13 + z2;
    d3 = z13 - z2;
    d1 = z11 + z4;
    d7 = z11 - z4;
}

inline void valueBits(int value, std::uint16_t &bits, std::uint8_t &count) noexcept {
    int magnitude = value < 0 ? -value : value;
    const int encoded = value < 0 ? value - 1 : value;
    count = 1u;
    while ((magnitude >>= 1) != 0) ++count;
    bits = static_cast<std::uint16_t>(encoded & ((1 << count) - 1));
}

inline int processBlock(
    WriteFn *write,
    void *context,
    std::uint32_t &bitBuffer,
    int &bitCount,
    float block[64],
    const float quantScale[64],
    int previousDc,
    const HuffmanTable &dc,
    const HuffmanTable &ac) {
    for (int i = 0; i < 64; i += 8) {
        dct(block[i], block[i+1], block[i+2], block[i+3], block[i+4], block[i+5], block[i+6], block[i+7]);
    }
    for (int i = 0; i < 8; ++i) {
        dct(block[i], block[i+8], block[i+16], block[i+24], block[i+32], block[i+40], block[i+48], block[i+56]);
    }

    int coefficients[64]{};
    for (int i = 0; i < 64; ++i) {
        const float value = block[i] * quantScale[i];
        coefficients[kZigZag[i]] = static_cast<int>(
            value < 0.0f ? std::ceil(value - 0.5f) : std::floor(value + 0.5f));
    }

    const int delta = coefficients[0] - previousDc;
    if (delta == 0) {
        writeBits(write, context, bitBuffer, bitCount, dc.code[0], dc.bits[0]);
    } else {
        std::uint16_t encoded = 0u;
        std::uint8_t encodedBits = 0u;
        valueBits(delta, encoded, encodedBits);
        writeBits(write, context, bitBuffer, bitCount, dc.code[encodedBits], dc.bits[encodedBits]);
        writeBits(write, context, bitBuffer, bitCount, encoded, encodedBits);
    }

    int last = 63;
    while (last > 0 && coefficients[last] == 0) --last;
    if (last == 0) {
        writeBits(write, context, bitBuffer, bitCount, ac.code[0x00], ac.bits[0x00]);
        return coefficients[0];
    }

    for (int i = 1; i <= last; ++i) {
        const int start = i;
        while (i <= last && coefficients[i] == 0) ++i;
        int zeroRun = i - start;
        while (zeroRun >= 16) {
            writeBits(write, context, bitBuffer, bitCount, ac.code[0xf0], ac.bits[0xf0]);
            zeroRun -= 16;
        }
        std::uint16_t encoded = 0u;
        std::uint8_t encodedBits = 0u;
        valueBits(coefficients[i], encoded, encodedBits);
        const int symbol = (zeroRun << 4) | encodedBits;
        writeBits(write, context, bitBuffer, bitCount, ac.code[symbol], ac.bits[symbol]);
        writeBits(write, context, bitBuffer, bitCount, encoded, encodedBits);
    }
    if (last != 63) {
        writeBits(write, context, bitBuffer, bitCount, ac.code[0x00], ac.bits[0x00]);
    }
    return coefficients[0];
}

inline bool encodeRgb24(
    WriteFn *write,
    void *context,
    const std::uint8_t *rgb,
    int width,
    int height) {
    if (write == nullptr || context == nullptr || rgb == nullptr || width <= 0 || height <= 0 ||
        width > 65535 || height > 65535) return false;

    static const std::uint8_t dcYCounts[17] = {0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0};
    static const std::uint8_t dcYValues[12] = {0,1,2,3,4,5,6,7,8,9,10,11};
    static const std::uint8_t acYCounts[17] = {0,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d};
    static const std::uint8_t acYValues[162] = {
        0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,
        0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,
        0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
        0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
        0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,
        0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,
        0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,
    };
    static const std::uint8_t dcCCounts[17] = {0,0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0};
    static const std::uint8_t dcCValues[12] = {0,1,2,3,4,5,6,7,8,9,10,11};
    static const std::uint8_t acCCounts[17] = {0,0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77};
    static const std::uint8_t acCValues[162] = {
        0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,
        0xa1,0xb1,0xc1,0x09,0x23,0x33,0x52,0xf0,0x15,0x62,0x72,0xd1,0x0a,0x16,0x24,0x34,0xe1,0x25,0xf1,0x17,0x18,0x19,0x1a,0x26,
        0x27,0x28,0x29,0x2a,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,
        0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x82,0x83,0x84,0x85,0x86,0x87,
        0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,
        0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,
        0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,
    };

    HuffmanTable dcY{}, acY{}, dcC{}, acC{};
    if (!buildHuffman(dcYCounts, dcYValues, 12u, dcY) ||
        !buildHuffman(acYCounts, acYValues, 162u, acY) ||
        !buildHuffman(dcCCounts, dcCValues, 12u, dcC) ||
        !buildHuffman(acCCounts, acCValues, 162u, acC)) return false;

    static const float aasf[8] = {
        2.828427125f,
        1.387039845f * 2.828427125f,
        1.306562965f * 2.828427125f,
        1.175875602f * 2.828427125f,
        2.828427125f,
        0.785694958f * 2.828427125f,
        0.541196100f * 2.828427125f,
        0.275899379f * 2.828427125f,
    };
    float quantScale[64]{};
    for (int row = 0, k = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col, ++k) {
            quantScale[k] = 1.0f / (aasf[row] * aasf[col]);
        }
    }

    static const std::uint8_t head0[] = {
        0xff,0xd8,0xff,0xe0,0x00,0x10,'J','F','I','F',0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x01,0x00,0x00,
        0xff,0xdb,0x00,0x84,0x00,
    };
    write(context, head0, static_cast<int>(sizeof(head0)));
    std::uint8_t ones[64];
    std::memset(ones, 1, sizeof(ones));
    write(context, ones, 64);
    putByte(write, context, 1u);
    write(context, ones, 64);

    const std::uint8_t head1[] = {
        0xff,0xc0,0x00,0x11,0x08,
        static_cast<std::uint8_t>((height >> 8) & 0xff), static_cast<std::uint8_t>(height & 0xff),
        static_cast<std::uint8_t>((width >> 8) & 0xff), static_cast<std::uint8_t>(width & 0xff),
        0x03, 0x01,0x11,0x00, 0x02,0x11,0x01, 0x03,0x11,0x01,
        0xff,0xc4,0x01,0xa2,0x00,
    };
    write(context, head1, static_cast<int>(sizeof(head1)));
    write(context, dcYCounts + 1, 16);
    write(context, dcYValues, 12);
    putByte(write, context, 0x10u);
    write(context, acYCounts + 1, 16);
    write(context, acYValues, 162);
    putByte(write, context, 0x01u);
    write(context, dcCCounts + 1, 16);
    write(context, dcCValues, 12);
    putByte(write, context, 0x11u);
    write(context, acCCounts + 1, 16);
    write(context, acCValues, 162);
    static const std::uint8_t head2[] = {0xff,0xda,0x00,0x0c,0x03,0x01,0x00,0x02,0x11,0x03,0x11,0x00,0x00,0x3f,0x00};
    write(context, head2, static_cast<int>(sizeof(head2)));

    int previousY = 0, previousCb = 0, previousCr = 0;
    std::uint32_t bitBuffer = 0u;
    int bitCount = 0;
    for (int y = 0; y < height; y += 8) {
        for (int x = 0; x < width; x += 8) {
            float yBlock[64], cbBlock[64], crBlock[64];
            int pos = 0;
            for (int row = 0; row < 8; ++row) {
                const int sy = std::min(y + row, height - 1);
                for (int col = 0; col < 8; ++col, ++pos) {
                    const int sx = std::min(x + col, width - 1);
                    const std::size_t p = (static_cast<std::size_t>(sy) * static_cast<std::size_t>(width) + static_cast<std::size_t>(sx)) * 3u;
                    const float r = rgb[p];
                    const float g = rgb[p + 1u];
                    const float b = rgb[p + 2u];
                    yBlock[pos]  =  0.29900f*r + 0.58700f*g + 0.11400f*b - 128.0f;
                    cbBlock[pos] = -0.16874f*r - 0.33126f*g + 0.50000f*b;
                    crBlock[pos] =  0.50000f*r - 0.41869f*g - 0.08131f*b;
                }
            }
            previousY = processBlock(write, context, bitBuffer, bitCount, yBlock, quantScale, previousY, dcY, acY);
            previousCb = processBlock(write, context, bitBuffer, bitCount, cbBlock, quantScale, previousCb, dcC, acC);
            previousCr = processBlock(write, context, bitBuffer, bitCount, crBlock, quantScale, previousCr, dcC, acC);
        }
    }

    writeBits(write, context, bitBuffer, bitCount, 0x7fu, 7u);
    putByte(write, context, 0xffu);
    putByte(write, context, 0xd9u);
    return true;
}

} // namespace draw_jpeg444_q100
