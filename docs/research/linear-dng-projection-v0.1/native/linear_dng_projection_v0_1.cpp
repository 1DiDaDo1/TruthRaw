#include "linear_dng_projection_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <vector>

namespace truthraw::linear_dng_projection::v0_1 {
namespace {

using Hash256 = std::array<std::uint8_t, 32>;

constexpr std::uint16_t BYTE = 1;
constexpr std::uint16_t ASCII = 2;
constexpr std::uint16_t SHORT = 3;
constexpr std::uint16_t LONG = 4;
constexpr std::uint16_t RATIONAL = 5;
constexpr std::uint16_t SRATIONAL = 10;

constexpr std::uint16_t TAG_NewSubFileType = 254;
constexpr std::uint16_t TAG_ImageWidth = 256;
constexpr std::uint16_t TAG_ImageLength = 257;
constexpr std::uint16_t TAG_BitsPerSample = 258;
constexpr std::uint16_t TAG_Compression = 259;
constexpr std::uint16_t TAG_PhotometricInterpretation = 262;
constexpr std::uint16_t TAG_ImageDescription = 270;
constexpr std::uint16_t TAG_Orientation = 274;
constexpr std::uint16_t TAG_SamplesPerPixel = 277;
constexpr std::uint16_t TAG_PlanarConfiguration = 284;
constexpr std::uint16_t TAG_Software = 305;
constexpr std::uint16_t TAG_TileWidth = 322;
constexpr std::uint16_t TAG_TileLength = 323;
constexpr std::uint16_t TAG_TileOffsets = 324;
constexpr std::uint16_t TAG_TileByteCounts = 325;
constexpr std::uint16_t TAG_SampleFormat = 339;
constexpr std::uint16_t TAG_DNGVersion = 50706;
constexpr std::uint16_t TAG_DNGBackwardVersion = 50707;
constexpr std::uint16_t TAG_UniqueCameraModel = 50708;
constexpr std::uint16_t TAG_WhiteLevel = 50717;
constexpr std::uint16_t TAG_ColorMatrix1 = 50721;
constexpr std::uint16_t TAG_AsShotNeutral = 50728;
constexpr std::uint16_t TAG_CalibrationIlluminant1 = 50778;

constexpr std::uint16_t PHOTOMETRIC_LinearRaw = 34892;
constexpr std::uint16_t LIGHTSOURCE_D50 = 23;
constexpr std::uint16_t BITS = 16;
constexpr std::uint16_t WHITE = 65535;
constexpr std::size_t IFD_COUNT = 23;
constexpr double D50_X = 0.96422;
constexpr double D50_Y = 1.0;
constexpr double D50_Z = 0.82521;
constexpr double DET_EPS = 1.0e-12;
constexpr std::int64_t RAT_DEN = 1000000;

struct Mat3 { std::array<double, 9> v{}; };
struct Vec3 { std::array<double, 3> v{}; };
struct Entry {
    std::uint16_t tag = 0;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::uint32_t value = 0;
};

bool zero_hash(const Hash256& h) noexcept {
    for (const auto b : h) if (b != 0) return false;
    return true;
}

std::string hash_hex(const Hash256& h) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto b : h) ss << std::setw(2) << static_cast<unsigned>(b);
    return ss.str();
}

const char* authority_text(
    scientific_preview_binding_v0_1::ColorBindingAuthority a) noexcept {
    using A = scientific_preview_binding_v0_1::ColorBindingAuthority;
    switch (a) {
        case A::Unverified: return "UNVERIFIED";
        case A::PreviewSentinel: return "PREVIEW_SENTINEL";
        case A::SourceMetadataBound: return "SOURCE_METADATA_BOUND";
        case A::GatehouseCertifiedMetadata: return "GATEHOUSE_CERTIFIED_METADATA";
        case A::IndependentCalibration: return "INDEPENDENT_CALIBRATION";
    }
    return "UNKNOWN";
}

std::uint32_t align4(std::uint32_t v) noexcept { return (v + 3u) & ~3u; }

void put16(std::vector<std::uint8_t>& b, std::size_t o, std::uint16_t v) {
    b[o] = static_cast<std::uint8_t>(v);
    b[o + 1] = static_cast<std::uint8_t>(v >> 8u);
}

