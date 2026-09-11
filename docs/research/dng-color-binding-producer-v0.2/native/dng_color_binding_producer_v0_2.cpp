#include "dng_color_binding_producer_v0_2.h"

#include "dng_color_binding_producer_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <utility>

namespace truthraw::dng_color_binding_producer_v0_2 {
namespace {

using scientific_preview_binding_v0_1::BindingStatusCode;
using scientific_preview_binding_v0_1::ColorBindingAuthority;
using scientific_preview_binding_v0_1::ScientificColorBindingRecord;
using scientific_preview_binding_v0_1::SourceSeal;
using tile_dng_v0_1::IRandomAccessByteSource;

constexpr std::uint16_t kTiffAscii = 2;
constexpr std::uint16_t kTiffShort = 3;
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
constexpr std::uint16_t kTagCalibrationIlluminant1 = 50778;
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
constexpr double kNeutralConvergence = 1.0e-7;
constexpr std::uint32_t kNeutralMaxPasses = 30;
constexpr double kD50x = 0.3457;
constexpr double kD50y = 0.3585;

struct TagRef {
    bool present = false;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::uint64_t dataOffset = 0;
    std::uint64_t dataBytes = 0;
};

struct RootTags {
    TagRef colorMatrix1;
    TagRef colorMatrix2;
    TagRef cameraCalibration1;
    TagRef cameraCalibration2;
    TagRef analogBalance;
    TagRef asShotNeutral;
    TagRef calibrationIlluminant1;
    TagRef calibrationIlluminant2;
    TagRef cameraCalibrationSignature;
    TagRef profileCalibrationSignature;
    TagRef forwardMatrix1;
    TagRef forwardMatrix2;
    bool secondCalibrationTagSeen = false;
    bool thirdCalibrationTagSeen = false;
};

struct Mat3 {
    std::array<double, 9> v{1,0,0, 0,1,0, 0,0,1};
};

using Vec3 = std::array<double, 3>;

struct CalibrationSet {
    std::uint16_t illuminant = 0;
    double temperatureK = 0.0;
    Mat3 colorMatrix{};
    Mat3 cameraCalibration{};
    Mat3 xyzToCameraAtCalibration{};
    Mat3 forwardMatrix{};
    bool hasForwardMatrix = false;
};

struct TempLine {
    double r;
    double u;
    double v;
    double t;
};

// Robertson/Wyszecki-Stiles isotemperature-line table used by the classic
// DNG temperature conversion. This table is public color-science reference
// data; the compact implementation below is independently written to match
// the DNG SDK's documented interpolation behavior for ordinary DNG profiles.
constexpr std::array<TempLine, 31> kTempTable{{
    {0,   0.18006, 0.26352, -0.24341},
    {10,  0.18066, 0.26589, -0.25479},
    {20,  0.18133, 0.26846, -0.26876},
    {30,  0.18208, 0.27119, -0.28539},
    {40,  0.18293, 0.27407, -0.30470},
    {50,  0.18388, 0.27709, -0.32675},
    {60,  0.18494, 0.28021, -0.35156},
    {70,  0.18611, 0.28342, -0.37915},
    {80,  0.18740, 0.28668, -0.40955},
    {90,  0.18880, 0.28997, -0.44278},
    {100, 0.19032, 0.29326, -0.47888},
    {125, 0.19462, 0.30141, -0.58204},
    {150, 0.19962, 0.30921, -0.70471},
    {175, 0.20525, 0.31647, -0.84901},
    {200, 0.21142, 0.32312, -1.0182},
    {225, 0.21807, 0.32909, -1.2168},
    {250, 0.22511, 0.33439, -1.4512},
    {275, 0.23247, 0.33904, -1.7298},
    {300, 0.24010, 0.34308, -2.0637},
    {325, 0.24702, 0.34655, -2.4681},
    {350, 0.25591, 0.34951, -2.9641},
    {375, 0.26400, 0.35200, -3.5814},
    {400, 0.27218, 0.35407, -4.3633},
    {425, 0.28039, 0.35577, -5.3762},
    {450, 0.28863, 0.35714, -6.7262},
    {475, 0.29685, 0.35823, -8.5955},
    {500, 0.30505, 0.35907, -11.324},
    {525, 0.31320, 0.35968, -15.628},
    {550, 0.32129, 0.36011, -23.325},
    {575, 0.32931, 0.36038, -40.770},
    {600, 0.33724, 0.36051, -116.45},
}};

std::uint16_t dec16(const std::uint8_t* p, bool little) noexcept {
    if (little) {
        return static_cast<std::uint16_t>(p[0]) |
               static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[1]) << 8u);
    }
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[0]) << 8u) |
           static_cast<std::uint16_t>(p[1]);
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

bool read_exact(IRandomAccessByteSource& source,
                std::uint64_t offset,
                void* dst,
                std::size_t n,
                ProducerAudit& audit) {
    if (offset > source.sizeBytes() || n > source.sizeBytes() - offset) return false;
    if (!source.readExact(offset, dst, n)) return false;
    audit.metadataBytesRead += n;
    return true;
}

