#include "truthnegative_n2_full_colour_candidate_v0_1.h"
#include "scientific_master_f64_reconstruction_v0_1.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace fc=truthraw::truthnegative_n2_full_colour_candidate::v0_1;
namespace audit=truthraw::truthnegative_n2_cfa_audit::v0_1;
namespace recon=truthraw::scientific_master_f64_reconstruction_v0_1;

#define R(x) do{if(!(x))throw std::runtime_error(#x);}while(0)

int main(){
    constexpr int w=12,h=12;
    std::vector<float> stage2(static_cast<std::size_t>(w)*h,0.20f);
    for(int y=0;y<h;++y){
        for(int x=0;x<w;++x){
            stage2[static_cast<std::size_t>(y)*w+x]+=
                0.002f*static_cast<float>((x+2*y)%5);
        }
    }

    audit::Result a{};
    a.samplingPeriod=2u;
    a.regionX=0u;a.regionY=0u;
    a.regionWidth=w;a.regionHeight=h;
    a.appearanceGridWidth=w;a.appearanceGridHeight=h;
    a.appearanceGridDerived=true;
    a.appearanceGrid.resize(static_cast<std::size_t>(w)*h);
    a.appearanceGridSha256[0]=1u;

    const auto channel=[](int x,int y){
        static constexpr int bggr[4]={2,1,1,0};
        return bggr[(y&1)*2+(x&1)];
    };
    for(int y=0;y<h;++y){
        for(int x=0;x<w;++x){
            auto& bin=a.appearanceGrid[static_cast<std::size_t>(y)*w+x];
            const auto cc=static_cast<std::size_t>(channel(x,y));
            bin.sampled[cc]=1u;
            if(x>=4&&x<8&&y>=4&&y<8){
                bin.corrected[cc]=1u;
                bin.correctionSum[cc]=-0.001;
            }else{
                bin.protectedCount[cc]=1u;
            }
        }
    }

    recon::ResearchEdgeAwareMeasuredPreservingReconstructionF64 backend;
    fc::Input in{};
    in.stage2=stage2.data();
    in.tileWidth=w;in.tileHeight=h;
    in.globalHx0=0;in.globalHy0=0;
    in.coreX0=3;in.coreY0=3;
    in.coreWidth=6;in.coreHeight=6;
    in.cfa=truthraw::CfaPattern::BGGR;
    in.n2Audit=&a;

    fc::Result out{};
    R(fc::reconstruct(in,backend,out));
    R(out.correctedStage2Sites==16u);
    R(out.baselineCameraRgb.size()==108u);
    R(out.candidateCameraRgb.size()==108u);
    R(out.changedRgbChannels>16u);
    R(out.maxAbsRgbDelta>0.0);
    R(!out.sourceStage2Modified);
    R(!out.scientificWritebackAllowed);
    R(!out.createsNewEvidence);
    R(std::any_of(
        out.candidateIdentitySha256.begin(),
        out.candidateIdentitySha256.end(),
        [](auto v){return v!=0u;}));

    for(std::size_t i=0;i<stage2.size();++i){
        const float expected=0.20f+
            0.002f*static_cast<float>((static_cast<int>(i%w)+
            2*static_cast<int>(i/w))%5);
        R(stage2[i]==expected);
    }

    std::cout<<"TruthNegativeN2FullColourCandidate/0.1 PASS corrected="
             <<out.correctedStage2Sites<<" changedRgb="
             <<out.changedRgbChannels<<"\n";
}
