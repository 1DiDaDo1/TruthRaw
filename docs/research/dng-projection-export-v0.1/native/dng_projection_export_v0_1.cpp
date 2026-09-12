#include "dng_projection_export_v0_1.h"

#include "full_frame_streaming_v0_1_internal.h"
#include "scientific_master_digest_v0_1.h"
#include "technical_backplane_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace truthraw::dng_projection_export::v0_1 {
namespace {

using scientific_master_digest::v0_1::ScientificMasterDigestAccumulator;
using scientific_master_digest::v0_1::TileView;
using streaming_v0_1::TileRect;
using streaming_v0_1::detail::Workspace;
using streaming_v0_1::detail::fill_stage2;
using streaming_v0_1::detail::vector_bytes;

constexpr int kCanonicalCore = scientific_master_streaming_binding::v0_2::kCanonicalCore;
constexpr std::uint16_t kTypeByte = 1;
constexpr std::uint16_t kTypeAscii = 2;
constexpr std::uint16_t kTypeShort = 3;
constexpr std::uint16_t kTypeLong = 4;
constexpr std::uint16_t kTypeRational = 5;
constexpr std::uint16_t kTypeSRational = 10;
constexpr std::uint16_t kPhotometricCfa = 32803;
constexpr std::uint16_t kPhotometricLinearRaw = 34892;
constexpr std::uint16_t kCompressionNone = 1;
constexpr std::uint16_t kPlanarChunky = 1;
constexpr std::uint16_t kSampleUnsigned = 1;
constexpr std::uint16_t kCalibrationD50 = 23;
constexpr std::uint32_t kWhite16 = 65535u;
constexpr std::size_t kRawSensorHeaderBytes = 128u;
constexpr std::int64_t kMatrixDenominator = 1000000;

struct Tag final {
    std::uint16_t id = 0;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::vector<std::uint8_t> data;
    std::uint32_t externalOffset = 0;
};

struct DngLayout final {
    std::vector<std::uint8_t> header;
    std::vector<std::uint32_t> stripByteCounts;
    std::uint32_t pixelDataOffset = 0;
};

void put16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
}

void put32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 16u) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 24u) & 0xffu));
}

void put64(std::vector<std::uint8_t>& out, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8) {
        out.push_back(static_cast<std::uint8_t>((value >> static_cast<unsigned>(shift)) & 0xffu));
    }
}

void set16(std::vector<std::uint8_t>& out, std::size_t offset, std::uint16_t value) {
    out[offset] = static_cast<std::uint8_t>(value & 0xffu);
    out[offset + 1u] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
}

void set32(std::vector<std::uint8_t>& out, std::size_t offset, std::uint32_t value) {
    out[offset] = static_cast<std::uint8_t>(value & 0xffu);
    out[offset + 1u] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
    out[offset + 2u] = static_cast<std::uint8_t>((value >> 16u) & 0xffu);
    out[offset + 3u] = static_cast<std::uint8_t>((value >> 24u) & 0xffu);
}

std::uint32_t align4(std::uint32_t value) noexcept {
    return (value + 3u) & ~3u;
}

std::vector<std::uint8_t> bytes(std::initializer_list<std::uint8_t> values) {
    return std::vector<std::uint8_t>(values);
}

std::vector<std::uint8_t> ascii(std::string value) {
    value.push_back('\0');
    return std::vector<std::uint8_t>(value.begin(), value.end());
}

std::vector<std::uint8_t> shorts(std::initializer_list<std::uint16_t> values) {
    std::vector<std::uint8_t> out;
    out.reserve(values.size() * 2u);
    for (const auto value : values) put16(out, value);
    return out;
}

std::vector<std::uint8_t> longs(const std::vector<std::uint32_t>& values) {
    std::vector<std::uint8_t> out;
    out.reserve(values.size() * 4u);
    for (const auto value : values) put32(out, value);
    return out;
}

std::vector<std::uint8_t> longs(std::initializer_list<std::uint32_t> values) {
    return longs(std::vector<std::uint32_t>(values));
}

std::vector<std::uint8_t> rationals(std::initializer_list<std::pair<std::uint32_t, std::uint32_t>> values) {
    std::vector<std::uint8_t> out;
    out.reserve(values.size() * 8u);
    for (const auto& value : values) {
        put32(out, value.first);
        put32(out, value.second == 0u ? 1u : value.second);
    }
    return out;
}

std::vector<std::uint8_t> srational_zero() {
    std::vector<std::uint8_t> out;
    put32(out, 0u);
    put32(out, 1u);
    return out;
}

