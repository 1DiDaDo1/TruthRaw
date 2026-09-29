#include "truthnegative_n2_support_distance_v0_1.h"

#include "full_frame_streaming_v0_1_internal.h"
#include "truthnegative_authority_aware_neighborhood_v0_1.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace truthraw::truthnegative_n2_support_distance::v0_1 {
namespace {

namespace detail = truthraw::streaming_v0_1::detail;
namespace neigh = truthraw::truthnegative_authority_aware_neighborhood::v0_1;

bool nonzero(const Digest& d) noexcept {
    return std::any_of(d.begin(), d.end(), [](std::uint8_t v){ return v != 0u; });
}

int measured_channel(CfaPattern cfa, int x, int y) noexcept {
    const int phase = (y & 1) * 2 + (x & 1);
    static constexpr int bggr[4] = {2,1,1,0};
    static constexpr int rggb[4] = {0,1,1,2};
    static constexpr int grbg[4] = {1,0,2,1};
    static constexpr int gbrg[4] = {1,2,0,1};
    const int* map = bggr;
    switch(cfa) {
        case CfaPattern::RGGB: map = rggb; break;
        case CfaPattern::GRBG: map = grbg; break;
        case CfaPattern::GBRG: map = gbrg; break;
        case CfaPattern::BGGR: map = bggr; break;
    }
    return map[phase];
}

void hash_u32(truthraw::sha256_v0_69::Hasher& h, std::uint32_t v) noexcept {
    const std::array<std::uint8_t,4u> b{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v >> 8u),
        static_cast<std::uint8_t>(v >> 16u),
        static_cast<std::uint8_t>(v >> 24u)};
    h.update(b);
}

void hash_u64(truthraw::sha256_v0_69::Hasher& h, std::uint64_t v) noexcept {
    std::array<std::uint8_t,8u> b{};
    for (std::size_t i = 0u; i < 8u; ++i) {
        b[i] = static_cast<std::uint8_t>(v >> (8u * i));
    }
    h.update(b);
}

void hash_f64(truthraw::sha256_v0_69::Hasher& h, double v) noexcept {
    hash_u64(h, std::bit_cast<std::uint64_t>(v));
}

bool valid_noise_profile(const DngMetadata& md) noexcept {
    if (!md.hasNoiseProfile) return true;
    for (float v : md.noiseProfile) {
        if (!std::isfinite(v) || v < 0.0f) return false;
    }
    return true;
}

bool variance_for(
    const DngMetadata& md,
    const detail::Workspace& w,
    std::size_t i,
    int channel,
    double mu,
    double& variance) noexcept {
    variance = 0.0;
    if (!md.hasNoiseProfile) return false;
    if (channel < 0 || channel > 2 || i >= w.stage2.size()) return false;
    const double g = md.hasGainField
        ? (i < w.gain.size() ? static_cast<double>(w.gain[i])
                             : std::numeric_limits<double>::quiet_NaN())
        : 1.0;
    if (!std::isfinite(g) || !(g > 0.0) || !std::isfinite(mu)) return false;
    const double s = md.noiseProfile[2 * channel];
    const double o = md.noiseProfile[2 * channel + 1];
    variance = g * s * std::max(mu, 0.0) + g * g * o;
    return std::isfinite(variance) && variance > 0.0;
}

double center_x(const QueryRegion& q) noexcept {
    return 0.5 * (
        static_cast<double>(q.left) +
        static_cast<double>(q.right - 1u));
}

double center_y(const QueryRegion& q) noexcept {
    return 0.5 * (
        static_cast<double>(q.top) +
        static_cast<double>(q.bottom - 1u));
}

