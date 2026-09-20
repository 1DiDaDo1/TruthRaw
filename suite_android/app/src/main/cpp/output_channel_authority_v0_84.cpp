#include "output_channel_authority_v0_84.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace truthraw::output_channel_authority::v0_84 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(d.begin(), d.end(), [](std::uint8_t v){ return v != 0u; });
}

void put_u32(truthraw::sha256_v0_69::Hasher& h, std::uint32_t v) noexcept {
    const std::array<std::uint8_t,4> b{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v >> 8u),
        static_cast<std::uint8_t>(v >> 16u),
        static_cast<std::uint8_t>(v >> 24u),
    };
    h.update(b);
}

void put_u64(truthraw::sha256_v0_69::Hasher& h, std::uint64_t v) noexcept {
    std::array<std::uint8_t,8> b{};
    for (std::size_t i=0;i<b.size();++i) {
        b[i]=static_cast<std::uint8_t>(v >> (8u*i));
    }
    h.update(b);
}

bool valid_binding(
    const truthraw::DngMetadata& md,
    const Binding& b) noexcept {
    return nonzero(b.sourceEvidenceSha256) &&
           nonzero(b.scientificMasterSha256) &&
           nonzero(b.canonicalOpenSceneSha256) &&
           nonzero(b.sourceChannelAuthoritySha256) &&
           nonzero(b.uncertaintyDecisionSha256) &&
           b.sourceWidth == static_cast<std::uint32_t>(md.width) &&
           b.sourceHeight == static_cast<std::uint32_t>(md.height) &&
           b.outputWidth > 0u && b.outputHeight > 0u &&
           b.physicalFrameCount == 1u &&
           b.independentEvidenceCount == 1u &&
           !b.reconstructionBackendId.empty();
}

Digest policy_hash(const Binding& b, MappingMode mode) noexcept {
    truthraw::sha256_v0_69::Hasher h;
    constexpr char policy[] =
        "schema=TruthRawOutputChannelAuthority/0.84\n"
        "domain=OUTPUT_RGB_CHANNELS\n"
        "direct_cfa_authority_is_not_copied_to_dense_rgb=1\n"
        "dense_color_output_requires_reconstruction_authority=1\n"
        "censored_support_cannot_become_exact=1\n"
        "resampling_without_explicit_bound_propagation_is_unknown=1\n"
        "orientation_is_coordinate_transform_only=1\n"
        "appearance_cannot_upgrade_authority=1\n"
        "scientific_writeback_allowed=0\n"
        "creates_new_evidence=0\n";
    h.update(reinterpret_cast<const std::uint8_t*>(policy), sizeof(policy)-1u);
    h.update(b.sourceEvidenceSha256);
    h.update(b.scientificMasterSha256);
    h.update(b.canonicalOpenSceneSha256);
    h.update(b.sourceChannelAuthoritySha256);
    h.update(b.uncertaintyDecisionSha256);
    put_u32(h,b.sourceWidth); put_u32(h,b.sourceHeight);
    put_u32(h,b.outputWidth); put_u32(h,b.outputHeight);
    put_u32(h,b.reconstructionSupportRadius);
    put_u32(h,static_cast<std::uint32_t>(mode));
    put_u32(h,b.reconstructedUncertaintyAdmitted?1u:0u);
    h.update(
        reinterpret_cast<const std::uint8_t*>(b.reconstructionBackendId.data()),
        b.reconstructionBackendId.size());
    return h.finalize();
}

Digest artifact_hash(
    const Digest& content,
    const Digest& policy,
    const Binding& b) noexcept {
    truthraw::sha256_v0_69::Hasher h;
    constexpr char domain[]="TRUTHRAW_OUTPUT_CHANNEL_AUTHORITY_ARTIFACT_V0_84";
    h.update(reinterpret_cast<const std::uint8_t*>(domain),sizeof(domain)-1u);
    h.update(b.canonicalOpenSceneSha256);
    h.update(b.sourceChannelAuthoritySha256);
    h.update(content);
    h.update(policy);
    return h.finalize();
}

void append_record(
    truthraw::sha256_v0_69::Hasher& h,
    std::uint64_t index,
    Authority authority) noexcept {
    put_u64(h,index);
    const std::array<std::uint8_t,4> record{
        static_cast<std::uint8_t>(authority),
        0u,0u,0u,
    };
    h.update(record);
}

} // namespace

