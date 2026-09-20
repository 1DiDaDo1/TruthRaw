#include "open_scene_channel_authority_v0_78.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace auth = truthraw::open_scene_channel_authority::v0_78;
namespace stream = truthraw::streaming_v0_1;

namespace {
#define REQUIRE(x) do { if(!(x)){ std::cerr<<"FAIL line "<<__LINE__<<": "<<#x<<"\n"; std::exit(2);} } while(0)

auth::Digest digest(std::uint8_t seed) {
    auth::Digest d{};
    for (std::size_t i=0;i<d.size();++i) d[i]=static_cast<std::uint8_t>(seed+i);
    return d;
}

auth::Binding base_binding(std::uint32_t w,std::uint32_t h) {
    auth::Binding b{};
    b.sourceEvidenceSha256=digest(1);
    b.scientificMasterSha256=digest(41);
    b.zeroLineSha256=digest(81);
    b.sceneScaleSha256=digest(121);
    b.parentOpenSceneV070Sha256=digest(151);
    b.width=w;b.height=h;
    b.reconstructionBackendId="research_edge_aware_support_limited_measured_preserving_v47i";
    return b;
}

auth::ChannelRecord calibrated() {
    auth::ChannelRecord r{};
    r.authority=auth::Authority::CalibratedEstimate;
    r.uncertainty=auth::UncertaintyKnowledge::Unresolved;
    r.supportKnown=true;r.support=1.0f;
    return r;
}
auth::ChannelRecord unknown() { return {}; }
auth::ChannelRecord censored(float bound=1023.0f) {
    auth::ChannelRecord r{};
    r.authority=auth::Authority::Censored;
    r.boundKnown=true;r.bound=bound;r.boundDomain=auth::BoundDomain::SourceRawCode;
    return r;
}
auth::ChannelRecord reconstructed(float p95=0.01f,float support=0.75f) {
    auth::ChannelRecord r{};
    r.authority=auth::Authority::Reconstructed;
    r.uncertainty=auth::UncertaintyKnowledge::BackendBoundP95;
    r.p95Known=true;r.p95=p95;r.supportKnown=true;r.support=support;
    return r;
}

class TinySource final : public stream::IRawTileSource {
public:
    TinySource(int w,int h,std::vector<std::uint16_t> raw) : raw_(std::move(raw)) {
        md_.width=w;md_.height=h;md_.cfa=truthraw::CfaPattern::BGGR;
        md_.orientation=truthraw::Orientation::Normal;md_.whiteLevel=1023.0f;
        md_.blackPhase={64.f,64.f,64.f,64.f};md_.hasGainField=false;md_.hasResidualBlack=false;
        md_.sourceId="tiny-v078";
        REQUIRE(raw_.size()==static_cast<std::size_t>(w)*h);
    }
    const truthraw::DngMetadata& metadata() const override { return md_; }
    std::size_t residentBytesUpperBound() const override { return raw_.size()*sizeof(std::uint16_t); }
    stream::StreamStatus readRawTile(
        const truthraw::TileRect& r,std::uint16_t* out,std::size_t count,
        float* gain,std::size_t gainCount) override {
        if(!out||gain!=nullptr||gainCount!=0u) return stream::StreamStatus::error(stream::StreamStatusCode::SourceFailed,"buffers");
        const int w=r.hx1-r.hx0,h=r.hy1-r.hy0;
        if(count!=static_cast<std::size_t>(w)*h) return stream::StreamStatus::error(stream::StreamStatusCode::SourceFailed,"count");
        ++readCalls;
        for(int y=0;y<h;++y)for(int x=0;x<w;++x)
            out[static_cast<std::size_t>(y)*w+x]=raw_[static_cast<std::size_t>(r.hy0+y)*md_.width+(r.hx0+x)];
        return stream::StreamStatus::ok();
    }
    stream::StreamStatus readRowBias(int,int,float*,std::size_t count) override {
        return count==0u?stream::StreamStatus::ok():stream::StreamStatus::error(stream::StreamStatusCode::SourceFailed,"rowbias");
    }
    stream::StreamStatus readColBias(int,int,float*,std::size_t count) override {
        return count==0u?stream::StreamStatus::ok():stream::StreamStatus::error(stream::StreamStatusCode::SourceFailed,"colbias");
    }
    std::size_t readCalls=0u;
private:
    truthraw::DngMetadata md_{};
    std::vector<std::uint16_t> raw_;
};

void chunk_invariance() {
    auto b=base_binding(2,1);
    std::vector<auth::ChannelRecord> r{
        calibrated(),unknown(),unknown(),
        unknown(),calibrated(),unknown()
    };
    auth::Builder a(b),c(b);REQUIRE(a.valid()&&c.valid());
    REQUIRE(a.append(r));
    REQUIRE(c.append(std::span<const auth::ChannelRecord>(r.data(),2)));
    REQUIRE(c.append(std::span<const auth::ChannelRecord>(r.data()+2,1)));
    REQUIRE(c.append(std::span<const auth::ChannelRecord>(r.data()+3,3)));
    auth::Summary sa{},sc{};REQUIRE(a.finalize(sa));REQUIRE(c.finalize(sc));
    REQUIRE(sa.contentSha256==sc.contentSha256);
    REQUIRE(sa.policySha256==sc.policySha256);
    REQUIRE(sa.artifactSha256==sc.artifactSha256);
    REQUIRE(sa.authorityCounts==sc.authorityCounts);
    REQUIRE(sa.recordCount==6u);
    REQUIRE(!sa.chunkingChangesScientificIdentity);
}

void reconstructed_requires_bound_uncertainty() {
    {
        auto b=base_binding(1,1);
        auth::Builder builder(b);REQUIRE(builder.valid());
        std::array<auth::ChannelRecord,3> records{reconstructed(),unknown(),unknown()};
        REQUIRE(!builder.append(records));
    }
    {
        auto b=base_binding(1,1);
        b.reconstructedAuthorityAllowed=true;
        auth::Builder builder(b);
        REQUIRE(!builder.valid());
    }
    {
        auto b=base_binding(1,1);
        b.reconstructedAuthorityAllowed=true;
        b.uncertaintyBindingSha256=digest(201);
        auth::Builder builder(b);REQUIRE(builder.valid());
        std::array<auth::ChannelRecord,3> records{reconstructed(),reconstructed(0.02f,0.6f),reconstructed(0.03f,0.5f)};
        REQUIRE(builder.append(records));
        auth::Summary s{};REQUIRE(builder.finalize(s));
        REQUIRE(s.authorityCounts[1]==3u);
        REQUIRE(s.uncertaintyCounts[2]==3u);
        REQUIRE(s.p95KnownCount==3u);
    }
}

void censor_and_unknown_fail_closed() {
    auto b=base_binding(1,1);
    {
        auth::Builder builder(b);
        auto bad=censored();bad.boundKnown=false;bad.boundDomain=auth::BoundDomain::None;
        std::array<auth::ChannelRecord,3> r{bad,unknown(),unknown()};
        REQUIRE(!builder.append(r));
    }
    {
        auth::Builder builder(b);
        auto bad=unknown();bad.supportKnown=true;bad.support=0.2f;
        std::array<auth::ChannelRecord,3> r{bad,unknown(),unknown()};
        REQUIRE(!builder.append(r));
    }
}

void generic_source_is_fail_closed() {
    // 4x4 BGGR. Mark two direct source sites at WhiteLevel.
    std::vector<std::uint16_t> raw(16u,300u);
    raw[0]=1023u;raw[15]=1023u;
    TinySource src(4,4,raw);
    auto b=base_binding(4,4);
    auth::Summary s{};
    REQUIRE(auth::build_generic_fail_closed_from_source(src,b,s));
    REQUIRE(s.recordCount==48u);
    REQUIRE(s.authorityCounts[0]==14u); // calibrated direct CFA
    REQUIRE(s.authorityCounts[1]==0u);  // no reconstructed authority
    REQUIRE(s.authorityCounts[2]==2u);  // censored direct CFA
    REQUIRE(s.authorityCounts[3]==32u); // two missing channels per pixel
    REQUIRE(s.uncertaintyCounts[0]==48u);
    REQUIRE(s.p95KnownCount==0u);
    REQUIRE(s.censorBoundCount==2u);
    REQUIRE(!s.createsNewEvidence);
    REQUIRE(!s.scientificWritebackAllowed);
    REQUIRE(!s.counterfactualAuthorityPresent);
    REQUIRE(!s.chunkingChangesScientificIdentity);
    REQUIRE(src.readCalls==1u);
}

void parent_and_uncertainty_change_policy() {
    auto a=base_binding(1,1);
    auto b=a;b.parentOpenSceneV070Sha256[0]^=1u;
    std::array<auth::ChannelRecord,3> r{calibrated(),unknown(),unknown()};
    auth::Builder ba(a),bb(b);REQUIRE(ba.append(r)&&bb.append(r));
    auth::Summary sa{},sb{};REQUIRE(ba.finalize(sa)&&bb.finalize(sb));
    REQUIRE(sa.contentSha256==sb.contentSha256);
    REQUIRE(sa.policySha256!=sb.policySha256);
    REQUIRE(sa.artifactSha256!=sb.artifactSha256);
}

} // namespace

int main(){
    chunk_invariance();
    reconstructed_requires_bound_uncertainty();
    censor_and_unknown_fail_closed();
    generic_source_is_fail_closed();
    parent_and_uncertainty_change_policy();
    std::cout<<"OPEN_SCENE_CHANNEL_AUTHORITY_V0_78_PASS\n";
    return 0;
}
