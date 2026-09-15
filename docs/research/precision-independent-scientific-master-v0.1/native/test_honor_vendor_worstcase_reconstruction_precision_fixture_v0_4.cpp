#include "reconstruction_precision_reference_v0_3.h"

#include <cassert>
#include <iostream>
#include <vector>

using namespace truthraw_precision_v03;

int main() {
    // Real 7x7 Stage-2 neighbourhood around global CFA coordinate (332,2164)
    // from IMG_BNC_TRUTHRAW20260906_142950_144.dng.
    // source SHA-256: f640875800adf4aeca131dabe0845f10bbda07886396d562ca16324421e9a3ce
    // Located by the full-file branch-divergence audit because it is a stronger
    // local F32/F64 amplification case than the first HONOR fixture.
    const float f32[49] = {
        0x1.a42aacp-3f,0x1.51e20cp-2f,0x1.08d63ap-2f,0x1.9725dap-2f,0x1.d28798p-3f,0x1.0f05f8p-2f,0x1.0ccffep-2f,
        0x1.0ee4d8p-2f,0x1.69c35ep-3f,0x1.2eccdap-2f,0x1.b1fc70p-3f,0x1.99496ap-2f,0x1.0e7d9ep-3f,0x1.9d2c18p-2f,
        0x1.af316cp-3f,0x1.83470ap-2f,0x1.bcc9d4p-3f,0x1.2f2218p-2f,0x1.d36434p-4f,0x1.75ba28p-2f,0x1.4efdfap-3f,
        0x1.498ff2p-2f,0x1.ba25d2p-3f,0x1.4579e8p-2f,0x1.71afa8p-3f,0x1.1ac234p-2f,0x1.7c49cep-3f,0x1.7a933ap-2f,
        0x1.d861dcp-3f,0x1.88a508p-2f,0x1.88c592p-3f,0x1.6ddaf2p-2f,0x1.f907a0p-3f,0x1.1b113ep-2f,0x1.5f7a82p-3f,
        0x1.4aeb82p-2f,0x1.499880p-3f,0x1.8ecbe0p-2f,0x1.972f2cp-3f,0x1.795f60p-2f,0x1.b48594p-3f,0x1.3003c0p-2f,
        0x1.445f70p-3f,0x1.6f51bcp-2f,0x1.027ecep-3f,0x1.61e042p-2f,0x1.5ce072p-3f,0x1.35c390p-2f,0x1.bcb15ep-3f
    };
    const double f64[49] = {
        0x1.a42aa78aac762p-3,0x1.51e20a75feb3bp-2,0x1.08d6378ac1405p-2,0x1.9725da945fef1p-2,0x1.d28795b42aeaap-3,0x1.0f05f7e14fffcp-2,0x1.0ccffdbc5ff51p-2,
        0x1.0ee4d6a8b922dp-2,0x1.69c35ced9cba6p-3,0x1.2eccda3b018fap-2,0x1.b1fc6e438d7f0p-3,0x1.99496b743a3fap-2,0x1.0e7d9db938048p-3,0x1.9d2c1a2da0732p-2,
        0x1.af3169e5ee0dap-3,0x1.83470a702ae9dp-2,0x1.bcc9d118e968fp-3,0x1.2f22182b3bd8dp-2,0x1.d364353601976p-4,0x1.75ba2a5ea91e6p-2,0x1.4efdfa0076c3cp-3,
        0x1.498ff04aff457p-2,0x1.ba25d3f441228p-3,0x1.4579e7e19383ep-2,0x1.71afa6d7284f0p-3,0x1.1ac23309b4a3ap-2,0x1.7c49cee1d5d2bp-3,0x1.7a93377804edfp-2,
        0x1.d861daa63798cp-3,0x1.88a508aecbc08p-2,0x1.88c59107dddaap-3,0x1.6ddaf2e4ddcb5p-2,0x1.f9079d7e78f0bp-3,0x1.1b113e6b81527p-2,0x1.5f7a80f5b8e2ap-3,
        0x1.4aeb80bb138b9p-2,0x1.4998806b54a3fp-3,0x1.8ecbdefe84d2fp-2,0x1.972f2c13c90a7p-3,0x1.795f5ec16631dp-2,0x1.b485941eaa025p-3,0x1.3003be12e823dp-2,
        0x1.445f6e2da4272p-3,0x1.6f51bd338b365p-2,0x1.027ecdb9cad6fp-3,0x1.61e044360cd15p-2,0x1.5ce071fefcab0p-3,0x1.35c38f4d80574p-2,0x1.bcb15d73565f3p-3
    };

    ReconstructionTraceV03 t32, t64;
    std::vector<float> out32;
    std::vector<double> out64;
    assert(v47i_edge_aware_reconstruct_f32_v0_3(f32,7,7,329,2161,332,2164,1,1,truthraw::CfaPattern::BGGR,out32,&t32));
    assert(v47i_edge_aware_reconstruct_f64_v0_3(f64,7,7,329,2161,332,2164,1,1,truthraw::CfaPattern::BGGR,out64,&t64));

    const std::size_t center = 3u*7u+3u;
    assert(t32.greenDirection[center] == static_cast<std::uint8_t>(DirectionDecisionV03::Horizontal));
    assert(t64.greenDirection[center] == static_cast<std::uint8_t>(DirectionDecisionV03::WeightedBlend));
    assert(t32.measuredChannelViolations == 0);
    assert(t64.measuredChannelViolations == 0);

    const auto s = compare_reconstruction_precision_v0_3(out32,t32,out64,t64);
    assert(s.greenDirectionDivergence == 1);
    assert(s.measuredChannelViolationsF32 == 0);
    assert(s.measuredChannelViolationsF64 == 0);
    assert(s.maxAbsError > 3.7e-2);
    assert(s.maxAbsError < 3.8e-2);

    std::cout << "test_honor_vendor_worstcase_reconstruction_precision_fixture_v0_4 PASS"
              << " max_abs=" << s.maxAbsError << "\n";
    return 0;
}
