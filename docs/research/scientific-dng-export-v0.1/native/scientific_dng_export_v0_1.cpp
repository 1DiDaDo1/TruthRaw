#include "scientific_dng_export_v0_1.h"

#include "full_frame_streaming_v0_1_internal.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace truthraw::scientific_dng_export::v0_1 {
namespace {

using scientific_master_digest::v0_1::ScientificMasterDigestAccumulator;
using scientific_master_digest::v0_1::TileView;
using scientific_master_digest::v0_1::kCanonicalCellEdge;
using streaming_v0_1::TileRect;
using streaming_v0_1::detail::Workspace;
using streaming_v0_1::detail::fill_stage2;
using streaming_v0_1::detail::vector_bytes;

constexpr std::uint16_t kTypeByte = 1;
constexpr std::uint16_t kTypeAscii = 2;
constexpr std::uint16_t kTypeShort = 3;
constexpr std::uint16_t kTypeLong = 4;
constexpr std::uint16_t kTypeRational = 5;
constexpr std::uint16_t kTypeSRational = 10;
constexpr std::uint32_t kPhotometricCfa = 32803;
constexpr std::uint32_t kPhotometricLinearRaw = 34892;
constexpr std::uint32_t kTileEdge = kCanonicalCellEdge;
constexpr std::uint32_t kMax16 = 65535u;
constexpr double kD50X = 0.96422;
constexpr double kD50Y = 1.0;
constexpr double kD50Z = 0.82521;
constexpr double kMatrixEpsilon = 1.0e-12;
constexpr std::int32_t kRationalDenominator = 1000000;

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

void put16(std::vector<std::uint8_t>& out, std::size_t at, std::uint16_t v) {
    out[at] = static_cast<std::uint8_t>(v & 0xffu);
    out[at + 1u] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
}

void put32(std::vector<std::uint8_t>& out, std::size_t at, std::uint32_t v) {
    out[at] = static_cast<std::uint8_t>(v & 0xffu);
    out[at + 1u] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
    out[at + 2u] = static_cast<std::uint8_t>((v >> 16u) & 0xffu);
    out[at + 3u] = static_cast<std::uint8_t>((v >> 24u) & 0xffu);
}

void append16(std::vector<std::uint8_t>& out, std::uint16_t v) {
    const auto at = out.size();
    out.resize(at + 2u);
    put16(out, at, v);
}

void append32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    const auto at = out.size();
    out.resize(at + 4u);
    put32(out, at, v);
}

std::uint32_t inline_bytes(std::uint8_t a, std::uint8_t b,
                           std::uint8_t c, std::uint8_t d) noexcept {
    return static_cast<std::uint32_t>(a) |
           (static_cast<std::uint32_t>(b) << 8u) |
           (static_cast<std::uint32_t>(c) << 16u) |
           (static_cast<std::uint32_t>(d) << 24u);
}

std::uint32_t inline_two_shorts(std::uint16_t a, std::uint16_t b) noexcept {
    return static_cast<std::uint32_t>(a) |
           (static_cast<std::uint32_t>(b) << 16u);
}

std::size_t align4(std::size_t v) noexcept {
    return (v + 3u) & ~std::size_t(3u);
}

bool finite_matrix(const std::array<float, 9>& m) noexcept {
    for (float v : m) if (!std::isfinite(v)) return false;
    return true;
}

bool inverse(const Mat3& a, Mat3& out) noexcept {
    const auto& m = a.v;
    const double d =
        m[0] * (m[4] * m[8] - m[5] * m[7]) -
        m[1] * (m[3] * m[8] - m[5] * m[6]) +
        m[2] * (m[3] * m[7] - m[4] * m[6]);
    if (!std::isfinite(d) || std::abs(d) <= kMatrixEpsilon) return false;
    out.v = {
        (m[4]*m[8]-m[5]*m[7])/d, (m[2]*m[7]-m[1]*m[8])/d, (m[1]*m[5]-m[2]*m[4])/d,
        (m[5]*m[6]-m[3]*m[8])/d, (m[0]*m[8]-m[2]*m[6])/d, (m[2]*m[3]-m[0]*m[5])/d,
        (m[3]*m[7]-m[4]*m[6])/d, (m[1]*m[6]-m[0]*m[7])/d, (m[0]*m[4]-m[1]*m[3])/d};
    for (double v : out.v) if (!std::isfinite(v)) return false;
    return true;
}