void capture_tag(RootTags& tags, std::uint16_t tag, const TagRef& ref) {
    switch (tag) {
        case kTagColorMatrix1: tags.colorMatrix1 = ref; break;
        case kTagColorMatrix2:
            tags.colorMatrix2 = ref;
            tags.secondCalibrationTagSeen = true;
            break;
        case kTagCameraCalibration1: tags.cameraCalibration1 = ref; break;
        case kTagCameraCalibration2:
            tags.cameraCalibration2 = ref;
            tags.secondCalibrationTagSeen = true;
            break;
        case kTagAnalogBalance: tags.analogBalance = ref; break;
        case kTagAsShotNeutral: tags.asShotNeutral = ref; break;
        case kTagCalibrationIlluminant1: tags.calibrationIlluminant1 = ref; break;
        case kTagCalibrationIlluminant2:
            tags.calibrationIlluminant2 = ref;
            tags.secondCalibrationTagSeen = true;
            break;
        case kTagCameraCalibrationSignature: tags.cameraCalibrationSignature = ref; break;
        case kTagProfileCalibrationSignature: tags.profileCalibrationSignature = ref; break;
        case kTagForwardMatrix1: tags.forwardMatrix1 = ref; break;
        case kTagForwardMatrix2:
            tags.forwardMatrix2 = ref;
            tags.secondCalibrationTagSeen = true;
            break;
        case kTagReductionMatrix2:
            tags.secondCalibrationTagSeen = true;
            break;
        case kTagCalibrationIlluminant3:
        case kTagCameraCalibration3:
        case kTagColorMatrix3:
        case kTagForwardMatrix3:
        case kTagReductionMatrix3:
            tags.thirdCalibrationTagSeen = true;
            break;
        default: break;
    }
}

ProducerStatus parse_root_ifd(IRandomAccessByteSource& source,
                              RootTags& tags,
                              bool& little,
                              ProducerAudit& audit) {
    std::array<std::uint8_t, 8> header{};
    if (!read_exact(source, 0, header.data(), header.size(), audit)) {
        return ProducerStatus::error(ProducerStatusCode::SourceReadFailed,
                                     "cannot read TIFF header");
    }
    if (header[0] == 'I' && header[1] == 'I') little = true;
    else if (header[0] == 'M' && header[1] == 'M') little = false;
    else return ProducerStatus::error(ProducerStatusCode::InvalidTiff,
                                      "invalid TIFF byte order");

    const auto magic = dec16(header.data() + 2, little);
    if (magic == 43) {
        return ProducerStatus::error(ProducerStatusCode::UnsupportedBigTiff,
                                     "BigTIFF color metadata is not supported in v0.2");
    }
    if (magic != 42) {
        return ProducerStatus::error(ProducerStatusCode::InvalidTiff,
                                     "classic TIFF magic 42 required");
    }

    const std::uint32_t root = dec32(header.data() + 4, little);
    if (root == 0 || root > source.sizeBytes() || source.sizeBytes() - root < 2) {
        return ProducerStatus::error(ProducerStatusCode::InvalidIfd,
                                     "IFD0 offset out of range");
    }

    std::array<std::uint8_t, 2> countBytes{};
    if (!read_exact(source, root, countBytes.data(), countBytes.size(), audit)) {
        return ProducerStatus::error(ProducerStatusCode::SourceReadFailed,
                                     "cannot read IFD0 count");
    }
    const std::uint16_t count = dec16(countBytes.data(), little);
    if (count > kMaxIfdEntries) {
        return ProducerStatus::error(ProducerStatusCode::InvalidIfd,
                                     "IFD0 entry cap exceeded");
    }

    std::uint64_t entryBytes = 0;
    if (!mul_ok(count, 12, entryBytes)) {
        return ProducerStatus::error(ProducerStatusCode::InvalidIfd,
                                     "IFD0 size overflow");
    }
    std::uint64_t end = 0;
    if (!add_ok(static_cast<std::uint64_t>(root) + 2u, entryBytes + 4u, end) ||
        end > source.sizeBytes()) {
        return ProducerStatus::error(ProducerStatusCode::InvalidIfd,
                                     "truncated IFD0");
    }

    std::array<std::uint8_t, 12> entry{};
    for (std::uint16_t i = 0; i < count; ++i) {
        const std::uint64_t entryOffset = static_cast<std::uint64_t>(root) + 2u + 12ull * i;
        if (!read_exact(source, entryOffset, entry.data(), entry.size(), audit)) {
            return ProducerStatus::error(ProducerStatusCode::SourceReadFailed,
                                         "cannot read IFD0 entry");
        }
        ++audit.ifdEntriesVisited;
        const std::uint16_t tag = dec16(entry.data(), little);
        const std::uint16_t type = dec16(entry.data() + 2, little);
        const std::uint32_t itemCount = dec32(entry.data() + 4, little);
        const std::uint64_t itemSize = type_size(type);
        if (itemSize == 0) continue;
        std::uint64_t bytes = 0;
        if (!mul_ok(itemCount, itemSize, bytes)) {
            return ProducerStatus::error(ProducerStatusCode::InvalidIfd,
                                         "tag size overflow");
        }
        const std::uint64_t dataOffset =
            (bytes <= 4) ? entryOffset + 8u : dec32(entry.data() + 8, little);
        if (dataOffset > source.sizeBytes() || bytes > source.sizeBytes() - dataOffset) {
            return ProducerStatus::error(ProducerStatusCode::InvalidIfd,
                                         "tag payload out of range");
        }
        capture_tag(tags, tag, TagRef{true, type, itemCount, dataOffset, bytes});
    }

    audit.parserWorkspacePeakBytes =
        sizeof(header) + sizeof(countBytes) + sizeof(entry) + sizeof(RootTags);
    audit.thirdCalibrationSeen = tags.thirdCalibrationTagSeen;
    return ProducerStatus::ok();
}

