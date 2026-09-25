#include "truthnegative_n2_cfa_audit_v0_1.h"

#include "full_frame_streaming_v0_1_internal.h"
#include "truthnegative_authority_aware_neighborhood_v0_1.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace truthraw::truthnegative_n2_cfa_audit::v0_1 {
namespace {

namespace detail = truthraw::streaming_v0_1::detail;
namespace neigh = truthraw::truthnegative_authority_aware_neighborhood::v0_1;

bool nonzero(const Digest& d) noexcept {
    return std::any_of(d.begin(),d.end(),[](std::uint8_t v){return v!=0u;});
}

int measured_channel(CfaPattern cfa,int x,int y) noexcept {
    const int phase=(y&1)*2+(x&1);
    static constexpr int bggr[4]={2,1,1,0};
    static constexpr int rggb[4]={0,1,1,2};
    static constexpr int grbg[4]={1,0,2,1};
    static constexpr int gbrg[4]={1,2,0,1};
    const int* map=bggr;
    switch(cfa){
        case CfaPattern::RGGB:map=rggb;break;
        case CfaPattern::GRBG:map=grbg;break;
        case CfaPattern::GBRG:map=gbrg;break;
        case CfaPattern::BGGR:map=bggr;break;
    }
    return map[phase];
}

void hash_u32(truthraw::sha256_v0_69::Hasher& h,std::uint32_t v) noexcept {
    const std::array<std::uint8_t,4u> b{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v>>8u),
        static_cast<std::uint8_t>(v>>16u),
        static_cast<std::uint8_t>(v>>24u)};
    h.update(b);
}

void hash_u64(truthraw::sha256_v0_69::Hasher& h,std::uint64_t v) noexcept {
    std::array<std::uint8_t,8u> b{};
    for(std::size_t i=0;i<8u;++i)b[i]=static_cast<std::uint8_t>(v>>(8u*i));
    h.update(b);
}

void hash_f64(truthraw::sha256_v0_69::Hasher& h,double v) noexcept {
    hash_u64(h,std::bit_cast<std::uint64_t>(v));
}

bool valid_noise_profile(const DngMetadata& md) noexcept {
    if(!md.hasNoiseProfile)return true;
    for(float v:md.noiseProfile){
        if(!std::isfinite(v)||v<0.0f)return false;
    }
    return true;
}

bool variance_for(
    const DngMetadata& md,
    const detail::Workspace& w,
    std::size_t i,
    int channel,
    double mu,
    double& variance) noexcept {
    variance=0.0;
    if(!md.hasNoiseProfile)return false;
    if(channel<0||channel>2||i>=w.stage2.size())return false;
    const double g=md.hasGainField
        ? (i<w.gain.size()?static_cast<double>(w.gain[i]):
           std::numeric_limits<double>::quiet_NaN())
        : 1.0;
    if(!std::isfinite(g)||!(g>0.0)||!std::isfinite(mu))return false;
    const double S=md.noiseProfile[2*channel];
    const double O=md.noiseProfile[2*channel+1];
    variance=g*S*std::max(mu,0.0)+g*g*O;
    return std::isfinite(variance)&&variance>0.0;
}

} // namespace

