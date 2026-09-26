#include "truthnegative_appearance_highlight_headroom_sweep_v0_2.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>

namespace truthraw::truthnegative_appearance_highlight_headroom_sweep::v0_2 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(d.begin(),d.end(),[](std::uint8_t v){return v!=0u;});
}
bool finiteNonNegative(double v) noexcept {
    return std::isfinite(v)&&v>=0.0;
}
void hashU64(truthraw::sha256_v0_69::Hasher& h,std::uint64_t v) noexcept {
    std::array<std::uint8_t,8u> b{};
    for(std::size_t i=0u;i<8u;++i)b[i]=static_cast<std::uint8_t>(v>>(8u*i));
    h.update(b);
}
void hashU32(truthraw::sha256_v0_69::Hasher& h,std::uint32_t v) noexcept {
    std::array<std::uint8_t,4u> b{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v>>8u),
        static_cast<std::uint8_t>(v>>16u),
        static_cast<std::uint8_t>(v>>24u)};
    h.update(b);
}
void hashF64(truthraw::sha256_v0_69::Hasher& h,double v) noexcept {
    hashU64(h,std::bit_cast<std::uint64_t>(v));
}
void hashString(truthraw::sha256_v0_69::Hasher& h,const std::string& s) noexcept {
    hashU64(h,static_cast<std::uint64_t>(s.size()));
    h.update(reinterpret_cast<const std::uint8_t*>(s.data()),s.size());
}
std::string hex(const Digest& d){
    static constexpr char kHex[]="0123456789abcdef";
    std::string out(d.size()*2u,'0');
    for(std::size_t i=0u;i<d.size();++i){
        out[2u*i]=kHex[d[i]>>4u];
        out[2u*i+1u]=kHex[d[i]&0x0fu];
    }
    return out;
}
double fraction(std::uint64_t n,std::uint64_t d) noexcept {
    return d>0u?static_cast<double>(n)/static_cast<double>(d):0.0;
}
bool sameDouble(double a,double b) noexcept {
    return std::bit_cast<std::uint64_t>(a)==std::bit_cast<std::uint64_t>(b);
}

} // namespace

