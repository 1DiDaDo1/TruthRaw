#include "dng_color_binding_producer_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>

namespace truthraw::dng_color_binding_producer_v0_1 {
namespace {

using scientific_preview_binding_v0_1::BindingStatusCode;
using scientific_preview_binding_v0_1::ColorBindingAuthority;
using scientific_preview_binding_v0_1::ScientificColorBindingRecord;
using scientific_preview_binding_v0_1::SourceSeal;
using tile_dng_v0_1::IRandomAccessByteSource;

constexpr std::uint16_t kTiffAscii = 2;
constexpr std::uint16_t kTiffByte = 1;
constexpr std::uint16_t kTiffRational = 5;
constexpr std::uint16_t kTiffSRational = 10;

constexpr std::uint16_t kTagColorMatrix1 = 50721;
constexpr std::uint16_t kTagColorMatrix2 = 50722;
constexpr std::uint16_t kTagCameraCalibration1 = 50723;
constexpr std::uint16_t kTagCameraCalibration2 = 50724;
constexpr std::uint16_t kTagReductionMatrix2 = 50726;
constexpr std::uint16_t kTagAnalogBalance = 50727;
constexpr std::uint16_t kTagAsShotNeutral = 50728;
constexpr std::uint16_t kTagCalibrationIlluminant2 = 50779;
constexpr std::uint16_t kTagCameraCalibrationSignature = 50931;
constexpr std::uint16_t kTagProfileCalibrationSignature = 50932;
constexpr std::uint16_t kTagForwardMatrix1 = 50964;
constexpr std::uint16_t kTagForwardMatrix2 = 50965;
constexpr std::uint16_t kTagCalibrationIlluminant3 = 52529;
constexpr std::uint16_t kTagCameraCalibration3 = 52530;
constexpr std::uint16_t kTagColorMatrix3 = 52531;
constexpr std::uint16_t kTagForwardMatrix3 = 52532;
constexpr std::uint16_t kTagReductionMatrix3 = 52538;

constexpr std::uint16_t kMaxIfdEntries = 512;
constexpr std::size_t kMaxSignatureBytes = 1024;
constexpr double kSingularEpsilon = 1.0e-12;

struct TagRef {
    bool present = false;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::uint64_t dataOffset = 0;
    std::uint64_t dataBytes = 0;
};

struct RootTags {
    TagRef colorMatrix1;
    TagRef cameraCalibration1;
    TagRef analogBalance;
    TagRef asShotNeutral;
    TagRef cameraCalibrationSignature;
    TagRef profileCalibrationSignature;
    TagRef forwardMatrix1;
    bool multipleCalibrationTagSeen = false;
};

struct Mat3 {
    std::array<double, 9> v{1,0,0, 0,1,0, 0,0,1};
};

using Vec3 = std::array<double, 3>;

std::uint16_t dec16(const std::uint8_t* p, bool little) noexcept {
    if (little) return static_cast<std::uint16_t>(p[0]) | static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[1]) << 8u);
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[0]) << 8u) | static_cast<std::uint16_t>(p[1]);
}

std::uint32_t dec32(const std::uint8_t* p, bool little) noexcept {
    if (little) {
        return static_cast<std::uint32_t>(p[0]) |
               (static_cast<std::uint32_t>(p[1]) << 8u) |
               (static_cast<std::uint32_t>(p[2]) << 16u) |
               (static_cast<std::uint32_t>(p[3]) << 24u);
    }
    return (static_cast<std::uint32_t>(p[0]) << 24u) |
           (static_cast<std::uint32_t>(p[1]) << 16u) |
           (static_cast<std::uint32_t>(p[2]) << 8u) |
           static_cast<std::uint32_t>(p[3]);
}

std::uint64_t type_size(std::uint16_t type) noexcept {
    switch (type) {
        case 1: case 2: case 6: case 7: return 1;
        case 3: case 8: return 2;
        case 4: case 9: case 11: return 4;
        case 5: case 10: case 12: return 8;
        default: return 0;
    }
}

bool add_ok(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a > std::numeric_limits<std::uint64_t>::max() - b) return false;
    out = a + b;
    return true;
}

bool mul_ok(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a != 0 && b > std::numeric_limits<std::uint64_t>::max() / a) return false;
    out = a * b;
    return true;
}

