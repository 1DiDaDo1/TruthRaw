#include "tile_native_dng_source_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <memory>
#include <stdexcept>
#include <vector>

using namespace truthraw;
using namespace truthraw::tile_dng_v0_1;

#define REQUIRE(x) do { if(!(x)) throw std::runtime_error(std::string("REQUIRE failed: ") + #x); } while(0)

namespace {
struct MemSource final : IRandomAccessByteSource {
    std::vector<std::uint8_t> b;
    explicit MemSource(std::vector<std::uint8_t> x):b(std::move(x)){}
    std::uint64_t sizeBytes() const override { return b.size(); }
    std::size_t residentBytesUpperBound() const override { return sizeof(*this)+b.capacity(); }
    bool readExact(std::uint64_t o,void*d,std::size_t n) override { if(o>b.size()||n>b.size()-o)return false;std::memcpy(d,b.data()+o,n);return true; }
};

struct SparseSource final : IRandomAccessByteSource {
    std::vector<std::uint8_t> prefix;
    std::uint64_t virtualSize=0;
    std::uint64_t rawBase=0;
    bool little=true;
    std::uint64_t sizeBytes() const override { return virtualSize; }
    std::size_t residentBytesUpperBound() const override { return sizeof(*this)+prefix.capacity(); }
    bool readExact(std::uint64_t o,void*d,std::size_t n) override {
        if(o>virtualSize||n>virtualSize-o)return false;
        auto* q=static_cast<std::uint8_t*>(d);
        for(std::size_t i=0;i<n;++i){
            auto pos=o+i;
            if(pos<prefix.size()){q[i]=prefix[std::size_t(pos)];continue;}
            if(pos>=rawBase){
                std::uint64_t sample=(pos-rawBase)/2;
                std::uint16_t v=std::uint16_t(64+(sample%900));
                bool hi=((pos-rawBase)&1)!=0;
                q[i]=little?(hi?std::uint8_t(v>>8):std::uint8_t(v)):(hi?std::uint8_t(v):std::uint8_t(v>>8));
            } else q[i]=0;
        }
        return true;
    }
};

void put16(std::vector<std::uint8_t>&b,std::size_t o,std::uint16_t v,bool le){if(b.size()<o+2)b.resize(o+2);if(le){b[o]=v&255;b[o+1]=v>>8;}else{b[o]=v>>8;b[o+1]=v&255;}}
void put32(std::vector<std::uint8_t>&b,std::size_t o,std::uint32_t v,bool le){if(b.size()<o+4)b.resize(o+4);if(le){for(int i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(8*i));}else{for(int i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(8*(3-i)));}}
void put64bits(std::vector<std::uint8_t>&b,std::size_t o,std::uint64_t v,bool le){if(b.size()<o+8)b.resize(o+8);if(le){for(int i=0;i<8;++i)b[o+i]=std::uint8_t(v>>(8*i));}else{for(int i=0;i<8;++i)b[o+i]=std::uint8_t(v>>(8*(7-i)));}}
std::vector<std::uint8_t> enc_short(std::initializer_list<std::uint16_t> xs,bool le){std::vector<std::uint8_t>b(xs.size()*2);std::size_t p=0;for(auto v:xs){put16(b,p,v,le);p+=2;}return b;}
std::vector<std::uint8_t> enc_long(const std::vector<std::uint32_t>&xs,bool le){std::vector<std::uint8_t>b(xs.size()*4);for(std::size_t i=0;i<xs.size();++i)put32(b,4*i,xs[i],le);return b;}
std::vector<std::uint8_t> enc_rational4(const std::array<std::uint32_t,4>&xs,bool le){std::vector<std::uint8_t>b(32);for(int i=0;i<4;++i){put32(b,8*i,xs[i],le);put32(b,8*i+4,1,le);}return b;}
std::vector<std::uint8_t> enc_doubles(const std::array<double,6>&xs,bool le){std::vector<std::uint8_t>b(48);for(int i=0;i<6;++i){std::uint64_t u=0;std::memcpy(&u,&xs[i],8);put64bits(b,8*i,u,le);}return b;}
void be32w(std::vector<std::uint8_t>&b,std::uint32_t v){std::size_t o=b.size();b.resize(o+4);b[o]=v>>24;b[o+1]=v>>16;b[o+2]=v>>8;b[o+3]=v;}
void be64dw(std::vector<std::uint8_t>&b,double d){std::uint64_t u=0;std::memcpy(&u,&d,8);for(int i=7;i>=0;--i)b.push_back(std::uint8_t(u>>(8*i)));}
void befloatw(std::vector<std::uint8_t>&b,float f){std::uint32_t u=0;std::memcpy(&u,&f,4);be32w(b,u);}

std::vector<std::uint8_t> gain_opcode_list(int w,int h){
    std::vector<std::uint8_t>b;be32w(b,4);
    for(int phase=0;phase<4;++phase){
        int py=phase/2,px=phase%2;std::vector<std::uint8_t>p;
        be32w(p,std::uint32_t(py));be32w(p,std::uint32_t(px));be32w(p,std::uint32_t(h));be32w(p,std::uint32_t(w));
        be32w(p,0);be32w(p,1);be32w(p,2);be32w(p,2);
        be32w(p,1);be32w(p,1);be64dw(p,1.0);be64dw(p,1.0);be64dw(p,0.0);be64dw(p,0.0);be32w(p,1);befloatw(p,1.0f+0.1f*phase);
        REQUIRE(p.size()==80);be32w(b,9);be32w(b,0x01030000);be32w(b,0);be32w(b,std::uint32_t(p.size()));b.insert(b.end(),p.begin(),p.end());
    }
    return b;
}

struct Entry {std::uint16_t tag=0,type=0;std::uint32_t count=0;std::vector<std::uint8_t> data;std::size_t entryPos=0;std::size_t payloadPos=0;};
struct Fixture {std::vector<std::uint8_t> bytes;std::vector<std::uint16_t> raw;std::uint32_t firstDataOffset=0;};

Fixture make_fixture(bool le,bool tiled,bool gain=true,std::uint16_t compression=1,std::uint16_t bits=16){
    const int w=7,h=6;std::vector<std::uint16_t> raw(std::size_t(w)*h);for(int y=0;y<h;++y)for(int x=0;x<w;++x)raw[std::size_t(y)*w+x]=std::uint16_t(64+y*100+x);
    std::vector<Entry> es;
    auto add=[&](std::uint16_t tag,std::uint16_t type,std::uint32_t count,std::vector<std::uint8_t>d){es.push_back({tag,type,count,std::move(d),0,0});};
    add(256,4,1,enc_long({std::uint32_t(w)},le));add(257,4,1,enc_long({std::uint32_t(h)},le));add(258,3,1,enc_short({bits},le));add(259,3,1,enc_short({compression},le));add(262,3,1,enc_short({32803},le));add(274,3,1,enc_short({1},le));add(277,3,1,enc_short({1},le));add(284,3,1,enc_short({1},le));add(339,3,1,enc_short({1},le));
    add(33421,3,2,enc_short({2,2},le));add(33422,1,4,{2,1,1,0});add(50710,1,3,{0,1,2});add(50713,3,2,enc_short({2,2},le));add(50714,5,4,enc_rational4({64,65,66,67},le));add(50717,4,1,enc_long({1023},le));add(51041,12,6,enc_doubles({1e-5,2e-6,1.1e-5,2.1e-6,1.2e-5,2.2e-6},le));if(gain){auto op=gain_opcode_list(w,h);const auto opCount=std::uint32_t(op.size());add(51009,7,opCount,std::move(op));}
    int nx=0,ny=0,striles=0;std::uint32_t rowsPer=0,tileW=0,tileH=0;
    if(!tiled){rowsPer=2;striles=(h+int(rowsPer)-1)/int(rowsPer);add(273,4,striles,std::vector<std::uint8_t>(std::size_t(striles)*4));add(278,4,1,enc_long({rowsPer},le));add(279,4,striles,std::vector<std::uint8_t>(std::size_t(striles)*4));}
    else{tileW=4;tileH=3;nx=(w+int(tileW)-1)/int(tileW);ny=(h+int(tileH)-1)/int(tileH);striles=nx*ny;add(322,4,1,enc_long({tileW},le));add(323,4,1,enc_long({tileH},le));add(324,4,striles,std::vector<std::uint8_t>(std::size_t(striles)*4));add(325,4,striles,std::vector<std::uint8_t>(std::size_t(striles)*4));}
    std::sort(es.begin(),es.end(),[](const Entry&a,const Entry&b){return a.tag<b.tag;});
    std::vector<std::uint8_t>b(8+2+es.size()*12+4,0);b[0]=le?'I':'M';b[1]=le?'I':'M';put16(b,2,42,le);put32(b,4,8,le);put16(b,8,std::uint16_t(es.size()),le);std::size_t external=b.size();
    for(std::size_t i=0;i<es.size();++i){auto&e=es[i];std::size_t q=10+i*12;e.entryPos=q;put16(b,q,e.tag,le);put16(b,q+2,e.type,le);put32(b,q+4,e.count,le);if(e.data.size()<=4){for(std::size_t j=0;j<e.data.size();++j)b[q+8+j]=e.data[j];}else{e.payloadPos=external;put32(b,q+8,std::uint32_t(external),le);b.insert(b.end(),e.data.begin(),e.data.end());external=b.size();}}
    std::vector<std::uint32_t> offsets,counts;
    std::uint32_t firstData=std::uint32_t(b.size());
    if(!tiled){for(int s=0;s<striles;++s){int y0=s*int(rowsPer),rows=std::min(int(rowsPer),h-y0);offsets.push_back(std::uint32_t(b.size()));counts.push_back(std::uint32_t(rows*w*2));for(int y=y0;y<y0+rows;++y)for(int x=0;x<w;++x){std::size_t o=b.size();b.resize(o+2);put16(b,o,raw[std::size_t(y)*w+x],le);}}}
    else{for(int ty=0;ty<ny;++ty)for(int tx=0;tx<nx;++tx){offsets.push_back(std::uint32_t(b.size()));counts.push_back(tileW*tileH*2);for(std::uint32_t ly=0;ly<tileH;++ly)for(std::uint32_t lx=0;lx<tileW;++lx){int x=tx*int(tileW)+int(lx),y=ty*int(tileH)+int(ly);std::uint16_t v=(x<w&&y<h)?raw[std::size_t(y)*w+x]:0xeeee;std::size_t o=b.size();b.resize(o+2);put16(b,o,v,le);}}}
    for(auto&e:es){if(e.tag==273||e.tag==324){auto d=enc_long(offsets,le);if(e.data.size()<=4)std::copy(d.begin(),d.end(),b.begin()+e.entryPos+8);else std::copy(d.begin(),d.end(),b.begin()+e.payloadPos);}if(e.tag==279||e.tag==325){auto d=enc_long(counts,le);if(e.data.size()<=4)std::copy(d.begin(),d.end(),b.begin()+e.entryPos+8);else std::copy(d.begin(),d.end(),b.begin()+e.payloadPos);}}
    return {std::move(b),std::move(raw),firstData};
}


std::shared_ptr<SparseSource> make_sparse_200mp_source(){
    constexpr std::uint32_t w=16320,h=12288,rawBase=4096;
    constexpr std::uint64_t rawBytes=std::uint64_t(w)*h*2ull;
    std::vector<Entry> es;
    auto add=[&](std::uint16_t tag,std::uint16_t type,std::uint32_t count,std::vector<std::uint8_t>d){es.push_back({tag,type,count,std::move(d),0,0});};
    const bool le=true;
    add(256,4,1,enc_long({w},le));add(257,4,1,enc_long({h},le));add(258,3,1,enc_short({16},le));add(259,3,1,enc_short({1},le));add(262,3,1,enc_short({32803},le));
    add(273,4,1,enc_long({rawBase},le));add(274,3,1,enc_short({1},le));add(277,3,1,enc_short({1},le));add(278,4,1,enc_long({h},le));add(279,4,1,enc_long({std::uint32_t(rawBytes)},le));add(284,3,1,enc_short({1},le));add(339,3,1,enc_short({1},le));
    add(33421,3,2,enc_short({2,2},le));add(33422,1,4,{2,1,1,0});add(50713,3,2,enc_short({2,2},le));add(50714,5,4,enc_rational4({64,64,64,64},le));add(50717,4,1,enc_long({1023},le));
    std::sort(es.begin(),es.end(),[](const Entry&a,const Entry&b){return a.tag<b.tag;});
    std::vector<std::uint8_t>b(8+2+es.size()*12+4,0);b[0]='I';b[1]='I';put16(b,2,42,le);put32(b,4,8,le);put16(b,8,std::uint16_t(es.size()),le);std::size_t external=b.size();
    for(std::size_t i=0;i<es.size();++i){auto&e=es[i];std::size_t q=10+i*12;e.entryPos=q;put16(b,q,e.tag,le);put16(b,q+2,e.type,le);put32(b,q+4,e.count,le);if(e.data.size()<=4){std::copy(e.data.begin(),e.data.end(),b.begin()+q+8);}else{e.payloadPos=external;put32(b,q+8,std::uint32_t(external),le);b.insert(b.end(),e.data.begin(),e.data.end());external=b.size();}}
    REQUIRE(b.size()<rawBase);b.resize(rawBase,0);
    auto out=std::make_shared<SparseSource>();out->prefix=std::move(b);out->virtualSize=std::uint64_t(rawBase)+rawBytes;out->rawBase=rawBase;out->little=true;return out;
}

std::size_t ifd_next_pointer_pos(const std::vector<std::uint8_t>&b,bool le){REQUIRE(b.size()>=10);auto n=le?std::uint16_t(b[8]|(std::uint16_t(b[9])<<8)):std::uint16_t((std::uint16_t(b[8])<<8)|b[9]);return 10+std::size_t(n)*12;}
void corrupt_first_strip_offset(std::vector<std::uint8_t>&b){
    bool le=true;auto n=std::uint16_t(b[8]|(std::uint16_t(b[9])<<8));
    for(std::uint16_t i=0;i<n;++i){std::size_t q=10+std::size_t(i)*12;auto tag=std::uint16_t(b[q]|(std::uint16_t(b[q+1])<<8));if(tag==273){auto count=std::uint32_t(b[q+4])|(std::uint32_t(b[q+5])<<8)|(std::uint32_t(b[q+6])<<16)|(std::uint32_t(b[q+7])<<24);REQUIRE(count>1);auto off=std::uint32_t(b[q+8])|(std::uint32_t(b[q+9])<<8)|(std::uint32_t(b[q+10])<<16)|(std::uint32_t(b[q+11])<<24);put32(b,off,std::uint32_t(b.size()+100),le);return;}}
    REQUIRE(false);
}

OpenOptions opts(){OpenOptions o;o.sourceEvidenceId="fixture-sha256";o.color.valid=true;o.color.bindingId="explicit-fixture-color";o.color.cameraToXyzD50={1,0,0,0,1,0,0,0,1};return o;}

void check_rect(TileNativeDngSource&src,const std::vector<std::uint16_t>&ref){TileRect r;r.x0=2;r.y0=2;r.x1=5;r.y1=4;r.hx0=1;r.hy0=1;r.hx1=6;r.hy1=5;std::size_t n=20;std::vector<std::uint16_t>raw(n);std::vector<float>gain(n);auto before=src.audit().rawPayloadBytesRead;auto s=src.readRawTile(r,raw.data(),raw.size(),src.metadata().hasGainField?gain.data():nullptr,gain.size());REQUIRE(s);for(int y=r.hy0;y<r.hy1;++y)for(int x=r.hx0;x<r.hx1;++x){std::size_t i=std::size_t(y-r.hy0)*(r.hx1-r.hx0)+(x-r.hx0);REQUIRE(raw[i]==ref[std::size_t(y)*src.metadata().width+x]);if(src.metadata().hasGainField){float want=1.f+0.1f*((y&1)*2+(x&1));REQUIRE(std::abs(gain[i]-want)<1e-6f);}}REQUIRE(src.audit().rawPayloadBytesRead-before==n*2);}
}

