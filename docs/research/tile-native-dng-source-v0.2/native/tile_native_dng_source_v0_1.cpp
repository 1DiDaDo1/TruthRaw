#include "tile_native_dng_source_v0_1_internal.h"

#include <cmath>
#include <set>
#include <sys/stat.h>
#include <unistd.h>

namespace truthraw::tile_dng_v0_1 {
using namespace detail;
PosixFdByteSource::PosixFdByteSource(int fd):fd_(fd){
    struct stat st{}; if(fd_>=0 && ::fstat(fd_,&st)==0 && st.st_size>=0) size_=static_cast<std::uint64_t>(st.st_size);
}
std::uint64_t PosixFdByteSource::sizeBytes() const{return size_;}
bool PosixFdByteSource::readExact(std::uint64_t offset,void* dst,std::size_t count){
    if(fd_<0||offset>size_||count>size_-offset)return false;
    auto* p=static_cast<std::uint8_t*>(dst);std::size_t done=0;
    while(done<count){ssize_t n=::pread(fd_,p+done,count-done,static_cast<off_t>(offset+done));if(n<=0)return false;done+=static_cast<std::size_t>(n);}return true;
}

bool TileNativeDngSource::GainMap::applies(int y,int x) const{
    return y>=area[0]&&y<area[2]&&x>=area[1]&&x<area[3]&&rowPitch>0&&colPitch>0&&((y-area[0])%static_cast<int>(rowPitch)==0)&&((x-area[1])%static_cast<int>(colPitch)==0);
}
float TileNativeDngSource::GainMap::interpolate(int y,int x,int imageH,int imageW) const{
    if(pointsV<1||pointsH<1||mapPlanes<1||values.empty())return std::numeric_limits<float>::quiet_NaN();
    double rf=(((double(y)+0.5)/double(imageH))-originV)/spacingV;
    double cf=(((double(x)+0.5)/double(imageW))-originH)/spacingH;
    rf=std::clamp(rf,0.0,double(pointsV-1));cf=std::clamp(cf,0.0,double(pointsH-1));
    auto r0=static_cast<std::uint32_t>(rf),c0=static_cast<std::uint32_t>(cf);auto r1=std::min(r0+1,pointsV-1),c1=std::min(c0+1,pointsH-1);
    auto at=[&](std::uint32_t r,std::uint32_t c){return values[(std::size_t(r)*pointsH+c)*mapPlanes];};
    float fr=float(rf-double(r0));float a=at(r0,c0)*(1.f-fr)+at(r1,c0)*fr;float b=at(r0,c1)*(1.f-fr)+at(r1,c1)*fr;
    return float(double(a)+(double(b)-double(a))*(cf-double(c0)));
}

bool TileNativeDngSource::readBytes(std::uint64_t offset,void* dst,std::size_t n,bool rawPayload){
    if(!bytes_||offset>bytes_->sizeBytes()||n>bytes_->sizeBytes()-offset)return false;
    if(!bytes_->readExact(offset,dst,n))return false;
    audit_.fileBytesRead+=n;if(rawPayload)audit_.rawPayloadBytesRead+=n;else audit_.metadataBytesRead+=n;return true;
}
bool TileNativeDngSource::readTagBytes(const TagRef&t,std::uint64_t byteOffset,void*dst,std::size_t n){
    if (byteOffset > t.dataBytes || n > t.dataBytes - byteOffset) return false;
    return readBytes(t.dataOffset + byteOffset, dst, n, false);
}
bool TileNativeDngSource::readUnsigned(const TagRef&t,std::uint32_t index,std::uint32_t&out){
    if (index >= t.count) return false;
    if (t.type == TIFF_SHORT) { std::uint8_t b[2]; if (!readTagBytes(t, 2ull*index, b, 2)) return false; out = dec16(b, littleEndian_); return true; }
    if (t.type == TIFF_LONG) { std::uint8_t b[4]; if (!readTagBytes(t, 4ull*index, b, 4)) return false; out = dec32(b, littleEndian_); return true; }
    if (t.type == TIFF_BYTE) { std::uint8_t b=0; if (!readTagBytes(t, index, &b, 1)) return false; out=b; return true; }
    return false;
}
bool TileNativeDngSource::readFloatLike(const TagRef&t,std::uint32_t index,double&out){
    if(index>=t.count)return false;
    if(t.type==TIFF_SHORT||t.type==TIFF_LONG||t.type==TIFF_BYTE){std::uint32_t v=0;if(!readUnsigned(t,index,v))return false;out=v;return true;}
    if(t.type==TIFF_RATIONAL){std::uint8_t b[8];if(!readTagBytes(t,8ull*index,b,8))return false;auto n=dec32(b,littleEndian_),d=dec32(b+4,littleEndian_);if(!d)return false;out=double(n)/double(d);return std::isfinite(out);}
    if(t.type==TIFF_DOUBLE){std::uint8_t b[8];if(!readTagBytes(t,8ull*index,b,8))return false;std::uint64_t u=0;if(littleEndian_){for(int i=7;i>=0;--i)u=(u<<8)|b[i];}else{for(int i=0;i<8;++i)u=(u<<8)|b[i];}std::memcpy(&out,&u,8);return std::isfinite(out);}
    return false;
}
bool TileNativeDngSource::readStrileScalar(const TagRef&t,std::uint32_t index,std::uint32_t&out){return readUnsigned(t,index,out);}

const TileNativeDngSource::TagRef* TileNativeDngSource::findTag(const Ifd&ifd,std::uint16_t tag)const{for(const auto&t:ifd.tags)if(t.tag==tag)return&t;return nullptr;}

DngSourceStatus TileNativeDngSource::parseIfd(std::uint32_t offset,const OpenOptions&options,Ifd&out){
    if(offset==0||offset>bytes_->sizeBytes()||bytes_->sizeBytes()-offset<2)return DngSourceStatus::error(DngSourceCode::InvalidTiff,"IFD offset out of range");
    std::uint8_t h[2];if(!readBytes(offset,h,2))return DngSourceStatus::error(DngSourceCode::IoError,"cannot read IFD count");auto n=dec16(h,littleEndian_);if(n>options.maxIfdEntries)return DngSourceStatus::error(DngSourceCode::InvalidTiff,"IFD entry cap exceeded");
    std::uint64_t bytes=0;if(!mul_ok(n,12,bytes))return DngSourceStatus::error(DngSourceCode::InvalidTiff,"IFD size overflow");std::uint64_t base=offset+2,end=0;if(!add_ok(base,bytes+4,end)||end>bytes_->sizeBytes())return DngSourceStatus::error(DngSourceCode::InvalidTiff,"truncated IFD");
    out=Ifd{};out.offset=offset;out.tags.reserve(n);
    for(std::uint16_t i=0;i<n;++i){std::uint8_t e[12];std::uint64_t q=base+12ull*i;if(!readBytes(q,e,12))return DngSourceStatus::error(DngSourceCode::IoError,"cannot read IFD entry");std::uint16_t tag=dec16(e,littleEndian_),type=dec16(e+2,littleEndian_);std::uint32_t count=dec32(e+4,littleEndian_);auto ts=type_size(type);if(!ts)continue;std::uint64_t sz=0;if(!mul_ok(count,ts,sz))return DngSourceStatus::error(DngSourceCode::InvalidTag,"tag size overflow");std::uint64_t dataOff=(sz<=4)?q+8:dec32(e+8,littleEndian_);if(dataOff>bytes_->sizeBytes()||sz>bytes_->sizeBytes()-dataOff)return DngSourceStatus::error(DngSourceCode::InvalidTag,"tag payload out of range");out.tags.push_back({tag,type,count,dataOff,sz});}
    std::uint8_t nb[4];if(!readBytes(base+bytes,nb,4))return DngSourceStatus::error(DngSourceCode::IoError,"cannot read next IFD");out.next=dec32(nb,littleEndian_);return DngSourceStatus::ok();
}

DngSourceStatus TileNativeDngSource::discoverIfds(const OpenOptions&options,std::vector<Ifd>&out){
    std::uint8_t head[8];if(!readBytes(0,head,8))return DngSourceStatus::error(DngSourceCode::InvalidTiff,"short TIFF header");
    if(head[0]=='I'&&head[1]=='I')littleEndian_=true;else if(head[0]=='M'&&head[1]=='M')littleEndian_=false;else return DngSourceStatus::error(DngSourceCode::InvalidTiff,"invalid TIFF byte order");
    auto magic=dec16(head+2,littleEndian_);if(magic==43)return DngSourceStatus::error(DngSourceCode::UnsupportedBigTiff,"BigTIFF is not supported in v0.1");if(magic!=42)return DngSourceStatus::error(DngSourceCode::InvalidTiff,"classic TIFF magic 42 required");
    std::uint32_t root=dec32(head+4,littleEndian_);std::vector<std::uint32_t> pending{root};std::set<std::uint32_t> seen;
    while(!pending.empty()){
        auto off=pending.back();pending.pop_back();if(!off)continue;if(seen.count(off))continue;if(seen.size()>=options.maxIfdCount)return DngSourceStatus::error(DngSourceCode::InvalidTiff,"IFD count cap exceeded");seen.insert(off);
        Ifd ifd;auto s=parseIfd(off,options,ifd);if(!s)return s;out.push_back(ifd);if(ifd.next)pending.push_back(ifd.next);
        if(const auto*sub=findTag(ifd,TAG_SubIFDs)){if(sub->type!=TIFF_LONG)return DngSourceStatus::error(DngSourceCode::InvalidTag,"SubIFDs must be LONG in classic TIFF v0.1");if(sub->count>options.maxIfdCount)return DngSourceStatus::error(DngSourceCode::InvalidTag,"SubIFD count cap exceeded");for(std::uint32_t i=0;i<sub->count;++i){std::uint32_t x=0;if(!readUnsigned(*sub,i,x))return DngSourceStatus::error(DngSourceCode::InvalidTag,"cannot read SubIFD offset");if(x)pending.push_back(x);}}
    }
    return DngSourceStatus::ok();
}

} // namespace truthraw::tile_dng_v0_1
