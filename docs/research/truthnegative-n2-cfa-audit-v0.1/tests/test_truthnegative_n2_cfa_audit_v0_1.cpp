#include "truthnegative_n2_cfa_audit_v0_1.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace a=truthraw::truthnegative_n2_cfa_audit::v0_1;
namespace s=truthraw::streaming_v0_1;
#define R(x) do{if(!(x))throw std::runtime_error(#x);}while(0)

class FakeSource final:public s::IRawTileSource{
public:
 FakeSource(){
  md.width=32;md.height=32;md.cfa=truthraw::CfaPattern::BGGR;
  md.whiteLevel=1023;md.blackPhase={64,64,64,64};
  md.hasNoiseProfile=true;
  md.noiseProfile={0.004f,0.00002f,0.004f,0.00002f,0.004f,0.00002f};
  raw.resize(32u*32u,300u);
  for(int y=0;y<32;++y)for(int x=0;x<32;++x)
   raw[std::size_t(y)*32u+x]=static_cast<std::uint16_t>(300+((x+y)%3)-1);
  raw[16u*32u+16u]=1023u;
 }
 const truthraw::DngMetadata& metadata() const override{return md;}
 std::size_t residentBytesUpperBound() const override{return raw.size()*sizeof(std::uint16_t);}
 s::StreamStatus readRawTile(const truthraw::TileRect&t,std::uint16_t*out,std::size_t n,float*,std::size_t) override{
  const int w=t.hx1-t.hx0,h=t.hy1-t.hy0;if(n!=std::size_t(w)*h)return s::StreamStatus::error(s::StreamStatusCode::InvalidArgument,"size");
  for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx)out[std::size_t(yy)*w+xx]=raw[std::size_t(t.hy0+yy)*32u+(t.hx0+xx)];
  return s::StreamStatus::ok();
 }
 s::StreamStatus readRowBias(int,int,float*,std::size_t) override{return s::StreamStatus::ok();}
 s::StreamStatus readColBias(int,int,float*,std::size_t) override{return s::StreamStatus::ok();}
 truthraw::DngMetadata md{};std::vector<std::uint16_t> raw;
};

int main(){
 FakeSource src;a::Binding b{};b.sourceEvidenceSha256[0]=1;b.truthNegativeStateSha256[0]=2;
 a::Options o{};o.tileEdge=16;o.samplingPeriod=2;
 a::Result r{};R(a::run(src,b,o,r));R(r.noiseProfileAvailable);R(r.sampled==1024);R(r.audit.total==r.sampled);
 R(r.audit.corrected>0);R(r.audit.censoredProtected>0||r.audit.censorBoundaryProtected>0);
 R(!r.sourceValuesModified&&!r.truthNegativeModified&&!r.createsNewEvidence&&!r.scientificWritebackAllowed);
 R(std::any_of(r.auditSha256.begin(),r.auditSha256.end(),[](auto v){return v!=0;}));
 R(std::any_of(r.spatialSha256.begin(),r.spatialSha256.end(),[](auto v){return v!=0;}));
 R(r.tileEdge==16u);R(r.tiles.size()==4u);std::uint64_t tileSamples=0;for(const auto&t:r.tiles){R(t.sampled>0);R(t.audit.total==t.sampled);tileSamples+=t.sampled;}R(tileSamples==r.sampled);
 std::cout<<"TruthNegativeN2CfaAudit/0.1 PASS corrected="<<r.audit.corrected<<" preserved="<<r.audit.preserved<<"\n";
}
