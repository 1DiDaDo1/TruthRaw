#include "precision_policy_v0_1.h"

#include <algorithm>

namespace truthraw_precision_v01 {

Matrix3dV01 multiply_v0_1(const Matrix3dV01& a, const Matrix3dV01& b) {
    Matrix3dV01 out{};
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            KahanSum64V01 acc;
            for (int k = 0; k < 3; ++k) {
                acc.add(a.at(r,k) * b.at(k,c));
            }
            out.at(r,c) = acc.value();
        }
    }
    return out;
}

Matrix3dV01 transpose_v0_1(const Matrix3dV01& a) {
    Matrix3dV01 out{};
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) out.at(r,c) = a.at(c,r);
    }
    return out;
}

bool propagate_full_covariance_v0_1(
    const Matrix3dV01& transform,
    const Covariance3dV01& input,
    Covariance3dV01& out) {

    out = Covariance3dV01{};
    if (!input.fullyKnown()) return false;

    Matrix3dV01 c{};
    c.v = input.v;
    const Matrix3dV01 temp = multiply_v0_1(transform, c);
    const Matrix3dV01 propagated = multiply_v0_1(temp, transpose_v0_1(transform));

    out.v = propagated.v;
    out.knownMask = Covariance3dV01::fullMask;
    // A linear transform preserves PSD when the input covariance is PSD.
    out.psdCertified = input.psdCertified;
    return true;
}

double mean_float64_v0_1(const double* values, std::size_t count) {
    if (!values || count == 0) return std::numeric_limits<double>::quiet_NaN();
    KahanSum64V01 acc;
    for (std::size_t i = 0; i < count; ++i) acc.add(values[i]);
    return acc.value() / static_cast<double>(count);
}

bool precision_policy_authority_invariant_v0_1(const PrecisionPolicyV01& p) {
    return p.exactIntegerEvidenceRequired
        && p.calibrationUsesFloat64
        && p.covarianceUsesFloat64
        && p.higherPrecisionIsReferenceOnly
        && !p.mayChangeEvidenceAuthority;
}

} // namespace truthraw_precision_v01
