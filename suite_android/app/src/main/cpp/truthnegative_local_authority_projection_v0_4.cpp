#include "truthnegative_local_authority_projection_v0_4.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace truthraw::truthnegative_local_authority_projection::v0_4 {
namespace {

std::uint32_t clamp_index(int value, std::uint32_t limit) noexcept {
    if(value<0) return 0u;
    const auto u=static_cast<std::uint32_t>(value);
    return u>=limit ? limit-1u : u;
}

void axis_map(
    std::uint32_t targetCoordinate,
    std::uint32_t sourceLimit,
    std::uint32_t& s0,
    std::uint32_t& s1,
    float& fraction) noexcept {
    const std::uint32_t n=targetCoordinate>>2u;
    int base=0;
    switch(targetCoordinate&3u){
        case 0u:base=static_cast<int>(n)-1;fraction=0.625f;break;
        case 1u:base=static_cast<int>(n)-1;fraction=0.875f;break;
        case 2u:base=static_cast<int>(n);fraction=0.125f;break;
        default:base=static_cast<int>(n);fraction=0.375f;break;
    }
    s0=clamp_index(base,sourceLimit);
    s1=clamp_index(base+1,sourceLimit);
}

void hash_u32(truthraw::sha256_v0_69::Hasher& h,std::uint32_t v) noexcept {
    const std::array<std::uint8_t,4> b{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v>>8u),
        static_cast<std::uint8_t>(v>>16u),
        static_cast<std::uint8_t>(v>>24u)};
    h.update(b);
}
void hash_f32(truthraw::sha256_v0_69::Hasher& h,float v) noexcept {
    hash_u32(h,std::bit_cast<std::uint32_t>(v));
}

std::size_t authority_index(field::Authority a) noexcept {
    const auto v=static_cast<std::uint8_t>(a);
    return (v>=1u&&v<=4u)?static_cast<std::size_t>(v-1u):3u;
}

bool project_one(
    const std::array<const field::ChannelRecord*,4>& source,
    const std::array<float,4>& weight,
    float targetValue,
    field::ChannelRecord& out) noexcept {
    if(!std::isfinite(targetValue)) return false;

    out={};
    out.value=targetValue;
    out.valuePresent=true;
    out.role=field::CreationRole::DenseProjection;
    out.authority=field::Authority::Unknown;
    out.uncertainty=field::UncertaintyKnowledge::Unresolved;
    out.boundDomain=field::BoundDomain::None;

    bool anyCensored=false;
    bool everyNonzeroCensoredSceneLinear=true;
    float sceneLinearLowerBound=0.0f;
    std::uint8_t contributions=field::ContributionNone;

    for(std::size_t i=0u;i<source.size();++i){
        if(weight[i]<=0.0f) continue;
        const auto* r=source[i];
        if(r==nullptr || !r->valuePresent || !std::isfinite(r->value)) return false;
        contributions=static_cast<std::uint8_t>(
            contributions | r->contributionMask);

        if(r->authority==field::Authority::Censored){
            anyCensored=true;
        }
        if(r->authority!=field::Authority::Censored ||
           !r->boundKnown ||
           r->boundDomain!=field::BoundDomain::SceneLinear){
            everyNonzeroCensoredSceneLinear=false;
        }else{
            sceneLinearLowerBound+=weight[i]*r->bound;
        }
    }

    out.contributionMask=contributions;
    if(anyCensored && everyNonzeroCensoredSceneLinear){
        out.authority=field::Authority::Censored;
        out.boundKnown=true;
        out.boundDomain=field::BoundDomain::SceneLinear;
        out.bound=sceneLinearLowerBound;
    }else{
        // Resampling creates a derived numeric value. v0.4 deliberately does not
        // promote uncertainty or scientific authority through interpolation.
        // Even an all-measured footprint therefore remains UNKNOWN until an
        // explicit projection-uncertainty operator is independently admitted.
        out.authority=field::Authority::Unknown;
    }

    return field::validate_record(out);
}

} // namespace

