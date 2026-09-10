#include "tile_native_dng_source_v0_1.h"
#include "streaming_test_support_v0_1.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

using namespace truthraw::tile_dng_v0_1;

namespace {
class DngMemSource final : public IRandomAccessByteSource {
public:
    explicit DngMemSource(std::vector<std::uint8_t> b) : bytes_(std::move(b)) {}
    std::uint64_t sizeBytes() const override { return bytes_.size(); }
    std::size_t residentBytesUpperBound() const override { return sizeof(*this) + bytes_.capacity(); }
    bool readExact(std::uint64_t o, void* d, std::size_t n) override {
        if (o > bytes_.size() || n > bytes_.size() - o) return false;
        std::memcpy(d, bytes_.data() + o, n);
        return true;
    }
private:
    std::vector<std::uint8_t> bytes_;
};

void p16(std::vector<std::uint8_t>& b,std::size_t o,std::uint16_t v){if(b.size()<o+2)b.resize(o+2);b[o]=std::uint8_t(v);b[o+1]=std::uint8_t(v>>8);}
void p32(std::vector<std::uint8_t>& b,std::size_t o,std::uint32_t v){if(b.size()<o+4)b.resize(o+4);for(int i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(8*i));}
void p64(std::vector<std::uint8_t>& b,std::size_t o,std::uint64_t v){if(b.size()<o+8)b.resize(o+8);for(int i=0;i<8;++i)b[o+i]=std::uint8_t(v>>(8*i));}
std::vector<std::uint8_t> shorts(std::initializer_list<std::uint16_t> xs){std::vector<std::uint8_t>b(xs.size()*2);std::size_t p=0;for(auto v:xs){p16(b,p,v);p+=2;}return b;}
std::vector<std::uint8_t> longs(const std::vector<std::uint32_t>& xs){std::vector<std::uint8_t>b(xs.size()*4);for(std::size_t i=0;i<xs.size();++i)p32(b,4*i,xs[i]);return b;}
std::vector<std::uint8_t> rationals(const std::array<std::uint32_t,4>& xs){std::vector<std::uint8_t>b(32);for(int i=0;i<4;++i){p32(b,8*i,xs[i]);p32(b,8*i+4,1);}return b;}
std::vector<std::uint8_t> doubles(const std::array<double,6>& xs){std::vector<std::uint8_t>b(48);for(int i=0;i<6;++i){std::uint64_t u=0;std::memcpy(&u,&xs[i],8);p64(b,8*i,u);}return b;}
struct E{std::uint16_t tag,type;std::uint32_t count;std::vector<std::uint8_t> data;std::size_t q=0,payload=0;};

std::vector<std::uint8_t> make_dng(int w,int h,const std::vector<std::uint16_t>&raw){
    std::vector<E> es;auto add=[&](std::uint16_t t,std::uint16_t ty,std::uint32_t c,std::vector<std::uint8_t>d){es.push_back({t,ty,c,std::move(d),0,0});};
    add(256,4,1,longs({std::uint32_t(w)}));add(257,4,1,longs({std::uint32_t(h)}));add(258,3,1,shorts({16}));add(259,3,1,shorts({1}));add(262,3,1,shorts({32803}));add(274,3,1,shorts({1}));add(277,3,1,shorts({1}));add(284,3,1,shorts({1}));add(339,3,1,shorts({1}));
    add(33421,3,2,shorts({2,2}));add(33422,1,4,{2,1,1,0});add(50710,1,3,{0,1,2});add(50713,3,2,shorts({2,2}));add(50714,5,4,rationals({64,65,66,67}));add(50717,4,1,longs({1023}));add(51041,12,6,doubles({0.0009,1e-6,0.0010,1.2e-6,0.0011,1.4e-6}));
    const std::uint32_t rps=2,strips=std::uint32_t((h+1)/2);add(273,4,strips,std::vector<std::uint8_t>(std::size_t(strips)*4));add(278,4,1,longs({rps}));add(279,4,strips,std::vector<std::uint8_t>(std::size_t(strips)*4));
    std::sort(es.begin(),es.end(),[](const E&a,const E&b){return a.tag<b.tag;});std::vector<std::uint8_t>b(8+2+es.size()*12+4,0);b[0]='I';b[1]='I';p16(b,2,42);p32(b,4,8);p16(b,8,std::uint16_t(es.size()));std::size_t ext=b.size();
    for(std::size_t i=0;i<es.size();++i){auto&e=es[i];e.q=10+12*i;p16(b,e.q,e.tag);p16(b,e.q+2,e.type);p32(b,e.q+4,e.count);if(e.data.size()<=4)std::copy(e.data.begin(),e.data.end(),b.begin()+e.q+8);else{e.payload=ext;p32(b,e.q+8,std::uint32_t(ext));b.insert(b.end(),e.data.begin(),e.data.end());ext=b.size();}}
    std::vector<std::uint32_t>offs,counts;for(std::uint32_t s=0;s<strips;++s){int y0=int(s*rps),rows=std::min<int>(rps,h-y0);offs.push_back(std::uint32_t(b.size()));counts.push_back(std::uint32_t(rows*w*2));for(int y=y0;y<y0+rows;++y)for(int x=0;x<w;++x){auto o=b.size();b.resize(o+2);p16(b,o,raw[std::size_t(y)*w+x]);}}
    for(auto&e:es){if(e.tag==273){auto d=longs(offs);std::copy(d.begin(),d.end(),b.begin()+e.payload);}if(e.tag==279){auto d=longs(counts);std::copy(d.begin(),d.end(),b.begin()+e.payload);}}
    return b;
}
}