ProducerStatus read_rational(IRandomAccessByteSource& source,
                             const TagRef& tag,
                             std::uint32_t index,
                             bool little,
                             double& out,
                             ProducerAudit& audit) {
    if (!tag.present || index >= tag.count) {
        return ProducerStatus::error(ProducerStatusCode::InvalidTagCardinality,
                                     "rational index out of range");
    }
    if (tag.type != kTiffRational && tag.type != kTiffSRational) {
        return ProducerStatus::error(ProducerStatusCode::InvalidTagType,
                                     "expected RATIONAL/SRATIONAL tag");
    }
    std::array<std::uint8_t, 8> bytes{};
    if (!read_exact(source,
                    tag.dataOffset + 8ull * index,
                    bytes.data(),
                    bytes.size(),
                    audit)) {
        return ProducerStatus::error(ProducerStatusCode::SourceReadFailed,
                                     "cannot read rational value");
    }
    if (tag.type == kTiffRational) {
        const auto n = dec32(bytes.data(), little);
        const auto d = dec32(bytes.data() + 4, little);
        if (d == 0) {
            return ProducerStatus::error(ProducerStatusCode::InvalidValue,
                                         "zero rational denominator");
        }
        out = static_cast<double>(n) / static_cast<double>(d);
    } else {
        const auto n = static_cast<std::int32_t>(dec32(bytes.data(), little));
        const auto d = static_cast<std::int32_t>(dec32(bytes.data() + 4, little));
        if (d == 0) {
            return ProducerStatus::error(ProducerStatusCode::InvalidValue,
                                         "zero signed-rational denominator");
        }
        out = static_cast<double>(n) / static_cast<double>(d);
    }
    if (!std::isfinite(out)) {
        return ProducerStatus::error(ProducerStatusCode::InvalidValue,
                                     "non-finite rational");
    }
    return ProducerStatus::ok();
}

ProducerStatus read_short(IRandomAccessByteSource& source,
                          const TagRef& tag,
                          bool little,
                          std::uint16_t& out,
                          ProducerAudit& audit) {
    if (!tag.present || tag.type != kTiffShort) {
        return ProducerStatus::error(ProducerStatusCode::InvalidTagType,
                                     "calibration illuminant must be TIFF SHORT");
    }
    if (tag.count != 1) {
        return ProducerStatus::error(ProducerStatusCode::InvalidTagCardinality,
                                     "calibration illuminant must have one value");
    }
    std::array<std::uint8_t, 2> bytes{};
    if (!read_exact(source, tag.dataOffset, bytes.data(), bytes.size(), audit)) {
        return ProducerStatus::error(ProducerStatusCode::SourceReadFailed,
                                     "cannot read calibration illuminant");
    }
    out = dec16(bytes.data(), little);
    return ProducerStatus::ok();
}

ProducerStatus read_matrix9(IRandomAccessByteSource& source,
                            const TagRef& tag,
                            bool little,
                            Mat3& out,
                            ProducerAudit& audit) {
    if (!tag.present) {
        return ProducerStatus::error(ProducerStatusCode::MissingColorMatrix,
                                     "matrix tag missing");
    }
    if (tag.type != kTiffSRational) {
        return ProducerStatus::error(ProducerStatusCode::InvalidTagType,
                                     "DNG matrix must be SRATIONAL");
    }
    if (tag.count != 9) {
        return ProducerStatus::error(ProducerStatusCode::InvalidTagCardinality,
                                     "v0.2 requires a 3x3 DNG matrix");
    }
    Mat3 result;
    for (std::uint32_t i = 0; i < 9; ++i) {
        double value = 0.0;
        auto status = read_rational(source, tag, i, little, value, audit);
        if (!status) return status;
        result.v[i] = value;
    }
    out = result;
    return ProducerStatus::ok();
}

ProducerStatus read_vec3_rational(IRandomAccessByteSource& source,
                                  const TagRef& tag,
                                  bool little,
                                  Vec3& out,
                                  ProducerAudit& audit) {
    if (!tag.present || tag.type != kTiffRational) {
        return ProducerStatus::error(ProducerStatusCode::InvalidTagType,
                                     "expected RATIONAL three-vector");
    }
    if (tag.count != 3) {
        return ProducerStatus::error(ProducerStatusCode::InvalidTagCardinality,
                                     "v0.2 requires a three-component vector");
    }
    Vec3 result{};
    for (std::uint32_t i = 0; i < 3; ++i) {
        double value = 0.0;
        auto status = read_rational(source, tag, i, little, value, audit);
        if (!status) return status;
        if (!(value > 0.0)) {
            return ProducerStatus::error(ProducerStatusCode::InvalidValue,
                                         "camera neutral/balance components must be positive");
        }
        result[i] = value;
    }
    out = result;
    return ProducerStatus::ok();
}

ProducerStatus read_signature(IRandomAccessByteSource& source,
                              const TagRef& tag,
                              std::string& out,
                              ProducerAudit& audit) {
    out.clear();
    if (!tag.present) return ProducerStatus::ok();
    if (tag.type != kTiffAscii && tag.type != kTiffByte) {
        return ProducerStatus::error(ProducerStatusCode::InvalidTagType,
                                     "calibration signature must be ASCII/BYTE");
    }
    if (tag.count == 0 || tag.count > kMaxSignatureBytes) {
        return ProducerStatus::error(ProducerStatusCode::InvalidTagCardinality,
                                     "calibration signature length invalid");
    }
    std::string value(tag.count, '\0');
    if (!read_exact(source, tag.dataOffset, value.data(), value.size(), audit)) {
        return ProducerStatus::error(ProducerStatusCode::SourceReadFailed,
                                     "cannot read calibration signature");
    }
    while (!value.empty() && value.back() == '\0') value.pop_back();
    out = std::move(value);
    audit.parserWorkspacePeakBytes =
        std::max(audit.parserWorkspacePeakBytes,
                 kMaxSignatureBytes + sizeof(RootTags));
    return ProducerStatus::ok();
}

Mat3 identity3() { return {}; }

