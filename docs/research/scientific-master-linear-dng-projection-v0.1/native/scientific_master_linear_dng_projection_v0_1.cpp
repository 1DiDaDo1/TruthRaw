#include "scientific_master_linear_dng_projection_v0_1.h"

#include "scientific_master_digest_v0_1.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>
#include <string>
#include <vector>

namespace truthraw::scientific_master_linear_dng_projection::v0_1 {
namespace {

using scientific_master_digest::v0_1::ScientificMasterDigestAccumulator;
using scientific_master_digest::v0_1::TileView;

static_assert(kCanonicalTileEdge == scientific_master_digest::v0_1::kCanonicalCellEdge,
              "projection tiles must equal the canonical Scientific Master digest grid");

constexpr std::uint16_t kTiffByte = 1u;
constexpr std::uint16_t kTiffAscii = 2u;
constexpr std::uint16_t kTiffShort = 3u;
constexpr std::uint16_t kTiffLong = 4u;
constexpr std::uint16_t kTiffRational = 5u;
constexpr std::uint16_t kTiffSRational = 10u;

constexpr std::uint16_t kTagNewSubFileType = 254u;
constexpr std::uint16_t kTagImageWidth = 256u;
constexpr std::uint16_t kTagImageLength = 257u;
constexpr std::uint16_t kTagBitsPerSample = 258u;
constexpr std::uint16_t kTagCompression = 259u;
constexpr std::uint16_t kTagPhotometricInterpretation = 262u;
constexpr std::uint16_t kTagOrientation = 274u;
constexpr std::uint16_t kTagSamplesPerPixel = 277u;
constexpr std::uint16_t kTagPlanarConfiguration = 284u;
constexpr std::uint16_t kTagSoftware = 305u;
constexpr std::uint16_t kTagTileWidth = 322u;
constexpr std::uint16_t kTagTileLength = 323u;
constexpr std::uint16_t kTagTileOffsets = 324u;
constexpr std::uint16_t kTagTileByteCounts = 325u;
constexpr std::uint16_t kTagSampleFormat = 339u;
constexpr std::uint16_t kTagDngVersion = 50706u;
constexpr std::uint16_t kTagDngBackwardVersion = 50707u;
constexpr std::uint16_t kTagUniqueCameraModel = 50708u;
constexpr std::uint16_t kTagColorMatrix1 = 50721u;
constexpr std::uint16_t kTagAsShotNeutral = 50728u;
constexpr std::uint16_t kTagDngPrivateData = 50740u;
constexpr std::uint16_t kTagCalibrationIlluminant1 = 50778u;

constexpr std::uint32_t kClassicTiffFirstIfd = 8u;
constexpr std::uint32_t kSamplesPerPixel = 3u;
constexpr std::uint32_t kBytesPerSample = 4u;
constexpr std::uint32_t kBytesPerPixel = kSamplesPerPixel * kBytesPerSample;
constexpr std::size_t kMaxIdentityTextBytes = 2048u;
constexpr double kMatrixDeterminantEpsilon = 1.0e-12;

struct IfdEntry final {
    std::uint16_t tag = 0u;
    std::uint16_t type = 0u;
    std::uint32_t count = 0u;
    std::vector<std::uint8_t> payload;
    std::uint32_t outOfLineOffset = 0u;
};

void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
}

void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 16u) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 24u) & 0xffu));
}

void append_i32(std::vector<std::uint8_t>& out, std::int32_t value) {
    append_u32(out, static_cast<std::uint32_t>(value));
}

void store_u32(std::vector<std::uint8_t>& out, std::size_t offset, std::uint32_t value) {
    out[offset + 0u] = static_cast<std::uint8_t>(value & 0xffu);
    out[offset + 1u] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
    out[offset + 2u] = static_cast<std::uint8_t>((value >> 16u) & 0xffu);
    out[offset + 3u] = static_cast<std::uint8_t>((value >> 24u) & 0xffu);
}

std::uint32_t align4(std::uint32_t value) noexcept {
    return (value + 3u) & ~std::uint32_t{3u};
}

bool add_u64(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a > std::numeric_limits<std::uint64_t>::max() - b) return false;
    out = a + b;
    return true;
}

bool mul_u64(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a != 0u && b > std::numeric_limits<std::uint64_t>::max() / a) return false;
    out = a * b;
    return true;
}

