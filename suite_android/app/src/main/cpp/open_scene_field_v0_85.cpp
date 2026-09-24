#include "open_scene_field_v0_85.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

namespace truthraw::open_scene_field::v0_85 {
namespace {

constexpr std::uint8_t kMagic[4] = {'O','S','F','5'};
constexpr std::uint8_t kEncodingVersion = 1u;

bool nonzero(const Digest& d) noexcept {
    return std::any_of(d.begin(), d.end(), [](std::uint8_t v){ return v != 0u; });
}

void put_u16(std::vector<std::uint8_t>& out, std::uint16_t v) {
    out.push_back(static_cast<std::uint8_t>(v));
    out.push_back(static_cast<std::uint8_t>(v >> 8u));
}
void put_u32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    out.push_back(static_cast<std::uint8_t>(v));
    out.push_back(static_cast<std::uint8_t>(v >> 8u));
    out.push_back(static_cast<std::uint8_t>(v >> 16u));
    out.push_back(static_cast<std::uint8_t>(v >> 24u));
}
void put_u64_hash(truthraw::sha256_v0_69::Hasher& h, std::uint64_t v) noexcept {
    std::array<std::uint8_t,8> b{};
    for(std::size_t i=0;i<b.size();++i) {
        b[i]=static_cast<std::uint8_t>(v >> (8u*i));
    }
    h.update(b);
}
void put_u32_hash(truthraw::sha256_v0_69::Hasher& h, std::uint32_t v) noexcept {
    const std::array<std::uint8_t,4> b{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v >> 8u),
        static_cast<std::uint8_t>(v >> 16u),
        static_cast<std::uint8_t>(v >> 24u)};
    h.update(b);
}
void put_f32(std::vector<std::uint8_t>& out, float v) {
    put_u32(out, std::bit_cast<std::uint32_t>(v));
}
void put_f32_hash(truthraw::sha256_v0_69::Hasher& h, float v) noexcept {
    put_u32_hash(h, std::bit_cast<std::uint32_t>(v));
}

bool get_u16(std::span<const std::uint8_t> in, std::size_t& off, std::uint16_t& v) noexcept {
    if(off > in.size() || in.size()-off < 2u) return false;
    v=static_cast<std::uint16_t>(in[off]) |
      static_cast<std::uint16_t>(static_cast<std::uint16_t>(in[off+1u]) << 8u);
    off+=2u; return true;
}
bool get_u32(std::span<const std::uint8_t> in, std::size_t& off, std::uint32_t& v) noexcept {
    if(off > in.size() || in.size()-off < 4u) return false;
    v=static_cast<std::uint32_t>(in[off]) |
      (static_cast<std::uint32_t>(in[off+1u]) << 8u) |
      (static_cast<std::uint32_t>(in[off+2u]) << 16u) |
      (static_cast<std::uint32_t>(in[off+3u]) << 24u);
    off+=4u; return true;
}
bool get_f32(std::span<const std::uint8_t> in, std::size_t& off, float& v) noexcept {
    std::uint32_t bits=0u;
    if(!get_u32(in,off,bits)) return false;
    v=std::bit_cast<float>(bits);
    return true;
}

int measured_channel(CfaPattern cfa, std::uint32_t x, std::uint32_t y) noexcept {
    const bool xe=(x&1u)==0u;
    const bool ye=(y&1u)==0u;
    switch(cfa){
        case CfaPattern::BGGR: if(ye&&xe)return 2;if(!ye&&!xe)return 0;return 1;
        case CfaPattern::RGGB: if(ye&&xe)return 0;if(!ye&&!xe)return 2;return 1;
        case CfaPattern::GRBG: if(ye&&!xe)return 0;if(!ye&&xe)return 2;return 1;
        case CfaPattern::GBRG: if(!ye&&xe)return 0;if(ye&&!xe)return 2;return 1;
    }
    return -1;
}

bool valid_binding(const Binding& b) noexcept {
    return nonzero(b.sourceEvidenceSha256) &&
           nonzero(b.scientificMasterSha256) &&
           nonzero(b.zeroLineSha256) &&
           nonzero(b.sceneScaleSha256) &&
           b.width>0u && b.height>0u &&
           b.physicalFrameCount==1u &&
           b.independentEvidenceCount==1u &&
           !b.reconstructionBackendId.empty() &&
           !b.colourBindingId.empty();
}