Mat3 diag3(const Vec3& d) {
    return Mat3{{d[0],0,0, 0,d[1],0, 0,0,d[2]}};
}

Mat3 mul(const Mat3& a, const Mat3& b) {
    Mat3 out{{0,0,0,0,0,0,0,0,0}};
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            for (int k = 0; k < 3; ++k) {
                out.v[static_cast<std::size_t>(3*r+c)] +=
                    a.v[static_cast<std::size_t>(3*r+k)] *
                    b.v[static_cast<std::size_t>(3*k+c)];
            }
        }
    }
    return out;
}

Vec3 mul(const Mat3& a, const Vec3& x) {
    Vec3 out{};
    for (int r = 0; r < 3; ++r) {
        out[static_cast<std::size_t>(r)] =
            a.v[static_cast<std::size_t>(3*r)] * x[0] +
            a.v[static_cast<std::size_t>(3*r+1)] * x[1] +
            a.v[static_cast<std::size_t>(3*r+2)] * x[2];
    }
    return out;
}

Mat3 lerp(const Mat3& a, const Mat3& b, double g) {
    Mat3 out{{0,0,0,0,0,0,0,0,0}};
    for (std::size_t i = 0; i < out.v.size(); ++i) {
        out.v[i] = g * a.v[i] + (1.0 - g) * b.v[i];
    }
    return out;
}

double determinant(const Mat3& a) noexcept {
    const auto& m = a.v;
    return m[0]*(m[4]*m[8]-m[5]*m[7]) -
           m[1]*(m[3]*m[8]-m[5]*m[6]) +
           m[2]*(m[3]*m[7]-m[4]*m[6]);
}

ProducerStatus inverse(const Mat3& a, Mat3& out) {
    const double d = determinant(a);
    if (!std::isfinite(d) || std::abs(d) <= kSingularEpsilon) {
        return ProducerStatus::error(ProducerStatusCode::SingularMatrix,
                                     "DNG color matrix is singular/ill-conditioned");
    }
    const auto& m = a.v;
    Mat3 inv{{
        (m[4]*m[8]-m[5]*m[7])/d, (m[2]*m[7]-m[1]*m[8])/d, (m[1]*m[5]-m[2]*m[4])/d,
        (m[5]*m[6]-m[3]*m[8])/d, (m[0]*m[8]-m[2]*m[6])/d, (m[2]*m[3]-m[0]*m[5])/d,
        (m[3]*m[7]-m[4]*m[6])/d, (m[1]*m[6]-m[0]*m[7])/d, (m[0]*m[4]-m[1]*m[3])/d
    }};
    for (const double v : inv.v) {
        if (!std::isfinite(v)) {
            return ProducerStatus::error(ProducerStatusCode::SingularMatrix,
                                         "matrix inverse is non-finite");
        }
    }
    out = inv;
    return ProducerStatus::ok();
}

bool xyz_to_xy(const Vec3& xyz, double& x, double& y) {
    const double sum = xyz[0] + xyz[1] + xyz[2];
    if (!std::isfinite(sum) || !(sum > kSingularEpsilon)) return false;
    x = xyz[0] / sum;
    y = xyz[1] / sum;
    return std::isfinite(x) && std::isfinite(y) && x > 0.0 && y > 0.0 && x + y < 1.0;
}

ProducerStatus correlated_temperature_from_xy(double x, double y, double& temperatureK) {
    const double denom = 1.5 - x + 6.0 * y;
    if (!std::isfinite(denom) || std::abs(denom) <= kSingularEpsilon) {
        return ProducerStatus::error(ProducerStatusCode::InvalidValue,
                                     "white xy cannot be converted to uv");
    }
    const double u = 2.0 * x / denom;
    const double v = 3.0 * y / denom;
    if (!std::isfinite(u) || !std::isfinite(v)) {
        return ProducerStatus::error(ProducerStatusCode::InvalidValue,
                                     "white uv is non-finite");
    }

    double lastDt = 0.0;
    for (std::size_t index = 1; index <= 30; ++index) {
        double du = 1.0;
        double dv = kTempTable[index].t;
        const double len = std::sqrt(1.0 + dv * dv);
        du /= len;
        dv /= len;
        const double uu = u - kTempTable[index].u;
        const double vv = v - kTempTable[index].v;
        double dt = -uu * dv + vv * du;
        if (dt <= 0.0 || index == 30) {
            if (dt > 0.0) dt = 0.0;
            dt = -dt;
            const double denominator = lastDt + dt;
            const double f =
                (index == 1 || std::abs(denominator) <= kSingularEpsilon)
                    ? 0.0
                    : dt / denominator;
            const double reciprocalMegaK =
                kTempTable[index - 1].r * f +
                kTempTable[index].r * (1.0 - f);
            if (!std::isfinite(reciprocalMegaK) ||
                reciprocalMegaK <= kSingularEpsilon) {
                return ProducerStatus::error(ProducerStatusCode::InvalidValue,
                                             "correlated color temperature is outside finite v0.2 range");
            }
            temperatureK = 1.0e6 / reciprocalMegaK;
            if (!std::isfinite(temperatureK) || !(temperatureK > 0.0)) {
                return ProducerStatus::error(ProducerStatusCode::InvalidValue,
                                             "correlated color temperature is invalid");
            }
            return ProducerStatus::ok();
        }
        lastDt = dt;
    }
    return ProducerStatus::error(ProducerStatusCode::InvalidValue,
                                 "correlated color temperature solve failed");
}