std::vector<std::uint8_t> short_payload(std::uint16_t value) {
    std::vector<std::uint8_t> out;
    append_u16(out, value);
    return out;
}

std::vector<std::uint8_t> long_payload(std::uint32_t value) {
    std::vector<std::uint8_t> out;
    append_u32(out, value);
    return out;
}

std::vector<std::uint8_t> ascii_payload(const std::string& text) {
    std::vector<std::uint8_t> out(text.begin(), text.end());
    out.push_back(0u);
    return out;
}

std::string hex_bytes(const std::uint8_t* data, std::size_t size) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(size * 2u);
    for (std::size_t i = 0u; i < size; ++i) {
        const auto byte = data[i];
        out.push_back(kHex[(byte >> 4u) & 0x0fu]);
        out.push_back(kHex[byte & 0x0fu]);
    }
    return out;
}

std::string hex_hash(const Hash256& hash) {
    return hex_bytes(hash.data(), hash.size());
}

std::string hex_u64(std::uint64_t value) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out(16u, '0');
    for (std::size_t i = 0u; i < 16u; ++i) {
        const unsigned shift = static_cast<unsigned>((15u - i) * 4u);
        out[i] = kHex[(value >> shift) & 0x0fu];
    }
    return out;
}

std::string hex_u32(std::uint32_t value) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out(8u, '0');
    for (std::size_t i = 0u; i < 8u; ++i) {
        const unsigned shift = static_cast<unsigned>((7u - i) * 4u);
        out[i] = kHex[(value >> shift) & 0x0fu];
    }
    return out;
}

const char* gauge_mode_name(TruthRangeGaugeModeV02 mode) noexcept {
    switch (mode) {
        case TruthRangeGaugeModeV02::SelfGauge: return "SELF_GAUGE";
        case TruthRangeGaugeModeV02::ExternalRelativeGauge: return "EXTERNAL_RELATIVE_GAUGE";
        case TruthRangeGaugeModeV02::PhysicalAbsoluteGauge: return "PHYSICAL_ABSOLUTE_GAUGE";
    }
    return "UNKNOWN";
}

std::string printable_identity(std::string text) {
    if (text.size() > kMaxIdentityTextBytes) text.resize(kMaxIdentityTextBytes);
    for (char& c : text) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (u < 0x20u || u > 0x7eu) c = '_';
    }
    return text;
}

