/*
 * D.RAW deterministic baseline JPEG writer for downstream presentation only.
 *
 * Derived from the public-domain jo_jpeg algorithm, but the fragile static
 * Huffman symbol lookup tables are deliberately removed. Canonical encoder
 * lookup tables are generated from the exact DHT count/value arrays written
 * into each JPEG. The only admitted public path is RGB24, quality 100, 4:4:4.
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

using jo_write_func = void(void *context, const void *data, int size);

namespace {

struct HuffmanCode {
    std::uint16_t code = 0;
    std::uint8_t length = 0;
};

constexpr std::array<std::uint8_t, 64> kZigZag = {
    0,1,5,6,14,15,27,28,2,4,7,13,16,26,29,42,
    3,8,12,17,25,30,41,43,9,11,18,24,31,40,44,53,
    10,19,23,32,39,45,52,54,20,22,33,38,46,51,55,60,
    21,34,37,47,50,56,59,61,35,36,48,49,57,58,62,63,
};

constexpr std::array<std::uint8_t, 17> kDcLumaCounts =
    {0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0};
constexpr std::array<std::uint8_t, 12> kDcLumaValues =
    {0,1,2,3,4,5,6,7,8,9,10,11};
constexpr std::array<std::uint8_t, 17> kAcLumaCounts =
    {0,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d};
constexpr std::array<std::uint8_t, 162> kAcLumaValues = {
    0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,
    0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,
    0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
    0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
    0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,
    0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,
    0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,
};
constexpr std::array<std::uint8_t, 17> kDcChromaCounts =
    {0,0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0};
constexpr std::array<std::uint8_t, 12> kDcChromaValues =
    {0,1,2,3,4,5,6,7,8,9,10,11};
constexpr std::array<std::uint8_t, 17> kAcChromaCounts =
    {0,0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77};
constexpr std::array<std::uint8_t, 162> kAcChromaValues = {
    0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,
    0xa1,0xb1,0xc1,0x09,0x23,0x33,0x52,0xf0,0x15,0x62,0x72,0xd1,0x0a,0x16,0x24,0x34,0xe1,0x25,0xf1,0x17,0x18,0x19,0x1a,0x26,
    0x27,0x28,0x29,0x2a,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,
    0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x82,0x83,0x84,0x85,0x86,0x87,
    0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,
    0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,
    0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,
};

constexpr std::array<int, 64> kYqt = {
    16,11,10,16,24,40,51,61,12,12,14,19,26,58,60,55,
    14,13,16,24,40,57,69,56,14,17,22,29,51,87,80,62,
    18,22,37,56,68,109,103,77,24,35,55,64,81,104,113,92,
    49,64,78,87,103,121,120,101,72,92,95,98,112,100,103,99,
};
constexpr std::array<int, 64> kUvqt = {
    17,18,24,47,99,99,99,99,18,21,26,66,99,99,99,99,
    24,26,56,99,99,99,99,99,47,66,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,
};
constexpr std::array<float, 8> kAasf = {
    1.0f * 2.828427125f,
    1.387039845f * 2.828427125f,
    1.306562965f * 2.828427125f,
    1.175875602f * 2.828427125f,
    1.0f * 2.828427125f,
    0.785694958f * 2.828427125f,
    0.541196100f * 2.828427125f,
    0.275899379f * 2.828427125f,
};

struct BitWriter {
    jo_write_func *func = nullptr;
    void *context = nullptr;
    std::uint64_t bits = 0;
    int count = 0;
    bool ok = true;

    void emitByte(std::uint8_t value) {
        if (!ok) return;
        func(context, &value, 1);
        if (value == 0xffu) {
            const std::uint8_t zero = 0u;
            func(context, &zero, 1);
        }
    }

    void write(std::uint16_t code, std::uint8_t length) {
        if (!ok || length == 0 || length > 16 || count > 48) {
            ok = false;
            return;
        }
        bits = (bits << length) | static_cast<std::uint64_t>(code);
        count += length;
        while (count >= 8) {
            const int shift = count - 8;
            emitByte(static_cast<std::uint8_t>((bits >> shift) & 0xffu));
            count -= 8;
            if (count == 0) {
                bits = 0;
            } else {
                bits &= ((std::uint64_t{1} << count) - 1u);
            }
        }
    }

    void finish() {
        if (count > 0) {
            const int pad = 8 - count;
            const std::uint16_t fill = static_cast<std::uint16_t>((std::uint16_t{1} << pad) - 1u);
            write(fill, static_cast<std::uint8_t>(pad));
        }
    }
};

template <std::size_t N>
bool buildHuffmanTable(
    const std::array<std::uint8_t, 17>& counts,
    const std::array<std::uint8_t, N>& values,
    std::array<HuffmanCode, 256>& out) {
    out.fill({});
    std::uint32_t code = 0u;
    std::size_t k = 0u;
    for (int length = 1; length <= 16; ++length) {
        for (int j = 0; j < counts[static_cast<std::size_t>(length)]; ++j) {
            if (k >= values.size() || code >= (std::uint32_t{1} << length)) return false;
            const auto symbol = values[k++];
            out[symbol] = HuffmanCode{
                static_cast<std::uint16_t>(code),
                static_cast<std::uint8_t>(length),
            };
            ++code;
        }
        code <<= 1u;
    }
    return k == values.size();
}

void dct8(float& d0, float& d1, float& d2, float& d3, float& d4, float& d5, float& d6, float& d7) {
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

struct MagnitudeBits {
    std::uint16_t bits = 0;
    std::uint8_t length = 0;
};

MagnitudeBits magnitudeBits(int value) {
    int magnitude = value < 0 ? -value : value;
    if (magnitude == 0) return {};
    std::uint8_t length = 0;
    int t = magnitude;
    while (t != 0) {
        ++length;
        t >>= 1;
    }
    const int adjusted = value < 0 ? value - 1 : value;
    const std::uint16_t mask = static_cast<std::uint16_t>((std::uint32_t{1} << length) - 1u);
    return MagnitudeBits{
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(adjusted) & mask),
        length,
    };
}

bool processDu(
    BitWriter& writer,
    float* values,
    const std::array<float,64>& fdtbl,
    int& previousDc,
    const std::array<HuffmanCode,256>& dcTable,
    const std::array<HuffmanCode,256>& acTable) {
    for (int off = 0; off < 64; off += 8) {
        dct8(values[off], values[off+1], values[off+2], values[off+3], values[off+4], values[off+5], values[off+6], values[off+7]);
    }
    for (int off = 0; off < 8; ++off) {
        dct8(values[off], values[off+8], values[off+16], values[off+24], values[off+32], values[off+40], values[off+48], values[off+56]);
    }

    std::array<int,64> du{};
    for (int i = 0; i < 64; ++i) {
        const float v = values[i] * fdtbl[static_cast<std::size_t>(i)];
        du[kZigZag[static_cast<std::size_t>(i)]] =
            static_cast<int>(v < 0.0f ? std::ceil(v - 0.5f) : std::floor(v + 0.5f));
    }

    const int diff = du[0] - previousDc;
    previousDc = du[0];
    const auto dcMagnitude = magnitudeBits(diff);
    const auto dcCode = dcTable[dcMagnitude.length];
    if (dcCode.length == 0) return false;
    writer.write(dcCode.code, dcCode.length);
    if (dcMagnitude.length != 0) writer.write(dcMagnitude.bits, dcMagnitude.length);

    int last = 63;
    while (last > 0 && du[static_cast<std::size_t>(last)] == 0) --last;
    if (last == 0) {
        const auto eob = acTable[0x00];
        if (eob.length == 0) return false;
        writer.write(eob.code, eob.length);
        return writer.ok;
    }

    int i = 1;
    while (i <= last) {
        int run = 0;
        while (i <= last && du[static_cast<std::size_t>(i)] == 0) {
            ++run;
            ++i;
        }
        while (run >= 16) {
            const auto zrl = acTable[0xf0];
            if (zrl.length == 0) return false;
            writer.write(zrl.code, zrl.length);
            run -= 16;
        }
        if (i > last) break;
        const auto magnitude = magnitudeBits(du[static_cast<std::size_t>(i)]);
        if (magnitude.length == 0 || magnitude.length > 10) return false;
        const std::uint8_t symbol = static_cast<std::uint8_t>((run << 4) | magnitude.length);
        const auto acCode = acTable[symbol];
        if (acCode.length == 0) return false;
        writer.write(acCode.code, acCode.length);
        writer.write(magnitude.bits, magnitude.length);
        ++i;
    }

    if (last != 63) {
        const auto eob = acTable[0x00];
        if (eob.length == 0) return false;
        writer.write(eob.code, eob.length);
    }
    return writer.ok;
}

void emit(jo_write_func* func, void* context, const void* data, std::size_t size) {
    func(context, data, static_cast<int>(size));
}

void emitByte(jo_write_func* func, void* context, std::uint8_t value) {
    emit(func, context, &value, 1u);
}

void emitDht(
    jo_write_func* func,
    void* context,
    std::uint8_t tableInfo,
    const std::array<std::uint8_t,17>& counts,
    const std::uint8_t* values,
    std::size_t valueCount) {
    emitByte(func, context, tableInfo);
    emit(func, context, counts.data()+1, 16u);
    emit(func, context, values, valueCount);
}

} // namespace

bool jo_write_jpg_to_func(
    jo_write_func* func,
    void* context,
    const void* data,
    int width,
    int height,
    int comp,
    int quality) {
    if (func == nullptr || context == nullptr || data == nullptr ||
        width <= 0 || height <= 0 || width > 65535 || height > 65535 ||
        comp != 3 || quality != 100) {
        return false;
    }

    std::array<HuffmanCode,256> ydc{}, yac{}, uvdc{}, uvac{};
    if (!buildHuffmanTable(kDcLumaCounts, kDcLumaValues, ydc) ||
        !buildHuffmanTable(kAcLumaCounts, kAcLumaValues, yac) ||
        !buildHuffmanTable(kDcChromaCounts, kDcChromaValues, uvdc) ||
        !buildHuffmanTable(kAcChromaCounts, kAcChromaValues, uvac)) {
        return false;
    }

    std::array<std::uint8_t,64> yTable{}, uvTable{};
    std::array<float,64> fdtblY{}, fdtblUv{};
    constexpr int scaledQuality = 0;
    for (int i = 0; i < 64; ++i) {
        const int yti = (kYqt[static_cast<std::size_t>(i)] * scaledQuality + 50) / 100;
        const int uvti = (kUvqt[static_cast<std::size_t>(i)] * scaledQuality + 50) / 100;
        yTable[kZigZag[static_cast<std::size_t>(i)]] =
            static_cast<std::uint8_t>(std::clamp(yti,1,255));
        uvTable[kZigZag[static_cast<std::size_t>(i)]] =
            static_cast<std::uint8_t>(std::clamp(uvti,1,255));
    }
    for (int row = 0, k = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col, ++k) {
            fdtblY[static_cast<std::size_t>(k)] = 1.0f /
                (yTable[kZigZag[static_cast<std::size_t>(k)]] *
                 kAasf[static_cast<std::size_t>(row)] *
                 kAasf[static_cast<std::size_t>(col)]);
            fdtblUv[static_cast<std::size_t>(k)] = 1.0f /
                (uvTable[kZigZag[static_cast<std::size_t>(k)]] *
                 kAasf[static_cast<std::size_t>(row)] *
                 kAasf[static_cast<std::size_t>(col)]);
        }
    }

    const std::uint8_t head0[] = {
        0xff,0xd8, 0xff,0xe0, 0x00,0x10, 'J','F','I','F',0x00,
        0x01,0x01, 0x00, 0x00,0x01, 0x00,0x01, 0x00,0x00,
        0xff,0xdb, 0x00,0x84, 0x00,
    };
    emit(func, context, head0, sizeof(head0));
    emit(func, context, yTable.data(), yTable.size());
    emitByte(func, context, 0x01);
    emit(func, context, uvTable.data(), uvTable.size());

    const std::uint8_t sof0[] = {
        0xff,0xc0,0x00,0x11,0x08,
        static_cast<std::uint8_t>((height >> 8) & 0xff),
        static_cast<std::uint8_t>(height & 0xff),
        static_cast<std::uint8_t>((width >> 8) & 0xff),
        static_cast<std::uint8_t>(width & 0xff),
        0x03,
        0x01,0x11,0x00,
        0x02,0x11,0x01,
        0x03,0x11,0x01,
        0xff,0xc4,0x01,0xa2,
    };
    emit(func, context, sof0, sizeof(sof0));
    emitDht(func, context, 0x00, kDcLumaCounts, kDcLumaValues.data(), kDcLumaValues.size());
    emitDht(func, context, 0x10, kAcLumaCounts, kAcLumaValues.data(), kAcLumaValues.size());
    emitDht(func, context, 0x01, kDcChromaCounts, kDcChromaValues.data(), kDcChromaValues.size());
    emitDht(func, context, 0x11, kAcChromaCounts, kAcChromaValues.data(), kAcChromaValues.size());

    const std::uint8_t sos[] = {
        0xff,0xda,0x00,0x0c,0x03,
        0x01,0x00,
        0x02,0x11,
        0x03,0x11,
        0x00,0x3f,0x00,
    };
    emit(func, context, sos, sizeof(sos));

    const auto* rgb = static_cast<const std::uint8_t*>(data);
    int dcY = 0, dcU = 0, dcV = 0;
    BitWriter writer{func, context};
    for (int y = 0; y < height; y += 8) {
        for (int x = 0; x < width; x += 8) {
            float ydu[64], udu[64], vdu[64];
            int pos = 0;
            for (int yy = 0; yy < 8; ++yy) {
                const int sy = std::min(y + yy, height - 1);
                for (int xx = 0; xx < 8; ++xx, ++pos) {
                    const int sx = std::min(x + xx, width - 1);
                    const std::size_t p =
                        (static_cast<std::size_t>(sy) * static_cast<std::size_t>(width) +
                         static_cast<std::size_t>(sx)) * 3u;
                    const float r = rgb[p];
                    const float g = rgb[p+1u];
                    const float b = rgb[p+2u];
                    ydu[pos] = +0.29900f*r + 0.58700f*g + 0.11400f*b - 128.0f;
                    udu[pos] = -0.16874f*r - 0.33126f*g + 0.50000f*b;
                    vdu[pos] = +0.50000f*r - 0.41869f*g - 0.08131f*b;
                }
            }
            if (!processDu(writer, ydu, fdtblY, dcY, ydc, yac) ||
                !processDu(writer, udu, fdtblUv, dcU, uvdc, uvac) ||
                !processDu(writer, vdu, fdtblUv, dcV, uvdc, uvac)) {
                return false;
            }
        }
    }
    writer.finish();
    if (!writer.ok) return false;
    const std::uint8_t eoi[] = {0xff,0xd9};
    emit(func, context, eoi, sizeof(eoi));
    return true;
}
