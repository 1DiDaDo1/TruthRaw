#include "linear_dng_projection_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>
#include <vector>

namespace truthraw::linear_dng_projection::v0_1 {
namespace {

using Hash256 = std::array<std::uint8_t, 32>;

constexpr std::uint16_t kTiffTypeByte = 1;
constexpr std::uint16_t kTiffTypeAscii = 2;
constexpr std::uint16_t kTiffTypeShort = 3;
constexpr std::uint16_t kTiffTypeLong = 4;
constexpr std::uint16_t kTiffTypeRational = 5;
constexpr std::uint16_t kTiffTypeSRational = 10;

constexpr std::uint16_t kTagNewSubFileType = 254;
constexpr std::uint16_t kTagImageWidth = 256;
constexpr std::uint16_t kTagImageLength = 257;
constexpr std::uint16_t kTagBitsPerSample = 258;
constexpr std::uint16_t kTagCompression = 259;
constexpr std::uint16_t kTagPhotometricInterpretation = 262;
constexpr std::uint16_t kTagImageDescription = 270;
constexpr std::uint16_t kTagOrientation = 274;
constexpr std::uint16_t kTagSamplesPerPixel = 277;
constexpr std::uint16_t kTagPlanarConfiguration = 284;
constexpr std::uint16_t kTagSoftware = 305;
constexpr std::uint16_t kTagTileWidth = 322;
constexpr std::uint16_t kTagTileLength = 323;
constexpr std::uint16_t kTagTileOffsets = 324;
constexpr std::uint16_t kTagTileByteCounts = 325;
constexpr std::uint16_t kTagSampleFormat = 339;
constexpr std::uint16_t kTagDngVersion = 50706;
constexpr std::uint16_t kTagDngBackwardVersion = 50707;
constexpr std::uint16_t kTagUniqueCameraModel = 50708;
constexpr std::uint16_t kTagWhiteLevel = 50717;
constexpr std::uint16_t kTagColorMatrix1 = 50721;
constexpr std::uint16_t kTagAsShotNeutral = 50728;
constexpr std::uint16_t kTagCalibrationIlluminant1 = 50778;

constexpr std::uint16_t kPhotometricLinearRaw = 34892;
constexpr std::uint16_t kCompressionNone = 1;
constexpr std::uint16_t kPlanarChunky = 1;
constexpr std::uint16_t kSampleFormatUnsigned = 1;
constexpr std::uint16_t kCalibrationIlluminantD50 = 23;
constexpr std::uint16_t kBitsPerSample = 16;
constexpr std::uint32_t kWhiteLevel = 65535u;
constexpr std::size_t kIfdEntryCount = 21;
constexpr double kD50X = 0.96422;
constexpr double kD50Y = 1.0;
constexpr double kD50Z = 0.82521;
constexpr double kMatrixSingularEpsilon = 1.0e-12;
constexpr std::int64_t kRationalDenominator = 1000000;

struct Mat3 final {
    std::array<double, 9> v{};
};

struct Vec3 final {
    std::array<double, 3> v{};
};

struct IfdEntry final {
    std::uint16_t tag = 0;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::uint32_t value = 0;
};

bool hash_is_zero(const Hash256& hash) noexcept {
    for (const auto b : hash) {
        if (b != 0) return false;
    }
    return true;
}

bool same_hash(const Hash256& a, const Hash256& b) noexcept {
    return a == b;
}

std::string hex_hash(const Hash256& hash) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto b : hash) ss << std::setw(2) << static_cast<unsigned>(b);
    return ss.str();
}

std::uint32_t align4(std::uint32_t value) noexcept {
    return (value + 3u) & ~3u;
}

void put_u16(std::vector<std::uint8_t>& bytes,
             std::size_t offset,
             std::uint16_t value) {
    bytes[offset + 0] = static_cast<std::uint8_t>(value & 0xffu);
    bytes[offset + 1] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
}