std::vector<std::uint8_t> private_data(const ProjectionDescriptor& descriptor) {
    const std::string id = "TruthRaw scientific-master-linear-dng-projection-v0.1";

    std::uint64_t l0Bits = 0u;
    static_assert(sizeof(l0Bits) == sizeof(descriptor.zeroLineGauge.L0),
                  "PURE zero-line provenance requires IEEE-754 binary64 storage");
    std::memcpy(&l0Bits, &descriptor.zeroLineGauge.L0, sizeof(l0Bits));

    constexpr std::size_t kBackplaneStoredCrcBytes = sizeof(std::uint32_t);
    static_assert(technical_backplane::v0_1::kSerializedBytes > kBackplaneStoredCrcBytes,
                  "Technical Backplane must contain payload bytes before its stored CRC32");
    const auto backplaneCrc = technical_backplane::v0_1::crc32(
        std::span<const std::uint8_t>(
            descriptor.serializedBackplane.data(),
            descriptor.serializedBackplane.size() - kBackplaneStoredCrcBytes));

    const bool derivative = nonzero_hash(descriptor.projectedRasterSha256);
    const std::string role = descriptor.projectionRole.empty()
        ? "TRUTHRAW_PURE_FLOAT32_XYZ_D50_LINEAR_DNG_PROJECTION"
        : printable_identity(descriptor.projectionRole);
    const std::string body =
        std::string("role=") + role + "\n" +
        "private_contract=TRUTHRAW_PURE_SELF_BINDING_V0_63\n" +
        "writer_identity=TruthRaw scientific-master-linear-dng-projection-v0.1\n" +
        "representation_only=1\n" +
        "scientific_master_modified=0\n" +
        "appearance_applied=0\n" +
        "counterfactual_observation_created=0\n" +
        "physical_frame_count=1\n" +
        "independent_evidence_count=1\n" +
        "sealed_source_sha256=" + hex_hash(descriptor.sealedSourceSha256) + "\n" +
        "scientific_master_sha256=" + hex_hash(descriptor.scientificMasterSha256) + "\n" +
        "projected_raster_sha256=" +
            (derivative ? hex_hash(descriptor.projectedRasterSha256)
                        : hex_hash(descriptor.scientificMasterSha256)) + "\n" +
        "restoration_derivative=" + std::string(descriptor.restorationDerivative ? "1\n" : "0\n") +
        "open_scene_state_sha256=" +
            (nonzero_hash(descriptor.openSceneStateSha256)
                ? hex_hash(descriptor.openSceneStateSha256)
                : std::string(64u, '0')) + "\n" +
        "zero_line_sha256=" + hex_hash(descriptor.zeroLineSha256) + "\n" +
        "zero_line_mode=" + gauge_mode_name(descriptor.zeroLineGauge.mode) + "\n" +
        "zero_line_l0_f64_bits=0x" + hex_u64(l0Bits) + "\n" +
        "zero_line_gauge_id=" + printable_identity(descriptor.zeroLineGauge.gaugeId) + "\n" +
        "zero_line_cross_scene_comparable=" +
            std::string(descriptor.zeroLineGauge.crossSceneComparable ? "1\n" : "0\n") +
        "zero_line_absolute_physical_units=" +
            std::string(descriptor.zeroLineGauge.absolutePhysicalUnits ? "1\n" : "0\n") +
        "scene_scale_sha256=" + hex_hash(descriptor.sceneScaleSha256) + "\n" +
        "scene_scale_id=" + printable_identity(descriptor.sceneBinding.sceneScaleId) + "\n" +
        "scene_gainmap_applied_exactly_once=" +
            std::string(descriptor.sceneBinding.gainMapAppliedExactlyOnce ? "1\n" : "0\n") +
        "scene_exposure_normalized=" +
            std::string(descriptor.sceneBinding.exposureNormalizedToCommonScene ? "1\n" : "0\n") +
        "scene_gain_normalized=" +
            std::string(descriptor.sceneBinding.gainNormalizedToCommonScene ? "1\n" : "0\n") +
        "technical_backplane_version=1\n" +
        "technical_backplane_crc_scope=PREFIX_176_BYTES\n" +
        "technical_backplane_crc32=0x" + hex_u32(backplaneCrc) + "\n" +
        "technical_backplane_serialized_hex=" +
            hex_bytes(descriptor.serializedBackplane.data(), descriptor.serializedBackplane.size()) + "\n" +
        "precision_policy_id=" + printable_identity(descriptor.precisionPolicyId) + "\n" +
        "runtime_reconstruction_backend_id=" +
            printable_identity(descriptor.runtimeReconstructionBackendId) + "\n" +
        "source_evidence_id=" + printable_identity(descriptor.sourceEvidenceId) + "\n" +
        "color_binding_id=" + printable_identity(descriptor.colorBindingId) + "\n";

    std::vector<std::uint8_t> out(id.begin(), id.end());
    out.push_back(0u);
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

std::vector<std::uint8_t> identity_color_matrix_payload() {
    std::vector<std::uint8_t> out;
    out.reserve(9u * 8u);
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            append_i32(out, row == col ? 1 : 0);
            append_i32(out, 1);
        }
    }
    return out;
}

std::vector<std::uint8_t> d50_neutral_payload() {
    std::vector<std::uint8_t> out;
    out.reserve(3u * 8u);
    append_u32(out, 9643u); append_u32(out, 10000u);
    append_u32(out, 1u);    append_u32(out, 1u);
    append_u32(out, 8251u); append_u32(out, 10000u);
    return out;
}

bool nonzero_hash(const Hash256& hash) noexcept {
    for (const auto byte : hash) if (byte != 0u) return true;
    return false;
}

bool valid_orientation(std::uint16_t orientation) noexcept {
    return orientation == 1u || orientation == 3u ||
           orientation == 6u || orientation == 8u;
}

