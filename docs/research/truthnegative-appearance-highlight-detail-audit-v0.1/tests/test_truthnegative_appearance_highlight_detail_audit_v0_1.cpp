#include "truthnegative_appearance_highlight_detail_audit_v0_1.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace audit =
    truthraw::truthnegative_appearance_highlight_detail_audit::v0_1;

#define R(x) do{if(!(x))throw std::runtime_error(#x);}while(0)

int main(){
    audit::Input input{};
    input.binding.sourceEvidenceSha256[0]=1u;
    input.binding.scientificMasterSha256[0]=2u;
    input.binding.authorityFieldSha256[0]=3u;
    input.binding.truthNegativeStateSha256[0]=4u;
    input.binding.appearanceStateSha256[0]=5u;
    input.width=3u;
    input.height=2u;
    input.tileEdge=2u;
    input.displayReferenceWhiteNits=100.0;
    input.displayPeakNits=100.0;
    input.samples={
        {50.0,  50.0,  false,false},
        {100.0,100.0, false,false},
        {120.0,100.0, false,false},
        {140.0,100.0, false,false},
        {200.0,100.0, false,false},
        {80.0,  80.0,  false,false},
    };

    audit::Report out{};
    R(audit::run(input,out));
    R(out.sampleCount==6u);
    R(out.sourceAboveReferenceWhite==3u);
    R(out.mappedAtPeak==4u);
    R(out.sourceCensored==0u);
    R(out.gamutOrDisplayClamp==0u);
    R(out.adjacentPairs==7u);
    R(out.sourceDistinctAdjacentPairs==7u);
    R(out.peakCollapsedDistinctAdjacentPairs==3u);
    R(out.brightPeakCollapsedDistinctAdjacentPairs==3u);
    R(std::abs(out.sourceAbsGradientSum-480.0)<1.0e-12);
    R(std::abs(out.mappedAbsGradientSum-140.0)<1.0e-12);
    R(std::abs(out.collapsedSourceAbsGradientSum-180.0)<1.0e-12);
    R(std::abs(out.maxCollapsedSourceAbsGradient-100.0)<1.0e-12);
    R(out.noHighlightHeadroom);
    R(out.mappedPeakCollapseObserved);
    R(!out.sourceSceneMutated);
    R(!out.createsNewEvidence);
    R(!out.scientificWritebackAllowed);
    R(out.tiles.size()==2u);
    R(out.json.find(
        "\"schema\":\"D.RAW/TruthNegative/"
        "AppearanceHighlightDetailAudit/0.1\"")!=std::string::npos);
    R(out.json.find(
        "\"no_highlight_headroom\":true")!=std::string::npos);
    R(out.json.find(
        "\"mapped_peak_collapse_observed\":true")!=std::string::npos);

    auto invalid=input;
    invalid.binding.appearanceStateSha256={};
    audit::Report failed{};
    R(!audit::run(invalid,failed));

    std::cout
        <<"AppearanceHighlightDetailAudit/0.1 PASS "
        <<"collapsed="<<out.peakCollapsedDistinctAdjacentPairs
        <<" sourceGrad="<<out.sourceAbsGradientSum
        <<" mappedGrad="<<out.mappedAbsGradientSum
        <<"\n";
}
