#include "truthnegative_bounded_residual_estimator_v0_1.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace e=truthraw::truthnegative_bounded_residual_estimator::v0_1; namespace g=truthraw::truthnegative_structure_preservation_gate::v0_1;
#define R(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
int main(){g::Result a{};a.decision=g::Decision::EligibleNoiseResidual;a.structureProtected=false;a.maxSuppressionFraction=.75;
 e::Result o{};R(e::estimate({1.02,1.0,.05,true,a},o));R(o.applied);R(o.suppressionFraction<=.75);R(std::abs((o.removedResidual+o.retainedResidual)-o.residual)<1e-12);
 auto p=a;p.decision=g::Decision::Preserve;p.structureProtected=true;R(e::estimate({1.02,1,.05,true,p},o));R(!o.applied&&o.output==1.02);
 R(e::estimate({1.2,1,.05,true,a},o));R(!o.applied);std::cout<<"TruthNegativeBoundedResidualEstimator/0.1 PASS\n";}