bool run(const Input& input,Report& out) noexcept {
    out={};
    try{
        if(!nonzero(input.binding.sourceEvidenceSha256)||
           !nonzero(input.binding.scientificMasterSha256)||
           !nonzero(input.binding.authorityFieldSha256)||
           !nonzero(input.binding.truthNegativeStateSha256)||
           input.width==0u||input.height==0u||
           input.variants.size()<2u){
            return false;
        }

        const auto& baseline=input.variants.front();
        if(baseline.id!="baseline_100_100"||
           baseline.referenceWhiteNits!=100.0||
           baseline.peakNits!=100.0){
            return false;
        }
        out.baselineIsFirst=true;

        const auto baseSamples=baseline.detailAudit.sampleCount;
        const auto basePairs=baseline.detailAudit.sourceDistinctAdjacentPairs;
        const auto baseCensored=baseline.detailAudit.sourceCensored;
        const auto baseSourceGradient=baseline.detailAudit.sourceAbsGradientSum;
        if(baseSamples!=static_cast<std::uint64_t>(input.width)*input.height||
           basePairs==0u||
           !finiteNonNegative(baseSourceGradient)){
            return false;
        }

        out.sourceIdentityConsistent=true;
        out.sourceGradientConsistent=true;
        out.sourceCensorCountConsistent=true;
        out.lowerRangePreservationAudited=true;

        const double baselineCollapse=
            fraction(
                baseline.detailAudit.peakCollapsedDistinctAdjacentPairs,
                basePairs);

        double previousReferenceWhite=101.0;
        for(const auto& v:input.variants){
            if(v.id.empty()||
               !finiteNonNegative(v.referenceWhiteNits)||
               !finiteNonNegative(v.peakNits)||
               v.referenceWhiteNits<=0.0||
               v.peakNits<=0.0||
               v.referenceWhiteNits>v.peakNits||
               v.peakNits!=100.0||
               v.referenceWhiteNits>=previousReferenceWhite||
               !nonzero(v.appearanceStateSha256)||
               !nonzero(v.detailAudit.auditSha256)||
               v.detailAudit.sampleCount!=baseSamples||
               v.detailAudit.sourceDistinctAdjacentPairs!=basePairs||
               v.detailAudit.sourceCensored!=baseCensored||
               !sameDouble(
                    v.detailAudit.sourceAbsGradientSum,
                    baseSourceGradient)||
               v.belowKneeMappedLuminanceChanged>
                    v.belowKneeSampleCount||
               v.detailAudit.sourceSceneMutated||
               v.detailAudit.createsNewEvidence||
               v.detailAudit.scientificWritebackAllowed){
                return false;
            }
            previousReferenceWhite=v.referenceWhiteNits;

            VariantResult r{};
            r.id=v.id;
            r.referenceWhiteNits=v.referenceWhiteNits;
            r.peakNits=v.peakNits;
            r.sampleCount=v.detailAudit.sampleCount;
            r.mappedAtPeak=v.detailAudit.mappedAtPeak;
            r.sourceCensored=v.detailAudit.sourceCensored;
            r.gamutOrDisplayClamp=v.detailAudit.gamutOrDisplayClamp;
            r.sourceDistinctAdjacentPairs=
                v.detailAudit.sourceDistinctAdjacentPairs;
            r.peakCollapsedDistinctAdjacentPairs=
                v.detailAudit.peakCollapsedDistinctAdjacentPairs;
            r.brightPeakCollapsedDistinctAdjacentPairs=
                v.detailAudit.brightPeakCollapsedDistinctAdjacentPairs;
            r.belowKneeSampleCount=v.belowKneeSampleCount;
            r.belowKneeMappedLuminanceChanged=
                v.belowKneeMappedLuminanceChanged;
            r.sourceGradientSum=v.detailAudit.sourceAbsGradientSum;
            r.mappedGradientSum=v.detailAudit.mappedAbsGradientSum;
            r.mappedGradientRetention=
                r.sourceGradientSum>0.0
                    ? r.mappedGradientSum/r.sourceGradientSum
                    : 1.0;
            r.collapseFractionOfDistinctPairs=
                fraction(
                    r.peakCollapsedDistinctAdjacentPairs,
                    r.sourceDistinctAdjacentPairs);
            r.collapseReductionVsBaseline=
                baselineCollapse>0.0
                    ? 1.0-r.collapseFractionOfDistinctPairs/baselineCollapse
                    : 0.0;
            r.noHighlightHeadroom=v.detailAudit.noHighlightHeadroom;
            r.mappedPeakCollapseObserved=
                v.detailAudit.mappedPeakCollapseObserved;
            r.appearanceStateSha256=v.appearanceStateSha256;
            r.detailAuditSha256=v.detailAudit.auditSha256;

            if(!finiteNonNegative(r.mappedGradientRetention)||
               !finiteNonNegative(r.collapseFractionOfDistinctPairs)||
               !std::isfinite(r.collapseReductionVsBaseline)){
                return false;
            }
            out.variants.push_back(r);
        }

        out.automaticWinnerSelected=false;
        out.sourceSceneMutated=false;
        out.createsNewEvidence=false;
        out.scientificWritebackAllowed=false;

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[]=
            "D_RAW_TN_APPEARANCE_HIGHLIGHT_HEADROOM_SWEEP_V0_2";
        h.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain)-1u);
        h.update(input.binding.sourceEvidenceSha256);
        h.update(input.binding.scientificMasterSha256);
        h.update(input.binding.authorityFieldSha256);
        h.update(input.binding.truthNegativeStateSha256);
        hashU32(h,input.width);
        hashU32(h,input.height);
        hashU64(h,out.variants.size());
        for(const auto& r:out.variants){
            hashString(h,r.id);
            hashF64(h,r.referenceWhiteNits);
            hashF64(h,r.peakNits);
            hashU64(h,r.sampleCount);
            hashU64(h,r.mappedAtPeak);
            hashU64(h,r.sourceCensored);
            hashU64(h,r.gamutOrDisplayClamp);
            hashU64(h,r.sourceDistinctAdjacentPairs);
            hashU64(h,r.peakCollapsedDistinctAdjacentPairs);
            hashU64(h,r.brightPeakCollapsedDistinctAdjacentPairs);
            hashU64(h,r.belowKneeSampleCount);
            hashU64(h,r.belowKneeMappedLuminanceChanged);
            hashF64(h,r.sourceGradientSum);
            hashF64(h,r.mappedGradientSum);
            hashF64(h,r.mappedGradientRetention);
            hashF64(h,r.collapseFractionOfDistinctPairs);
            hashF64(h,r.collapseReductionVsBaseline);
            h.update(r.appearanceStateSha256);
            h.update(r.detailAuditSha256);
        }
        out.sweepSha256=h.finalize();

        std::ostringstream o;
        o.setf(std::ios::fixed);
        o<<std::setprecision(12);
        o<<"{\n";
        o<<"  \"schema\":\""<<kSchemaName<<"\",\n";
        o<<"  \"width\":"<<input.width<<",\n";
        o<<"  \"height\":"<<input.height<<",\n";
        o<<"  \"source_sha256\":\""
         <<hex(input.binding.sourceEvidenceSha256)<<"\",\n";
        o<<"  \"scientific_master_sha256\":\""
         <<hex(input.binding.scientificMasterSha256)<<"\",\n";
        o<<"  \"authority_field_sha256\":\""
         <<hex(input.binding.authorityFieldSha256)<<"\",\n";
        o<<"  \"truthnegative_state_sha256\":\""
         <<hex(input.binding.truthNegativeStateSha256)<<"\",\n";
        o<<"  \"sweep_sha256\":\""<<hex(out.sweepSha256)<<"\",\n";
        o<<"  \"source_identity_consistent\":true,\n";
        o<<"  \"source_gradient_consistent\":true,\n";
        o<<"  \"source_censor_count_consistent\":true,\n";
        o<<"  \"baseline_is_first\":true,\n";
        o<<"  \"lower_range_preservation_audited\":true,\n";
        o<<"  \"automatic_winner_selected\":false,\n";
        o<<"  \"source_scene_mutated\":false,\n";
        o<<"  \"creates_new_evidence\":false,\n";
        o<<"  \"scientific_writeback_allowed\":false,\n";
        o<<"  \"variants\":[\n";
        for(std::size_t i=0u;i<out.variants.size();++i){
            const auto& r=out.variants[i];
            o<<"    {";
            o<<"\"id\":\""<<r.id<<"\"";
            o<<",\"reference_white_nits\":"<<r.referenceWhiteNits;
            o<<",\"peak_nits\":"<<r.peakNits;
            o<<",\"sample_count\":"<<r.sampleCount;
            o<<",\"mapped_at_peak\":"<<r.mappedAtPeak;
            o<<",\"source_censored\":"<<r.sourceCensored;
            o<<",\"gamut_or_display_clamp\":"<<r.gamutOrDisplayClamp;
            o<<",\"source_distinct_adjacent_pairs\":"
             <<r.sourceDistinctAdjacentPairs;
            o<<",\"peak_collapsed_distinct_adjacent_pairs\":"
             <<r.peakCollapsedDistinctAdjacentPairs;
            o<<",\"bright_peak_collapsed_distinct_adjacent_pairs\":"
             <<r.brightPeakCollapsedDistinctAdjacentPairs;
            o<<",\"below_knee_sample_count\":"
             <<r.belowKneeSampleCount;
            o<<",\"below_knee_mapped_luminance_changed\":"
             <<r.belowKneeMappedLuminanceChanged;
            o<<",\"source_gradient_sum\":"<<r.sourceGradientSum;
            o<<",\"mapped_gradient_sum\":"<<r.mappedGradientSum;
            o<<",\"mapped_gradient_retention\":"
             <<r.mappedGradientRetention;
            o<<",\"collapse_fraction_of_distinct_pairs\":"
             <<r.collapseFractionOfDistinctPairs;
            o<<",\"collapse_reduction_vs_baseline\":"
             <<r.collapseReductionVsBaseline;
            o<<",\"no_highlight_headroom\":"
             <<(r.noHighlightHeadroom?"true":"false");
            o<<",\"mapped_peak_collapse_observed\":"
             <<(r.mappedPeakCollapseObserved?"true":"false");
            o<<",\"appearance_state_sha256\":\""
             <<hex(r.appearanceStateSha256)<<"\"";
            o<<",\"detail_audit_sha256\":\""
             <<hex(r.detailAuditSha256)<<"\"";
            o<<"}";
            if(i+1u<out.variants.size())o<<",";
            o<<"\n";
        }
        o<<"  ]\n";
        o<<"}\n";
        out.json=o.str();

        truthraw::sha256_v0_69::Hasher jh;
        jh.update(
            reinterpret_cast<const std::uint8_t*>(out.json.data()),
            out.json.size());
        out.jsonSha256=jh.finalize();

        return nonzero(out.sweepSha256)&&nonzero(out.jsonSha256);
    }catch(...){
        out={};
        return false;
    }
}

} // namespace truthraw::truthnegative_appearance_highlight_headroom_sweep::v0_2
