#include "tile_native_dng_source_v0_1_internal.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace truthraw::tile_dng_v0_1 {
using namespace detail;
StreamStatus TileNativeDngSource::readRawRectStripped(const TileRect&r,std::uint16_t*out){
    int rw=r.hx1-r.hx0;for(int y=r.hy0;y<r.hy1;++y){std::uint32_t strip=std::uint32_t(y)/rowsPerStrip_,local=std::uint32_t(y)%rowsPerStrip_;std::uint32_t off=0,bc=0;if(!readStrileScalar(offsets_,strip,off)||!readStrileScalar(byteCounts_,strip,bc))return source_error("strip locator read failed");std::uint32_t rows=std::min(rowsPerStrip_,std::uint32_t(metadata_.height)-strip*rowsPerStrip_);std::uint64_t expect=std::uint64_t(metadata_.width)*rows*2ull;if(bc!=expect||off>bytes_->sizeBytes()||bc>bytes_->sizeBytes()-off)return source_error("invalid strip locator");std::uint64_t pos=std::uint64_t(off)+(std::uint64_t(local)*metadata_.width+r.hx0)*2ull;auto*dst=out+std::size_t(y-r.hy0)*rw;std::size_t bytes=std::size_t(rw)*2;if(!readBytes(pos,dst,bytes,true))return source_error("strip raw read failed");if(littleEndian_!=host_little())for(int x=0;x<rw;++x)dst[x]=std::uint16_t((dst[x]>>8)|(dst[x]<<8));}return StreamStatus::ok();
}
StreamStatus TileNativeDngSource::readRawRectTiled(const TileRect&r,std::uint16_t*out){
    int rw=r.hx1-r.hx0;std::uint32_t nx=(std::uint32_t(metadata_.width)+tileWidth_-1)/tileWidth_;
    for(int y=r.hy0;y<r.hy1;++y){int x=r.hx0;while(x<r.hx1){std::uint32_t tx=std::uint32_t(x)/tileWidth_,ty=std::uint32_t(y)/tileLength_,idx=ty*nx+tx;std::uint32_t off=0,bc=0;if(!readStrileScalar(offsets_,idx,off)||!readStrileScalar(byteCounts_,idx,bc))return source_error("tile locator read failed");std::uint64_t expect=std::uint64_t(tileWidth_)*tileLength_*2ull;if(bc!=expect||off>bytes_->sizeBytes()||bc>bytes_->sizeBytes()-off)return source_error("invalid tile locator");int tileX0=int(tx*tileWidth_),take=std::min(r.hx1,tileX0+int(tileWidth_))-x;int lx=x-tileX0,ly=y-int(ty*tileLength_);std::uint64_t pos=std::uint64_t(off)+(std::uint64_t(ly)*tileWidth_+lx)*2ull;auto*dst=out+std::size_t(y-r.hy0)*rw+(x-r.hx0);std::size_t bytes=std::size_t(take)*2;if(!readBytes(pos,dst,bytes,true))return source_error("tile raw read failed");if(littleEndian_!=host_little())for(int i=0;i<take;++i)dst[i]=std::uint16_t((dst[i]>>8)|(dst[i]<<8));x+=take;}}
    return StreamStatus::ok();
}
float TileNativeDngSource::gainAt(int y,int x)const{const GainMap*m=nullptr;for(const auto&g:gainMaps_)if(g.applies(y,x)){if(m)return std::numeric_limits<float>::quiet_NaN();m=&g;}return m?m->interpolate(y,x,metadata_.height,metadata_.width):1.f;}
StreamStatus TileNativeDngSource::readRawTile(const TileRect&r,std::uint16_t*rawOut,std::size_t rawCount,float*gainOut,std::size_t gainCount){
    if (!rawOut || r.hx0<0 || r.hy0<0 || r.hx1>metadata_.width || r.hy1>metadata_.height || r.hx1<=r.hx0 || r.hy1<=r.hy0) return StreamStatus::error(StreamStatusCode::InvalidArgument,"invalid tile rectangle");
    std::size_t n=std::size_t(r.hx1-r.hx0)*std::size_t(r.hy1-r.hy0);
    if (rawCount<n) return StreamStatus::error(StreamStatusCode::InvalidArgument,"raw output too small");
    if (metadata_.hasGainField) {
        if (!gainOut || gainCount<n) return StreamStatus::error(StreamStatusCode::InvalidArgument,"gain output too small");
    } else if (gainOut!=nullptr && gainCount<n) {
        return StreamStatus::error(StreamStatusCode::InvalidArgument,"gain output count inconsistent");
    }
    auto s=tiled_?readRawRectTiled(r,rawOut):readRawRectStripped(r,rawOut);if(!s)return s;if(metadata_.hasGainField){int rw=r.hx1-r.hx0;for(int y=r.hy0;y<r.hy1;++y)for(int x=r.hx0;x<r.hx1;++x){float g=gainAt(y,x);if(!std::isfinite(g)||g<=0)return source_error("GainMap evaluation failed");gainOut[std::size_t(y-r.hy0)*rw+(x-r.hx0)]=g;}}
    ++audit_.tileReadCalls;return StreamStatus::ok();
}
StreamStatus TileNativeDngSource::readRowBias(int y0,int y1,float*out,std::size_t count){
    if(y0<0||y1<y0||y1>metadata_.height||count<std::size_t(y1-y0)||(y1>y0&&!out))return StreamStatus::error(StreamStatusCode::InvalidArgument,"invalid row-bias request");
    if(y0==y1)return StreamStatus::ok();
    std::fill(out,out+(y1-y0),0.f);return StreamStatus::ok();
}
StreamStatus TileNativeDngSource::readColBias(int x0,int x1,float*out,std::size_t count){
    if(x0<0||x1<x0||x1>metadata_.width||count<std::size_t(x1-x0)||(x1>x0&&!out))return StreamStatus::error(StreamStatusCode::InvalidArgument,"invalid col-bias request");
    if(x0==x1)return StreamStatus::ok();
    std::fill(out,out+(x1-x0),0.f);return StreamStatus::ok();
}

} // namespace truthraw::tile_dng_v0_1