bool matrix_inverse(const std::array<float, 9>& a, std::array<double, 9>& out) noexcept {
    const double m0 = a[0], m1 = a[1], m2 = a[2];
    const double m3 = a[3], m4 = a[4], m5 = a[5];
    const double m6 = a[6], m7 = a[7], m8 = a[8];
    const double d = m0 * (m4 * m8 - m5 * m7) -
                     m1 * (m3 * m8 - m5 * m6) +
                     m2 * (m3 * m7 - m4 * m6);
    if (!std::isfinite(d) || std::abs(d) <= 1.0e-12) return false;
    out = {
        (m4*m8-m5*m7)/d, (m2*m7-m1*m8)/d, (m1*m5-m2*m4)/d,
        (m5*m6-m3*m8)/d, (m0*m8-m2*m6)/d, (m2*m3-m0*m5)/d,
        (m3*m7-m4*m6)/d, (m1*m6-m0*m7)/d, (m0*m4-m1*m3)/d,
    };
    for (const auto value : out) if (!std::isfinite(value)) return false;
    return true;
}

bool matrix_payload(const std::array<double, 9>& matrix, std::vector<std::uint8_t>& out) {
    out.clear();
    out.reserve(9u * 8u);
    for (const auto value : matrix) {
        if (!std::isfinite(value)) return false;
        const long double scaled = static_cast<long double>(value) *
                                   static_cast<long double>(kMatrixDenominator);
        if (scaled < static_cast<long double>(std::numeric_limits<std::int32_t>::min()) ||
            scaled > static_cast<long double>(std::numeric_limits<std::int32_t>::max())) {
            return false;
        }
        const auto numerator = static_cast<std::int32_t>(std::llround(scaled));
        put32(out, static_cast<std::uint32_t>(numerator));
        put32(out, static_cast<std::uint32_t>(kMatrixDenominator));
    }
    return true;
}

Tag tag(std::uint16_t id, std::uint16_t type, std::uint32_t count,
        std::vector<std::uint8_t> data) {
    Tag out;
    out.id = id;
    out.type = type;
    out.count = count;
    out.data = std::move(data);
    return out;
}

bool same_hash(const std::array<std::uint8_t, 32>& a,
               const std::array<std::uint8_t, 32>& b) noexcept {
    return std::equal(a.begin(), a.end(), b.begin(), b.end());
}

bool validate_authority(const AuthorityContext& a) noexcept {
    using scientific_preview_binding_v0_1::ColorClaimScope;
    if (!a.prepared.mainHouseComputeAllowed ||
        !a.prepared.sourceBoundAppearanceReleaseAllowed ||
        a.prepared.scientificPreviewReleaseAllowed ||
        a.prepared.scientificClaimAllowed ||
        a.prepared.physicalFrameCount != 1u ||
        a.prepared.independentEvidenceCount != 1u) {
        return false;
    }
    if (technical_backplane::v0_1::validate(a.phase2.backplane) !=
        technical_backplane::v0_1::Status::Ok) {
        return false;
    }
    if (a.phase2.admission.claimScope == ColorClaimScope::None) return false;
    if (a.phase2.backplane.physicalFrameCount != 1u ||
        a.phase2.backplane.independentEvidenceCount != 1u ||
        a.phase2.backplane.forbiddenFlags != 0u) {
        return false;
    }
    if (!same_hash(a.prepared.source.sha256, a.phase2.backplane.sourceEvidenceHash) ||
        !same_hash(a.prepared.source.sha256, a.phase2.admission.sourceSeal.sha256) ||
        !same_hash(a.scientificIdentity.scientificMasterHash,
                   a.phase2.backplane.scientificMasterHash)) {
        return false;
    }
    if (a.color.sourceEvidenceId != a.prepared.source.sourceEvidenceId ||
        a.color.bindingId != a.prepared.color.bindingId ||
        !a.color.validated || a.color.physicalFrameCount != 1u ||
        a.color.independentEvidenceCount != 1u) {
        return false;
    }
    return true;
}

std::array<std::uint8_t, 4> cfa_pattern(CfaPattern cfa) noexcept {
    switch (cfa) {
        case CfaPattern::BGGR: return {2u, 1u, 1u, 0u};
        case CfaPattern::RGGB: return {0u, 1u, 1u, 2u};
        case CfaPattern::GRBG: return {1u, 0u, 2u, 1u};
        case CfaPattern::GBRG: return {1u, 2u, 0u, 1u};
    }
    return {2u, 1u, 1u, 0u};
}