ProducerStatus illuminant_temperature(std::uint16_t light, double& temperatureK) {
    switch (light) {
        case 3:  // Tungsten
        case 17: // Standard Light A
            temperatureK = 2850.0;
            return ProducerStatus::ok();
        case 24: // ISO studio tungsten
            temperatureK = 3200.0;
            return ProducerStatus::ok();
        case 23: // D50
            temperatureK = 5000.0;
            return ProducerStatus::ok();
        case 1:  // Daylight
        case 4:  // Flash
        case 9:  // Fine weather
        case 18: // Standard Light B
        case 20: // D55
            temperatureK = 5500.0;
            return ProducerStatus::ok();
        case 10: // Cloudy
        case 19: // Standard Light C
        case 21: // D65
            temperatureK = 6500.0;
            return ProducerStatus::ok();
        case 11: // Shade
        case 22: // D75
            temperatureK = 7500.0;
            return ProducerStatus::ok();
        case 12: // Daylight fluorescent
            temperatureK = (5700.0 + 7100.0) * 0.5;
            return ProducerStatus::ok();
        case 13: // Day white fluorescent
            temperatureK = (4600.0 + 5500.0) * 0.5;
            return ProducerStatus::ok();
        case 2:  // Fluorescent
        case 14: // Cool white fluorescent
            temperatureK = (3800.0 + 4500.0) * 0.5;
            return ProducerStatus::ok();
        case 15: // White fluorescent
            temperatureK = (3250.0 + 3800.0) * 0.5;
            return ProducerStatus::ok();
        case 16: // Warm white fluorescent
            temperatureK = (2600.0 + 3250.0) * 0.5;
            return ProducerStatus::ok();
        case 0:
            return ProducerStatus::error(ProducerStatusCode::UnsupportedCalibrationIlluminant,
                                         "unknown calibration illuminant is invalid for dual interpolation");
        case 255:
            return ProducerStatus::error(ProducerStatusCode::UnsupportedCalibrationIlluminant,
                                         "custom calibration illuminant requires IlluminantData parsing, not implemented in v0.2");
        default:
            return ProducerStatus::error(ProducerStatusCode::UnsupportedCalibrationIlluminant,
                                         "calibration illuminant is not mapped by DNG dual-interpolation v0.2");
    }
}

double interpolation_weight_low(double temperatureK,
                                double lowTemperatureK,
                                double highTemperatureK) {
    if (temperatureK <= lowTemperatureK) return 1.0;
    if (temperatureK >= highTemperatureK) return 0.0;
    const double invT = 1.0 / temperatureK;
    return (invT - 1.0 / highTemperatureK) /
           (1.0 / lowTemperatureK - 1.0 / highTemperatureK);
}

ProducerStatus normalize_forward_matrix(Mat3& matrix) {
    constexpr Vec3 d50{{0.96422, 1.0, 0.82521}};
    constexpr Vec3 ones{{1.0, 1.0, 1.0}};
    const Vec3 xyz = mul(matrix, ones);
    Vec3 scale{};
    for (std::size_t i = 0; i < 3; ++i) {
        if (!std::isfinite(xyz[i]) || std::abs(xyz[i]) <= kSingularEpsilon) {
            return ProducerStatus::error(ProducerStatusCode::InvalidValue,
                                         "ForwardMatrix cannot be normalized to D50");
        }
        scale[i] = d50[i] / xyz[i];
        if (!std::isfinite(scale[i])) {
            return ProducerStatus::error(ProducerStatusCode::InvalidValue,
                                         "ForwardMatrix normalization produced non-finite scale");
        }
    }
    matrix = mul(diag3(scale), matrix);
    return ProducerStatus::ok();
}

ProducerStatus bradford_to_d50(const Vec3& sourceWhite, Mat3& out) {
    constexpr Mat3 bradford{{
        0.8951,0.2664,-0.1614,
        -0.7502,1.7135,0.0367,
        0.0389,-0.0685,1.0296}};
    constexpr Mat3 bradfordInv{{
        0.9869929,-0.1470543,0.1599627,
        0.4323053,0.5183603,0.0492912,
        -0.0085287,0.0400428,0.9684867}};
    constexpr Vec3 d50{{0.96422,1.0,0.82521}};
    const Vec3 srcCone = mul(bradford, sourceWhite);
    const Vec3 dstCone = mul(bradford, d50);
    Vec3 scale{};
    for (std::size_t i = 0; i < 3; ++i) {
        if (!std::isfinite(srcCone[i]) || std::abs(srcCone[i]) <= kSingularEpsilon) {
            return ProducerStatus::error(ProducerStatusCode::InvalidValue,
                                         "invalid source white for Bradford adaptation");
        }
        scale[i] = dstCone[i] / srcCone[i];
        if (!std::isfinite(scale[i]) || !(scale[i] > 0.0)) {
            return ProducerStatus::error(ProducerStatusCode::InvalidValue,
                                         "invalid Bradford adaptation scale");
        }
    }
    out = mul(mul(bradfordInv, diag3(scale)), bradford);
    return ProducerStatus::ok();
}

ProducerStatus build_with_forward(const Mat3& fm,
                                  const Mat3& cc,
                                  const Vec3& analog,
                                  const Vec3& neutral,
                                  Mat3& out) {
    const Mat3 abcc = mul(diag3(analog), cc);
    Mat3 invAbcc;
    auto status = inverse(abcc, invAbcc);
    if (!status) return status;
    const Vec3 referenceNeutral = mul(invAbcc, neutral);
    Vec3 invNeutral{};
    for (std::size_t i = 0; i < 3; ++i) {
        if (!std::isfinite(referenceNeutral[i]) ||
            !(referenceNeutral[i] > kSingularEpsilon)) {
            return ProducerStatus::error(ProducerStatusCode::InvalidValue,
                                         "reference camera neutral must be positive");
        }
        invNeutral[i] = 1.0 / referenceNeutral[i];
    }
    out = mul(mul(fm, diag3(invNeutral)), invAbcc);
    return ProducerStatus::ok();
}

