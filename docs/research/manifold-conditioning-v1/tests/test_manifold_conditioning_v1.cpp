#include "manifold_conditioning_v1.h"
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <cstdlib>
#include <limits>
#include <random>
using namespace truthraw::conditioning::v1;
#define CHECK(x) do { if(!(x)) { std::cerr << "CHECK failed: " #x " at " << __FILE__ << ":" << __LINE__ << "\n"; return 2; } } while(0)
static std::uint32_t bits(float x){ return std::bit_cast<std::uint32_t>(x); }
int main(){
    Config cfg;
    std::mt19937 rng(20260910u);
    std::uniform_real_distribution<float> mant(0.5f,1.0f);
    std::uniform_int_distribution<int> expo(-80,80);
    std::uniform_real_distribution<float> ratio(-8.f,8.f);
    std::size_t tested=0, exact=0;
    double maxSNR=0.0, maxZ=0.0;
    for(int i=0;i<250000;i++){
        float sigma=std::scalbn(mant(rng),expo(rng));
        float mean=sigma*ratio(rng);
        if(!std::isfinite(mean)||!std::isfinite(sigma)||!(sigma>0)) continue;
        GaussianScalar a{mean,sigma}; ConditioningGauge g;
        CHECK(choose_best_conditioning_gauge(a,cfg,g)==Status::Ok);
        ConditionedScalar c; if(condition_exact(a,g,c)!=Status::Ok) continue;
        GaussianScalar b; CHECK(decondition_exact(c,b)==Status::Ok);
        tested++;
        if(bits(a.mean)==bits(b.mean) && bits(a.sigma)==bits(b.sigma)) exact++;
        const double s0=snr_abs(a), s1=snr_abs(c.value);
        maxSNR=std::max(maxSNR,std::fabs(s0-s1));
        const float sample=a.mean + 0.75f*a.sigma;
        const float sampleC=std::scalbn(sample,g.ev);
        maxZ=std::max(maxZ,std::fabs(standardized_residual(sample,a)-standardized_residual(sampleC,c.value)));
    }
    CHECK(tested>200000);
    CHECK(exact==tested);
    CHECK(maxSNR < 1e-6);
    CHECK(maxZ < 2e-5);

    // Broad IEEE-754 fuzz: successful exact-conditioning paths must round-trip bit-exactly.
    std::uniform_int_distribution<std::uint32_t> rawbits(0u,0xffffffffu);
    std::size_t fuzzValid=0, fuzzSuccess=0, fuzzRangeReject=0;
    for(int i=0;i<1000000;i++){
        float m=std::bit_cast<float>(rawbits(rng));
        float s=std::bit_cast<float>(rawbits(rng) & 0x7fffffffu);
        if(!std::isfinite(m)||!std::isfinite(s)||!(s>0.0f)) continue;
        fuzzValid++;
        GaussianScalar a{m,s}; ConditioningGauge g;
        CHECK(choose_best_conditioning_gauge(a,cfg,g)==Status::Ok);
        ConditionedScalar c;
        auto cs=condition_exact(a,g,c);
        if(cs==Status::RangeFailure){ fuzzRangeReject++; continue; }
        CHECK(cs==Status::Ok);
        GaussianScalar b; CHECK(decondition_exact(c,b)==Status::Ok);
        CHECK(bits(a.mean)==bits(b.mean)); CHECK(bits(a.sigma)==bits(b.sigma));
        fuzzSuccess++;
    }
    CHECK(fuzzValid>400000); CHECK(fuzzSuccess>300000);

    // Spatial-interaction rule: all operands in one interaction domain share one gauge.
    GaussianScalar tile[4]={{0.001f,0.0002f},{0.2f,0.01f},{2.0f,0.2f},{-0.05f,0.005f}};
    ConditioningGauge common;
    CHECK(choose_common_conditioning_gauge(tile,cfg,common)==Status::Ok);
    for (const auto& v: tile) {
        ConditionedScalar cv; GaussianScalar rv;
        CHECK(condition_exact(v,common,cv)==Status::Ok);
        CHECK(decondition_exact(cv,rv)==Status::Ok);
        CHECK(bits(v.mean)==bits(rv.mean) && bits(v.sigma)==bits(rv.sigma));
    }
    ConditioningGauge g0,g1;
    CHECK(choose_best_conditioning_gauge(tile[0],cfg,g0)==Status::Ok);
    CHECK(choose_best_conditioning_gauge(tile[2],cfg,g1)==Status::Ok);
    CHECK(g0.ev != g1.ev);

    // Zero-line / TruthRange gauge: infinities are censor bounds and must survive.
    TruthRangeInterval hi{3.25,std::numeric_limits<double>::infinity()}, shifted{}, restored{};
    CHECK(shift_truthrange_for_conditioning(hi,7,shifted)==Status::Ok);
    CHECK(shifted.lowerEv==10.25 && std::isinf(shifted.upperEv));
    CHECK(unshift_truthrange_after_conditioning(shifted,7,restored)==Status::Ok);
    CHECK(restored.lowerEv==hi.lowerEv && std::isinf(restored.upperEv));

    AdmissionEvidence e{};
    CHECK(decide_candidate(e)==CandidateDecision::CandidateRejected);
    e={true,true,true,false};
    CHECK(decide_candidate(e)==CandidateDecision::CandidateRejected);
    e={true,true,true,true};
    CHECK(decide_candidate(e)==CandidateDecision::CandidateAdmitted);

    GaussianScalar bad{1.0f,std::numeric_limits<float>::quiet_NaN()}; ConditioningGauge gg;
    CHECK(choose_best_conditioning_gauge(bad,cfg,gg)==Status::InvalidInput);

    std::cout << "Manifold Conditioning v1: PASS\n"
              << "roundtrip_tested=" << tested << " exact=" << exact << "\n"
              << "max_snr_delta=" << maxSNR << "\n"
              << "max_standardized_residual_delta=" << maxZ << "\n"
              << "fuzz_valid=" << fuzzValid << " fuzz_success=" << fuzzSuccess << " fuzz_range_reject=" << fuzzRangeReject << "\n"
              << "candidate_fallback=CANDIDATE_REJECTED unless all admission gates pass\n";
}
