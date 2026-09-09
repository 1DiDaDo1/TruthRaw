#include "truthrange_real_latent_v0_5.h"
#include "truthraw/core.h"
#include <iomanip>
#include <iostream>
#include <string>

int main(int argc,char**argv){
    if(argc<2){std::cerr<<"usage: truthrange_real_latent_v0_5_cli file.dng [radius] [tile]\n";return 2;}
    int radius=argc>2?std::stoi(argv[2]):40;int tile=argc>3?std::stoi(argv[3]):256;
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction;
    truthraw_v05::RealLatentSummaryV05 s;std::vector<truthraw_v05::BackendAnchorV05> anchors;
    auto st=truthraw_v05::run_real_latent_bridge_v0_5(argv[1],reconstruction,s,&anchors,radius,tile);
    if(!st){std::cerr<<"ERROR: "<<st.message<<"\n";return 1;}
    std::cout<<std::setprecision(17);
    std::cout<<"{\n";
    std::cout<<"\"width\":"<<s.width<<",\"height\":"<<s.height<<",\"iso\":"<<s.iso<<",\n";
    std::cout<<"\"stage2_parity_max_abs\":"<<s.stage2ParityMaxAbs<<",\"stage2_float_bits_sum\":"<<s.stage2FloatBitsSum<<",\"stage2_float_bits_xor\":"<<s.stage2FloatBitsXor<<",\n";
    std::cout<<"\"measured_cfa_reinjection_max_abs\":"<<s.measuredCfaReinjectionMaxAbs<<",\"self_gauge_L0\":"<<s.selfGaugeL0<<",\"self_gauge_id\":\""<<s.selfGaugeId<<"\",\n";
    std::cout<<"\"anchors_by_runtime_role\":{"<<"\"R\":"<<s.anchorsByRole[0]<<",\"G1\":"<<s.anchorsByRole[1]<<",\"G2\":"<<s.anchorsByRole[2]<<",\"B\":"<<s.anchorsByRole[3]<<"},\"anchors_skipped_source_clip\":"<<s.anchorsSkippedSourceClip<<",\n";
    std::cout<<"\"anchor_p50_min\":"<<s.anchorP50Min<<",\"anchor_p50_max\":"<<s.anchorP50Max<<",\"anchor_p95_min\":"<<s.anchorP95Min<<",\"anchor_p95_max\":"<<s.anchorP95Max<<",\n";
    std::cout<<"\"dense\":{\"total_rgb_entries\":"<<s.dense.totalRgbEntries<<",\"measured_uncensored\":"<<s.dense.measuredUncensored<<",\"measured_high_censored\":"<<s.dense.measuredHighCensored<<",\"reconstructed_proxy_valid\":"<<s.dense.reconstructedProxyValid<<",\"reconstructed_proxy_unresolved\":"<<s.dense.reconstructedProxyUnresolved<<",\"truthrange_finite_estimate\":"<<s.dense.truthrangeFiniteEstimate<<",\"dark_p95_lower_infinity\":"<<s.dense.darkP95LowerInfinity<<",\"bright_evidence_upper_infinity\":"<<s.dense.brightEvidenceUpperInfinity<<",\"estimate_ev_min\":"<<s.dense.estimateEvMin<<",\"estimate_ev_max\":"<<s.dense.estimateEvMax<<"},\n";
    std::cout<<"\"probe_anchors\":[";
    bool firstProbe=true;
    for(int wantedRole=0; wantedRole<4; ++wantedRole){
        int emittedForRole=0;
        for(const auto& a:anchors){
            const int rr=truthraw_v05::role_runtime_code_v0_5(a.role);
            if(rr!=wantedRole) continue;
            if(!firstProbe) std::cout<<",";
            firstProbe=false;
            std::cout<<"{\"y\":"<<a.y<<",\"x\":"<<a.x<<",\"role\":\""<<truthraw_v05::role_name_v0_5(a.role)<<"\",\"runtime_role\":"<<rr<<",\"pred\":"<<a.predictedHidden<<",\"sigma\":"<<a.sigmaFeature<<",\"snr\":"<<a.snr<<",\"p50\":"<<a.p50Abs<<",\"p95\":"<<a.p95Abs<<",\"features\":[";
            for(int k=0;k<18;++k){if(k)std::cout<<",";std::cout<<a.features[std::size_t(k)];}
            std::cout<<"]}";
            if(++emittedForRole==2) break;
        }
    }
    std::cout<<"]\n}\n";
    return 0;
}