ProducerStatus build_without_forward(const Mat3& xyzToCamera,
                                     const Vec3& neutral,
                                     Mat3& out) {
    Mat3 cameraToXyz;
    auto status = inverse(xyzToCamera, cameraToXyz);
    if (!status) return status;
    const Vec3 white = mul(cameraToXyz, neutral);
    if (!std::isfinite(white[0]) || !std::isfinite(white[1]) ||
        !std::isfinite(white[2]) || !(white[1] > 0.0)) {
        return ProducerStatus::error(ProducerStatusCode::InvalidValue,
                                     "AsShotNeutral maps to invalid XYZ white");
    }
    const Vec3 normalizedWhite{{white[0] / white[1], 1.0, white[2] / white[1]}};
    Mat3 adaptation;
    status = bradford_to_d50(normalizedWhite, adaptation);
    if (!status) return status;
    out = mul(adaptation, cameraToXyz);
    return ProducerStatus::ok();
}

ProducerStatus validate_output_matrix(const Mat3& matrix) {
    for (const double v : matrix.v) {
        if (!std::isfinite(v) || std::abs(v) > 128.0) {
            return ProducerStatus::error(ProducerStatusCode::InvalidValue,
                                         "derived cameraToXyzD50 matrix is non-finite/out of research bounds");
        }
    }
    if (std::abs(determinant(matrix)) <= kSingularEpsilon) {
        return ProducerStatus::error(ProducerStatusCode::SingularMatrix,
                                     "derived cameraToXyzD50 matrix is singular");
    }
    return ProducerStatus::ok();
}

ProducerStatus map_reverify_status(
    const scientific_preview_binding_v0_1::BindingStatus& status) {
    if (status.code == BindingStatusCode::SourceReadFailed) {
        return ProducerStatus::error(ProducerStatusCode::SourceReadFailed,
                                     status.message);
    }
    return ProducerStatus::error(ProducerStatusCode::SourceSealMismatch,
                                 status.message);
}

ProducerStatus map_v01_status(
    const dng_color_binding_producer_v0_1::ProducerStatus& status) {
    using V1 = dng_color_binding_producer_v0_1::ProducerStatusCode;
    switch (status.code) {
        case V1::Ok: return ProducerStatus::ok();
        case V1::InvalidArgument: return ProducerStatus::error(ProducerStatusCode::InvalidArgument, status.message);
        case V1::SourceSealMismatch: return ProducerStatus::error(ProducerStatusCode::SourceSealMismatch, status.message);
        case V1::SourceReadFailed: return ProducerStatus::error(ProducerStatusCode::SourceReadFailed, status.message);
        case V1::InvalidTiff: return ProducerStatus::error(ProducerStatusCode::InvalidTiff, status.message);
        case V1::UnsupportedBigTiff: return ProducerStatus::error(ProducerStatusCode::UnsupportedBigTiff, status.message);
        case V1::InvalidIfd: return ProducerStatus::error(ProducerStatusCode::InvalidIfd, status.message);
        case V1::MissingColorMatrix: return ProducerStatus::error(ProducerStatusCode::MissingColorMatrix, status.message);
        case V1::MissingAsShotNeutral: return ProducerStatus::error(ProducerStatusCode::MissingAsShotNeutral, status.message);
        case V1::MultipleCalibrationsUnsupported:
            return ProducerStatus::error(ProducerStatusCode::IncompleteDualCalibration, status.message);
        case V1::InvalidTagType: return ProducerStatus::error(ProducerStatusCode::InvalidTagType, status.message);
        case V1::InvalidTagCardinality: return ProducerStatus::error(ProducerStatusCode::InvalidTagCardinality, status.message);
        case V1::InvalidValue: return ProducerStatus::error(ProducerStatusCode::InvalidValue, status.message);
        case V1::SingularMatrix: return ProducerStatus::error(ProducerStatusCode::SingularMatrix, status.message);
    }
    return ProducerStatus::error(ProducerStatusCode::InvalidValue,
                                 "unknown v0.1 delegated status");
}

void copy_v01_audit(const dng_color_binding_producer_v0_1::ProducerAudit& source,
                    ProducerAudit& target) {
    target.metadataBytesRead += source.metadataBytesRead;
    target.ifdEntriesVisited += source.ifdEntriesVisited;
    target.parserWorkspacePeakBytes =
        std::max(target.parserWorkspacePeakBytes, source.parserWorkspacePeakBytes);
    target.sourceVerifiedBeforeParse = source.sourceVerifiedBeforeParse;
    target.sourceVerifiedAfterParse = source.sourceVerifiedAfterParse;
    target.usedForwardMatrix = source.usedForwardMatrix;
    target.cameraCalibrationPresent = source.cameraCalibrationPresent;
    target.cameraCalibrationSignatureMatched = source.cameraCalibrationSignatureMatched;
    target.cameraCalibrationApplied = source.cameraCalibrationApplied;
    target.delegatedSingleIlluminantV01 = true;
}

