#include "truthnegative_dense_uncertainty_adapter_v0_1.h"
#include <cmath>
namespace truthraw::truthnegative_noise_state::v0_1 {
bool bindDenseMeasuredSample(const State& state,const truthraw::DenseUncertaintyFieldV03& field,std::size_t pixelIndex,std::size_t channel,bool measured,DenseSampleBinding& out) noexcept{
 out={}; if(!state.finalized||channel>=3||field.width<=0||field.height<=0)return false;
 const auto n=static_cast<std::size_t>(field.width)*static_cast<std::size_t>(field.height);
 if(field.rgb.size()!=3*n||pixelIndex>=n)return false;
 const auto& e=field.rgb[3*pixelIndex+channel]; out.measuredRole=measured;out.topologyCertified=e.topologyCertified;out.source=e.source;
 using S=truthraw::DenseUncertaintySourceV03;
 if(e.source==S::MeasuredHighCensored){
  if(!measured||e.valid||std::isfinite(e.sigmaEquivalent))return false;
  out.noise.authority=state.input.authority;out.noise.sourceCensored=true;return true;
 }
 if(e.source==S::MeasuredNoiseProfileGaussianEquivalent){
  if(!measured||!e.valid||!e.topologyCertified||!std::isfinite(e.sigmaEquivalent)||e.sigmaEquivalent<0||!std::isfinite(e.p95Abs)||e.p95Abs<0)return false;
  out.noise.authority=state.input.authority;out.noise.variance=double(e.sigmaEquivalent)*e.sigmaEquivalent;out.noise.sigma=e.sigmaEquivalent;out.noise.p95Abs=e.p95Abs;out.noise.valid=true;out.varianceAdmitted=true;return true;
 }
 // Critical boundary: reconstructed v5.0g quantile proxies and unresolved
 // entries are not a sensor-noise variance and may not be promoted here.
 if(e.source==S::V5GMeasuredRoleAnchor||e.source==S::V5GLocalMaxTransportProxy||e.source==S::Unresolved){return !measured;}
 return false;
}
}
