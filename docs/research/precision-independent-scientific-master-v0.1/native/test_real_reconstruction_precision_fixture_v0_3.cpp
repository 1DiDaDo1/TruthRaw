#include "reconstruction_precision_reference_v0_3.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

using namespace truthraw_precision_v03;

int main() {
    // Real 7x7 Stage-2 fixture around global CFA coordinate (x=1408,y=884)
    // from IMG_BNC_TRUTHRAW20260906_151129_256.dng
    // source SHA-256: f900c9d8911072530d43c532a02328c62ed1e380ab6659c36bc2ed0f2dc9b229
    // F32 fixture byte SHA-256: 42f7f08ccc298780ccf3cf78b0f23ecd56b518fe3322cf5ac8cc21710bfd0194
    // F64 fixture byte SHA-256: 64b4ce7d19aa8312dc19510bd9537a7ac1466b3d706c5c92e78d74cc6b520aa9
    //
    // The two arrays are not new photographic evidence: they are two numerical
    // executions from the same exact source codes/metadata, captured here only
    // to reproduce the downstream precision sensitivity in host CI.
    const float f32[49] = {
        0x1.1287da0000000p+0f, 0x1.2561140000000p+0f, 0x1.185ff00000000p+0f, 0x1.25496e0000000p+0f,
        0x1.1d06900000000p+0f, 0x1.2531c60000000p+0f, 0x1.1ceb580000000p+0f, 0x1.251e480000000p+0f,
        0x1.23cff20000000p+0f, 0x1.2508ce0000000p+0f, 0x1.23bfc40000000p+0f, 0x1.24f3540000000p+0f,
        0x1.23af960000000p+0f, 0x1.24ddda0000000p+0f, 0x1.03e6040000000p+0f, 0x1.2551960000000p+0f,
        0x1.0842aa0000000p+0f, 0x1.2539f20000000p+0f, 0x1.0f4b120000000p+0f, 0x1.25224e0000000p+0f,
        0x1.1521f40000000p+0f, 0x1.2510420000000p+0f, 0x1.23b82a0000000p+0f, 0x1.24fac40000000p+0f,
        0x1.23a7f60000000p+0f, 0x1.24e5440000000p+0f, 0x1.2397c40000000p+0f, 0x1.24cfc60000000p+0f,
        0x1.ea8bac0000000p-1f, 0x1.25421c0000000p+0f, 0x1.f770540000000p-1f, 0x1.252a7a0000000p+0f,
        0x1.02293c0000000p+0f, 0x1.2512d80000000p+0f, 0x1.0801040000000p+0f, 0x1.25023c0000000p+0f,
        0x1.23a0600000000p+0f, 0x1.24ecba0000000p+0f, 0x1.2390260000000p+0f, 0x1.24d7380000000p+0f,
        0x1.237fee0000000p+0f, 0x1.24c1b60000000p+0f, 0x1.d932540000000p-1f, 0x1.2532a00000000p+0f,
        0x1.ddc6120000000p-1f, 0x1.251b000000000p+0f, 0x1.ea11d00000000p-1f, 0x1.2503600000000p+0f,
        0x1.f1030c0000000p-1f
    };
    const double f64[49] = {
        0x1.1287d912c892bp+0, 0x1.256113b3b3b3bp+0, 0x1.185ff1b92e5ebp+0, 0x1.25496d8d8d8d8p+0,
        0x1.1d068f9f9f9fap+0, 0x1.2531c76767676p+0, 0x1.1ceb58a8a8a8ap+0, 0x1.251e484848484p+0,
        0x1.23cff32323232p+0, 0x1.2508cdcdcdcddp+0, 0x1.23bfc4b4b4b4bp+0, 0x1.24f3535353536p+0,
        0x1.23af964646464p+0, 0x1.24ddd8d8d8d8ep+0, 0x1.03e6030dc3d23p+0, 0x1.255197b7b7b7cp+0,
        0x1.0842a7522eadbp+0, 0x1.2539f39393939p+0, 0x1.0f4b10a255fd5p+0, 0x1.25224f6f6f6f7p+0,
        0x1.1521f3551e209p+0, 0x1.2510424242424p+0, 0x1.23b8291919192p+0, 0x1.24fac3c3c3c3cp+0,
        0x1.23a7f5a5a5a5ap+0, 0x1.24e5454545454p+0, 0x1.2397c23232324p+0, 0x1.24cfc6c6c6c6cp+0,
        0x1.ea8badd37fad9p-1, 0x1.25421bbbbbbbcp+0, 0x1.f77054179887cp-1, 0x1.252a79999999ap+0,
        0x1.02293d10f8695p+0, 0x1.2512d77777778p+0, 0x1.080103ded93e3p+0, 0x1.25023c3c3c3c4p+0,
        0x1.23a05f0f0f0f1p+0, 0x1.24ecb9b9b9b9cp+0, 0x1.239026969696ap+0, 0x1.24d7373737374p+0,
        0x1.237fee1e1e1e2p+0, 0x1.24c1b4b4b4b4cp+0, 0x1.d9325692c4f48p-1, 0x1.25329fbfbfbfcp+0,
        0x1.ddc613ff4f202p-1, 0x1.251aff9f9f9fap+0, 0x1.ea11d1aee1553p-1, 0x1.25035f7f7f7f8p+0,
        0x1.f1030d61a2c53p-1
    };

    ReconstructionTraceV03 t32, t64, t64Promoted;
    std::vector<float> out32;
    std::vector<double> out64;
    std::vector<double> promoted(49);
    for (int i = 0; i < 49; ++i) promoted[static_cast<std::size_t>(i)] = static_cast<double>(f32[i]);
    std::vector<double> out64Promoted;

    // 7x7 halo fixture begins at global (1405,881), core is one pixel at (1408,884).
    assert(v47i_edge_aware_reconstruct_f32_v0_3(
        f32, 7, 7, 1405, 881, 1408, 884, 1, 1,
        truthraw::CfaPattern::BGGR, out32, &t32));
    assert(v47i_edge_aware_reconstruct_f64_v0_3(
        f64, 7, 7, 1405, 881, 1408, 884, 1, 1,
        truthraw::CfaPattern::BGGR, out64, &t64));
    assert(v47i_edge_aware_reconstruct_f64_v0_3(
        promoted.data(), 7, 7, 1405, 881, 1408, 884, 1, 1,
        truthraw::CfaPattern::BGGR, out64Promoted, &t64Promoted));

    const std::size_t center = 3u * 7u + 3u;
    assert(t32.greenDirection[center] == static_cast<std::uint8_t>(DirectionDecisionV03::Vertical));
    assert(t64.greenDirection[center] == static_cast<std::uint8_t>(DirectionDecisionV03::WeightedBlend));
    assert(t64Promoted.greenDirection[center] == static_cast<std::uint8_t>(DirectionDecisionV03::Vertical));

    // The measured CFA channel at this BGGR coordinate is blue (channel 2) and
    // must remain an exact copy in both precision routes.
    assert(t32.measuredChannelViolations == 0);
    assert(t64.measuredChannelViolations == 0);
    assert(t64Promoted.measuredChannelViolations == 0);
    assert(out32[2] == f32[center]);
    assert(out64[2] == f64[center]);

    const double redEndToEndError = std::abs(static_cast<double>(out32[0]) - out64[0]);
    const double redArithmeticOnlyError = std::abs(static_cast<double>(out32[0]) - out64Promoted[0]);

    // This is the key regression: a sub-2e-7 Stage-2 perturbation crosses the
    // hard 0.72 directional threshold and is amplified into a ~1.95e-2
    // reconstructed-red difference. By contrast, promoting the exact same F32
    // Stage-2 samples to double does not cross the branch and remains tiny.
    assert(redEndToEndError > 1.9e-2);
    assert(redEndToEndError < 2.0e-2);
    assert(redArithmeticOnlyError < 1.0e-5);

    std::cout << "test_real_reconstruction_precision_fixture_v0_3 PASS"
              << " red_end_to_end=" << redEndToEndError
              << " red_arithmetic_only=" << redArithmeticOnlyError
              << " f32_direction=" << int(t32.greenDirection[center])
              << " f64_direction=" << int(t64.greenDirection[center])
              << "\n";
    return 0;
}