int main(){
    // The upstream support header intentionally exposes a TU-local fixture helper.
    // Exercise it rather than weakening -Werror for unused support code.
    { auto supportFixture = make_frame(2,2); REQUIRE(supportFixture.meta.width==2 && supportFixture.meta.height==2); }
    const int w=66,h=50;DecodedDngFrame frame;frame.meta.width=w;frame.meta.height=h;frame.meta.cfa=CfaPattern::BGGR;frame.meta.orientation=Orientation::Normal;frame.meta.whiteLevel=1023.f;frame.meta.blackPhase={64,65,66,67};frame.meta.noiseProfile={0.0009f,1e-6f,0.0010f,1.2e-6f,0.0011f,1.4e-6f};frame.meta.hasNoiseProfile=true;frame.meta.hasGainField=false;frame.meta.hasResidualBlack=false;frame.meta.cameraToXyzD50={0.62f,0.21f,0.08f,0.18f,0.71f,0.07f,0.03f,0.12f,0.79f};frame.meta.sourceId="tile_dng_streaming_fixture";frame.raw.resize(std::size_t(w)*h);for(int y=0;y<h;++y)for(int x=0;x<w;++x){int v=70+((x*37+y*53+x*y*3)%900);if((x+y)%97==0)v=1023;frame.raw[std::size_t(y)*w+x]=std::uint16_t(v);}
    auto bytes=std::make_shared<DngMemSource>(make_dng(w,h,frame.raw));OpenOptions oo;oo.sourceEvidenceId=frame.meta.sourceId;oo.color.valid=true;oo.color.bindingId="fixture_color_binding";oo.color.cameraToXyzD50=frame.meta.cameraToXyzD50;std::unique_ptr<TileNativeDngSource>source;auto os=TileNativeDngSource::open(bytes,oo,source);REQUIRE(os);REQUIRE(source->metadata().noiseProfile==frame.meta.noiseProfile);
    auto recon=std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();auto appearance=std::make_shared<SkinSafeDetailedCrispAppearance>();ProcessOptions po;po.tile={16,7};po.threads=1;po.hdrEnabled=true;po.appearance=AppearanceProfile::SkinSafeDetailedCrisp;po.keepScientificDiagnostics=true;po.sdrLutSize=4096;ProcessResult canonical;TruthRawProcessor cp(recon,appearance);REQUIRE(cp.processFrame(frame,po,canonical));
    CollectSink sink(w,h,true);StreamingOptions so;so.tile={16,7};so.workers=1;so.hdrEnabled=true;so.streamScientificDiagnostics=true;so.sdrLutSize=4096;StreamingResult sr;StreamingTruthRawProcessor sp(recon,appearance);REQUIRE(sp.process(*source,sink,so,sr));REQUIRE(sink.finished());compare_exposure(canonical.exposure,sr.exposure);float sd=max_abs_diff(canonical.sdrRgb,sink.sdr()),gd=max_abs_diff(canonical.halfLogGain,sink.gain()),dd=max_abs_diff(canonical.stage2Diagnostic,sink.diagnostic());REQUIRE(sd==0.f);REQUIRE(gd==0.f);REQUIRE(dd==0.f);REQUIRE(source->audit().rawPayloadBytesRead<2ull*std::uint64_t(w)*h*std::uint64_t(sr.tilesProcessedPass1+sr.tilesProcessedPass2));REQUIRE(!source->audit().fullRawMaterialized&&!source->audit().fullFileMaterialized);
    std::cout<<"TILE_NATIVE_DNG_STREAMING_INTEGRATION_V0_1_PASS\n"<<"sdr_max_abs="<<sd<<"\nhalf_gain_max_abs="<<gd<<"\nstage2_diag_max_abs="<<dd<<"\nsource_resident_bytes="<<source->residentBytesUpperBound()<<"\nraw_payload_bytes_read="<<source->audit().rawPayloadBytesRead<<"\n";
}
