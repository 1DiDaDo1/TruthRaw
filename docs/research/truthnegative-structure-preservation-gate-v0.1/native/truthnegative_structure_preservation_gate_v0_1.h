#pragma once
#include <array>
#include <cstdint>
namespace truthraw::truthnegative_structure_preservation_gate::v0_1 {
enum class Decision:std::uint8_t{Preserve=1,EligibleNoiseResidual=2,Unresolved=3};
struct Input{
 double center=0,left=0,right=0,up=0,down=0;
 double sigma=0; bool sigmaKnown=false,censored=false,boundaryCensored=false;
 bool measuredSupport=false; double registrationConfidence=1,visibilityConfidence=1;
};
struct Result{
 Decision decision=Decision::Unresolved; double gradientSigma=0,laplacianSigma=0;
 double maxSuppressionFraction=0; bool structureProtected=true,createsNewEvidence=false,scientificWritebackAllowed=false;
};
bool evaluate(const Input&,Result&) noexcept;
}
