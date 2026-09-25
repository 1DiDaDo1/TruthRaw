#include "truthnegative_n2_candidate_pipeline_v0_1.h"
#include <algorithm>
#include <cmath>

namespace truthraw::truthnegative_n2_candidate_pipeline::v0_1 {
namespace {
bool close(double a,double b) noexcept{
 const double s=std::max({1.0,std::abs(a),std::abs(b)});
 return std::abs(a-b)<=1e-9*s;
}
PreserveReason classifyPreserve(const gate::Input& i,const gate::Result& g) noexcept{
 // Evidence state outranks the availability of a point-noise model. A censored
 // sample remains CENSORED even though Gaussian sigma is intentionally absent.
 if(i.censored) return PreserveReason::Censored;
 if(i.boundaryCensored) return PreserveReason::CensorBoundary;
 if(!i.sigmaKnown||!std::isfinite(i.sigma)||i.sigma<=0.0) return PreserveReason::InvalidOrUnknownNoise;
 if(!i.measuredSupport) return PreserveReason::NonMeasuredSupport;
 if(i.registrationConfidence<0.9||i.visibilityConfidence<0.9) return PreserveReason::WeakRegistrationOrVisibility;
 if(g.structureProtected) return PreserveReason::Structure;
 return PreserveReason::None;
}
}

bool evaluatePixel(const PixelInput& in,PixelResult& out) noexcept{
 out={};
 const auto& s=in.structure;
 const auto& n=in.neighborhood;
 if(!std::isfinite(s.center)||!std::isfinite(n.center)||!close(s.center,n.center)) return false;
 if(s.sigmaKnown){
  if(!std::isfinite(s.sigma)||s.sigma<0.0) return false;
  if(!n.centerVarianceKnown||!std::isfinite(n.centerVariance)||n.centerVariance<=0.0) return false;
  if(!close(n.centerVariance,s.sigma*s.sigma)) return false;
 }
 out.inputValue=s.center; out.candidateValue=s.center;

 gate::Result gr{};
 if(!gate::evaluate(s,gr)) return false;
 if(gr.decision!=gate::Decision::EligibleNoiseResidual){
  out.preserveReason=classifyPreserve(s,gr);
  return true;
 }
 out.eligible=true;

 neigh::Result nr{};
 if(!neigh::estimate(n,nr)) return false;
 out.neighborhoodContributors=nr.contributors;
 out.neighborhoodValid=nr.valid;
 if(!nr.valid){
  out.preserveReason=PreserveReason::NoCompatibleNeighborhood;
  return true;
 }

 residual::Result rr{};
 residual::Input ri{};
 ri.observation=s.center;
 ri.localEstimate=nr.estimate;
 ri.sigma=s.sigma;
 ri.sigmaKnown=s.sigmaKnown;
 ri.admission=gr;
 if(!residual::estimate(ri,rr)) return false;

 out.candidateValue=rr.output;
 out.correction=rr.output-s.center;
 out.originalResidual=rr.residual;
 out.removedResidual=rr.removedResidual;
 out.retainedResidual=rr.retainedResidual;
 out.suppressionFraction=rr.suppressionFraction;
 out.correctionApplied=rr.applied;
 if(!rr.applied){
  const double z=(s.sigmaKnown&&s.sigma>0.0)?std::abs(rr.residual)/s.sigma:0.0;
  if(z>2.0) out.preserveReason=PreserveReason::ResidualOutlier;
 }
 return true;
}

bool accumulate(const PixelResult& r,Audit& a) noexcept{
 const double values[]={r.inputValue,r.candidateValue,r.correction,r.originalResidual,r.removedResidual,r.retainedResidual,r.suppressionFraction};
 for(double v:values) if(!std::isfinite(v)) return false;
 a.total++;
 if(r.eligible) a.eligible++;
 if(r.correctionApplied){
  a.corrected++;
  a.maxAbsCorrection=std::max(a.maxAbsCorrection,std::abs(r.correction));
 }else{
  a.preserved++;
  switch(r.preserveReason){
   case PreserveReason::InvalidOrUnknownNoise:a.unknownNoiseProtected++;break;
   case PreserveReason::Censored:a.censoredProtected++;break;
   case PreserveReason::CensorBoundary:a.censorBoundaryProtected++;break;
   case PreserveReason::NonMeasuredSupport:a.nonMeasuredProtected++;break;
   case PreserveReason::WeakRegistrationOrVisibility:a.weakRegistrationProtected++;break;
   case PreserveReason::Structure:a.structureProtected++;break;
   case PreserveReason::NoCompatibleNeighborhood:a.noNeighborhoodProtected++;break;
   case PreserveReason::ResidualOutlier:a.residualOutlierProtected++;break;
   default:break;
  }
 }
 a.totalResidualEnergy+=r.originalResidual*r.originalResidual;
 a.removedResidualEnergy+=r.removedResidual*r.removedResidual;
 return std::isfinite(a.totalResidualEnergy)&&std::isfinite(a.removedResidualEnergy)&&std::isfinite(a.maxAbsCorrection);
}
}