bool build_procedural_binding(
    const field::Digest& sourceEvidenceSha256,
    const field::Digest& scientificMasterSha256,
    const field::Digest& parentOpenSceneSha256,
    const field::Digest& projectedRasterSha256,
    const Geometry& geometry,
    const std::string& reconstructionBackendId,
    ProceduralBinding& out) noexcept {
    out={};
    const auto nonzero=[](const field::Digest& d) noexcept {
        return std::any_of(d.begin(),d.end(),[](std::uint8_t v){return v!=0u;});
    };
    if(!nonzero(sourceEvidenceSha256) ||
       !nonzero(scientificMasterSha256) ||
       !nonzero(parentOpenSceneSha256) ||
       !nonzero(projectedRasterSha256) ||
       geometry.sourceWidth==0u || geometry.sourceHeight==0u ||
       geometry.targetWidth!=geometry.sourceWidth*kScale ||
       geometry.targetHeight!=geometry.sourceHeight*kScale ||
       reconstructionBackendId.empty()) {
        return false;
    }

    truthraw::sha256_v0_69::Hasher policy;
    constexpr char policyText[]=
        "schema=TruthNegativeLocalAuthorityProjection/0.4\n"
        "representation=PROCEDURAL_PER_TARGET_CHANNEL_FIELD\n"
        "source_field_schema=TruthRawOpenSceneField/0.85\n"
        "pixel_center_operator=PIXEL_CENTER_BILINEAR_F32_EXACT_ORDER_V0_3\n"
        "target_creation_role=DENSE_PROJECTION\n"
        "target_measured_claims=0\n"
        "uncertainty_promotion_by_resampling=0\n"
        "source_raw_code_bound_is_not_scene_linear_bound=1\n"
        "mixed_censor_footprint_fails_closed_unknown=1\n"
        "materialized_field_required=0\n"
        "scientific_writeback_allowed=0\n"
        "creates_new_evidence=0\n";
    policy.update(
        reinterpret_cast<const std::uint8_t*>(policyText),
        sizeof(policyText)-1u);
    out.policySha256=policy.finalize();

    truthraw::sha256_v0_69::Hasher artifact;
    constexpr char domain[]=
        "TRUTHNEGATIVE_PROCEDURAL_LOCAL_AUTHORITY_FIELD_V0_4";
    artifact.update(
        reinterpret_cast<const std::uint8_t*>(domain),
        sizeof(domain)-1u);
    artifact.update(sourceEvidenceSha256);
    artifact.update(scientificMasterSha256);
    artifact.update(parentOpenSceneSha256);
    artifact.update(projectedRasterSha256);
    artifact.update(out.policySha256);
    hash_u32(artifact,geometry.sourceWidth);
    hash_u32(artifact,geometry.sourceHeight);
    hash_u32(artifact,geometry.targetWidth);
    hash_u32(artifact,geometry.targetHeight);
    artifact.update(
        reinterpret_cast<const std::uint8_t*>(reconstructionBackendId.data()),
        reconstructionBackendId.size());

    out.sourceEvidenceSha256=sourceEvidenceSha256;
    out.scientificMasterSha256=scientificMasterSha256;
    out.parentOpenSceneSha256=parentOpenSceneSha256;
    out.projectedRasterSha256=projectedRasterSha256;
    out.geometry=geometry;
    out.reconstructionBackendId=reconstructionBackendId;
    out.artifactSha256=artifact.finalize();
    out.perTargetChannelQueryable=true;
    out.materializedFieldRequired=false;
    out.targetMeasuredClaimsCreated=false;
    out.uncertaintyPromotedByResampling=false;
    out.createsNewEvidence=false;
    out.scientificWritebackAllowed=false;
    return nonzero(out.policySha256)&&nonzero(out.artifactSha256);
}

