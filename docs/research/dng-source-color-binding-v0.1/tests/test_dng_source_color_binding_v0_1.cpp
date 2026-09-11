#include "dng_source_color_binding_v0_1.h"
#include "technical_backplane_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace truthraw::dng_source_color_v0_1;
using namespace truthraw::scientific_preview_binding_v0_1;
using truthraw::technical_backplane::v0_1::State;
using truthraw::tile_dng_v0_1::IRandomAccessByteSource;

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(std::string("REQUIRE failed: ") + #x); } while (0)

namespace {
constexpr std::uint16_t BYTE=1, ASCII=2, SHORT=3, RATIONAL=5, SRATIONAL=10;

struct MemSource final : IRandomAccessByteSource {
    std::vector<std::uint8_t> bytes;
    explicit MemSource(std::vector<std::uint8_t> b):bytes(std::move(b)){}
    std::uint64_t sizeBytes() const override { return bytes.size(); }
    std::size_t residentBytesUpperBound() const override { return bytes.capacity(); }
    bool readExact(std::uint64_t off, void* dst, std::size_t n) override {
        if(off>bytes.size() || n>bytes.size()-off) return false;
        std::memcpy(dst,bytes.data()+std::size_t(off),n); return true;
    }
};

void put16(std::vector<std::uint8_t>& b,std::size_t p,std::uint16_t v,bool le){
    if(le){b[p]=std::uint8_t(v);b[p+1]=std::uint8_t(v>>8);}else{b[p]=std::uint8_t(v>>8);b[p+1]=std::uint8_t(v);}
}
void put32(std::vector<std::uint8_t>& b,std::size_t p,std::uint32_t v,bool le){
    if(le){for(int i=0;i<4;++i)b[p+std::size_t(i)]=std::uint8_t(v>>(8*i));}
    else{for(int i=0;i<4;++i)b[p+std::size_t(i)]=std::uint8_t(v>>(8*(3-i)));}
}
void append32(std::vector<std::uint8_t>& b,std::uint32_t v,bool le){auto p=b.size();b.resize(p+4);put32(b,p,v,le);}

std::vector<std::uint8_t> srats(const std::array<std::pair<std::int32_t,std::int32_t>,9>& x,bool le){
    std::vector<std::uint8_t>b; b.reserve(72); for(auto [n,d]:x){append32(b,std::uint32_t(n),le);append32(b,std::uint32_t(d),le);} return b;
}
std::vector<std::uint8_t> rats3(const std::array<std::pair<std::uint32_t,std::uint32_t>,3>& x,bool le){
    std::vector<std::uint8_t>b; b.reserve(24); for(auto [n,d]:x){append32(b,n,le);append32(b,d,le);} return b;
}
std::vector<std::uint8_t> rats2(const std::array<std::pair<std::uint32_t,std::uint32_t>,2>& x,bool le){
    std::vector<std::uint8_t>b; b.reserve(16); for(auto [n,d]:x){append32(b,n,le);append32(b,d,le);} return b;
}
std::vector<std::uint8_t> short1(std::uint16_t v,bool le){std::vector<std::uint8_t>b(2);put16(b,0,v,le);return b;}
std::vector<std::uint8_t> ascii(const std::string&s){std::vector<std::uint8_t>b(s.begin(),s.end());b.push_back(0);return b;}

struct Tag {std::uint16_t tag,type;std::uint32_t count;std::vector<std::uint8_t> payload;};

std::array<std::pair<std::int32_t,std::int32_t>,9> identity(){
    return {{{1,1},{0,1},{0,1},{0,1},{1,1},{0,1},{0,1},{0,1},{1,1}}};
}
std::array<std::pair<std::int32_t,std::int32_t>,9> forward_d50(){
    return {{{9643,10000},{0,1},{0,1},{0,1},{1,1},{0,1},{0,1},{0,1},{8251,10000}}};
}

std::vector<Tag> base_tags(bool le){
    return {
        {50706,BYTE,4,{1,7,1,0}},
        {50721,SRATIONAL,9,srats(identity(),le)},
        {50728,RATIONAL,3,rats3({{{9643,10000},{1,1},{8251,10000}}},le)},
        {50778,SHORT,1,short1(21,le)},
        {50964,SRATIONAL,9,srats(forward_d50(),le)},
    };
}

std::vector<std::uint8_t> make_dng(std::vector<Tag> tags,bool le=true){
    std::sort(tags.begin(),tags.end(),[](const Tag&a,const Tag&b){return a.tag<b.tag;});
    const std::size_t dirStart=8, entriesStart=10;
    std::vector<std::uint8_t>b(entriesStart+tags.size()*12+4,0);
    b[0]=le?'I':'M';b[1]=b[0];put16(b,2,42,le);put32(b,4,std::uint32_t(dirStart),le);put16(b,dirStart,std::uint16_t(tags.size()),le);
    std::size_t payloadCursor=b.size();
    for(std::size_t i=0;i<tags.size();++i){
        const Tag&t=tags[i];const std::size_t p=entriesStart+i*12;put16(b,p,t.tag,le);put16(b,p+2,t.type,le);put32(b,p+4,t.count,le);
        if(t.payload.size()<=4){std::copy(t.payload.begin(),t.payload.end(),b.begin()+std::ptrdiff_t(p+8));}
        else{put32(b,p+8,std::uint32_t(payloadCursor),le);b.insert(b.end(),t.payload.begin(),t.payload.end());payloadCursor+=t.payload.size();}
    }
    return b;
}

SourceSeal seal(MemSource& source){SourceSeal s;auto st=seal_source_sha256(source,s,4096);REQUIRE(st);return s;}
State backplane_for(const SourceSeal&s){State b;b.sourceEvidenceHash=s.sha256;b.scientificMasterHash.fill(0x11);b.zeroLineHash.fill(0x22);b.sceneScaleHash.fill(0x33);return b;}

void require_identityish(const std::array<float,9>&m){
    for(std::size_t i=0;i<9;++i){const float expected=(i==0||i==4||i==8)?1.f:0.f;REQUIRE(std::abs(m[i]-expected)<0.0025f);}
}
}

int main(){
    {
        MemSource src(make_dng(base_tags(true),true)); const auto s=seal(src); Result r;
        auto st=produce_source_bound_color_binding(src,s,r); REQUIRE(st);
        REQUIRE(r.binding.authority==ColorBindingAuthority::SourceMetadataBound); REQUIRE(r.binding.validated); REQUIRE(r.binding.sourceEvidenceId==s.sourceEvidenceId);
        REQUIRE(r.binding.physicalFrameCount==1 && r.binding.independentEvidenceCount==1); REQUIRE(r.metrics.usedAsShotNeutral); REQUIRE(!r.metrics.usedAsShotWhiteXY);
        REQUIRE(r.metrics.temporaryBytesUpperBound<2048); require_identityish(r.binding.cameraToXyzD50);
        ScientificPreviewAdmission a; auto bp=backplane_for(s); auto bst=admit_scientific_color_preview(s,r.binding,bp,a); REQUIRE(bst); REQUIRE(a.tileNativeOptions.color.valid);
    }
    {
        MemSource src(make_dng(base_tags(false),false)); const auto s=seal(src); Result r; auto st=produce_source_bound_color_binding(src,s,r); REQUIRE(st); REQUIRE(!r.metrics.littleEndian); require_identityish(r.binding.cameraToXyzD50);
    }
    {
        auto tags=base_tags(true); tags.erase(std::remove_if(tags.begin(),tags.end(),[](const Tag&t){return t.tag==50728;}),tags.end());
        tags.push_back({50729,RATIONAL,2,rats2({{{3457,10000},{3585,10000}}},true)});
        MemSource src(make_dng(tags));auto s=seal(src);Result r;auto st=produce_source_bound_color_binding(src,s,r);REQUIRE(st);REQUIRE(r.metrics.usedAsShotWhiteXY);require_identityish(r.binding.cameraToXyzD50);
    }
    {
        auto tags=base_tags(true); tags.push_back({50722,SRATIONAL,9,srats(identity(),true)});
        MemSource src(make_dng(tags));auto s=seal(src);Result r;auto st=produce_source_bound_color_binding(src,s,r);REQUIRE(!st&&st.code==StatusCode::UnsupportedProfileTopology);
    }
    {
        auto tags=base_tags(true); tags.push_back({52529,SHORT,1,short1(21,true)});
        MemSource src(make_dng(tags));auto s=seal(src);Result r;auto st=produce_source_bound_color_binding(src,s,r);REQUIRE(!st&&st.code==StatusCode::UnsupportedProfileTopology);
    }
    {
        auto tags=base_tags(true); tags.erase(std::remove_if(tags.begin(),tags.end(),[](const Tag&t){return t.tag==50964;}),tags.end());
        MemSource src(make_dng(tags));auto s=seal(src);Result r;auto st=produce_source_bound_color_binding(src,s,r);REQUIRE(!st&&st.code==StatusCode::MissingForwardMatrix1);
    }
    {
        auto tags=base_tags(true); tags.push_back({50879,SHORT,1,short1(1,true)});
        MemSource src(make_dng(tags));auto s=seal(src);Result r;auto st=produce_source_bound_color_binding(src,s,r);REQUIRE(!st&&st.code==StatusCode::UnsupportedColorimetricReference);
    }
    {
        auto tags=base_tags(true); auto bad=forward_d50(); bad[0].second=0;
        for(auto&t:tags)if(t.tag==50964)t.payload=srats(bad,true);
        MemSource src(make_dng(tags));auto s=seal(src);Result r;auto st=produce_source_bound_color_binding(src,s,r);REQUIRE(!st&&st.code==StatusCode::InvalidRational);
    }
    {
        auto tags=base_tags(true);tags.push_back({50723,SRATIONAL,9,srats(identity(),true)});
        MemSource src(make_dng(tags));auto s=seal(src);Result r;auto st=produce_source_bound_color_binding(src,s,r);REQUIRE(!st&&st.code==StatusCode::InvalidCalibrationSignature);
        tags.push_back({50931,ASCII,10,ascii("truthraw")});tags.push_back({50932,ASCII,10,ascii("truthraw")});
        MemSource src2(make_dng(tags));auto s2=seal(src2);st=produce_source_bound_color_binding(src2,s2,r);REQUIRE(st);REQUIRE(r.metrics.usedCameraCalibration1);
    }
    {
        MemSource src(make_dng(base_tags(true)));auto s=seal(src);src.bytes.push_back(0x7f);Result r;auto st=produce_source_bound_color_binding(src,s,r);REQUIRE(!st&&st.code==StatusCode::SourceChanged);
    }

    std::cout<<"DNG_SOURCE_COLOR_BINDING_V0_1_PASS\n";
    std::cout<<"profile_scope=STRICT_SINGLE_ILLUMINANT_FORWARD_MATRIX\n";
    std::cout<<"dual_triple_profiles=FAIL_CLOSED\n";
    std::cout<<"source_mutation=FAIL_CLOSED\n";
    std::cout<<"frame_count=1\n";
    std::cout<<"evidence_count=1\n";
    return 0;
}