std::size_t role_index(CreationRole r) noexcept {
    const auto v=static_cast<std::uint8_t>(r);
    return v<=4u ? static_cast<std::size_t>(v) : 0u;
}
std::size_t authority_index(Authority a) noexcept {
    const auto v=static_cast<std::uint8_t>(a);
    return (v>=1u && v<=4u) ? static_cast<std::size_t>(v-1u) : 3u;
}
std::size_t uncertainty_index(UncertaintyKnowledge u) noexcept {
    const auto v=static_cast<std::uint8_t>(u);
    return v<=3u ? static_cast<std::size_t>(v) : 0u;
}

bool implicit_unit_support(const ChannelRecord& r) noexcept {
    return r.supportKnown &&
           r.role==CreationRole::SourceMeasuredCfa &&
           r.support==1.0f;
}

ChannelRecord from_word(std::uint32_t word) noexcept {
    ChannelRecord r{};
    r.role=static_cast<CreationRole>(word & 0x0fu);
    r.authority=static_cast<Authority>((word >> 4u) & 0x0fu);
    r.uncertainty=static_cast<UncertaintyKnowledge>((word >> 8u) & 0x0fu);
    r.boundDomain=static_cast<BoundDomain>((word >> 12u) & 0x0fu);
    r.valuePresent=((word >> 16u)&1u)!=0u;
    r.p95Known=((word >> 17u)&1u)!=0u;
    r.supportKnown=((word >> 18u)&1u)!=0u;
    r.boundKnown=((word >> 19u)&1u)!=0u;
    r.contributionMask=static_cast<std::uint8_t>((word >> 20u)&0x0fu);
    const bool unitSupport=((word >> 24u)&1u)!=0u;
    if(unitSupport){
        r.supportKnown=true;
        r.support=1.0f;
    }
    return r;
}

Digest policy_hash(const Binding& b) noexcept {
    truthraw::sha256_v0_69::Hasher h;
    constexpr char policy[]=
        "schema=TruthRawOpenSceneField/0.85\n"
        "domain=PER_PIXEL_PER_RGB_CHANNEL\n"
        "value_role_authority_uncertainty_support_bound_are_separate_axes=1\n"
        "measured_cfa_value_may_be_censored_without_becoming_exact_scene_value=1\n"
        "reconstructed_numeric_value_does_not_imply_reconstructed_authority=1\n"
        "unknown_may_still_have_a_numeric_reconstruction=1\n"
        "source_raw_code_bound_is_not_scene_linear_bound=1\n"
        "dense_projection_never_becomes_measurement=1\n"
        "appearance_cannot_write_scientific_field=1\n"
        "encoding_is_not_scientific_identity=1\n"
        "canonical_tile_edge=64\n"
        "creates_new_evidence=0\n"
        "scientific_writeback_allowed=0\n";
    h.update(reinterpret_cast<const std::uint8_t*>(policy),sizeof(policy)-1u);
    h.update(b.sourceEvidenceSha256);
    h.update(b.scientificMasterSha256);
    h.update(b.zeroLineSha256);
    h.update(b.sceneScaleSha256);
    put_u32_hash(h,b.width);
    put_u32_hash(h,b.height);
    put_u32_hash(h,b.physicalFrameCount);
    put_u32_hash(h,b.independentEvidenceCount);
    h.update(reinterpret_cast<const std::uint8_t*>(b.reconstructionBackendId.data()),
             b.reconstructionBackendId.size());
    h.update(reinterpret_cast<const std::uint8_t*>(b.colourBindingId.data()),
             b.colourBindingId.size());
    return h.finalize();
}

Digest artifact_hash(
    const Binding& b,
    const Digest& parent,
    const Digest& content,
    const Digest& policy) noexcept {
    truthraw::sha256_v0_69::Hasher h;
    constexpr char domain[]="TRUTHRAW_OPEN_SCENE_FIELD_ARTIFACT_V0_85";
    h.update(reinterpret_cast<const std::uint8_t*>(domain),sizeof(domain)-1u);
    h.update(b.sourceEvidenceSha256);
    h.update(b.scientificMasterSha256);
    h.update(parent);
    h.update(content);
    h.update(policy);
    return h.finalize();
}

} // namespace

