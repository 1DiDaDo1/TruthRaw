#pragma once
#include "truthnegative_authority_aware_neighborhood_v0_1.h"
#include "truthnegative_structure_preservation_gate_v0_1.h"
#include "truthnegative_bounded_residual_estimator_v0_1.h"
#include <cstdint>
#include <limits>

namespace truthraw::truthnegative_n2_candidate_pipeline::v0_1 {
namespace neigh=truthraw::truthnegative_authority_aware_neighborhood::v0_1;
namespace gate=truthraw::truthnegative_structure_preservation_gate::v0_1;
namespace residual=truthraw::truthnegative_bounded_residual_estimator::v0_1;

enum class PreserveReason:std::uint8_t{
 None=0,InvalidOrUnknownNoise=1,Censored=2,CensorBoundary=3,NonMeasuredSupport=4,
 WeakRegistrationOrVisibility=5,Structure=6,NoCompatibleNeighborhood=7,ResidualOutlier=8
};

struct PixelInput{
 gate::Input structure{};
 neigh::Input neighborhood{};
};

struct PixelResult{
 double inputValue=0.0;
 double candidateValue=0.0;
 double correction=0.0;
 double originalResidual=0.0;
 double removedResidual=0.0;
 double retainedResidual=0.0;
 double suppressionFraction=0.0;
 std::uint32_t neighborhoodContributors=0;
 PreserveReason preserveReason=PreserveReason::None;
 bool neighborhoodValid=false;
 bool eligible=false;
 bool correctionApplied=false;
 bool createsNewEvidence=false;
 bool scientificWritebackAllowed=false;
};

struct Audit{
 std::uint64_t total=0;
 std::uint64_t preserved=0;
 std::uint64_t eligible=0;
 std::uint64_t corrected=0;
 std::uint64_t censoredProtected=0;
 std::uint64_t censorBoundaryProtected=0;
 std::uint64_t unknownNoiseProtected=0;
 std::uint64_t nonMeasuredProtected=0;
 std::uint64_t weakRegistrationProtected=0;
 std::uint64_t structureProtected=0;
 std::uint64_t noNeighborhoodProtected=0;
 std::uint64_t residualOutlierProtected=0;
 double totalResidualEnergy=0.0;
 double removedResidualEnergy=0.0;
 double maxAbsCorrection=0.0;
 bool createsNewEvidence=false;
 bool scientificWritebackAllowed=false;
};

bool evaluatePixel(const PixelInput&,PixelResult&) noexcept;
bool accumulate(const PixelResult&,Audit&) noexcept;
}