Vec3 mul(const Mat3& m, const Vec3& x) noexcept {
    Vec3 out{};
    for (int r = 0; r < 3; ++r) {
        out.v[static_cast<std::size_t>(r)] =
            m.v[static_cast<std::size_t>(3*r)] * x.v[0] +
            m.v[static_cast<std::size_t>(3*r+1)] * x.v[1] +
            m.v[static_cast<std::size_t>(3*r+2)] * x.v[2];
    }
    return out;
}

bool derive_embedded_profile(const std::array<float, 9>& cameraToXyzD50,
                             Mat3& colorMatrix,
                             Mat3& forwardMatrix,
                             Vec3& asShotNeutral) noexcept {
    if (!finite_matrix(cameraToXyzD50)) return false;
    Mat3 cameraToXyz{};
    for (std::size_t i = 0; i < 9; ++i) {
        cameraToXyz.v[i] = static_cast<double>(cameraToXyzD50[i]);
    }
    if (!inverse(cameraToXyz, colorMatrix)) return false;
    const Vec3 d50{{kD50X, kD50Y, kD50Z}};
    asShotNeutral = mul(colorMatrix, d50);
    for (double v : asShotNeutral.v) {
        if (!std::isfinite(v) || !(v > kMatrixEpsilon)) return false;
    }
    forwardMatrix = cameraToXyz;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            forwardMatrix.v[static_cast<std::size_t>(3*r+c)] *=
                asShotNeutral.v[static_cast<std::size_t>(c)];
        }
    }
    const Vec3 mapped = mul(forwardMatrix, Vec3{{1.0, 1.0, 1.0}});
    return std::abs(mapped.v[0] - kD50X) < 2.0e-6 &&
           std::abs(mapped.v[1] - kD50Y) < 2.0e-6 &&
           std::abs(mapped.v[2] - kD50Z) < 2.0e-6;
}

bool to_srational(double value, std::int32_t& num, std::int32_t& den) noexcept {
    if (!std::isfinite(value)) return false;
    const double scaled = std::round(value * static_cast<double>(kRationalDenominator));
    if (scaled < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
        scaled > static_cast<double>(std::numeric_limits<std::int32_t>::max())) return false;
    num = static_cast<std::int32_t>(scaled);
    den = kRationalDenominator;
    return true;
}

