#pragma once
#include "truthnegative_noise_state_v0_1.h"
#include "truthrange_dense_uncertainty_v0_3.h"
#include <cstddef>
namespace truthraw::truthnegative_noise_state::v0_1 {
struct DenseSampleBinding {
 SampleNoise noise{};
 bool measuredRole=false;
 bool topologyCertified=false;
 bool varianceAdmitted=false;
 truthraw::DenseUncertaintySourceV03 source=truthraw::DenseUncertaintySourceV03::Unresolved;
};
bool bindDenseMeasuredSample(const State&,const truthraw::DenseUncertaintyFieldV03&,std::size_t pixelIndex,std::size_t channel,bool isMeasuredChannel,DenseSampleBinding&) noexcept;
}