bool read_exact(IRandomAccessByteSource& source, std::uint64_t offset, void* dst, std::size_t n, ProducerAudit& audit) {
    if (offset > source.sizeBytes() || n > source.sizeBytes() - offset) return false;
    if (!source.readExact(offset, dst, n)) return false;
    audit.metadataBytesRead += n;
    return true;
}

void capture_tag(RootTags& tags, std::uint16_t tag, const TagRef& ref) {
    switch (tag) {
        case kTagColorMatrix1: tags.colorMatrix1 = ref; break;
        case kTagCameraCalibration1: tags.cameraCalibration1 = ref; break;
        case kTagAnalogBalance: tags.analogBalance = ref; break;
        case kTagAsShotNeutral: tags.asShotNeutral = ref; break;
        case kTagCameraCalibrationSignature: tags.cameraCalibrationSignature = ref; break;
        case kTagProfileCalibrationSignature: tags.profileCalibrationSignature = ref; break;
        case kTagForwardMatrix1: tags.forwardMatrix1 = ref; break;
        case kTagColorMatrix2:
        case kTagCameraCalibration2:
        case kTagReductionMatrix2:
        case kTagCalibrationIlluminant2:
        case kTagForwardMatrix2:
        case kTagCalibrationIlluminant3:
        case kTagCameraCalibration3:
        case kTagColorMatrix3:
        case kTagForwardMatrix3:
        case kTagReductionMatrix3:
            tags.multipleCalibrationTagSeen = true;
            break;
        default: break;
    }
}

ProducerStatus parse_root_ifd(IRandomAccessByteSource& source, RootTags& tags, bool& little, ProducerAudit& audit) {
    std::array<std::uint8_t, 8> header{};
    if (!read_exact(source, 0, header.data(), header.size(), audit))
        return ProducerStatus::error(ProducerStatusCode::SourceReadFailed, "cannot read TIFF header");
    if (header[0] == 'I' && header[1] == 'I') little = true;
    else if (header[0] == 'M' && header[1] == 'M') little = false;
    else return ProducerStatus::error(ProducerStatusCode::InvalidTiff, "invalid TIFF byte order");

    const auto magic = dec16(header.data() + 2, little);
    if (magic == 43) return ProducerStatus::error(ProducerStatusCode::UnsupportedBigTiff, "BigTIFF color metadata is not supported in v0.1");
    if (magic != 42) return ProducerStatus::error(ProducerStatusCode::InvalidTiff, "classic TIFF magic 42 required");

    const std::uint32_t root = dec32(header.data() + 4, little);
    if (root == 0 || root > source.sizeBytes() || source.sizeBytes() - root < 2)
        return ProducerStatus::error(ProducerStatusCode::InvalidIfd, "IFD0 offset out of range");

    std::array<std::uint8_t, 2> countBytes{};
    if (!read_exact(source, root, countBytes.data(), countBytes.size(), audit))
        return ProducerStatus::error(ProducerStatusCode::SourceReadFailed, "cannot read IFD0 count");
    const std::uint16_t count = dec16(countBytes.data(), little);
    if (count > kMaxIfdEntries)
        return ProducerStatus::error(ProducerStatusCode::InvalidIfd, "IFD0 entry cap exceeded");

    std::uint64_t entryBytes = 0;
    if (!mul_ok(count, 12, entryBytes)) return ProducerStatus::error(ProducerStatusCode::InvalidIfd, "IFD0 size overflow");
    std::uint64_t end = 0;
    if (!add_ok(static_cast<std::uint64_t>(root) + 2u, entryBytes + 4u, end) || end > source.sizeBytes())
        return ProducerStatus::error(ProducerStatusCode::InvalidIfd, "truncated IFD0");

    std::array<std::uint8_t, 12> entry{};
    for (std::uint16_t i = 0; i < count; ++i) {
        const std::uint64_t entryOffset = static_cast<std::uint64_t>(root) + 2u + 12ull * i;
        if (!read_exact(source, entryOffset, entry.data(), entry.size(), audit))
            return ProducerStatus::error(ProducerStatusCode::SourceReadFailed, "cannot read IFD0 entry");
        ++audit.ifdEntriesVisited;
        const std::uint16_t tag = dec16(entry.data(), little);
        const std::uint16_t type = dec16(entry.data() + 2, little);
        const std::uint32_t itemCount = dec32(entry.data() + 4, little);
        const std::uint64_t itemSize = type_size(type);
        if (itemSize == 0) continue;
        std::uint64_t bytes = 0;
        if (!mul_ok(itemCount, itemSize, bytes)) return ProducerStatus::error(ProducerStatusCode::InvalidIfd, "tag size overflow");
        const std::uint64_t dataOffset = (bytes <= 4) ? entryOffset + 8u : dec32(entry.data() + 8, little);
        if (dataOffset > source.sizeBytes() || bytes > source.sizeBytes() - dataOffset)
            return ProducerStatus::error(ProducerStatusCode::InvalidIfd, "tag payload out of range");
        capture_tag(tags, tag, TagRef{true, type, itemCount, dataOffset, bytes});
    }
    audit.parserWorkspacePeakBytes = sizeof(header) + sizeof(countBytes) + sizeof(entry) + sizeof(RootTags);
    audit.secondOrThirdCalibrationSeen = tags.multipleCalibrationTagSeen;
    return ProducerStatus::ok();
}

