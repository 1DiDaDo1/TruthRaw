#include "multiworker_streaming_v0_2.h"
#include "tile_native_dng_source_v0_1.h"
#include "streaming_test_support_v0_1.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

using namespace truthraw::tile_dng_v0_1;
using truthraw::streaming_v0_2::MultiWorkerTelemetry;
using truthraw::streaming_v0_2::process_multiworker_streaming;

namespace {

class DngMemSource final : public IRandomAccessByteSource {
public:
    explicit DngMemSource(std::vector<std::uint8_t> b) : bytes_(std::move(b)) {}
    std::uint64_t sizeBytes() const override { return bytes_.size(); }
    std::size_t residentBytesUpperBound() const override { return sizeof(*this) + bytes_.capacity(); }
    bool readExact(std::uint64_t o, void* d, std::size_t n) override {
        if (o > bytes_.size() || n > bytes_.size() - std::size_t(o)) return false;
        std::memcpy(d, bytes_.data() + std::size_t(o), n);
        return true;
    }
private:
    std::vector<std::uint8_t> bytes_;
};

void p16(std::vector<std::uint8_t>& b,std::size_t o,std::uint16_t v){if(b.size()<o+2)b.resize(o+2);b[o]=std::uint8_t(v);b[o+1]=std::uint8_t(v>>8);}
void p32(std::vector<std::uint8_t>& b,std::size_t o,std::uint32_t v){if(b.size()<o+4)b.resize(o+4);for(int i=0;i<4;++i)b[o+std::size_t(i)]=std::uint8_t(v>>(8*i));}
void p64(std::vector<std::uint8_t>& b,std::size_t o,std::uint64_t v){if(b.size()<o+8)b.resize(o+8);for(int i=0;i<8;++i)b[o+std::size_t(i)]=std::uint8_t(v>>(8*i));}
std::vector<std::uint8_t> shorts(std::initializer_list<std::uint16_t> xs){std::vector<std::uint8_t>b(xs.size()*2);std::size_t p=0;for(auto v:xs){p16(b,p,v);p+=2;}return b;}
std::vector<std::uint8_t> longs(const std::vector<std::uint32_t>& xs){std::vector<std::uint8_t>b(xs.size()*4);for(std::size_t i=0;i<xs.size();++i)p32(b,4*i,xs[i]);return b;}
std::vector<std::uint8_t> rationals(const std::array<std::uint32_t,4>& xs){std::vector<std::uint8_t>b(32);for(int i=0;i<4;++i){p32(b,std::size_t(8*i),xs[std::size_t(i)]);p32(b,std::size_t(8*i+4),1);}return b;}
std::vector<std::uint8_t> doubles(const std::array<double,6>& xs){std::vector<std::uint8_t>b(48);for(int i=0;i<6;++i){std::uint64_t u=0;std::memcpy(&u,&xs[std::size_t(i)],8);p64(b,std::size_t(8*i),u);}return b;}
struct E{std::uint16_t tag,type;std::uint32_t count;std::vector<std::uint8_t> data;std::size_t q=0,payload=0;};

std::vector<std::uint8_t> make_dng(int w,int h,const std::vector<std::uint16_t>&raw){
    std::vector<E> es;auto add=[&](std::uint16_t t,std::uint16_t ty,std::uint32_t c,std::vector<std::uint8_t>d){es.push_back({t,ty,c,std::move(d),0,0});};
    add(256,4,1,longs({std::uint32_t(w)}));add(257,4,1,longs({std::uint32_t(h)}));add(258,3,1,shorts({16}));add(259,3,1,shorts({1}));add(262,3,1,shorts({32803}));add(274,3,1,shorts({1}));add(277,3,1,shorts({1}));add(284,3,1,shorts({1}));add(339,3,1,shorts({1}));
    add(33421,3,2,shorts({2,2}));add(33422,1,4,{2,1,1,0});add(50710,1,3,{0,1,2});add(50713,3,2,shorts({2,2}));add(50714,5,4,rationals({64,65,66,67}));add(50717,4,1,longs({1023}));add(51041,12,6,doubles({0.0009,1e-6,0.0010,1.2e-6,0.0011,1.4e-6}));
    const std::uint32_t rps=2,strips=std::uint32_t((h+1)/2);add(273,4,strips,std::vector<std::uint8_t>(std::size_t(strips)*4));add(278,4,1,longs({rps}));add(279,4,strips,std::vector<std::uint8_t>(std::size_t(strips)*4));
    std::sort(es.begin(),es.end(),[](const E&a,const E&b){return a.tag<b.tag;});std::vector<std::uint8_t>b(8+2+es.size()*12+4,0);b[0]='I';b[1]='I';p16(b,2,42);p32(b,4,8);p16(b,8,std::uint16_t(es.size()));std::size_t ext=b.size();
    for(std::size_t i=0;i<es.size();++i){auto&e=es[i];e.q=10+12*i;p16(b,e.q,e.tag);p16(b,e.q+2,e.type);p32(b,e.q+4,e.count);if(e.data.size()<=4)std::copy(e.data.begin(),e.data.end(),b.begin()+std::ptrdiff_t(e.q+8));else{e.payload=ext;p32(b,e.q+8,std::uint32_t(ext));b.insert(b.end(),e.data.begin(),e.data.end());ext=b.size();}}
    std::vector<std::uint32_t>offs,counts;for(std::uint32_t s=0;s<strips;++s){int y0=int(s*rps),rows=std::min<int>(int(rps),h-y0);offs.push_back(std::uint32_t(b.size()));counts.push_back(std::uint32_t(rows*w*2));for(int y=y0;y<y0+rows;++y)for(int x=0;x<w;++x){auto o=b.size();b.resize(o+2);p16(b,o,raw[std::size_t(y)*std::size_t(w)+std::size_t(x)]);}}
    for(auto&e:es){if(e.tag==273){auto d=longs(offs);std::copy(d.begin(),d.end(),b.begin()+std::ptrdiff_t(e.payload));}if(e.tag==279){auto d=longs(counts);std::copy(d.begin(),d.end(),b.begin()+std::ptrdiff_t(e.payload));}}
    return b;
}

std::unique_ptr<TileNativeDngSource> open_source(const std::vector<std::uint8_t>& bytes,
                                                  const std::array<float,9>& matrix) {
    auto mem=std::make_shared<DngMemSource>(bytes);OpenOptions oo;oo.sourceEvidenceId="tile_dng_multiworker_streaming_fixture";oo.color.valid=true;oo.color.bindingId="fixture_color_binding";oo.color.cameraToXyzD50=matrix;std::unique_ptr<TileNativeDngSource> source;auto os=TileNativeDngSource::open(mem,oo,source);REQUIRE(os);return source;
}

class ProbeSource final : public IRawTileSource {
public:
    explicit ProbeSource(IRawTileSource& inner):inner_(inner){}
    const DngMetadata& metadata()const override{return inner_.metadata();}
    std::size_t residentBytesUpperBound()const override{return inner_.residentBytesUpperBound();}
    StreamStatus readRawTile(const TileRect&r,std::uint16_t*raw,std::size_t rawN,float*gain,std::size_t gainN)override{enter();auto s=inner_.readRawTile(r,raw,rawN,gain,gainN);leave();return s;}
    StreamStatus readRowBias(int y0,int y1,float*out,std::size_t n)override{enter();auto s=inner_.readRowBias(y0,y1,out,n);leave();return s;}
    StreamStatus readColBias(int x0,int x1,float*out,std::size_t n)override{enter();auto s=inner_.readColBias(x0,x1,out,n);leave();return s;}
    int peak()const{return peak_.load();}
private:
    void enter(){const int now=active_.fetch_add(1)+1;int old=peak_.load();while(now>old&&!peak_.compare_exchange_weak(old,now)){} }
    void leave(){active_.fetch_sub(1);} IRawTileSource& inner_;std::atomic<int>active_{0},peak_{0};
};

void exact_float_bytes(const std::vector<float>&a,const std::vector<float>&b){REQUIRE(a.size()==b.size());REQUIRE(a.empty()||std::memcmp(a.data(),b.data(),a.size()*sizeof(float))==0);}

struct Output { StreamingResult result; std::vector<float>sdr,gain,diag; SourceAudit audit; int sourcePeak=0; MultiWorkerTelemetry telemetry; };

Output reference_run(const std::vector<std::uint8_t>& bytes,const std::array<float,9>& matrix,int w,int h){
    auto source=open_source(bytes,matrix);auto recon=std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();auto appearance=std::make_shared<SkinSafeDetailedCrispAppearance>();CollectSink sink(w,h,true);StreamingTruthRawProcessor processor(recon,appearance);StreamingOptions o;o.tile={32,7};o.workers=1;o.hdrEnabled=true;o.streamScientificDiagnostics=true;StreamingResult r;auto st=processor.process(*source,sink,o,r);REQUIRE(st);REQUIRE(sink.finished());return {r,sink.sdr(),sink.gain(),sink.diagnostic(),source->audit(),1,{}};
}

Output candidate_run(const std::vector<std::uint8_t>& bytes,const std::array<float,9>& matrix,int w,int h,int workers){
    auto source=open_source(bytes,matrix);ProbeSource probe(*source);ResearchEdgeAwareMeasuredPreservingReconstruction recon;SkinSafeDetailedCrispAppearance appearance;CollectSink sink(w,h,true);StreamingOptions o;o.tile={32,7};o.workers=workers;o.hdrEnabled=true;o.streamScientificDiagnostics=true;StreamingResult r;MultiWorkerTelemetry telemetry;auto st=process_multiworker_streaming(probe,sink,recon,appearance,o,r,telemetry);REQUIRE(st);REQUIRE(sink.finished());return {r,sink.sdr(),sink.gain(),sink.diagnostic(),source->audit(),probe.peak(),telemetry};
}

void compare(const Output& ref,const Output& c,int workers){compare_exposure(ref.result.exposure,c.result.exposure);REQUIRE(ref.result.stage2Over1Count==c.result.stage2Over1Count);REQUIRE(ref.result.clippedCount==c.result.clippedCount);REQUIRE(ref.result.tilesProcessedPass1==c.result.tilesProcessedPass1);REQUIRE(ref.result.tilesProcessedPass2==c.result.tilesProcessedPass2);exact_float_bytes(ref.sdr,c.sdr);exact_float_bytes(ref.gain,c.gain);exact_float_bytes(ref.diag,c.diag);REQUIRE(c.sourcePeak==1);REQUIRE(c.telemetry.effectiveWorkers==workers);REQUIRE(c.telemetry.orderedCommit);REQUIRE(c.telemetry.maxReadyPackets<=c.telemetry.queueDepth);REQUIRE(c.audit.tileReadCalls==c.result.tilesProcessedPass1+c.result.tilesProcessedPass2);REQUIRE(!c.audit.fullRawMaterialized&&!c.audit.fullFileMaterialized);REQUIRE(c.result.provenance.physicalFrameCount==1&&c.result.provenance.independentEvidenceCount==1);}

} // namespace

int main(){
    {auto fixture=make_frame(2,2);REQUIRE(fixture.meta.width==2);}
    const int w=258,h=194;std::vector<std::uint16_t>raw(std::size_t(w)*std::size_t(h));for(int y=0;y<h;++y)for(int x=0;x<w;++x){int v=70+((x*37+y*53+x*y*3)%900);if((x+y)%97==0)v=1023;raw[std::size_t(y)*std::size_t(w)+std::size_t(x)]=std::uint16_t(v);}const std::array<float,9>matrix={0.62f,0.21f,0.08f,0.18f,0.71f,0.07f,0.03f,0.12f,0.79f};const auto bytes=make_dng(w,h,raw);const auto ref=reference_run(bytes,matrix,w,h);const auto two=candidate_run(bytes,matrix,w,h,2);compare(ref,two,2);const auto four=candidate_run(bytes,matrix,w,h,4);compare(ref,four,4);std::cout<<"TILE_NATIVE_DNG_MULTIWORKER_STREAMING_V0_2_PASS\nsource_concurrency=1\nworkers_tested=2,4\nauthoritative_output_equivalence=EXACT_FLOAT_BYTES\n";
}