bool project_target_tile(
    IFieldTileSource& source,
    std::uint32_t tx,
    std::uint32_t ty,
    std::uint32_t tw,
    std::uint32_t th,
    std::span<const float> targetRgb,
    std::vector<field::ChannelRecord>& out) noexcept {
    out.clear();
    try{
        const auto g=source.geometry();
        if(g.sourceWidth==0u||g.sourceHeight==0u||
           g.targetWidth!=g.sourceWidth*kScale||
           g.targetHeight!=g.sourceHeight*kScale||
           tw==0u||th==0u||
           tx>=g.targetWidth||ty>=g.targetHeight||
           tx+tw>g.targetWidth||ty+th>g.targetHeight){
            return false;
        }
        const std::size_t targetPixels=static_cast<std::size_t>(tw)*th;
        if(targetRgb.size()!=targetPixels*3u)return false;

        std::uint32_t ax0=0u,ax1=0u,bx0=0u,bx1=0u;
        std::uint32_t ay0=0u,ay1=0u,by0=0u,by1=0u;
        float unused=0.0f;
        axis_map(tx,g.sourceWidth,ax0,ax1,unused);
        axis_map(tx+tw-1u,g.sourceWidth,bx0,bx1,unused);
        axis_map(ty,g.sourceHeight,ay0,ay1,unused);
        axis_map(ty+th-1u,g.sourceHeight,by0,by1,unused);
        const std::uint32_t minX=std::min(std::min(ax0,ax1),std::min(bx0,bx1));
        const std::uint32_t maxX=std::max(std::max(ax0,ax1),std::max(bx0,bx1));
        const std::uint32_t minY=std::min(std::min(ay0,ay1),std::min(by0,by1));
        const std::uint32_t maxY=std::max(std::max(ay0,ay1),std::max(by0,by1));
        const std::uint32_t sw=maxX-minX+1u;
        const std::uint32_t sh=maxY-minY+1u;

        std::vector<field::ChannelRecord> patch(
            static_cast<std::size_t>(sw)*sh*3u);
        if(!source.readSourceTile(
                minX,minY,sw,sh,patch.data(),patch.size())){
            return false;
        }

        const auto sample=[&](std::uint32_t sx,std::uint32_t sy,int ch)
            -> const field::ChannelRecord* {
            if(sx<minX||sy<minY||sx>maxX||sy>maxY||ch<0||ch>2)return nullptr;
            const std::size_t i=
                (static_cast<std::size_t>(sy-minY)*sw+(sx-minX))*3u+
                static_cast<std::size_t>(ch);
            return i<patch.size()?&patch[i]:nullptr;
        };

        out.resize(targetPixels*3u);
        for(std::uint32_t oy=0u;oy<th;++oy){
            std::uint32_t y0=0u,y1=0u;float fy=0.0f;
            axis_map(ty+oy,g.sourceHeight,y0,y1,fy);
            for(std::uint32_t ox=0u;ox<tw;++ox){
                std::uint32_t x0=0u,x1=0u;float fx=0.0f;
                axis_map(tx+ox,g.sourceWidth,x0,x1,fx);
                const std::array<float,4> weights{
                    (1.0f-fx)*(1.0f-fy),
                    fx*(1.0f-fy),
                    (1.0f-fx)*fy,
                    fx*fy};
                const std::size_t pi=static_cast<std::size_t>(oy)*tw+ox;
                for(int ch=0;ch<3;++ch){
                    const std::array<const field::ChannelRecord*,4> src{
                        sample(x0,y0,ch),sample(x1,y0,ch),
                        sample(x0,y1,ch),sample(x1,y1,ch)};
                    if(!project_one(
                            src,weights,
                            targetRgb[3u*pi+static_cast<std::size_t>(ch)],
                            out[3u*pi+static_cast<std::size_t>(ch)])){
                        out.clear();return false;
                    }
                }
            }
        }
        return true;
    }catch(...){
        out.clear();return false;
    }
}