ProducerStatus read_rational(IRandomAccessByteSource& source, const TagRef& tag, std::uint32_t index, bool little, double& out, ProducerAudit& audit) {
    if (!tag.present || index >= tag.count) return ProducerStatus::error(ProducerStatusCode::InvalidTagCardinality, "rational index out of range");
    if (tag.type != kTiffRational && tag.type != kTiffSRational)
        return ProducerStatus::error(ProducerStatusCode::InvalidTagType, "expected RATIONAL/SRATIONAL tag");
    std::array<std::uint8_t, 8> bytes{};
    if (!read_exact(source, tag.dataOffset + 8ull * index, bytes.data(), bytes.size(), audit))
        return ProducerStatus::error(ProducerStatusCode::SourceReadFailed, "cannot read rational value");
    if (tag.type == kTiffRational) {
        const auto n = dec32(bytes.data(), little);
        const auto d = dec32(bytes.data() + 4, little);
        if (d == 0) return ProducerStatus::error(ProducerStatusCode::InvalidValue, "zero rational denominator");
        out = static_cast<double>(n) / static_cast<double>(d);
    } else {
        const auto n = static_cast<std::int32_t>(dec32(bytes.data(), little));
        const auto d = static_cast<std::int32_t>(dec32(bytes.data() + 4, little));
        if (d == 0) return ProducerStatus::error(ProducerStatusCode::InvalidValue, "zero signed-rational denominator");
        out = static_cast<double>(n) / static_cast<double>(d);
    }
    if (!std::isfinite(out)) return ProducerStatus::error(ProducerStatusCode::InvalidValue, "non-finite rational");
    return ProducerStatus::ok();
}

ProducerStatus read_matrix9(IRandomAccessByteSource& source, const TagRef& tag, bool little, Mat3& out, ProducerAudit& audit) {
    if (!tag.present) return ProducerStatus::error(ProducerStatusCode::MissingColorMatrix, "matrix tag missing");
    if (tag.type != kTiffSRational) return ProducerStatus::error(ProducerStatusCode::InvalidTagType, "DNG matrix must be SRATIONAL");
    if (tag.count != 9) return ProducerStatus::error(ProducerStatusCode::InvalidTagCardinality, "v0.1 requires a 3x3 DNG matrix");
    Mat3 result;
    for (std::uint32_t i = 0; i < 9; ++i) {
        double value = 0;
        auto s = read_rational(source, tag, i, little, value, audit);
        if (!s) return s;
        result.v[i] = value;
    }
    out = result;
    return ProducerStatus::ok();
}

ProducerStatus read_vec3_rational(IRandomAccessByteSource& source, const TagRef& tag, bool little, Vec3& out, ProducerAudit& audit) {
    if (tag.type != kTiffRational) return ProducerStatus::error(ProducerStatusCode::InvalidTagType, "expected RATIONAL vector");
    if (tag.count != 3) return ProducerStatus::error(ProducerStatusCode::InvalidTagCardinality, "v0.1 requires a three-component vector");
    Vec3 result{};
    for (std::uint32_t i = 0; i < 3; ++i) {
        double value = 0;
        auto s = read_rational(source, tag, i, little, value, audit);
        if (!s) return s;
        if (!(value > 0.0)) return ProducerStatus::error(ProducerStatusCode::InvalidValue, "camera neutral/balance components must be positive");
        result[i] = value;
    }
    out = result;
    return ProducerStatus::ok();
}

