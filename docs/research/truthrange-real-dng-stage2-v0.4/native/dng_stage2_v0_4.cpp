#include "dng_stage2_v0_4.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace truthraw_v04 {
namespace {
[[noreturn]] void fail(const char* s){ throw std::runtime_error(s); }
std::uint16_t rd16(const std::vector<std::uint8_t>& b,std::size_t o,bool le){
    if(o+2>b.size()) fail("truncated u16");
    return le ? std::uint16_t(b[o]|(std::uint16_t(b[o+1])<<8)) : std::uint16_t((std::uint16_t(b[o])<<8)|b[o+1]);
}
std::uint32_t rd32(const std::vector<std::uint8_t>& b,std::size_t o,bool le){
    if(o+4>b.size()) fail("truncated u32");
    if(le) return std::uint32_t(b[o])|(std::uint32_t(b[o+1])<<8)|(std::uint32_t(b[o+2])<<16)|(std::uint32_t(b[o+3])<<24);
    return (std::uint32_t(b[o])<<24)|(std::uint32_t(b[o+1])<<16)|(std::uint32_t(b[o+2])<<8)|std::uint32_t(b[o+3]);
}
std::uint32_t be32(const std::vector<std::uint8_t>& b,std::size_t o){return rd32(b,o,false);}
std::int32_t bei32(const std::vector<std::uint8_t>& b,std::size_t o){return static_cast<std::int32_t>(be32(b,o));}
double be64f(const std::vector<std::uint8_t>& b,std::size_t o){
    if(o+8>b.size()) fail("truncated f64");
    std::uint64_t u=0; for(int i=0;i<8;++i) u=(u<<8)|b[o+i]; double x; std::memcpy(&x,&u,8); return x;
}
float be32f(const std::vector<std::uint8_t>& b,std::size_t o){
    std::uint32_t u=be32(b,o); float x; std::memcpy(&x,&u,4); return x;
}
std::size_t typeSize(std::uint16_t t){ switch(t){case 1:case 2:case 7:return 1;case 3:return 2;case 4:case 9:case 11:return 4;case 5:case 10:case 12:return 8;default:return 0;} }
struct Ent{std::uint16_t type=0;std::uint32_t count=0;std::vector<std::uint8_t> raw;};
std::vector<std::uint8_t> load(const std::string& p){std::ifstream f(p,std::ios::binary);if(!f)fail("cannot open DNG");f.seekg(0,std::ios::end);auto n=f.tellg();if(n<0)fail("size");std::vector<std::uint8_t>b(static_cast<std::size_t>(n));f.seekg(0);f.read(reinterpret_cast<char*>(b.data()),static_cast<std::streamsize>(b.size()));if(!f)fail("read DNG");return b;}
std::vector<std::uint32_t> uints(const Ent&e,bool le){
    std::vector<std::uint32_t> o; o.reserve(e.count);
    if(e.type==3){for(std::uint32_t i=0;i<e.count;++i)o.push_back(rd16(e.raw,2ull*i,le));}
    else if(e.type==4){for(std::uint32_t i=0;i<e.count;++i)o.push_back(rd32(e.raw,4ull*i,le));}
    else { fail("integer TIFF type unsupported"); }
    return o;
}
float rational1(const std::vector<std::uint8_t>&b,std::size_t o,bool le){auto n=rd32(b,o,le),d=rd32(b,o+4,le);if(!d)fail("zero rational denominator");return float(double(n)/double(d));}
}

bool GainMapOpcode::applies(int y,int x) const{
    return y>=area[0]&&y<area[2]&&x>=area[1]&&x<area[3]&&((y-area[0])%int(rowPitch)==0)&&((x-area[1])%int(colPitch)==0);
}
float GainMapOpcode::entry(std::uint32_t r,std::uint32_t c,std::uint32_t p) const{
    if(r>=pointsV||c>=pointsH||p>=mapPlanes) { fail("GainMap entry OOB"); }
    return values[(std::size_t(r)*pointsH+c)*mapPlanes+p];
}
float GainMapOpcode::interpolate(int y,int x,int H,int W,std::uint32_t p) const{
    if(H<=0 || W<=0 || !(spacingV>0.0) || !(spacingH>0.0)) { fail("invalid interpolation geometry"); }
    double rf=(((double(y)+0.5)/double(H))-originV)/spacingV;
    double cf=(((double(x)+0.5)/double(W))-originH)/spacingH;
    if(!std::isfinite(rf)) rf=0;
    if(!std::isfinite(cf)) cf=0;
    rf=std::max(0.0,std::min(double(pointsV-1),rf)); cf=std::max(0.0,std::min(double(pointsH-1),cf));
    auto r0=std::uint32_t(rf), c0=std::uint32_t(cf); auto r1=std::min(r0+1,pointsV-1), c1=std::min(c0+1,pointsH-1);
    float fr=float(rf-double(r0));
    // Mirror SDK: row interpolation returns real32, then column base/delta are converted back to real32.
    float a=entry(r0,c0,p)*(1.0f-fr)+entry(r1,c0,p)*fr;
    float b=entry(r0,c1,p)*(1.0f-fr)+entry(r1,c1,p)*fr;
    double base=a, delta=double(b)-base;
    float valueBase=float(base+delta*(cf-double(c0)));
    return valueBase;
}