bool summarize_full_projection(
    IFieldTileSource& source,
    const field::Digest& projectedRasterSha256,
    std::uint32_t tileEdge,
    ProjectionSummary& out) noexcept {
    out={};
    try{
        const auto g=source.geometry();
        if(g.sourceWidth==0u||g.sourceHeight==0u||
           g.targetWidth!=g.sourceWidth*kScale||
           g.targetHeight!=g.sourceHeight*kScale||
           tileEdge==0u||
           !std::any_of(
               projectedRasterSha256.begin(),projectedRasterSha256.end(),
               [](std::uint8_t v){return v!=0u;}))return false;

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[]=
            "TruthNegativeLocalAuthorityProjection/0.4\n"
            "field_value_binding=PROJECTED_RASTER_SHA256\n"
            "target_role=DENSE_PROJECTION\n"
            "measured_target_claims=0\n"
            "resampling_uncertainty_promotion=0\n"
            "source_raw_bounds_not_relabelled_scene_linear=1\n"
            "censored_mixed_footprint_fails_to_unknown=1\n"
            "creates_new_evidence=0\n";
        h.update(reinterpret_cast<const std::uint8_t*>(domain),sizeof(domain)-1u);
        h.update(projectedRasterSha256);
        hash_u32(h,g.sourceWidth);hash_u32(h,g.sourceHeight);
        hash_u32(h,g.targetWidth);hash_u32(h,g.targetHeight);

        std::vector<field::ChannelRecord> records;
        std::vector<float> placeholder;
        for(std::uint32_t y=0u;y<g.targetHeight;y+=tileEdge){
            const auto th=std::min(tileEdge,g.targetHeight-y);
            for(std::uint32_t x=0u;x<g.targetWidth;x+=tileEdge){
                const auto tw=std::min(tileEdge,g.targetWidth-x);
                placeholder.assign(static_cast<std::size_t>(tw)*th*3u,0.0f);
                if(!project_target_tile(
                        source,x,y,tw,th,placeholder,records))return false;

                hash_u32(h,x);hash_u32(h,y);hash_u32(h,tw);hash_u32(h,th);
                for(const auto& r:records){
                    hash_u32(h,field::classification_word(r));
                    if(r.boundKnown)hash_f32(h,r.bound);
                    ++out.authorityCounts[authority_index(r.authority)];
                    ++out.contributionMaskCounts[
                        static_cast<std::size_t>(r.contributionMask&0x0fu)];
                    if((r.contributionMask&field::ContributionCensored)!=0u)
                        ++out.sourceCensoredSupportRecords;
                    if((r.contributionMask&field::ContributionUnknown)!=0u)
                        ++out.sourceUnknownSupportRecords;
                    if(r.boundKnown&&r.boundDomain==field::BoundDomain::SceneLinear)
                        ++out.sceneLinearBoundRecords;
                    ++out.recordCount;
                }
            }
        }

        const std::uint64_t expected=
            static_cast<std::uint64_t>(g.targetWidth)*g.targetHeight*3u;
        if(out.recordCount!=expected)return false;
        out.contentSha256=h.finalize();
        out.perTargetChannelFieldAvailable=true;
        out.targetMeasuredClaimsCreated=false;
        out.uncertaintyPromotedByResampling=false;
        out.createsNewEvidence=false;
        return std::any_of(
            out.contentSha256.begin(),out.contentSha256.end(),
            [](std::uint8_t v){return v!=0u;});
    }catch(...){
        out={};return false;
    }
}

const char* schema_name() noexcept {
    return "TruthNegativeLocalAuthorityProjection/0.4";
}
const char* policy_name() noexcept {
    return "DENSE_ROLE_LOCAL_AUTHORITY_FAIL_CLOSED_NO_UNCERTAINTY_PROMOTION";
}

} // namespace truthraw::truthnegative_local_authority_projection::v0_4
