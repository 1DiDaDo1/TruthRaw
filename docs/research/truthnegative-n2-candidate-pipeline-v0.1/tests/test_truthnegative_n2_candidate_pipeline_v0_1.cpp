#include "truthnegative_n2_candidate_pipeline_v0_1.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace p=truthraw::truthnegative_n2_candidate_pipeline::v0_1;
namespace n=truthraw::truthnegative_authority_aware_neighborhood::v0_1;
#define R(x) do{if(!(x))throw std::runtime_error(#x);}while(0)

static p::PixelInput flat(){
 p::PixelInput i{};
 i.structure={1.02,1.01,1.00,1.01,1.00,.05,true,false,false,true,1,1};
 i.neighborhood.center=1.02;i.neighborhood.centerVariance=.0025;i.neighborhood.centerVarianceKnown=true;
 i.neighborhood.neighbors={
  {1.01,.0025,1,n::SampleAuthority::Measured,true,true,true,false},
  {1.00,.0025,1,n::SampleAuthority::Measured,true,true,true,false},
  {1.01,.0025,1,n::SampleAuthority::Measured,true,true,true,false},
  {1.00,.0025,1,n::SampleAuthority::Measured,true,true,true,false}};
 return i;
}
int main(){
 p::PixelResult r{};auto i=flat();R(p::evaluatePixel(i,r));R(r.eligible);R(r.neighborhoodValid);R(r.correctionApplied);R(std::abs(r.correction)>0);
 p::Audit a{};R(p::accumulate(r,a));R(a.total==1&&a.corrected==1);
 auto c=flat();c.structure.censored=true;R(p::evaluatePixel(c,r));R(!r.correctionApplied&&r.preserveReason==p::PreserveReason::Censored);R(p::accumulate(r,a));R(a.censoredProtected==1);
 auto e=flat();e.structure.left=.2;e.structure.right=1.8;R(p::evaluatePixel(e,r));R(!r.correctionApplied&&r.preserveReason==p::PreserveReason::Structure);
 std::cout<<"TruthNegativeN2CandidatePipeline/0.1 PASS\n";
}