std::vector<GainMapOpcode> parseOpcodeList2(const std::vector<std::uint8_t>& b){
    if(b.size()<4) { fail("OpcodeList2 too short"); }
    std::size_t p=0; auto count=be32(b,p); p+=4;
    if(count>(b.size()-4)/16) { fail("invalid opcode count"); }
    std::vector<GainMapOpcode> out; out.reserve(count);
    for(std::uint32_t i=0;i<count;++i){
        if(p+16>b.size()) { fail("truncated opcode header"); }
        GainMapOpcode g; g.opcodeId=be32(b,p);g.minVersion=be32(b,p+4);g.flags=be32(b,p+8);auto size=be32(b,p+12);p+=16;
        if(size>b.size()-p) { fail("invalid opcode data size"); }
        auto end=p+size;
        if(g.opcodeId!=9) { fail("non-GainMap opcode rejected"); }
        if(size<76) { fail("truncated GainMap"); }
        g.area={bei32(b,p),bei32(b,p+4),bei32(b,p+8),bei32(b,p+12)};g.plane=be32(b,p+16);g.planes=be32(b,p+20);g.rowPitch=be32(b,p+24);g.colPitch=be32(b,p+28);p+=32;
        if(g.area[2]<=g.area[0]||g.area[3]<=g.area[1]||g.planes<1||g.rowPitch<1||g.colPitch<1)fail("invalid area spec");
        g.pointsV=be32(b,p);g.pointsH=be32(b,p+4);p+=8;g.spacingV=be64f(b,p);g.spacingH=be64f(b,p+8);p+=16;g.originV=be64f(b,p);g.originH=be64f(b,p+8);p+=16;g.mapPlanes=be32(b,p);p+=4;
        if(g.pointsV<1||g.pointsH<1||g.mapPlanes<1||!std::isfinite(g.spacingV)||!std::isfinite(g.spacingH)||!std::isfinite(g.originV)||!std::isfinite(g.originH)||g.spacingV<=0||g.spacingH<=0)fail("invalid GainMap geometry");
        auto n=std::size_t(g.pointsV)*g.pointsH*g.mapPlanes;if(n>(end-p)/4||p+n*4!=end)fail("GainMap payload mismatch");g.values.resize(n);for(std::size_t j=0;j<n;++j){g.values[j]=be32f(b,p+4*j);if(!std::isfinite(g.values[j]))fail("non-finite gain");}p=end;out.push_back(std::move(g));
    }
    if(p!=b.size()) { fail("OpcodeList2 not fully consumed"); }
    return out;
}

