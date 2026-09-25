#pragma once
#include <cstdint>
#include <vector>
namespace truthraw::truthnegative_authority_aware_neighborhood::v0_1 {
enum class SampleAuthority:std::uint8_t{Measured=1,CalibratedEstimate=2,Reconstructed=3,Unknown=4,Censored=5};
struct Sample{
 double value=0,variance=0,spatialDistance=0;
 SampleAuthority authority=SampleAuthority::Unknown;
 bool varianceKnown=false,sameChannel=true,sameObject=true,censorBoundary=false;
 // Unknown object identity must not be silently converted into a cross-object
 // rejection or an equality claim. When known, sameObject is enforced.
 bool objectIdentityKnown=false;
};
struct Input{double center=0,centerVariance=0;bool centerVarianceKnown=false;std::vector<Sample> neighbors;};
struct Result{double estimate=0,effectiveWeight=0;std::uint32_t contributors=0;bool valid=false,createsNewEvidence=false,scientificWritebackAllowed=false;};
bool estimate(const Input&,Result&) noexcept;
}
