#include "truthnegative_center_excluded_neighborhood_v0_2.h"

#include <cmath>
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
    }

    {
        n::Input in{};
        add_pair(in, 2, 0, 0.99, 1.01);
        n::Result out{};
        R(n::estimate(in, out));
        R(!out.valid);
        R(out.scalesAccepted == 0u);
    }

    std::cout
        << "TruthNegativeCenterExcludedNeighborhood/0.2 PASS\n";
}
