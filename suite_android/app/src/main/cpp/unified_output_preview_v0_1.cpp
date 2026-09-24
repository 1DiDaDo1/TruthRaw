#include "unified_output_preview_v0_1.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <unistd.h>

namespace truthraw::unified_output_preview::v0_1 {
namespace {

constexpr float kXyzD50ToLinearSrgb[9]={
    3.1338561f,-1.6168667f,-0.4906146f,
   -0.9787684f, 1.9161415f, 0.0334540f,
    0.0719453f,-0.2289914f, 1.4052427f};

bool finite_matrix(const std::array<float,9>& m) noexcept {
    return std::all_of(m.begin(),m.end(),[](float v){return std::isfinite(v);});
}

void mat3(const float* m,float a,float b,float c,float& x,float& y,float& z) noexcept {
    x=m[0]*a+m[1]*b+m[2]*c;
    y=m[3]*a+m[4]*b+m[5]*c;
    z=m[6]*a+m[7]*b+m[8]*c;
}

std::uint8_t srgb_u8(float linear) noexcept {
    if(!std::isfinite(linear)) return 0u;
    const float x=std::clamp(linear,0.0f,1.0f);
    const float encoded=x<=0.0031308f
        ? 12.92f*x
        : 1.055f*std::pow(x,1.0f/2.4f)-0.055f;
    const long q=std::lround(std::clamp(encoded,0.0f,1.0f)*255.0f);
    return static_cast<std::uint8_t>(std::clamp<long>(q,0,255));
}

std::uint32_t sampled_coordinate(
    std::uint32_t previewIndex,
    std::uint32_t previewSize,
    std::uint32_t sourceSize) noexcept {
    const double position=
        (static_cast<double>(previewIndex)+0.5)*
        static_cast<double>(sourceSize)/
        static_cast<double>(previewSize);
    const auto sample=static_cast<std::uint64_t>(std::floor(position));
    return static_cast<std::uint32_t>(
        std::min<std::uint64_t>(
            sample,
            static_cast<std::uint64_t>(sourceSize-1u)));
}

bool write_all(int fd,const void* data,std::size_t size) noexcept {
    const auto* p=static_cast<const std::uint8_t*>(data);
    std::size_t done=0u;
    while(done<size){
        const ssize_t n=::write(fd,p+done,size-done);
        if(n<=0)return false;
        done+=static_cast<std::size_t>(n);
    }
    return true;
}

void put_u32(std::array<std::uint8_t,40>& h,std::size_t off,std::uint32_t v) noexcept {
    h[off]=static_cast<std::uint8_t>(v);
    h[off+1u]=static_cast<std::uint8_t>(v>>8u);
    h[off+2u]=static_cast<std::uint8_t>(v>>16u);
    h[off+3u]=static_cast<std::uint8_t>(v>>24u);
}

void put_u64(std::array<std::uint8_t,40>& h,std::size_t off,std::uint64_t v) noexcept {
    for(std::size_t i=0u;i<8u;++i)h[off+i]=static_cast<std::uint8_t>(v>>(8u*i));
}

} // namespace

bool render(
    float_dng::IScientificMasterTileSource& primary,
    const Descriptor& d,
    Result& out) noexcept {
    out={};
    try{
        if(d.sourceWidth==0u||d.sourceHeight==0u||
           d.maxEdge==0u||d.maxEdge>kMaxEdgeHardLimit||
           d.outputRole.empty()||
           !finite_matrix(d.cameraToXyzD50)) return false;

        std::uint32_t pw=d.maxEdge,ph=d.maxEdge;
        if(d.sourceWidth>=d.sourceHeight){
            ph=std::max<std::uint32_t>(
                1u,
                static_cast<std::uint32_t>(
                    std::llround(
                        static_cast<double>(d.maxEdge)*d.sourceHeight/d.sourceWidth)));
        }else{
            pw=std::max<std::uint32_t>(
                1u,
                static_cast<std::uint32_t>(
                    std::llround(
                        static_cast<double>(d.maxEdge)*d.sourceWidth/d.sourceHeight)));
        }

        const std::size_t previewPixels=static_cast<std::size_t>(pw)*ph;
        if(previewPixels>static_cast<std::size_t>(kMaxEdgeHardLimit)*kMaxEdgeHardLimit)
            return false;
        out.argb8888.resize(previewPixels);

        std::vector<float> row(
            static_cast<std::size_t>(d.sourceWidth)*3u);

        for(std::uint32_t py=0u;py<ph;++py){
            const std::uint32_t sy=sampled_coordinate(py,ph,d.sourceHeight);
            const auto status=primary.readCameraNativeTile(
                0u,sy,d.sourceWidth,1u,row.data(),row.size());
            if(!status){out={};return false;}

            for(std::uint32_t px=0u;px<pw;++px){
                const std::uint32_t sx=sampled_coordinate(px,pw,d.sourceWidth);
                const std::size_t i=static_cast<std::size_t>(sx)*3u;
                const float a=row[i],b=row[i+1u],c=row[i+2u];
                if(!std::isfinite(a)||!std::isfinite(b)||!std::isfinite(c)){
                    out={};return false;
                }

                float lr=0.0f,lg=0.0f,lb=0.0f;
                if(d.sourceSpace==SourceSpace::LinearSrgb){
                    lr=a;lg=b;lb=c;
                }else{
                    float x=a,y=b,z=c;
                    if(d.sourceSpace==SourceSpace::CameraNative){
                        mat3(
                            d.cameraToXyzD50.data(),
                            a,b,c,
                            x,y,z);
                    }else if(d.sourceSpace!=SourceSpace::XyzD50){
                        out={};return false;
                    }
                    mat3(kXyzD50ToLinearSrgb,x,y,z,lr,lg,lb);
                }

                const float display[3]={lr,lg,lb};
                for(float v:display){
                    if(v<0.0f)++out.negativeDisplayClampedComponents;
                    if(v>1.0f)++out.overOneDisplayClampedComponents;
                }

                const std::uint32_t r=srgb_u8(lr);
                const std::uint32_t g=srgb_u8(lg);
                const std::uint32_t bl=srgb_u8(lb);
                out.argb8888[
                    static_cast<std::size_t>(py)*pw+px]=
                    0xff000000u|(r<<16u)|(g<<8u)|bl;
                ++out.sampledPrimaryPixels;
            }
        }

        out.width=pw;
        out.height=ph;
        out.primaryTileSourceUsedDirectly=true;
        out.appearanceAddedByPreview=false;
        out.scientificWritebackAllowed=false;
        return true;
    }catch(...){
        out={};return false;
    }
}

bool write_uop1_fd(
    int fd,
    const Descriptor& d,
    const Result& r) noexcept {
    if(fd<0||r.width==0u||r.height==0u||
       r.argb8888.size()!=static_cast<std::size_t>(r.width)*r.height||
       !r.primaryTileSourceUsedDirectly||
       r.appearanceAddedByPreview||
       r.scientificWritebackAllowed) return false;

    if(::ftruncate(fd,0)!=0||::lseek(fd,0,SEEK_SET)<0)return false;

    std::array<std::uint8_t,40> header{};
    put_u32(header,0u,kMagic);
    put_u32(header,4u,1u);
    put_u32(header,8u,r.width);
    put_u32(header,12u,r.height);
    put_u32(header,16u,d.sourceWidth);
    put_u32(header,20u,d.sourceHeight);
    put_u32(header,24u,static_cast<std::uint32_t>(d.sourceSpace));
    put_u32(header,28u,0u); // flags: preview adds no appearance
    put_u64(header,32u,r.sampledPrimaryPixels);
    if(!write_all(fd,header.data(),header.size()))return false;

    std::array<std::uint8_t,4> pixel{};
    for(const auto argb:r.argb8888){
        pixel[0]=static_cast<std::uint8_t>(argb);
        pixel[1]=static_cast<std::uint8_t>(argb>>8u);
        pixel[2]=static_cast<std::uint8_t>(argb>>16u);
        pixel[3]=static_cast<std::uint8_t>(argb>>24u);
        if(!write_all(fd,pixel.data(),pixel.size()))return false;
    }
    return ::fsync(fd)==0;
}

const char* schema_name() noexcept {
    return "TruthRawUnifiedOutputPreview/0.1";
}

const char* source_space_name(SourceSpace s) noexcept {
    switch(s){
        case SourceSpace::CameraNative:return "CAMERA_NATIVE";
        case SourceSpace::LinearSrgb:return "LINEAR_SRGB";
        case SourceSpace::XyzD50:return "XYZ_D50";
    }
    return "INVALID";
}

} // namespace truthraw::unified_output_preview::v0_1