void put_u32(std::vector<std::uint8_t>& bytes,
             std::size_t offset,
             std::uint32_t value) {
    bytes[offset + 0] = static_cast<std::uint8_t>(value & 0xffu);
    bytes[offset + 1] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
    bytes[offset + 2] = static_cast<std::uint8_t>((value >> 16u) & 0xffu);
    bytes[offset + 3] = static_cast<std::uint8_t>((value >> 24u) & 0xffu);
}

void put_i32(std::vector<std::uint8_t>& bytes,
             std::size_t offset,
             std::int32_t value) {
    put_u32(bytes, offset, static_cast<std::uint32_t>(value));
}

void append_u16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
}

void append_u32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16u) & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 24u) & 0xffu));
}

void append_i32(std::vector<std::uint8_t>& bytes, std::int32_t value) {
    append_u32(bytes, static_cast<std::uint32_t>(value));
}

std::uint32_t add_blob(std::vector<std::uint8_t>& extras,
                       std::uint32_t extraStart,
                       const std::vector<std::uint8_t>& data,
                       std::size_t alignment = 2u) {
    const std::uint32_t current = extraStart + static_cast<std::uint32_t>(extras.size());
    const std::uint32_t aligned = alignment == 4u ? align4(current)
                                                   : ((current + 1u) & ~1u);
    extras.insert(extras.end(), aligned - current, 0u);
    const std::uint32_t offset = extraStart + static_cast<std::uint32_t>(extras.size());
    extras.insert(extras.end(), data.begin(), data.end());
    return offset;
}

std::vector<std::uint8_t> ascii_blob(const std::string& text) {
    std::vector<std::uint8_t> out(text.begin(), text.end());
    out.push_back(0);
    return out;
}

std::vector<std::uint8_t> shorts_blob(std::initializer_list<std::uint16_t> values) {
    std::vector<std::uint8_t> out;
    out.reserve(values.size() * 2u);
    for (const auto v : values) append_u16(out, v);
    return out;
}

std::vector<std::uint8_t> longs_blob(const std::vector<std::uint32_t>& values) {
    std::vector<std::uint8_t> out;
    out.reserve(values.size() * 4u);
    for (const auto v : values) append_u32(out, v);
    return out;
}

double determinant(const Mat3& m) noexcept {
    const auto& a = m.v;
    return a[0] * (a[4] * a[8] - a[5] * a[7]) -
           a[1] * (a[3] * a[8] - a[5] * a[6]) +
           a[2] * (a[3] * a[7] - a[4] * a[6]);
}

bool inverse(const Mat3& m, Mat3& out) noexcept {
    const double d = determinant(m);
    if (!std::isfinite(d) || std::abs(d) <= kMatrixSingularEpsilon) return false;
    const auto& a = m.v;
    auto& r = out.v;
    r[0] =  (a[4] * a[8] - a[5] * a[7]) / d;
    r[1] = -(a[1] * a[8] - a[2] * a[7]) / d;
    r[2] =  (a[1] * a[5] - a[2] * a[4]) / d;
    r[3] = -(a[3] * a[8] - a[5] * a[6]) / d;
    r[4] =  (a[0] * a[8] - a[2] * a[6]) / d;
    r[5] = -(a[0] * a[5] - a[2] * a[3]) / d;
    r[6] =  (a[3] * a[7] - a[4] * a[6]) / d;
    r[7] = -(a[0] * a[7] - a[1] * a[6]) / d;
    r[8] =  (a[0] * a[4] - a[1] * a[3]) / d;
    for (const auto v : r) {
        if (!std::isfinite(v) || std::abs(v) > 1000.0) return false;
    }
    return true;
}

Vec3 mul(const Mat3& m, const Vec3& x) noexcept {
    Vec3 out;
    for (int row = 0; row < 3; ++row) {
        out.v[row] = m.v[row * 3 + 0] * x.v[0] +
                     m.v[row * 3 + 1] * x.v[1] +
                     m.v[row * 3 + 2] * x.v[2];
    }
    return out;
}

