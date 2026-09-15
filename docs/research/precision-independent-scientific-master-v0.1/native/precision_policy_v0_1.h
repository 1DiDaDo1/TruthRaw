#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace truthraw_precision_v01 {

enum class EvidenceEncodingV01 : std::uint8_t {
    Unknown = 0,
    RawSensorU16 = 1,
    Raw10Packed = 2,
    Raw12Packed = 3,
    Raw14Packed = 4,
};

struct ExactEvidenceDescriptorV01 {
    EvidenceEncodingV01 encoding = EvidenceEncodingV01::Unknown;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t rowStrideBytes = 0;
    std::uint32_t pixelStrideBytes = 0;
    std::uint64_t payloadBytes = 0;
    bool exactSourceBytesRetained = false;
};

enum class ReconstructionScalarV01 : std::uint8_t {
    Float32 = 0,
    Float64 = 1,
};

struct PrecisionPolicyV01 {
    ReconstructionScalarV01 reconstruction = ReconstructionScalarV01::Float32;
    bool exactIntegerEvidenceRequired = true;
    bool calibrationUsesFloat64 = true;
    bool covarianceUsesFloat64 = true;
    bool higherPrecisionIsReferenceOnly = true;
    bool mayChangeEvidenceAuthority = false;
};

struct Stage2ParamsV01 {
    double black = 0.0;
    double white = 1.0;
    double gain = 1.0;
};

template <typename WorkT>
inline WorkT normalize_raw_u16_v0_1(std::uint16_t code, const Stage2ParamsV01& p) {
    static_assert(std::is_same<WorkT, float>::value || std::is_same<WorkT, double>::value,
                  "TruthRaw v0.1 reconstruction work type must be float or double");
    const double denom = std::max(p.white - p.black, 1.0);
    const double normalized = ((static_cast<double>(code) - p.black) / denom) * p.gain;
    return static_cast<WorkT>(normalized);
}

// Convert a rectangular tile from exact unpacked uint16 evidence into a selected
// floating work type. CFA black phase is evaluated in GLOBAL coordinates so tile
// boundaries cannot silently shift phase. The source integer buffer is read-only.
template <typename WorkT>
inline bool normalize_raw_tile_u16_v0_1(
    const std::uint16_t* raw,
    int rawWidth,
    int rawHeight,
    int rawRowStrideSamples,
    int x0,
    int y0,
    int tileWidth,
    int tileHeight,
    const std::array<double,4>& blackPhase,
    double white,
    const double* gainField,
    int gainRowStrideSamples,
    WorkT* out,
    int outRowStrideSamples) {

    static_assert(std::is_same<WorkT, float>::value || std::is_same<WorkT, double>::value,
                  "TruthRaw v0.1 reconstruction work type must be float or double");
    if (!raw || !out || rawWidth <= 0 || rawHeight <= 0 || rawRowStrideSamples < rawWidth ||
        tileWidth <= 0 || tileHeight <= 0 || x0 < 0 || y0 < 0 ||
        x0 + tileWidth > rawWidth || y0 + tileHeight > rawHeight ||
        outRowStrideSamples < tileWidth) return false;
    if (gainField && gainRowStrideSamples < rawWidth) return false;

    for (int yy = 0; yy < tileHeight; ++yy) {
        const int gy = y0 + yy;
        for (int xx = 0; xx < tileWidth; ++xx) {
            const int gx = x0 + xx;
            const std::size_t ri = static_cast<std::size_t>(gy) * static_cast<std::size_t>(rawRowStrideSamples) + static_cast<std::size_t>(gx);
            const int phase = (gy & 1) * 2 + (gx & 1);
            Stage2ParamsV01 p;
            p.black = blackPhase[static_cast<std::size_t>(phase)];
            p.white = white;
            p.gain = gainField
                ? gainField[static_cast<std::size_t>(gy) * static_cast<std::size_t>(gainRowStrideSamples) + static_cast<std::size_t>(gx)]
                : 1.0;
            out[static_cast<std::size_t>(yy) * static_cast<std::size_t>(outRowStrideSamples) + static_cast<std::size_t>(xx)] =
                normalize_raw_u16_v0_1<WorkT>(raw[ri], p);
        }
    }
    return true;
}

// Neumaier-style compensated summation. The extra correction term is retained
// separately so large cancellation does not discard small scientific terms.
struct CompensatedSum64V01 {
    double sum = 0.0;
    double correction = 0.0;

    void add(double x) {
        const double t = sum + x;
        if (std::abs(sum) >= std::abs(x)) {
            correction += (sum - t) + x;
        } else {
            correction += (x - t) + sum;
        }
        sum = t;
    }

    double value() const { return sum + correction; }
};

// Welford running moments for dark/noise/PTC and calibration statistics.
struct RunningMoments64V01 {
    std::uint64_t count = 0;
    double mean = 0.0;
    double m2 = 0.0;

    void add(double x) {
        ++count;
        const double delta = x - mean;
        mean += delta / static_cast<double>(count);
        const double delta2 = x - mean;
        m2 += delta * delta2;
    }

    double populationVariance() const {
        return count ? m2 / static_cast<double>(count) : std::numeric_limits<double>::quiet_NaN();
    }

    double sampleVariance() const {
        return count > 1 ? m2 / static_cast<double>(count - 1) : std::numeric_limits<double>::quiet_NaN();
    }
};

struct Matrix3dV01 {
    std::array<double, 9> v {1.0,0.0,0.0, 0.0,1.0,0.0, 0.0,0.0,1.0};

    double at(int r, int c) const { return v[static_cast<std::size_t>(r) * 3u + static_cast<std::size_t>(c)]; }
    double& at(int r, int c) { return v[static_cast<std::size_t>(r) * 3u + static_cast<std::size_t>(c)]; }
};

struct Covariance3dV01 {
    std::array<double, 9> v {
        std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()
    };
    std::uint16_t knownMask = 0; // 9 bits, row-major. Unknown is never assumed zero.
    bool psdCertified = false;

    static constexpr std::uint16_t fullMask = 0x01FFu;
    bool fullyKnown() const { return (knownMask & fullMask) == fullMask; }
    double at(int r, int c) const { return v[static_cast<std::size_t>(r) * 3u + static_cast<std::size_t>(c)]; }
    double& at(int r, int c) { return v[static_cast<std::size_t>(r) * 3u + static_cast<std::size_t>(c)]; }
};

Matrix3dV01 multiply_v0_1(const Matrix3dV01& a, const Matrix3dV01& b);
Matrix3dV01 transpose_v0_1(const Matrix3dV01& a);

// Computes A*C*A^T only when the full covariance is known. On failure, out is reset to unknown/NaN.
bool propagate_full_covariance_v0_1(
    const Matrix3dV01& transform,
    const Covariance3dV01& input,
    Covariance3dV01& out);

// Scientific reduction helper used for calibration/reference computations.
double mean_float64_v0_1(const double* values, std::size_t count);

// Returns true when policy changes only numerical execution and cannot raise evidence authority.
bool precision_policy_authority_invariant_v0_1(const PrecisionPolicyV01& p);

} // namespace truthraw_precision_v01
