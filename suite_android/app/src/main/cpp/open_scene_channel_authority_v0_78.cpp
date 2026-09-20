#include "open_scene_channel_authority_v0_78.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace truthraw::open_scene_channel_authority::v0_78 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(d.begin(),d.end(),[](std::uint8_t v){return v!=0u;});
}
bool finite_unit(float v) noexcept { return std::isfinite(v) && v>=0.0f && v<=1.0f; }

void u32(truthraw::sha256_v0_69::Hasher& h,std::uint32_t v) noexcept {
    const std::array<std::uint8_t,4> b{
        static_cast<std::uint8_t>(v),static_cast<std::uint8_t>(v>>8u),
        static_cast<std::uint8_t>(v>>16u),static_cast<std::uint8_t>(v>>24u)};
    h.update(b);
}
void u64(truthraw::sha256_v0_69::Hasher& h,std::uint64_t v) noexcept {
    std::array<std::uint8_t,8> b{};
    for(unsigned i=0;i<8;++i)b[i]=static_cast<std::uint8_t>(v>>(8u*i));
    h.update(b);
}
void f32(truthraw::sha256_v0_69::Hasher& h,float v) noexcept {
    u32(h,std::bit_cast<std::uint32_t>(v));
}

bool valid_binding(const Binding& b) noexcept {
    if(!nonzero(b.sourceEvidenceSha256)||!nonzero(b.scientificMasterSha256)||
       !nonzero(b.zeroLineSha256)||!nonzero(b.sceneScaleSha256)||
       !nonzero(b.parentOpenSceneV070Sha256)||b.width==0u||b.height==0u||
       b.physicalFrameCount!=1u||b.independentEvidenceCount!=1u||
       b.reconstructionBackendId.empty()) return false;
    if(b.reconstructedAuthorityAllowed && !nonzero(b.uncertaintyBindingSha256)) return false;
    if(!b.reconstructedAuthorityAllowed && nonzero(b.uncertaintyBindingSha256)) return false;
    return true;
}

bool valid_record(const Binding& b,const ChannelRecord& r) noexcept {
    if(r.supportKnown && !finite_unit(r.support)) return false;
    if(r.p95Known && (!std::isfinite(r.p95)||r.p95<0.0f)) return false;
    if(r.boundKnown && !std::isfinite(r.bound)) return false;

    switch(r.authority){
        case Authority::CalibratedEstimate:
            if(!r.supportKnown || r.support<=0.0f || r.boundKnown ||
               r.boundDomain!=BoundDomain::None) return false;
            if(r.uncertainty==UncertaintyKnowledge::Unresolved) return !r.p95Known;
            return r.p95Known;
        case Authority::Reconstructed:
            return b.reconstructedAuthorityAllowed &&
                   nonzero(b.uncertaintyBindingSha256) &&
                   r.supportKnown && r.support>0.0f &&
                   r.p95Known &&
                   (r.uncertainty==UncertaintyKnowledge::BackendBoundP95 ||
                    r.uncertainty==UncertaintyKnowledge::CertifiedVariance) &&
                   !r.boundKnown && r.boundDomain==BoundDomain::None;
        case Authority::Censored:
            return !r.p95Known &&
                   r.uncertainty==UncertaintyKnowledge::Unresolved &&
                   (!r.supportKnown || r.support==0.0f) &&
                   r.boundKnown && r.boundDomain!=BoundDomain::None;
        case Authority::Unknown:
            return !r.p95Known && !r.boundKnown &&
                   r.uncertainty==UncertaintyKnowledge::Unresolved &&
                   r.boundDomain==BoundDomain::None &&
                   (!r.supportKnown || r.support==0.0f);
    }
    return false;
}

