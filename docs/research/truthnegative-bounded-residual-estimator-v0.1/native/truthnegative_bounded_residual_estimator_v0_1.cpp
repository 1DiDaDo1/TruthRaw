#include "truthnegative_bounded_residual_estimator_v0_1.h"
#include <algorithm>
#include <cmath>
namespace truthraw::truthnegative_bounded_residual_estimator::v0_1 {
bool estimate(const Input&i,Result&o) noexcept{o={};if(!std::isfinite(i.observation)||!std::isfinite(i.localEstimate)||!std::isfinite(i.sigma)||i.sigma<0)return false;o.output=i.observation;o.residual=i.observation-i.localEstimate;o.retainedResidual=o.residual;
 if(!i.sigmaKnown||i.sigma<=0||i.admission.decision!=gate::Decision::EligibleNoiseResidual||i.admission.structureProtected)return true;
 const double z=std::abs(o.residual)/i.sigma;
 // Residuals beyond two admitted sigmas are conservatively treated as possible
 // scene structure/outliers, not noise to erase.
 if(z>2.0)return true;
 const double cap=std::clamp(i.admission.maxSuppressionFraction,0.0,0.75);
 // Taper suppression to zero at 2 sigma; never flatten even a perfect flat-field residual.
 const double confidence=std::clamp(1.0-z/2.0,0.0,1.0);
 o.suppressionFraction=cap*confidence;
 o.removedResidual=o.residual*o.suppressionFraction;
 o.retainedResidual=o.residual-o.removedResidual;
 o.output=i.localEstimate+o.retainedResidual;
 o.applied=o.suppressionFraction>0;
 return std::isfinite(o.output);
}
}