Status build_dng_layout(const DngMetadata& metadata,
                        const ScientificColorBindingRecord& color,
                        ProjectionKind kind,
                        int stripRows,
                        DngLayout& out) {
    if (kind != ProjectionKind::LinearDng16 && kind != ProjectionKind::CfaDng16) {
        return Status::error(StatusCode::UnsupportedProjection, "DNG layout requested for non-DNG projection");
    }
    if (metadata.width <= 0 || metadata.height <= 0 || stripRows <= 0) {
        return Status::error(StatusCode::InvalidArgument, "invalid DNG projection dimensions");
    }
    std::array<double, 9> colorMatrix{};
    if (!matrix_inverse(color.cameraToXyzD50, colorMatrix)) {
        return Status::error(StatusCode::MetadataFailed, "source-bound cameraToXyzD50 cannot be inverted for DNG ColorMatrix1");
    }
    std::array<double, 9> forward{};
    for (std::size_t i = 0; i < 9u; ++i) forward[i] = static_cast<double>(color.cameraToXyzD50[i]);
    std::vector<std::uint8_t> colorMatrixBytes;
    std::vector<std::uint8_t> forwardBytes;
    if (!matrix_payload(colorMatrix, colorMatrixBytes) || !matrix_payload(forward, forwardBytes)) {
        return Status::error(StatusCode::MetadataFailed, "DNG matrix serialization failed");
    }

    const bool linear = kind == ProjectionKind::LinearDng16;
    const std::uint16_t samplesPerPixel = linear ? 3u : 1u;
    const std::uint32_t stripCount =
        static_cast<std::uint32_t>((metadata.height + stripRows - 1) / stripRows);
    std::vector<std::uint32_t> stripByteCounts(stripCount, 0u);
    const std::uint64_t rowBytes64 = static_cast<std::uint64_t>(metadata.width) *
                                     static_cast<std::uint64_t>(samplesPerPixel) * 2u;
    if (rowBytes64 > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::MetadataFailed, "DNG row byte count exceeds classic TIFF limit");
    }
    for (std::uint32_t i = 0; i < stripCount; ++i) {
        const int y0 = static_cast<int>(i) * stripRows;
        const int rows = std::min(stripRows, metadata.height - y0);
        const std::uint64_t bytes64 = rowBytes64 * static_cast<std::uint64_t>(rows);
        if (bytes64 > std::numeric_limits<std::uint32_t>::max()) {
            return Status::error(StatusCode::MetadataFailed, "DNG strip exceeds classic TIFF limit");
        }
        stripByteCounts[i] = static_cast<std::uint32_t>(bytes64);
    }

    std::vector<Tag> tags;
    tags.reserve(40u);
    tags.push_back(tag(254, kTypeLong, 1, longs({0u})));
    tags.push_back(tag(256, kTypeLong, 1, longs({static_cast<std::uint32_t>(metadata.width)})));
    tags.push_back(tag(257, kTypeLong, 1, longs({static_cast<std::uint32_t>(metadata.height)})));
    tags.push_back(tag(258, kTypeShort, samplesPerPixel,
                       linear ? shorts({16u,16u,16u}) : shorts({16u})));
    tags.push_back(tag(259, kTypeShort, 1, shorts({kCompressionNone})));
    tags.push_back(tag(262, kTypeShort, 1,
                       shorts({linear ? kPhotometricLinearRaw : kPhotometricCfa})));
    tags.push_back(tag(270, kTypeAscii, 0,
                       ascii(linear
                           ? "TruthRaw Linear DNG compatibility projection; reconstructed camera-native RGB; not measured sensor evidence"
                           : "TruthRaw CFA DNG compatibility projection; normalized Stage-2 CFA; not raw sensor-count evidence")));
    tags.back().count = static_cast<std::uint32_t>(tags.back().data.size());
    tags.push_back(tag(271, kTypeAscii, 9, ascii("TruthRaw")));
    tags.push_back(tag(272, kTypeAscii, 31, ascii("TruthRaw Scientific Projection")));
    tags.back().count = static_cast<std::uint32_t>(tags.back().data.size());
    tags.push_back(tag(273, kTypeLong, stripCount,
                       longs(std::vector<std::uint32_t>(stripCount, 0u))));
    tags.push_back(tag(274, kTypeShort, 1, shorts({static_cast<std::uint16_t>(metadata.orientation)})));
    tags.push_back(tag(277, kTypeShort, 1, shorts({samplesPerPixel})));
    tags.push_back(tag(278, kTypeLong, 1, longs({static_cast<std::uint32_t>(stripRows)})));
    tags.push_back(tag(279, kTypeLong, stripCount, longs(stripByteCounts)));
    tags.push_back(tag(284, kTypeShort, 1, shorts({kPlanarChunky})));
    tags.push_back(tag(305, kTypeAscii, 24, ascii("TruthRaw DNG Export v0.1")));
    tags.back().count = static_cast<std::uint32_t>(tags.back().data.size());
    tags.push_back(tag(339, kTypeShort, samplesPerPixel,
                       linear ? shorts({kSampleUnsigned,kSampleUnsigned,kSampleUnsigned}) : shorts({kSampleUnsigned})));

    tags.push_back(tag(50706, kTypeByte, 4, bytes({1u,4u,0u,0u})));
    tags.push_back(tag(50707, kTypeByte, 4, bytes({1u,1u,0u,0u})));
    tags.push_back(tag(50708, kTypeAscii, 0,
                       ascii(linear ? "TruthRaw Linear Reconstructed CameraRGB v0.1"
                                    : "TruthRaw CFA Reconstructed Projection v0.1")));
    tags.back().count = static_cast<std::uint32_t>(tags.back().data.size());
    tags.push_back(tag(50713, kTypeShort, 2, shorts({1u,1u})));
    if (linear) {
        tags.push_back(tag(50714, kTypeRational, 3,
                           rationals({{0u,1u},{0u,1u},{0u,1u}})));
        tags.push_back(tag(50717, kTypeLong, 3, longs({kWhite16,kWhite16,kWhite16})));
    } else {
        tags.push_back(tag(50714, kTypeRational, 1, rationals({{0u,1u}})));
        tags.push_back(tag(50717, kTypeLong, 1, longs({kWhite16})));
    }
    tags.push_back(tag(50718, kTypeRational, 2, rationals({{1u,1u},{1u,1u}})));
    tags.push_back(tag(50719, kTypeLong, 2, longs({0u,0u})));
    tags.push_back(tag(50720, kTypeLong, 2,
                       longs({static_cast<std::uint32_t>(metadata.width),
                              static_cast<std::uint32_t>(metadata.height)})));
    tags.push_back(tag(50721, kTypeSRational, 9, std::move(colorMatrixBytes)));
    tags.push_back(tag(50728, kTypeRational, 3,
                       rationals({{1u,1u},{1u,1u},{1u,1u}})));
    tags.push_back(tag(50730, kTypeSRational, 1, srational_zero()));
    tags.push_back(tag(50731, kTypeRational, 1, rationals({{1u,1u}})));
    tags.push_back(tag(50732, kTypeRational, 1, rationals({{1u,1u}})));
    tags.push_back(tag(50734, kTypeRational, 1, rationals({{1u,1u}})));
    tags.push_back(tag(50778, kTypeShort, 1, shorts({kCalibrationD50})));
    tags.push_back(tag(50964, kTypeSRational, 9, std::move(forwardBytes)));

    if (!linear) {
        const auto cfa = cfa_pattern(metadata.cfa);
        tags.push_back(tag(33421, kTypeShort, 2, shorts({2u,2u})));
        tags.push_back(tag(33422, kTypeByte, 4, std::vector<std::uint8_t>(cfa.begin(), cfa.end())));
        tags.push_back(tag(50710, kTypeByte, 3, bytes({0u,1u,2u})));
        tags.push_back(tag(50711, kTypeShort, 1, shorts({1u})));
        tags.push_back(tag(50733, kTypeLong, 1, longs({0u})));
    }

    std::sort(tags.begin(), tags.end(), [](const Tag& a, const Tag& b) { return a.id < b.id; });
    if (tags.size() > std::numeric_limits<std::uint16_t>::max()) {
        return Status::error(StatusCode::MetadataFailed, "too many TIFF tags");
    }

    const std::uint64_t ifdEnd64 = 8u + 2u + tags.size() * 12u + 4u;
    if (ifdEnd64 > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::MetadataFailed, "TIFF IFD offset overflow");
    }
    std::uint32_t external = align4(static_cast<std::uint32_t>(ifdEnd64));
    for (auto& entry : tags) {
        if (entry.data.size() > 4u) {
            entry.externalOffset = external;
            const std::uint64_t next = static_cast<std::uint64_t>(external) + entry.data.size();
            if (next > std::numeric_limits<std::uint32_t>::max()) {
                return Status::error(StatusCode::MetadataFailed, "TIFF metadata offset overflow");
            }
            external = align4(static_cast<std::uint32_t>(next));
        }
    }
    const std::uint32_t pixelDataOffset = align4(external);

    std::vector<std::uint32_t> stripOffsets(stripCount, 0u);
    std::uint64_t running = pixelDataOffset;
    for (std::uint32_t i = 0; i < stripCount; ++i) {
        if (running > std::numeric_limits<std::uint32_t>::max()) {
            return Status::error(StatusCode::MetadataFailed, "classic TIFF output exceeds 4 GiB offset space");
        }
        stripOffsets[i] = static_cast<std::uint32_t>(running);
        running += stripByteCounts[i];
    }
    if (running > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::MetadataFailed, "classic TIFF output exceeds 4 GiB");
    }
    for (auto& entry : tags) {
        if (entry.id == 273) {
            entry.data = longs(stripOffsets);
            break;
        }
    }

    std::vector<std::uint8_t> header(pixelDataOffset, 0u);
    header[0] = 'I'; header[1] = 'I';
    set16(header, 2u, 42u);
    set32(header, 4u, 8u);
    set16(header, 8u, static_cast<std::uint16_t>(tags.size()));
    std::size_t entryOffset = 10u;
    for (const auto& entry : tags) {
        set16(header, entryOffset + 0u, entry.id);
        set16(header, entryOffset + 2u, entry.type);
        set32(header, entryOffset + 4u, entry.count);
        if (entry.data.size() <= 4u) {
            std::copy(entry.data.begin(), entry.data.end(), header.begin() + static_cast<std::ptrdiff_t>(entryOffset + 8u));
        } else {
            set32(header, entryOffset + 8u, entry.externalOffset);
            std::copy(entry.data.begin(), entry.data.end(),
                      header.begin() + static_cast<std::ptrdiff_t>(entry.externalOffset));
        }
        entryOffset += 12u;
    }
    set32(header, entryOffset, 0u);

    out.header = std::move(header);
    out.stripByteCounts = std::move(stripByteCounts);
    out.pixelDataOffset = pixelDataOffset;
    return Status::ok();
}