bool to_rational(double value, std::uint32_t& num, std::uint32_t& den) noexcept {
    if (!std::isfinite(value) || !(value > 0.0)) return false;
    const double scaled = std::round(value * static_cast<double>(kRationalDenominator));
    if (scaled < 1.0 || scaled > static_cast<double>(std::numeric_limits<std::uint32_t>::max())) return false;
    num = static_cast<std::uint32_t>(scaled);
    den = static_cast<std::uint32_t>(kRationalDenominator);
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

int cfa_channel(CfaPattern cfa, int x, int y) noexcept {
    const auto p = cfa_pattern(cfa);
    return static_cast<int>(p[static_cast<std::size_t>(((y & 1) << 1) | (x & 1))]);
}

std::uint16_t quantize(float value) noexcept {
    if (!std::isfinite(value)) return 0u;
    const double x = std::clamp(static_cast<double>(value), 0.0, 1.0);
    return static_cast<std::uint16_t>(std::floor(x * 65535.0 + 0.5));
}

bool all_zero(const scientific_master_digest::v0_1::Sha256& h) noexcept {
    for (std::uint8_t v : h) if (v != 0u) return false;
    return true;
}

template <typename Fn>
Status replay_master(streaming_v0_1::IRawTileSource& source,
                     IReconstructionBackend& reconstruction,
                     Fn&& fn,
                     scientific_master_digest::v0_1::Sha256& digestOut,
                     std::size_t& workspacePeak) noexcept {
    const auto& m = source.metadata();
    const int halo = reconstruction.requiredHalo();
    if (halo < 0) {
        return Status::error(StatusCode::InvalidArgument,
                             "reconstruction backend returned negative halo");
    }
    ScientificMasterDigestAccumulator digest(
        static_cast<std::uint32_t>(m.width), static_cast<std::uint32_t>(m.height));
    if (!digest.valid()) {
        return Status::error(StatusCode::DigestFailed,
                             "Scientific Master digest initialization failed");
    }
    Workspace w{};
    for (int y0 = 0; y0 < m.height; y0 += static_cast<int>(kTileEdge)) {
        const int y1 = std::min(m.height, y0 + static_cast<int>(kTileEdge));
        for (int x0 = 0; x0 < m.width; x0 += static_cast<int>(kTileEdge)) {
            const int x1 = std::min(m.width, x0 + static_cast<int>(kTileEdge));
            TileRect t{};
            t.x0 = x0; t.y0 = y0; t.x1 = x1; t.y1 = y1;
            t.hx0 = std::max(0, x0 - halo);
            t.hy0 = std::max(0, y0 - halo);
            t.hx1 = std::min(m.width, x1 + halo);
            t.hy1 = std::min(m.height, y1 + halo);
            const auto filled = fill_stage2(source, t, w);
            if (!filled) {
                return Status::error(StatusCode::SourceFailed,
                                     "Stage-2 source read failed: " + filled.message);
            }
            const int tw = t.hx1 - t.hx0;
            const int th = t.hy1 - t.hy0;
            const int cw = t.x1 - t.x0;
            const int ch = t.y1 - t.y0;
            const std::size_t samples = static_cast<std::size_t>(cw) * static_cast<std::size_t>(ch);
            w.cam.resize(3u * samples);
            const auto reconstructed = reconstruction.reconstructTile(
                w.stage2.data(), tw, th, t.hx0, t.hy0,
                t.x0, t.y0, cw, ch, m.cfa, w.cam.data());
            if (!reconstructed) {
                return Status::error(StatusCode::ReconstructionFailed,
                                     "camera-native reconstruction failed: " + reconstructed.message);
            }
            TileView view{};
            view.x = static_cast<std::uint32_t>(t.x0);
            view.y = static_cast<std::uint32_t>(t.y0);
            view.width = static_cast<std::uint32_t>(cw);
            view.height = static_cast<std::uint32_t>(ch);
            view.rgb = w.cam.data();
            view.rowStrideSamples = static_cast<std::size_t>(cw) * 3u;
            if (!digest.add_tile(view)) {
                return Status::error(StatusCode::DigestFailed,
                                     "Scientific Master digest tile rejected: " + digest.error());
            }
            const auto callback = fn(t, w.cam.data(), cw, ch);
            if (!callback) return callback;
            workspacePeak = std::max(workspacePeak, vector_bytes(w));
        }
    }
    if (!digest.finalize(digestOut)) {
        return Status::error(StatusCode::DigestFailed,
                             "Scientific Master digest finalization failed: " + digest.error());
    }
    return Status::ok();
}

struct PrefixBuild final {
    std::vector<std::uint8_t> bytes;
    std::uint32_t tileByteCount = 0;
    std::uint32_t tileCount = 0;
};

Status build_prefix(const DngMetadata& m,
                    ProjectionRole role,
                    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
                    PrefixBuild& out) {
    const std::uint32_t channels = role == ProjectionRole::LinearRawCompatibility ? 3u : 1u;
    const std::uint32_t cols = (static_cast<std::uint32_t>(m.width) + kTileEdge - 1u) / kTileEdge;
    const std::uint32_t rows = (static_cast<std::uint32_t>(m.height) + kTileEdge - 1u) / kTileEdge;
    if (cols == 0u || rows == 0u || cols > std::numeric_limits<std::uint32_t>::max() / rows) {
        return Status::error(StatusCode::InvalidArgument, "invalid DNG tile geometry");
    }
    const std::uint32_t tileCount = cols * rows;
    const std::uint64_t tileBytes64 =
        static_cast<std::uint64_t>(kTileEdge) * kTileEdge * channels * sizeof(std::uint16_t);
    if (tileBytes64 > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::InvalidArgument, "DNG tile byte count overflow");
    }
    const std::uint32_t tileByteCount = static_cast<std::uint32_t>(tileBytes64);

    Mat3 cm{};
    Mat3 fm{};
    Vec3 neutral{};
    if (!derive_embedded_profile(prepared.color.cameraToXyzD50, cm, fm, neutral)) {
        return Status::error(StatusCode::InvalidColorTransform,
                             "cannot derive source-bound D50 DNG profile from finalized color transform");
    }

    const std::string software = "TruthRaw scientific-dng-export-v0.1";
    const std::string cameraModel = "TruthRaw Reconstructed Camera-Native Projection";
    const std::string description =
        role == ProjectionRole::LinearRawCompatibility
            ? "TruthRaw LINEAR_RAW_COMPATIBILITY_PROJECTION; derived from finalized Scientific Master; not measured sensor evidence"
            : "TruthRaw RECONSTRUCTED_CFA_PROJECTION; remosaiced from finalized Scientific Master; not measured sensor evidence";

    const bool linear = role == ProjectionRole::LinearRawCompatibility;
    const std::size_t entryCount = linear ? 21u : 23u;
    const std::size_t ifdOffset = 8u;
    const std::size_t ifdBytes = 2u + entryCount * 12u + 4u;
    const std::size_t extraBase = align4(ifdOffset + ifdBytes);
    std::vector<std::uint8_t> extra;

    auto add_extra = [&](const std::vector<std::uint8_t>& data) -> std::uint32_t {
        while ((extra.size() & 3u) != 0u) extra.push_back(0u);
        const std::size_t absolute = extraBase + extra.size();
        if (absolute > std::numeric_limits<std::uint32_t>::max()) return 0u;
        const auto offset = static_cast<std::uint32_t>(absolute);
        extra.insert(extra.end(), data.begin(), data.end());
        return offset;
    };
    auto shorts = [](std::initializer_list<std::uint16_t> values) {
        std::vector<std::uint8_t> b;
        for (auto v : values) append16(b, v);
        return b;
    };
    auto longs = [](std::size_t count, std::uint32_t value) {
        std::vector<std::uint8_t> b;
        b.reserve(count * 4u);
        for (std::size_t i = 0; i < count; ++i) append32(b, value);
        return b;
    };
    auto ascii = [](const std::string& s) {
        std::vector<std::uint8_t> b(s.begin(), s.end());
        b.push_back(0u);
        return b;
    };
    auto srationals = [](const Mat3& matrix, bool& ok) {
        std::vector<std::uint8_t> b;
        ok = true;
        for (double v : matrix.v) {
            std::int32_t n = 0, d = 0;
            if (!to_srational(v, n, d)) { ok = false; return b; }
            append32(b, static_cast<std::uint32_t>(n));
            append32(b, static_cast<std::uint32_t>(d));
        }
        return b;
    };
    auto rationals = [](const Vec3& v, bool& ok) {
        std::vector<std::uint8_t> b;
        ok = true;
        for (double x : v.v) {
            std::uint32_t n = 0, d = 0;
            if (!to_rational(x, n, d)) { ok = false; return b; }
            append32(b, n);
            append32(b, d);
        }
        return b;
    };

    const std::uint32_t bitsOffset = linear ? add_extra(shorts({16u,16u,16u})) : 0u;
    const std::uint32_t sampleFormatOffset = linear ? add_extra(shorts({1u,1u,1u})) : 0u;
    const std::uint32_t softwareOffset = add_extra(ascii(software));
    const std::uint32_t descriptionOffset = add_extra(ascii(description));
    const std::uint32_t cameraOffset = add_extra(ascii(cameraModel));
    const std::uint32_t whiteOffset = linear ? add_extra(longs(3u, kMax16)) : 0u;
    bool ok = false;
    const std::uint32_t cmOffset = add_extra(srationals(cm, ok));
    if (!ok) return Status::error(StatusCode::InvalidColorTransform, "ColorMatrix1 rational encoding failed");
    const std::uint32_t neutralOffset = add_extra(rationals(neutral, ok));
    if (!ok) return Status::error(StatusCode::InvalidColorTransform, "AsShotNeutral rational encoding failed");
    const std::uint32_t fmOffset = add_extra(srationals(fm, ok));
    if (!ok) return Status::error(StatusCode::InvalidColorTransform, "ForwardMatrix1 rational encoding failed");

    std::vector<std::uint8_t> offsets(tileCount * 4u, 0u);
    const std::uint32_t offsetsOffset = add_extra(offsets);
    const std::uint32_t byteCountsOffset = add_extra(longs(tileCount, tileByteCount));
    const std::size_t pixelOffsetSize = align4(extraBase + extra.size());
    if (pixelOffsetSize > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::InvalidArgument, "DNG pixel offset exceeds classic TIFF range");
    }
    const std::uint32_t pixelOffset = static_cast<std::uint32_t>(pixelOffsetSize);
    const std::uint64_t finalSize = static_cast<std::uint64_t>(pixelOffset) +
        static_cast<std::uint64_t>(tileCount) * tileByteCount;
    if (finalSize > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::InvalidArgument, "classic TIFF DNG would exceed 4 GiB");
    }
    const std::size_t offsetsRel = static_cast<std::size_t>(offsetsOffset) - extraBase;
    for (std::uint32_t i = 0; i < tileCount; ++i) {
        const std::uint32_t off = pixelOffset + i * tileByteCount;
        put32(extra, offsetsRel + static_cast<std::size_t>(i) * 4u, off);
    }

    std::vector<IfdEntry> e;
    auto add = [&](std::uint16_t tag, std::uint16_t type, std::uint32_t count, std::uint32_t value) {
        e.push_back({tag,type,count,value});
    };
    add(254, kTypeLong, 1, 0); // NewSubFileType
    add(256, kTypeLong, 1, static_cast<std::uint32_t>(m.width));
    add(257, kTypeLong, 1, static_cast<std::uint32_t>(m.height));
    add(258, kTypeShort, channels, linear ? bitsOffset : 16u);
    add(259, kTypeShort, 1, 1u); // uncompressed
    add(262, kTypeShort, 1, linear ? kPhotometricLinearRaw : kPhotometricCfa);
    add(270, kTypeAscii, static_cast<std::uint32_t>(description.size() + 1u), descriptionOffset);
    add(274, kTypeShort, 1, static_cast<std::uint32_t>(m.orientation));
    add(277, kTypeShort, 1, channels);
    if (linear) add(284, kTypeShort, 1, 1u); // chunky RGB planes
    add(305, kTypeAscii, static_cast<std::uint32_t>(software.size() + 1u), softwareOffset);
    add(322, kTypeLong, 1, kTileEdge);
    add(323, kTypeLong, 1, kTileEdge);
    add(324, kTypeLong, tileCount, offsetsOffset);
    add(325, kTypeLong, tileCount, byteCountsOffset);
    add(339, kTypeShort, channels, linear ? sampleFormatOffset : 1u);
    if (!linear) {
        const auto p = cfa_pattern(m.cfa);
        add(33421, kTypeShort, 2, inline_two_shorts(2u, 2u));
        add(33422, kTypeByte, 4, inline_bytes(p[0], p[1], p[2], p[3]));
        add(50710, kTypeByte, 3, inline_bytes(0u,1u,2u,0u));
    }
    add(50706, kTypeByte, 4, inline_bytes(1u,4u,0u,0u));
    add(50707, kTypeByte, 4, inline_bytes(1u,2u,0u,0u));
    add(50708, kTypeAscii, static_cast<std::uint32_t>(cameraModel.size() + 1u), cameraOffset);
    add(50717, kTypeLong, channels, linear ? whiteOffset : kMax16);
    add(50721, kTypeSRational, 9, cmOffset);
    add(50728, kTypeRational, 3, neutralOffset);
    add(50778, kTypeShort, 1, 23u); // D50
    add(50964, kTypeSRational, 9, fmOffset);

    if (e.size() != entryCount) {
        return Status::error(StatusCode::InvalidArgument, "internal DNG IFD entry-count mismatch");
    }
    std::sort(e.begin(), e.end(), [](const IfdEntry& a, const IfdEntry& b){ return a.tag < b.tag; });

    std::vector<std::uint8_t> prefix(pixelOffset, 0u);
    prefix[0] = 'I'; prefix[1] = 'I';
    put16(prefix, 2u, 42u);
    put32(prefix, 4u, static_cast<std::uint32_t>(ifdOffset));
    put16(prefix, ifdOffset, static_cast<std::uint16_t>(entryCount));
    std::size_t cursor = ifdOffset + 2u;
    for (const auto& entry : e) {
        put16(prefix, cursor, entry.tag);
        put16(prefix, cursor + 2u, entry.type);
        put32(prefix, cursor + 4u, entry.count);
        put32(prefix, cursor + 8u, entry.value);
        cursor += 12u;
    }
    put32(prefix, cursor, 0u);
    std::copy(extra.begin(), extra.end(), prefix.begin() + static_cast<std::ptrdiff_t>(extraBase));

    out.bytes = std::move(prefix);
    out.tileByteCount = tileByteCount;
    out.tileCount = tileCount;
    return Status::ok();
}

} // namespace