ProducerStatus solve_neutral_white(const CalibrationSet& low,
                                   const CalibrationSet& high,
                                   const Vec3& neutral,
                                   ProducerAudit& audit,
                                   double& whiteX,
                                   double& whiteY,
                                   double& whiteTemperatureK,
                                   double& weightLow) {
    double lastX = kD50x;
    double lastY = kD50y;

    for (std::uint32_t pass = 0; pass < kNeutralMaxPasses; ++pass) {
        double temperatureK = 0.0;
        auto status = correlated_temperature_from_xy(lastX, lastY, temperatureK);
        if (!status) return status;
        const double g = interpolation_weight_low(
            temperatureK, low.temperatureK, high.temperatureK);
        const Mat3 xyzToCamera = lerp(
            low.xyzToCameraAtCalibration,
            high.xyzToCameraAtCalibration,
            g);
        Mat3 cameraToXyz;
        status = inverse(xyzToCamera, cameraToXyz);
        if (!status) return status;
        const Vec3 xyz = mul(cameraToXyz, neutral);
        double nextX = 0.0;
        double nextY = 0.0;
        if (!xyz_to_xy(xyz, nextX, nextY)) {
            return ProducerStatus::error(ProducerStatusCode::InvalidValue,
                                         "NeutralToXY produced invalid chromaticity");
        }
        audit.neutralSolveIterations = pass + 1u;
        if (std::abs(nextX - lastX) + std::abs(nextY - lastY) <
            kNeutralConvergence) {
            whiteX = nextX;
            whiteY = nextY;
            status = correlated_temperature_from_xy(whiteX, whiteY, whiteTemperatureK);
            if (!status) return status;
            weightLow = interpolation_weight_low(
                whiteTemperatureK, low.temperatureK, high.temperatureK);
            return ProducerStatus::ok();
        }
        lastX = nextX;
        lastY = nextY;
    }

    return ProducerStatus::error(ProducerStatusCode::NeutralSolveDidNotConverge,
                                 "AsShotNeutral to white-xy iteration did not converge within 30 passes");
}

} // namespace