bool write_bytes(ISequentialByteSink& sink, const void* data, std::size_t bytes,
                 std::uint64_t& total) noexcept {
    if (bytes == 0u) return true;
    if (!sink.writeExact(data, bytes)) return false;
    total += static_cast<std::uint64_t>(bytes);
    return true;
}

void encode_u16(std::vector<std::uint8_t>& dst, std::size_t sampleIndex,
                std::uint16_t value) noexcept {
    const std::size_t offset = sampleIndex * 2u;
    dst[offset] = static_cast<std::uint8_t>(value & 0xffu);
    dst[offset + 1u] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
}

void encode_f32(std::vector<std::uint8_t>& dst, std::size_t sampleIndex, float value) noexcept {
    std::uint32_t bits = 0u;
    static_assert(sizeof(bits) == sizeof(value));
    std::memcpy(&bits, &value, sizeof(bits));
    const std::size_t offset = sampleIndex * 4u;
    dst[offset] = static_cast<std::uint8_t>(bits & 0xffu);
    dst[offset + 1u] = static_cast<std::uint8_t>((bits >> 8u) & 0xffu);
    dst[offset + 2u] = static_cast<std::uint8_t>((bits >> 16u) & 0xffu);
    dst[offset + 3u] = static_cast<std::uint8_t>((bits >> 24u) & 0xffu);
}

