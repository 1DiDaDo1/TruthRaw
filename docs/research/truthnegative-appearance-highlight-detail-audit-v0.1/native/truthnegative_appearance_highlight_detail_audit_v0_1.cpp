#include "truthnegative_appearance_highlight_detail_audit_v0_1.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>

namespace truthraw::truthnegative_appearance_highlight_detail_audit::v0_1 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(
        d.begin(),d.end(),[](std::uint8_t v){return v!=0u;});
}

bool finiteNonNegative(double v) noexcept {
    return std::isfinite(v)&&v>=0.0;
}

std::string hex(const Digest& d) {
    static constexpr char kHex[]="0123456789abcdef";
    std::string out(d.size()*2u,'0');
    for(std::size_t i=0u;i<d.size();++i){
        out[2u*i]=kHex[d[i]>>4u];
        out[2u*i+1u]=kHex[d[i]&0x0fu];
    }
    return out;
}

void hashU32(
    truthraw::sha256_v0_69::Hasher& h,
    std::uint32_t v) noexcept {
    const std::array<std::uint8_t,4u> b{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v>>8u),
        static_cast<std::uint8_t>(v>>16u),
        static_cast<std::uint8_t>(v>>24u)};
    h.update(b);
}

void hashU64(
    truthraw::sha256_v0_69::Hasher& h,
    std::uint64_t v) noexcept {
    std::array<std::uint8_t,8u> b{};
    for(std::size_t i=0u;i<8u;++i){
        b[i]=static_cast<std::uint8_t>(v>>(8u*i));
    }
    h.update(b);
}

void hashF64(
    truthraw::sha256_v0_69::Hasher& h,
    double v) noexcept {
    hashU64(h,std::bit_cast<std::uint64_t>(v));
}

void hashBool(
    truthraw::sha256_v0_69::Hasher& h,
    bool v) noexcept {
    hashU32(h,v?1u:0u);
}

bool sampleValid(const Sample& s,double peak) noexcept {
    return finiteNonNegative(s.sourceLuminanceNits)&&
           finiteNonNegative(s.mappedLuminanceNits)&&
           s.mappedLuminanceNits<=peak;
}

bool mappedAtPeak(const Sample& s,double peak) noexcept {
    return s.mappedLuminanceNits==peak;
}

void addSample(
    TileMetrics& m,
    const Sample& s,
    double referenceWhite,
    double peak) noexcept {
    ++m.sampleCount;
    if(s.sourceLuminanceNits>referenceWhite){
        ++m.sourceAboveReferenceWhite;
    }
    if(mappedAtPeak(s,peak))++m.mappedAtPeak;
    if(s.sourceCensored)++m.sourceCensored;
    if(s.gamutOrDisplayClampApplied)++m.gamutOrDisplayClamp;
}

void addPair(
    TileMetrics& m,
    const Sample& a,
    const Sample& b,
    double referenceWhite,
    double peak) noexcept {
    ++m.adjacentPairs;
    const double sourceDelta=
        std::abs(a.sourceLuminanceNits-b.sourceLuminanceNits);
    const double mappedDelta=
        std::abs(a.mappedLuminanceNits-b.mappedLuminanceNits);
    m.sourceAbsGradientSum+=sourceDelta;
    m.mappedAbsGradientSum+=mappedDelta;

    if(a.sourceLuminanceNits==b.sourceLuminanceNits)return;
    ++m.sourceDistinctAdjacentPairs;

    if(mappedAtPeak(a,peak)&&mappedAtPeak(b,peak)){
        ++m.peakCollapsedDistinctAdjacentPairs;
        m.collapsedSourceAbsGradientSum+=sourceDelta;
        m.maxCollapsedSourceAbsGradient=
            std::max(m.maxCollapsedSourceAbsGradient,sourceDelta);
        if(std::max(
                a.sourceLuminanceNits,
                b.sourceLuminanceNits)>referenceWhite){
            ++m.brightPeakCollapsedDistinctAdjacentPairs;
        }
    }
}

void mergeGlobal(
    Report& out,
    const TileMetrics& m) noexcept {
    out.sampleCount+=m.sampleCount;
    out.sourceAboveReferenceWhite+=m.sourceAboveReferenceWhite;
    out.mappedAtPeak+=m.mappedAtPeak;
    out.sourceCensored+=m.sourceCensored;
    out.gamutOrDisplayClamp+=m.gamutOrDisplayClamp;
}

