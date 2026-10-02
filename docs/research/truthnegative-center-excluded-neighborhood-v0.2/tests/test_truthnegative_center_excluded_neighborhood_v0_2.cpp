#include "truthnegative_center_excluded_neighborhood_v0_2.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace n = truthraw::truthnegative_center_excluded_neighborhood::v0_2;
#define R(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)

static void add_pair(
    n::Input& in,
    int dx,
    int dy,
    double a,
    double b,
    double variance = 0.0025) {
    in.neighbors.push_back({
        a, variance, -dx, -dy, n::SampleAuthority::Measured,
        true, true, true, false, false});
    in.neighbors.push_back({
        b, variance, dx, dy, n::SampleAuthority::Measured,
        true, true, true, false, false});
}

static n::Input flat_multiscale() {
    n::Input in{};
    for (int r : {2, 4, 8}) {
        add_pair(in, r, 0, 0.99, 1.01);
        add_pair(in, 0, r, 1.01, 0.99);
        add_pair(in, r, r, 1.00, 1.01);
        add_pair(in, r, -r, 0.99, 1.00);
    }
    return in;
}

static bool same_double(double a, double b) {
    return std::bit_cast<std::uint64_t>(a) ==
           std::bit_cast<std::uint64_t>(b);
}

static bool exact_result_equal(const n::Result& a, const n::Result& b) {
    return
        same_double(a.estimate,b.estimate) &&
        same_double(a.estimateVariance,b.estimateVariance) &&
        same_double(a.effectiveWeight,b.effectiveWeight) &&
        same_double(
            a.maxDirectionalDisagreementSigma,
            b.maxDirectionalDisagreementSigma) &&
        same_double(
            a.maxCrossScaleDisagreementSigma,
            b.maxCrossScaleDisagreementSigma) &&
        a.admissibleSamples == b.admissibleSamples &&
        a.symmetricPairsConsidered == b.symmetricPairsConsidered &&
        a.symmetricPairsAccepted == b.symmetricPairsAccepted &&
        a.symmetricPairsRejected == b.symmetricPairsRejected &&
        a.scalesConsidered == b.scalesConsidered &&
        a.scalesAccepted == b.scalesAccepted &&
        a.scalesRejected == b.scalesRejected &&
        a.finestAcceptedRadius == b.finestAcceptedRadius &&
        a.coarsestAcceptedRadius == b.coarsestAcceptedRadius &&
        a.valid == b.valid &&
        a.centerExcluded == b.centerExcluded &&
        a.createsNewEvidence == b.createsNewEvidence &&
        a.scientificWritebackAllowed == b.scientificWritebackAllowed;
}

static void require_fixed_parity(const n::Input& in) {
    n::Result generic{};
    n::Diagnostics forceGeneric{};
    R(n::estimate(in,generic,&forceGeneric));

    n::Result fixed{};
    R(n::estimateFixedTopology(in,fixed,nullptr) ==
      n::FixedTopologyOutcome::Success);
    R(exact_result_equal(generic,fixed));

    n::Result routed{};
    R(n::estimate(in,routed));
    R(exact_result_equal(generic,routed));
}

int main() {
    {
        const auto in = flat_multiscale();
        n::Result out{};
        R(n::estimate(in, out));
        R(out.valid);
        R(out.centerExcluded);
        R(out.symmetricPairsAccepted == 12u);
        R(out.scalesAccepted == 3u);
        R(out.finestAcceptedRadius == 2u);
        R(out.coarsestAcceptedRadius == 8u);
        R(out.estimate > 0.99 && out.estimate < 1.01);
        R(!out.createsNewEvidence);
        R(!out.scientificWritebackAllowed);
        require_fixed_parity(in);
    }

    {
        n::Input in{};
        add_pair(in, 2, 2, 0.995, 1.005);
        add_pair(in, 2, -2, 1.004, 0.996);
        n::Result out{};
        R(n::estimate(in, out));
        R(out.valid);
        R(out.scalesAccepted == 1u);
        R(out.symmetricPairsAccepted == 2u);
        require_fixed_parity(in);
    }

    {
        n::Input in{};
        add_pair(in, 2, 0, 0.50, 1.50, 0.0004);
        add_pair(in, 0, 2, 0.995, 1.005, 0.0025);
        add_pair(in, 2, 2, 1.004, 0.996, 0.0025);
        n::Result out{};
        R(n::estimate(in, out));
        R(out.valid);
        R(out.symmetricPairsRejected >= 1u);
        R(out.symmetricPairsAccepted == 2u);
        R(out.estimate > 0.99 && out.estimate < 1.01);
        require_fixed_parity(in);
    }

    {
        n::Input in{};
        add_pair(in, 2, 0, 0.99, 1.01, 0.0025);
        add_pair(in, 0, 2, 1.01, 0.99, 0.0025);
        add_pair(in, 8, 0, 1.29, 1.31, 0.0025);
        add_pair(in, 0, 8, 1.31, 1.29, 0.0025);
        n::Result out{};
        R(n::estimate(in, out));
        R(out.valid);
        R(out.scalesAccepted == 1u);
        R(out.scalesRejected >= 1u);
        R(out.coarsestAcceptedRadius == 2u);
        R(out.estimate > 0.99 && out.estimate < 1.01);
        require_fixed_parity(in);
    }

    {
        n::Input in{};
        add_pair(in, 2, 0, 0.99, 1.01);
        add_pair(in, 0, 2, 1.01, 0.99);
        in.neighbors.push_back({
            1.0, 0.0025, 2, 2, n::SampleAuthority::Censored,
            true, true, true, false, true});
        in.neighbors.push_back({
            1.0, 0.0025, -2, -2, n::SampleAuthority::Unknown,
            true, true, true, false, false});
        n::Result out{};
        R(n::estimate(in, out));
        R(out.valid);
        R(out.admissibleSamples == 4u);
        require_fixed_parity(in);
    }

    {
        n::Input in{};
        add_pair(in, 2, 0, 0.99, 1.01);
        n::Result out{};
        R(n::estimate(in, out));
        R(!out.valid);
        R(out.scalesAccepted == 0u);
        require_fixed_parity(in);
    }

    // Duplicate geometry must remain fail-closed in exactly the legacy way.
    {
        n::Input in{};
        add_pair(in,2,0,0.99,1.01);
        in.neighbors.push_back({
            1.0,0.0025,-2,0,n::SampleAuthority::Measured,
            true,true,true,false,false});
        add_pair(in,0,2,1.01,0.99);
        require_fixed_parity(in);
    }

    // An admissible radius outside {2,4,8} is not silently reinterpreted.
    // The fast path declines it and estimate() falls back to the generic route.
    {
        n::Input in{};
        add_pair(in,6,0,0.99,1.01);
        add_pair(in,0,6,1.01,0.99);
        n::Result fixed{};
        R(n::estimateFixedTopology(in,fixed,nullptr) ==
          n::FixedTopologyOutcome::NotApplicable);
        n::Result generic{};
        n::Diagnostics forceGeneric{};
        R(n::estimate(in,generic,&forceGeneric));
        n::Result routed{};
        R(n::estimate(in,routed));
        R(exact_result_equal(generic,routed));
    }

    std::cout
        << "TruthNegativeCenterExcludedNeighborhood/0.2 PASS\n";
}