bool run(
    stream::IRawTileSource& source,
    const Binding& binding,
    const Options& options,
    Result& out) noexcept {
    out={};
    try{
        const auto& md=source.metadata();
        if(!nonzero(binding.sourceEvidenceSha256)||
           !nonzero(binding.truthNegativeStateSha256)||
           md.width<=4||md.height<=4||
           options.tileEdge<8u||
           options.samplingPeriod<2u||
           (options.samplingPeriod&1u)!=0u||
           !valid_noise_profile(md)){
            return false;
        }

        out.noiseProfileAvailable=md.hasNoiseProfile;
        out.samplingPeriod=options.samplingPeriod;
        out.tileEdge=options.tileEdge;

        truthraw::sha256_v0_69::Hasher candidateHasher;
        constexpr char candidateDomain[]="D_RAW_TN_N2_CFA_AUDIT_CANDIDATE_V0_1";
        candidateHasher.update(
            reinterpret_cast<const std::uint8_t*>(candidateDomain),
            sizeof(candidateDomain)-1u);
        candidateHasher.update(binding.sourceEvidenceSha256);
        candidateHasher.update(binding.truthNegativeStateSha256);
        hash_u32(candidateHasher,options.samplingPeriod);

        truthraw::sha256_v0_69::Hasher spatialHasher;
        constexpr char spatialDomain[]="D_RAW_TN_N2_CFA_SPATIAL_AUDIT_V0_1";
        spatialHasher.update(
            reinterpret_cast<const std::uint8_t*>(spatialDomain),
            sizeof(spatialDomain)-1u);
        spatialHasher.update(binding.sourceEvidenceSha256);
        spatialHasher.update(binding.truthNegativeStateSha256);
        hash_u32(spatialHasher,options.tileEdge);
        hash_u32(spatialHasher,options.samplingPeriod);

        detail::Workspace workspace{};
        constexpr int kStep=2;

        for(int y0=0;y0<md.height;y0+=static_cast<int>(options.tileEdge)){
            const int y1=std::min(md.height,y0+static_cast<int>(options.tileEdge));
            for(int x0=0;x0<md.width;x0+=static_cast<int>(options.tileEdge)){
                const int x1=std::min(md.width,x0+static_cast<int>(options.tileEdge));
                TileAudit tile{};
                tile.x=static_cast<std::uint32_t>(x0);
                tile.y=static_cast<std::uint32_t>(y0);
                tile.width=static_cast<std::uint32_t>(x1-x0);
                tile.height=static_cast<std::uint32_t>(y1-y0);
                TileRect t{};
                t.x0=x0;t.y0=y0;t.x1=x1;t.y1=y1;
                t.hx0=std::max(0,x0-kStep);
                t.hy0=std::max(0,y0-kStep);
                t.hx1=std::min(md.width,x1+kStep);
                t.hy1=std::min(md.height,y1+kStep);

                const auto filled=detail::fill_stage2(source,t,workspace);
                if(!filled)return false;
                const int tw=t.hx1-t.hx0;
                const int th=t.hy1-t.hy0;
                const std::size_t expected=static_cast<std::size_t>(tw)*th;
                if(workspace.stage2.size()!=expected||workspace.raw.size()!=expected||
                   (md.hasGainField&&workspace.gain.size()!=expected)){
                    return false;
                }

                const auto idx=[&](int gx,int gy)->std::size_t{
                    return static_cast<std::size_t>(gy-t.hy0)*static_cast<std::size_t>(tw)+
                           static_cast<std::size_t>(gx-t.hx0);
                };

                n2::PixelInput pi{};
                pi.neighborhood.neighbors.reserve(4u);

                for(int gy=y0;gy<y1;++gy){
                    if(static_cast<std::uint32_t>(gy)%options.samplingPeriod!=
                       static_cast<std::uint32_t>(gy&1))continue;
                    for(int gx=x0;gx<x1;++gx){
                        if(static_cast<std::uint32_t>(gx)%options.samplingPeriod!=
                           static_cast<std::uint32_t>(gx&1))continue;

                        const std::size_t ci=idx(gx,gy);
                        const double center=workspace.stage2[ci];
                        if(!std::isfinite(center))return false;

                        ++out.sampled;
                        ++tile.sampled;
                        const auto phaseIndex=
                            static_cast<std::size_t>((gy&1)*2+(gx&1));
                        ++out.cfaPhaseSamples[phaseIndex];
                        ++tile.cfaPhaseSamples[phaseIndex];

                        n2::PixelResult pr{};
                        const bool border=
                            gx<kStep||gy<kStep||
                            gx+kStep>=md.width||gy+kStep>=md.height;
                        if(border){
                            pr.inputValue=center;
                            pr.candidateValue=center;
                            pr.preserveReason=n2::PreserveReason::NoCompatibleNeighborhood;
                            if(!n2::accumulate(pr,out.audit)||
                               !n2::accumulate(pr,tile.audit))return false;
                            ++out.borderProtected;
                            ++tile.borderProtected;
                        }else{
                            const int channel=measured_channel(md.cfa,gx,gy);
                            const bool centerCensored=
                                static_cast<float>(workspace.raw[ci])>=md.whiteLevel;

                            const std::array<std::pair<int,int>,4u> xy{{
                                {gx-kStep,gy},{gx+kStep,gy},
                                {gx,gy-kStep},{gx,gy+kStep}}};

                            std::array<double,4u> values{};
                            std::array<bool,4u> censored{};
                            for(std::size_t k=0;k<xy.size();++k){
                                const auto ni=idx(xy[k].first,xy[k].second);
                                values[k]=workspace.stage2[ni];
                                censored[k]=
                                    static_cast<float>(workspace.raw[ni])>=md.whiteLevel;
                                if(!std::isfinite(values[k]))return false;
                            }
                            const bool boundaryCensored=
                                std::any_of(censored.begin(),censored.end(),
                                            [](bool v){return v;});

                            double centerVariance=0.0;
                            const bool centerVarianceKnown=
                                !centerCensored&&variance_for(
                                    md,workspace,ci,channel,center,centerVariance);
                            const double sigma=centerVarianceKnown
                                ? std::sqrt(centerVariance):0.0;

                            pi.structure={};
                            pi.structure.center=center;
                            pi.structure.left=values[0];
                            pi.structure.right=values[1];
                            pi.structure.up=values[2];
                            pi.structure.down=values[3];
                            pi.structure.sigma=sigma;
                            pi.structure.sigmaKnown=centerVarianceKnown;
                            pi.structure.censored=centerCensored;
                            pi.structure.boundaryCensored=boundaryCensored;
                            pi.structure.measuredSupport=true;
                            pi.structure.registrationConfidence=1.0;
                            pi.structure.visibilityConfidence=1.0;
                            pi.structure.sampleStep=static_cast<double>(kStep);

                            pi.neighborhood.center=center;
                            pi.neighborhood.centerVariance=centerVariance;
                            pi.neighborhood.centerVarianceKnown=centerVarianceKnown;
                            pi.neighborhood.neighbors.clear();

                            for(std::size_t k=0;k<xy.size();++k){
                                const auto ni=idx(xy[k].first,xy[k].second);
                                double nv=0.0;
                                const bool vk=
                                    !censored[k]&&variance_for(
                                        md,workspace,ni,channel,values[k],nv);
                                neigh::Sample s{};
                                s.value=values[k];
                                s.variance=nv;
                                s.spatialDistance=static_cast<double>(kStep);
                                s.authority=censored[k]
                                    ? neigh::SampleAuthority::Censored
                                    : neigh::SampleAuthority::Measured;
                                s.varianceKnown=vk;
                                s.sameChannel=true;
                                s.sameObject=true;
                                s.censorBoundary=censored[k];
                                s.objectIdentityKnown=false;
                                pi.neighborhood.neighbors.push_back(s);
                            }

                            if(!n2::evaluatePixel(pi,pr)||
                               !n2::accumulate(pr,out.audit)||
                               !n2::accumulate(pr,tile.audit)){
                                return false;
                            }
                        }

                        hash_u32(candidateHasher,static_cast<std::uint32_t>(gx));
                        hash_u32(candidateHasher,static_cast<std::uint32_t>(gy));
                        hash_f64(candidateHasher,pr.inputValue);
                        hash_f64(candidateHasher,pr.candidateValue);
                        hash_f64(candidateHasher,pr.correction);
                        hash_u32(
                            candidateHasher,
                            static_cast<std::uint32_t>(pr.preserveReason));
                    }
                }

                if(tile.sampled==0u||tile.audit.total!=tile.sampled)return false;
                hash_u32(spatialHasher,tile.x);
                hash_u32(spatialHasher,tile.y);
                hash_u32(spatialHasher,tile.width);
                hash_u32(spatialHasher,tile.height);
                hash_u64(spatialHasher,tile.sampled);
                hash_u64(spatialHasher,tile.audit.eligible);
                hash_u64(spatialHasher,tile.audit.corrected);
                hash_u64(spatialHasher,tile.audit.preserved);
                hash_u64(spatialHasher,tile.audit.censoredProtected);
                hash_u64(spatialHasher,tile.audit.censorBoundaryProtected);
                hash_u64(spatialHasher,tile.audit.structureProtected);
                hash_u64(spatialHasher,tile.audit.unknownNoiseProtected);
                hash_u64(spatialHasher,tile.audit.noNeighborhoodProtected);
                hash_u64(spatialHasher,tile.audit.residualOutlierProtected);
                hash_u64(spatialHasher,tile.borderProtected);
                hash_f64(spatialHasher,tile.audit.totalResidualEnergy);
                hash_f64(spatialHasher,tile.audit.removedResidualEnergy);
                hash_f64(spatialHasher,tile.audit.maxAbsCorrection);
                for(auto v:tile.cfaPhaseSamples)hash_u64(spatialHasher,v);
                out.tiles.push_back(tile);
            }
        }

        if(out.sampled==0u||out.audit.total!=out.sampled||out.tiles.empty())return false;
        out.spatialSha256=spatialHasher.finalize();
        if(!nonzero(out.spatialSha256))return false;
        out.candidateSha256=candidateHasher.finalize();
        if(!nonzero(out.candidateSha256))return false;

        truthraw::sha256_v0_69::Hasher auditHasher;
        constexpr char auditDomain[]="D_RAW_TN_N2_CFA_AUDIT_REPORT_V0_1";
        auditHasher.update(
            reinterpret_cast<const std::uint8_t*>(auditDomain),
            sizeof(auditDomain)-1u);
        auditHasher.update(binding.sourceEvidenceSha256);
        auditHasher.update(binding.truthNegativeStateSha256);
        auditHasher.update(out.candidateSha256);
        hash_u64(auditHasher,out.sampled);
        hash_u64(auditHasher,out.audit.eligible);
        hash_u64(auditHasher,out.audit.corrected);
        hash_u64(auditHasher,out.audit.preserved);
        hash_u64(auditHasher,out.audit.censoredProtected);
        hash_u64(auditHasher,out.audit.censorBoundaryProtected);
        hash_u64(auditHasher,out.audit.structureProtected);
        hash_u64(auditHasher,out.audit.unknownNoiseProtected);
        hash_u64(auditHasher,out.audit.noNeighborhoodProtected);
        hash_u64(auditHasher,out.audit.residualOutlierProtected);
        hash_u64(auditHasher,out.borderProtected);
        hash_f64(auditHasher,out.audit.totalResidualEnergy);
        hash_f64(auditHasher,out.audit.removedResidualEnergy);
        hash_f64(auditHasher,out.audit.maxAbsCorrection);
        for(auto v:out.cfaPhaseSamples)hash_u64(auditHasher,v);
        out.auditSha256=auditHasher.finalize();

        return nonzero(out.auditSha256)&&
               nonzero(out.spatialSha256)&&
               !out.sourceValuesModified&&
               !out.truthNegativeModified&&
               !out.createsNewEvidence&&
               !out.scientificWritebackAllowed;
    }catch(...){
        out={};
        return false;
    }
}

} // namespace truthraw::truthnegative_n2_cfa_audit::v0_1