bool make_srational_blob(const Mat3& matrix, std::vector<std::uint8_t>& out) {
    out.clear();
    out.reserve(9u * 8u);
    for (const double v : matrix.v) {
        if (!std::isfinite(v)) return false;
        const double scaled = std::round(v * static_cast<double>(kRationalDenominator));
        if (scaled < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
            scaled > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
            return false;
        }
        append_i32(out, static_cast<std::int32_t>(scaled));
        append_i32(out, static_cast<std::int32_t>(kRationalDenominator));
    }
    return true;
}

bool make_rational_blob(const Vec3& value, std::vector<std::uint8_t>& out) {
    out.clear();
    out.reserve(3u * 8u);
    for (const double v : value.v) {
        if (!std::isfinite(v) || !(v > 0.0)) return false;
        const double scaled = std::round(v * static_cast<double>(kRationalDenominator));
        if (!(scaled > 0.0) ||
            scaled > static_cast<double>(std::numeric_limits<std::uint32_t>::max())) {
            return false;
        }
        append_u32(out, static_cast<std::uint32_t>(scaled));
        append_u32(out, static_cast<std::uint32_t>(kRationalDenominator));
    }
    return true;
}

void write_ifd_entry(std::vector<std::uint8_t>& prefix,
                     std::size_t entryOffset,
                     const IfdEntry& entry) {
    put_u16(prefix, entryOffset + 0u, entry.tag);
    put_u16(prefix, entryOffset + 2u, entry.type);
    put_u32(prefix, entryOffset + 4u, entry.count);
    put_u32(prefix, entryOffset + 8u, entry.value);
}

std::uint32_t inline_bytes4(std::array<std::uint8_t, 4> bytes) noexcept {
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8u) |
           (static_cast<std::uint32_t>(bytes[2]) << 16u) |
           (static_cast<std::uint32_t>(bytes[3]) << 24u);
}

}  // namespace

Status admit_linear_dng_projection(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const finalized_scientific_preview_release::v0_2::ReleaseResult& release,
    ProjectionAdmission& out) noexcept {
    using finalized_scientific_preview_release::v0_2::PreviewAuthority;
    using scientific_preview_binding_v0_1::ColorBindingAuthority;

    ProjectionAdmission result;
    if (release.authority == PreviewAuthority::None) {
        return Status::error(StatusCode::FinalizedLineageRequired,
                             "Linear DNG projection requires a finalized Scientific Preview lineage");
    }
    if (!prepared.mainHouseComputeAllowed || !prepared.color.validated) {
        return Status::error(StatusCode::FinalizedLineageRequired,
                             "prepared source/color lineage is not valid for Main-House projection");
    }
    if (prepared.physicalFrameCount != 1u ||
        prepared.independentEvidenceCount != 1u ||
        release.scientificIdentity.physicalFrameCount != 1u ||
        release.scientificIdentity.independentEvidenceCount != 1u ||
        release.canonicalPhase2.backplane.physicalFrameCount != 1u ||
        release.canonicalPhase2.backplane.independentEvidenceCount != 1u) {
        return Status::error(StatusCode::FinalizedLineageRequired,
                             "v0.1 Linear DNG projection requires one physical frame/evidence root");
    }
    if (prepared.color.authority == ColorBindingAuthority::Unverified ||
        prepared.color.authority == ColorBindingAuthority::PreviewSentinel) {
        return Status::error(StatusCode::UnauthorizedColorBinding,
                             "unverified/sentinel color cannot enter Linear DNG projection");
    }
    const auto& admissionSeal = release.canonicalPhase2.admission.sourceSeal;
    if (!same_hash(prepared.source.sha256, admissionSeal.sha256) ||
        prepared.source.byteLength != admissionSeal.byteLength ||
        prepared.source.sourceEvidenceId != admissionSeal.sourceEvidenceId ||
        !same_hash(prepared.source.sha256,
                   release.canonicalPhase2.backplane.sourceEvidenceHash)) {
        return Status::error(StatusCode::SourceIdentityMismatch,
                             "finalized release source identity does not match prepared source");
    }
    if (hash_is_zero(release.scientificIdentity.scientificMasterHash) ||
        !same_hash(release.scientificIdentity.scientificMasterHash,
                   release.canonicalPhase2.backplane.scientificMasterHash)) {
        return Status::error(StatusCode::ScientificIdentityMismatch,
                             "finalized release Scientific Master identity is missing/mismatched");
    }
    for (const float value : prepared.color.cameraToXyzD50) {
        if (!std::isfinite(value)) {
            return Status::error(StatusCode::InvalidArgument,
                                 "cameraToXyzD50 contains non-finite value");
        }
    }

    result.source = prepared.source;
    result.scientificMasterHash = release.scientificIdentity.scientificMasterHash;
    result.cameraToXyzD50 = prepared.color.cameraToXyzD50;
    result.colorAuthority = prepared.color.authority;
    result.strongerPhysicalColorClaim =
        release.authority == PreviewAuthority::FinalizedIndependentlyCalibratedScientificPreview;
    result.physicalFrameCount = 1;
    result.independentEvidenceCount = 1;
    out = result;
    return Status::ok();
}