ProducerStatus read_signature(IRandomAccessByteSource& source, const TagRef& tag, std::string& out, ProducerAudit& audit) {
    out.clear();
    if (!tag.present) return ProducerStatus::ok();
    if (tag.type != kTiffAscii && tag.type != kTiffByte)
        return ProducerStatus::error(ProducerStatusCode::InvalidTagType, "calibration signature must be ASCII/BYTE");
    if (tag.count == 0 || tag.count > kMaxSignatureBytes)
        return ProducerStatus::error(ProducerStatusCode::InvalidTagCardinality, "calibration signature length invalid");
    std::string value(tag.count, '\0');
    if (!read_exact(source, tag.dataOffset, value.data(), value.size(), audit))
        return ProducerStatus::error(ProducerStatusCode::SourceReadFailed, "cannot read calibration signature");
    while (!value.empty() && value.back() == '\0') value.pop_back();
    out = std::move(value);
    audit.parserWorkspacePeakBytes = std::max(audit.parserWorkspacePeakBytes, kMaxSignatureBytes + sizeof(RootTags));
    return ProducerStatus::ok();
}

Mat3 identity3() { return {}; }

Mat3 diag3(const Vec3& d) {
    Mat3 out{{d[0],0,0, 0,d[1],0, 0,0,d[2]}};
    return out;
}

Mat3 mul(const Mat3& a, const Mat3& b) {
    Mat3 out{{0,0,0,0,0,0,0,0,0}};
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            for (int k = 0; k < 3; ++k)
                out.v[static_cast<std::size_t>(3*r+c)] += a.v[static_cast<std::size_t>(3*r+k)] * b.v[static_cast<std::size_t>(3*k+c)];
    return out;
}

Vec3 mul(const Mat3& a, const Vec3& x) {
    Vec3 out{};
    for (int r = 0; r < 3; ++r)
        out[static_cast<std::size_t>(r)] = a.v[static_cast<std::size_t>(3*r)] * x[0] + a.v[static_cast<std::size_t>(3*r+1)] * x[1] + a.v[static_cast<std::size_t>(3*r+2)] * x[2];
    return out;
}

double determinant(const Mat3& a) noexcept {
    const auto& m = a.v;
    return m[0]*(m[4]*m[8]-m[5]*m[7]) - m[1]*(m[3]*m[8]-m[5]*m[6]) + m[2]*(m[3]*m[7]-m[4]*m[6]);
}

ProducerStatus inverse(const Mat3& a, Mat3& out) {
    const double d = determinant(a);
    if (!std::isfinite(d) || std::abs(d) <= kSingularEpsilon)
        return ProducerStatus::error(ProducerStatusCode::SingularMatrix, "DNG color matrix is singular/ill-conditioned for v0.1");
    const auto& m = a.v;
    Mat3 inv{{
        (m[4]*m[8]-m[5]*m[7])/d, (m[2]*m[7]-m[1]*m[8])/d, (m[1]*m[5]-m[2]*m[4])/d,
        (m[5]*m[6]-m[3]*m[8])/d, (m[0]*m[8]-m[2]*m[6])/d, (m[2]*m[3]-m[0]*m[5])/d,
        (m[3]*m[7]-m[4]*m[6])/d, (m[1]*m[6]-m[0]*m[7])/d, (m[0]*m[4]-m[1]*m[3])/d
    }};
    for (const double v : inv.v) if (!std::isfinite(v)) return ProducerStatus::error(ProducerStatusCode::SingularMatrix, "matrix inverse is non-finite");
    out = inv;
    return ProducerStatus::ok();
}

ProducerStatus bradford_to_d50(const Vec3& sourceWhite, Mat3& out) {
    constexpr Mat3 bradford{{0.8951,0.2664,-0.1614, -0.7502,1.7135,0.0367, 0.0389,-0.0685,1.0296}};
    constexpr Mat3 bradfordInv{{0.9869929,-0.1470543,0.1599627, 0.4323053,0.5183603,0.0492912, -0.0085287,0.0400428,0.9684867}};
    constexpr Vec3 d50{{0.96422,1.0,0.82521}};
    const Vec3 srcCone = mul(bradford, sourceWhite);
    const Vec3 dstCone = mul(bradford, d50);
    Vec3 scale{};
    for (std::size_t i = 0; i < 3; ++i) {
        if (!std::isfinite(srcCone[i]) || std::abs(srcCone[i]) <= kSingularEpsilon)
            return ProducerStatus::error(ProducerStatusCode::InvalidValue, "invalid source white for Bradford adaptation");
        scale[i] = dstCone[i] / srcCone[i];
        if (!std::isfinite(scale[i]) || !(scale[i] > 0.0))
            return ProducerStatus::error(ProducerStatusCode::InvalidValue, "invalid Bradford adaptation scale");
    }
    out = mul(mul(bradfordInv, diag3(scale)), bradford);
    return ProducerStatus::ok();
}

