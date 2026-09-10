#include "manifold_conditioning_v1.h"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <cmath>
#include <limits>

namespace truthraw::conditioning::v1 {
namespace {
bool valid(const GaussianScalar& x) {
    return std::isfinite(x.mean) && std::isfinite(x.sigma) && x.sigma > 0.0f;
}
bool valid_cfg(const Config& c) {
    return c.minConditioningEv <= c.maxConditioningEv &&
           c.minConditioningEv >= -120 && c.maxConditioningEv <= 120 &&
           c.preferredBinaryExponent >= -100 && c.preferredBinaryExponent <= 100;
}
double shift_bound(double x, int ev) {
    if (std::isnan(x) || std::isinf(x)) return x;
    return x + static_cast<double>(ev);
}
bool same_bits(float a, float b) {
    return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b);
}
bool exact_roundtrip(float x, int ev) {
    const float y = std::scalbn(x, ev);
    if (!std::isfinite(y)) return false;
    if (x != 0.0f && y == 0.0f) return false;
    const float z = std::scalbn(y, -ev);
    return std::isfinite(z) && same_bits(x, z);
}
bool exact_state_roundtrip(const GaussianScalar& x, int ev) {
    return exact_roundtrip(x.mean, ev) && exact_roundtrip(x.sigma, ev);
}
int move_toward_zero(int ev) { return ev > 0 ? ev - 1 : (ev < 0 ? ev + 1 : 0); }
}

Status choose_best_conditioning_gauge(const GaussianScalar& in, const Config& cfg,
                                      ConditioningGauge& out) noexcept {
    out = {};
    if (!valid(in) || !valid_cfg(cfg)) return Status::InvalidInput;
    const float mag = std::max(std::fabs(in.mean), in.sigma);
    if (!(mag > 0.0f) || !std::isfinite(mag)) return Status::InvalidInput;
    const int current = std::ilogb(mag);
    if (current == FP_ILOGB0 || current == FP_ILOGBNAN) return Status::InvalidInput;
    const long long desired = static_cast<long long>(cfg.preferredBinaryExponent) - current;
    int ev = static_cast<int>(std::clamp<long long>(desired, cfg.minConditioningEv, cfg.maxConditioningEv));
    while (ev != 0 && !exact_state_roundtrip(in, ev)) ev = move_toward_zero(ev);
    if (!exact_state_roundtrip(in, ev)) return Status::RangeFailure;
    const float mapped = std::scalbn(mag, ev);
    if (!std::isfinite(mapped) || mapped <= 0.0f) return Status::RangeFailure;
    out.ev = ev;
    out.targetReached = (std::ilogb(mapped) == cfg.preferredBinaryExponent);
    return Status::Ok;
}

Status choose_common_conditioning_gauge(std::span<const GaussianScalar> values, const Config& cfg,
                                        ConditioningGauge& out) noexcept {
    out = {};
    if (values.empty() || !valid_cfg(cfg)) return Status::InvalidInput;
    float maxMag = 0.0f;
    for (const auto& v : values) {
        if (!valid(v)) return Status::InvalidInput;
        maxMag = std::max(maxMag, std::max(std::fabs(v.mean), v.sigma));
    }
    if (!(maxMag > 0.0f) || !std::isfinite(maxMag)) return Status::InvalidInput;
    const int current = std::ilogb(maxMag);
    if (current == FP_ILOGB0 || current == FP_ILOGBNAN) return Status::InvalidInput;
    const long long desired = static_cast<long long>(cfg.preferredBinaryExponent) - current;
    int ev = static_cast<int>(std::clamp<long long>(desired, cfg.minConditioningEv, cfg.maxConditioningEv));
    auto domain_exact = [&](int e) {
        for (const auto& v : values) if (!exact_state_roundtrip(v, e)) return false;
        return true;
    };
    while (ev != 0 && !domain_exact(ev)) ev = move_toward_zero(ev);
    if (!domain_exact(ev)) return Status::RangeFailure;
    const float mapped = std::scalbn(maxMag, ev);
    if (!std::isfinite(mapped) || mapped <= 0.0f) return Status::RangeFailure;
    out.ev = ev;
    out.targetReached = (std::ilogb(mapped) == cfg.preferredBinaryExponent);
    return Status::Ok;
}

Status condition_exact(const GaussianScalar& in, const ConditioningGauge& gauge,
                       ConditionedScalar& out) noexcept {
    out = {};
    if (!valid(in) || gauge.ev < -120 || gauge.ev > 120) return Status::InvalidInput;
    const float m = std::scalbn(in.mean, gauge.ev);
    const float s = std::scalbn(in.sigma, gauge.ev);
    if (!std::isfinite(m) || !std::isfinite(s) || !(s > 0.0f)) return Status::RangeFailure;
    if (in.mean != 0.0f && m == 0.0f) return Status::RangeFailure;
    if (!exact_state_roundtrip(in, gauge.ev)) return Status::RangeFailure;
    out.value = {m,s};
    out.gauge = gauge;
    return Status::Ok;
}

Status decondition_exact(const ConditionedScalar& in, GaussianScalar& out) noexcept {
    out = {};
    if (!valid(in.value) || in.gauge.ev < -120 || in.gauge.ev > 120) return Status::InvalidInput;
    const float m = std::scalbn(in.value.mean, -in.gauge.ev);
    const float s = std::scalbn(in.value.sigma, -in.gauge.ev);
    if (!std::isfinite(m) || !std::isfinite(s) || !(s > 0.0f)) return Status::RangeFailure;
    if (in.value.mean != 0.0f && m == 0.0f) return Status::RangeFailure;
    out = {m,s};
    return Status::Ok;
}

Status shift_truthrange_for_conditioning(const TruthRangeInterval& in, int ev,
                                         TruthRangeInterval& out) noexcept {
    if (std::isnan(in.lowerEv) || std::isnan(in.upperEv) || in.lowerEv > in.upperEv)
        return Status::InvalidInput;
    out = {shift_bound(in.lowerEv, ev), shift_bound(in.upperEv, ev)};
    return Status::Ok;
}
Status unshift_truthrange_after_conditioning(const TruthRangeInterval& in, int ev,
                                             TruthRangeInterval& out) noexcept {
    return shift_truthrange_for_conditioning(in, -ev, out);
}

double snr_abs(const GaussianScalar& x) noexcept {
    if (!valid(x)) return std::numeric_limits<double>::quiet_NaN();
    return std::fabs(static_cast<double>(x.mean))/static_cast<double>(x.sigma);
}

double standardized_residual(float sample, const GaussianScalar& model) noexcept {
    if (!std::isfinite(sample) || !valid(model)) return std::numeric_limits<double>::quiet_NaN();
    return (static_cast<double>(sample)-static_cast<double>(model.mean))/static_cast<double>(model.sigma);
}

CandidateDecision decide_candidate(const AdmissionEvidence& e) noexcept {
    if (e.parametersFrozen && e.independentHeldoutEvidence && e.scalarErrorGatesPass && e.topologyGatesPass)
        return CandidateDecision::CandidateAdmitted;
    return CandidateDecision::CandidateRejected;
}
const char* decision_name(CandidateDecision d) noexcept {
    switch(d) {
        case CandidateDecision::ExactReparameterization: return "EXACT_REPARAMETERIZATION";
        case CandidateDecision::CandidateRejected: return "CANDIDATE_REJECTED";
        case CandidateDecision::CandidateAdmitted: return "CANDIDATE_ADMITTED";
    }
    return "UNKNOWN";
}
} // namespace truthraw::conditioning::v1
