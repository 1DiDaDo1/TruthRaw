#pragma once
#include <cstdint>
#include <span>

namespace truthraw::conditioning::v1 {

enum class Status : std::uint8_t { Ok=0, InvalidInput=1, RangeFailure=2 };
enum class CandidateDecision : std::uint8_t {
    ExactReparameterization=0,
    CandidateRejected=1,
    CandidateAdmitted=2
};

struct Config {
    int minConditioningEv = -32;
    int maxConditioningEv = 32;
    int preferredBinaryExponent = -2;
};

struct GaussianScalar {
    float mean = 0.0f;
    float sigma = 1.0f;
};

struct ConditioningGauge {
    int ev = 0;
    bool targetReached = false;
};

struct ConditionedScalar {
    GaussianScalar value;
    ConditioningGauge gauge;
};

struct TruthRangeInterval {
    double lowerEv;
    double upperEv;
};

struct AdmissionEvidence {
    bool parametersFrozen = false;
    bool independentHeldoutEvidence = false;
    bool scalarErrorGatesPass = false;
    bool topologyGatesPass = false;
};

Status choose_best_conditioning_gauge(const GaussianScalar& in, const Config& cfg,
                                      ConditioningGauge& out) noexcept;
Status choose_common_conditioning_gauge(std::span<const GaussianScalar> values, const Config& cfg,
                                        ConditioningGauge& out) noexcept;
Status condition_exact(const GaussianScalar& in, const ConditioningGauge& gauge,
                       ConditionedScalar& out) noexcept;
Status decondition_exact(const ConditionedScalar& in, GaussianScalar& out) noexcept;
Status shift_truthrange_for_conditioning(const TruthRangeInterval& in, int ev,
                                         TruthRangeInterval& out) noexcept;
Status unshift_truthrange_after_conditioning(const TruthRangeInterval& in, int ev,
                                             TruthRangeInterval& out) noexcept;

double snr_abs(const GaussianScalar& x) noexcept;
double standardized_residual(float sample, const GaussianScalar& model) noexcept;
CandidateDecision decide_candidate(const AdmissionEvidence& evidence) noexcept;
const char* decision_name(CandidateDecision d) noexcept;

} // namespace truthraw::conditioning::v1
