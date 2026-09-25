#include "truthnegative_native_container_v0_1.h"

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cstring>
#include <limits>
#include <sstream>

namespace truthraw::truthnegative_native_container::v0_1 {
namespace {

constexpr std::size_t kTileHeaderBytes = 64u;

bool nonzero(const Digest& d) noexcept {
    return std::any_of(d.begin(), d.end(),
        [](std::uint8_t v){ return v != 0u; });
}

std::string hex(const Digest& d) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out(d.size()*2u, '0');
    for(std::size_t i=0;i<d.size();++i){
        out[2*i]=kHex[d[i]>>4u];
        out[2*i+1]=kHex[d[i]&0x0fu];
    }
    return out;
}

bool unhex(std::string_view s, Digest& out) noexcept {
    if(s.size()!=64u) return false;
    auto nib=[](char c)->int{
        if(c>='0'&&c<='9') return c-'0';
        if(c>='a'&&c<='f') return 10+c-'a';
        if(c>='A'&&c<='F') return 10+c-'A';
        return -1;
    };
    for(std::size_t i=0;i<32u;++i){
        int a=nib(s[2*i]), b=nib(s[2*i+1]);
        if(a<0||b<0) return false;
        out[i]=static_cast<std::uint8_t>((a<<4)|b);
    }
    return true;
}

void put32(std::uint8_t* p, std::uint32_t v) noexcept {
    p[0]=static_cast<std::uint8_t>(v);
    p[1]=static_cast<std::uint8_t>(v>>8u);
    p[2]=static_cast<std::uint8_t>(v>>16u);
    p[3]=static_cast<std::uint8_t>(v>>24u);
}
void put64(std::uint8_t* p, std::uint64_t v) noexcept {
    for(unsigned i=0;i<8u;++i) p[i]=static_cast<std::uint8_t>(v>>(8u*i));
}
std::uint32_t get32(const std::uint8_t* p) noexcept {
    return static_cast<std::uint32_t>(p[0]) |
      (static_cast<std::uint32_t>(p[1])<<8u) |
      (static_cast<std::uint32_t>(p[2])<<16u) |
      (static_cast<std::uint32_t>(p[3])<<24u);
}
std::uint64_t get64(const std::uint8_t* p) noexcept {
    std::uint64_t v=0u;
    for(unsigned i=0;i<8u;++i) v|=static_cast<std::uint64_t>(p[i])<<(8u*i);
    return v;
}

std::string makeHeader(const WriteInput& input, const Summary& s) {
    std::ostringstream o;
    o<<"magic=DRAW_TRUTHNEGATIVE_NATIVE_CONTAINER_V01\n";
    o<<"container_version=1\n";
    o<<"schema="<<kSchemaName<<"\n";
    o<<"width="<<s.width<<"\n";
    o<<"height="<<s.height<<"\n";
    o<<"tile_edge="<<field::kCanonicalTileEdge<<"\n";
    o<<"tile_count="<<s.tileCount<<"\n";
    o<<"record_count="<<s.recordCount<<"\n";
    o<<"body_bytes="<<s.bodyBytes<<"\n";
    o<<"source_sha256="<<hex(s.sourceEvidenceSha256)<<"\n";
    o<<"scientific_master_sha256="<<hex(s.scientificMasterSha256)<<"\n";
    o<<"authority_field_sha256="<<hex(s.authorityFieldSha256)<<"\n";
    o<<"truthnegative_state_sha256="<<hex(s.truthNegativeStateSha256)<<"\n";
    o<<"body_sha256="<<hex(s.bodySha256)<<"\n";
    o<<"color_binding_id="<<input.colorBindingId<<"\n";
    o<<"physical_frame_count=1\n";
    o<<"independent_evidence_count=1\n";
    o<<"creates_new_evidence=0\n";
    o<<"scientific_writeback_allowed=0\n";
    o<<"payload=OPEN_SCENE_FIELD_V085_ENCODED_CANONICAL_TILES\n";
    o<<"END_HEADER\n";
    return o.str();
}

bool parseUnsigned(
    std::string_view header,
    std::string_view key,
    std::uint64_t& out) noexcept {
    const std::string needle = std::string(key)+"=";
    auto p=header.find(needle);
    if(p==std::string_view::npos) return false;
    p+=needle.size();
    auto e=header.find('\n',p);
    auto v=header.substr(p,e-p);
    auto res=std::from_chars(v.data(),v.data()+v.size(),out);
    return res.ec==std::errc{} && res.ptr==v.data()+v.size();
}

bool parseDigest(
    std::string_view header,
    std::string_view key,
    Digest& out) noexcept {
    const std::string needle=std::string(key)+"=";
    auto p=header.find(needle);
    if(p==std::string_view::npos) return false;
    p+=needle.size();
    auto e=header.find('\n',p);
    return unhex(header.substr(p,e-p),out);
}

} // namespace

