#include "truthnegative_n2_spatial_sidecar_v0_1.h"
#include "truthnegative_n2_cfa_audit_v0_1.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace a=truthraw::truthnegative_n2_cfa_audit::v0_1;
namespace s=truthraw::truthnegative_n2_spatial_sidecar::v0_1;
namespace st=truthraw::streaming_v0_1;
#define R(x) do{if(!(x))throw std::runtime_error(#x);}while(0)

class FakeSource final:public st::IRawTileSource{
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
 st::StreamStatus readRawTile(const truthraw::TileRect&t,std::uint16_t*out,std::size_t n,float*,std::size_t) override{
  const int w=t.hx1-t.hx0,h=t.hy1-t.hy0;if(n!=std::size_t(w)*h)return st::StreamStatus::error(st::StreamStatusCode::InvalidArgument,"size");
  for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx)out[std::size_t(yy)*w+xx]=raw[std::size_t(t.hy0+yy)*32u+(t.hx0+xx)];
  return st::StreamStatus::ok();
 }
 st::StreamStatus readRowBias(int,int,float*,std::size_t) override{return st::StreamStatus::ok();}
 st::StreamStatus readColBias(int,int,float*,std::size_t) override{return st::StreamStatus::ok();}
 truthraw::DngMetadata md{};std::vector<std::uint16_t> raw;
};

int main(){
 FakeSource src;
 a::Binding ab{};ab.sourceEvidenceSha256[0]=1;ab.truthNegativeStateSha256[0]=2;
 a::Options opt{};opt.tileEdge=16;opt.samplingPeriod=2;
 a::Result ar{};R(a::run(src,ab,opt,ar));R(ar.tiles.size()==4u);R(ar.sampled==1024u);
 std::uint64_t sum=0;for(const auto&t:ar.tiles)sum+=t.sampled;R(sum==ar.sampled);
 s::Binding b{};b.sourceEvidenceSha256[0]=1;b.scientificMasterSha256[0]=3;b.authorityFieldSha256[0]=4;b.truthNegativeStateSha256[0]=2;
 s::Report r{};R(s::encode(b,32,32,ar,r));R(r.tileCount==4u);
 R(!r.createsNewEvidence&&!r.scientificWritebackAllowed&&!r.candidateApplied);
 R(r.json.find("\"tiles\"")!=std::string::npos);
 R(r.json.find("\"candidate_applied\":false")!=std::string::npos);
 R(std::any_of(r.jsonSha256.begin(),r.jsonSha256.end(),[](auto v){return v!=0;}));
 std::cout<<"TruthNegativeN2SpatialSidecar/0.1 PASS bytes="<<r.json.size()<<"\n";
}