void put32(std::vector<std::uint8_t>& b, std::size_t o, std::uint32_t v) {
    b[o] = static_cast<std::uint8_t>(v);
    b[o + 1] = static_cast<std::uint8_t>(v >> 8u);
    b[o + 2] = static_cast<std::uint8_t>(v >> 16u);
    b[o + 3] = static_cast<std::uint8_t>(v >> 24u);
}

void append16(std::vector<std::uint8_t>& b, std::uint16_t v) {
    b.push_back(static_cast<std::uint8_t>(v));
    b.push_back(static_cast<std::uint8_t>(v >> 8u));
}

void append32(std::vector<std::uint8_t>& b, std::uint32_t v) {
    b.push_back(static_cast<std::uint8_t>(v));
    b.push_back(static_cast<std::uint8_t>(v >> 8u));
    b.push_back(static_cast<std::uint8_t>(v >> 16u));
    b.push_back(static_cast<std::uint8_t>(v >> 24u));
}

void append_i32(std::vector<std::uint8_t>& b, std::int32_t v) {
    append32(b, static_cast<std::uint32_t>(v));
}

std::vector<std::uint8_t> ascii(const std::string& s) {
    std::vector<std::uint8_t> out(s.begin(), s.end());
    out.push_back(0);
    return out;
}

std::vector<std::uint8_t> shorts(std::initializer_list<std::uint16_t> x) {
    std::vector<std::uint8_t> out;
    out.reserve(x.size() * 2u);
    for (const auto v : x) append16(out, v);
    return out;
}

std::vector<std::uint8_t> longs(const std::vector<std::uint32_t>& x) {
    std::vector<std::uint8_t> out;
    out.reserve(x.size() * 4u);
    for (const auto v : x) append32(out, v);
    return out;
}

std::uint32_t add_blob(std::vector<std::uint8_t>& extra,
                       std::uint32_t base,
                       const std::vector<std::uint8_t>& data,
                       std::uint32_t align) {
    const auto absolute = base + static_cast<std::uint32_t>(extra.size());
    const auto aligned = align == 4u ? align4(absolute) : ((absolute + 1u) & ~1u);
    extra.insert(extra.end(), aligned - absolute, 0u);
    const auto offset = base + static_cast<std::uint32_t>(extra.size());
    extra.insert(extra.end(), data.begin(), data.end());
    return offset;
}

double det(const Mat3& m) noexcept {
    const auto& a = m.v;
    return a[0] * (a[4] * a[8] - a[5] * a[7]) -
           a[1] * (a[3] * a[8] - a[5] * a[6]) +
           a[2] * (a[3] * a[7] - a[4] * a[6]);
}

bool inv(const Mat3& m, Mat3& r) noexcept {
    const double d = det(m);
    if (!std::isfinite(d) || std::abs(d) <= DET_EPS) return false;
    const auto& a = m.v;
    r.v = {
         (a[4]*a[8]-a[5]*a[7])/d, -(a[1]*a[8]-a[2]*a[7])/d,  (a[1]*a[5]-a[2]*a[4])/d,
        -(a[3]*a[8]-a[5]*a[6])/d,  (a[0]*a[8]-a[2]*a[6])/d, -(a[0]*a[5]-a[2]*a[3])/d,
         (a[3]*a[7]-a[4]*a[6])/d, -(a[0]*a[7]-a[1]*a[6])/d,  (a[0]*a[4]-a[1]*a[3])/d
    };
    for (const auto v : r.v) if (!std::isfinite(v) || std::abs(v) > 1000.0) return false;
    return true;
}

Vec3 mul(const Mat3& m, const Vec3& x) noexcept {
    Vec3 y;
    for (int r = 0; r < 3; ++r) {
        y.v[r] = m.v[r*3] * x.v[0] + m.v[r*3+1] * x.v[1] + m.v[r*3+2] * x.v[2];
    }
    return y;
}

bool srational_matrix(const Mat3& m, std::vector<std::uint8_t>& out) {
    out.clear();
    out.reserve(72u);
    for (const auto v : m.v) {
        const double n = std::round(v * static_cast<double>(RAT_DEN));
        if (!std::isfinite(n) ||
            n < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
            n > static_cast<double>(std::numeric_limits<std::int32_t>::max())) return false;
        append_i32(out, static_cast<std::int32_t>(n));
        append_i32(out, static_cast<std::int32_t>(RAT_DEN));
    }
    return true;
}

