#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace truthraw::appearance::bridge::v1 {

enum class Status : std::uint8_t { Applied=0, InvalidInput=1, InvalidConfig=2, ConditioningAuditMissing=3 };

struct RgbLinear { float r; float g; float b; };

struct Config {
    float globalCurveStrength = 0.42f;
    float shadowPivot = 0.18f;
    float maxResidualCompression = 0.35f;
    float lowConfidenceChromaGain = 0.82f;
    float highConfidenceChromaGain = 1.25f;
    float censoredMaxChromaGain = 1.0f;
    int filterRadius = 4;
    float sigmaSpatial = 2.0f;
    float sigmaRange = 0.08f;
};

struct EvidenceImage {
    std::span<const float> lumaConfidence;
    std::span<const float> chromaConfidence;
    std::span<const std::uint8_t> sourceHighCensored;
    bool conditioningAuditPassed = false;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
};

struct Result {
    Status status = Status::InvalidInput;
    std::vector<RgbLinear> rgb;
    std::vector<float> regularizedLumaConfidence;
    std::vector<float> regularizedChromaConfidence;
    std::vector<float> residualGain;
    std::vector<float> chromaGain;
    bool scientificMasterModified = false;
};

bool validate_config(const Config& config) noexcept;
Result apply_image(std::span<const RgbLinear> displayLinearRgb, std::size_t width, std::size_t height,
                   const EvidenceImage& evidence, const Config& config = {});

} // namespace truthraw::appearance::bridge::v1
