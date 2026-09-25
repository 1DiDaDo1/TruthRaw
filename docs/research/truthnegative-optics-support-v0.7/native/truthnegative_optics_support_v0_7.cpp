#include "truthnegative_optics_support_v0_7.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <map>
#include <utility>

namespace truthraw::truthnegative_optics_support::v0_7 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(
        d.begin(), d.end(), [](std::uint8_t v) { return v != 0u; });
}

bool finitePositive(double v) noexcept {
    return std::isfinite(v) && v > 0.0;
}

void hash_u32(
    truthraw::sha256_v0_69::Hasher& h,
    std::uint32_t v) noexcept {
    const std::array<std::uint8_t, 4u> b{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v >> 8u),
        static_cast<std::uint8_t>(v >> 16u),
        static_cast<std::uint8_t>(v >> 24u),
    };
    h.update(b);
}

void hash_u64(
    truthraw::sha256_v0_69::Hasher& h,
    std::uint64_t v) noexcept {
    std::array<std::uint8_t, 8u> b{};
    for (unsigned i = 0u; i < 8u; ++i) {
        b[i] = static_cast<std::uint8_t>(v >> (8u * i));
    }
    h.update(b);
}

void hash_f64(
    truthraw::sha256_v0_69::Hasher& h,
    double v) noexcept {
    hash_u64(h, std::bit_cast<std::uint64_t>(v));
}

bool validAuthority(CalibrationAuthority a) noexcept {
    const auto v = static_cast<std::uint8_t>(a);
    return v >= 1u && v <= 4u;
}

}  // namespace

bool finalizeCalibration(
    const CalibrationInput& input,
    CalibrationState& out) noexcept {
    out = CalibrationState{};
    try {
        if (!nonzero(input.sourceEvidenceSha256) ||
            !nonzero(input.lensIdentitySha256) ||
            !nonzero(input.sensorIdentitySha256) ||
            !nonzero(input.calibrationEvidenceSha256) ||
            !validAuthority(input.authority) ||
            input.kernelWidth == 0u ||
            input.kernelHeight == 0u ||
            (input.kernelWidth % 2u) == 0u ||
            (input.kernelHeight % 2u) == 0u ||
            input.kernelWidth > 31u ||
            input.kernelHeight > 31u ||
            input.psfKernel.size() !=
                static_cast<std::size_t>(input.kernelWidth) *
                input.kernelHeight ||
            !finitePositive(input.mtf50XCyclesPerPixel) ||
            !finitePositive(input.mtf50YCyclesPerPixel) ||
            input.mtf50XCyclesPerPixel > 0.5 ||
            input.mtf50YCyclesPerPixel > 0.5) {
            return false;
        }

        double sum = 0.0;
        for (double w : input.psfKernel) {
            if (!std::isfinite(w) || w < 0.0) return false;
            sum += w;
        }
        if (!finitePositive(sum)) return false;

        out.input = input;
        out.normalizedPsfKernel.resize(input.psfKernel.size());
        for (std::size_t i = 0u; i < input.psfKernel.size(); ++i) {
            out.normalizedPsfKernel[i] = input.psfKernel[i] / sum;
        }

        out.scientificSupportUseAllowed =
            input.authority == CalibrationAuthority::Measured ||
            input.authority == CalibrationAuthority::CalibratedEstimate;
        out.deconvolutionAllowed = false;
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[] =
            "D_RAW_TRUTHNEGATIVE_OPTICS_SUPPORT_V0_7";
        h.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain) - 1u);
        h.update(input.sourceEvidenceSha256);
        h.update(input.lensIdentitySha256);
        h.update(input.sensorIdentitySha256);
        h.update(input.calibrationEvidenceSha256);
        const auto authority =
            static_cast<std::uint8_t>(input.authority);
        h.update(&authority, 1u);
        hash_u32(h, input.kernelWidth);
        hash_u32(h, input.kernelHeight);
        for (double w : out.normalizedPsfKernel) hash_f64(h, w);
        hash_f64(h, input.mtf50XCyclesPerPixel);
        hash_f64(h, input.mtf50YCyclesPerPixel);
        out.stateSha256 = h.finalize();
        return nonzero(out.stateSha256);
    } catch (...) {
        out = CalibrationState{};
        return false;
    }
}

