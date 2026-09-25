#include "truthnegative_authority_aware_neighborhood_v0_1.h"
#include <algorithm>
#include <cmath>
namespace truthraw::truthnegative_authority_aware_neighborhood::v0_1 {
bool estimate(const Input&i,Result&o) noexcept{o={};if(!std::isfinite(i.center)||!i.centerVarianceKnown||!std::isfinite(i.centerVariance)||i.centerVariance<=0)return false;
 double sw=0,swx=0;for(const auto&s:i.neighbors){if(!std::isfinite(s.value)||!std::isfinite(s.spatialDistance)||s.spatialDistance<0)continue;
  if(!s.sameChannel||!s.sameObject||s.censorBoundary||s.authority==SampleAuthority::Unknown||s.authority==SampleAuthority::Censored||!s.varianceKnown||!std::isfinite(s.variance)||s.variance<=0)continue;
  const double combined=i.centerVariance+s.variance;const double delta=s.value-i.center;
  const double z2=delta*delta/combined;
  // Bilateral-like statistical compatibility: radiometric similarity is
  // normalized by admitted uncertainty, not by an arbitrary RGB threshold.
  if(z2>4.0)continue;
  const double spatial=1.0/(1.0+s.spatialDistance*s.spatialDistance);
  const double compatibility=std::exp(-0.5*z2);
  const double w=spatial*compatibility/s.variance;
  sw+=w;swx+=w*s.value;o.contributors++;
 }
 if(o.contributors<2||sw<=0)return true;
 o.estimate=swx/sw;o.effectiveWeight=sw;o.valid=std::isfinite(o.estimate);return true;
}
}