bool validate_record(const ChannelRecord& r) noexcept {
    const auto role=static_cast<std::uint8_t>(r.role);
    const auto authority=static_cast<std::uint8_t>(r.authority);
    const auto uncertainty=static_cast<std::uint8_t>(r.uncertainty);
    const auto domain=static_cast<std::uint8_t>(r.boundDomain);

    if(role>4u || authority<1u || authority>4u || uncertainty>3u || domain>2u) {
        return false;
    }
    if(r.valuePresent && !std::isfinite(r.value)) return false;
    if(r.p95Known && (!std::isfinite(r.p95) || r.p95<0.0f)) return false;
    if(r.supportKnown &&
       (!std::isfinite(r.support) || r.support<0.0f || r.support>1.0f)) return false;
    if(r.boundKnown && !std::isfinite(r.bound)) return false;
    if(r.boundKnown != (r.boundDomain!=BoundDomain::None)) return false;
    if(r.p95Known && r.uncertainty==UncertaintyKnowledge::Unresolved) return false;
    if(!r.p95Known && r.uncertainty!=UncertaintyKnowledge::Unresolved) return false;

    switch(r.authority){
        case Authority::CalibratedEstimate:
            return r.role==CreationRole::SourceMeasuredCfa &&
                   r.valuePresent &&
                   r.supportKnown && r.support==1.0f &&
                   !r.boundKnown &&
                   (r.contributionMask & ContributionMeasured)!=0u;
        case Authority::Reconstructed:
            return (r.role==CreationRole::ScientificReconstruction ||
                    r.role==CreationRole::DenseProjection ||
                    r.role==CreationRole::RestorationDerivative) &&
                   r.valuePresent &&
                   r.p95Known &&
                   r.supportKnown && r.support>0.0f &&
                   !r.boundKnown &&
                   (r.uncertainty==UncertaintyKnowledge::BackendBoundP95 ||
                    r.uncertainty==UncertaintyKnowledge::CertifiedVariance) &&
                   (r.contributionMask & ContributionReconstructed)!=0u;
        case Authority::Censored:
            return r.valuePresent &&
                   r.boundKnown &&
                   (r.contributionMask & ContributionCensored)!=0u;
        case Authority::Unknown:
            return !r.p95Known &&
                   r.uncertainty==UncertaintyKnowledge::Unresolved &&
                   !r.boundKnown;
    }
    return false;
}

std::uint32_t classification_word(const ChannelRecord& r) noexcept {
    std::uint32_t word=0u;
    word|=static_cast<std::uint32_t>(static_cast<std::uint8_t>(r.role)&0x0fu);
    word|=static_cast<std::uint32_t>(static_cast<std::uint8_t>(r.authority)&0x0fu)<<4u;
    word|=static_cast<std::uint32_t>(static_cast<std::uint8_t>(r.uncertainty)&0x0fu)<<8u;
    word|=static_cast<std::uint32_t>(static_cast<std::uint8_t>(r.boundDomain)&0x0fu)<<12u;
    if(r.valuePresent) word|=1u<<16u;
    if(r.p95Known) word|=1u<<17u;
    if(r.supportKnown) word|=1u<<18u;
    if(r.boundKnown) word|=1u<<19u;
    word|=static_cast<std::uint32_t>(r.contributionMask&0x0fu)<<20u;
    if(implicit_unit_support(r)) word|=1u<<24u;
    return word;
}