ProducerStatus produce_source_metadata_color_binding(
    IRandomAccessByteSource& source,
    const SourceSeal& sourceSeal,
    ProducerResult& out) {
    ProducerResult result;

    auto verified = scientific_preview_binding_v0_1::reverify_source_sha256(
        source, sourceSeal);
    if (!verified) return map_reverify_status(verified);
    result.audit.sourceVerifiedBeforeParse = true;

    RootTags tags;
    bool little = true;
    auto parsed = parse_root_ifd(source, tags, little, result.audit);
    if (!parsed) return parsed;

    if (tags.thirdCalibrationTagSeen) {
        return ProducerStatus::error(
            ProducerStatusCode::TripleCalibrationUnsupported,
            "v0.2 supports single/dual DNG calibration only; third calibration remains fail-closed");
    }

    if (!tags.secondCalibrationTagSeen) {
        dng_color_binding_producer_v0_1::ProducerResult delegated;
        const auto oldStatus =
            dng_color_binding_producer_v0_1::produce_source_metadata_color_binding(
                source, sourceSeal, delegated);
        if (!oldStatus) return map_v01_status(oldStatus);
        copy_v01_audit(delegated.audit, result.audit);
        result.color = delegated.color;
        out = result;
        return ProducerStatus::ok();
    }

    result.audit.dualIlluminantUsed = true;

    if (!tags.colorMatrix1.present || !tags.colorMatrix2.present ||
        !tags.calibrationIlluminant1.present || !tags.calibrationIlluminant2.present) {
        return ProducerStatus::error(
            ProducerStatusCode::IncompleteDualCalibration,
            "dual DNG calibration requires ColorMatrix1/2 and CalibrationIlluminant1/2");
    }
    if (!tags.asShotNeutral.present) {
        return ProducerStatus::error(ProducerStatusCode::MissingAsShotNeutral,
                                     "AsShotNeutral is required for dual DNG interpolation");
    }

    std::uint16_t illum1 = 0;
    std::uint16_t illum2 = 0;
    auto status = read_short(source, tags.calibrationIlluminant1, little, illum1, result.audit);
    if (!status) return status;
    status = read_short(source, tags.calibrationIlluminant2, little, illum2, result.audit);
    if (!status) return status;
    result.audit.calibrationIlluminant1 = illum1;
    result.audit.calibrationIlluminant2 = illum2;

    CalibrationSet first;
    CalibrationSet second;
    first.illuminant = illum1;
    second.illuminant = illum2;
    status = illuminant_temperature(illum1, first.temperatureK);
    if (!status) return status;
    status = illuminant_temperature(illum2, second.temperatureK);
    if (!status) return status;
    if (std::abs(first.temperatureK - second.temperatureK) <= kSingularEpsilon) {
        return ProducerStatus::error(
            ProducerStatusCode::InvalidValue,
            "dual DNG calibration temperatures must differ for inverse-temperature interpolation");
    }

    status = read_matrix9(source, tags.colorMatrix1, little, first.colorMatrix, result.audit);
    if (!status) return status;
    status = read_matrix9(source, tags.colorMatrix2, little, second.colorMatrix, result.audit);
    if (!status) return status;

    Vec3 neutral{};
    status = read_vec3_rational(source, tags.asShotNeutral, little, neutral, result.audit);
    if (!status) return status;

    Vec3 analog{{1.0, 1.0, 1.0}};
    if (tags.analogBalance.present) {
        status = read_vec3_rational(source, tags.analogBalance, little, analog, result.audit);
        if (!status) return status;
    }

    first.cameraCalibration = identity3();
    second.cameraCalibration = identity3();
    result.audit.cameraCalibrationPresent =
        tags.cameraCalibration1.present || tags.cameraCalibration2.present;
    if (result.audit.cameraCalibrationPresent) {
        std::string cameraSig;
        std::string profileSig;
        status = read_signature(source, tags.cameraCalibrationSignature, cameraSig, result.audit);
        if (!status) return status;
        status = read_signature(source, tags.profileCalibrationSignature, profileSig, result.audit);
        if (!status) return status;
        result.audit.cameraCalibrationSignatureMatched = (cameraSig == profileSig);
        if (result.audit.cameraCalibrationSignatureMatched) {
            if (tags.cameraCalibration1.present) {
                status = read_matrix9(source,
                                      tags.cameraCalibration1,
                                      little,
                                      first.cameraCalibration,
                                      result.audit);
                if (!status) return status;
            }
            if (tags.cameraCalibration2.present) {
                status = read_matrix9(source,
                                      tags.cameraCalibration2,
                                      little,
                                      second.cameraCalibration,
                                      result.audit);
                if (!status) return status;
            }
            result.audit.cameraCalibrationApplied = true;
        }
    }

    if (tags.forwardMatrix1.present) {
        status = read_matrix9(source, tags.forwardMatrix1, little, first.forwardMatrix, result.audit);
        if (!status) return status;
        status = normalize_forward_matrix(first.forwardMatrix);
        if (!status) return status;
        first.hasForwardMatrix = true;
    }
    if (tags.forwardMatrix2.present) {
        status = read_matrix9(source, tags.forwardMatrix2, little, second.forwardMatrix, result.audit);
        if (!status) return status;
        status = normalize_forward_matrix(second.forwardMatrix);
        if (!status) return status;
        second.hasForwardMatrix = true;
    }

    first.xyzToCameraAtCalibration =
        mul(mul(diag3(analog), first.cameraCalibration), first.colorMatrix);
    second.xyzToCameraAtCalibration =
        mul(mul(diag3(analog), second.cameraCalibration), second.colorMatrix);

    CalibrationSet low = first;
    CalibrationSet high = second;
    if (low.temperatureK > high.temperatureK) std::swap(low, high);
    result.audit.calibrationTemperatureLowK = low.temperatureK;
    result.audit.calibrationTemperatureHighK = high.temperatureK;

    double whiteX = 0.0;
    double whiteY = 0.0;
    double whiteTemperatureK = 0.0;
    double weightLow = 0.0;
    status = solve_neutral_white(low,
                                 high,
                                 neutral,
                                 result.audit,
                                 whiteX,
                                 whiteY,
                                 whiteTemperatureK,
                                 weightLow);
    if (!status) return status;
    result.audit.resolvedWhiteX = whiteX;
    result.audit.resolvedWhiteY = whiteY;
    result.audit.resolvedWhiteTemperatureK = whiteTemperatureK;
    result.audit.interpolationWeightLow = weightLow;

    const Mat3 xyzToCamera = lerp(
        low.xyzToCameraAtCalibration,
        high.xyzToCameraAtCalibration,
        weightLow);
    const Mat3 interpolatedCc = lerp(
        low.cameraCalibration,
        high.cameraCalibration,
        weightLow);

    Mat3 cameraToXyzD50;
    if (low.hasForwardMatrix && high.hasForwardMatrix) {
        const Mat3 fm = lerp(low.forwardMatrix, high.forwardMatrix, weightLow);
        status = build_with_forward(fm,
                                    interpolatedCc,
                                    analog,
                                    neutral,
                                    cameraToXyzD50);
        if (!status) return status;
        result.audit.usedForwardMatrix = true;
    } else if (low.hasForwardMatrix || high.hasForwardMatrix) {
        const Mat3& fm = low.hasForwardMatrix ? low.forwardMatrix : high.forwardMatrix;
        status = build_with_forward(fm,
                                    interpolatedCc,
                                    analog,
                                    neutral,
                                    cameraToXyzD50);
        if (!status) return status;
        result.audit.usedForwardMatrix = true;
        result.audit.usedSingleForwardMatrixAcrossTemperatures = true;
    } else {
        status = build_without_forward(xyzToCamera, neutral, cameraToXyzD50);
        if (!status) return status;
    }

    status = validate_output_matrix(cameraToXyzD50);
    if (!status) return status;

    verified = scientific_preview_binding_v0_1::reverify_source_sha256(source, sourceSeal);
    if (!verified) return map_reverify_status(verified);
    result.audit.sourceVerifiedAfterParse = true;

    ScientificColorBindingRecord record;
    record.authority = ColorBindingAuthority::SourceMetadataBound;
    record.sourceEvidenceId = sourceSeal.sourceEvidenceId;
    record.bindingId = std::string("dng-ifd0-source-metadata-v0.2:dual-ict:") +
        (result.audit.usedForwardMatrix ? "fm:" : "cm:") +
        sourceSeal.sourceEvidenceId.substr(7, 16);
    for (std::size_t i = 0; i < 9; ++i) {
        record.cameraToXyzD50[i] = static_cast<float>(cameraToXyzD50.v[i]);
    }
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
        case ProducerStatusCode::IncompleteDualCalibration: return "INCOMPLETE_DUAL_CALIBRATION";
        case ProducerStatusCode::TripleCalibrationUnsupported: return "TRIPLE_CALIBRATION_UNSUPPORTED";
        case ProducerStatusCode::UnsupportedCalibrationIlluminant: return "UNSUPPORTED_CALIBRATION_ILLUMINANT";
        case ProducerStatusCode::InvalidTagType: return "INVALID_TAG_TYPE";
        case ProducerStatusCode::InvalidTagCardinality: return "INVALID_TAG_CARDINALITY";
        case ProducerStatusCode::InvalidValue: return "INVALID_VALUE";
        case ProducerStatusCode::SingularMatrix: return "SINGULAR_MATRIX";
        case ProducerStatusCode::NeutralSolveDidNotConverge: return "NEUTRAL_SOLVE_DID_NOT_CONVERGE";
    }
    return "UNKNOWN";
}

} // namespace truthraw::dng_color_binding_producer_v0_2
