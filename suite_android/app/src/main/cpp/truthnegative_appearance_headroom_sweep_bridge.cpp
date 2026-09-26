#include <jni.h>

#include "free_world_appearance_resolve_v0_7.h"
#include "free_world_scientific_open_scene_binding_v0_3.h"
#include "truthnegative_appearance_highlight_detail_audit_v0_1.h"
#include "truthnegative_appearance_highlight_headroom_sweep_v0_2.h"
#include "truthnegative_deep_scene_bridge_v0_8.h"
#include "truthnegative_pipeline_bridge_common.h"
#include "truthraw_sha256_v0_69.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {

namespace pipeline =
    truthraw::android_truthnegative_pipeline::v0_1;
namespace appearance =
    truthraw::free_world_appearance_resolve::v0_7;
namespace binding =
    truthraw::free_world_scientific_open_scene_binding::v0_3;
namespace deep =
    truthraw::free_world_deep_scene_contribution::v0_4;
namespace free_world =
    truthraw::free_world_pixel_resolve_2d::v0_2;
namespace tn =
    truthraw::truthnegative_continuous::v0_5;
namespace tn_deep =
    truthraw::truthnegative_deep_scene_bridge::v0_8;
namespace detail =
    truthraw::truthnegative_appearance_highlight_detail_audit::v0_1;
namespace sweep =
    truthraw::truthnegative_appearance_highlight_headroom_sweep::v0_2;
namespace sha = truthraw::sha256_v0_69;

struct VariantRuntime final {
    const char* id = "";
    double referenceWhiteNits = 100.0;
    appearance::DisplayTarget display{};
    std::vector<detail::Sample> samples{};
    sha::Digest appearanceState{};
    bool appearanceStateKnown = false;
    std::uint64_t belowKneeSampleCount = 0u;
    std::uint64_t belowKneeMappedLuminanceChanged = 0u;
};

jstring status(JNIEnv* env,int code,const std::string& message) {
    std::ostringstream o;
    o<<"{\"status\":"<<code<<",\"message\":\"";
    for(char c:message){
        if(c=='"'||c=='\\')o<<'\\';
        if(c=='\n'||c=='\r')o<<' ';
        else o<<c;
    }
    o<<"\"}";
    return env->NewStringUTF(o.str().c_str());
}

bool writeAll(int fd,const std::string& data) noexcept {
    if(fd<0)return false;
    if(::ftruncate(fd,0)!=0)return false;
    std::size_t done=0u;
    while(done<data.size()){
        const ssize_t n=::pwrite(
            fd,
            data.data()+done,
            data.size()-done,
            static_cast<off_t>(done));
        if(n<=0)return false;
        done+=static_cast<std::size_t>(n);
    }
    return ::fsync(fd)==0;
}

bool readbackSha(
    int fd,
    std::size_t expectedBytes,
    sha::Digest& digest) noexcept {
    if(fd<0)return false;
    struct stat st{};
    if(::fstat(fd,&st)!=0||st.st_size<0||
       static_cast<std::uint64_t>(st.st_size)!=expectedBytes){
        return false;
    }
    sha::Hasher h;
    std::vector<std::uint8_t> buffer(64u*1024u);
    std::size_t offset=0u;
    while(offset<expectedBytes){
        const std::size_t want=
            std::min(buffer.size(),expectedBytes-offset);
        const ssize_t n=::pread(
            fd,buffer.data(),want,static_cast<off_t>(offset));
        if(n<=0)return false;
        h.update(buffer.data(),static_cast<std::size_t>(n));
        offset+=static_cast<std::size_t>(n);
    }
    digest=h.finalize();
    return true;
}

sha::Digest labeledDigest(
    const char* label,
    const sha::Digest* parent=nullptr,
    const std::string* text=nullptr) noexcept {
    sha::Hasher h;
    const std::size_t n=std::char_traits<char>::length(label);
    h.update(reinterpret_cast<const std::uint8_t*>(label),n);
    if(parent!=nullptr)h.update(*parent);
    if(text!=nullptr){
        h.update(
            reinterpret_cast<const std::uint8_t*>(text->data()),
            text->size());
    }
    return h.finalize();
}