bool rational_vec(const Vec3& x, std::vector<std::uint8_t>& out) {
    out.clear();
    out.reserve(24u);
    for (const auto v : x.v) {
        const double n = std::round(v * static_cast<double>(RAT_DEN));
        if (!std::isfinite(n) || !(n > 0.0) ||
            n > static_cast<double>(std::numeric_limits<std::uint32_t>::max())) return false;
        append32(out, static_cast<std::uint32_t>(n));
        append32(out, static_cast<std::uint32_t>(RAT_DEN));
    }
    return true;
}

std::uint32_t inline4(std::array<std::uint8_t, 4> x) noexcept {
    return std::uint32_t(x[0]) | (std::uint32_t(x[1]) << 8u) |
           (std::uint32_t(x[2]) << 16u) | (std::uint32_t(x[3]) << 24u);
}

void write_entry(std::vector<std::uint8_t>& b, std::size_t o, const Entry& e) {
    put16(b, o, e.tag);
    put16(b, o + 2u, e.type);
    put32(b, o + 4u, e.count);
    put32(b, o + 8u, e.value);
}

}  // namespace

Status admit_linear_dng_projection(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const finalized_scientific_preview_release::v0_2::ReleaseResult& release,
    ProjectionAdmission& out) noexcept {
    using A = scientific_preview_binding_v0_1::ColorBindingAuthority;
    using P = finalized_scientific_preview_release::v0_2::PreviewAuthority;

    if (release.authority == P::None || !prepared.mainHouseComputeAllowed || !prepared.color.validated) {
        return Status::error(StatusCode::FinalizedLineageRequired,
                             "Linear DNG requires an already-finalized scientific lineage");
    }
    if (prepared.physicalFrameCount != 1u || prepared.independentEvidenceCount != 1u ||
        release.scientificIdentity.physicalFrameCount != 1u ||
        release.scientificIdentity.independentEvidenceCount != 1u ||
        release.canonicalPhase2.backplane.physicalFrameCount != 1u ||
        release.canonicalPhase2.backplane.independentEvidenceCount != 1u) {
        return Status::error(StatusCode::FinalizedLineageRequired,
                             "Linear DNG v0.1 preserves one physical frame/evidence root");
    }
    if (prepared.color.authority == A::Unverified || prepared.color.authority == A::PreviewSentinel) {
        return Status::error(StatusCode::UnauthorizedColorBinding,
                             "unverified/sentinel color is not export-authoritative");
    }

    const auto& seal = release.canonicalPhase2.admission.sourceSeal;
    if (prepared.source.sha256 != seal.sha256 || prepared.source.byteLength != seal.byteLength ||
        prepared.source.sourceEvidenceId != seal.sourceEvidenceId ||
        prepared.source.sha256 != release.canonicalPhase2.backplane.sourceEvidenceHash) {
        return Status::error(StatusCode::SourceIdentityMismatch,
                             "projection source identity does not match finalized Backplane");
    }
    if (zero_hash(release.scientificIdentity.scientificMasterHash) ||
        release.scientificIdentity.scientificMasterHash !=
            release.canonicalPhase2.backplane.scientificMasterHash) {
        return Status::error(StatusCode::ScientificIdentityMismatch,
                             "projection Scientific Master identity is missing/mismatched");
    }
    for (const auto v : prepared.color.cameraToXyzD50) {
        if (!std::isfinite(v)) {
            return Status::error(StatusCode::InvalidArgument, "non-finite cameraToXyzD50");
        }
    }

    ProjectionAdmission admitted;
    admitted.source = prepared.source;
    admitted.scientificMasterHash = release.scientificIdentity.scientificMasterHash;
    admitted.cameraToXyzD50 = prepared.color.cameraToXyzD50;
    admitted.colorAuthority = prepared.color.authority;
    admitted.strongerPhysicalColorClaim =
        release.authority == P::FinalizedIndependentlyCalibratedScientificPreview;
    admitted.physicalFrameCount = 1;
    admitted.independentEvidenceCount = 1;
    out = admitted;
    return Status::ok();
}

