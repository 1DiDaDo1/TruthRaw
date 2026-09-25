#pragma once
#include "truthraw_sha256_v0_69.h"
#include <array>
#include <cstdint>
namespace truthraw::truthnegative_noise_state::v0_1 {
using Digest=truthraw::sha256_v0_69::Digest;
enum class NoiseAuthority:std::uint8_t{Unresolved=0,MetadataNoiseProfile=1,IndependentCalibration=2};
struct ChannelModel{double shotSlope=0,readVariance=0;};
struct StateInput{
 Digest truthNegativeStateSha256{}; Digest modelIdentitySha256{};
 std::array<ChannelModel,3> channel{}; NoiseAuthority authority=NoiseAuthority::Unresolved;
 std::uint32_t physicalFrameCount=1,independentEvidenceCount=1;
};
struct State{StateInput input{};Digest stateSha256{};bool finalized=false,createsNewEvidence=false,scientificWritebackAllowed=false;};
struct SampleNoise{double variance=0,sigma=0,p95Abs=0;NoiseAuthority authority=NoiseAuthority::Unresolved;bool valid=false,sourceCensored=false;};
bool finalize(const StateInput&,State&) noexcept;
bool evaluate(const State&,std::size_t channel,double positiveSignal,bool censored,SampleNoise&) noexcept;
}