Digest policy_hash(const Binding& b) noexcept {
    truthraw::sha256_v0_69::Hasher h;
    constexpr char p[]=
        "schema=TruthRawOpenSceneChannelAuthority/0.78\n"
        "authority_and_uncertainty_are_independent_axes=1\n"
        "reconstructed_requires_bound_uncertainty=1\n"
        "censored_requires_explicit_bound_and_no_exact_value_claim=1\n"
        "unknown_has_zero_support=1\n"
        "counterfactual_and_appearance_excluded=1\n"
        "scientific_writeback_allowed=0\n"
        "creates_new_evidence=0\n"
        "chunking_changes_scientific_identity=0\n";
    h.update(reinterpret_cast<const std::uint8_t*>(p),sizeof(p)-1u);
    h.update(b.sourceEvidenceSha256);h.update(b.scientificMasterSha256);
    h.update(b.zeroLineSha256);h.update(b.sceneScaleSha256);
    h.update(b.parentOpenSceneV070Sha256);h.update(b.uncertaintyBindingSha256);
    u32(h,b.width);u32(h,b.height);
    const std::uint8_t allow=b.reconstructedAuthorityAllowed?1u:0u;h.update(&allow,1u);
    h.update(reinterpret_cast<const std::uint8_t*>(b.reconstructionBackendId.data()),b.reconstructionBackendId.size());
    return h.finalize();
}

Digest artifact_hash(const Digest& content,const Digest& policy,const Digest& parent) noexcept {
    truthraw::sha256_v0_69::Hasher h;
    constexpr char d[]="TRUTHRAW_OPEN_SCENE_CHANNEL_AUTHORITY_ARTIFACT_V0_78";
    h.update(reinterpret_cast<const std::uint8_t*>(d),sizeof(d)-1u);
    h.update(parent);h.update(content);h.update(policy);
    return h.finalize();
}

int measured_channel(truthraw::CfaPattern cfa,int x,int y) noexcept {
    const bool xe=(x&1)==0,ye=(y&1)==0;
    switch(cfa){
        case truthraw::CfaPattern::BGGR: if(ye&&xe)return 2;if(!ye&&!xe)return 0;return 1;
        case truthraw::CfaPattern::RGGB: if(ye&&xe)return 0;if(!ye&&!xe)return 2;return 1;
        case truthraw::CfaPattern::GRBG: if(ye&&!xe)return 0;if(!ye&&xe)return 2;return 1;
        case truthraw::CfaPattern::GBRG: if(!ye&&xe)return 0;if(ye&&!xe)return 2;return 1;
    }
    return -1;
}
}

Builder::Builder(Binding binding):binding_(std::move(binding)),valid_(valid_binding(binding_)){}

bool Builder::append(std::span<const ChannelRecord> rs) noexcept {
    if(!valid_||finalized_)return false;
    const std::uint64_t expected=static_cast<std::uint64_t>(binding_.width)*binding_.height*3u;
    if(records_+rs.size()>expected)return false;
    for(const auto& r:rs){
        if(!valid_record(binding_,r))return false;
        const std::uint64_t index=records_;
        u64(contentHasher_,index);
        const std::array<std::uint8_t,7> head{
            static_cast<std::uint8_t>(r.authority),
            static_cast<std::uint8_t>(r.uncertainty),
            static_cast<std::uint8_t>(r.boundDomain),
            static_cast<std::uint8_t>(r.p95Known),
            static_cast<std::uint8_t>(r.supportKnown),
            static_cast<std::uint8_t>(r.boundKnown),
            0u};
        contentHasher_.update(head);
        f32(contentHasher_,r.p95Known?r.p95:0.0f);
        f32(contentHasher_,r.supportKnown?r.support:0.0f);
        f32(contentHasher_,r.boundKnown?r.bound:0.0f);
        ++authorityCounts_[static_cast<std::size_t>(static_cast<std::uint8_t>(r.authority)-1u)];
        ++uncertaintyCounts_[static_cast<std::size_t>(r.uncertainty)];
        if(r.p95Known)++p95Known_;
        if(r.boundKnown)++boundKnown_;
        ++records_;
    }
    return true;
}