bool quantize16(float value, std::uint16_t& out,
                std::uint64_t& negative, std::uint64_t& overOne) noexcept {
    if (!std::isfinite(value)) return false;
    if (value < 0.0f) {
        ++negative;
        value = 0.0f;
    } else if (value > 1.0f) {
        ++overOne;
        value = 1.0f;
    }
    const float scaled = value * 65535.0f;
    const long rounded = std::lround(scaled);
    out = static_cast<std::uint16_t>(std::clamp<long>(rounded, 0l, 65535l));
    return true;
}

Status write_rawsensor_header(const DngMetadata& metadata,
                              const AuthorityContext& authority,
                              ISequentialByteSink& sink,
                              std::uint64_t& total) {
    std::vector<std::uint8_t> header;
    header.reserve(kRawSensorHeaderBytes);
    const char magic[8] = {'T','R','R','A','W','S','0','1'};
    header.insert(header.end(), magic, magic + 8);
    put32(header, 1u);
    put32(header, static_cast<std::uint32_t>(kRawSensorHeaderBytes));
    put32(header, static_cast<std::uint32_t>(metadata.width));
    put32(header, static_cast<std::uint32_t>(metadata.height));
    put32(header, 3u); // camera-native reconstructed RGB channels
    put32(header, 3u); // TIFF SampleFormat code 3 = IEEE floating point
    put32(header, 32u);
    put32(header, static_cast<std::uint32_t>(metadata.orientation));
    put32(header, static_cast<std::uint32_t>(metadata.cfa));
    header.insert(header.end(), authority.prepared.source.sha256.begin(), authority.prepared.source.sha256.end());
    header.insert(header.end(), authority.scientificIdentity.scientificMasterHash.begin(),
                  authority.scientificIdentity.scientificMasterHash.end());
    std::uint64_t l0Bits = 0u;
    const double l0 = authority.scientificIdentity.zeroLineGauge.L0;
    static_assert(sizeof(l0Bits) == sizeof(l0));
    std::memcpy(&l0Bits, &l0, sizeof(l0Bits));
    put64(header, l0Bits);
    put32(header, 1u);
    put32(header, 1u);
    put32(header, static_cast<std::uint32_t>(authority.phase2.admission.claimScope));
    put32(header, 0u); // reserved
    if (header.size() > kRawSensorHeaderBytes) {
        return Status::error(StatusCode::MetadataFailed, "rawsensor header exceeded fixed v0.1 size");
    }
    header.resize(kRawSensorHeaderBytes, 0u);
    if (!write_bytes(sink, header.data(), header.size(), total)) {
        return Status::error(StatusCode::SinkFailed, "failed writing rawsensor header");
    }
    return Status::ok();
}

