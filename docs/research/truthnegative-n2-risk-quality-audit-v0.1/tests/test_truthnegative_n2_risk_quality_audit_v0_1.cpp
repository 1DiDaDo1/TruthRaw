#include "truthnegative_n2_risk_quality_audit_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace qa=truthraw::truthnegative_n2_risk_quality_audit::v0_1;
namespace n2=truthraw::truthnegative_n2_candidate_pipeline::v0_1;
#define R(x) do{if(!(x))throw std::runtime_error(#x);}while(0)

int main(){
    constexpr std::uint32_t w=8,h=8;
    const std::size_t pixels=static_cast<std::size_t>(w)*h;
    std::vector<double> a(pixels*3u,0.25);
    std::vector<double> b=a;
    std::vector<std::uint32_t> mask(pixels,0u);

    const auto structureBit=
        1u<<static_cast<std::uint32_t>(n2::PreserveReason::Structure);
    const auto censorBit=
        1u<<static_cast<std::uint32_t>(n2::PreserveReason::Censored);
    mask[3u*w+3u]=structureBit;
    mask[7u*w+7u]=censorBit;

    const std::size_t maxI=4u*w+4u;
    b[maxI*3u+0u]+=0.10;
    b[maxI*3u+1u]+=0.05;
    b[maxI*3u+2u]-=0.02;
    b[(2u*w+2u)*3u+1u]+=0.01;

    qa::Input in{};
    in.aEncodedRgb=a.data();
    in.bEncodedRgb=b.data();
    in.preserveReasonMask=mask.data();
    in.width=w;in.height=h;
    in.sourceX=100u;in.sourceY=200u;
    in.candidateIdentitySha256[0]=1u;

    qa::Result out{};
    R(qa::evaluate(in,out));
    R(out.changedPixels==2u);
    R(out.maxLocalX==4u&&out.maxLocalY==4u);
    R(out.maxSourceX==104u&&out.maxSourceY==204u);
    R(out.pixelMaxAbsDelta.max>0.099);
    R(out.channelAbsDelta[0].max>0.099);
    R(out.channelAbsDelta[1].max>0.049);
    R(out.channelAbsDelta[2].max>0.019);
    R(out.lumaAbsDelta.max>0.0);
    R(out.chromaDelta.max>0.0);
    R(std::abs(out.distanceToStructurePx-std::sqrt(2.0))<1e-12);
    R(out.distanceToCensorOrBoundaryPx>4.0);
    R(out.structureMaskPixels==1u);
    R(out.censorMaskPixels==1u);
    R(out.edgeEnergyRatio>0.0);
    R(out.meanAbsGradientDelta>0.0);
    R(!out.createsNewEvidence&&!out.scientificWritebackAllowed);
    R(std::any_of(
        out.qualitySha256.begin(),out.qualitySha256.end(),
        [](auto v){return v!=0u;}));

    std::cout<<"TruthNegativeN2RiskQualityAudit/0.1 PASS max=("
             <<out.maxSourceX<<","<<out.maxSourceY<<") p99="
             <<out.pixelMaxAbsDelta.p99<<"\n";
}