Status validate_scientific_binding(const ProjectionDescriptor& descriptor) noexcept {
    if (!nonzero_hash(descriptor.zeroLineSha256) ||
        !nonzero_hash(descriptor.sceneScaleSha256) ||
        descriptor.precisionPolicyId.empty() ||
        descriptor.runtimeReconstructionBackendId.empty()) {
        return Status::error(
            StatusCode::ScientificBindingMismatch,
            "PURE self-binding requires zero-line, scene-scale and precision identities");
    }

    technical_backplane::v0_1::State backplane{};
    const auto backplaneStatus = technical_backplane::v0_1::deserialize(
        std::span<const std::uint8_t>(
            descriptor.serializedBackplane.data(), descriptor.serializedBackplane.size()),
        backplane);
    if (backplaneStatus != technical_backplane::v0_1::Status::Ok) {
        return Status::error(
            StatusCode::ScientificBindingMismatch,
            std::string("serialized Technical Backplane rejected: ") +
                technical_backplane::v0_1::status_name(backplaneStatus));
    }

    if (backplane.sourceEvidenceHash != descriptor.sealedSourceSha256 ||
        backplane.scientificMasterHash != descriptor.scientificMasterSha256 ||
        backplane.zeroLineHash != descriptor.zeroLineSha256 ||
        backplane.sceneScaleHash != descriptor.sceneScaleSha256 ||
        backplane.physicalFrameCount != 1u ||
        backplane.independentEvidenceCount != 1u ||
        backplane.forbiddenFlags != 0u) {
        return Status::error(
            StatusCode::ScientificBindingMismatch,
            "Technical Backplane identity does not match PURE projection lineage");
    }

    if (descriptor.restorationDerivative &&
        (!nonzero_hash(descriptor.projectedRasterSha256) || descriptor.projectionRole.empty())) {
        return Status::error(
            StatusCode::ScientificBindingMismatch,
            "restoration derivative projection requires explicit raster hash and role");
    }

    if (!(descriptor.zeroLineGauge.L0 > 0.0) ||
        !std::isfinite(descriptor.zeroLineGauge.L0) ||
        descriptor.zeroLineGauge.gaugeId.empty() ||
        descriptor.sceneBinding.sceneScaleId.empty() ||
        !descriptor.sceneBinding.gainMapAppliedExactlyOnce) {
        return Status::error(
            StatusCode::ScientificBindingMismatch,
            "zero-line/scene-scale payload is incomplete or invalid");
    }

    return Status::ok();
}

bool valid_matrix(const std::array<float, 9>& m) noexcept {
    for (const float value : m) {
        if (!std::isfinite(value) || std::abs(static_cast<double>(value)) > 128.0) return false;
    }
    const double a = m[0], b = m[1], c = m[2];
    const double d = m[3], e = m[4], f = m[5];
    const double g = m[6], h = m[7], i = m[8];
    const double det = a * (e * i - f * h) -
                       b * (d * i - f * g) +
                       c * (d * h - e * g);
    return std::isfinite(det) && std::abs(det) > kMatrixDeterminantEpsilon;
}

void encode_float32_le(float value, std::uint8_t* dst) noexcept {
    std::uint32_t bits = 0u;
    static_assert(sizeof(bits) == sizeof(value), "float32 required");
    std::memcpy(&bits, &value, sizeof(bits));
    dst[0] = static_cast<std::uint8_t>(bits & 0xffu);
    dst[1] = static_cast<std::uint8_t>((bits >> 8u) & 0xffu);
    dst[2] = static_cast<std::uint8_t>((bits >> 16u) & 0xffu);
    dst[3] = static_cast<std::uint8_t>((bits >> 24u) & 0xffu);
}

