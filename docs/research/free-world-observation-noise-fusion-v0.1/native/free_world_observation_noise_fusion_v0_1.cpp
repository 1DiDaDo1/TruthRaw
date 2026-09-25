#include "free_world_observation_noise_fusion_v0_1.h"
#include <algorithm>
#include <cmath>
namespace truthraw::free_world_observation_noise_fusion::v0_1 {
namespace { bool nz(const Digest& d){return std::any_of(d.begin(),d.end(),[](auto v){return v!=0;});} }
bool varianceFromShotRead(const NoiseModel& m,double s,double& v) noexcept {
 if(!m.calibrated||!nz(m.identitySha256)||!std::isfinite(s)||m.shotSlope<0||m.readVariance<0) return false;
 v=m.shotSlope*std::max(s,0.0)+m.readVariance; return std::isfinite(v)&&v>0;
}
bool fuseConsistentObservations(const std::vector<Observation>& in,FusionResult& out) noexcept {
 out={}; if(in.empty()) return false; truthraw::sha256_v0_69::Hasher hasher;
 constexpr char domain[]="D_RAW_FREE_WORLD_OBSERVATION_NOISE_FUSION_V0_1";
 hasher.update(reinterpret_cast<const std::uint8_t*>(domain),sizeof(domain)-1);
 std::array<double,3> sw{}, swx{};
 for(const auto& o:in){
  if(!o.independentlySealed||!nz(o.sourceSha256)||o.frameId==0||o.registrationConfidence<0.75||o.visibilityConfidence<0.75) continue;
  hasher.update(o.sourceSha256);
  for(std::size_t c=0;c<3;c++){
   if(o.authority[c]==ObservationAuthority::Censored||o.authority[c]==ObservationAuthority::Unknown) continue;
   if(!std::isfinite(o.value[c])||!std::isfinite(o.variance[c])||o.variance[c]<=0) continue;
   const double w=o.registrationConfidence*o.visibilityConfidence/o.variance[c];
   sw[c]+=w; swx[c]+=w*o.value[c]; out.contributorCount[c]++;
  }
 }
 for(std::size_t c=0;c<3;c++){if(sw[c]<=0) return false; out.value[c]=swx[c]/sw[c]; out.variance[c]=1.0/sw[c];}
 out.ancestrySha256=hasher.finalize();
 out.usedMultipleIndependentFrames=out.contributorCount[0]>1||out.contributorCount[1]>1||out.contributorCount[2]>1;
 return nz(out.ancestrySha256);
}
}