Status export_impl(streaming_v0_1::IRawTileSource& source,
                   IReconstructionBackend& reconstruction,
                   const AuthorityContext& authority,
                   ProjectionKind kind,
                   ISequentialByteSink& sink,
                   const Options& options,
                   Result& out) {
    out = {};
    out.kind = kind;
    if (!validate_authority(authority)) {
        return Status::error(StatusCode::AuthorityRejected,
                             "projection export requires a valid phase-2 finalized source/master lineage");
    }
    const auto& metadata = source.metadata();
    if (metadata.width <= 1 || metadata.height <= 1 || options.stripRows != kCanonicalCore) {
        return Status::error(StatusCode::InvalidArgument,
                             "v0.1 export requires valid dimensions and canonical 64-row strips");
    }
    const int halo = reconstruction.requiredHalo();
    if (halo < 0) {
        return Status::error(StatusCode::InvalidArgument, "reconstruction backend returned negative halo");
    }
    if (kind != ProjectionKind::LinearDng16 &&
        kind != ProjectionKind::CfaDng16 &&
        kind != ProjectionKind::ScientificRawSensorF32) {
        return Status::error(StatusCode::UnsupportedProjection, "unknown projection kind");
    }

    std::uint64_t bytesWritten = 0u;
    if (kind == ProjectionKind::ScientificRawSensorF32) {
        const auto headerStatus = write_rawsensor_header(metadata, authority, sink, bytesWritten);
        if (!headerStatus) return headerStatus;
    } else {
        DngLayout layout;
        const auto layoutStatus = build_dng_layout(metadata, authority.color, kind,
                                                   options.stripRows, layout);
        if (!layoutStatus) return layoutStatus;
        if (!write_bytes(sink, layout.header.data(), layout.header.size(), bytesWritten)) {
            return Status::error(StatusCode::SinkFailed, "failed writing DNG header/IFD");
        }
    }

    ScientificMasterDigestAccumulator digest(static_cast<std::uint32_t>(metadata.width),
                                             static_cast<std::uint32_t>(metadata.height));
    if (!digest.valid()) {
        return Status::error(StatusCode::DigestFailed,
                             "Scientific Master digest accumulator initialization failed");
    }

    Workspace workspace{};
    std::vector<std::uint8_t> strip;
    std::size_t workspacePeak = 0u;
    std::size_t residentPeak = source.residentBytesUpperBound() +
                               digest.metrics().residentBytesUpperBound;
    const std::size_t samplesPerPixel =
        kind == ProjectionKind::CfaDng16 ? 1u : 3u;
    const std::size_t bytesPerSample =
        kind == ProjectionKind::ScientificRawSensorF32 ? 4u : 2u;

    for (int y0 = 0; y0 < metadata.height; y0 += kCanonicalCore) {
        const int y1 = std::min(metadata.height, y0 + kCanonicalCore);
        const int rows = y1 - y0;
        const std::size_t stripSamples = static_cast<std::size_t>(rows) *
                                         static_cast<std::size_t>(metadata.width) *
                                         samplesPerPixel;
        if (stripSamples > std::numeric_limits<std::size_t>::max() / bytesPerSample) {
            return Status::error(StatusCode::BudgetExceeded, "projection strip size overflow");
        }
        strip.assign(stripSamples * bytesPerSample, 0u);

        for (int x0 = 0; x0 < metadata.width; x0 += kCanonicalCore) {
            const int x1 = std::min(metadata.width, x0 + kCanonicalCore);
            TileRect tile{};
            tile.x0 = x0; tile.y0 = y0; tile.x1 = x1; tile.y1 = y1;
            tile.hx0 = std::max(0, x0 - halo);
            tile.hy0 = std::max(0, y0 - halo);
            tile.hx1 = std::min(metadata.width, x1 + halo);
            tile.hy1 = std::min(metadata.height, y1 + halo);

            const auto fill = fill_stage2(source, tile, workspace);
            if (!fill) {
                return Status::error(StatusCode::SourceFailed,
                                     "projection Stage-2 read failed: " + fill.message);
            }
            const int tileW = tile.hx1 - tile.hx0;
            const int tileH = tile.hy1 - tile.hy0;
            const int coreW = x1 - x0;
            const int coreH = y1 - y0;
            const std::size_t coreSamples = static_cast<std::size_t>(coreW) *
                                            static_cast<std::size_t>(coreH);
            workspace.cam.resize(coreSamples * 3u);
            const auto reconstructed = reconstruction.reconstructTile(
                workspace.stage2.data(), tileW, tileH,
                tile.hx0, tile.hy0, tile.x0, tile.y0,
                coreW, coreH, metadata.cfa, workspace.cam.data());
            if (!reconstructed) {
                return Status::error(StatusCode::ReconstructionFailed,
                                     "projection reconstruction failed: " + reconstructed.message);
            }

            TileView digestTile{};
            digestTile.x = static_cast<std::uint32_t>(x0);
            digestTile.y = static_cast<std::uint32_t>(y0);
            digestTile.width = static_cast<std::uint32_t>(coreW);
            digestTile.height = static_cast<std::uint32_t>(coreH);
            digestTile.rgb = workspace.cam.data();
            digestTile.rowStrideSamples = static_cast<std::size_t>(coreW) * 3u;
            if (!digest.add_tile(digestTile)) {
                return Status::error(StatusCode::DigestFailed,
                                     "projection Scientific Master digest tile rejected: " + digest.error());
            }

            for (int cy = 0; cy < coreH; ++cy) {
                for (int cx = 0; cx < coreW; ++cx) {
                    const int globalX = x0 + cx;
                    const int globalY = y0 + cy;
                    const std::size_t stripPixel =
                        static_cast<std::size_t>(globalY - y0) * static_cast<std::size_t>(metadata.width) +
                        static_cast<std::size_t>(globalX);
                    const std::size_t corePixel =
                        static_cast<std::size_t>(cy) * static_cast<std::size_t>(coreW) +
                        static_cast<std::size_t>(cx);

                    if (kind == ProjectionKind::CfaDng16) {
                        const int localX = globalX - tile.hx0;
                        const int localY = globalY - tile.hy0;
                        const std::size_t stageIndex =
                            static_cast<std::size_t>(localY) * static_cast<std::size_t>(tileW) +
                            static_cast<std::size_t>(localX);
                        if (stageIndex >= workspace.stage2.size()) {
                            return Status::error(StatusCode::SourceFailed,
                                                 "Stage-2 projection index escaped tile workspace");
                        }
                        std::uint16_t q = 0u;
                        if (!quantize16(workspace.stage2[stageIndex], q,
                                        out.negativeSamplesClamped,
                                        out.overOneSamplesClamped)) {
                            return Status::error(StatusCode::SourceFailed,
                                                 "non-finite Stage-2 sample cannot be projected to integer DNG");
                        }
                        encode_u16(strip, stripPixel, q);
                    } else if (kind == ProjectionKind::LinearDng16) {
                        for (std::size_t channel = 0; channel < 3u; ++channel) {
                            std::uint16_t q = 0u;
                            if (!quantize16(workspace.cam[3u * corePixel + channel], q,
                                            out.negativeSamplesClamped,
                                            out.overOneSamplesClamped)) {
                                return Status::error(StatusCode::ReconstructionFailed,
                                                     "non-finite Scientific Master sample cannot be projected to integer DNG");
                            }
                            encode_u16(strip, 3u * stripPixel + channel, q);
                        }
                    } else {
                        for (std::size_t channel = 0; channel < 3u; ++channel) {
                            const float value = workspace.cam[3u * corePixel + channel];
                            if (!std::isfinite(value)) {
                                return Status::error(StatusCode::ReconstructionFailed,
                                                     "non-finite Scientific Master sample cannot be serialized");
                            }
                            encode_f32(strip, 3u * stripPixel + channel, value);
                        }
                    }
                }
            }

            ++out.tilesProcessed;
            workspacePeak = std::max(workspacePeak, vector_bytes(workspace) + strip.capacity());
            const std::size_t resident = source.residentBytesUpperBound() +
                                         digest.metrics().residentBytesUpperBound + workspacePeak;
            residentPeak = std::max(residentPeak, resident);
            if (options.memoryBudgetBytes != 0u && residentPeak > options.memoryBudgetBytes) {
                return Status::error(StatusCode::BudgetExceeded,
                                     "projection export exceeded caller logical memory budget");
            }
        }

        if (!write_bytes(sink, strip.data(), strip.size(), bytesWritten)) {
            return Status::error(StatusCode::SinkFailed, "failed writing projection pixel strip");
        }
    }

    std::array<std::uint8_t, 32> exportedHash{};
    if (!digest.finalize(exportedHash)) {
        return Status::error(StatusCode::DigestFailed,
                             "projection Scientific Master digest finalization failed: " + digest.error());
    }
    if (!same_hash(exportedHash, authority.scientificIdentity.scientificMasterHash) ||
        !same_hash(exportedHash, authority.phase2.backplane.scientificMasterHash)) {
        return Status::error(StatusCode::ScientificIdentityMismatch,
                             "export reconstruction differs from finalized Scientific Master identity");
    }

    out.bytesWritten = bytesWritten;
    out.logicalWorkspacePeakBytes = workspacePeak;
    out.logicalResidentUpperBound = residentPeak;
    out.scientificMasterDigestVerified = true;
    out.fullScientificMasterMaterialized = false;
    out.compatibilityProjection = kind != ProjectionKind::ScientificRawSensorF32;
    out.physicalFrameCount = 1u;
    out.independentEvidenceCount = 1u;
    return Status::ok();
}

} // namespace