Status make_header(
    const ProjectionDescriptor& descriptor,
    std::uint32_t tileCount,
    std::uint32_t tileByteCount,
    std::vector<std::uint8_t>& header,
    std::uint32_t& dataStart,
    std::uint64_t& totalBytes) {
    std::vector<IfdEntry> entries;
    entries.reserve(22u);

    const auto add = [&](std::uint16_t tag, std::uint16_t type,
                         std::uint32_t count, std::vector<std::uint8_t> payload) {
        entries.push_back(IfdEntry{tag, type, count, std::move(payload), 0u});
    };
    const auto add_ascii = [&](std::uint16_t tag, const std::string& text) {
        auto payload = ascii_payload(text);
        const auto count = static_cast<std::uint32_t>(payload.size());
        add(tag, kTiffAscii, count, std::move(payload));
    };

    add(kTagNewSubFileType, kTiffLong, 1u, long_payload(0u));
    add(kTagImageWidth, kTiffLong, 1u, long_payload(descriptor.width));
    add(kTagImageLength, kTiffLong, 1u, long_payload(descriptor.height));

    std::vector<std::uint8_t> bits;
    for (int n = 0; n < 3; ++n) append_u16(bits, 32u);
    add(kTagBitsPerSample, kTiffShort, 3u, std::move(bits));

    add(kTagCompression, kTiffShort, 1u, short_payload(1u));
    add(kTagPhotometricInterpretation, kTiffShort, 1u,
        short_payload(kPhotometricLinearRaw));
    add(kTagOrientation, kTiffShort, 1u, short_payload(descriptor.orientation));
    add(kTagSamplesPerPixel, kTiffShort, 1u, short_payload(3u));
    add(kTagPlanarConfiguration, kTiffShort, 1u, short_payload(1u));
    add_ascii(kTagSoftware, "TruthRaw scientific-master-linear-dng-projection-v0.1");
    add(kTagTileWidth, kTiffLong, 1u, long_payload(kCanonicalTileEdge));
    add(kTagTileLength, kTiffLong, 1u, long_payload(kCanonicalTileEdge));

    std::vector<std::uint8_t> tileOffsets(static_cast<std::size_t>(tileCount) * 4u, 0u);
    add(kTagTileOffsets, kTiffLong, tileCount, std::move(tileOffsets));

    std::vector<std::uint8_t> tileByteCounts;
    tileByteCounts.reserve(static_cast<std::size_t>(tileCount) * 4u);
    for (std::uint32_t n = 0; n < tileCount; ++n) append_u32(tileByteCounts, tileByteCount);
    add(kTagTileByteCounts, kTiffLong, tileCount, std::move(tileByteCounts));

    std::vector<std::uint8_t> sampleFormat;
    for (int n = 0; n < 3; ++n) append_u16(sampleFormat, 3u);
    add(kTagSampleFormat, kTiffShort, 3u, std::move(sampleFormat));

    add(kTagDngVersion, kTiffByte, 4u, std::vector<std::uint8_t>{1u, 4u, 0u, 0u});
    add(kTagDngBackwardVersion, kTiffByte, 4u,
        std::vector<std::uint8_t>{1u, 4u, 0u, 0u});
    add_ascii(kTagUniqueCameraModel, "TruthRaw Scientific Master XYZ D50 Projection");
    add(kTagColorMatrix1, kTiffSRational, 9u, identity_color_matrix_payload());
    add(kTagAsShotNeutral, kTiffRational, 3u, d50_neutral_payload());
    auto privatePayload = private_data(descriptor);
    const auto privateCount = static_cast<std::uint32_t>(privatePayload.size());
    add(kTagDngPrivateData, kTiffByte, privateCount, std::move(privatePayload));
    add(kTagCalibrationIlluminant1, kTiffShort, 1u,
        short_payload(kCalibrationIlluminantD50));

    std::sort(entries.begin(), entries.end(),
              [](const IfdEntry& a, const IfdEntry& b) { return a.tag < b.tag; });

    if (entries.size() > std::numeric_limits<std::uint16_t>::max()) {
        return Status::error(StatusCode::SizeOverflow, "IFD entry count overflow");
    }

    std::uint64_t ifdEnd64 = 0u;
    if (!mul_u64(entries.size(), 12u, ifdEnd64) ||
        !add_u64(ifdEnd64, kClassicTiffFirstIfd + 2u + 4u, ifdEnd64) ||
        ifdEnd64 > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::SizeOverflow, "IFD layout overflow");
    }

    std::uint32_t cursor = static_cast<std::uint32_t>(ifdEnd64);
    for (auto& entry : entries) {
        if (entry.payload.size() <= 4u) continue;
        cursor = align4(cursor);
        if (entry.payload.size() >
            static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max() - cursor)) {
            return Status::error(StatusCode::SizeOverflow, "DNG metadata offset overflow");
        }
        entry.outOfLineOffset = cursor;
        cursor += static_cast<std::uint32_t>(entry.payload.size());
    }
    dataStart = align4(cursor);

    const std::uint64_t tileBytesTotal =
        static_cast<std::uint64_t>(tileCount) * static_cast<std::uint64_t>(tileByteCount);
    if (!add_u64(dataStart, tileBytesTotal, totalBytes) ||
        totalBytes > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::SizeOverflow,
                             "classic TIFF/DNG projection would exceed 4 GiB");
    }

    for (auto& entry : entries) {
        if (entry.tag != kTagTileOffsets) continue;
        for (std::uint32_t n = 0; n < tileCount; ++n) {
            const std::uint64_t offset =
                static_cast<std::uint64_t>(dataStart) +
                static_cast<std::uint64_t>(n) * tileByteCount;
            if (offset > std::numeric_limits<std::uint32_t>::max()) {
                return Status::error(StatusCode::SizeOverflow, "tile offset exceeds classic TIFF");
            }
            store_u32(entry.payload, static_cast<std::size_t>(n) * 4u,
                      static_cast<std::uint32_t>(offset));
        }
    }

    header.clear();
    header.reserve(dataStart);
    header.push_back('I');
    header.push_back('I');
    append_u16(header, 42u);
    append_u32(header, kClassicTiffFirstIfd);
    append_u16(header, static_cast<std::uint16_t>(entries.size()));

    for (const auto& entry : entries) {
        append_u16(header, entry.tag);
        append_u16(header, entry.type);
        append_u32(header, entry.count);
        if (entry.payload.size() <= 4u) {
            header.insert(header.end(), entry.payload.begin(), entry.payload.end());
            for (std::size_t pad = entry.payload.size(); pad < 4u; ++pad) header.push_back(0u);
        } else {
            append_u32(header, entry.outOfLineOffset);
        }
    }
    append_u32(header, 0u);

    for (const auto& entry : entries) {
        if (entry.payload.size() <= 4u) continue;
        if (header.size() > entry.outOfLineOffset) {
            return Status::error(StatusCode::SizeOverflow, "internal DNG metadata overlap");
        }
        header.resize(entry.outOfLineOffset, 0u);
        header.insert(header.end(), entry.payload.begin(), entry.payload.end());
    }
    if (header.size() > dataStart) {
        return Status::error(StatusCode::SizeOverflow, "internal DNG header exceeds data start");
    }
    header.resize(dataStart, 0u);
    return Status::ok();
}