bool build_conservative(
    truthraw::streaming_v0_1::IRawTileSource& source,
    const Binding& binding,
    Summary& out) noexcept {
    out={};
    try {
        const auto& md=source.metadata();
        if(!valid_binding(md,binding)) return false;

        const bool sameResolution =
            binding.sourceWidth==binding.outputWidth &&
            binding.sourceHeight==binding.outputHeight;
        out.mappingMode=sameResolution
            ? MappingMode::FullResolutionConservative
            : MappingMode::ResampledFailClosedUnknown;
        out.outputPixelCount=
            static_cast<std::uint64_t>(binding.outputWidth)*binding.outputHeight;
        out.recordCount=out.outputPixelCount*3u;
        out.reconstructedUncertaintyAdmitted=binding.reconstructedUncertaintyAdmitted;
        out.orientationTransformChangesAuthority=false;
        out.createsNewEvidence=false;
        out.scientificWritebackAllowed=false;

        truthraw::sha256_v0_69::Hasher content;
        constexpr char domain[]="TruthRawOutputChannelAuthorityContent/0.84";
        content.update(reinterpret_cast<const std::uint8_t*>(domain),sizeof(domain)-1u);
        put_u32(content,binding.outputWidth);
        put_u32(content,binding.outputHeight);

        if(!sameResolution) {
            for(std::uint64_t pixel=0;pixel<out.outputPixelCount;++pixel) {
                for(std::uint32_t ch=0;ch<3u;++ch) {
                    const std::uint64_t recordIndex=pixel*3u+ch;
                    append_record(content,recordIndex,Authority::Unknown);
                    ++out.authorityCounts[3];
                }
            }
        } else {
            constexpr int edge=64;
            const int radius=static_cast<int>(binding.reconstructionSupportRadius);
            std::vector<std::uint16_t> raw;
            std::vector<float> gain;
            for(int y0=0;y0<md.height;y0+=edge) {
                const int y1=std::min(md.height,y0+edge);
                for(int x0=0;x0<md.width;x0+=edge) {
                    const int x1=std::min(md.width,x0+edge);
                    const int rx0=std::max(0,x0-radius);
                    const int ry0=std::max(0,y0-radius);
                    const int rx1=std::min(md.width,x1+radius);
                    const int ry1=std::min(md.height,y1+radius);
                    const int rw=rx1-rx0;
                    const int rh=ry1-ry0;
                    const std::size_t n=static_cast<std::size_t>(rw)*rh;
                    raw.resize(n);
                    if(md.hasGainField) gain.resize(n); else gain.clear();
                    truthraw::TileRect rect{rx0,ry0,rx1,ry1,rx0,ry0,rx1,ry1};
                    const auto s=source.readRawTile(
                        rect,
                        raw.data(),raw.size(),
                        md.hasGainField?gain.data():nullptr,
                        md.hasGainField?gain.size():0u);
                    if(!s) return false;

                    for(int y=y0;y<y1;++y) {
                        for(int x=x0;x<x1;++x) {
                            bool censored=false;
                            for(int yy=std::max(0,y-radius);
                                yy<=std::min(md.height-1,y+radius) && !censored;
                                ++yy) {
                                for(int xx=std::max(0,x-radius);
                                    xx<=std::min(md.width-1,x+radius);
                                    ++xx) {
                                    const std::size_t i=
                                        static_cast<std::size_t>(yy-ry0)*rw+
                                        static_cast<std::size_t>(xx-rx0);
                                    if(static_cast<float>(raw[i])>=md.whiteLevel) {
                                        censored=true;
                                        break;
                                    }
                                }
                            }

                            const Authority a = censored
                                ? Authority::Censored
                                : (binding.reconstructedUncertaintyAdmitted
                                    ? Authority::Reconstructed
                                    : Authority::Unknown);
                            if(censored) ++out.censoredSupportPixels;
                            const std::uint64_t pixel=
                                static_cast<std::uint64_t>(y)*binding.outputWidth+
                                static_cast<std::uint32_t>(x);
                            for(std::uint32_t ch=0;ch<3u;++ch) {
                                append_record(content,pixel*3u+ch,a);
                                ++out.authorityCounts[
                                    static_cast<std::size_t>(
                                        static_cast<std::uint8_t>(a)-1u)];
                            }
                        }
                    }
                }
            }
        }

        const std::uint64_t counted =
            out.authorityCounts[0]+out.authorityCounts[1]+
            out.authorityCounts[2]+out.authorityCounts[3];
        if(counted!=out.recordCount) return false;

        out.contentSha256=content.finalize();
        out.policySha256=policy_hash(binding,out.mappingMode);
        out.artifactSha256=artifact_hash(out.contentSha256,out.policySha256,binding);
        out.perOutputChannelAuthorityAvailable=
            nonzero(out.contentSha256)&&nonzero(out.policySha256)&&nonzero(out.artifactSha256);
        return out.perOutputChannelAuthorityAvailable;
    } catch(...) {
        out={};
        return false;
    }
}

const char* schema_name() noexcept {
    return "TruthRawOutputChannelAuthority/0.84";
}

const char* mapping_mode_name(MappingMode mode) noexcept {
    switch(mode) {
        case MappingMode::FullResolutionConservative:
            return "FULL_RESOLUTION_CONSERVATIVE";
        case MappingMode::ResampledFailClosedUnknown:
            return "RESAMPLED_FAIL_CLOSED_UNKNOWN";
    }
    return "INVALID";
}

} // namespace truthraw::output_channel_authority::v0_84
