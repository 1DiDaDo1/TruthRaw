#include "reconstruction_precision_reference_v0_3.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

using namespace truthraw_precision_v03;

int main() {
    // Real 7x7 Stage-2 fixture around global CFA coordinate (x=3623,y=95)
    // from MotionCam Direct-CFA IMG_260816_134122_304_005.dng.
    // source SHA-256: 04068eabc681f241b9fa102e46b27b964a636be13361e1d8bacabe0db60471e5
    // GainMaps in this source are unity, so this fixture demonstrates that the
    // precision hazard is intrinsic to the branch-sensitive reconstruction and
    // is not dependent on the HONOR vendor non-unity GainMap family.
    const float f32[49] = {
        0x1.5e3b44p-5f,0x1.459674p-4f,0x1.cd47cep-6f,0x1.49db92p-4f,0x1.a291c0p-5f,0x1.a7cc3ap-4f,0x1.338534p-5f,
        0x1.08cf26p-4f,0x1.004456p-5f,0x1.9a06f0p-5f,0x1.9a06f0p-5f,0x1.c4bd00p-4f,0x1.ef730ep-5f,0x1.c4bd00p-4f,
        0x1.226f96p-6f,0x1.2d0902p-5f,0x1.004456p-6f,0x1.8af9aap-5f,0x1.cd47cep-6f,0x1.78d3e6p-4f,0x1.55b074p-5f,
        0x1.e6e83ep-5f,0x1.449ad4p-6f,0x1.3c1004p-5f,0x1.88f152p-6f,0x1.806682p-5f,0x1.ef730ep-6f,0x1.ef730ep-5f,
        0x1.cd47cep-6f,0x1.f1748ep-5f,0x1.77dbb2p-6f,0x1.8af9aap-5f,0x1.55b074p-6f,0x1.b5acdep-5f,0x1.ab1c90p-6f,
        0x1.c4bd00p-5f,0x1.004456p-5f,0x1.159f5ep-4f,0x1.88f152p-6f,0x1.bc3230p-5f,0x1.bc3230p-6f,0x1.08cf26p-4f,
        0x1.ab1c90p-6f,0x1.2bf7bap-4f,0x1.9a06f0p-6f,0x1.014486p-4f,0x1.55b074p-6f,0x1.e8ea50p-5f,0x1.1159f6p-5f
    };
    const double f64[49] = {
        0x1.5e3b4300cd038p-5,0x1.4596736c6b045p-4,0x1.cd47cee1d5d2ap-6,0x1.49db9250767e2p-4,0x1.a291c077975b9p-5,0x1.a7cc39e972f4ep-4,0x1.338534968e8c7p-5,
        0x1.08cf262c50156p-4,0x1.0044567d76ca6p-5,0x1.9a06f0c8be109p-5,0x1.9a06f0c8be109p-5,0x1.c4bcff32fc87ap-4,0x1.ef730d9d3afebp-5,0x1.c4bcff32fc87ap-4,
        0x1.226f9538dbf66p-6,0x1.2d0901cd29083p-5,0x1.0044567d76ca6p-6,0x1.8af9a966257efp-5,0x1.cd47cee1d5d2ap-6,0x1.78d3e61cf4b98p-4,0x1.55b07351f3b87p-5,
        0x1.e6e83dee61b3ap-5,0x1.449ad3f441227p-6,0x1.3c10044567d77p-5,0x1.88f1516b0b7a8p-6,0x1.806681bc322f8p-5,0x1.ef730d9d3afebp-6,0x1.ef730d9d3afebp-5,
        0x1.cd47cee1d5d2ap-6,0x1.f1748ec738e94p-5,0x1.77dbb20d58e48p-6,0x1.8af9a966257efp-5,0x1.55b07351f3b87p-6,0x1.b5acde4e98409p-5,0x1.ab1c902670a69p-6,
        0x1.c4bcff32fc87ap-5,0x1.0044567d76ca6p-5,0x1.159f5db29605ep-4,0x1.88f1516b0b7a8p-6,0x1.bc322f84233cap-5,0x1.bc322f84233cap-6,0x1.08cf262c50156p-4,
        0x1.ab1c902670a69p-6,0x1.2bf7ba142629cp-4,0x1.9a06f0c8be109p-6,0x1.0144852bb3682p-4,0x1.55b07351f3b87p-6,0x1.e8ea50ff21f5cp-5,0x1.1159f5db29606p-5
    };

    ReconstructionTraceV03 t32, t64, promotedTrace;
    std::vector<float> out32;
    std::vector<double> out64, promotedOut;
    std::vector<double> promoted(49);
    for (int i=0;i<49;++i) promoted[static_cast<std::size_t>(i)] = static_cast<double>(f32[i]);

    assert(v47i_edge_aware_reconstruct_f32_v0_3(f32,7,7,3620,92,3623,95,1,1,truthraw::CfaPattern::BGGR,out32,&t32));
    assert(v47i_edge_aware_reconstruct_f64_v0_3(f64,7,7,3620,92,3623,95,1,1,truthraw::CfaPattern::BGGR,out64,&t64));
    assert(v47i_edge_aware_reconstruct_f64_v0_3(promoted.data(),7,7,3620,92,3623,95,1,1,truthraw::CfaPattern::BGGR,promotedOut,&promotedTrace));

    const std::size_t center = 3u*7u+3u;
    assert(t32.greenDirection[center] == static_cast<std::uint8_t>(DirectionDecisionV03::WeightedBlend));
    assert(t64.greenDirection[center] == static_cast<std::uint8_t>(DirectionDecisionV03::Horizontal));
    assert(promotedTrace.greenDirection[center] == static_cast<std::uint8_t>(DirectionDecisionV03::WeightedBlend));

    // At global BGGR coordinate (odd,odd) red is the measured CFA component.
    assert(t32.measuredChannelViolations == 0);
    assert(t64.measuredChannelViolations == 0);
    assert(promotedTrace.measuredChannelViolations == 0);
    assert(out32[0] == f32[center]);
    assert(out64[0] == f64[center]);

    const double greenEndToEnd = std::abs(static_cast<double>(out32[1]) - out64[1]);
    const double greenArithmeticOnly = std::abs(static_cast<double>(out32[1]) - promotedOut[1]);
    assert(greenEndToEnd > 2.0e-4);
    assert(greenEndToEnd < 2.5e-4);
    assert(greenArithmeticOnly < 1.0e-6);

    std::cout << "test_motioncam_direct_reconstruction_precision_fixture_v0_3 PASS"
              << " green_end_to_end=" << greenEndToEnd
              << " green_arithmetic_only=" << greenArithmeticOnly
              << " f32_direction=" << int(t32.greenDirection[center])
              << " f64_direction=" << int(t64.greenDirection[center])
              << "\n";
    return 0;
}