Status write_linear_dng(ICameraRgbTileSource& source,
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
        admission.independentEvidenceCount != 1u || zero_hash(admission.source.sha256) ||
        zero_hash(admission.scientificMasterHash)) {
        return Status::error(StatusCode::InvalidArgument, "invalid Linear DNG projection state");
    }

    Mat3 cameraToXyz;
    for (std::size_t i = 0; i < 9u; ++i) {
        cameraToXyz.v[i] = admission.cameraToXyzD50[i];
        if (!std::isfinite(cameraToXyz.v[i])) {
            return Status::error(StatusCode::InvalidArgument, "non-finite camera matrix");
        }
    }
    Mat3 xyzToCamera;
    if (!inv(cameraToXyz, xyzToCamera)) {
        return Status::error(StatusCode::SingularColorMatrix,
                             "cameraToXyzD50 cannot form DNG ColorMatrix1");
    }

    Vec3 neutral = mul(xyzToCamera, Vec3{{D50_X, D50_Y, D50_Z}});
    if (!std::isfinite(neutral.v[1]) || !(neutral.v[1] > 0.0)) {
        return Status::error(StatusCode::InvalidNeutral, "invalid effective D50 camera neutral");
    }
    const double g = neutral.v[1];
    for (auto& v : neutral.v) v /= g;
    for (const auto v : neutral.v) {
        if (!std::isfinite(v) || !(v > 0.0) || v > 64.0) {
            return Status::error(StatusCode::InvalidNeutral,
                                 "effective AsShotNeutral outside projection bounds");
        }
    }

    const std::uint64_t edge = static_cast<std::uint64_t>(options.tileEdge);
    const std::uint64_t tilesX = (static_cast<std::uint64_t>(width) + edge - 1u) / edge;
    const std::uint64_t tilesY = (static_cast<std::uint64_t>(height) + edge - 1u) / edge;
    const std::uint64_t tileCount64 = tilesX * tilesY;
    const std::uint64_t samplesPerTile = edge * edge * 3u;
    const std::uint64_t tileBytes64 = samplesPerTile * 2u;
    if (tileCount64 == 0u || tileCount64 > std::numeric_limits<std::uint32_t>::max() ||
        tileBytes64 > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::OutputTooLarge, "tile geometry exceeds classic TIFF");
    }
    const auto tileCount = static_cast<std::uint32_t>(tileCount64);
    const auto tileBytes = static_cast<std::uint32_t>(tileBytes64);

    const std::uint32_t ifdOffset = 8u;
    const std::uint32_t ifdBytes = 2u + static_cast<std::uint32_t>(IFD_COUNT) * 12u + 4u;
    const std::uint32_t extraStart = align4(ifdOffset + ifdBytes);
    std::vector<std::uint8_t> extra;

    const auto bitsData = shorts({BITS, BITS, BITS});
    const auto bitsOff = add_blob(extra, extraStart, bitsData, 2u);

    const std::string description =
        std::string("TruthRaw role=LINEAR_DNG_COMPATIBILITY_PROJECTION; creates_evidence=0; source=") +
        admission.source.sourceEvidenceId + "; scientific_master_sha256=" +
        hash_hex(admission.scientificMasterHash) + "; color_authority=" +
        authority_text(admission.colorAuthority) + "; stronger_physical_color_claim=" +
        (admission.strongerPhysicalColorClaim ? "1" : "0");
    const auto descData = ascii(description);
    const auto descOff = add_blob(extra, extraStart, descData, 2u);
    const auto softwareData = ascii(options.software);
    const auto softwareOff = add_blob(extra, extraStart, softwareData, 2u);

    std::vector<std::uint32_t> offsets(tileCount, 0u);
    const auto offsetsData = longs(offsets);
    const auto offsetsOff = tileCount == 1u ? 0u : add_blob(extra, extraStart, offsetsData, 4u);
    const auto offsetsRelative = tileCount == 1u ? 0u : offsetsOff - extraStart;

    std::vector<std::uint32_t> counts(tileCount, tileBytes);
    const auto countsData = longs(counts);
    const auto countsOff = tileCount == 1u ? 0u : add_blob(extra, extraStart, countsData, 4u);

    const auto sampleFormatData = shorts({1u, 1u, 1u});
    const auto sampleFormatOff = add_blob(extra, extraStart, sampleFormatData, 2u);
    const auto modelData = ascii(options.uniqueCameraModel);
    const auto modelOff = add_blob(extra, extraStart, modelData, 2u);
    const auto whiteData = shorts({WHITE, WHITE, WHITE});
    const auto whiteOff = add_blob(extra, extraStart, whiteData, 2u);

    std::vector<std::uint8_t> matrixData;
    if (!srational_matrix(xyzToCamera, matrixData)) {
        return Status::error(StatusCode::SingularColorMatrix,
                             "ColorMatrix1 cannot be represented as SRATIONAL");
    }
    const auto matrixOff = add_blob(extra, extraStart, matrixData, 4u);
    std::vector<std::uint8_t> neutralData;
    if (!rational_vec(neutral, neutralData)) {
        return Status::error(StatusCode::InvalidNeutral,
                             "AsShotNeutral cannot be represented as RATIONAL");
    }
    const auto neutralOff = add_blob(extra, extraStart, neutralData, 4u);

    const std::uint64_t pixelStart64 =
        align4(extraStart + static_cast<std::uint32_t>(extra.size()));
    const std::uint64_t outputSize64 = pixelStart64 + tileCount64 * tileBytes64;
    if (pixelStart64 > std::numeric_limits<std::uint32_t>::max() ||
        outputSize64 > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::OutputTooLarge, "DNG exceeds classic TIFF 32-bit offsets");
    }
    const auto pixelStart = static_cast<std::uint32_t>(pixelStart64);
    extra.insert(extra.end(),
                 pixelStart - (extraStart + static_cast<std::uint32_t>(extra.size())), 0u);
    if (tileCount > 1u) {
        for (std::uint32_t i = 0; i < tileCount; ++i) {
            put32(extra, offsetsRelative + static_cast<std::size_t>(i) * 4u,
                  pixelStart + i * tileBytes);
        }
    }

    std::vector<Entry> e;
    e.reserve(IFD_COUNT);
    e.push_back({TAG_NewSubFileType, LONG, 1u, 0u});
    e.push_back({TAG_ImageWidth, LONG, 1u, static_cast<std::uint32_t>(width)});
    e.push_back({TAG_ImageLength, LONG, 1u, static_cast<std::uint32_t>(height)});
    e.push_back({TAG_BitsPerSample, SHORT, 3u, bitsOff});
    e.push_back({TAG_Compression, SHORT, 1u, 1u});
    e.push_back({TAG_PhotometricInterpretation, SHORT, 1u, PHOTOMETRIC_LinearRaw});
    e.push_back({TAG_ImageDescription, ASCII, static_cast<std::uint32_t>(descData.size()), descOff});
    e.push_back({TAG_Orientation, SHORT, 1u, static_cast<std::uint32_t>(source.orientation())});
    e.push_back({TAG_SamplesPerPixel, SHORT, 1u, 3u});
    e.push_back({TAG_PlanarConfiguration, SHORT, 1u, 1u});
    e.push_back({TAG_Software, ASCII, static_cast<std::uint32_t>(softwareData.size()), softwareOff});
    e.push_back({TAG_TileWidth, LONG, 1u, static_cast<std::uint32_t>(options.tileEdge)});
    e.push_back({TAG_TileLength, LONG, 1u, static_cast<std::uint32_t>(options.tileEdge)});
    e.push_back({TAG_TileOffsets, LONG, tileCount, tileCount == 1u ? pixelStart : offsetsOff});
    e.push_back({TAG_TileByteCounts, LONG, tileCount, tileCount == 1u ? tileBytes : countsOff});
    e.push_back({TAG_SampleFormat, SHORT, 3u, sampleFormatOff});
    e.push_back({TAG_DNGVersion, BYTE, 4u, inline4({1u, 4u, 0u, 0u})});
    e.push_back({TAG_DNGBackwardVersion, BYTE, 4u, inline4({1u, 4u, 0u, 0u})});
    e.push_back({TAG_UniqueCameraModel, ASCII, static_cast<std::uint32_t>(modelData.size()), modelOff});
    e.push_back({TAG_WhiteLevel, SHORT, 3u, whiteOff});
    e.push_back({TAG_ColorMatrix1, SRATIONAL, 9u, matrixOff});
    e.push_back({TAG_AsShotNeutral, RATIONAL, 3u, neutralOff});
    e.push_back({TAG_CalibrationIlluminant1, SHORT, 1u, LIGHTSOURCE_D50});
    if (e.size() != IFD_COUNT) {
        return Status::error(StatusCode::InvalidArgument, "internal IFD tag-count mismatch");
    }
    std::sort(e.begin(), e.end(), [](const Entry& a, const Entry& b) { return a.tag < b.tag; });

    std::vector<std::uint8_t> prefix(pixelStart, 0u);
    prefix[0] = 'I'; prefix[1] = 'I';
    put16(prefix, 2u, 42u);
    put32(prefix, 4u, ifdOffset);
    put16(prefix, ifdOffset, static_cast<std::uint16_t>(e.size()));
    std::size_t p = ifdOffset + 2u;
    for (const auto& x : e) { write_entry(prefix, p, x); p += 12u; }
    put32(prefix, p, 0u);
    std::copy(extra.begin(), extra.end(), prefix.begin() + extraStart);

    auto status = sink.append(prefix.data(), prefix.size());
    if (!status) return Status::error(StatusCode::SinkFailed, status.message);
    audit.logicalWorkspacePeakBytes = prefix.size();
    std::vector<std::uint8_t>().swap(prefix);
    std::vector<std::uint8_t>().swap(extra);

    std::vector<std::uint8_t> encoded(static_cast<std::size_t>(tileBytes), 0u);
    std::vector<float> rgb;
    rgb.reserve(static_cast<std::size_t>(samplesPerTile));
    audit.logicalWorkspacePeakBytes = std::max(
        audit.logicalWorkspacePeakBytes,
        encoded.capacity() + static_cast<std::size_t>(samplesPerTile) * sizeof(float));

    for (std::uint32_t ty = 0; ty < tilesY; ++ty) {
        for (std::uint32_t tx = 0; tx < tilesX; ++tx) {
            const int x0 = static_cast<int>(tx) * options.tileEdge;
            const int y0 = static_cast<int>(ty) * options.tileEdge;
            const int x1 = std::min(width, x0 + options.tileEdge);
            const int y1 = std::min(height, y0 + options.tileEdge);
            const int validW = x1 - x0;
            const int validH = y1 - y0;
            rgb.resize(static_cast<std::size_t>(validW) * static_cast<std::size_t>(validH) * 3u);
            status = source.readCameraRgbTile(x0, y0, x1, y1, rgb.data(), rgb.size());
            if (!status) return Status::error(StatusCode::SourceFailed, status.message);
            std::fill(encoded.begin(), encoded.end(), 0u);

            for (int y = 0; y < validH; ++y) {
                for (int x = 0; x < validW; ++x) {
                    for (int c = 0; c < 3; ++c) {
                        const auto si = (static_cast<std::size_t>(y) * validW + x) * 3u + c;
                        double v = rgb[si];
                        ++audit.cameraRgbSamplesRead;
                        if (!std::isfinite(v)) {
                            return Status::error(StatusCode::NonFiniteSample,
                                                 "non-finite reconstructed camera RGB sample");
                        }
                        v *= options.linearScale;
                        if (v < 0.0) {
                            if (!options.clampToLinearReferenceRange) {
                                return Status::error(StatusCode::InvalidArgument,
                                                     "negative sample requires explicit bounded projection");
                            }
                            v = 0.0;
                            ++audit.negativeSamplesClamped;
                        } else if (v > 1.0) {
                            if (!options.clampToLinearReferenceRange) {
                                return Status::error(StatusCode::InvalidArgument,
                                                     "overrange sample requires explicit bounded projection");
                            }
                            v = 1.0;
                            ++audit.overrangeSamplesClamped;
                        }
                        const auto q = static_cast<std::uint16_t>(std::llround(v * WHITE));
                        ++audit.quantizedSamples;
                        const auto di = (static_cast<std::size_t>(y) * options.tileEdge + x) * 3u + c;
                        encoded[di * 2u] = static_cast<std::uint8_t>(q);
                        encoded[di * 2u + 1u] = static_cast<std::uint8_t>(q >> 8u);
                    }
                }
            }
            status = sink.append(encoded.data(), encoded.size());
            if (!status) return Status::error(StatusCode::SinkFailed, status.message);
            ++audit.tilesWritten;
        }
    }

    audit.outputBytes = sink.bytesWritten();
    if (audit.outputBytes != outputSize64) {
        return Status::error(StatusCode::SinkFailed, "DNG byte count differs from planned TIFF layout");
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
