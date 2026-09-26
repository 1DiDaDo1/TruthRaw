#include "truthnegative_appearance_highlight_headroom_sweep_v0_2.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace sweep =
    truthraw::truthnegative_appearance_highlight_headroom_sweep::v0_2;
namespace detail =
    truthraw::truthnegative_appearance_highlight_detail_audit::v0_1;

#define R(x) do{if(!(x))throw std::runtime_error(#x);}while(0)

static detail::Report report(
    std::uint64_t atPeak,
    std::uint64_t collapsed,
    double mappedGrad,
    bool noHeadroom){
    detail::Report r{};
    r.auditSha256[0]=static_cast<std::uint8_t>(collapsed+1u);
    r.sampleCount=6u;
    r.mappedAtPeak=atPeak;
    r.sourceCensored=0u;
    r.gamutOrDisplayClamp=atPeak;
    r.sourceDistinctAdjacentPairs=7u;
    r.peakCollapsedDistinctAdjacentPairs=collapsed;
    r.brightPeakCollapsedDistinctAdjacentPairs=collapsed;
    r.sourceAbsGradientSum=480.0;
    r.mappedAbsGradientSum=mappedGrad;
    r.noHighlightHeadroom=noHeadroom;
    r.mappedPeakCollapseObserved=collapsed>0u;
    return r;
}

int main(){
    sweep::Input in{};
    in.binding.sourceEvidenceSha256[0]=1u;
    in.binding.scientificMasterSha256[0]=2u;
    in.binding.authorityFieldSha256[0]=3u;
    in.binding.truthNegativeStateSha256[0]=4u;
    in.width=3u;
    in.height=2u;

    const char* ids[]={
        "baseline_100_100","shoulder_90_100",
        "shoulder_80_100","shoulder_70_100"};
    const double knees[]={100.0,90.0,80.0,70.0};
    const std::uint64_t peaks[]={4u,2u,1u,0u};
    const std::uint64_t collapsed[]={3u,1u,0u,0u};
    const double grads[]={140.0,300.0,400.0,420.0};

    for(int i=0;i<4;++i){
        sweep::VariantInput v{};
        v.id=ids[i];
        v.referenceWhiteNits=knees[i];
        v.peakNits=100.0;
        v.detailAudit=report(
            peaks[i],collapsed[i],grads[i],i==0);
        v.appearanceStateSha256[0]=
            static_cast<std::uint8_t>(20+i);
        v.belowKneeSampleCount=static_cast<std::uint64_t>(2+i);
        v.belowKneeMappedLuminanceChanged=0u;
        in.variants.push_back(v);
    }

    sweep::Report out{};
    R(sweep::run(in,out));
    R(out.variants.size()==4u);
    R(out.baselineIsFirst);
    R(out.sourceIdentityConsistent);
    R(out.sourceGradientConsistent);
    R(out.sourceCensorCountConsistent);
    R(out.lowerRangePreservationAudited);
    R(!out.automaticWinnerSelected);
    R(!out.sourceSceneMutated);
    R(!out.createsNewEvidence);
    R(!out.scientificWritebackAllowed);
    R(out.variants[0].collapseReductionVsBaseline==0.0);
    R(out.variants[2].collapseReductionVsBaseline==1.0);
    R(out.variants[3].belowKneeMappedLuminanceChanged==0u);
    R(out.json.find("\"automatic_winner_selected\":false")!=std::string::npos);

    auto bad=in;
    bad.variants[2].detailAudit.sourceAbsGradientSum=479.0;
    sweep::Report fail{};
    R(!sweep::run(bad,fail));

    std::cout<<"AppearanceHighlightHeadroomSweep/0.2 PASS\n";
}