bool write(
    const WriteInput& input,
    local::IFieldTileSource& fieldSource,
    IRandomAccessSink& sink,
    Summary& out) noexcept {
    out = Summary{};
    try {
        const auto g=fieldSource.geometry();
        if(!input.state.finalized ||
           input.state.createsNewEvidence ||
           input.state.scientificWritebackAllowed ||
           input.state.input.physicalFrameCount!=1u ||
           input.state.input.independentEvidenceCount!=1u ||
           g.sourceWidth!=input.state.input.width ||
           g.sourceHeight!=input.state.input.height ||
           input.colorBindingId.empty()) return false;

        out.sourceEvidenceSha256=input.state.input.sourceEvidenceSha256;
        out.scientificMasterSha256=input.state.input.scientificMasterSha256;
        out.authorityFieldSha256=input.state.input.authorityFieldSha256;
        out.truthNegativeStateSha256=input.state.stateSha256;
        out.width=g.sourceWidth; out.height=g.sourceHeight;

        std::array<std::uint8_t,kHeaderBytes> blank{};
        if(!sink.writeAt(0u,blank.data(),blank.size())) return false;

        truthraw::sha256_v0_69::Hasher bodyHasher;
        std::uint64_t offset=kHeaderBytes;
        std::vector<field::ChannelRecord> records;

        for(std::uint32_t y=0;y<g.sourceHeight;y+=field::kCanonicalTileEdge){
            const auto h=std::min(field::kCanonicalTileEdge,g.sourceHeight-y);
            for(std::uint32_t x=0;x<g.sourceWidth;x+=field::kCanonicalTileEdge){
                const auto w=std::min(field::kCanonicalTileEdge,g.sourceWidth-x);
                const std::size_t count=static_cast<std::size_t>(w)*h*3u;
                records.assign(count,field::ChannelRecord{});
                if(!fieldSource.readSourceTile(x,y,w,h,records.data(),records.size()))
                    return false;
                field::EncodedTile enc{};
                if(!field::encode_tile(x,y,w,h,records,enc)) return false;

                const Digest payloadSha=
                    truthraw::sha256_v0_69::compute(enc.bytes);
                std::array<std::uint8_t,kTileHeaderBytes> th{};
                put32(th.data()+0,x); put32(th.data()+4,y);
                put32(th.data()+8,w); put32(th.data()+12,h);
                put32(th.data()+16,static_cast<std::uint32_t>(enc.mode));
                put32(th.data()+20,enc.recordCount);
                put32(th.data()+24,static_cast<std::uint32_t>(enc.bytes.size()));
                std::memcpy(th.data()+32,payloadSha.data(),payloadSha.size());

                if(!sink.writeAt(offset,th.data(),th.size())) return false;
                bodyHasher.update(th);
                offset+=th.size();
                if(!enc.bytes.empty()){
                    if(!sink.writeAt(offset,enc.bytes.data(),enc.bytes.size())) return false;
                    bodyHasher.update(enc.bytes);
                    offset+=enc.bytes.size();
                }
                ++out.tileCount;
                out.recordCount+=enc.recordCount;
            }
        }

        out.bodyBytes=offset-kHeaderBytes;
        out.fileBytes=offset;
        out.bodySha256=bodyHasher.finalize();

        const std::string header=makeHeader(input,out);
        if(header.size()>kHeaderBytes) return false;
        std::array<std::uint8_t,kHeaderBytes> hb{};
        std::memcpy(hb.data(),header.data(),header.size());
        if(!sink.writeAt(0u,hb.data(),hb.size())) return false;
        if(!sink.resize(offset)) return false;

        truthraw::sha256_v0_69::Hasher containerHasher;
        containerHasher.update(hb);
        containerHasher.update(out.bodySha256);
        out.containerSha256=containerHasher.finalize();
        out.physicalFrameCountOne=true;
        out.independentEvidenceCountOne=true;
        out.createsNewEvidence=false;
        out.scientificWritebackAllowed=false;
        return nonzero(out.containerSha256);
    } catch(...) {
        out=Summary{};
        return false;
    }
}

