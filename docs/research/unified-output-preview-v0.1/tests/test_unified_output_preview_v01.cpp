#include "unified_output_preview_v0_1.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

namespace uop = truthraw::unified_output_preview::v0_1;
namespace fdng = truthraw::scientific_master_linear_dng_projection::v0_1;

namespace {

void require(bool value,const char* message){
    if(!value){
        std::cerr<<"REQUIRE FAILED: "<<message<<"\n";
        std::exit(2);
    }
}

class FakeSource final : public fdng::IScientificMasterTileSource {
public:
    FakeSource(std::uint32_t w,std::uint32_t h)
        : w_(w),h_(h),rgb_(static_cast<std::size_t>(w)*h*3u) {
        for(std::uint32_t y=0;y<h_;++y){
            for(std::uint32_t x=0;x<w_;++x){
                const std::size_t p=(static_cast<std::size_t>(y)*w_+x)*3u;
                rgb_[p+0u]=x<=1u?-0.25f:static_cast<float>(x)/static_cast<float>(w_-1u);
                rgb_[p+1u]=static_cast<float>(y)/static_cast<float>(h_-1u);
                rgb_[p+2u]=x==w_-1u?1.5f:0.25f;
            }
        }
    }

    std::size_t residentBytesUpperBound() const noexcept override {
        return rgb_.size()*sizeof(float);
    }

    fdng::Status readCameraNativeTile(
        std::uint32_t x,std::uint32_t y,std::uint32_t w,std::uint32_t h,
        float* out,std::size_t count) noexcept override {
        ++reads_;
        if(!out||w==0u||h==0u||x+w>w_||y+h>h_||
           count!=static_cast<std::size_t>(w)*h*3u){
            return fdng::Status::error(fdng::StatusCode::InvalidArgument,"fake tile");
        }
        for(std::uint32_t yy=0;yy<h;++yy){
            for(std::uint32_t xx=0;xx<w;++xx){
                const std::size_t src=
                    (static_cast<std::size_t>(y+yy)*w_+(x+xx))*3u;
                const std::size_t dst=
                    (static_cast<std::size_t>(yy)*w+xx)*3u;
                out[dst]=rgb_[src];
                out[dst+1u]=rgb_[src+1u];
                out[dst+2u]=rgb_[src+2u];
            }
        }
        return fdng::Status::ok();
    }

    std::uint64_t reads() const noexcept { return reads_; }

private:
    std::uint32_t w_,h_;
    std::vector<float> rgb_;
    std::uint64_t reads_=0u;
};

void test_linear_srgb_primary(){
    FakeSource source(8u,4u);
    uop::Descriptor d{};
    d.sourceWidth=8u;
    d.sourceHeight=4u;
    d.maxEdge=4u;
    d.sourceSpace=uop::SourceSpace::LinearSrgb;
    d.displayQuarterTurns=1u;
    d.outputRole="TEST_LINEAR_PRIMARY";

    uop::Result r{};
    require(uop::render(source,d,r),"render linear sRGB");
    require(r.width==4u&&r.height==2u,"aspect ratio");
    require(r.argb8888.size()==8u,"pixel count");
    require(r.sampledPrimaryPixels==8u,"sampled primary count");
    require(source.reads()==2u,"one source row read per preview row");
    require(r.primaryTileSourceUsedDirectly,"direct source");
    require(!r.appearanceAddedByPreview,"no preview appearance");
    require(!r.scientificWritebackAllowed,"no writeback");
    require(r.negativeDisplayClampedComponents>0u,"negative display clamp counted");
    require(r.overOneDisplayClampedComponents>0u,"over-one display clamp counted");

    for(auto px:r.argb8888){
        require((px>>24u)==0xffu,"opaque alpha");
    }

    char path[]="/tmp/truthraw_uop1_XXXXXX";
    const int fd=::mkstemp(path);
    require(fd>=0,"mkstemp");
    require(uop::write_uop1_fd(fd,d,r),"write UOP1");
    const off_t size=::lseek(fd,0,SEEK_END);
    require(size==static_cast<off_t>(40u+r.argb8888.size()*4u),"UOP1 bytes");
    ::close(fd);
    ::unlink(path);
}

void test_camera_native_path(){
    FakeSource source(4u,4u);
    uop::Descriptor d{};
    d.sourceWidth=4u;
    d.sourceHeight=4u;
    d.maxEdge=4u;
    d.sourceSpace=uop::SourceSpace::CameraNative;
    d.outputRole="TEST_CAMERA_NATIVE";
    d.cameraToXyzD50={
        0.4360747f,0.3850649f,0.1430804f,
        0.2225045f,0.7168786f,0.0606169f,
        0.0139322f,0.0971045f,0.7141733f};

    uop::Result r{};
    require(uop::render(source,d,r),"render camera native");
    require(r.width==4u&&r.height==4u,"camera preview geometry");
    require(r.argb8888.size()==16u,"camera preview pixel count");
    require(r.primaryTileSourceUsedDirectly&&!r.appearanceAddedByPreview,
            "camera preview primary only");
}

void test_invalid(){
    FakeSource source(2u,2u);
    uop::Descriptor d{};
    d.sourceWidth=2u;d.sourceHeight=2u;d.maxEdge=0u;
    d.outputRole="INVALID";
    uop::Result r{};
    require(!uop::render(source,d,r),"zero max edge fails");
}

} // namespace

int main(){
    test_linear_srgb_primary();
    test_camera_native_path();
    test_invalid();
    std::cout<<"UNIFIED_OUTPUT_PREVIEW_V01_PASS\n";
    std::cout<<"primary_tile_source_direct=1\n";
    std::cout<<"appearance_added_by_preview=0\n";
    std::cout<<"scientific_writeback_allowed=0\n";
    return 0;
}