Status export_scientific_dng(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const scientific_master_digest::v0_1::Sha256& expectedScientificMasterHash,
    ISequentialByteSink& sink,
    const Options& options,
    Result& out) noexcept {
    out = {};
    out.role = options.role;
    const auto& m = source.metadata();
    if (m.width <= 1 || m.height <= 1 || all_zero(expectedScientificMasterHash)) {
        return Status::error(StatusCode::InvalidArgument, "invalid DNG export input");
    }
    if (options.role != ProjectionRole::LinearRawCompatibility &&
        options.role != ProjectionRole::ReconstructedCfaCompatibility) {
        return Status::error(StatusCode::InvalidArgument, "unsupported DNG projection role");
    }
    if (!prepared.mainHouseComputeAllowed || !prepared.color.validated ||
        prepared.physicalFrameCount != 1u || prepared.independentEvidenceCount != 1u ||
        prepared.source.sourceEvidenceId.empty() || m.sourceId != prepared.source.sourceEvidenceId) {
        return Status::error(StatusCode::InvalidAuthority,
                             "DNG export requires exact prepared single-frame source/color lineage");
    }
    using Authority = scientific_preview_binding_v0_1::ColorBindingAuthority;
    if (prepared.color.authority != Authority::SourceMetadataBound &&
        prepared.color.authority != Authority::GatehouseCertifiedMetadata &&
        prepared.color.authority != Authority::IndependentCalibration) {
        return Status::error(StatusCode::InvalidAuthority,
                             "DNG export refuses unverified/preview-sentinel color authority");
    }

    PrefixBuild prefix{};
    const auto prefixStatus = build_prefix(m, options.role, prepared, prefix);
    if (!prefixStatus) return prefixStatus;

    std::size_t workspacePeak = 0u;
    scientific_master_digest::v0_1::Sha256 preflightHash{};
    const auto noWrite = [](const TileRect&, const float*, int, int) { return Status::ok(); };
    const auto preflight = replay_master(source, reconstruction, noWrite, preflightHash, workspacePeak);
    if (!preflight) return preflight;
    if (preflightHash != expectedScientificMasterHash) {
        return Status::error(StatusCode::ScientificIdentityMismatch,
                             "preflight Scientific Master digest differs from finalized identity");
    }

    const std::size_t channels = options.role == ProjectionRole::LinearRawCompatibility ? 3u : 1u;
    const std::size_t tileSamples = static_cast<std::size_t>(kTileEdge) * kTileEdge * channels;
    std::vector<std::uint8_t> tileBytes(tileSamples * sizeof(std::uint16_t), 0u);
    const std::size_t logical = source.residentBytesUpperBound() + workspacePeak +
        prefix.bytes.size() + tileBytes.size();
    if (options.memoryBudgetBytes != 0u && logical > options.memoryBudgetBytes) {
        return Status::error(StatusCode::BudgetExceeded,
                             "DNG export logical resident bound exceeds caller budget");
    }
    if (!sink.write(prefix.bytes.data(), prefix.bytes.size())) {
        return Status::error(StatusCode::OutputFailed, "failed writing DNG TIFF/IFD prefix");
    }

    scientific_master_digest::v0_1::Sha256 exportHash{};
    std::uint32_t writtenTiles = 0u;
    auto writeTile = [&](const TileRect& t, const float* cam, int cw, int ch) -> Status {
        std::fill(tileBytes.begin(), tileBytes.end(), 0u);
        for (int y = 0; y < ch; ++y) {
            for (int x = 0; x < cw; ++x) {
                const std::size_t srcPixel = static_cast<std::size_t>(y) * static_cast<std::size_t>(cw) +
                                             static_cast<std::size_t>(x);
                const std::size_t dstPixel = static_cast<std::size_t>(y) * kTileEdge +
                                             static_cast<std::size_t>(x);
                if (options.role == ProjectionRole::LinearRawCompatibility) {
                    for (std::size_t c = 0; c < 3u; ++c) {
                        const std::uint16_t q = quantize(cam[3u * srcPixel + c]);
                        const std::size_t at = (3u * dstPixel + c) * 2u;
                        tileBytes[at] = static_cast<std::uint8_t>(q & 0xffu);
                        tileBytes[at + 1u] = static_cast<std::uint8_t>((q >> 8u) & 0xffu);
                    }
                } else {
                    const int globalX = t.x0 + x;
                    const int globalY = t.y0 + y;
                    const int channel = cfa_channel(source.metadata().cfa, globalX, globalY);
                    const std::uint16_t q = quantize(cam[3u * srcPixel + static_cast<std::size_t>(channel)]);
                    const std::size_t at = dstPixel * 2u;
                    tileBytes[at] = static_cast<std::uint8_t>(q & 0xffu);
                    tileBytes[at + 1u] = static_cast<std::uint8_t>((q >> 8u) & 0xffu);
                }
            }
        }
        if (!sink.write(tileBytes.data(), tileBytes.size())) {
            return Status::error(StatusCode::OutputFailed, "failed writing DNG tile payload");
        }
        ++writtenTiles;
        return Status::ok();
    };

    const auto replay = replay_master(source, reconstruction, writeTile, exportHash, workspacePeak);
    if (!replay) return replay;
    if (exportHash != expectedScientificMasterHash) {
        return Status::error(StatusCode::ScientificIdentityMismatch,
                             "export-pass Scientific Master digest differs from finalized identity");
    }
    if (writtenTiles != prefix.tileCount) {
        return Status::error(StatusCode::OutputFailed, "DNG tile count incomplete");
    }
    if (!sink.flush()) {
        return Status::error(StatusCode::OutputFailed, "failed flushing DNG output");
    }

    out.replayedScientificMasterHash = exportHash;
    out.width = static_cast<std::uint32_t>(m.width);
    out.height = static_cast<std::uint32_t>(m.height);
    out.tileCount = writtenTiles;
    out.bytesWritten = sink.bytesWritten();
    out.logicalWorkspacePeakBytes = std::max(workspacePeak, logical);
    out.scientificMasterIdentityMatched = true;
    out.boundedCompatibilityProjection = true;
    out.fullScientificMasterMaterialized = false;
    out.sourceMetadataBoundColor = prepared.color.authority == Authority::SourceMetadataBound;
    out.independentPhysicalColor = prepared.color.authority == Authority::IndependentCalibration;
    out.physicalFrameCount = 1u;
    out.independentEvidenceCount = 1u;
    return Status::ok();
}

