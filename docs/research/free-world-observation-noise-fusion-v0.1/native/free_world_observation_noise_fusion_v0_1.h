#pragma once
#include "truthraw_sha256_v0_69.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace truthraw::free_world_observation_noise_fusion::v0_1 {
using Digest = truthraw::sha256_v0_69::Digest;
enum class ObservationAuthority : std::uint8_t { Measured=1, CalibratedEstimate=2, Reconstructed=3, Unknown=4, Censored=5 };
struct NoiseModel { double shotSlope=0.0; double readVariance=0.0; Digest identitySha256{}; bool calibrated=false; };
struct Observation {
 Digest sourceSha256{}; std::uint64_t frameId=0; std::array<double,3> value{};
 std::array<double,3> variance{}; std::array<ObservationAuthority,3> authority{};
 double registrationConfidence=0.0, visibilityConfidence=0.0; bool independentlySealed=false;
};
struct FusionResult {
 std::array<double,3> value{}; std::array<double,3> variance{};
 std::array<std::uint32_t,3> contributorCount{}; Digest ancestrySha256{};
 bool usedMultipleIndependentFrames=false, createsNewEvidence=false, scientificWritebackAllowed=false;
};
bool varianceFromShotRead(const NoiseModel&, double signal, double& variance) noexcept;
bool fuseConsistentObservations(const std::vector<Observation>&, FusionResult&) noexcept;
}