bool build_source_tile_records(
    CfaPattern cfa,
    std::uint32_t globalX,
    std::uint32_t globalY,
    std::uint32_t width,
    std::uint32_t height,
    std::span<const std::uint16_t> raw,
    float whiteLevel,
    std::span<const float> rgb,
    std::vector<ChannelRecord>& out) noexcept {
    try {
        if(width==0u || height==0u || !std::isfinite(whiteLevel) || whiteLevel<=0.0f) {
            return false;
        }
        const std::size_t pixels=static_cast<std::size_t>(width)*height;
        if(raw.size()!=pixels || rgb.size()!=pixels*3u) return false;

        out.assign(pixels*3u,ChannelRecord{});
        for(std::uint32_t y=0u;y<height;++y){
            for(std::uint32_t x=0u;x<width;++x){
                const std::size_t pi=static_cast<std::size_t>(y)*width+x;
                const int measured=measured_channel(cfa,globalX+x,globalY+y);
                if(measured<0 || measured>2) return false;
                const bool censored=static_cast<float>(raw[pi])>=whiteLevel;

                for(int ch=0;ch<3;++ch){
                    auto& r=out[3u*pi+static_cast<std::size_t>(ch)];
                    r.value=rgb[3u*pi+static_cast<std::size_t>(ch)];
                    r.valuePresent=std::isfinite(r.value);
                    if(!r.valuePresent) return false;

                    if(ch==measured){
                        r.role=CreationRole::SourceMeasuredCfa;
                        r.supportKnown=true;
                        r.support=1.0f;
                        r.contributionMask=ContributionMeasured;
                        if(censored){
                            r.authority=Authority::Censored;
                            r.boundKnown=true;
                            r.bound=whiteLevel;
                            r.boundDomain=BoundDomain::SourceRawCode;
                            r.contributionMask=static_cast<std::uint8_t>(
                                r.contributionMask | ContributionCensored);
                        }else{
                            r.authority=Authority::CalibratedEstimate;
                        }
                    }else{
                        r.role=CreationRole::ScientificReconstruction;
                        r.authority=Authority::Unknown;
                        r.contributionMask=
                            static_cast<std::uint8_t>(
                                ContributionReconstructed | ContributionUnknown);
                    }
                    if(!validate_record(r)) return false;
                }
            }
        }
        return true;
    } catch(...) {
        out.clear();
        return false;
    }
}

bool encode_tile(
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t width,
    std::uint32_t height,
    std::span<const ChannelRecord> records,
    EncodedTile& out) noexcept {
    out={};
    try {
        if(width==0u || height==0u) return false;
        const std::size_t pixels=static_cast<std::size_t>(width)*height;
        if(pixels>std::numeric_limits<std::size_t>::max()/3u ||
           records.size()!=pixels*3u) return false;

        std::vector<std::uint32_t> words;
        words.reserve(records.size());
        std::vector<std::uint32_t> palette;
        palette.reserve(16u);

        std::uint32_t p95Count=0u,supportScalarCount=0u,boundCount=0u;
        for(const auto& r:records){
            if(!validate_record(r)) return false;
            const auto word=classification_word(r);
            words.push_back(word);
            if(std::find(palette.begin(),palette.end(),word)==palette.end()){
                palette.push_back(word);
            }
            if(r.p95Known) ++p95Count;
            if(r.supportKnown && !implicit_unit_support(r)) ++supportScalarCount;
            if(r.boundKnown) ++boundCount;
        }

        EncodingMode mode=EncodingMode::Dense32;
        if(palette.size()==1u) mode=EncodingMode::Uniform;
        else if(palette.size()<=4u) mode=EncodingMode::Palette2;
        else if(palette.size()<=16u) mode=EncodingMode::Palette4;

        std::vector<std::uint8_t> bytes;
        bytes.reserve(40u + palette.size()*4u + words.size());
        bytes.insert(bytes.end(),std::begin(kMagic),std::end(kMagic));
        bytes.push_back(kEncodingVersion);
        bytes.push_back(static_cast<std::uint8_t>(mode));
        bytes.push_back(static_cast<std::uint8_t>(
            mode==EncodingMode::Dense32 ? 0u : palette.size()));
        bytes.push_back(0u);
        put_u32(bytes,x);put_u32(bytes,y);put_u32(bytes,width);put_u32(bytes,height);
        put_u32(bytes,static_cast<std::uint32_t>(records.size()));
        put_u32(bytes,p95Count);
        put_u32(bytes,supportScalarCount);
        put_u32(bytes,boundCount);

        if(mode!=EncodingMode::Dense32){
            for(const auto word:palette) put_u32(bytes,word);
        }

        if(mode==EncodingMode::Palette2){
            for(std::size_t i=0u;i<words.size();i+=4u){
                std::uint8_t packed=0u;
                for(std::size_t k=0u;k<4u && i+k<words.size();++k){
                    const auto it=std::find(palette.begin(),palette.end(),words[i+k]);
                    if(it==palette.end()) return false;
                    const auto idx=static_cast<std::uint8_t>(it-palette.begin());
                    packed|=static_cast<std::uint8_t>((idx&0x03u)<<(2u*k));
                }
                bytes.push_back(packed);
            }
        }else if(mode==EncodingMode::Palette4){
            for(std::size_t i=0u;i<words.size();i+=2u){
                const auto it0=std::find(palette.begin(),palette.end(),words[i]);
                if(it0==palette.end()) return false;
                std::uint8_t packed=static_cast<std::uint8_t>(it0-palette.begin())&0x0fu;
                if(i+1u<words.size()){
                    const auto it1=std::find(palette.begin(),palette.end(),words[i+1u]);
                    if(it1==palette.end()) return false;
                    packed|=static_cast<std::uint8_t>(
                        (static_cast<std::uint8_t>(it1-palette.begin())&0x0fu)<<4u);
                }
                bytes.push_back(packed);
            }
        }else if(mode==EncodingMode::Dense32){
            for(const auto word:words) put_u32(bytes,word);
        }

        for(const auto& r:records) if(r.p95Known) put_f32(bytes,r.p95);
        for(const auto& r:records) {
            if(r.supportKnown && !implicit_unit_support(r)) put_f32(bytes,r.support);
        }
        for(const auto& r:records) if(r.boundKnown) put_f32(bytes,r.bound);

        out.x=x;out.y=y;out.width=width;out.height=height;
        out.mode=mode;
        out.paletteCount=static_cast<std::uint8_t>(
            mode==EncodingMode::Dense32?0u:palette.size());
        out.recordCount=static_cast<std::uint32_t>(records.size());
        out.p95ScalarCount=p95Count;
        out.supportScalarCount=supportScalarCount;
        out.boundScalarCount=boundCount;
        out.bytes=std::move(bytes);
        return true;
    } catch(...) {
        out={};
        return false;
    }
}