appearance::Matrix3 matrixFromF32(
    const std::array<float,9u>& source) noexcept {
    appearance::Matrix3 out{};
    for(std::size_t i=0u;i<source.size();++i){
        out.m[i]=static_cast<double>(source[i]);
    }
    return out;
}

appearance::Matrix3 xyzD50ToLinearSrgb() noexcept {
    appearance::Matrix3 out{};
    out.m={
         3.1338561,-1.6168667,-0.4906146,
        -0.9787684, 1.9161415, 0.0334540,
         0.0719453,-0.2289914, 1.4052427,
    };
    return out;
}

bool targetGeometry(
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight,
    std::uint32_t maxEdge,
    std::uint32_t& targetWidth,
    std::uint32_t& targetHeight) noexcept {
    if(sourceWidth==0u||sourceHeight==0u||maxEdge==0u)return false;
    if(sourceWidth>=sourceHeight){
        targetWidth=maxEdge;
        targetHeight=std::max<std::uint32_t>(
            1u,
            static_cast<std::uint32_t>(std::llround(
                static_cast<double>(maxEdge)*
                static_cast<double>(sourceHeight)/
                static_cast<double>(sourceWidth))));
    }else{
        targetHeight=maxEdge;
        targetWidth=std::max<std::uint32_t>(
            1u,
            static_cast<std::uint32_t>(std::llround(
                static_cast<double>(maxEdge)*
                static_cast<double>(sourceWidth)/
                static_cast<double>(sourceHeight))));
    }
    return targetWidth>0u&&targetHeight>0u;
}

} // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_truthraw_adaptiveui_TruthNegativeAppearanceHeadroomSweepBridge_exportAndVerify(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint destinationFd,
    jint requestedMaxEdge,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if(sourceFd<0||destinationFd<0||
       requestedMaxEdge<32||requestedMaxEdge>256||
       maxSourceResidentBytes<=0||maxLogicalResidentBytes<=0){
        return status(env,-120,"invalid appearance-headroom sweep arguments");
    }

    pipeline::Context ctx{};
    const auto prepared=pipeline::prepare(
        sourceFd,
        static_cast<std::size_t>(maxSourceResidentBytes),
        static_cast<std::size_t>(maxLogicalResidentBytes),
        ctx);
    if(!prepared)return status(env,prepared.code,prepared.message);

    binding::BoundScenePlane scene(
        *ctx.masterSource,*ctx.fieldSource,ctx.width,ctx.height);
    if(!scene.valid())return status(env,-121,"scientific/open-scene binding invalid");

    std::uint32_t targetWidth=0u,targetHeight=0u;
    if(!targetGeometry(
            ctx.width,ctx.height,
            static_cast<std::uint32_t>(requestedMaxEdge),
            targetWidth,targetHeight)){
        return status(env,-122,"headroom sweep target geometry failed");
    }

    tn::RasterResolver resolver(
        scene,ctx.truthNegativeState,targetWidth,targetHeight);
    if(!resolver.valid())return status(env,-123,"TruthNegative raster resolver invalid");

    appearance::SceneColorimetry sceneColor{};
    sceneColor.rgbToXyz=matrixFromF32(ctx.prepared.color.cameraToXyzD50);
    sceneColor.referenceWhiteXyz={0.96422,1.0,0.82521};
    sceneColor.sceneReferenceWhiteNits=100.0;
    sceneColor.identitySha256=labeledDigest(
        "D_RAW_TN_CONT_V05_SCENE_COLORIMETRY",
        &ctx.truthNegativeState.stateSha256,
        &ctx.prepared.color.bindingId);

    appearance::ViewingConditions viewing{};
    viewing.adaptingWhiteXyz={0.95047,1.0,1.08883};
    viewing.adaptingLuminanceNits=20.0;
    viewing.backgroundLuminanceNits=20.0;
    viewing.surround=appearance::Surround::Average;
    viewing.viewingDistanceMeters=0.5;
    viewing.identitySha256=labeledDigest(
        "D_RAW_TN_CONT_V05_PRO_VIEW_AVERAGE_20_NIT");

    appearance::AppearancePolicy policy{};
    policy.exposureEv=0.0;
    policy.colorfulnessScale=1.0;
    policy.highlightCompression=1.0;
    policy.identitySha256=labeledDigest(
        "D_RAW_TN_CONT_V05_NEUTRAL_APPEARANCE_POLICY");

    std::array<VariantRuntime,4u> variants{};
    const std::array<const char*,4u> ids{
        "baseline_100_100",
        "shoulder_90_100",
        "shoulder_80_100",
        "shoulder_70_100"};
    const std::array<double,4u> refs{100.0,90.0,80.0,70.0};

    const auto targetSamples=
        static_cast<std::size_t>(targetWidth)*targetHeight;
    for(std::size_t i=0u;i<variants.size();++i){
        auto& v=variants[i];
        v.id=ids[i];
        v.referenceWhiteNits=refs[i];
        v.display.xyzToRgb=xyzD50ToLinearSrgb();
        v.display.whitePointXyz={0.95047,1.0,1.08883};
        v.display.referenceWhiteNits=refs[i];
        v.display.peakLuminanceNits=100.0;
        v.display.blackLuminanceNits=0.0;
        v.display.transfer=appearance::TransferFunction::Srgb;
        std::string label=
            std::string("D_RAW_AH_SWEEP_")+v.id+"_SRGB_100_NIT";
        v.display.identitySha256=labeledDigest(label.c_str());
        v.samples.reserve(targetSamples);
    }

    for(std::uint32_t y=0u;y<targetHeight;++y){
        for(std::uint32_t x=0u;x<targetWidth;++x){
            tn::QueryResult query{};
            if(!resolver.resolvePixel(x,y,query)||
               query.stateSha256!=ctx.truthNegativeState.stateSha256||
               query.stateIdentityChangedByTargetRaster||
               query.createsNewEvidence||
               query.scientificWritebackAllowed){
                return status(env,-124,"TruthNegative headroom sweep query failed");
            }

            tn_deep::CameraPlaneObjectInput cameraPlane{};
            cameraPlane.provenanceId=
                1u+static_cast<std::uint64_t>(y)*targetWidth+x;
            cameraPlane.regionId=1u;
            cameraPlane.objectId=1u;
            cameraPlane.depth=0.0;
            cameraPlane.geometryAuthority=
                truthraw::free_world_deep_scene_binding::v0_5::
                    GeometryAuthority::ImagePlaneBound;
            cameraPlane.parentAncestrySha256=query.querySha256;

            tn_deep::ScenePacket scenePacket{};
            if(!tn_deep::buildCameraPlaneObject(
                    ctx.truthNegativeState,query,cameraPlane,scenePacket)||
               !scenePacket.radiometryBoundToTruthNegative||
               !scenePacket.geometryAuthoritySeparate||
               scenePacket.createsNewEvidence||
               scenePacket.scientificWritebackAllowed){
                return status(env,-125,"Deep Scene headroom sweep bridge failed");
            }

            deep::DeepResolvedPixel scientificView{};
            if(!deep::resolve(
                    scenePacket.deepPacket,
                    deep::ResolveView::ScientificView,
                    scientificView)||
               scientificView.createsNewEvidence||
               scientificView.scientificWritebackAllowed||
               scientificView.physicalFrameCount!=1u||
               scientificView.independentEvidenceCount!=1u){
                return status(env,-126,"Scientific View headroom sweep resolve failed");
            }

            bool sourceCensored=false;
            for(const auto& support:query.pixel.support){
                sourceCensored=
                    sourceCensored||
                    support.authority==
                        free_world::ResolvedAuthority::Censored;
            }

            double commonSourceLuminance=-1.0;
            for(auto& v:variants){
                appearance::AppearanceInput app{};
                app.scene=scientificView;
                app.sceneColorimetry=sceneColor;
                app.viewing=viewing;
                app.display=v.display;
                app.policy=policy;

                appearance::AppearanceResolvedPixel visible{};
                if(!appearance::resolveAppearance(app,visible)||
                   visible.sourceSceneSha256!=scientificView.sourcePacketSha256||
                   visible.sourceSceneMutated||
                   visible.createsNewEvidence||
                   visible.scientificWritebackAllowed||
                   !visible.appearanceApplied||
                   !visible.displayEncoded){
                    return status(env,-127,"Appearance headroom variant resolve failed");
                }

                if(commonSourceLuminance<0.0){
                    commonSourceLuminance=visible.sourceLuminanceNits;
                }else if(visible.sourceLuminanceNits!=commonSourceLuminance){
                    return status(env,-128,"source luminance changed across headroom variants");
                }

                if(!v.appearanceStateKnown){
                    v.appearanceState=visible.appearanceStateSha256;
                    v.appearanceStateKnown=true;
                }else if(v.appearanceState!=visible.appearanceStateSha256){
                    return status(env,-129,"appearance state changed within headroom variant");
                }

                if(visible.sourceLuminanceNits<=v.referenceWhiteNits){
                    ++v.belowKneeSampleCount;
                    if(std::abs(
                            visible.mappedLuminanceNits-
                            visible.sourceLuminanceNits)>1.0e-9){
                        ++v.belowKneeMappedLuminanceChanged;
                    }
                }

                detail::Sample sample{};
                sample.sourceLuminanceNits=visible.sourceLuminanceNits;
                sample.mappedLuminanceNits=visible.mappedLuminanceNits;
                sample.gamutOrDisplayClampApplied=
                    visible.gamutOrDisplayClampApplied;
                sample.sourceCensored=sourceCensored;
                v.samples.push_back(sample);
            }
        }
    }

    sweep::Input sweepInput{};
    sweepInput.binding.sourceEvidenceSha256=ctx.sourceSeal.sha256;
    sweepInput.binding.scientificMasterSha256=
        ctx.scientific.scientificMasterHash;
    sweepInput.binding.authorityFieldSha256=
        ctx.authorityField.contentSha256;
    sweepInput.binding.truthNegativeStateSha256=
        ctx.truthNegativeState.stateSha256;
    sweepInput.width=targetWidth;
    sweepInput.height=targetHeight;

    for(auto& v:variants){
        if(!v.appearanceStateKnown)return status(env,-130,"appearance state missing");

        detail::Input auditInput{};
        auditInput.binding.sourceEvidenceSha256=ctx.sourceSeal.sha256;
        auditInput.binding.scientificMasterSha256=
            ctx.scientific.scientificMasterHash;
        auditInput.binding.authorityFieldSha256=
            ctx.authorityField.contentSha256;
        auditInput.binding.truthNegativeStateSha256=
            ctx.truthNegativeState.stateSha256;
        auditInput.binding.appearanceStateSha256=v.appearanceState;
        auditInput.width=targetWidth;
        auditInput.height=targetHeight;
        auditInput.tileEdge=16u;
        auditInput.displayReferenceWhiteNits=v.referenceWhiteNits;
        auditInput.displayPeakNits=100.0;
        auditInput.samples=std::move(v.samples);

        detail::Report auditReport{};
        if(!detail::run(auditInput,auditReport)||
           auditReport.sourceSceneMutated||
           auditReport.createsNewEvidence||
           auditReport.scientificWritebackAllowed){
            return status(env,-131,"detail audit failed inside headroom sweep");
        }

        sweep::VariantInput in{};
        in.id=v.id;
        in.referenceWhiteNits=v.referenceWhiteNits;
        in.peakNits=100.0;
        in.detailAudit=std::move(auditReport);
        in.appearanceStateSha256=v.appearanceState;
        in.belowKneeSampleCount=v.belowKneeSampleCount;
        in.belowKneeMappedLuminanceChanged=
            v.belowKneeMappedLuminanceChanged;
        sweepInput.variants.push_back(std::move(in));
    }

    sweep::Report report{};
    if(!sweep::run(sweepInput,report)||
       report.automaticWinnerSelected||
       report.sourceSceneMutated||
       report.createsNewEvidence||
       report.scientificWritebackAllowed){
        return status(env,-132,"Appearance Highlight Headroom Sweep v0.2 failed");
    }

    if(!writeAll(destinationFd,report.json)){
        return status(env,-133,"headroom sweep write failed");
    }
    sha::Digest writtenSha{};
    if(!readbackSha(destinationFd,report.json.size(),writtenSha)||
       writtenSha!=report.jsonSha256){
        return status(env,-134,"headroom sweep post-write SHA mismatch");
    }
    if(!pipeline::reverify(ctx)){
        return status(env,-135,"source changed during headroom sweep");
    }

    const auto reportBinding=scene.report();
    if(!reportBinding.fieldSchemaValidated||
       !reportBinding.valueIdentityVerifiedForLoadedRecords||
       reportBinding.masterFieldValueBitMismatches!=0u||
       reportBinding.createsNewEvidence||
       reportBinding.scientificWritebackAllowed){
        return status(env,-136,"scene binding changed during headroom sweep");
    }

    std::ostringstream o;
    o<<"{\"status\":0";
    o<<",\"width\":"<<targetWidth;
    o<<",\"height\":"<<targetHeight;
    o<<",\"fileBytes\":"<<report.json.size();
    o<<",\"sweepSha256\":\""<<sha::hex(report.sweepSha256)<<"\"";
    o<<",\"jsonSha256\":\""<<sha::hex(report.jsonSha256)<<"\"";
    o<<",\"sourceSha256\":\""<<sha::hex(ctx.sourceSeal.sha256)<<"\"";
    o<<",\"scientificMasterSha256\":\""
      <<sha::hex(ctx.scientific.scientificMasterHash)<<"\"";
    o<<",\"authorityFieldSha256\":\""
      <<sha::hex(ctx.authorityField.contentSha256)<<"\"";
    o<<",\"truthNegativeStateSha256\":\""
      <<sha::hex(ctx.truthNegativeState.stateSha256)<<"\"";
    o<<",\"variantCount\":"<<report.variants.size();
    o<<",\"variants\":[";
    for(std::size_t i=0u;i<report.variants.size();++i){
        const auto& v=report.variants[i];
        if(i>0u)o<<",";
        o<<"{\"id\":\""<<v.id<<"\"";
        o<<",\"referenceWhiteNits\":"<<v.referenceWhiteNits;
        o<<",\"peakNits\":"<<v.peakNits;
        o<<",\"mappedAtPeak\":"<<v.mappedAtPeak;
        o<<",\"collapsedPairs\":"
          <<v.peakCollapsedDistinctAdjacentPairs;
        o<<",\"collapseFraction\":"
          <<v.collapseFractionOfDistinctPairs;
        o<<",\"collapseReductionVsBaseline\":"
          <<v.collapseReductionVsBaseline;
        o<<",\"gradientRetention\":"
          <<v.mappedGradientRetention;
        o<<",\"belowKneeSamples\":"<<v.belowKneeSampleCount;
        o<<",\"belowKneeChanged\":"
          <<v.belowKneeMappedLuminanceChanged;
        o<<",\"gamutOrDisplayClamp\":"
          <<v.gamutOrDisplayClamp;
        o<<",\"sourceCensored\":"<<v.sourceCensored;
        o<<"}";
    }
    o<<"]";
    o<<",\"postWriteVerified\":true";
    o<<",\"automaticWinnerSelected\":false";
    o<<",\"sourceSceneMutated\":false";
    o<<",\"createsNewEvidence\":false";
    o<<",\"scientificWritebackAllowed\":false}";
    return env->NewStringUTF(o.str().c_str());
}