Status write_linear_dng(
    ICameraRgbTileSource& source,
    ISequentialByteSink& sink,
    const ProjectionAdmission& admission,
    const Options& options,
    Audit& out) noexcept {
    Audit audit;
    audit.strongerPhysicalColorClaim = admission.strongerPhysicalColorClaim;
    audit.sourceMetadataColorOnly =
        admission.colorAuthority != scientific_preview_binding_v0_1::ColorBindingAuthority::IndependentCalibration;
    audit.physicalFrameCount = admission.physicalFrameCount;
    audit.independentEvidenceCount = admission.independentEvidenceCount;

    const int width = source.width();
    const int height = source.height();
    if (width <= 0 || height <= 0 || options.tileEdge <= 0 || options.tileEdge > 1024 ||
        !std::isfinite(options.linearScale) || !(options.linearScale > 0.0) ||
        options.uniqueCameraModel.size() < 4u || options.software.size() < 4u ||
        sink.bytesWritten() != 0u || admission.physicalFrameCount != 1u ||
        admission.independentEvidenceCount != 1u || hash_is_zero(admission.scientificMasterHash)) {
        return Status::error(StatusCode::InvalidArgument,
                             "invalid Linear DNG projection arguments/admission");
    }

    Mat3 cameraToXyz;
    for (std::size_t i = 0; i < 9u; ++i) {
        cameraToXyz.v[i] = static_cast<double>(admission.cameraToXyzD50[i]);
        if (!std::isfinite(cameraToXyz.v[i])) {
            return Status::error(StatusCode::InvalidArgument,
                                 "cameraToXyzD50 contains non-finite value");
        }
    }
    Mat3 xyzToCamera;
    if (!inverse(cameraToXyz, xyzToCamera)) {
        return Status::error(StatusCode::SingularColorMatrix,
                             "cameraToXyzD50 cannot be inverted into DNG ColorMatrix1");
    }

    Vec3 d50{{kD50X, kD50Y, kD50Z}};
    Vec3 neutral = mul(xyzToCamera, d50);
    if (!std::isfinite(neutral.v[1]) || !(neutral.v[1] > 0.0)) {
        return Status::error(StatusCode::InvalidNeutral,
                             "effective D50 profile produced invalid camera neutral");
    }
    for (double& v : neutral.v) v /= neutral.v[1];
    for (const double v : neutral.v) {
        if (!std::isfinite(v) || !(v > 0.0) || v > 64.0) {
            return Status::error(StatusCode::InvalidNeutral,
                                 "effective AsShotNeutral is outside projection bounds");
        }
    }

    const std::uint64_t tilesX =
        (static_cast<std::uint64_t>(width) + static_cast<std::uint64_t>(options.tileEdge) - 1u) /
        static_cast<std::uint64_t>(options.tileEdge);
    const std::uint64_t tilesY =
        (static_cast<std::uint64_t>(height) + static_cast<std::uint64_t>(options.tileEdge) - 1u) /
        static_cast<std::uint64_t>(options.tileEdge);
    const std::uint64_t tileCount64 = tilesX * tilesY;
    const std::uint64_t samplesPerTile =
        static_cast<std::uint64_t>(options.tileEdge) *
        static_cast<std::uint64_t>(options.tileEdge) * 3u;
    const std::uint64_t tileBytes64 = samplesPerTile * sizeof(std::uint16_t);
    if (tileCount64 == 0u || tileCount64 > std::numeric_limits<std::uint32_t>::max() ||
        tileBytes64 > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::OutputTooLarge,
                             "Linear DNG tile geometry exceeds classic TIFF bounds");
    }
    const std::uint32_t tileCount = static_cast<std::uint32_t>(tileCount64);
    const std::uint32_t tileBytes = static_cast<std::uint32_t>(tileBytes64);

    const std::uint32_t ifdOffset = 8u;
    const std::uint32_t ifdBytes = 2u + static_cast<std::uint32_t>(kIfdEntryCount) * 12u + 4u;
    const std::uint32_t extraStart = align4(ifdOffset + ifdBytes);
    std::vector<std::uint8_t> extras;

    const auto bits = shorts_blob({kBitsPerSample, kBitsPerSample, kBitsPerSample});
    const std::uint32_t bitsOffset = add_blob(extras, extraStart, bits, 2u);

    const std::string description =
        std::string("TruthRaw role=LINEAR_DNG_COMPATIBILITY_PROJECTION; creates_evidence=0; ") +
        "source=" + admission.source.sourceEvidenceId +
        "; scientific_master_sha256=" + hex_hash(admission.scientificMasterHash) +
        "; color_authority=" +
        scientific_preview_binding_v0_1::authority_name(admission.colorAuthority) +
        "; stronger_physical_color_claim=" +
        (admission.strongerPhysicalColorClaim ? "1" : "0");
    const auto descriptionData = ascii_blob(description);
    const std::uint32_t descriptionOffset = add_blob(extras, extraStart, descriptionData, 2u);

    const auto softwareData = ascii_blob(options.software);
    const std::uint32_t softwareOffset = add_blob(extras, extraStart, softwareData, 2u);

    std::vector<std::uint32_t> zeroOffsets(tileCount, 0u);
    const auto tileOffsetsData = longs_blob(zeroOffsets);
    const std::uint32_t tileOffsetsOffset =
        tileCount == 1u ? 0u : add_blob(extras, extraStart, tileOffsetsData, 4u);
    const std::size_t tileOffsetsRelative =
        tileCount == 1u ? 0u : static_cast<std::size_t>(tileOffsetsOffset - extraStart);

    std::vector<std::uint32_t> byteCounts(tileCount, tileBytes);
    const auto tileByteCountsData = longs_blob(byteCounts);
    const std::uint32_t tileByteCountsOffset =
        tileCount == 1u ? 0u : add_blob(extras, extraStart, tileByteCountsData, 4u);

    const auto sampleFormatData = shorts_blob({kSampleFormatUnsigned,
                                                kSampleFormatUnsigned,
                                                kSampleFormatUnsigned});
    const std::uint32_t sampleFormatOffset =
        add_blob(extras, extraStart, sampleFormatData, 2u);

    const auto modelData = ascii_blob(options.uniqueCameraModel);
    const std::uint32_t modelOffset = add_blob(extras, extraStart, modelData, 2u);

    const auto whiteData = shorts_blob({static_cast<std::uint16_t>(kWhiteLevel),
                                        static_cast<std::uint16_t>(kWhiteLevel),
                                        static_cast<std::uint16_t>(kWhiteLevel)});
    const std::uint32_t whiteOffset = add_blob(extras, extraStart, whiteData, 2u);

    std::vector<std::uint8_t> colorMatrixData;
    if (!make_srational_blob(xyzToCamera, colorMatrixData)) {
        return Status::error(StatusCode::SingularColorMatrix,
                             "effective DNG ColorMatrix1 cannot be represented as SRATIONAL");
    }
    const std::uint32_t colorMatrixOffset =
        add_blob(extras, extraStart, colorMatrixData, 4u);

    std::vector<std::uint8_t> neutralData;
    if (!make_rational_blob(neutral, neutralData)) {
        return Status::error(StatusCode::InvalidNeutral,
                             "effective AsShotNeutral cannot be represented as RATIONAL");
    }
    const std::uint32_t neutralOffset = add_blob(extras, extraStart, neutralData, 4u);

    std::uint64_t pixelStart64 = align4(extraStart + static_cast<std::uint32_t>(extras.size()));
    const std::uint64_t outputBytes64 = pixelStart64 + tileCount64 * tileBytes64;
    if (pixelStart64 > std::numeric_limits<std::uint32_t>::max() ||
        outputBytes64 > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::OutputTooLarge,
                             "Linear DNG output exceeds classic TIFF 32-bit offset limit");
    }
    const std::uint32_t pixelStart = static_cast<std::uint32_t>(pixelStart64);
    extras.insert(extras.end(), pixelStart - (extraStart + static_cast<std::uint32_t>(extras.size())), 0u);

    if (tileCount > 1u) {
        for (std::uint32_t i = 0; i < tileCount; ++i) {
            const std::uint32_t offset = pixelStart + i * tileBytes;
            put_u32(extras, tileOffsetsRelative + static_cast<std::size_t>(i) * 4u, offset);
        }
    }

    std::vector<IfdEntry> entries;
    entries.reserve(kIfdEntryCount);
    entries.push_back({kTagNewSubFileType, kTiffTypeLong, 1u, 0u});
    entries.push_back({kTagImageWidth, kTiffTypeLong, 1u, static_cast<std::uint32_t>(width)});
    entries.push_back({kTagImageLength, kTiffTypeLong, 1u, static_cast<std::uint32_t>(height)});
    entries.push_back({kTagBitsPerSample, kTiffTypeShort, 3u, bitsOffset});
    entries.push_back({kTagCompression, kTiffTypeShort, 1u, kCompressionNone});
    entries.push_back({kTagPhotometricInterpretation, kTiffTypeShort, 1u, kPhotometricLinearRaw});
    entries.push_back({kTagImageDescription, kTiffTypeAscii,
                       static_cast<std::uint32_t>(descriptionData.size()), descriptionOffset});
    entries.push_back({kTagOrientation, kTiffTypeShort, 1u,
                       static_cast<std::uint32_t>(source.orientation())});
    entries.push_back({kTagSamplesPerPixel, kTiffTypeShort, 1u, 3u});
    entries.push_back({kTagPlanarConfiguration, kTiffTypeShort, 1u, kPlanarChunky});
    entries.push_back({kTagSoftware, kTiffTypeAscii,
                       static_cast<std::uint32_t>(softwareData.size()), softwareOffset});
    entries.push_back({kTagTileWidth, kTiffTypeLong, 1u,
                       static_cast<std::uint32_t>(options.tileEdge)});
    entries.push_back({kTagTileLength, kTiffTypeLong, 1u,
                       static_cast<std::uint32_t>(options.tileEdge)});
    entries.push_back({kTagTileOffsets, kTiffTypeLong, tileCount,
                       tileCount == 1u ? pixelStart : tileOffsetsOffset});
    entries.push_back({kTagTileByteCounts, kTiffTypeLong, tileCount,
                       tileCount == 1u ? tileBytes : tileByteCountsOffset});
    entries.push_back({kTagSampleFormat, kTiffTypeShort, 3u, sampleFormatOffset});
    entries.push_back({kTagDngVersion, kTiffTypeByte, 4u,
                       inline_bytes4({1u, 4u, 0u, 0u})});
    entries.push_back({kTagDngBackwardVersion, kTiffTypeByte, 4u,
                       inline_bytes4({1u, 4u, 0u, 0u})});
    entries.push_back({kTagUniqueCameraModel, kTiffTypeAscii,
                       static_cast<std::uint32_t>(modelData.size()), modelOffset});
    entries.push_back({kTagWhiteLevel, kTiffTypeShort, 3u, whiteOffset});
    entries.push_back({kTagColorMatrix1, kTiffTypeSRational, 9u, colorMatrixOffset});
    entries.push_back({kTagAsShotNeutral, kTiffTypeRational, 3u, neutralOffset});
    entries.push_back({kTagCalibrationIlluminant1, kTiffTypeShort, 1u,
                       kCalibrationIlluminantD50});

    // Keep the compile-time count honest if tags are added/removed.
    if (entries.size() != kIfdEntryCount) {
        return Status::error(StatusCode::InvalidArgument,
                             "internal Linear DNG IFD entry-count mismatch");
    }
    std::sort(entries.begin(), entries.end(),
              [](const IfdEntry& a, const IfdEntry& b) { return a.tag < b.tag; });

    std::vector<std::uint8_t> prefix(pixelStart, 0u);
    prefix[0] = 'I';
    prefix[1] = 'I';
    put_u16(prefix, 2u, 42u);
    put_u32(prefix, 4u, ifdOffset);
    put_u16(prefix, ifdOffset, static_cast<std::uint16_t>(entries.size()));
    std::size_t entryOffset = ifdOffset + 2u;
    for (const auto& entry : entries) {
        write_ifd_entry(prefix, entryOffset, entry);
        entryOffset += 12u;
    }
    put_u32(prefix, entryOffset, 0u);
    std::copy(extras.begin(), extras.end(), prefix.begin() + extraStart);

    auto status = sink.append(prefix.data(), prefix.size());
    if (!status) {
        return Status::error(StatusCode::SinkFailed, status.message);
    }
    audit.logicalWorkspacePeakBytes = prefix.size();
    std::vector<std::uint8_t>().swap(prefix);
    std::vector<std::uint8_t>().swap(extras);

    const std::size_t encodedBytes = static_cast<std::size_t>(tileBytes);
    std::vector<std::uint8_t> encoded(encodedBytes, 0u);
    std::vector<float> cameraRgb;
    cameraRgb.reserve(static_cast<std::size_t>(samplesPerTile));
    audit.logicalWorkspacePeakBytes = std::max(
        audit.logicalWorkspacePeakBytes,
        encoded.capacity() + static_cast<std::size_t>(samplesPerTile) * sizeof(float));

    for (std::uint32_t ty = 0; ty < static_cast<std::uint32_t>(tilesY); ++ty) {
        for (std::uint32_t tx = 0; tx < static_cast<std::uint32_t>(tilesX); ++tx) {
            const int x0 = static_cast<int>(tx) * options.tileEdge;
            const int y0 = static_cast<int>(ty) * options.tileEdge;
            const int x1 = std::min(width, x0 + options.tileEdge);
            const int y1 = std::min(height, y0 + options.tileEdge);
            const int validW = x1 - x0;
            const int validH = y1 - y0;
            const std::size_t validSamples =
                static_cast<std::size_t>(validW) * static_cast<std::size_t>(validH) * 3u;
            cameraRgb.resize(validSamples);
            status = source.readCameraRgbTile(
                x0, y0, x1, y1, cameraRgb.data(), cameraRgb.size());
            if (!status) {
                return Status::error(StatusCode::SourceFailed, status.message);
            }
            std::fill(encoded.begin(), encoded.end(), 0u);
            for (int py = 0; py < validH; ++py) {
                for (int px = 0; px < validW; ++px) {
                    for (int c = 0; c < 3; ++c) {
                        const std::size_t inputIndex =
                            (static_cast<std::size_t>(py) * static_cast<std::size_t>(validW) +
                             static_cast<std::size_t>(px)) * 3u + static_cast<std::size_t>(c);
                        double value = static_cast<double>(cameraRgb[inputIndex]);
                        ++audit.cameraRgbSamplesRead;
                        if (!std::isfinite(value)) {
                            return Status::error(StatusCode::NonFiniteSample,
                                                 "non-finite camera RGB sample in Linear DNG projection");
                        }
                        value *= options.linearScale;
                        if (value < 0.0) {
                            if (!options.clampToLinearReferenceRange) {
                                return Status::error(StatusCode::InvalidArgument,
                                                     "negative sample requires bounded projection clamp");
                            }
                            value = 0.0;
                            ++audit.negativeSamplesClamped;
                        } else if (value > 1.0) {
                            if (!options.clampToLinearReferenceRange) {
                                return Status::error(StatusCode::InvalidArgument,
                                                     "overrange sample requires bounded projection clamp");
                            }
                            value = 1.0;
                            ++audit.overrangeSamplesClamped;
                        }
                        const auto q = static_cast<std::uint16_t>(
                            std::llround(value * static_cast<double>(kWhiteLevel)));
                        ++audit.quantizedSamples;
                        const std::size_t outputIndex =
                            (static_cast<std::size_t>(py) * static_cast<std::size_t>(options.tileEdge) +
                             static_cast<std::size_t>(px)) * 3u + static_cast<std::size_t>(c);
                        encoded[outputIndex * 2u + 0u] = static_cast<std::uint8_t>(q & 0xffu);
                        encoded[outputIndex * 2u + 1u] = static_cast<std::uint8_t>((q >> 8u) & 0xffu);
                    }
                }
            }
            status = sink.append(encoded.data(), encoded.size());
            if (!status) {
                return Status::error(StatusCode::SinkFailed, status.message);
            }
            ++audit.tilesWritten;
        }
    }

    audit.outputBytes = sink.bytesWritten();
    if (audit.outputBytes != outputBytes64) {
        return Status::error(StatusCode::SinkFailed,
                             "Linear DNG sink byte count does not match planned TIFF size");
    }
    out = audit;
    return Status::ok();
}

const char* status_name(StatusCode code) noexcept {
    switch (code) {
        case StatusCode::Ok: return "OK";
        case StatusCode::InvalidArgument: return "INVALID_ARGUMENT";
        case StatusCode::FinalizedLineageRequired: return "FINALIZED_LINEAGE_REQUIRED";
        case StatusCode::SourceIdentityMismatch: return "SOURCE_IDENTITY_MISMATCH";
        case StatusCode::ScientificIdentityMismatch: return "SCIENTIFIC_IDENTITY_MISMATCH";
        case StatusCode::UnauthorizedColorBinding: return "UNAUTHORIZED_COLOR_BINDING";
        case StatusCode::SingularColorMatrix: return "SINGULAR_COLOR_MATRIX";
        case StatusCode::InvalidNeutral: return "INVALID_NEUTRAL";
        case StatusCode::SourceFailed: return "SOURCE_FAILED";
        case StatusCode::SinkFailed: return "SINK_FAILED";
        case StatusCode::OutputTooLarge: return "OUTPUT_TOO_LARGE";
        case StatusCode::NonFiniteSample: return "NONFINITE_SAMPLE";
    }
    return "UNKNOWN";
}

}  // namespace truthraw::linear_dng_projection::v0_1