bool decode_tile(
    std::span<const std::uint8_t> encoded,
    EncodedTile& metadata,
    std::vector<ChannelRecord>& records) noexcept {
    metadata={}; records.clear();
    try {
        if(encoded.size()<40u ||
           !std::equal(std::begin(kMagic),std::end(kMagic),encoded.begin())) return false;
        std::size_t off=4u;
        const auto version=encoded[off++];
        const auto modeByte=encoded[off++];
        const auto paletteCount=encoded[off++];
        ++off;
        if(version!=kEncodingVersion ||
           modeByte<static_cast<std::uint8_t>(EncodingMode::Uniform) ||
           modeByte>static_cast<std::uint8_t>(EncodingMode::Dense32)) return false;

        std::uint32_t x=0u,y=0u,w=0u,h=0u,count=0u,p95Count=0u,supportCount=0u,boundCount=0u;
        if(!get_u32(encoded,off,x)||!get_u32(encoded,off,y)||
           !get_u32(encoded,off,w)||!get_u32(encoded,off,h)||
           !get_u32(encoded,off,count)||!get_u32(encoded,off,p95Count)||
           !get_u32(encoded,off,supportCount)||!get_u32(encoded,off,boundCount)) return false;
        if(w==0u||h==0u||
           static_cast<std::uint64_t>(w)*h*3u!=count) return false;

        const auto mode=static_cast<EncodingMode>(modeByte);
        if((mode==EncodingMode::Uniform && paletteCount!=1u) ||
           (mode==EncodingMode::Palette2 && (paletteCount<2u||paletteCount>4u)) ||
           (mode==EncodingMode::Palette4 && (paletteCount<2u||paletteCount>16u)) ||
           (mode==EncodingMode::Dense32 && paletteCount!=0u)) return false;

        std::vector<std::uint32_t> palette;
        for(std::uint8_t i=0u;i<paletteCount;++i){
            std::uint32_t word=0u;if(!get_u32(encoded,off,word))return false;
            palette.push_back(word);
        }

        std::vector<std::uint32_t> words(count,0u);
        if(mode==EncodingMode::Uniform){
            std::fill(words.begin(),words.end(),palette[0]);
        }else if(mode==EncodingMode::Palette2){
            const std::size_t bytesNeeded=(count+3u)/4u;
            if(off>encoded.size()||encoded.size()-off<bytesNeeded)return false;
            for(std::uint32_t i=0u;i<count;++i){
                const auto idx=static_cast<std::uint8_t>(
                    (encoded[off+i/4u]>>(2u*(i%4u)))&0x03u);
                if(idx>=palette.size())return false;
                words[i]=palette[idx];
            }
            off+=bytesNeeded;
        }else if(mode==EncodingMode::Palette4){
            const std::size_t bytesNeeded=(count+1u)/2u;
            if(off>encoded.size()||encoded.size()-off<bytesNeeded)return false;
            for(std::uint32_t i=0u;i<count;++i){
                const auto byte=encoded[off+i/2u];
                const auto idx=static_cast<std::uint8_t>(
                    (i&1u)?(byte>>4u):(byte&0x0fu));
                if(idx>=palette.size())return false;
                words[i]=palette[idx];
            }
            off+=bytesNeeded;
        }else{
            for(std::uint32_t i=0u;i<count;++i){
                if(!get_u32(encoded,off,words[i]))return false;
            }
        }

        records.resize(count);
        std::uint32_t expectedP95=0u,expectedSupport=0u,expectedBound=0u;
        for(std::uint32_t i=0u;i<count;++i){
            records[i]=from_word(words[i]);
            if(records[i].p95Known)++expectedP95;
            if(records[i].supportKnown &&
               !(records[i].role==CreationRole::SourceMeasuredCfa &&
                 records[i].support==1.0f)) ++expectedSupport;
            if(records[i].boundKnown)++expectedBound;
        }
        if(expectedP95!=p95Count||expectedSupport!=supportCount||expectedBound!=boundCount) return false;

        for(auto& r:records) if(r.p95Known) { if(!get_f32(encoded,off,r.p95))return false; }
        for(auto& r:records) {
            if(r.supportKnown &&
               !(r.role==CreationRole::SourceMeasuredCfa && r.support==1.0f)) {
                if(!get_f32(encoded,off,r.support))return false;
            }
        }
        for(auto& r:records) if(r.boundKnown) { if(!get_f32(encoded,off,r.bound))return false; }
        if(off!=encoded.size()) return false;

        for(auto& r:records){
            // The numeric channel value is not duplicated in the metadata stream.
            // Keep valuePresent exactly as encoded so classification identity can be
            // verified; value itself is rebound from the Scientific Master/raster.
            r.value=0.0f;
            if(r.p95Known && (!std::isfinite(r.p95)||r.p95<0.0f))return false;
            if(r.supportKnown && (!std::isfinite(r.support)||r.support<0.0f||r.support>1.0f))return false;
            if(r.boundKnown && !std::isfinite(r.bound))return false;
        }

        metadata.x=x;metadata.y=y;metadata.width=w;metadata.height=h;
        metadata.mode=mode;metadata.paletteCount=paletteCount;metadata.recordCount=count;
        metadata.p95ScalarCount=p95Count;metadata.supportScalarCount=supportCount;
        metadata.boundScalarCount=boundCount;
        metadata.bytes.assign(encoded.begin(),encoded.end());
        return true;
    } catch(...) {
        metadata={};records.clear();return false;
    }
}