void hashTile(
    truthraw::sha256_v0_69::Hasher& h,
    const TileMetrics& m) noexcept {
    hashU32(h,m.x);
    hashU32(h,m.y);
    hashU32(h,m.width);
    hashU32(h,m.height);
    hashU64(h,m.sampleCount);
    hashU64(h,m.sourceAboveReferenceWhite);
    hashU64(h,m.mappedAtPeak);
    hashU64(h,m.sourceCensored);
    hashU64(h,m.gamutOrDisplayClamp);
    hashU64(h,m.adjacentPairs);
    hashU64(h,m.sourceDistinctAdjacentPairs);
    hashU64(h,m.peakCollapsedDistinctAdjacentPairs);
    hashU64(h,m.brightPeakCollapsedDistinctAdjacentPairs);
    hashF64(h,m.sourceAbsGradientSum);
    hashF64(h,m.mappedAbsGradientSum);
    hashF64(h,m.collapsedSourceAbsGradientSum);
    hashF64(h,m.maxCollapsedSourceAbsGradient);
}

void writeTile(std::ostringstream& o,const TileMetrics& m){
    o<<"{";
    o<<"\"x\":"<<m.x;
    o<<",\"y\":"<<m.y;
    o<<",\"width\":"<<m.width;
    o<<",\"height\":"<<m.height;
    o<<",\"sample_count\":"<<m.sampleCount;
    o<<",\"source_above_reference_white\":"
     <<m.sourceAboveReferenceWhite;
    o<<",\"mapped_at_peak\":"<<m.mappedAtPeak;
    o<<",\"source_censored\":"<<m.sourceCensored;
    o<<",\"gamut_or_display_clamp\":"<<m.gamutOrDisplayClamp;
    o<<",\"adjacent_pairs\":"<<m.adjacentPairs;
    o<<",\"source_distinct_adjacent_pairs\":"
     <<m.sourceDistinctAdjacentPairs;
    o<<",\"peak_collapsed_distinct_adjacent_pairs\":"
     <<m.peakCollapsedDistinctAdjacentPairs;
    o<<",\"bright_peak_collapsed_distinct_adjacent_pairs\":"
     <<m.brightPeakCollapsedDistinctAdjacentPairs;
    o<<",\"source_abs_gradient_sum\":"
     <<m.sourceAbsGradientSum;
    o<<",\"mapped_abs_gradient_sum\":"
     <<m.mappedAbsGradientSum;
    o<<",\"collapsed_source_abs_gradient_sum\":"
     <<m.collapsedSourceAbsGradientSum;
    o<<",\"max_collapsed_source_abs_gradient\":"
     <<m.maxCollapsedSourceAbsGradient;
    o<<"}";
}

} // namespace