Status export_projection(streaming_v0_1::IRawTileSource& source,
                         IReconstructionBackend& reconstruction,
                         const AuthorityContext& authority,
                         ProjectionKind kind,
                         ISequentialByteSink& sink,
                         const Options& options,
                         Result& out) noexcept {
    try {
        return export_impl(source, reconstruction, authority, kind, sink, options, out);
    } catch (const std::bad_alloc&) {
        out = {};
        return Status::error(StatusCode::BudgetExceeded, "projection allocation failed");
    } catch (...) {
        out = {};
        return Status::error(StatusCode::MetadataFailed, "unexpected projection-export exception");
    }
}

const char* status_name(StatusCode code) noexcept {
    switch (code) {
        case StatusCode::Ok: return "OK";
        case StatusCode::InvalidArgument: return "INVALID_ARGUMENT";
        case StatusCode::AuthorityRejected: return "AUTHORITY_REJECTED";
        case StatusCode::BudgetExceeded: return "BUDGET_EXCEEDED";
        case StatusCode::SourceFailed: return "SOURCE_FAILED";
        case StatusCode::ReconstructionFailed: return "RECONSTRUCTION_FAILED";
        case StatusCode::DigestFailed: return "DIGEST_FAILED";
        case StatusCode::ScientificIdentityMismatch: return "SCIENTIFIC_IDENTITY_MISMATCH";
        case StatusCode::UnsupportedProjection: return "UNSUPPORTED_PROJECTION";
        case StatusCode::SinkFailed: return "SINK_FAILED";
        case StatusCode::MetadataFailed: return "METADATA_FAILED";
    }
    return "UNKNOWN";
}

const char* projection_name(ProjectionKind kind) noexcept {
    switch (kind) {
        case ProjectionKind::LinearDng16: return "LINEAR_DNG_16_COMPATIBILITY_PROJECTION";
        case ProjectionKind::CfaDng16: return "CFA_DNG_16_RECONSTRUCTED_PROJECTION";
        case ProjectionKind::ScientificRawSensorF32: return "SCIENTIFIC_MASTER_CAMERA_RGB_F32_RAWSENSOR";
    }
    return "UNKNOWN";
}

} // namespace truthraw::dng_projection_export::v0_1