Builder::Builder(Binding binding)
    : binding_(std::move(binding)),valid_(valid_binding(binding_)){}

bool Builder::appendTile(
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t width,
    std::uint32_t height,
    std::span<const ChannelRecord> records,
    std::span<const std::uint8_t> encoded) noexcept {
    if(!valid_||finalized_||width==0u||height==0u)return false;
    if(x!=expectedTileX_||y!=expectedTileY_)return false;
    const std::size_t pixels=static_cast<std::size_t>(width)*height;
    if(records.size()!=pixels*3u)return false;
    if(x+width>binding_.width||y+height>binding_.height)return false;
    if(width!=std::min(kCanonicalTileEdge,binding_.width-x) ||
       height!=std::min(kCanonicalTileEdge,binding_.height-y))return false;

    EncodedTile decodedMeta{};
    std::vector<ChannelRecord> decoded;
    if(!decode_tile(encoded,decodedMeta,decoded) ||
       decodedMeta.x!=x||decodedMeta.y!=y||
       decodedMeta.width!=width||decodedMeta.height!=height||
       decodedMeta.recordCount!=records.size() ||
       decoded.size()!=records.size()) return false;

    for(std::size_t i=0u;i<records.size();++i){
        if(classification_word(decoded[i])!=classification_word(records[i]))return false;
        if(records[i].p95Known && decoded[i].p95!=records[i].p95)return false;
        if(records[i].supportKnown && decoded[i].support!=records[i].support)return false;
        if(records[i].boundKnown && decoded[i].bound!=records[i].bound)return false;
    }

    put_u32_hash(contentHasher_,x);put_u32_hash(contentHasher_,y);
    put_u32_hash(contentHasher_,width);put_u32_hash(contentHasher_,height);

    for(const auto& r:records){
        if(!validate_record(r))return false;
        put_u32_hash(contentHasher_,classification_word(r));
        put_f32_hash(contentHasher_,r.value);
        if(r.p95Known)put_f32_hash(contentHasher_,r.p95);
        if(r.supportKnown)put_f32_hash(contentHasher_,r.support);
        if(r.boundKnown)put_f32_hash(contentHasher_,r.bound);

        ++roleCounts_[role_index(r.role)];
        ++authorityCounts_[authority_index(r.authority)];
        ++uncertaintyCounts_[uncertainty_index(r.uncertainty)];
        if(r.p95Known)++p95Known_;
        if(r.supportKnown)++supportKnown_;
        if(r.boundKnown)++boundKnown_;
        ++records_;
    }

    put_u32_hash(encodingHasher_,static_cast<std::uint32_t>(encoded.size()));
    encodingHasher_.update(encoded);
    encodedBytes_+=encoded.size();
    ++tiles_;

    const std::uint32_t nextX=x+kCanonicalTileEdge;
    if(nextX>=binding_.width){
        expectedTileX_=0u;
        expectedTileY_=y+kCanonicalTileEdge;
    }else{
        expectedTileX_=nextX;
        expectedTileY_=y;
    }
    return true;
}