ProducerStatus build_without_forward(const Mat3& cm, const Mat3& cc, const Vec3& analog, const Vec3& neutral, Mat3& out) {
    const Mat3 xyzToCamera = mul(mul(diag3(analog), cc), cm);
    Mat3 cameraToXyz;
    auto s = inverse(xyzToCamera, cameraToXyz);
    if (!s) return s;
    const Vec3 white = mul(cameraToXyz, neutral);
    if (!std::isfinite(white[0]) || !std::isfinite(white[1]) || !std::isfinite(white[2]) || !(white[1] > 0.0))
        return ProducerStatus::error(ProducerStatusCode::InvalidValue, "AsShotNeutral maps to invalid XYZ white");
    const Vec3 normalizedWhite{{white[0]/white[1], 1.0, white[2]/white[1]}};
    Mat3 adaptation;
    s = bradford_to_d50(normalizedWhite, adaptation);
    if (!s) return s;
    out = mul(adaptation, cameraToXyz);
    return ProducerStatus::ok();
}

ProducerStatus build_with_forward(const Mat3& fm, const Mat3& cc, const Vec3& analog, const Vec3& neutral, Mat3& out) {
    const Mat3 abcc = mul(diag3(analog), cc);
    Mat3 invAbcc;
    auto s = inverse(abcc, invAbcc);
    if (!s) return s;
    const Vec3 referenceNeutral = mul(invAbcc, neutral);
    Vec3 invNeutral{};
    for (std::size_t i = 0; i < 3; ++i) {
        if (!std::isfinite(referenceNeutral[i]) || !(referenceNeutral[i] > kSingularEpsilon))
            return ProducerStatus::error(ProducerStatusCode::InvalidValue, "reference camera neutral must be positive");
        invNeutral[i] = 1.0 / referenceNeutral[i];
    }
    out = mul(mul(fm, diag3(invNeutral)), invAbcc);
    return ProducerStatus::ok();
}

ProducerStatus validate_output_matrix(const Mat3& matrix) {
    for (const double v : matrix.v) {
        if (!std::isfinite(v) || std::abs(v) > 128.0)
            return ProducerStatus::error(ProducerStatusCode::InvalidValue, "derived cameraToXyzD50 matrix is non-finite/out of research bounds");
    }
    if (std::abs(determinant(matrix)) <= kSingularEpsilon)
        return ProducerStatus::error(ProducerStatusCode::SingularMatrix, "derived cameraToXyzD50 matrix is singular");
    return ProducerStatus::ok();
}

ProducerStatus map_reverify_status(const scientific_preview_binding_v0_1::BindingStatus& status) {
    if (status.code == BindingStatusCode::SourceReadFailed)
        return ProducerStatus::error(ProducerStatusCode::SourceReadFailed, status.message);
    return ProducerStatus::error(ProducerStatusCode::SourceSealMismatch, status.message);
}

} // namespace