bool propagateSupport(
    const CalibrationState& calibration,
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight,
    const free_world::ResolvedPixel& query,
    EffectiveSupport& out) noexcept {
    out = EffectiveSupport{};
    try {
        if (!calibration.scientificSupportUseAllowed ||
            !nonzero(calibration.stateSha256) ||
            calibration.deconvolutionAllowed ||
            calibration.createsNewEvidence ||
            calibration.scientificWritebackAllowed ||
            sourceWidth == 0u ||
            sourceHeight == 0u ||
            query.footprint.empty() ||
            query.createsNewEvidence ||
            query.measuredTargetClaimCount != 0u) {
            return false;
        }

        const int rx =
            static_cast<int>(calibration.input.kernelWidth / 2u);
        const int ry =
            static_cast<int>(calibration.input.kernelHeight / 2u);

        std::map<std::pair<std::uint32_t,std::uint32_t>, double> weights;
        double inputSum = 0.0;
        for (const auto& f : query.footprint) {
            if (f.x >= sourceWidth ||
                f.y >= sourceHeight ||
                !(f.weight > 0.0) ||
                !std::isfinite(f.weight)) {
                return false;
            }
            inputSum += f.weight;

            for (std::uint32_t ky = 0u;
                 ky < calibration.input.kernelHeight; ++ky) {
                for (std::uint32_t kx = 0u;
                     kx < calibration.input.kernelWidth; ++kx) {
                    const std::size_t ki =
                        static_cast<std::size_t>(ky) *
                            calibration.input.kernelWidth + kx;
                    const double kw =
                        calibration.normalizedPsfKernel[ki];
                    if (kw <= 0.0) continue;

                    const int sx = std::clamp(
                        static_cast<int>(f.x) +
                            static_cast<int>(kx) - rx,
                        0,
                        static_cast<int>(sourceWidth) - 1);
                    const int sy = std::clamp(
                        static_cast<int>(f.y) +
                            static_cast<int>(ky) - ry,
                        0,
                        static_cast<int>(sourceHeight) - 1);
                    weights[{
                        static_cast<std::uint32_t>(sx),
                        static_cast<std::uint32_t>(sy)}] +=
                            f.weight * kw;
                }
            }
        }
        if (std::abs(inputSum - 1.0) > 1e-12) return false;

        double sum = 0.0;
        for (const auto& [_, w] : weights) sum += w;
        if (!std::isfinite(sum) || std::abs(sum - 1.0) > 1e-12) {
            return false;
        }

        out.footprint.reserve(weights.size());
        for (const auto& [xy, w] : weights) {
            if (!(w > 0.0) || !std::isfinite(w)) return false;
            out.footprint.push_back({xy.first, xy.second, w});
            out.weightSum += w;
        }

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[] =
            "D_RAW_TRUTHNEGATIVE_EFFECTIVE_OPTICAL_SUPPORT_V0_7";
        h.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain) - 1u);
        h.update(calibration.stateSha256);
        hash_u32(h, sourceWidth);
        hash_u32(h, sourceHeight);
        hash_u64(
            h,
            static_cast<std::uint64_t>(out.footprint.size()));
        for (const auto& f : out.footprint) {
            hash_u32(h, f.x);
            hash_u32(h, f.y);
            hash_f64(h, f.weight);
        }

        out.calibrationStateSha256 = calibration.stateSha256;
        out.supportSha256 = h.finalize();
        out.numericSceneValueChanged = false;
        out.authorityUpgraded = false;
        out.deconvolutionApplied = false;
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;

        return nonzero(out.supportSha256) &&
               std::abs(out.weightSum - 1.0) <= 1e-12;
    } catch (...) {
        out = EffectiveSupport{};
        return false;
    }
}

const char* toString(CalibrationAuthority authority) noexcept {
    switch (authority) {
        case CalibrationAuthority::Measured: return "MEASURED";
        case CalibrationAuthority::CalibratedEstimate:
            return "CALIBRATED_ESTIMATE";
        case CalibrationAuthority::Inferred: return "INFERRED";
        case CalibrationAuthority::Unknown: return "UNKNOWN";
    }
    return "INVALID";
}

const char* schema_name() noexcept {
    return kSchemaName;
}

}  // namespace truthraw::truthnegative_optics_support::v0_7
