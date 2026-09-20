#include <jni.h>

#include "dng_color_binding_producer_v0_2.h"
#include "scientific_master_digest_v0_1.h"
#include "scientific_master_linear_dng_projection_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "technical_backplane_phase2_v0_1.h"
#include "technical_backplane_v0_1.h"
#include "tile_native_dng_source_v0_1.h"
#include "raw_source_adapter_bridge_common.h"
#include "truthraw/core.h"
#include "truthraw_sha256_v0_69.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {

using truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction;
using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::scientific_preview_binding_v0_1::ColorClaimScope;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource;
using truthraw::tile_dng_v0_1::PosixFdByteSource;

namespace float_dng = truthraw::scientific_master_linear_dng_projection::v0_1;
namespace digest = truthraw::scientific_master_digest::v0_1;
namespace adapter = truthraw::multivendor_raw_source_adapter::v0_1;
namespace sha = truthraw::sha256_v0_69;

constexpr jlong kMagic = 0x5452504a; // TRPJ
constexpr std::size_t kPacketLongs = 20u;
constexpr std::size_t kTrrHeaderBytes = 8192u;
constexpr std::uint32_t kTileEdge = 64u;
constexpr jint kFormatDng = 1;
constexpr jint kFormatTiff = 2;
constexpr jint kFormatExr = 3;
constexpr const char* kPrecisionPolicy =
    "TRUTHRAW_V069_RESTORATION_PROJECTION__F32_DERIVATIVE__"
    "SOURCE_BOUND_COLOR_TRANSFORM__NO_TONE_NO_CLIP";

bool pread_all(int fd, std::uint64_t offset, std::uint8_t* dst, std::size_t size) noexcept {
    std::size_t done = 0u;
    while (done < size) {
        const auto pos = static_cast<off_t>(offset + done);
        const ssize_t n = ::pread(fd, dst + done, size - done, pos);
        if (n <= 0) return false;
        done += static_cast<std::size_t>(n);
    }
    return true;
}

bool write_all(int fd, const std::uint8_t* data, std::size_t size) noexcept {
    std::size_t done = 0u;
    while (done < size) {
        const ssize_t n = ::write(fd, data + done, size - done);
        if (n <= 0) return false;
        done += static_cast<std::size_t>(n);
    }
    return true;
}

std::uint16_t get_u16_le(const std::uint8_t* p) noexcept {
    return static_cast<std::uint16_t>(p[0]) |
           (static_cast<std::uint16_t>(p[1]) << 8u);
}

std::uint32_t get_u32_le(const std::uint8_t* p) noexcept {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8u) |
           (static_cast<std::uint32_t>(p[2]) << 16u) |
           (static_cast<std::uint32_t>(p[3]) << 24u);
}