int main(){
    std::size_t maxFixtureResident=0;
    std::size_t sparseResident=0;
    std::uint64_t sparseMetadataRead=0;
    std::uint64_t sparseRawRead=0;
    for(bool le:{true,false})for(bool tiled:{false,true}){
        auto f=make_fixture(le,tiled,true);auto mem=std::make_shared<MemSource>(f.bytes);std::unique_ptr<TileNativeDngSource>src;auto st=TileNativeDngSource::open(mem,opts(),src);if(!st){std::cerr<<"OPEN_FAIL code="<<int(st.code)<<" msg="<<st.message<<" le="<<le<<" tiled="<<tiled<<"\n";return 2;}REQUIRE(src);REQUIRE(src->metadata().width==7&&src->metadata().height==6);REQUIRE(src->metadata().cfa==CfaPattern::BGGR);REQUIRE(src->metadata().whiteLevel==1023.f);REQUIRE(src->metadata().blackPhase[0]==64.f&&src->metadata().blackPhase[3]==67.f);REQUIRE(src->metadata().hasNoiseProfile);REQUIRE(src->metadata().hasGainField);REQUIRE(src->colorBindingId()=="explicit-fixture-color");REQUIRE(!src->audit().fullFileMaterialized&&!src->audit().fullRawMaterialized);REQUIRE(!src->audit().stripArraysMaterialized&&!src->audit().tileArraysMaterialized);REQUIRE(src->audit().rawPayloadBytesRead==0);check_rect(*src,f.raw);REQUIRE(src->residentBytesUpperBound()<128u*1024u);maxFixtureResident=std::max(maxFixtureResident,src->residentBytesUpperBound());std::array<float,3> rb{};REQUIRE(src->readRowBias(1,4,rb.data(),rb.size()));REQUIRE(rb[0]==0&&rb[2]==0);std::array<float,4> cb{};REQUIRE(src->readColBias(2,6,cb.data(),cb.size()));REQUIRE(cb[0]==0&&cb[3]==0);
    }
    {auto f=make_fixture(true,false,false,5,16);std::unique_ptr<TileNativeDngSource>s;auto st=TileNativeDngSource::open(std::make_shared<MemSource>(f.bytes),opts(),s);REQUIRE(!st&&st.code==DngSourceCode::UnsupportedCompression);}
    {auto f=make_fixture(true,false,false,1,12);std::unique_ptr<TileNativeDngSource>s;auto st=TileNativeDngSource::open(std::make_shared<MemSource>(f.bytes),opts(),s);REQUIRE(!st&&st.code==DngSourceCode::UnsupportedBitsPerSample);}
    {auto f=make_fixture(true,false,false);auto o=opts();o.color.valid=false;std::unique_ptr<TileNativeDngSource>s;auto st=TileNativeDngSource::open(std::make_shared<MemSource>(f.bytes),o,s);REQUIRE(!st&&st.code==DngSourceCode::BindingMissing);}
    {auto f=make_fixture(true,false,true);auto o=opts();o.maxResidentBytes=256;std::unique_ptr<TileNativeDngSource>s;auto st=TileNativeDngSource::open(std::make_shared<MemSource>(f.bytes),o,s);REQUIRE(!st&&st.code==DngSourceCode::BudgetExceeded);}
    {auto f=make_fixture(true,false,true);auto o=opts();o.maxOpcodeListBytes=64;std::unique_ptr<TileNativeDngSource>s;auto st=TileNativeDngSource::open(std::make_shared<MemSource>(f.bytes),o,s);REQUIRE(!st&&st.code==DngSourceCode::BudgetExceeded);}
    {auto f=make_fixture(true,false,false);auto o=opts();o.maxIfdEntries=1;std::unique_ptr<TileNativeDngSource>s;auto st=TileNativeDngSource::open(std::make_shared<MemSource>(f.bytes),o,s);REQUIRE(!st&&st.code==DngSourceCode::InvalidTiff);}
    {auto f=make_fixture(true,false,false);auto o=opts();o.sourceEvidenceId.clear();std::unique_ptr<TileNativeDngSource>s;auto st=TileNativeDngSource::open(std::make_shared<MemSource>(f.bytes),o,s);REQUIRE(!st&&st.code==DngSourceCode::BindingMissing);}
    {auto f=make_fixture(true,false,false);corrupt_first_strip_offset(f.bytes);std::unique_ptr<TileNativeDngSource>s;auto st=TileNativeDngSource::open(std::make_shared<MemSource>(f.bytes),opts(),s);REQUIRE(st);TileRect r;r.hx0=0;r.hy0=0;r.hx1=2;r.hy1=2;std::array<std::uint16_t,4> raw{};auto rs=s->readRawTile(r,raw.data(),raw.size(),nullptr,0);REQUIRE(!rs&&rs.code==streaming_v0_1::StreamStatusCode::SourceFailed);}
    {auto f=make_fixture(true,false,false);auto np=ifd_next_pointer_pos(f.bytes,true);auto dirBytes=np+4-8;auto dup=std::uint32_t(f.bytes.size());std::vector<std::uint8_t> dir(f.bytes.begin()+8,f.bytes.begin()+8+dirBytes);f.bytes.insert(f.bytes.end(),dir.begin(),dir.end());put32(f.bytes,np,dup,true);put32(f.bytes,dup+dirBytes-4,0,true);std::unique_ptr<TileNativeDngSource>s;auto st=TileNativeDngSource::open(std::make_shared<MemSource>(f.bytes),opts(),s);REQUIRE(!st&&st.code==DngSourceCode::AmbiguousRawIfd);auto o=opts();o.explicitRawIfdOffset=8;st=TileNativeDngSource::open(std::make_shared<MemSource>(f.bytes),o,s);REQUIRE(st);}
    {std::vector<std::uint8_t>b(16,0);b[0]='I';b[1]='I';put16(b,2,43,true);std::unique_ptr<TileNativeDngSource>s;auto st=TileNativeDngSource::open(std::make_shared<MemSource>(b),opts(),s);REQUIRE(!st&&st.code==DngSourceCode::UnsupportedBigTiff);}
    {auto sparse=make_sparse_200mp_source();std::unique_ptr<TileNativeDngSource>s;auto st=TileNativeDngSource::open(sparse,opts(),s);REQUIRE(st);REQUIRE(s->metadata().width==16320&&s->metadata().height==12288);REQUIRE(s->residentBytesUpperBound()<32u*1024u);REQUIRE(s->audit().metadataBytesRead<4096);REQUIRE(s->audit().rawPayloadBytesRead==0);sparseResident=s->residentBytesUpperBound();sparseMetadataRead=s->audit().metadataBytesRead;TileRect r;r.hx0=100;r.hy0=200;r.hx1=164;r.hy1=264;std::vector<std::uint16_t>raw(64*64);auto rs=s->readRawTile(r,raw.data(),raw.size(),nullptr,0);REQUIRE(rs);REQUIRE(s->audit().rawPayloadBytesRead==64u*64u*2u);sparseRawRead=s->audit().rawPayloadBytesRead;REQUIRE(s->residentBytesUpperBound()<32u*1024u);}
    {auto f=make_fixture(true,false,false);char path[]="/tmp/truthraw_dng_v01_XXXXXX";int fd=::mkstemp(path);REQUIRE(fd>=0);std::size_t done=0;while(done<f.bytes.size()){ssize_t n=::write(fd,f.bytes.data()+done,f.bytes.size()-done);REQUIRE(n>0);done+=std::size_t(n);}auto posix=std::make_shared<PosixFdByteSource>(fd);std::unique_ptr<TileNativeDngSource>s;auto st=TileNativeDngSource::open(posix,opts(),s);REQUIRE(st);check_rect(*s,f.raw);::close(fd);::unlink(path);}
    std::cout<<"TILE_NATIVE_DNG_SOURCE_V0_1_PASS\n";
    std::cout<<"little_big_endian=PASS\nstrips_tiles=PASS\nphase_gainmap=PASS\nnoise_profile=PASS\nfull_file_materialized=false\nfull_raw_materialized=false\nstrile_arrays_materialized=false\nlazy_locator_validation=true\nposix_pread_source=PASS\n";
    std::cout<<"max_small_fixture_resident_bytes="<<maxFixtureResident<<"\n";
    std::cout<<"sparse_200mp_resident_bytes="<<sparseResident<<"\n";
    std::cout<<"sparse_200mp_open_metadata_bytes_read="<<sparseMetadataRead<<"\n";
    std::cout<<"sparse_200mp_64x64_raw_bytes_read="<<sparseRawRead<<"\n";
}
