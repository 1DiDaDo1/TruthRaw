#include "truthnegative_noise_state_v0_1.h"
#include <algorithm>
#include <bit>
#include <cmath>
namespace truthraw::truthnegative_noise_state::v0_1 {
namespace {bool nz(const Digest& d){return std::any_of(d.begin(),d.end(),[](auto v){return v;});}
void hf(truthraw::sha256_v0_69::Hasher& h,double v){auto u=std::bit_cast<std::uint64_t>(v);std::array<std::uint8_t,8>b{};for(int i=0;i<8;i++)b[i]=static_cast<std::uint8_t>(u>>(8*i));h.update(b);}}
bool finalize(const StateInput& in,State& out) noexcept{out={};if(!nz(in.truthNegativeStateSha256)||!nz(in.modelIdentitySha256)||in.authority==NoiseAuthority::Unresolved||in.physicalFrameCount!=1||in.independentEvidenceCount!=1)return false;
 for(auto m:in.channel)if(!std::isfinite(m.shotSlope)||!std::isfinite(m.readVariance)||m.shotSlope<0||m.readVariance<0)return false;
 truthraw::sha256_v0_69::Hasher h;constexpr char d[]="D_RAW_TRUTHNEGATIVE_NOISE_STATE_V0_1";h.update(reinterpret_cast<const std::uint8_t*>(d),sizeof(d)-1);h.update(in.truthNegativeStateSha256);h.update(in.modelIdentitySha256);for(auto m:in.channel){hf(h,m.shotSlope);hf(h,m.readVariance);}out.input=in;out.stateSha256=h.finalize();out.finalized=nz(out.stateSha256);return out.finalized;}
bool evaluate(const State& s,std::size_t c,double x,bool censored,SampleNoise& out) noexcept{out={};if(!s.finalized||c>=3||!std::isfinite(x))return false;out.authority=s.input.authority;out.sourceCensored=censored;if(censored)return true;auto m=s.input.channel[c];out.variance=m.shotSlope*std::max(x,0.0)+m.readVariance;if(!std::isfinite(out.variance)||out.variance<=0)return false;out.sigma=std::sqrt(out.variance);out.p95Abs=1.959963984540054*out.sigma;out.valid=true;return true;}
}