double squared_distance_to_rect(
    const QueryRegion& q,
    double x,
    double y) noexcept {
    const double right = static_cast<double>(q.right - 1u);
    const double bottom = static_cast<double>(q.bottom - 1u);
    double dx = 0.0;
    double dy = 0.0;
    if (x < static_cast<double>(q.left)) dx = static_cast<double>(q.left) - x;
    else if (x > right) dx = x - right;
    if (y < static_cast<double>(q.top)) dy = static_cast<double>(q.top) - y;
    else if (y > bottom) dy = y - bottom;
    return dx * dx + dy * dy;
}

void update_nearest(
    Nearest& n,
    double d2,
    std::uint32_t x,
    std::uint32_t y) noexcept {
    if (!std::isfinite(d2) || d2 < 0.0) return;
    if (!n.available || d2 < n.squaredDistance) {
        n.available = true;
        n.squaredDistance = d2;
        n.x = x;
        n.y = y;
    }
}

double fraction(std::uint64_t n, std::uint64_t d) noexcept {
    return d == 0u ? 0.0
                   : static_cast<double>(n) / static_cast<double>(d);
}

std::string hex(const Digest& d) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out(d.size() * 2u, '0');
    for (std::size_t i = 0u; i < d.size(); ++i) {
        out[2u * i] = kHex[d[i] >> 4u];
        out[2u * i + 1u] = kHex[d[i] & 0x0fu];
    }
    return out;
}

std::string base64(const std::vector<std::uint8_t>& bytes) {
    static constexpr char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((bytes.size() + 2u) / 3u) * 4u);
    std::size_t i = 0u;
    while (i + 3u <= bytes.size()) {
        const std::uint32_t v =
            (static_cast<std::uint32_t>(bytes[i]) << 16u) |
            (static_cast<std::uint32_t>(bytes[i + 1u]) << 8u) |
            static_cast<std::uint32_t>(bytes[i + 2u]);
        out.push_back(table[(v >> 18u) & 63u]);
        out.push_back(table[(v >> 12u) & 63u]);
        out.push_back(table[(v >> 6u) & 63u]);
        out.push_back(table[v & 63u]);
        i += 3u;
    }
    if (i < bytes.size()) {
        const std::size_t remain = bytes.size() - i;
        std::uint32_t v = static_cast<std::uint32_t>(bytes[i]) << 16u;
        if (remain == 2u) {
            v |= static_cast<std::uint32_t>(bytes[i + 1u]) << 8u;
        }
        out.push_back(table[(v >> 18u) & 63u]);
        out.push_back(table[(v >> 12u) & 63u]);
        out.push_back(remain == 2u ? table[(v >> 6u) & 63u] : '=');
        out.push_back('=');
    }
    return out;
}

std::string encode_points(const std::vector<ProtectedPoint>& points) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(points.size() * 8u);
    for (const auto& p : points) {
        for (std::uint32_t v : {p.x, p.y}) {
            bytes.push_back(static_cast<std::uint8_t>(v));
            bytes.push_back(static_cast<std::uint8_t>(v >> 8u));
            bytes.push_back(static_cast<std::uint8_t>(v >> 16u));
            bytes.push_back(static_cast<std::uint8_t>(v >> 24u));
        }
    }
    return base64(bytes);
}

void write_nearest(std::ostringstream& o, const Nearest& n) {
    if (!n.available) {
        o << "null";
        return;
    }
    o << "{\"distance_px\":" << std::sqrt(n.squaredDistance)
      << ",\"x\":" << n.x
      << ",\"y\":" << n.y << "}";
}

template <std::size_t N>
void write_u32_array(
    std::ostringstream& o,
    const std::array<std::uint32_t,N>& a) {
    o << "[";
    for (std::size_t i = 0u; i < N; ++i) {
        if (i) o << ",";
        o << a[i];
    }
    o << "]";
}

template <std::size_t N>
void write_u64_array(
    std::ostringstream& o,
    const std::array<std::uint64_t,N>& a) {
    o << "[";
    for (std::size_t i = 0u; i < N; ++i) {
        if (i) o << ",";
        o << a[i];
    }
    o << "]";
}