const char* role_name(ProjectionRole role) noexcept {
    switch (role) {
        case ProjectionRole::LinearRawCompatibility: return "LINEAR_RAW_COMPATIBILITY_PROJECTION";
        case ProjectionRole::ReconstructedCfaCompatibility: return "RECONSTRUCTED_CFA_PROJECTION";
    }
    return "UNKNOWN";
}

const char* status_name(StatusCode code) noexcept {
    switch (code) {
        case StatusCode::Ok: return "OK";
        case StatusCode::InvalidArgument: return "INVALID_ARGUMENT";
        case StatusCode::InvalidAuthority: return "INVALID_AUTHORITY";
        case StatusCode::InvalidColorTransform: return "INVALID_COLOR_TRANSFORM";
        case StatusCode::SourceFailed: return "SOURCE_FAILED";
        case StatusCode::ReconstructionFailed: return "RECONSTRUCTION_FAILED";
        case StatusCode::DigestFailed: return "DIGEST_FAILED";
        case StatusCode::ScientificIdentityMismatch: return "SCIENTIFIC_IDENTITY_MISMATCH";
        case StatusCode::OutputFailed: return "OUTPUT_FAILED";
        case StatusCode::BudgetExceeded: return "BUDGET_EXCEEDED";
    }
    return "UNKNOWN";
}

} // namespace truthraw::scientific_dng_export::v0_1
