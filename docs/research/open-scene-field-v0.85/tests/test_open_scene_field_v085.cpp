#include "open_scene_field_v0_85.h"
#include "open_scene_local_policy_v0_86.h"
#include "truthnegative_local_authority_projection_v0_4.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace field = truthraw::open_scene_field::v0_85;
namespace policy = truthraw::open_scene_local_policy::v0_86;
namespace projection = truthraw::truthnegative_local_authority_projection::v0_4;

namespace {

void require(bool condition,const char* message){
    if(!condition){
        std::cerr<<"REQUIRE FAILED: "<<message<<"\n";
        std::exit(2);
    }
}

field::Digest digest(std::uint8_t seed){
    field::Digest d{};
    for(std::size_t i=0;i<d.size();++i)d[i]=static_cast<std::uint8_t>(seed+i);
    return d;
}

int measured_channel(truthraw::CfaPattern cfa,int x,int y){
    const bool xe=(x&1)==0,ye=(y&1)==0;
    switch(cfa){
        case truthraw::CfaPattern::BGGR:if(ye&&xe)return 2;if(!ye&&!xe)return 0;return 1;
        case truthraw::CfaPattern::RGGB:if(ye&&xe)return 0;if(!ye&&!xe)return 2;return 1;
        case truthraw::CfaPattern::GRBG:if(ye&&!xe)return 0;if(!ye&&xe)return 2;return 1;
        case truthraw::CfaPattern::GBRG:if(!ye&&xe)return 0;if(ye&&!xe)return 2;return 1;
    }
    return -1;
}

struct VectorFieldSource final : projection::IFieldTileSource {
    projection::Geometry g{};
    std::vector<field::ChannelRecord> records;

    projection::Geometry geometry() const noexcept override { return g; }

