#pragma once

#include "truthnegative_n2_candidate_pipeline_v0_1.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace truthraw::truthnegative_n2_risk_quality_audit::v0_1 {

using Digest = truthraw::sha256_v0_69::Digest;
namespace n2 = truthraw::truthnegative_n2_candidate_pipeline::v0_1;

struct Quantiles final {
    double mean = 0.0;
    double p50 = 0.0;
    double p95 = 0.0;
    double p99 = 0.0;
    double max = 0.0;
};

struct Input final {
    const double* aEncodedRgb = nullptr;
    const double* bEncodedRgb = nullptr;
    const std::uint32_t* preserveReasonMask = nullptr;
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    std::uint32_t sourceX = 0u;
    std::uint32_t sourceY = 0u;
    Digest candidateIdentitySha256{};
};

struct Result final {
    Quantiles pixelMaxAbsDelta{};
    std::array<Quantiles,3u> channelAbsDelta{};
    Quantiles lumaAbsDelta{};
    Quantiles chromaDelta{};

    std::uint32_t maxLocalX = 0u;
    std::uint32_t maxLocalY = 0u;
    std::uint32_t maxSourceX = 0u;
    std::uint32_t maxSourceY = 0u;
    std::uint32_t maxPreserveReasonMask = 0u;

    double distanceToStructurePx = -1.0;
    double distanceToCensorOrBoundaryPx = -1.0;

    double edgeEnergyA = 0.0;
    double edgeEnergyB = 0.0;
    double edgeEnergyRatio = 1.0;
    double meanAbsGradientDelta = 0.0;

    std::uint64_t structureMaskPixels = 0u;
    std::uint64_t censorMaskPixels = 0u;
    std::uint64_t changedPixels = 0u;

    Digest qualitySha256{};
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

bool evaluate(const Input& input,Result& out) noexcept;

} // namespace truthraw::truthnegative_n2_risk_quality_audit::v0_1