template <std::size_t N>
void write_fraction_array(
    std::ostringstream& o,
    const std::array<std::uint64_t,N>& n,
    const std::array<std::uint64_t,N>& d) {
    o << "[";
    for (std::size_t i = 0u; i < N; ++i) {
        if (i) o << ",";
        o << fraction(n[i], d[i]);
    }
    o << "]";
}

bool valid_queries(
    std::uint32_t width,
    std::uint32_t height,
    const std::vector<QueryRegion>& queries) noexcept {
    if (queries.empty() || queries.size() > 512u) return false;
    for (const auto& q : queries) {
        if (q.right <= q.left || q.bottom <= q.top ||
            q.left >= width || q.top >= height ||
            q.right > width || q.bottom > height ||
            q.frontsideWidth == 0u || q.frontsideHeight == 0u) {
            return false;
        }
    }
    return true;
}

} // namespace

bool run(
    stream::IRawTileSource& source,
    const Binding& binding,
    const std::vector<QueryRegion>& queries,
    Report& out) noexcept {
    out = {};
    try {
        const auto& md = source.metadata();
        const auto width = static_cast<std::uint32_t>(md.width);
        const auto height = static_cast<std::uint32_t>(md.height);
        if (!nonzero(binding.sourceEvidenceSha256) ||
            !nonzero(binding.scientificMasterSha256) ||
            !nonzero(binding.authorityFieldSha256) ||
            !nonzero(binding.truthNegativeStateSha256) ||
            md.width <= 4 || md.height <= 4 ||
            !valid_noise_profile(md) ||
            !valid_queries(width, height, queries)) {
            return false;
        }

        out.queries.reserve(queries.size());
        for (const auto& q : queries) {
            QueryMetrics m{};
            m.query = q;
            out.queries.push_back(m);
        }

        truthraw::sha256_v0_69::Hasher candidateHasher;
        constexpr char candidateDomain[] = "D_RAW_TN_N2_CFA_AUDIT_CANDIDATE_V0_1";
        candidateHasher.update(
            reinterpret_cast<const std::uint8_t*>(candidateDomain),
            sizeof(candidateDomain) - 1u);
        candidateHasher.update(binding.sourceEvidenceSha256);
        candidateHasher.update(binding.truthNegativeStateSha256);
        hash_u32(candidateHasher, kSamplingPeriod);
        hash_u32(candidateHasher, 0u);
        hash_u32(candidateHasher, 0u);
        hash_u32(candidateHasher, width);
        hash_u32(candidateHasher, height);

        truthraw::sha256_v0_69::Hasher supportHasher;
        constexpr char supportDomain[] =
            "D_RAW_TN_N2_SAMPLE_SUPPORT_POINT_STREAM_V0_1";
        supportHasher.update(
            reinterpret_cast<const std::uint8_t*>(supportDomain),
            sizeof(supportDomain) - 1u);
        supportHasher.update(binding.sourceEvidenceSha256);
        supportHasher.update(binding.truthNegativeStateSha256);
        hash_u32(supportHasher, kSamplingPeriod);

        detail::Workspace workspace{};
        constexpr int kStep = 2;

        for (int y0 = 0; y0 < md.height;
             y0 += static_cast<int>(kEvaluationTileEdge)) {
            const int y1 = std::min(
                md.height,
                y0 + static_cast<int>(kEvaluationTileEdge));
            for (int x0 = 0; x0 < md.width;
                 x0 += static_cast<int>(kEvaluationTileEdge)) {
                const int x1 = std::min(
                    md.width,
                    x0 + static_cast<int>(kEvaluationTileEdge));

                TileRect t{};
                t.x0 = x0; t.y0 = y0; t.x1 = x1; t.y1 = y1;
                t.hx0 = std::max(0, x0 - kStep);
                t.hy0 = std::max(0, y0 - kStep);
                t.hx1 = std::min(md.width, x1 + kStep);
                t.hy1 = std::min(md.height, y1 + kStep);

                const auto filled = detail::fill_stage2(source, t, workspace);
                if (!filled) return false;
                const int tw = t.hx1 - t.hx0;
                const int th = t.hy1 - t.hy0;
                const std::size_t expected =
                    static_cast<std::size_t>(tw) * th;
                if (workspace.stage2.size() != expected ||
                    workspace.raw.size() != expected ||
                    (md.hasGainField && workspace.gain.size() != expected)) {
                    return false;
                }

                const auto idx = [&](int gx, int gy)->std::size_t {
                    return static_cast<std::size_t>(gy - t.hy0) *
                               static_cast<std::size_t>(tw) +
                           static_cast<std::size_t>(gx - t.hx0);
                };

                n2::PixelInput pi{};
                pi.neighborhood.neighbors.reserve(4u);

                for (int gy = y0; gy < y1; ++gy) {
                    if (static_cast<std::uint32_t>(gy) % kSamplingPeriod !=
                        static_cast<std::uint32_t>(gy & 1)) continue;
                    for (int gx = x0; gx < x1; ++gx) {
                        if (static_cast<std::uint32_t>(gx) % kSamplingPeriod !=
                            static_cast<std::uint32_t>(gx & 1)) continue;

                        const std::size_t ci = idx(gx, gy);
                        const double center = workspace.stage2[ci];
                        if (!std::isfinite(center)) return false;

                        ++out.sampled;
                        const auto phaseIndex =
                            static_cast<std::size_t>((gy & 1) * 2 + (gx & 1));
                        ++out.cfaPhaseSamples[phaseIndex];

                        const int channel = measured_channel(md.cfa, gx, gy);
                        if (channel < 0 || channel > 2) return false;

                        n2::PixelResult pr{};
                        const bool border =
                            gx < kStep || gy < kStep ||
                            gx + kStep >= md.width ||
                            gy + kStep >= md.height;
                        if (border) {
                            pr.inputValue = center;
                            pr.candidateValue = center;
                            pr.preserveReason =
                                n2::PreserveReason::NoCompatibleNeighborhood;
                            if (!n2::accumulate(pr, out.audit)) return false;
                            ++out.borderProtected;
                        } else {
                            const bool centerCensored =
                                static_cast<float>(workspace.raw[ci]) >=
                                md.whiteLevel;

                            const std::array<std::pair<int,int>,4u> xy{{
                                {gx - kStep, gy},
                                {gx + kStep, gy},
                                {gx, gy - kStep},
                                {gx, gy + kStep}}};

                            std::array<double,4u> values{};
                            std::array<bool,4u> censored{};
                            for (std::size_t k = 0u; k < xy.size(); ++k) {
                                const auto ni = idx(xy[k].first, xy[k].second);
                                values[k] = workspace.stage2[ni];
                                censored[k] =
                                    static_cast<float>(workspace.raw[ni]) >=
                                    md.whiteLevel;
                                if (!std::isfinite(values[k])) return false;
                            }
                            const bool boundaryCensored =
                                std::any_of(
                                    censored.begin(),
                                    censored.end(),
                                    [](bool v){ return v; });

                            double centerVariance = 0.0;
                            const bool centerVarianceKnown =
                                !centerCensored &&
                                variance_for(
                                    md,
                                    workspace,
                                    ci,
                                    channel,
                                    center,
                                    centerVariance);
                            const double sigma = centerVarianceKnown
                                ? std::sqrt(centerVariance)
                                : 0.0;

                            pi.structure = {};
                            pi.structure.center = center;
                            pi.structure.left = values[0];
                            pi.structure.right = values[1];
                            pi.structure.up = values[2];
                            pi.structure.down = values[3];
                            pi.structure.sigma = sigma;
                            pi.structure.sigmaKnown = centerVarianceKnown;
                            pi.structure.censored = centerCensored;
                            pi.structure.boundaryCensored = boundaryCensored;
                            pi.structure.measuredSupport = true;
                            pi.structure.registrationConfidence = 1.0;
                            pi.structure.visibilityConfidence = 1.0;
                            pi.structure.sampleStep =
                                static_cast<double>(kStep);

                            pi.neighborhood.center = center;
                            pi.neighborhood.centerVariance = centerVariance;
                            pi.neighborhood.centerVarianceKnown =
                                centerVarianceKnown;
                            pi.neighborhood.neighbors.clear();

                            for (std::size_t k = 0u; k < xy.size(); ++k) {
                                const auto ni = idx(xy[k].first, xy[k].second);
                                double nv = 0.0;
                                const bool vk =
                                    !censored[k] &&
                                    variance_for(
                                        md,
                                        workspace,
                                        ni,
                                        channel,
                                        values[k],
                                        nv);
                                neigh::Sample s{};
                                s.value = values[k];
                                s.variance = nv;
                                s.spatialDistance =
                                    static_cast<double>(kStep);
                                s.authority = censored[k]
                                    ? neigh::SampleAuthority::Censored
                                    : neigh::SampleAuthority::Measured;
                                s.varianceKnown = vk;
                                s.sameChannel = true;
                                s.sameObject = true;
                                s.censorBoundary = censored[k];
                                s.objectIdentityKnown = false;
                                pi.neighborhood.neighbors.push_back(s);
                            }

                            if (!n2::evaluatePixel(pi, pr) ||
                                !n2::accumulate(pr, out.audit)) {
                                return false;
                            }
                        }

                        hash_u32(
                            candidateHasher,
                            static_cast<std::uint32_t>(gx));
                        hash_u32(
                            candidateHasher,
                            static_cast<std::uint32_t>(gy));
                        hash_f64(candidateHasher, pr.inputValue);
                        hash_f64(candidateHasher, pr.candidateValue);
                        hash_f64(candidateHasher, pr.correction);
                        hash_u32(
                            candidateHasher,
                            static_cast<std::uint32_t>(pr.preserveReason));

                        const auto reason = pr.preserveReason;
                        const bool isStructure =
                            reason == n2::PreserveReason::Structure;
                        const bool isCensored =
                            reason == n2::PreserveReason::Censored;
                        const bool isCensorBoundary =
                            reason == n2::PreserveReason::CensorBoundary;

                        if (isStructure || isCensored || isCensorBoundary) {
                            hash_u32(
                                supportHasher,
                                static_cast<std::uint32_t>(gx));
                            hash_u32(
                                supportHasher,
                                static_cast<std::uint32_t>(gy));
                            hash_u32(
                                supportHasher,
                                static_cast<std::uint32_t>(reason));
                        }
                        if (isStructure) {
                            out.structurePoints.push_back(
                                {static_cast<std::uint32_t>(gx),
                                 static_cast<std::uint32_t>(gy)});
                        } else if (isCensored) {
                            out.censoredPoints.push_back(
                                {static_cast<std::uint32_t>(gx),
                                 static_cast<std::uint32_t>(gy)});
                        } else if (isCensorBoundary) {
                            out.censorBoundaryPoints.push_back(
                                {static_cast<std::uint32_t>(gx),
                                 static_cast<std::uint32_t>(gy)});
                        }

                        const double sx = static_cast<double>(gx);
                        const double sy = static_cast<double>(gy);
                        for (auto& qm : out.queries) {
                            const double dx = sx - center_x(qm.query);
                            const double dy = sy - center_y(qm.query);
                            const double centerD2 = dx * dx + dy * dy;
                            const double rectD2 =
                                squared_distance_to_rect(qm.query, sx, sy);

                            for (std::size_t r = 0u;
                                 r < kCenterRadiiPx.size(); ++r) {
                                const double rr =
                                    static_cast<double>(kCenterRadiiPx[r]);
                                if (centerD2 <= rr * rr) {
                                    ++qm.center.sampled[r];
                                    if (isStructure) ++qm.center.structure[r];
                                    if (isCensored) ++qm.center.censored[r];
                                    if (isCensorBoundary) {
                                        ++qm.center.censorBoundary[r];
                                    }
                                }
                            }

                            for (std::size_t r = 0u;
                                 r < kRectMarginRadiiPx.size(); ++r) {
                                const double rr =
                                    static_cast<double>(kRectMarginRadiiPx[r]);
                                if (rectD2 <= rr * rr) {
                                    ++qm.rectMargin.sampled[r];
                                    if (isStructure) {
                                        ++qm.rectMargin.structure[r];
                                    }
                                    if (isCensored) {
                                        ++qm.rectMargin.censored[r];
                                    }
                                    if (isCensorBoundary) {
                                        ++qm.rectMargin.censorBoundary[r];
                                    }
                                }
                            }

                            if (isStructure) {
                                update_nearest(
                                    qm.nearestStructureFromCenter,
                                    centerD2,
                                    static_cast<std::uint32_t>(gx),
                                    static_cast<std::uint32_t>(gy));
                                update_nearest(
                                    qm.nearestStructureToRect,
                                    rectD2,
                                    static_cast<std::uint32_t>(gx),
                                    static_cast<std::uint32_t>(gy));
                            } else if (isCensored) {
                                update_nearest(
                                    qm.nearestCensoredFromCenter,
                                    centerD2,
                                    static_cast<std::uint32_t>(gx),
                                    static_cast<std::uint32_t>(gy));
                                update_nearest(
                                    qm.nearestCensoredToRect,
                                    rectD2,
                                    static_cast<std::uint32_t>(gx),
                                    static_cast<std::uint32_t>(gy));
                            } else if (isCensorBoundary) {
                                update_nearest(
                                    qm.nearestCensorBoundaryFromCenter,
                                    centerD2,
                                    static_cast<std::uint32_t>(gx),
                                    static_cast<std::uint32_t>(gy));
                                update_nearest(
                                    qm.nearestCensorBoundaryToRect,
                                    rectD2,
                                    static_cast<std::uint32_t>(gx),
                                    static_cast<std::uint32_t>(gy));
                            }
                        }
                    }
                }
            }
        }

        if (out.sampled == 0u ||
            out.audit.total != out.sampled ||
            out.queries.empty()) {
            return false;
        }

        out.candidateSha256 = candidateHasher.finalize();
        out.supportPointStreamSha256 = supportHasher.finalize();
        if (!nonzero(out.candidateSha256) ||
            !nonzero(out.supportPointStreamSha256)) {
            return false;
        }

        truthraw::sha256_v0_69::Hasher auditHasher;
        constexpr char auditDomain[] = "D_RAW_TN_N2_CFA_AUDIT_REPORT_V0_1";
        auditHasher.update(
            reinterpret_cast<const std::uint8_t*>(auditDomain),
            sizeof(auditDomain) - 1u);
        auditHasher.update(binding.sourceEvidenceSha256);
        auditHasher.update(binding.truthNegativeStateSha256);
        auditHasher.update(out.candidateSha256);
        hash_u64(auditHasher, out.sampled);
        hash_u64(auditHasher, out.audit.eligible);
        hash_u64(auditHasher, out.audit.corrected);
        hash_u64(auditHasher, out.audit.preserved);
        hash_u64(auditHasher, out.audit.censoredProtected);
        hash_u64(auditHasher, out.audit.censorBoundaryProtected);
        hash_u64(auditHasher, out.audit.structureProtected);
        hash_u64(auditHasher, out.audit.unknownNoiseProtected);
        hash_u64(auditHasher, out.audit.noNeighborhoodProtected);
        hash_u64(auditHasher, out.audit.residualOutlierProtected);
        hash_u64(auditHasher, out.borderProtected);
        hash_f64(auditHasher, out.audit.totalResidualEnergy);
        hash_f64(auditHasher, out.audit.removedResidualEnergy);
        hash_f64(auditHasher, out.audit.maxAbsCorrection);
        for (auto v : out.cfaPhaseSamples) hash_u64(auditHasher, v);
        out.auditSha256 = auditHasher.finalize();
        if (!nonzero(out.auditSha256)) return false;

        const std::string structureEncoded = encode_points(out.structurePoints);
        const std::string censoredEncoded = encode_points(out.censoredPoints);
        const std::string boundaryEncoded =
            encode_points(out.censorBoundaryPoints);

        std::ostringstream o;
        o.setf(std::ios::fixed);
        o << std::setprecision(12);
        o << "{\n";
        o << "  \"schema\":\"" << kSchemaName << "\",\n";
        o << "  \"source_width\":" << width << ",\n";
        o << "  \"source_height\":" << height << ",\n";
        o << "  \"evaluation_tile_edge\":" << kEvaluationTileEdge << ",\n";
        o << "  \"sampling_period\":" << kSamplingPeriod << ",\n";
        o << "  \"source_sha256\":\""
          << hex(binding.sourceEvidenceSha256) << "\",\n";
        o << "  \"scientific_master_sha256\":\""
          << hex(binding.scientificMasterSha256) << "\",\n";
        o << "  \"authority_field_sha256\":\""
          << hex(binding.authorityFieldSha256) << "\",\n";
        o << "  \"truthnegative_state_sha256\":\""
          << hex(binding.truthNegativeStateSha256) << "\",\n";
        o << "  \"candidate_sha256\":\""
          << hex(out.candidateSha256) << "\",\n";
        o << "  \"audit_sha256\":\""
          << hex(out.auditSha256) << "\",\n";
        o << "  \"support_point_stream_sha256\":\""
          << hex(out.supportPointStreamSha256) << "\",\n";
        o << "  \"coordinate_encoding\":"
          << "\"BASE64_LE_U32_XY_PAIRS_SOURCE_NATIVE\",\n";
        o << "  \"exact_sample_coordinates_recorded\":true,\n";
        o << "  \"sample_grid_evidence_only\":true,\n";
        o << "  \"unsampled_pixels_inferred\":false,\n";
        o << "  \"scalar_probability_created\":false,\n";
        o << "  \"can_reduce_protection\":false,\n";
        o << "  \"can_enable_correction\":false,\n";
        o << "  \"promotion_eligible\":false,\n";
        o << "  \"candidate_applied\":false,\n";
        o << "  \"creates_new_evidence\":false,\n";
        o << "  \"scientific_writeback_allowed\":false,\n";
        o << "  \"global\":{";
        o << "\"sampled\":" << out.sampled;
        o << ",\"eligible\":" << out.audit.eligible;
        o << ",\"corrected_private_candidate\":" << out.audit.corrected;
        o << ",\"preserved\":" << out.audit.preserved;
        o << ",\"structure_protected\":" << out.audit.structureProtected;
        o << ",\"censored_protected\":" << out.audit.censoredProtected;
        o << ",\"censor_boundary_protected\":"
          << out.audit.censorBoundaryProtected;
        o << ",\"structure_fraction\":"
          << fraction(out.audit.structureProtected, out.sampled);
        o << "},\n";
        o << "  \"coordinate_streams\":{";
        o << "\"structure_count\":" << out.structurePoints.size();
        o << ",\"structure_xy_base64\":\"" << structureEncoded << "\"";
        o << ",\"censored_count\":" << out.censoredPoints.size();
        o << ",\"censored_xy_base64\":\"" << censoredEncoded << "\"";
        o << ",\"censor_boundary_count\":"
          << out.censorBoundaryPoints.size();
        o << ",\"censor_boundary_xy_base64\":\""
          << boundaryEncoded << "\"},\n";

        o << "  \"center_radii_px\":";
        write_u32_array(o, kCenterRadiiPx);
        o << ",\n  \"rect_margin_radii_px\":";
        write_u32_array(o, kRectMarginRadiiPx);
        o << ",\n  \"queries\":[\n";

        for (std::size_t qi = 0u; qi < out.queries.size(); ++qi) {
            const auto& q = out.queries[qi];
            o << "    {\"id\":" << q.query.id;
            o << ",\"frontside_x\":" << q.query.frontsideX;
            o << ",\"frontside_y\":" << q.query.frontsideY;
            o << ",\"frontside_width\":" << q.query.frontsideWidth;
            o << ",\"frontside_height\":" << q.query.frontsideHeight;
            o << ",\"source_rect\":["
              << q.query.left << "," << q.query.top << ","
              << q.query.right << "," << q.query.bottom << "]";
            o << ",\"source_center\":["
              << center_x(q.query) << "," << center_y(q.query) << "]";

            o << ",\"center_sampled\":";
            write_u64_array(o, q.center.sampled);
            o << ",\"center_structure\":";
            write_u64_array(o, q.center.structure);
            o << ",\"center_structure_fraction\":";
            write_fraction_array(o, q.center.structure, q.center.sampled);
            o << ",\"center_censored\":";
            write_u64_array(o, q.center.censored);
            o << ",\"center_censor_boundary\":";
            write_u64_array(o, q.center.censorBoundary);

            o << ",\"rect_margin_sampled\":";
            write_u64_array(o, q.rectMargin.sampled);
            o << ",\"rect_margin_structure\":";
            write_u64_array(o, q.rectMargin.structure);
            o << ",\"rect_margin_structure_fraction\":";
            write_fraction_array(
                o,
                q.rectMargin.structure,
                q.rectMargin.sampled);
            o << ",\"rect_margin_censored\":";
            write_u64_array(o, q.rectMargin.censored);
            o << ",\"rect_margin_censor_boundary\":";
            write_u64_array(o, q.rectMargin.censorBoundary);

            o << ",\"nearest_structure_from_center\":";
            write_nearest(o, q.nearestStructureFromCenter);
            o << ",\"nearest_structure_to_rect\":";
            write_nearest(o, q.nearestStructureToRect);
            o << ",\"nearest_censored_from_center\":";
            write_nearest(o, q.nearestCensoredFromCenter);
            o << ",\"nearest_censored_to_rect\":";
            write_nearest(o, q.nearestCensoredToRect);
            o << ",\"nearest_censor_boundary_from_center\":";
            write_nearest(o, q.nearestCensorBoundaryFromCenter);
            o << ",\"nearest_censor_boundary_to_rect\":";
            write_nearest(o, q.nearestCensorBoundaryToRect);
            o << "}";
            if (qi + 1u < out.queries.size()) o << ",";
            o << "\n";
        }
        o << "  ]\n";
        o << "}\n";

        out.json = o.str();
        truthraw::sha256_v0_69::Hasher jsonHasher;
        jsonHasher.update(
            reinterpret_cast<const std::uint8_t*>(out.json.data()),
            out.json.size());
        out.jsonSha256 = jsonHasher.finalize();

        return !out.json.empty() &&
               nonzero(out.jsonSha256) &&
               out.exactSampleCoordinatesRecorded &&
               !out.unsampledPixelsInferred &&
               !out.scalarProbabilityCreated &&
               !out.canReduceProtection &&
               !out.canEnableCorrection &&
               !out.candidateApplied &&
               !out.createsNewEvidence &&
               !out.scientificWritebackAllowed;
    } catch (...) {
        out = {};
        return false;
    }
}

} // namespace truthraw::truthnegative_n2_support_distance::v0_1