bool run(const Input& input,Report& out) noexcept {
    out={};
    try{
        if(!nonzero(input.binding.sourceEvidenceSha256)||
           !nonzero(input.binding.scientificMasterSha256)||
           !nonzero(input.binding.authorityFieldSha256)||
           !nonzero(input.binding.truthNegativeStateSha256)||
           !nonzero(input.binding.appearanceStateSha256)||
           input.width==0u||input.height==0u||
           input.tileEdge==0u||
           !std::isfinite(input.displayReferenceWhiteNits)||
           !std::isfinite(input.displayPeakNits)||
           input.displayReferenceWhiteNits<=0.0||
           input.displayPeakNits<=0.0||
           input.samples.size()!=
               static_cast<std::size_t>(input.width)*
               static_cast<std::size_t>(input.height)){
            return false;
        }

        for(const auto& s:input.samples){
            if(!sampleValid(s,input.displayPeakNits))return false;
        }

        const std::uint32_t tilesX=
            (input.width+input.tileEdge-1u)/input.tileEdge;
        const std::uint32_t tilesY=
            (input.height+input.tileEdge-1u)/input.tileEdge;
        out.tiles.reserve(
            static_cast<std::size_t>(tilesX)*tilesY);

        for(std::uint32_t ty=0u;ty<tilesY;++ty){
            for(std::uint32_t tx=0u;tx<tilesX;++tx){
                TileMetrics m{};
                m.x=tx*input.tileEdge;
                m.y=ty*input.tileEdge;
                m.width=std::min(
                    input.tileEdge,input.width-m.x);
                m.height=std::min(
                    input.tileEdge,input.height-m.y);

                for(std::uint32_t y=m.y;y<m.y+m.height;++y){
                    for(std::uint32_t x=m.x;x<m.x+m.width;++x){
                        const auto index=
                            static_cast<std::size_t>(y)*input.width+x;
                        addSample(
                            m,input.samples[index],
                            input.displayReferenceWhiteNits,
                            input.displayPeakNits);
                        if(x+1u<m.x+m.width){
                            addPair(
                                m,
                                input.samples[index],
                                input.samples[index+1u],
                                input.displayReferenceWhiteNits,
                                input.displayPeakNits);
                        }
                        if(y+1u<m.y+m.height){
                            addPair(
                                m,
                                input.samples[index],
                                input.samples[
                                    index+input.width],
                                input.displayReferenceWhiteNits,
                                input.displayPeakNits);
                        }
                    }
                }
                mergeGlobal(out,m);
                out.tiles.push_back(m);
            }
        }

        // Global adjacency includes tile boundaries as well. Tile-level pair
        // metrics intentionally exclude cross-tile boundaries.
        for(std::uint32_t y=0u;y<input.height;++y){
            for(std::uint32_t x=0u;x<input.width;++x){
                const auto index=
                    static_cast<std::size_t>(y)*input.width+x;
                auto addGlobalPair=[&](const Sample& a,const Sample& b){
                    ++out.adjacentPairs;
                    const double sourceDelta=
                        std::abs(
                            a.sourceLuminanceNits-
                            b.sourceLuminanceNits);
                    const double mappedDelta=
                        std::abs(
                            a.mappedLuminanceNits-
                            b.mappedLuminanceNits);
                    out.sourceAbsGradientSum+=sourceDelta;
                    out.mappedAbsGradientSum+=mappedDelta;
                    if(a.sourceLuminanceNits==
                       b.sourceLuminanceNits){
                        return;
                    }
                    ++out.sourceDistinctAdjacentPairs;
                    if(mappedAtPeak(a,input.displayPeakNits)&&
                       mappedAtPeak(b,input.displayPeakNits)){
                        ++out.peakCollapsedDistinctAdjacentPairs;
                        out.collapsedSourceAbsGradientSum+=sourceDelta;
                        out.maxCollapsedSourceAbsGradient=
                            std::max(
                                out.maxCollapsedSourceAbsGradient,
                                sourceDelta);
                        if(std::max(
                                a.sourceLuminanceNits,
                                b.sourceLuminanceNits)>
                           input.displayReferenceWhiteNits){
                            ++out.brightPeakCollapsedDistinctAdjacentPairs;
                        }
                    }
                };
                if(x+1u<input.width){
                    addGlobalPair(
                        input.samples[index],
                        input.samples[index+1u]);
                }
                if(y+1u<input.height){
                    addGlobalPair(
                        input.samples[index],
                        input.samples[index+input.width]);
                }
            }
        }

        out.noHighlightHeadroom=
            input.displayPeakNits<=
            input.displayReferenceWhiteNits;
        out.mappedPeakCollapseObserved=
            out.peakCollapsedDistinctAdjacentPairs>0u;
        out.sourceSceneMutated=false;
        out.createsNewEvidence=false;
        out.scientificWritebackAllowed=false;

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[]=
            "D_RAW_TN_APPEARANCE_HIGHLIGHT_DETAIL_AUDIT_V0_1";
        h.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain)-1u);
        h.update(input.binding.sourceEvidenceSha256);
        h.update(input.binding.scientificMasterSha256);
        h.update(input.binding.authorityFieldSha256);
        h.update(input.binding.truthNegativeStateSha256);
        h.update(input.binding.appearanceStateSha256);
        hashU32(h,input.width);
        hashU32(h,input.height);
        hashU32(h,input.tileEdge);
        hashF64(h,input.displayReferenceWhiteNits);
        hashF64(h,input.displayPeakNits);
        hashU64(h,out.sampleCount);
        hashU64(h,out.sourceAboveReferenceWhite);
        hashU64(h,out.mappedAtPeak);
        hashU64(h,out.sourceCensored);
        hashU64(h,out.gamutOrDisplayClamp);
        hashU64(h,out.adjacentPairs);
        hashU64(h,out.sourceDistinctAdjacentPairs);
        hashU64(h,out.peakCollapsedDistinctAdjacentPairs);
        hashU64(h,out.brightPeakCollapsedDistinctAdjacentPairs);
        hashF64(h,out.sourceAbsGradientSum);
        hashF64(h,out.mappedAbsGradientSum);
        hashF64(h,out.collapsedSourceAbsGradientSum);
        hashF64(h,out.maxCollapsedSourceAbsGradient);
        hashBool(h,out.noHighlightHeadroom);
        hashBool(h,out.mappedPeakCollapseObserved);
        for(const auto& tile:out.tiles)hashTile(h,tile);
        out.auditSha256=h.finalize();

        std::ostringstream o;
        o.setf(std::ios::fixed);
        o<<std::setprecision(12);
        o<<"{\n";
        o<<"  \"schema\":\""<<kSchemaName<<"\",\n";
        o<<"  \"width\":"<<input.width<<",\n";
        o<<"  \"height\":"<<input.height<<",\n";
        o<<"  \"tile_edge\":"<<input.tileEdge<<",\n";
        o<<"  \"display_reference_white_nits\":"
         <<input.displayReferenceWhiteNits<<",\n";
        o<<"  \"display_peak_nits\":"
         <<input.displayPeakNits<<",\n";
        o<<"  \"source_sha256\":\""
         <<hex(input.binding.sourceEvidenceSha256)<<"\",\n";
        o<<"  \"scientific_master_sha256\":\""
         <<hex(input.binding.scientificMasterSha256)<<"\",\n";
        o<<"  \"authority_field_sha256\":\""
         <<hex(input.binding.authorityFieldSha256)<<"\",\n";
        o<<"  \"truthnegative_state_sha256\":\""
         <<hex(input.binding.truthNegativeStateSha256)<<"\",\n";
        o<<"  \"appearance_state_sha256\":\""
         <<hex(input.binding.appearanceStateSha256)<<"\",\n";
        o<<"  \"audit_sha256\":\""
         <<hex(out.auditSha256)<<"\",\n";
        o<<"  \"no_highlight_headroom\":"
         <<(out.noHighlightHeadroom?"true":"false")<<",\n";
        o<<"  \"mapped_peak_collapse_observed\":"
         <<(out.mappedPeakCollapseObserved?"true":"false")<<",\n";
        o<<"  \"source_scene_mutated\":false,\n";
        o<<"  \"creates_new_evidence\":false,\n";
        o<<"  \"scientific_writeback_allowed\":false,\n";
        o<<"  \"global\":{";
        o<<"\"sample_count\":"<<out.sampleCount;
        o<<",\"source_above_reference_white\":"
         <<out.sourceAboveReferenceWhite;
        o<<",\"mapped_at_peak\":"<<out.mappedAtPeak;
        o<<",\"source_censored\":"<<out.sourceCensored;
        o<<",\"gamut_or_display_clamp\":"
         <<out.gamutOrDisplayClamp;
        o<<",\"adjacent_pairs\":"<<out.adjacentPairs;
        o<<",\"source_distinct_adjacent_pairs\":"
         <<out.sourceDistinctAdjacentPairs;
        o<<",\"peak_collapsed_distinct_adjacent_pairs\":"
         <<out.peakCollapsedDistinctAdjacentPairs;
        o<<",\"bright_peak_collapsed_distinct_adjacent_pairs\":"
         <<out.brightPeakCollapsedDistinctAdjacentPairs;
        o<<",\"source_abs_gradient_sum\":"
         <<out.sourceAbsGradientSum;
        o<<",\"mapped_abs_gradient_sum\":"
         <<out.mappedAbsGradientSum;
        o<<",\"collapsed_source_abs_gradient_sum\":"
         <<out.collapsedSourceAbsGradientSum;
        o<<",\"max_collapsed_source_abs_gradient\":"
         <<out.maxCollapsedSourceAbsGradient;
        o<<"},\n";
        o<<"  \"tiles\":[\n";
        for(std::size_t i=0u;i<out.tiles.size();++i){
            o<<"    ";
            writeTile(o,out.tiles[i]);
            if(i+1u<out.tiles.size())o<<",";
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

        return nonzero(out.auditSha256)&&
               nonzero(out.jsonSha256);
    }catch(...){
        out={};
        return false;
    }
}

} // namespace truthraw::truthnegative_appearance_highlight_detail_audit::v0_1