Status fail_transaction(ITransactionalByteSink& sink,
                        StatusCode code,
                        std::string message) noexcept {
    sink.abort();
    return Status::error(code, std::move(message));
}

}  // namespace

Status write_xyz_d50_linear_dng_projection(
    IScientificMasterTileSource& source,
    const ProjectionDescriptor& descriptor,
    const std::array<float, 9>& cameraToXyzD50,
    ITransactionalByteSink& sink,
    Result& out) noexcept {
    out = {};

    try {
        if (descriptor.width == 0u || descriptor.height == 0u ||
            !valid_orientation(descriptor.orientation) ||
            !nonzero_hash(descriptor.scientificMasterSha256) ||
            !nonzero_hash(descriptor.sealedSourceSha256)) {
            return Status::error(StatusCode::InvalidArgument,
                                 "invalid projection dimensions/orientation/identity");
        }
        const auto bindingStatus = validate_scientific_binding(descriptor);
        if (!bindingStatus) return bindingStatus;

        if (!valid_matrix(cameraToXyzD50)) {
            return Status::error(StatusCode::InvalidColorTransform,
                                 "cameraToXyzD50 is non-finite, singular, or out of bounds");
        }

        const std::uint64_t cols64 =
            (static_cast<std::uint64_t>(descriptor.width) + kCanonicalTileEdge - 1u) /
            kCanonicalTileEdge;
        const std::uint64_t rows64 =
            (static_cast<std::uint64_t>(descriptor.height) + kCanonicalTileEdge - 1u) /
            kCanonicalTileEdge;
        std::uint64_t tileCount64 = 0u;
        if (!mul_u64(cols64, rows64, tileCount64) ||
            tileCount64 == 0u ||
            tileCount64 > std::numeric_limits<std::uint32_t>::max()) {
            return Status::error(StatusCode::SizeOverflow, "projection tile count overflow");
        }
        const std::uint32_t tileCount = static_cast<std::uint32_t>(tileCount64);

        const std::uint64_t tilePixels64 =
            static_cast<std::uint64_t>(kCanonicalTileEdge) * kCanonicalTileEdge;
        const std::uint64_t tileByteCount64 = tilePixels64 * kBytesPerPixel;
        if (tileByteCount64 > std::numeric_limits<std::uint32_t>::max()) {
            return Status::error(StatusCode::SizeOverflow, "projection tile byte count overflow");
        }
        const std::uint32_t tileByteCount = static_cast<std::uint32_t>(tileByteCount64);

        std::vector<std::uint8_t> header;
        std::uint32_t dataStart = 0u;
        std::uint64_t expectedBytes = 0u;
        auto headerStatus = make_header(
            descriptor, tileCount, tileByteCount, header, dataStart, expectedBytes);
        if (!headerStatus) return headerStatus;
        (void)dataStart;

        ScientificMasterDigestAccumulator digest(descriptor.width, descriptor.height);
        if (!digest.valid()) {
            return Status::error(StatusCode::DigestFailed,
                                 "Scientific Master digest initialization failed: " + digest.error());
        }

        std::vector<float> cameraTile;
        std::vector<std::uint8_t> encodedTile(static_cast<std::size_t>(tileByteCount), 0u);

        const auto digestResident = digest.metrics().residentBytesUpperBound;
        out.logicalWorkspacePeakBytes =
            header.size() + encodedTile.size() + digestResident;
        out.logicalResidentUpperBound =
            out.logicalWorkspacePeakBytes +
            source.residentBytesUpperBound() + sink.residentBytesUpperBound();

        if (!sink.begin(expectedBytes)) {
            sink.abort();
            return Status::error(StatusCode::SinkFailed, "transactional sink begin failed");
        }
        if (!sink.write(header.data(), header.size())) {
            return fail_transaction(sink, StatusCode::SinkFailed, "DNG header write failed");
        }
        out.bytesWritten += header.size();

        std::uint32_t tileOrdinal = 0u;
        for (std::uint32_t y = 0u; y < descriptor.height; y += kCanonicalTileEdge) {
            const std::uint32_t coreH =
                std::min(kCanonicalTileEdge, descriptor.height - y);
            for (std::uint32_t x = 0u; x < descriptor.width; x += kCanonicalTileEdge) {
                const std::uint32_t coreW =
                    std::min(kCanonicalTileEdge, descriptor.width - x);
                const std::size_t corePixels =
                    static_cast<std::size_t>(coreW) * static_cast<std::size_t>(coreH);
                const std::size_t coreFloats = corePixels * 3u;
                cameraTile.resize(coreFloats);

                const auto sourceStatus = source.readCameraNativeTile(
                    x, y, coreW, coreH, cameraTile.data(), cameraTile.size());
                if (!sourceStatus) {
                    return fail_transaction(
                        sink, StatusCode::SourceFailed,
                        "Scientific Master tile source failed: " + sourceStatus.message);
                }

                TileView digestTile{};
                digestTile.x = x;
                digestTile.y = y;
                digestTile.width = coreW;
                digestTile.height = coreH;
                digestTile.rgb = cameraTile.data();
                digestTile.rowStrideSamples = static_cast<std::size_t>(coreW) * 3u;
                if (!digest.add_tile(digestTile)) {
                    return fail_transaction(
                        sink, StatusCode::DigestFailed,
                        "Scientific Master digest rejected tile: " + digest.error());
                }

                std::fill(encodedTile.begin(), encodedTile.end(), 0u);
                for (std::uint32_t localY = 0u; localY < coreH; ++localY) {
                    for (std::uint32_t localX = 0u; localX < coreW; ++localX) {
                        const std::size_t src =
                            (static_cast<std::size_t>(localY) * coreW + localX) * 3u;
                        const double r = cameraTile[src + 0u];
                        const double g = cameraTile[src + 1u];
                        const double b = cameraTile[src + 2u];
                        const float xyz[3] = {
                            static_cast<float>(cameraToXyzD50[0] * r + cameraToXyzD50[1] * g + cameraToXyzD50[2] * b),
                            static_cast<float>(cameraToXyzD50[3] * r + cameraToXyzD50[4] * g + cameraToXyzD50[5] * b),
                            static_cast<float>(cameraToXyzD50[6] * r + cameraToXyzD50[7] * g + cameraToXyzD50[8] * b),
                        };
                        for (const float value : xyz) {
                            if (!std::isfinite(value)) {
                                return fail_transaction(
                                    sink, StatusCode::InvalidColorTransform,
                                    "XYZ D50 projection produced non-finite component");
                            }
                            if (value < 0.0f) ++out.negativeComponentCount;
                            if (value > 1.0f) ++out.overOneComponentCount;
                        }

                        const std::size_t dstPixel =
                            static_cast<std::size_t>(localY) * kCanonicalTileEdge + localX;
                        const std::size_t dst = dstPixel * kBytesPerPixel;
                        encode_float32_le(xyz[0], encodedTile.data() + dst + 0u);
                        encode_float32_le(xyz[1], encodedTile.data() + dst + 4u);
                        encode_float32_le(xyz[2], encodedTile.data() + dst + 8u);
                    }
                }

                if (!sink.write(encodedTile.data(), encodedTile.size())) {
                    return fail_transaction(sink, StatusCode::SinkFailed, "DNG tile write failed");
                }
                out.bytesWritten += encodedTile.size();
                out.projectedPixels += corePixels;
                ++out.tilesWritten;
                ++tileOrdinal;

                const std::size_t workspace =
                    header.size() + encodedTile.size() +
                    cameraTile.capacity() * sizeof(float) + digestResident;
                out.logicalWorkspacePeakBytes =
                    std::max(out.logicalWorkspacePeakBytes, workspace);
                out.logicalResidentUpperBound = std::max(
                    out.logicalResidentUpperBound,
                    workspace + source.residentBytesUpperBound() + sink.residentBytesUpperBound());
            }
        }

        if (tileOrdinal != tileCount || out.bytesWritten != expectedBytes) {
            return fail_transaction(sink, StatusCode::SizeOverflow,
                                    "projection byte/tile accounting mismatch");
        }

        Hash256 actualMaster{};
        if (!digest.finalize(actualMaster)) {
            return fail_transaction(
                sink, StatusCode::DigestFailed,
                "Scientific Master digest finalization failed: " + digest.error());
        }
        const bool derivativeProjection = nonzero_hash(descriptor.projectedRasterSha256);
        const auto& expectedRasterHash = derivativeProjection
            ? descriptor.projectedRasterSha256
            : descriptor.scientificMasterSha256;
        if (actualMaster != expectedRasterHash) {
            return fail_transaction(
                sink, StatusCode::ScientificMasterMismatch,
                derivativeProjection
                    ? "camera-native derivative source does not match declared projected raster identity"
                    : "camera-native export source does not match admitted Scientific Master identity");
        }
        out.projectedRasterIdentityVerified = true;
        out.scientificMasterIdentityVerified =
            actualMaster == descriptor.scientificMasterSha256;

        if (!sink.commit()) {
            sink.abort();
            return Status::error(StatusCode::SinkFailed, "transactional sink commit failed");
        }
        out.artifactCommitted = true;
        return Status::ok();
    } catch (const std::bad_alloc&) {
        sink.abort();
        return Status::error(StatusCode::SizeOverflow,
                             "allocation failed while building bounded DNG projection");
    } catch (...) {
        sink.abort();
        return Status::error(StatusCode::InvalidArgument,
                             "unexpected exception while building DNG projection");
    }
}

const char* status_name(StatusCode code) noexcept {
    switch (code) {
        case StatusCode::Ok: return "OK";
        case StatusCode::InvalidArgument: return "INVALID_ARGUMENT";
        case StatusCode::InvalidColorTransform: return "INVALID_COLOR_TRANSFORM";
        case StatusCode::SizeOverflow: return "SIZE_OVERFLOW";
        case StatusCode::SourceFailed: return "SOURCE_FAILED";
        case StatusCode::DigestFailed: return "DIGEST_FAILED";
        case StatusCode::ScientificMasterMismatch: return "SCIENTIFIC_MASTER_MISMATCH";
        case StatusCode::SinkFailed: return "SINK_FAILED";
        case StatusCode::ScientificBindingMismatch: return "SCIENTIFIC_BINDING_MISMATCH";
    }
    return "UNKNOWN";
}

}  // namespace truthraw::scientific_master_linear_dng_projection::v0_1