bool Builder::finalize(Summary& out) noexcept {
    if(!valid_||finalized_)return false;
    const std::uint64_t expected=static_cast<std::uint64_t>(binding_.width)*binding_.height*3u;
    if(records_!=expected)return false;
    out={};out.contentSha256=contentHasher_.finalize();out.policySha256=policy_hash(binding_);
    out.artifactSha256=artifact_hash(out.contentSha256,out.policySha256,binding_.parentOpenSceneV070Sha256);
    out.authorityCounts=authorityCounts_;out.uncertaintyCounts=uncertaintyCounts_;
    out.recordCount=records_;out.p95KnownCount=p95Known_;out.censorBoundCount=boundKnown_;
    out.createsNewEvidence=false;out.scientificWritebackAllowed=false;
    out.counterfactualAuthorityPresent=false;out.chunkingChangesScientificIdentity=false;
    finalized_=true;return true;
}

bool build_generic_fail_closed_from_source(
    truthraw::streaming_v0_1::IRawTileSource& source,const Binding& binding,Summary& out) noexcept {
    try{
        const auto& md=source.metadata();
        if(md.width<=0||md.height<=0||
           binding.width!=static_cast<std::uint32_t>(md.width)||
           binding.height!=static_cast<std::uint32_t>(md.height)||
           binding.reconstructedAuthorityAllowed) return false;
        Builder b(binding);if(!b.valid())return false;
        constexpr int edge=64;
        std::vector<std::uint16_t> raw;std::vector<float> gain;
        std::vector<ChannelRecord> records;
        for(int y=0;y<md.height;y+=edge){
            const int h=std::min(edge,md.height-y);
            for(int x=0;x<md.width;x+=edge){
                const int w=std::min(edge,md.width-x);
                const std::size_t pixels=static_cast<std::size_t>(w)*h;
                raw.resize(pixels);if(md.hasGainField)gain.resize(pixels);else gain.clear();
                truthraw::TileRect rect{x,y,x+w,y+h,x,y,x+w,y+h};
                const auto s=source.readRawTile(rect,raw.data(),raw.size(),
                    md.hasGainField?gain.data():nullptr,md.hasGainField?gain.size():0u);
                if(!s)return false;
                records.assign(pixels*3u,ChannelRecord{});
                for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx){
                    const std::size_t i=static_cast<std::size_t>(yy)*w+xx;
                    const int ch=measured_channel(md.cfa,x+xx,y+yy);if(ch<0)return false;
                    auto& direct=records[3u*i+static_cast<std::size_t>(ch)];
                    if(static_cast<float>(raw[i])>=md.whiteLevel){
                        direct.authority=Authority::Censored;
                        direct.boundKnown=true;direct.bound=md.whiteLevel;
                        direct.boundDomain=BoundDomain::SourceRawCode;
                    }else{
                        direct.authority=Authority::CalibratedEstimate;
                        direct.supportKnown=true;direct.support=1.0f;
                        direct.uncertainty=UncertaintyKnowledge::Unresolved;
                    }
                }
                if(!b.append(records))return false;
            }
        }
        return b.finalize(out);
    }catch(...){return false;}
}

const char* schema_name() noexcept{return "TruthRawOpenSceneChannelAuthority/0.78";}
const char* authority_name(Authority a) noexcept{
    switch(a){case Authority::CalibratedEstimate:return "CALIBRATED_ESTIMATE";
    case Authority::Reconstructed:return "RECONSTRUCTED";case Authority::Censored:return "CENSORED";
    case Authority::Unknown:return "UNKNOWN";}return "INVALID";
}
const char* uncertainty_name(UncertaintyKnowledge u) noexcept{
    switch(u){case UncertaintyKnowledge::Unresolved:return "UNRESOLVED";
    case UncertaintyKnowledge::SourceNoiseProfileP95:return "SOURCE_NOISE_PROFILE_P95";
    case UncertaintyKnowledge::BackendBoundP95:return "BACKEND_BOUND_P95";
    case UncertaintyKnowledge::CertifiedVariance:return "CERTIFIED_VARIANCE";}return "INVALID";
}

} // namespace truthraw::open_scene_channel_authority::v0_78