void put_u16_le(std::vector<std::uint8_t>& out, std::uint16_t v) {
    out.push_back(static_cast<std::uint8_t>(v & 0xffu));
    out.push_back(static_cast<std::uint8_t>((v >> 8u) & 0xffu));
}
void put_u32_le(std::vector<std::uint8_t>& out, std::uint32_t v) {
    out.push_back(static_cast<std::uint8_t>(v & 0xffu));
    out.push_back(static_cast<std::uint8_t>((v >> 8u) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((v >> 16u) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((v >> 24u) & 0xffu));
}
void put_u64_le(std::vector<std::uint8_t>& out, std::uint64_t v) {
    for (unsigned i = 0; i < 8u; ++i) out.push_back(static_cast<std::uint8_t>((v >> (8u * i)) & 0xffu));
}
void store_u32_le(std::vector<std::uint8_t>& out, std::size_t off, std::uint32_t v) {
    out[off] = static_cast<std::uint8_t>(v & 0xffu);
    out[off+1] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
    out[off+2] = static_cast<std::uint8_t>((v >> 16u) & 0xffu);
    out[off+3] = static_cast<std::uint8_t>((v >> 24u) & 0xffu);
}
void put_f32_le(std::vector<std::uint8_t>& out, float v) {
    put_u32_le(out, std::bit_cast<std::uint32_t>(v));
}
void put_f32_le(std::uint8_t* p, float v) noexcept {
    const auto u = std::bit_cast<std::uint32_t>(v);
    p[0]=static_cast<std::uint8_t>(u); p[1]=static_cast<std::uint8_t>(u>>8u);
    p[2]=static_cast<std::uint8_t>(u>>16u); p[3]=static_cast<std::uint8_t>(u>>24u);
}

bool parse_u64(const std::string& s, std::uint64_t& out) noexcept {
    if (s.empty()) return false;
    std::uint64_t v = 0u;
    for (char ch : s) {
        if (ch < '0' || ch > '9') return false;
        const auto d = static_cast<std::uint64_t>(ch - '0');
        if (v > (std::numeric_limits<std::uint64_t>::max() - d) / 10u) return false;
        v = v * 10u + d;
    }
    out = v;
    return true;
}

bool parse_hex32(const std::string& s, std::array<std::uint8_t,32>& out) noexcept {
    if (s.size() != 64u) return false;
    auto nib=[](char c)->int {
        if(c>='0'&&c<='9') return c-'0';
        if(c>='a'&&c<='f') return 10+c-'a';
        if(c>='A'&&c<='F') return 10+c-'A';
        return -1;
    };
    for(std::size_t i=0;i<32u;++i){
        const int a=nib(s[2u*i]), b=nib(s[2u*i+1u]);
        if(a<0||b<0) return false;
        out[i]=static_cast<std::uint8_t>((a<<4)|b);
    }
    return true;
}

bool parse_hex_bytes(const std::string& s, std::vector<std::uint8_t>& out) {
    if ((s.size() & 1u) != 0u) return false;
    out.resize(s.size()/2u);
    auto nib=[](char c)->int {
        if(c>='0'&&c<='9') return c-'0';
        if(c>='a'&&c<='f') return 10+c-'a';
        if(c>='A'&&c<='F') return 10+c-'A';
        return -1;
    };
    for(std::size_t i=0;i<out.size();++i){
        const int a=nib(s[2u*i]), b=nib(s[2u*i+1u]);
        if(a<0||b<0) return false;
        out[i]=static_cast<std::uint8_t>((a<<4)|b);
    }
    return true;
}

std::map<std::string,std::string> parse_kv(const std::string& text) {
    std::map<std::string,std::string> out;
    std::size_t pos=0;
    while(pos<text.size()){
        const auto end=text.find('\n',pos);
        const auto stop=end==std::string::npos?text.size():end;
        const auto line=text.substr(pos,stop-pos);
        const auto eq=line.find('=');
        if(eq!=std::string::npos && eq>0) out[line.substr(0,eq)]=line.substr(eq+1);
        if(end==std::string::npos) break;
        pos=end+1;
    }
    return out;
}

struct TrrMeta final {
    std::uint32_t width=0, height=0, orientation=1, tileCount=0;
    std::uint64_t totalBytes=0;
    digest::Sha256 sourceHash{}, masterHash{}, derivativeHash{}, zeroHash{}, sceneHash{};
    truthraw::technical_backplane::v0_1::SerializedBackplane backplane{};
};

struct TileIndex final {
    std::uint32_t x=0,y=0,w=0,h=0;
    std::uint64_t rgbOffset=0;
    std::uint64_t roleOffset=0;
};

class TrrReader final : public float_dng::IScientificMasterTileSource {
public:
    explicit TrrReader(int fd):fd_(fd){}

    bool open(std::string& error) {
        struct stat st{};
        if(fd_<0 || ::fstat(fd_,&st)!=0 || st.st_size < static_cast<off_t>(kTrrHeaderBytes)){
            error="TRR file is missing or too short"; return false;
        }
        fileBytes_=static_cast<std::uint64_t>(st.st_size);
        std::array<std::uint8_t,kTrrHeaderBytes> header{};
        if(!pread_all(fd_,0,header.data(),header.size())){error="TRR header read failed";return false;}
        const auto zero=std::find(header.begin(),header.end(),std::uint8_t{0});
        const std::string text(reinterpret_cast<const char*>(header.data()),
                               static_cast<std::size_t>(zero-header.begin()));
        const auto kv=parse_kv(text);
        auto get=[&](const char* k)->std::string{
            const auto it=kv.find(k); return it==kv.end()?std::string{}:it->second;
        };
        if(get("magic")!="TRUTHRAW_FULLRES_RESTORATION_V0_67" ||
           get("role")!="FULL_RESOLUTION_RETREATABLE_RESTORATION_DERIVATIVE" ||
           get("full_resolution")!="1" || get("retreatable")!="1" ||
           get("scientific_master_modified")!="0" || get("scientific_writeback_allowed")!="0" ||
           get("creates_new_evidence")!="0" || get("creates_second_scientific_world")!="0"){
            error="TRR scientific contract rejected"; return false;
        }
        std::uint64_t tmp=0;
        if(!parse_u64(get("width"),tmp)||tmp==0||tmp>std::numeric_limits<std::uint32_t>::max()){error="bad TRR width";return false;} meta_.width=static_cast<std::uint32_t>(tmp);
        if(!parse_u64(get("height"),tmp)||tmp==0||tmp>std::numeric_limits<std::uint32_t>::max()){error="bad TRR height";return false;} meta_.height=static_cast<std::uint32_t>(tmp);
        if(!parse_u64(get("orientation"),tmp)||tmp>8u){error="bad TRR orientation";return false;} meta_.orientation=static_cast<std::uint32_t>(tmp);
        if(!parse_u64(get("tile_count"),tmp)||tmp==0||tmp>std::numeric_limits<std::uint32_t>::max()){error="bad TRR tile count";return false;} meta_.tileCount=static_cast<std::uint32_t>(tmp);
        if(!parse_u64(get("total_bytes"),meta_.totalBytes)||meta_.totalBytes!=fileBytes_){error="TRR byte count mismatch";return false;}
        if(!parse_hex32(get("source_sha256"),meta_.sourceHash) ||
           !parse_hex32(get("scientific_master_sha256"),meta_.masterHash) ||
           !parse_hex32(get("restoration_derivative_rgb_sha256"),meta_.derivativeHash) ||
           !parse_hex32(get("zero_line_sha256"),meta_.zeroHash) ||
           !parse_hex32(get("scene_scale_sha256"),meta_.sceneHash)){
            error="TRR lineage hash parse failed";return false;
        }
        std::vector<std::uint8_t> bp;
        if(!parse_hex_bytes(get("technical_backplane_serialized_hex"),bp) ||
           bp.size()!=meta_.backplane.size()){error="TRR backplane parse failed";return false;}
        std::copy(bp.begin(),bp.end(),meta_.backplane.begin());

        const std::uint32_t cols=(meta_.width+kTileEdge-1u)/kTileEdge;
        const std::uint32_t rows=(meta_.height+kTileEdge-1u)/kTileEdge;
        if(static_cast<std::uint64_t>(cols)*rows!=meta_.tileCount){error="TRR canonical tile count mismatch";return false;}
        tiles_.clear(); tiles_.reserve(meta_.tileCount);
        std::uint64_t off=kTrrHeaderBytes;
        std::array<std::uint8_t,16> rh{};
        for(std::uint32_t ty=0;ty<rows;++ty){
            const std::uint32_t y=ty*kTileEdge, h=std::min(kTileEdge,meta_.height-y);
            for(std::uint32_t tx=0;tx<cols;++tx){
                const std::uint32_t x=tx*kTileEdge, w=std::min(kTileEdge,meta_.width-x);
                if(off+16u>fileBytes_ || !pread_all(fd_,off,rh.data(),rh.size())){error="TRR record header truncated";return false;}
                if(get_u32_le(rh.data())!=x || get_u32_le(rh.data()+4)!=y ||
                   get_u32_le(rh.data()+8)!=w || get_u32_le(rh.data()+12)!=h){error="TRR tile order/geometry mismatch";return false;}
                const std::uint64_t pixels=static_cast<std::uint64_t>(w)*h;
                const std::uint64_t rgbBytes=pixels*3u*4u;
                const std::uint64_t roleBytes=pixels;
                const std::uint64_t rgbOff=off+16u, roleOff=rgbOff+rgbBytes, next=roleOff+roleBytes;
                if(next>fileBytes_){error="TRR record payload truncated";return false;}
                tiles_.push_back(TileIndex{x,y,w,h,rgbOff,roleOff});
                off=next;
            }
        }
        if(off!=fileBytes_){error="TRR trailing or missing bytes";return false;}
        return validatePayload(error);
    }

    const TrrMeta& meta() const noexcept { return meta_; }
    const sha::Digest& roleHash() const noexcept { return roleHash_; }
    std::uint64_t role0() const noexcept { return role0_; }
    std::uint64_t role1() const noexcept { return role1_; }
    std::uint64_t role2() const noexcept { return role2_; }
    std::uint64_t negative() const noexcept { return negative_; }
    std::uint64_t overOne() const noexcept { return overOne_; }

    std::size_t residentBytesUpperBound() const noexcept override {
        return tiles_.capacity()*sizeof(TileIndex)+
               static_cast<std::size_t>(kTileEdge)*kTileEdge*3u*sizeof(float)+
               static_cast<std::size_t>(kTileEdge)*kTileEdge;
    }

    float_dng::Status readCameraNativeTile(
        std::uint32_t x,std::uint32_t y,std::uint32_t w,std::uint32_t h,
        float* rgb,std::size_t floatCount) noexcept override {
        if(rgb==nullptr || floatCount!=static_cast<std::size_t>(w)*h*3u)
            return float_dng::Status::error(float_dng::StatusCode::InvalidArgument,"TRR tile output size mismatch");
        const auto cols=(meta_.width+kTileEdge-1u)/kTileEdge;
        if((x%kTileEdge)!=0u || (y%kTileEdge)!=0u)
            return float_dng::Status::error(float_dng::StatusCode::InvalidArgument,"TRR tile origin not canonical");
        const std::size_t idx=static_cast<std::size_t>(y/kTileEdge)*cols+(x/kTileEdge);
        if(idx>=tiles_.size()) return float_dng::Status::error(float_dng::StatusCode::SourceFailed,"TRR tile index out of range");
        const auto& t=tiles_[idx];
        if(t.x!=x||t.y!=y||t.w!=w||t.h!=h)
            return float_dng::Status::error(float_dng::StatusCode::SourceFailed,"TRR tile geometry mismatch");
        const auto bytes=floatCount*sizeof(float);
        if(!pread_all(fd_,t.rgbOffset,reinterpret_cast<std::uint8_t*>(rgb),bytes))
            return float_dng::Status::error(float_dng::StatusCode::SourceFailed,"TRR tile read failed");
        for(std::size_t i=0;i<floatCount;++i) if(!std::isfinite(rgb[i]))
            return float_dng::Status::error(float_dng::StatusCode::SourceFailed,"TRR contains non-finite float");
        return float_dng::Status::ok();
    }

    bool readTileByOrdinal(std::size_t idx,std::vector<float>& rgb,std::vector<std::uint8_t>& roles) const {
        if(idx>=tiles_.size()) return false;
        const auto& t=tiles_[idx];
        const std::size_t pixels=static_cast<std::size_t>(t.w)*t.h;
        rgb.resize(pixels*3u); roles.resize(pixels);
        return pread_all(fd_,t.rgbOffset,reinterpret_cast<std::uint8_t*>(rgb.data()),rgb.size()*sizeof(float)) &&
               pread_all(fd_,t.roleOffset,roles.data(),roles.size());
    }
    const TileIndex& tile(std::size_t idx) const { return tiles_[idx]; }
    std::size_t tileCount() const noexcept { return tiles_.size(); }

private:
    bool validatePayload(std::string& error) {
        digest::ScientificMasterDigestAccumulator d(meta_.width,meta_.height);
        if(!d.valid()){error="derivative digest init failed";return false;}
        sha::Hasher roleHasher;
        std::vector<float> rgb;
        std::vector<std::uint8_t> roles;
        role0_=role1_=role2_=negative_=overOne_=0u;
        for(std::size_t i=0;i<tiles_.size();++i){
            if(!readTileByOrdinal(i,rgb,roles)){error="TRR payload read failed";return false;}
            const auto& t=tiles_[i];
            for(float v:rgb){
                if(!std::isfinite(v)){error="TRR non-finite component";return false;}
                if(v<0.f) ++negative_; if(v>1.f) ++overOne_;
            }
            for(auto r:roles){
                if(r==0u)++role0_; else if(r==1u)++role1_; else if(r==2u)++role2_;
                else {error="TRR invalid restoration role";return false;}
            }
            roleHasher.update(roles.data(),roles.size());
            digest::TileView tv{};
            tv.x=t.x;tv.y=t.y;tv.width=t.w;tv.height=t.h;tv.rgb=rgb.data();tv.rowStrideSamples=static_cast<std::size_t>(t.w)*3u;
            if(!d.add_tile(tv)){error="TRR derivative digest rejected tile";return false;}
        }
        digest::Sha256 actual{};
        if(!d.finalize(actual)||actual!=meta_.derivativeHash){error="TRR derivative digest mismatch";return false;}
        roleHash_=roleHasher.finalize();
        const std::uint64_t pixels=static_cast<std::uint64_t>(meta_.width)*meta_.height;
        if(role0_+role1_+role2_!=pixels){error="TRR role coverage mismatch";return false;}
        return true;
    }

    int fd_=-1;
    std::uint64_t fileBytes_=0;
    TrrMeta meta_{};
    std::vector<TileIndex> tiles_;
    sha::Digest roleHash_{};
    std::uint64_t role0_=0,role1_=0,role2_=0,negative_=0,overOne_=0;
};

class FdSink final : public float_dng::ITransactionalByteSink {
public:
    explicit FdSink(int fd):fd_(fd){}
    std::size_t residentBytesUpperBound() const noexcept override {return 0u;}
    bool begin(std::uint64_t expected) noexcept override {
        expected_=expected;written_=0;active_=false;
        if(fd_<0||expected==0u||::ftruncate(fd_,0)!=0||::lseek(fd_,0,SEEK_SET)<0)return false;
        active_=true;return true;
    }
    bool write(const std::uint8_t* data,std::size_t size) noexcept override {
        if(!active_||data==nullptr||size==0||written_+size>expected_)return false;
        if(!write_all(fd_,data,size))return false;written_+=size;return true;
    }
    bool commit() noexcept override {
        if(!active_||written_!=expected_||::fsync(fd_)!=0)return false;active_=false;return true;
    }
    void abort() noexcept override {
        if(fd_>=0){(void)::ftruncate(fd_,0);(void)::lseek(fd_,0,SEEK_SET);}active_=false;written_=0;
    }
private:int fd_=-1;std::uint64_t expected_=0,written_=0;bool active_=false;
};

struct Lineage final {
    SourceSeal seal{};
    ProducerResult color{};
    PreparedScientificPreviewSource prepared{};
    truthraw::scientific_master_streaming_binding::v0_2::Result scientific{};
    truthraw::technical_backplane_phase2::v0_1::Phase2Result phase2{};
    std::shared_ptr<truthraw::tile_dng_v0_1::ITileRawSource> source;
    std::shared_ptr<ResearchEdgeAwareMeasuredPreservingReconstruction> reconstruction;
};

jlong binding_status(const truthraw::scientific_preview_binding_v0_1::BindingStatus& s){return 2000+static_cast<jlong>(s.code);}
jlong producer_status(const truthraw::dng_color_binding_producer_v0_2::ProducerStatus& s){return 2100+static_cast<jlong>(s.code);}
jlong adapter_status(const adapter::AdapterStatus& s){return 7000+static_cast<jlong>(s.code);}
jlong science_status(const truthraw::scientific_master_streaming_binding::v0_2::Status& s){return 8000+static_cast<jlong>(s.code);}
jlong phase2_status(const truthraw::technical_backplane_phase2::v0_1::Status& s){return 9000+static_cast<jlong>(s.code);}
jlong dng_status(const float_dng::Status& s){return 10000+static_cast<jlong>(s.code);}

bool establish_lineage(
    int sourceFd,int maxSourceResidentBytes,int maxLogicalResidentBytes,
    Lineage& out,jlong& status) {
    auto bytes=std::make_shared<PosixFdByteSource>(sourceFd);
    auto s=truthraw::scientific_preview_binding_v0_1::seal_source_sha256(*bytes,out.seal);
    if(!s){status=binding_status(s);return false;}
    auto cs=truthraw::dng_color_binding_producer_v0_2::produce_source_metadata_color_binding(*bytes,out.seal,out.color);
    if(!cs){status=producer_status(cs);return false;}
    auto ps=truthraw::scientific_preview_binding_v0_2::prepare_scientific_color_source(out.seal,out.color.color,out.prepared);
    if(!ps){status=binding_status(ps);return false;}
    if(!out.prepared.mainHouseComputeAllowed||out.prepared.physicalFrameCount!=1u||out.prepared.independentEvidenceCount!=1u){status=-20;return false;}
    auto opts=out.prepared.tileNativeOptions;opts.maxResidentBytes=static_cast<std::size_t>(maxSourceResidentBytes);
    truthraw::android_raw_adapter_bridge::v0_1::OpenedDngSource opened;
    auto os=truthraw::android_raw_adapter_bridge::v0_1::openDngViaAdapter(bytes,out.seal,opts,opened);
    if(!os){status=adapter_status(os);return false;}
    out.source=opened.source;
    out.reconstruction=std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    truthraw::scientific_master_streaming_binding::v0_2::Options so;so.memoryBudgetBytes=static_cast<std::size_t>(maxLogicalResidentBytes);
    auto ss=truthraw::scientific_master_streaming_binding::v0_2::bind_scientific_master_streaming(*out.source,*out.reconstruction,so,out.scientific);
    if(!ss){status=science_status(ss);return false;}
    truthraw::technical_backplane_phase2::v0_1::Phase2Input pi;
    pi.prepared=out.prepared;pi.scientificMasterHash=out.scientific.scientificMasterHash;pi.zeroLineGauge=out.scientific.zeroLineGauge;pi.sceneBinding=out.scientific.sceneBinding;
    pi.roomStatus.fill(truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);
    pi.claimStatus=truthraw::technical_backplane::v0_1::ClaimStatus::Candidate;
    auto p2=truthraw::technical_backplane_phase2::v0_1::finalize_phase2(pi,out.phase2);
    if(!p2){status=phase2_status(p2);return false;}
    if(out.phase2.admission.claimScope==ColorClaimScope::None||out.phase2.backplane.forbiddenFlags!=0u||
       out.scientific.physicalFrameCount!=1u||out.scientific.independentEvidenceCount!=1u){status=-21;return false;}
    status=0;return true;
}

bool lineage_matches(const TrrMeta& m,const Lineage& l) {
    return m.sourceHash==l.seal.sha256 &&
           m.masterHash==l.scientific.scientificMasterHash &&
           m.zeroHash==l.phase2.zeroLineHash &&
           m.sceneHash==l.phase2.sceneScaleHash &&
           m.backplane==l.phase2.serializedBackplane &&
           m.width==static_cast<std::uint32_t>(l.source->metadata().width) &&
           m.height==static_cast<std::uint32_t>(l.source->metadata().height);
}

std::string provenance_text(const TrrReader& trr) {
    return std::string("TruthRaw Restoration Projection v0.69\n")+
        "role=RETREATABLE_RESTORATION_DERIVATIVE\n"+
        "scientific_master_sha256="+digest::to_hex(trr.meta().masterHash)+"\n"+
        "restoration_derivative_rgb_sha256="+digest::to_hex(trr.meta().derivativeHash)+"\n"+
        "restoration_role_mask_sha256="+sha::hex(trr.roleHash())+"\n"+
        "role0_preserve="+std::to_string(trr.role0())+"\n"+
        "role1_aesthetic="+std::to_string(trr.role1())+"\n"+
        "role2_unresolved="+std::to_string(trr.role2())+"\n"+
        "scientific_master_modified=0\nscientific_writeback_allowed=0\ncreates_new_evidence=0\n";
}

std::array<float,9> camera_to_linear_srgb(const std::array<float,9>& c2x) {
    // Bradford-adapt XYZ D50 -> D65 then XYZ D65 -> linear sRGB.
    constexpr double A[9]={
         0.9555766,-0.0230393, 0.0631636,
        -0.0282895, 1.0099416, 0.0210077,
         0.0122982,-0.0204830, 1.3299098};
    constexpr double S[9]={
         3.2404542,-1.5371385,-0.4985314,
        -0.9692660, 1.8760108, 0.0415560,
         0.0556434,-0.2040259, 1.0572252};
    double ax[9]{};
    for(int r=0;r<3;++r)for(int c=0;c<3;++c)for(int k=0;k<3;++k)ax[3*r+c]+=A[3*r+k]*c2x[3*k+c];
    std::array<float,9> out{};
    for(int r=0;r<3;++r)for(int c=0;c<3;++c){double v=0;for(int k=0;k<3;++k)v+=S[3*r+k]*ax[3*k+c];out[3*r+c]=static_cast<float>(v);}
    return out;
}

void transform_rgb(const std::array<float,9>& m,const float* in,float* out) noexcept {
    const double r=in[0],g=in[1],b=in[2];
    out[0]=static_cast<float>(m[0]*r+m[1]*g+m[2]*b);
    out[1]=static_cast<float>(m[3]*r+m[4]*g+m[5]*b);
    out[2]=static_cast<float>(m[6]*r+m[7]*g+m[8]*b);
}

struct IfdEntry {std::uint16_t tag=0,type=0;std::uint32_t count=0;std::vector<std::uint8_t> payload;std::uint32_t off=0;};
std::uint32_t align4(std::uint32_t v) noexcept {return (v+3u)&~3u;}
std::vector<std::uint8_t> shortp(std::uint16_t v){std::vector<std::uint8_t> o;put_u16_le(o,v);return o;}
std::vector<std::uint8_t> longp(std::uint32_t v){std::vector<std::uint8_t> o;put_u32_le(o,v);return o;}
std::vector<std::uint8_t> asciip(const std::string& s){std::vector<std::uint8_t> o(s.begin(),s.end());o.push_back(0);return o;}

bool write_tiff(int fd,TrrReader& trr,const std::array<float,9>& c2srgb,
                std::uint64_t& bytesOut,std::uint64_t& neg,std::uint64_t& over) {
    constexpr std::uint16_t BYTE=1,ASCII=2,SHORT=3,LONG=4;
    constexpr std::uint32_t tileBytes=kTileEdge*kTileEdge*3u*4u;
    std::vector<IfdEntry> e;
    auto add=[&](std::uint16_t tag,std::uint16_t type,std::uint32_t count,std::vector<std::uint8_t> p){e.push_back({tag,type,count,std::move(p),0});};
    add(256,LONG,1,longp(trr.meta().width));add(257,LONG,1,longp(trr.meta().height));
    std::vector<std::uint8_t> bits;for(int i=0;i<3;++i)put_u16_le(bits,32);add(258,SHORT,3,std::move(bits));
    add(259,SHORT,1,shortp(1));add(262,SHORT,1,shortp(2));add(274,SHORT,1,shortp(static_cast<std::uint16_t>(trr.meta().orientation)));
    add(277,SHORT,1,shortp(3));add(284,SHORT,1,shortp(1));add(305,ASCII,31,asciip("TruthRaw v0.69 Restoration TIFF"));
    add(322,LONG,1,longp(kTileEdge));add(323,LONG,1,longp(kTileEdge));
    std::vector<std::uint8_t> offs(trr.tileCount()*4u,0);add(324,LONG,static_cast<std::uint32_t>(trr.tileCount()),std::move(offs));
    std::vector<std::uint8_t> counts;counts.reserve(trr.tileCount()*4u);for(std::size_t i=0;i<trr.tileCount();++i)put_u32_le(counts,tileBytes);add(325,LONG,static_cast<std::uint32_t>(trr.tileCount()),std::move(counts));
    std::vector<std::uint8_t> sf;for(int i=0;i<3;++i)put_u16_le(sf,3);add(339,SHORT,3,std::move(sf));
    auto desc=asciip(provenance_text(trr)+"pixel_space=LINEAR_SRGB_D65_FLOAT32\n");add(270,ASCII,static_cast<std::uint32_t>(desc.size()),std::move(desc));
    std::sort(e.begin(),e.end(),[](auto&a,auto&b){return a.tag<b.tag;});
    std::uint32_t cursor=8u+2u+static_cast<std::uint32_t>(e.size())*12u+4u;
    for(auto& x:e){if(x.payload.size()>4u){cursor=align4(cursor);x.off=cursor;cursor+=static_cast<std::uint32_t>(x.payload.size());}}
    const std::uint32_t dataStart=align4(cursor);
    for(auto& x:e)if(x.tag==324){for(std::size_t i=0;i<trr.tileCount();++i)store_u32_le(x.payload,i*4u,dataStart+static_cast<std::uint32_t>(i)*tileBytes);}
    std::vector<std::uint8_t> header;header.push_back('I');header.push_back('I');put_u16_le(header,42);put_u32_le(header,8);put_u16_le(header,static_cast<std::uint16_t>(e.size()));
    for(const auto& x:e){put_u16_le(header,x.tag);put_u16_le(header,x.type);put_u32_le(header,x.count);if(x.payload.size()<=4u){header.insert(header.end(),x.payload.begin(),x.payload.end());while((header.size()%12u)!=2u && header.size()<8u+2u+e.size()*12u){} // no-op guard
            for(std::size_t p=x.payload.size();p<4u;++p)header.push_back(0);}else put_u32_le(header,x.off);}
    put_u32_le(header,0);
    for(const auto& x:e)if(x.payload.size()>4u){header.resize(x.off,0);header.insert(header.end(),x.payload.begin(),x.payload.end());}
    header.resize(dataStart,0);
    const std::uint64_t expected=static_cast<std::uint64_t>(dataStart)+static_cast<std::uint64_t>(trr.tileCount())*tileBytes;
    if(expected>std::numeric_limits<std::uint32_t>::max())return false;
    if(::ftruncate(fd,0)!=0||::lseek(fd,0,SEEK_SET)<0||!write_all(fd,header.data(),header.size()))return false;
    std::vector<float> rgb;std::vector<std::uint8_t> roles;std::vector<std::uint8_t> tile(tileBytes,0);
    neg=over=0;
    for(std::size_t i=0;i<trr.tileCount();++i){
        if(!trr.readTileByOrdinal(i,rgb,roles))return false;std::fill(tile.begin(),tile.end(),0);
        const auto& ti=trr.tile(i);
        for(std::uint32_t y=0;y<ti.h;++y)for(std::uint32_t x=0;x<ti.w;++x){
            const auto s=(static_cast<std::size_t>(y)*ti.w+x)*3u;float o[3];transform_rgb(c2srgb,rgb.data()+s,o);
            for(float v:o){if(!std::isfinite(v))return false;if(v<0)++neg;if(v>1)++over;}
            const auto d=(static_cast<std::size_t>(y)*kTileEdge+x)*12u;put_f32_le(tile.data()+d,o[0]);put_f32_le(tile.data()+d+4,o[1]);put_f32_le(tile.data()+d+8,o[2]);
        }
        if(!write_all(fd,tile.data(),tile.size()))return false;
    }
    if(::fsync(fd)!=0)return false;bytesOut=expected;return true;
}

void exr_attr(std::vector<std::uint8_t>& h,const std::string& name,const std::string& type,const std::vector<std::uint8_t>& value){
    h.insert(h.end(),name.begin(),name.end());h.push_back(0);h.insert(h.end(),type.begin(),type.end());h.push_back(0);put_u32_le(h,static_cast<std::uint32_t>(value.size()));h.insert(h.end(),value.begin(),value.end());
}
std::vector<std::uint8_t> exr_i32x4(std::int32_t a,std::int32_t b,std::int32_t c,std::int32_t d){std::vector<std::uint8_t> o;put_u32_le(o,static_cast<std::uint32_t>(a));put_u32_le(o,static_cast<std::uint32_t>(b));put_u32_le(o,static_cast<std::uint32_t>(c));put_u32_le(o,static_cast<std::uint32_t>(d));return o;}

bool write_exr(int fd,TrrReader& trr,const std::array<float,9>& c2srgb,
               std::uint64_t& bytesOut,std::uint64_t& neg,std::uint64_t& over){
    std::vector<std::uint8_t> h;put_u32_le(h,20000630u);put_u32_le(h,2u);
    std::vector<std::uint8_t> ch;
    for(const char* name:{"B","G","R"}){ch.insert(ch.end(),name,name+1);ch.push_back(0);put_u32_le(ch,2u);ch.push_back(0);ch.push_back(0);ch.push_back(0);ch.push_back(0);put_u32_le(ch,1u);put_u32_le(ch,1u);}ch.push_back(0);
    exr_attr(h,"channels","chlist",ch);exr_attr(h,"compression","compression",std::vector<std::uint8_t>{0});
    exr_attr(h,"dataWindow","box2i",exr_i32x4(0,0,static_cast<std::int32_t>(trr.meta().width-1u),static_cast<std::int32_t>(trr.meta().height-1u)));
    exr_attr(h,"displayWindow","box2i",exr_i32x4(0,0,static_cast<std::int32_t>(trr.meta().width-1u),static_cast<std::int32_t>(trr.meta().height-1u)));
    exr_attr(h,"lineOrder","lineOrder",std::vector<std::uint8_t>{0});
    std::vector<std::uint8_t> one;put_f32_le(one,1.f);exr_attr(h,"pixelAspectRatio","float",one);
    std::vector<std::uint8_t> center;put_f32_le(center,0.f);put_f32_le(center,0.f);exr_attr(h,"screenWindowCenter","v2f",center);exr_attr(h,"screenWindowWidth","float",one);
    std::vector<std::uint8_t> chrom;for(float v:{0.64f,0.33f,0.30f,0.60f,0.15f,0.06f,0.3127f,0.3290f})put_f32_le(chrom,v);exr_attr(h,"chromaticities","chromaticities",chrom);
    const auto pv=provenance_text(trr)+"pixel_space=LINEAR_SRGB_D65_FLOAT32\n";
    exr_attr(h,"truthrawProvenance","string",std::vector<std::uint8_t>(pv.begin(),pv.end()));h.push_back(0);
    const std::uint64_t tableStart=h.size(), tableBytes=static_cast<std::uint64_t>(trr.meta().height)*8u;
    const std::uint64_t rowData=static_cast<std::uint64_t>(trr.meta().width)*3u*4u;
    const std::uint64_t blockBytes=8u+rowData, firstBlock=tableStart+tableBytes;
    const std::uint64_t expected=firstBlock+static_cast<std::uint64_t>(trr.meta().height)*blockBytes;
    if(::ftruncate(fd,0)!=0||::lseek(fd,0,SEEK_SET)<0||!write_all(fd,h.data(),h.size()))return false;
    std::vector<std::uint8_t> table;table.reserve(static_cast<std::size_t>(tableBytes));for(std::uint32_t y=0;y<trr.meta().height;++y)put_u64_le(table,firstBlock+static_cast<std::uint64_t>(y)*blockBytes);
    if(!write_all(fd,table.data(),table.size()))return false;
    const std::uint32_t cols=(trr.meta().width+kTileEdge-1u)/kTileEdge;
    std::vector<float> band;std::vector<float> rgb;std::vector<std::uint8_t> roles;std::vector<std::uint8_t> row(static_cast<std::size_t>(rowData));
    neg=over=0;
    for(std::uint32_t y0=0;y0<trr.meta().height;y0+=kTileEdge){
        const std::uint32_t bh=std::min(kTileEdge,trr.meta().height-y0);
        band.assign(static_cast<std::size_t>(bh)*trr.meta().width*3u,0.f);
        const std::size_t tileRow=y0/kTileEdge;
        for(std::uint32_t tx=0;tx<cols;++tx){
            const std::size_t idx=tileRow*cols+tx;if(!trr.readTileByOrdinal(idx,rgb,roles))return false;const auto& t=trr.tile(idx);
            for(std::uint32_t yy=0;yy<t.h;++yy)for(std::uint32_t xx=0;xx<t.w;++xx){
                const auto si=(static_cast<std::size_t>(yy)*t.w+xx)*3u;const auto di=(static_cast<std::size_t>(yy)*trr.meta().width+t.x+xx)*3u;float o[3];transform_rgb(c2srgb,rgb.data()+si,o);
                for(int k=0;k<3;++k){if(!std::isfinite(o[k]))return false;if(o[k]<0)++neg;if(o[k]>1)++over;band[di+k]=o[k];}
            }
        }
        for(std::uint32_t yy=0;yy<bh;++yy){
            std::fill(row.begin(),row.end(),0);const float* src=band.data()+static_cast<std::size_t>(yy)*trr.meta().width*3u;
            for(int channel=2;channel>=0;--channel){const std::size_t plane=static_cast<std::size_t>(2-channel)*trr.meta().width*4u;for(std::uint32_t x=0;x<trr.meta().width;++x)put_f32_le(row.data()+plane+static_cast<std::size_t>(x)*4u,src[3u*x+channel]);}
            std::vector<std::uint8_t> prefix;put_u32_le(prefix,y0+yy);put_u32_le(prefix,static_cast<std::uint32_t>(rowData));
            if(!write_all(fd,prefix.data(),prefix.size())||!write_all(fd,row.data(),row.size()))return false;
        }
    }
    if(::fsync(fd)!=0)return false;bytesOut=expected;return true;
}

jlongArray packet(JNIEnv* env,jlong status){
    std::array<jlong,kPacketLongs> v{};v[0]=kMagic;v[1]=status;
    auto out=env->NewLongArray(static_cast<jsize>(v.size()));if(out)env->SetLongArrayRegion(out,0,static_cast<jsize>(v.size()),v.data());return out;
}
jlong clamp_jlong(std::uint64_t v){return static_cast<jlong>(std::min<std::uint64_t>(v,std::numeric_limits<jlong>::max()));}

} // namespace

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_truthraw_adaptiveui_RestorationProjectionNativeBridge_projectRestoration(
    JNIEnv* env,jobject,jint sourceFd,jint trrFd,jint outputFd,jint format,
    jint maxSourceResidentBytes,jint maxLogicalResidentBytes) {
    if(sourceFd<0||trrFd<0||outputFd<0||(format!=kFormatDng&&format!=kFormatTiff&&format!=kFormatExr)||
       maxSourceResidentBytes<=0||maxLogicalResidentBytes<=0)return packet(env,-1);

    TrrReader trr(static_cast<int>(trrFd));std::string err;
    if(!trr.open(err))return packet(env,-2);

    Lineage line{};jlong ls=0;
    if(!establish_lineage(static_cast<int>(sourceFd),maxSourceResidentBytes,maxLogicalResidentBytes,line,ls))return packet(env,ls);
    if(!lineage_matches(trr.meta(),line))return packet(env,-3);

    std::uint64_t outBytes=0,neg=0,over=0;
    bool rasterVerified=false;
    if(format==kFormatDng){
        float_dng::ProjectionDescriptor d{};
        d.width=trr.meta().width;d.height=trr.meta().height;d.orientation=static_cast<std::uint16_t>(trr.meta().orientation);
        d.sealedSourceSha256=line.seal.sha256;d.scientificMasterSha256=line.scientific.scientificMasterHash;
        d.zeroLineSha256=line.phase2.zeroLineHash;d.sceneScaleSha256=line.phase2.sceneScaleHash;
        d.projectedRasterSha256=trr.meta().derivativeHash;d.zeroLineGauge=line.scientific.zeroLineGauge;d.sceneBinding=line.scientific.sceneBinding;
        d.serializedBackplane=line.phase2.serializedBackplane;d.sourceEvidenceId=line.seal.sourceEvidenceId;d.colorBindingId=line.color.color.bindingId;
        d.precisionPolicyId=kPrecisionPolicy;d.runtimeReconstructionBackendId=line.reconstruction->name();
        d.projectionRole="TRUTHRAW_RESTORATION_FLOAT32_XYZ_D50_LINEAR_DNG_PROJECTION_V0_69";d.restorationDerivative=true;
        FdSink sink(static_cast<int>(outputFd));float_dng::Result r{};
        const auto s=float_dng::write_xyz_d50_linear_dng_projection(trr,d,line.color.color.cameraToXyzD50,sink,r);
        if(!s)return packet(env,dng_status(s));
        if(!r.projectedRasterIdentityVerified||!r.artifactCommitted||r.scientificMasterModified||r.appearanceApplied||r.counterfactualObservationCreated){sink.abort();return packet(env,-4);}
        outBytes=r.bytesWritten;neg=r.negativeComponentCount;over=r.overOneComponentCount;rasterVerified=true;
    } else {
        const auto c2s=camera_to_linear_srgb(line.color.color.cameraToXyzD50);
        bool ok=format==kFormatTiff
            ? write_tiff(static_cast<int>(outputFd),trr,c2s,outBytes,neg,over)
            : write_exr(static_cast<int>(outputFd),trr,c2s,outBytes,neg,over);
        if(!ok){(void)::ftruncate(outputFd,0);return packet(env,-5);}
        rasterVerified=true; // TrrReader already verified derivative identity before projection.
    }

    std::array<jlong,kPacketLongs> v{};
    v[0]=kMagic;v[1]=0;v[2]=format;v[3]=trr.meta().width;v[4]=trr.meta().height;v[5]=clamp_jlong(outBytes);
    v[6]=clamp_jlong(static_cast<std::uint64_t>(trr.meta().width)*trr.meta().height);
    v[7]=clamp_jlong(neg);v[8]=clamp_jlong(over);v[9]=clamp_jlong(trr.role0());v[10]=clamp_jlong(trr.role1());v[11]=clamp_jlong(trr.role2());
    v[12]=rasterVerified?1:0;v[13]=1;v[14]=1;v[15]=1;v[16]=0;v[17]=0;v[18]=1;v[19]=1;
    auto out=env->NewLongArray(static_cast<jsize>(v.size()));if(out)env->SetLongArrayRegion(out,0,static_cast<jsize>(v.size()),v.data());return out;
}