ClassicDng readClassicDng(const std::string& path){
    auto b=load(path); if(b.size()<8)fail("short TIFF"); bool le;if(b[0]=='I'&&b[1]=='I')le=true;else if(b[0]=='M'&&b[1]=='M')le=false;else fail("bad TIFF byte order"); if(rd16(b,2,le)!=42)fail("not classic TIFF");
    auto ifdOff=rd32(b,4,le); if(ifdOff+2>b.size())fail("IFD OOB"); auto n=rd16(b,ifdOff,le); auto base=std::size_t(ifdOff)+2;if(base+std::size_t(n)*12+4>b.size())fail("truncated IFD");
    std::vector<std::pair<std::uint16_t,Ent>> es;es.reserve(n);
    for(std::uint16_t i=0;i<n;++i){auto q=base+std::size_t(i)*12;auto tag=rd16(b,q,le),typ=rd16(b,q+2,le);auto cnt=rd32(b,q+4,le);auto ts=typeSize(typ);if(!ts)continue;auto sz=std::size_t(cnt)*ts;Ent e;e.type=typ;e.count=cnt;if(sz<=4)e.raw.assign(b.begin()+q+8,b.begin()+q+8+sz);else{auto off=rd32(b,q+8,le);if(std::size_t(off)+sz>b.size())fail("tag data OOB");e.raw.assign(b.begin()+off,b.begin()+off+sz);}es.push_back({tag,std::move(e)});}
    auto get=[&](std::uint16_t tag)->const Ent&{for(auto&x:es)if(x.first==tag)return x.second;fail("missing required TIFF/DNG tag");};
    ClassicDng d;d.littleEndian=le;
    auto one=[&](std::uint16_t tag){auto v=uints(get(tag),le);if(v.size()!=1)fail("scalar count");return v[0];};
    d.width=int(one(256));d.height=int(one(257));d.bitsPerSample=std::uint16_t(one(258));d.compression=std::uint16_t(one(259));d.photometric=std::uint16_t(one(262));d.stripOffsets=uints(get(273),le);d.samplesPerPixel=std::uint16_t(one(277));d.rowsPerStrip=one(278);d.stripByteCounts=uints(get(279),le);
    auto cr=uints(get(33421),le);if(cr.size()!=2)fail("CFA repeat");d.cfaRepeat={std::uint16_t(cr[0]),std::uint16_t(cr[1])};auto&cp=get(33422);if(cp.type!=1||cp.count!=4)fail("CFA pattern");std::copy(cp.raw.begin(),cp.raw.end(),d.cfaPattern.begin());
    auto&bl=get(50714);if(bl.type!=5||bl.count!=4)fail("BlackLevel layout");for(int i=0;i<4;++i)d.blackPhase[i]=rational1(bl.raw,8ull*i,le);d.whiteLevel=float(one(50717));auto&ol=get(51009);if(ol.type!=7)fail("OpcodeList2 type");d.opcodeList2=ol.raw;
    if(d.width<=0||d.height<=0||d.bitsPerSample!=16||d.compression!=1||d.photometric!=32803||d.samplesPerPixel!=1||d.rowsPerStrip!=1||d.cfaRepeat!=std::array<std::uint16_t,2>{2,2}||d.stripOffsets.size()!=std::size_t(d.height)||d.stripByteCounts.size()!=std::size_t(d.height))fail("unsupported v0.4 source layout");
    d.gainMaps=parseOpcodeList2(d.opcodeList2);for(int py=0;py<2;++py)for(int px=0;px<2;++px){int c=0;for(auto&g:d.gainMaps)c+=g.applies(py,px);if(c!=1)fail("GainMap CFA phase coverage");}
    d.raw.resize(std::size_t(d.width)*d.height);for(int y=0;y<d.height;++y){auto off=d.stripOffsets[y],bc=d.stripByteCounts[y];if(bc!=std::uint32_t(d.width*2)||std::size_t(off)+bc>b.size())fail("bad strip");for(int x=0;x<d.width;++x)d.raw[std::size_t(y)*d.width+x]=rd16(b,std::size_t(off)+2ull*x,le);}
    return d;
}
float ClassicDng::gainAt(int y,int x) const{const GainMapOpcode* m=nullptr;for(auto&g:gainMaps)if(g.applies(y,x)){if(m)fail("multiple GainMaps at pixel");m=&g;}if(!m)fail("no GainMap at pixel");return m->interpolate(y,x,height,width,0);}
float ClassicDng::stage2At(int y,int x) const{auto i=std::size_t(y)*width+x;float bl=blackPhase[phaseIndex(y,x)];float denom=std::max(whiteLevel-bl,1.0f);float s=(float(raw[i])-bl)/denom;return s*gainAt(y,x);}
Stage2Summary summarizeStage2(const ClassicDng& d){Stage2Summary s;s.total=std::uint64_t(d.width)*d.height;double sum=0;bool first=true;for(int y=0;y<d.height;++y)for(int x=0;x<d.width;++x){float v=d.stage2At(y,x);if(first){s.minValue=s.maxValue=v;first=false;}else{s.minValue=std::min(s.minValue,double(v));s.maxValue=std::max(s.maxValue,double(v));}sum+=v;std::uint32_t bits=0;std::memcpy(&bits,&v,4);s.floatBitsSum+=bits;s.floatBitsXor^=bits;s.negativeCount+=v<0;s.over1Count+=v>1.0f;s.clipCount+=float(d.raw[std::size_t(y)*d.width+x])>=d.whiteLevel;}s.mean=sum/double(s.total);return s;}
} // namespace truthraw_v04