ProducerStatus produce_source_metadata_color_binding(IRandomAccessByteSource& source, const SourceSeal& sourceSeal, ProducerResult& out) {
    ProducerResult result;
    auto verified = scientific_preview_binding_v0_1::reverify_source_sha256(source, sourceSeal);
    if (!verified) return map_reverify_status(verified);
    result.audit.sourceVerifiedBeforeParse = true;

    RootTags tags;
    bool little = true;
    auto parsed = parse_root_ifd(source, tags, little, result.audit);
    if (!parsed) return parsed;
    if (tags.multipleCalibrationTagSeen)
        return ProducerStatus::error(ProducerStatusCode::MultipleCalibrationsUnsupported, "v0.1 refuses DNG dual/triple calibration rather than selecting a matrix without required interpolation");
    if (!tags.colorMatrix1.present)
        return ProducerStatus::error(ProducerStatusCode::MissingColorMatrix, "ColorMatrix1 is required in IFD0 for v0.1");
    if (!tags.asShotNeutral.present)
        return ProducerStatus::error(ProducerStatusCode::MissingAsShotNeutral, "AsShotNeutral is required in v0.1");

    Mat3 cm;
    auto s = read_matrix9(source, tags.colorMatrix1, little, cm, result.audit);
    if (!s) return s;

    Vec3 neutral{};
    s = read_vec3_rational(source, tags.asShotNeutral, little, neutral, result.audit);
    if (!s) return s;

    Vec3 analog{{1.0,1.0,1.0}};
    if (tags.analogBalance.present) {
        s = read_vec3_rational(source, tags.analogBalance, little, analog, result.audit);
        if (!s) return s;
    }

    Mat3 cc = identity3();
    result.audit.cameraCalibrationPresent = tags.cameraCalibration1.present;
    if (tags.cameraCalibration1.present) {
        std::string cameraSig, profileSig;
        s = read_signature(source, tags.cameraCalibrationSignature, cameraSig, result.audit);
        if (!s) return s;
        s = read_signature(source, tags.profileCalibrationSignature, profileSig, result.audit);
        if (!s) return s;
        result.audit.cameraCalibrationSignatureMatched = (cameraSig == profileSig);
        if (result.audit.cameraCalibrationSignatureMatched) {
            s = read_matrix9(source, tags.cameraCalibration1, little, cc, result.audit);
            if (!s) return s;
            result.audit.cameraCalibrationApplied = true;
        }
    }

    Mat3 cameraToXyzD50;
    if (tags.forwardMatrix1.present) {
        Mat3 fm;
        s = read_matrix9(source, tags.forwardMatrix1, little, fm, result.audit);
        if (!s) return s;
        s = build_with_forward(fm, cc, analog, neutral, cameraToXyzD50);
        if (!s) return s;
        result.audit.usedForwardMatrix = true;
    } else {
        s = build_without_forward(cm, cc, analog, neutral, cameraToXyzD50);
        if (!s) return s;
    }

    s = validate_output_matrix(cameraToXyzD50);
    if (!s) return s;

    verified = scientific_preview_binding_v0_1::reverify_source_sha256(source, sourceSeal);
    if (!verified) return map_reverify_status(verified);
    result.audit.sourceVerifiedAfterParse = true;

    ScientificColorBindingRecord record;
    record.authority = ColorBindingAuthority::SourceMetadataBound;
    record.sourceEvidenceId = sourceSeal.sourceEvidenceId;
    record.bindingId = std::string("dng-ifd0-source-metadata-v0.1:") +
        (result.audit.usedForwardMatrix ? "fm1:" : "cm1:") + sourceSeal.sourceEvidenceId.substr(7, 16);
    for (std::size_t i = 0; i < 9; ++i) record.cameraToXyzD50[i] = static_cast<float>(cameraToXyzD50.v[i]);
    record.normalized = true;
    record.validated = true;
    record.physicalFrameCount = 1;
    record.independentEvidenceCount = 1;
    result.color = record;
    out = result;
    return ProducerStatus::ok();
}

const char* status_name(ProducerStatusCode code) noexcept {
    switch (code) {
        case ProducerStatusCode::Ok: return "OK";
        case ProducerStatusCode::InvalidArgument: return "INVALID_ARGUMENT";
        case ProducerStatusCode::SourceSealMismatch: return "SOURCE_SEAL_MISMATCH";
        case ProducerStatusCode::SourceReadFailed: return "SOURCE_READ_FAILED";
        case ProducerStatusCode::InvalidTiff: return "INVALID_TIFF";
        case ProducerStatusCode::UnsupportedBigTiff: return "UNSUPPORTED_BIGTIFF";
        case ProducerStatusCode::InvalidIfd: return "INVALID_IFD";
        case ProducerStatusCode::MissingColorMatrix: return "MISSING_COLOR_MATRIX";
        case ProducerStatusCode::MissingAsShotNeutral: return "MISSING_AS_SHOT_NEUTRAL";
        case ProducerStatusCode::MultipleCalibrationsUnsupported: return "MULTIPLE_CALIBRATIONS_UNSUPPORTED";
        case ProducerStatusCode::InvalidTagType: return "INVALID_TAG_TYPE";
        case ProducerStatusCode::InvalidTagCardinality: return "INVALID_TAG_CARDINALITY";
        case ProducerStatusCode::InvalidValue: return "INVALID_VALUE";
        case ProducerStatusCode::SingularMatrix: return "SINGULAR_MATRIX";
    }
    return "UNKNOWN";
}

} // namespace truthraw::dng_color_binding_producer_v0_1