bool Builder::finalize(
    const Digest& parent,
    Summary& out) noexcept {
    out={};
    if(!valid_||finalized_||!nonzero(parent))return false;
    const std::uint64_t expected=
        static_cast<std::uint64_t>(binding_.width)*binding_.height*3u;
    const std::uint64_t expectedTilesX=
        (binding_.width+kCanonicalTileEdge-1u)/kCanonicalTileEdge;
    const std::uint64_t expectedTilesY=
        (binding_.height+kCanonicalTileEdge-1u)/kCanonicalTileEdge;
    if(records_!=expected||tiles_!=expectedTilesX*expectedTilesY||
       expectedTileX_!=0u||expectedTileY_<binding_.height) return false;

    out.contentSha256=contentHasher_.finalize();
    out.encodingSha256=encodingHasher_.finalize();
    out.policySha256=policy_hash(binding_);
    out.artifactSha256=artifact_hash(binding_,parent,out.contentSha256,out.policySha256);
    out.creationRoleCounts=roleCounts_;
    out.authorityCounts=authorityCounts_;
    out.uncertaintyCounts=uncertaintyCounts_;
    out.recordCount=records_;
    out.tileCount=tiles_;
    out.p95KnownCount=p95Known_;
    out.supportKnownCount=supportKnown_;
    out.boundKnownCount=boundKnown_;
    out.encodedBytes=encodedBytes_;
    out.valueFieldBound=true;
    out.perPixelPerChannelAuthority=true;
    out.perPixelPerChannelUncertainty=true;
    out.perPixelPerChannelBounds=true;
    out.encodingChangesScientificIdentity=false;
    out.createsNewEvidence=false;
    out.scientificWritebackAllowed=false;
    finalized_=true;
    return nonzero(out.contentSha256)&&nonzero(out.encodingSha256)&&
           nonzero(out.policySha256)&&nonzero(out.artifactSha256);
}

const char* schema_name() noexcept {
    return "TruthRawOpenSceneField/0.85";
}
const char* creation_role_name(CreationRole role) noexcept {
    switch(role){
        case CreationRole::Unknown:return "UNKNOWN";
        case CreationRole::SourceMeasuredCfa:return "SOURCE_MEASURED_CFA";
        case CreationRole::ScientificReconstruction:return "SCIENTIFIC_RECONSTRUCTION";
        case CreationRole::DenseProjection:return "DENSE_PROJECTION";
        case CreationRole::RestorationDerivative:return "RESTORATION_DERIVATIVE";
    }
    return "INVALID";
}
const char* encoding_mode_name(EncodingMode mode) noexcept {
    switch(mode){
        case EncodingMode::Uniform:return "UNIFORM";
        case EncodingMode::Palette2:return "PALETTE_2BIT";
        case EncodingMode::Palette4:return "PALETTE_4BIT";
        case EncodingMode::Dense32:return "DENSE_U32";
    }
    return "INVALID";
}

} // namespace truthraw::open_scene_field::v0_85
