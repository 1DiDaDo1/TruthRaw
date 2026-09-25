#include "truthnegative_structure_preservation_gate_v0_1.h"
#include <algorithm>
#include <cmath>
namespace truthraw::truthnegative_structure_preservation_gate::v0_1 {
bool evaluate(const Input& i,Result& o) noexcept{o={};
 const double vals[]={i.center,i.left,i.right,i.up,i.down,i.sigma,i.registrationConfidence,i.visibilityConfidence};
 for(double v:vals)if(!std::isfinite(v))return false;
 if(i.sigma<0||i.registrationConfidence<0||i.registrationConfidence>1||i.visibilityConfidence<0||i.visibilityConfidence>1)return false;
 if(!i.sigmaKnown||i.sigma<=0||!i.measuredSupport||i.censored||i.boundaryCensored||i.registrationConfidence<0.9||i.visibilityConfidence<0.9){o.decision=Decision::Preserve;return true;}
 const double gx=0.5*(i.right-i.left), gy=0.5*(i.down-i.up);
 const double grad=std::hypot(gx,gy);
 const double lap=std::abs(i.left+i.right+i.up+i.down-4*i.center);
 o.gradientSigma=grad/i.sigma;o.laplacianSigma=lap/i.sigma;
 // Conservative N2 admission: only locally weak structure can become
 // eligible. This gate does not itself alter a value.
 if(o.gradientSigma<=1.0&&o.laplacianSigma<=2.0){
   o.decision=Decision::EligibleNoiseResidual;o.structureProtected=false;
   const double evidence=std::max(o.gradientSigma,o.laplacianSigma/2.0);
   o.maxSuppressionFraction=std::clamp(1.0-evidence,0.0,0.75);
 } else {o.decision=Decision::Preserve;}
 return true;
}
}