    bool readSourceTile(
        std::uint32_t x,std::uint32_t y,std::uint32_t w,std::uint32_t h,
        field::ChannelRecord* out,std::size_t count) noexcept override {
        if(!out||w==0u||h==0u||
           x+w>g.sourceWidth||y+h>g.sourceHeight||
           count!=static_cast<std::size_t>(w)*h*3u)return false;
        for(std::uint32_t yy=0;yy<h;++yy){
            for(std::uint32_t xx=0;xx<w;++xx){
                for(std::uint32_t c=0;c<3u;++c){
                    const std::size_t src=
                        (static_cast<std::size_t>(y+yy)*g.sourceWidth+(x+xx))*3u+c;
                    const std::size_t dst=
                        (static_cast<std::size_t>(yy)*w+xx)*3u+c;
                    out[dst]=records[src];
                }
            }
        }
        return true;
    }
};


void test_source_field_and_encoding(){
    constexpr std::uint32_t w=4u,h=4u;
    std::vector<std::uint16_t> raw(w*h,100u);
    raw[0]=1023u;
    std::vector<float> rgb(static_cast<std::size_t>(w)*h*3u);
    for(std::size_t i=0;i<rgb.size();++i){
        rgb[i]=static_cast<float>(i)*0.01f-0.15f;
    }

    std::vector<field::ChannelRecord> records;
    require(field::build_source_tile_records(
        truthraw::CfaPattern::BGGR,0u,0u,w,h,raw,1023.0f,rgb,records),
        "source field build");

    std::uint64_t measured=0u,reconstructed=0u,calibrated=0u,censored=0u,unknown=0u;
    for(std::uint32_t y=0;y<h;++y){
        for(std::uint32_t x=0;x<w;++x){
            const std::size_t p=static_cast<std::size_t>(y)*w+x;
            const int m=measured_channel(truthraw::CfaPattern::BGGR,x,y);
            for(int c=0;c<3;++c){
                const auto& r=records[3u*p+static_cast<std::size_t>(c)];
                require(field::validate_record(r),"source record valid");
                require(r.valuePresent,"source value present");
                require(std::bit_cast<std::uint32_t>(r.value)==
                        std::bit_cast<std::uint32_t>(rgb[3u*p+static_cast<std::size_t>(c)]),
                        "source value bit identity");
                if(c==m){
                    ++measured;
                    require(r.role==field::CreationRole::SourceMeasuredCfa,"measured role");
                    require(r.supportKnown&&r.support==1.0f,"direct support=1");
                    if(p==0u){
                        ++censored;
                        require(r.authority==field::Authority::Censored,"censored authority");
                        require(r.boundKnown&&r.bound==1023.0f,"source code bound");
                        require(r.boundDomain==field::BoundDomain::SourceRawCode,"source code domain");
                    }else{
                        ++calibrated;
                        require(r.authority==field::Authority::CalibratedEstimate,"direct authority");
                    }
                }else{
                    ++reconstructed;++unknown;
                    require(r.role==field::CreationRole::ScientificReconstruction,"reconstruction role");
                    require(r.authority==field::Authority::Unknown,"missing channel fail closed");
                    require(!r.p95Known&&r.uncertainty==field::UncertaintyKnowledge::Unresolved,
                            "unknown uncertainty unresolved");
                }
            }
        }
    }
    require(measured==16u&&reconstructed==32u&&calibrated==15u&&censored==1u&&unknown==32u,
            "source counts");

    field::EncodedTile encoded{};
    require(field::encode_tile(0u,0u,w,h,records,encoded),"encode source tile");
    require(encoded.mode==field::EncodingMode::Palette2,"CFA source field uses compact 2-bit palette");
    require(encoded.supportScalarCount==0u,"direct unit support implicit");
    require(encoded.boundScalarCount==1u,"only censored bound stored");
    require(encoded.bytes.size()<records.size()*4u,"encoding smaller than dense classification");

    field::EncodedTile decodedMeta{};
    std::vector<field::ChannelRecord> decoded;
    require(field::decode_tile(encoded.bytes,decodedMeta,decoded),"decode source tile");
    require(decoded.size()==records.size(),"decoded count");
    for(std::size_t i=0;i<records.size();++i){
        require(field::classification_word(decoded[i])==
                field::classification_word(records[i]),"classification round trip");
        if(records[i].boundKnown)require(decoded[i].bound==records[i].bound,"bound round trip");
        if(records[i].supportKnown)require(decoded[i].support==records[i].support,"support round trip");
    }

    field::Binding b{};
    b.sourceEvidenceSha256=digest(1u);
    b.scientificMasterSha256=digest(40u);
    b.zeroLineSha256=digest(80u);
    b.sceneScaleSha256=digest(120u);
    b.width=w;b.height=h;
    b.reconstructionBackendId="research_edge_aware_support_limited_measured_preserving_f64_v0_1";
    b.colourBindingId="TEST_SOURCE_METADATA_BOUND";

    field::Builder builder(b);
    require(builder.valid(),"field builder valid");
    require(builder.appendTile(0u,0u,w,h,records,encoded.bytes),"field append");
    field::Summary summary{};
    require(builder.finalize(digest(160u),summary),"field finalize");
    require(summary.recordCount==48u&&summary.tileCount==1u,"summary counts");
    require(summary.creationRoleCounts[1]==16u&&summary.creationRoleCounts[2]==32u,
            "summary role counts");
    require(summary.authorityCounts[0]==15u&&summary.authorityCounts[2]==1u&&
            summary.authorityCounts[3]==32u,"summary authority counts");
    require(summary.boundKnownCount==1u&&summary.supportKnownCount==16u,
            "summary local metadata counts");
    require(!summary.createsNewEvidence&&!summary.scientificWritebackAllowed,
            "field never upgrades evidence");

    policy::Decision d{};
    require(policy::evaluate(records[2],d),"policy measured/censored eval");
    require(d.restoration==policy::RestorationDisposition::CensorBoundOnly,
            "censored restoration bound only");
    require(d.hdr==policy::HdrDisposition::CensoredExactGainForbidden,
            "censored hdr blocked");
    require(!d.exactCensoredRecoveryAllowed&&!d.scientificWritebackAllowed,
            "censored exact recovery impossible");

    require(policy::evaluate(records[0],d),"policy unknown eval");
    require(d.hdr==policy::HdrDisposition::UnknownHeadroomForbidden,
            "unknown hdr headroom blocked");
    require(d.detail==policy::DetailSupportDisposition::UnknownSupportBlocked,
            "unknown detail blocked");
}

void test_dense_projection_fail_closed(){
    constexpr std::uint32_t sw=4u,sh=4u;
    std::vector<std::uint16_t> raw(sw*sh,100u);
    raw[0]=1023u;
    std::vector<float> rgb(static_cast<std::size_t>(sw)*sh*3u,0.5f);
    std::vector<field::ChannelRecord> sourceRecords;
    require(field::build_source_tile_records(
        truthraw::CfaPattern::BGGR,0u,0u,sw,sh,raw,1023.0f,rgb,sourceRecords),
        "projection source field");

    VectorFieldSource source{};
    source.g={sw,sh,sw*4u,sh*4u};
    source.records=sourceRecords;

    std::vector<float> targetRgb(
        static_cast<std::size_t>(source.g.targetWidth)*source.g.targetHeight*3u,0.75f);
    std::vector<field::ChannelRecord> projected;
    require(projection::project_target_tile(
        source,0u,0u,source.g.targetWidth,source.g.targetHeight,targetRgb,projected),
        "dense local field projection");
    require(projected.size()==targetRgb.size(),"dense projected record count");

    std::uint64_t unknown=0u,censoredContribution=0u,reconstructedContribution=0u;
    for(const auto& r:projected){
        require(r.role==field::CreationRole::DenseProjection,"dense role");
        require(r.authority==field::Authority::Unknown,"resampling fails closed");
        require(!r.p95Known&&r.uncertainty==field::UncertaintyKnowledge::Unresolved,
                "resampling does not invent uncertainty");
        require(!r.boundKnown,"raw-code censor bound not mislabeled scene-linear");
        if((r.contributionMask&field::ContributionCensored)!=0u)++censoredContribution;
        if((r.contributionMask&field::ContributionReconstructed)!=0u)++reconstructedContribution;
        ++unknown;
    }
    require(unknown==projected.size(),"all generic dense channels unknown");
    require(censoredContribution>0u,"censored footprint survives as local provenance");
    require(reconstructedContribution>0u,"reconstruction footprint survives");

    projection::ProjectionSummary summary{};
    const auto projectedRasterSha = digest(210u);
    require(projection::summarize_full_projection(
                source,projectedRasterSha,7u,summary),
            "dense projection summary");
    require(summary.recordCount==
            static_cast<std::uint64_t>(source.g.targetWidth)*source.g.targetHeight*3u,
            "dense summary count");
    require(summary.authorityCounts[3]==summary.recordCount,
            "dense summary all unknown without projection uncertainty");
    require(summary.sourceCensoredSupportRecords>0u,
            "dense summary counts censored support");
    require(summary.perTargetChannelFieldAvailable &&
            !summary.targetMeasuredClaimsCreated &&
            !summary.uncertaintyPromotedByResampling &&
            !summary.createsNewEvidence,
            "dense projection authority invariants");
}

void test_scene_linear_bound_projection(){
    VectorFieldSource source{};
    source.g={1u,1u,4u,4u};
    source.records.resize(3u);
    for(int c=0;c<3;++c){
        auto& r=source.records[static_cast<std::size_t>(c)];
        r.value=2.0f+static_cast<float>(c);
        r.valuePresent=true;
        r.role=field::CreationRole::SourceMeasuredCfa;
        r.authority=field::Authority::Censored;
        r.boundKnown=true;
        r.bound=1.5f+static_cast<float>(c);
        r.boundDomain=field::BoundDomain::SceneLinear;
        r.supportKnown=true;
        r.support=1.0f;
        r.contributionMask=
            static_cast<std::uint8_t>(field::ContributionMeasured|field::ContributionCensored);
        require(field::validate_record(r),"synthetic scene-linear censored record valid");
    }

    std::vector<float> target(4u*4u*3u,3.0f);
    std::vector<field::ChannelRecord> out;
    require(projection::project_target_tile(source,0u,0u,4u,4u,target,out),
            "scene-linear bound projection");
    for(std::size_t i=0;i<out.size();++i){
        const int c=static_cast<int>(i%3u);
        require(out[i].authority==field::Authority::Censored,
                "all-censored scene-linear footprint remains censored");
        require(out[i].boundKnown&&out[i].boundDomain==field::BoundDomain::SceneLinear,
                "scene-linear lower bound retained");
        require(std::abs(out[i].bound-(1.5f+static_cast<float>(c)))<1.0e-6f,
                "scene-linear lower bound convex projection");
    }
}

} // namespace

int main(){
    test_source_field_and_encoding();
    test_dense_projection_fail_closed();
    test_scene_linear_bound_projection();

    std::cout<<"OPEN_SCENE_FIELD_V085_PASS\n";
    std::cout<<"per_pixel_per_channel_authority=1\n";
    std::cout<<"per_pixel_per_channel_uncertainty=1\n";
    std::cout<<"censor_bound_domain_preserved=1\n";
    std::cout<<"dense_projection_measured_claims=0\n";
    std::cout<<"dense_projection_uncertainty_promotion=0\n";
    std::cout<<"local_hdr_restoration_detail_policy=1\n";
    return 0;
}
