#pragma once
#include "truthnegative_structure_preservation_gate_v0_1.h"
#include <cstdint>
namespace truthraw::truthnegative_bounded_residual_estimator::v0_1 {
namespace gate=truthraw::truthnegative_structure_preservation_gate::v0_1;
struct Input{double observation=0,localEstimate=0,sigma=0;bool sigmaKnown=false;gate::Result admission{};};
struct Result{double output=0,residual=0,removedResidual=0,retainedResidual=0,suppressionFraction=0;bool applied=false,createsNewEvidence=false,scientificWritebackAllowed=false;};
bool estimate(const Input&,Result&) noexcept;
}
