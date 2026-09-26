#include "truthnegative_center_excluded_spatial_audit_v0_2_1.h"
#include "truthnegative_n2_cfa_audit_v0_1.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace ce=
    truthraw::truthnegative_center_excluded_spatial_audit::v0_2_1;
namespace a=truthraw::truthnegative_n2_cfa_audit::v0_1;
namespace st=truthraw::streaming_v0_1;

#define R(x) do{if(!(x))throw std::runtime_error(#x);}while(0)

class FakeSource final:public st::IRawTileSource{
public:
    FakeSource(){
        md.width=64;
        md.height=64;
        md.cfa=truthraw::CfaPattern::BGGR;
        md.whiteLevel=1023;
        md.blackPhase={64,64,64,64};
        md.hasNoiseProfile=true;
        md.noiseProfile={
            0.004f,0.00002f,
            0.004f,0.00002f,
            0.004f,0.00002f};
        raw.resize(64u*64u,300u);
        for(int y=0;y<64;++y){
            for(int x=0;x<64;++x){
                int v=300+((x+y)%3)-1;
                if(x>=32&&y>=8&&y<56){
                    v+=((y/4)%2)==0?20:-20;
                }
                raw[static_cast<std::size_t>(y)*64u+
                    static_cast<std::size_t>(x)]=
                    static_cast<std::uint16_t>(v);
            }
        }
        for(int y=24;y<32;++y){
            for(int x=40;x<48;++x){
                raw[static_cast<std::size_t>(y)*64u+
                    static_cast<std::size_t>(x)]=1023u;
            }
        }
    }

    const truthraw::DngMetadata& metadata() const override{return md;}
    std::size_t residentBytesUpperBound() const override{
        return raw.size()*sizeof(std::uint16_t);
    }

    st::StreamStatus readRawTile(
        const truthraw::TileRect& t,
        std::uint16_t* out,
        std::size_t n,
        float*,
        std::size_t) override {
        const int w=t.hx1-t.hx0;
        const int h=t.hy1-t.hy0;
        if(w<=0||h<=0||
           n!=static_cast<std::size_t>(w)*
               static_cast<std::size_t>(h)){
            return st::StreamStatus::error(
                st::StreamStatusCode::InvalidArgument,"size");
        }
        for(int yy=0;yy<h;++yy){
            for(int xx=0;xx<w;++xx){
                out[static_cast<std::size_t>(yy)*
                        static_cast<std::size_t>(w)+
                    static_cast<std::size_t>(xx)]=
                    raw[static_cast<std::size_t>(t.hy0+yy)*64u+
                        static_cast<std::size_t>(t.hx0+xx)];
            }
        }
        return st::StreamStatus::ok();
    }

    st::StreamStatus readRowBias(
        int,int,float*,std::size_t) override {
        return st::StreamStatus::ok();
    }

    st::StreamStatus readColBias(
        int,int,float*,std::size_t) override {
        return st::StreamStatus::ok();
    }

    truthraw::DngMetadata md{};
    std::vector<std::uint16_t> raw{};
};

int main(){
    FakeSource source;

    a::Binding v01Binding{};
    v01Binding.sourceEvidenceSha256[0]=1u;
    v01Binding.truthNegativeStateSha256[0]=2u;

    a::Options options{};
    options.tileEdge=32u;
    options.samplingPeriod=8u;

    a::Result reference{};
    R(a::run(source,v01Binding,options,reference));
    R(reference.sampled>0u);
    R(reference.audit.corrected>0u);
    R(reference.tiles.size()==4u);

    ce::Binding binding{};
    binding.sourceEvidenceSha256=v01Binding.sourceEvidenceSha256;
    binding.scientificMasterSha256[0]=3u;
    binding.authorityFieldSha256[0]=4u;
    binding.truthNegativeStateSha256=
        v01Binding.truthNegativeStateSha256;
    binding.v01CandidateSha256=reference.candidateSha256;
    binding.v01AuditSha256=reference.auditSha256;
    binding.v01SpatialSha256=reference.spatialSha256;

    ce::Result result{};
    R(ce::run(source,binding,reference,result));
    R(result.v01TileParityVerified);
    R(result.centerOnlySigmaPrimary);
    R(result.combinedSigmaDiagnosticOnly);
    R(!result.noiseIndependenceAdmitted);
    R(!result.candidateApplied);
    R(!result.createsNewEvidence);
    R(!result.scientificWritebackAllowed);
    R(result.tiles.size()==reference.tiles.size());
    R(result.metrics.sampled==reference.sampled);
    R(result.metrics.v01CandidateCenters==
      reference.audit.corrected);
    R(result.metrics.predictorValid+
      result.metrics.predictorInvalid==
      result.metrics.v01CandidateCenters);
    R(result.metrics.predictorValid>0u);
    R(result.metrics.centerResidualWithin1Sigma+
      result.metrics.centerResidualBetween1And2Sigma+
      result.metrics.centerResidualAbove2Sigma==
      result.metrics.predictorValid);
    R(result.metrics.combinedResidualWithin1Sigma+
      result.metrics.combinedResidualBetween1And2Sigma+
      result.metrics.combinedResidualAbove2Sigma==
      result.metrics.predictorValid);
    R(result.metrics.combinedResidualAbove2Sigma<=
      result.metrics.centerResidualAbove2Sigma);
    R(result.metrics.centerVarianceSum>0.0);
    R(result.metrics.estimateVarianceSum>0.0);

    ce::Report report{};
    R(ce::encode(
        binding,
        static_cast<std::uint32_t>(source.md.width),
        static_cast<std::uint32_t>(source.md.height),
        result,
        report));
    R(report.tileCount==reference.tiles.size());
    R(!report.candidateApplied);
    R(!report.createsNewEvidence);
    R(!report.scientificWritebackAllowed);
    R(report.json.find(
        "\"schema\":\"D.RAW/TruthNegative/"
        "N2CenterExcludedSpatialAudit/0.2.1\"")!=
      std::string::npos);
    R(report.json.find(
        "\"center_only_sigma_primary\":true")!=
      std::string::npos);
    R(report.json.find(
        "\"combined_sigma_diagnostic_only\":true")!=
      std::string::npos);
    R(report.json.find(
        "\"noise_independence_admitted\":false")!=
      std::string::npos);
    R(std::any_of(
        report.jsonSha256.begin(),
        report.jsonSha256.end(),
        [](auto v){return v!=0u;}));

    auto badBinding=binding;
    badBinding.v01CandidateSha256[0]^=0x55u;
    ce::Result shouldFail{};
    R(!ce::run(source,badBinding,reference,shouldFail));

    std::cout
        <<"TruthNegativeCenterExcludedSpatialAudit/0.2.1 PASS "
        <<"sampled="<<result.metrics.sampled
        <<" candidates="<<result.metrics.v01CandidateCenters
        <<" valid="<<result.metrics.predictorValid
        <<" tiles="<<result.tiles.size()
        <<"\n";
}
