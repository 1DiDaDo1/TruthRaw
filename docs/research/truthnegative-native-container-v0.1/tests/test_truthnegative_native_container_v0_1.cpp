#include "truthnegative_native_container_v0_1.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace c = truthraw::truthnegative_native_container::v0_1;
namespace field = truthraw::open_scene_field::v0_85;
namespace local = truthraw::truthnegative_local_authority_projection::v0_4;
namespace tn = truthraw::truthnegative_continuous::v0_5;

#define REQUIRE(x) do{ if(!(x)) throw std::runtime_error(#x); }while(0)

namespace {

c::Digest digest(std::uint8_t seed){
 c::Digest d{}; for(std::size_t i=0;i<d.size();++i)d[i]=seed+i; return d;
}

class Mem final : public c::IRandomAccessSink, public c::IRandomAccessSource {
public:
 bool writeAt(std::uint64_t o,const void* p,std::size_t n) noexcept override{
  try{ if(o+n>bytes.size()) bytes.resize(static_cast<std::size_t>(o+n));
   std::memcpy(bytes.data()+o,p,n); return true;}catch(...){return false;}
 }
 bool resize(std::uint64_t n) noexcept override{
  try{bytes.resize(static_cast<std::size_t>(n));return true;}catch(...){return false;}
 }
 std::uint64_t sizeBytes() const noexcept override{return bytes.size();}
 bool readAt(std::uint64_t o,void* p,std::size_t n) const noexcept override{
  if(o+n>bytes.size()) return false;
  std::memcpy(p,bytes.data()+o,n);
  return true;
 }
 std::vector<std::uint8_t> bytes;
};

class SyntheticField final : public local::IFieldTileSource {
public:
 local::Geometry geometry() const noexcept override{return {9u,7u,9u,7u};}
 bool readSourceTile(std::uint32_t x,std::uint32_t y,std::uint32_t w,std::uint32_t h,
  field::ChannelRecord* out,std::size_t count) noexcept override{
  if(!out||x+w>9u||y+h>7u||count!=static_cast<std::size_t>(w)*h*3u)return false;
  for(std::uint32_t yy=0;yy<h;++yy)for(std::uint32_t xx=0;xx<w;++xx)for(std::size_t ch=0;ch<3;++ch){
   auto& r=out[(static_cast<std::size_t>(yy)*w+xx)*3u+ch];
   r.value=static_cast<float>(0.1*(ch+1)+0.01*(x+xx)+0.02*(y+yy));
   r.valuePresent=true;
   r.role=ch==1?field::CreationRole::SourceMeasuredCfa:field::CreationRole::ScientificReconstruction;
   r.authority=ch==1?field::Authority::CalibratedEstimate:field::Authority::Unknown;
   r.uncertainty=field::UncertaintyKnowledge::Unresolved;
   if(ch==1){r.supportKnown=true;r.support=1.0f;r.contributionMask=field::ContributionMeasured;}
   else r.contributionMask=field::ContributionReconstructed|field::ContributionUnknown;
  }
  return true;
 }
};

tn::State state(){
 tn::StateInput in{};
 in.sourceEvidenceSha256=digest(1); in.scientificMasterSha256=digest(33);
 in.authorityFieldSha256=digest(65); in.width=9; in.height=7;
 in.reconstructionBackendId="TEST_F64"; in.colourBindingId="TEST_COLOR";
 tn::State s{}; REQUIRE(tn::finalizeState(in,s)); return s;
}

void test_write_read_roundtrip(){
 SyntheticField f; Mem mem; c::WriteInput in{state(),"TEST_COLOR"};
 c::Summary written{}; REQUIRE(c::write(in,f,mem,written));
 REQUIRE(written.fileBytes==mem.sizeBytes());
 REQUIRE(!written.createsNewEvidence);
 REQUIRE(written.recordCount==9u*7u*3u);
 REQUIRE(written.roleSourceMeasuredCfa==9u*7u);
 REQUIRE(written.roleScientificReconstruction==9u*7u*2u);
 REQUIRE(written.authorityCalibratedEstimate==9u*7u);
 REQUIRE(written.authorityUnknown==9u*7u*2u);
 REQUIRE(written.authorityReconstructed==0u);
 REQUIRE(written.authorityCensored==0u);
 REQUIRE(written.valueNonFiniteCount==0u);

 c::Reader reader; REQUIRE(reader.open(mem)); REQUIRE(reader.valid());
 REQUIRE(reader.summary().sourceEvidenceSha256==written.sourceEvidenceSha256);
 REQUIRE(reader.summary().scientificMasterSha256==written.scientificMasterSha256);
 REQUIRE(reader.summary().truthNegativeStateSha256==written.truthNegativeStateSha256);
 REQUIRE(reader.summary().bodySha256==written.bodySha256);
 REQUIRE(reader.summary().roleSourceMeasuredCfa==written.roleSourceMeasuredCfa);
 REQUIRE(reader.summary().roleScientificReconstruction==written.roleScientificReconstruction);
 REQUIRE(reader.summary().authorityCalibratedEstimate==written.authorityCalibratedEstimate);
 REQUIRE(reader.summary().authorityUnknown==written.authorityUnknown);
 REQUIRE(reader.summary().valueNonFiniteCount==0u);

 std::vector<field::ChannelRecord> a(9u*7u*3u), b(a.size());
 REQUIRE(f.readSourceTile(0,0,9,7,a.data(),a.size()));
 REQUIRE(reader.readSourceTile(0,0,9,7,b.data(),b.size()));
 for(std::size_t i=0;i<a.size();++i){
  REQUIRE(std::bit_cast<std::uint32_t>(a[i].value)==std::bit_cast<std::uint32_t>(b[i].value));
  REQUIRE(a[i].role==b[i].role); REQUIRE(a[i].authority==b[i].authority);
  REQUIRE(a[i].contributionMask==b[i].contributionMask);
 }
}

void test_corruption_fails_closed(){
 SyntheticField f; Mem mem; c::WriteInput in{state(),"TEST_COLOR"};
 c::Summary s{}; REQUIRE(c::write(in,f,mem,s));
 REQUIRE(mem.bytes.size()>c::kHeaderBytes+64u);
 mem.bytes.back()^=1u;
 c::Reader reader; REQUIRE(!reader.open(mem));
}

}

int main(){
 test_write_read_roundtrip(); test_corruption_fails_closed();
 std::cout<<"TruthNegativeNativeContainer/0.1 PASS\n";
 std::cout<<"export_import_roundtrip=1\n";
 return 0;
}
