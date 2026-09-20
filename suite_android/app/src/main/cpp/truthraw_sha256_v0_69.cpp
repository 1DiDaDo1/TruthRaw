#include "truthraw_sha256_v0_69.h"
#include <algorithm>
#include <cstring>

namespace truthraw::sha256_v0_69 {
namespace {
constexpr std::array<std::uint32_t,64> K={
0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u};
inline std::uint32_t rotr(std::uint32_t v,unsigned n) noexcept {return (v>>n)|(v<<(32u-n));}
}
void Hasher::update(const std::uint8_t* data,std::size_t size) noexcept {
    if(finalized_||data==nullptr||size==0) return;
    total_+=static_cast<std::uint64_t>(size);
    while(size){
        const auto take=std::min<std::size_t>(size,block_.size()-used_);
        std::memcpy(block_.data()+used_,data,take);
        used_+=take; data+=take; size-=take;
        if(used_==block_.size()){transform(block_.data());used_=0;}
    }
}
void Hasher::transform(const std::uint8_t* b) noexcept {
    std::array<std::uint32_t,64> w{};
    for(std::size_t i=0;i<16;++i) w[i]=(std::uint32_t(b[4*i])<<24)|(std::uint32_t(b[4*i+1])<<16)|(std::uint32_t(b[4*i+2])<<8)|std::uint32_t(b[4*i+3]);
    for(std::size_t i=16;i<64;++i){auto s0=rotr(w[i-15],7)^rotr(w[i-15],18)^(w[i-15]>>3);auto s1=rotr(w[i-2],17)^rotr(w[i-2],19)^(w[i-2]>>10);w[i]=w[i-16]+s0+w[i-7]+s1;}
    auto a=state_[0],b0=state_[1],c=state_[2],d=state_[3],e=state_[4],f=state_[5],g=state_[6],h=state_[7];
    for(std::size_t i=0;i<64;++i){auto S1=rotr(e,6)^rotr(e,11)^rotr(e,25);auto ch=(e&f)^((~e)&g);auto t1=h+S1+ch+K[i]+w[i];auto S0=rotr(a,2)^rotr(a,13)^rotr(a,22);auto maj=(a&b0)^(a&c)^(b0&c);auto t2=S0+maj;h=g;g=f;f=e;e=d+t1;d=c;c=b0;b0=a;a=t1+t2;}
    state_[0]+=a;state_[1]+=b0;state_[2]+=c;state_[3]+=d;state_[4]+=e;state_[5]+=f;state_[6]+=g;state_[7]+=h;
}
Digest Hasher::finalize() noexcept {
    if(finalized_) return {};
    const std::uint64_t bits=total_*8u;
    block_[used_++]=0x80u;
    if(used_>56u){std::fill(block_.begin()+static_cast<std::ptrdiff_t>(used_),block_.end(),0u);transform(block_.data());used_=0;}
    std::fill(block_.begin()+static_cast<std::ptrdiff_t>(used_),block_.begin()+56,0u);
    for(unsigned i=0;i<8;++i) block_[63u-i]=static_cast<std::uint8_t>((bits>>(8u*i))&0xffu);
    transform(block_.data());
    Digest out{};
    for(std::size_t i=0;i<8;++i){out[4*i]=std::uint8_t(state_[i]>>24);out[4*i+1]=std::uint8_t(state_[i]>>16);out[4*i+2]=std::uint8_t(state_[i]>>8);out[4*i+3]=std::uint8_t(state_[i]);}
    finalized_=true; return out;
}
std::string hex(const Digest& d){static constexpr char h[]="0123456789abcdef";std::string out(d.size()*2,'0');for(std::size_t i=0;i<d.size();++i){out[2*i]=h[(d[i]>>4)&15];out[2*i+1]=h[d[i]&15];}return out;}
}