bool Reader::open(const IRandomAccessSource& source) noexcept {
    source_=nullptr; summary_=Summary{}; tiles_.clear(); valid_=false; error_.clear();
    try{
        if(source.sizeBytes()<kHeaderBytes){ error_="container too short"; return false; }
        std::array<std::uint8_t,kHeaderBytes> hb{};
        if(!source.readAt(0u,hb.data(),hb.size())){error_="header read failed";return false;}
        const std::string_view header(
            reinterpret_cast<const char*>(hb.data()),hb.size());
        if(header.find("magic=DRAW_TRUTHNEGATIVE_NATIVE_CONTAINER_V01\n")==std::string_view::npos ||
           header.find("container_version=1\n")==std::string_view::npos ||
           header.find("creates_new_evidence=0\n")==std::string_view::npos ||
           header.find("scientific_writeback_allowed=0\n")==std::string_view::npos){
            error_="header contract mismatch"; return false;
        }
        std::uint64_t w=0,h=0,tc=0,rc=0,bb=0;
        if(!parseUnsigned(header,"width",w)||!parseUnsigned(header,"height",h)||
           !parseUnsigned(header,"tile_count",tc)||!parseUnsigned(header,"record_count",rc)||
           !parseUnsigned(header,"body_bytes",bb)||
           w==0||h==0||w>std::numeric_limits<std::uint32_t>::max()||
           h>std::numeric_limits<std::uint32_t>::max()||
           kHeaderBytes+bb!=source.sizeBytes()){
            error_="header geometry/size invalid"; return false;
        }
        summary_.width=static_cast<std::uint32_t>(w);
        summary_.height=static_cast<std::uint32_t>(h);
        summary_.tileCount=tc; summary_.recordCount=rc; summary_.bodyBytes=bb;
        summary_.fileBytes=source.sizeBytes();
        if(!parseDigest(header,"source_sha256",summary_.sourceEvidenceSha256)||
           !parseDigest(header,"scientific_master_sha256",summary_.scientificMasterSha256)||
           !parseDigest(header,"authority_field_sha256",summary_.authorityFieldSha256)||
           !parseDigest(header,"truthnegative_state_sha256",summary_.truthNegativeStateSha256)||
           !parseDigest(header,"body_sha256",summary_.bodySha256)){
            error_="header digest invalid"; return false;
        }

        truthraw::sha256_v0_69::Hasher bodyHasher;
        std::uint64_t off=kHeaderBytes;
        for(std::uint64_t ti=0;ti<tc;++ti){
            if(off+kTileHeaderBytes>source.sizeBytes()){error_="tile header overflow";return false;}
            std::array<std::uint8_t,kTileHeaderBytes> th{};
            if(!source.readAt(off,th.data(),th.size())){error_="tile header read failed";return false;}
            bodyHasher.update(th);
            TileIndex idx{};
            idx.x=get32(th.data()+0); idx.y=get32(th.data()+4);
            idx.width=get32(th.data()+8); idx.height=get32(th.data()+12);
            const auto recordCount=get32(th.data()+20);
            idx.payloadBytes=get32(th.data()+24);
            std::memcpy(idx.payloadSha256.data(),th.data()+32,32u);
            idx.payloadOffset=off+kTileHeaderBytes;
            if(idx.width==0u||idx.height==0u||
               recordCount!=static_cast<std::uint64_t>(idx.width)*idx.height*3u||
               idx.payloadOffset+idx.payloadBytes>source.sizeBytes()){
                error_="tile metadata invalid";return false;
            }
            std::vector<std::uint8_t> payload(idx.payloadBytes);
            if(idx.payloadBytes>0u &&
               !source.readAt(idx.payloadOffset,payload.data(),payload.size())){
                error_="tile payload read failed";return false;
            }
            bodyHasher.update(payload);
            if(truthraw::sha256_v0_69::compute(payload)!=idx.payloadSha256){
                error_="tile payload sha mismatch";return false;
            }
            field::EncodedTile meta{};
            std::vector<field::ChannelRecord> decoded;
            if(!field::decode_tile(payload,meta,decoded)||
               meta.x!=idx.x||meta.y!=idx.y||
               meta.width!=idx.width||meta.height!=idx.height||
               decoded.size()!=recordCount){
                error_="tile decode mismatch";return false;
            }
            tiles_.push_back(idx);
            off=idx.payloadOffset+idx.payloadBytes;
        }
        if(off!=source.sizeBytes()||bodyHasher.finalize()!=summary_.bodySha256){
            error_="body digest/length mismatch";return false;
        }
        truthraw::sha256_v0_69::Hasher containerHasher;
        containerHasher.update(hb);
        containerHasher.update(summary_.bodySha256);
        summary_.containerSha256=containerHasher.finalize();
        summary_.physicalFrameCountOne=true;
        summary_.independentEvidenceCountOne=true;
        summary_.createsNewEvidence=false;
        summary_.scientificWritebackAllowed=false;
        source_=&source; valid_=true;
        return true;
    }catch(...){ error_="unexpected container parse failure"; return false; }
}

local::Geometry Reader::geometry() const noexcept {
    return {summary_.width,summary_.height,summary_.width,summary_.height};
}

bool Reader::readSourceTile(
    std::uint32_t x,std::uint32_t y,std::uint32_t width,std::uint32_t height,
    field::ChannelRecord* out,std::size_t recordCount) noexcept {
    if(!valid_||source_==nullptr||out==nullptr) return false;
    const auto it=std::find_if(tiles_.begin(),tiles_.end(),[&](const TileIndex& t){
        return t.x==x&&t.y==y&&t.width==width&&t.height==height;
    });
    if(it==tiles_.end()||
       recordCount!=static_cast<std::size_t>(width)*height*3u) return false;
    try{
        std::vector<std::uint8_t> payload(it->payloadBytes);
        if(it->payloadBytes>0u &&
           !source_->readAt(it->payloadOffset,payload.data(),payload.size())) return false;
        field::EncodedTile meta{};
        std::vector<field::ChannelRecord> decoded;
        if(!field::decode_tile(payload,meta,decoded)||decoded.size()!=recordCount) return false;
        std::copy(decoded.begin(),decoded.end(),out);
        return true;
    }catch(...){ return false; }
}

const char* schema_name() noexcept { return kSchemaName; }

} // namespace truthraw::truthnegative_native_container::v0_1
