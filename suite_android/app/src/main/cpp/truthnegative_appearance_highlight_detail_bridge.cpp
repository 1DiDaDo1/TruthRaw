#include <jni.h>

#include "free_world_appearance_resolve_v0_7.h"
#include "free_world_scientific_open_scene_binding_v0_3.h"
#include "truthnegative_appearance_highlight_detail_audit_v0_1.h"
#include "truthnegative_deep_scene_bridge_v0_8.h"
#include "truthnegative_pipeline_bridge_common.h"
#include "truthraw_sha256_v0_69.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>
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
namespace audit =
    truthraw::truthnegative_appearance_highlight_detail_audit::v0_1;
namespace sha = truthraw::sha256_v0_69;

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
    h.update(
        reinterpret_cast<const std::uint8_t*>(label),n);
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

double fraction(std::uint64_t n,std::uint64_t d) noexcept {
    return d>0u
        ? static_cast<double>(n)/static_cast<double>(d)
        : 0.0;
}

} // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_truthraw_adaptiveui_TruthNegativeAppearanceHighlightDetailBridge_exportAndVerify(
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
        return status(env,-100,"invalid appearance-highlight audit arguments");
    }

    pipeline::Context ctx{};
    const auto prepared=pipeline::prepare(
        sourceFd,
        static_cast<std::size_t>(maxSourceResidentBytes),
        static_cast<std::size_t>(maxLogicalResidentBytes),
        ctx);
    if(!prepared){
        return status(env,prepared.code,prepared.message);
    }

    binding::BoundScenePlane scene(
        *ctx.masterSource,
        *ctx.fieldSource,
        ctx.width,
        ctx.height);
    if(!scene.valid()){
        return status(env,-101,"scientific/open-scene binding invalid");
    }

    std::uint32_t targetWidth=0u;
    std::uint32_t targetHeight=0u;
    if(!targetGeometry(
            ctx.width,
            ctx.height,
            static_cast<std::uint32_t>(requestedMaxEdge),
            targetWidth,
            targetHeight)){
        return status(env,-102,"appearance audit target geometry failed");
    }

    tn::RasterResolver resolver(
        scene,
        ctx.truthNegativeState,
        targetWidth,
        targetHeight);
    if(!resolver.valid()){
        return status(env,-103,"TruthNegative raster resolver invalid");
    }

    appearance::SceneColorimetry sceneColor{};
    sceneColor.rgbToXyz=
        matrixFromF32(ctx.prepared.color.cameraToXyzD50);
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

    appearance::DisplayTarget display{};
    display.xyzToRgb=xyzD50ToLinearSrgb();
    display.whitePointXyz={0.95047,1.0,1.08883};
    display.referenceWhiteNits=100.0;
    display.peakLuminanceNits=100.0;
    display.blackLuminanceNits=0.0;
    display.transfer=appearance::TransferFunction::Srgb;
    display.identitySha256=labeledDigest(
        "D_RAW_TN_CONT_V05_SRGB_100_NIT_DISPLAY");

    appearance::AppearancePolicy policy{};
    policy.exposureEv=0.0;
    policy.colorfulnessScale=1.0;
    policy.highlightCompression=1.0;
    policy.identitySha256=labeledDigest(
        "D_RAW_TN_CONT_V05_NEUTRAL_APPEARANCE_POLICY");

    audit::Input input{};
    input.binding.sourceEvidenceSha256=ctx.sourceSeal.sha256;
    input.binding.scientificMasterSha256=
        ctx.scientific.scientificMasterHash;
    input.binding.authorityFieldSha256=
        ctx.authorityField.contentSha256;
    input.binding.truthNegativeStateSha256=
        ctx.truthNegativeState.stateSha256;
    input.width=targetWidth;
    input.height=targetHeight;
    input.tileEdge=16u;
    input.displayReferenceWhiteNits=display.referenceWhiteNits;
    input.displayPeakNits=display.peakLuminanceNits;
    input.samples.reserve(
        static_cast<std::size_t>(targetWidth)*targetHeight);

    bool appearanceStateKnown=false;
    sha::Digest appearanceState{};

    for(std::uint32_t y=0u;y<targetHeight;++y){
        for(std::uint32_t x=0u;x<targetWidth;++x){
            tn::QueryResult query{};
            if(!resolver.resolvePixel(x,y,query)||
               query.stateSha256!=ctx.truthNegativeState.stateSha256||
               query.stateIdentityChangedByTargetRaster||
               query.createsNewEvidence||
               query.scientificWritebackAllowed){
                return status(env,-104,"TruthNegative appearance audit query failed");
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
                    ctx.truthNegativeState,
                    query,
                    cameraPlane,
                    scenePacket)||
               !scenePacket.radiometryBoundToTruthNegative||
               !scenePacket.geometryAuthoritySeparate||
               scenePacket.createsNewEvidence||
               scenePacket.scientificWritebackAllowed){
                return status(env,-105,"Deep Scene appearance audit bridge failed");
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
                return status(env,-106,"Scientific View appearance audit resolve failed");
            }

            appearance::AppearanceInput appearanceInput{};
            appearanceInput.scene=scientificView;
            appearanceInput.sceneColorimetry=sceneColor;
            appearanceInput.viewing=viewing;
            appearanceInput.display=display;
            appearanceInput.policy=policy;

            appearance::AppearanceResolvedPixel visible{};
            if(!appearance::resolveAppearance(
                    appearanceInput,visible)||
               visible.sourceSceneSha256!=
                    scientificView.sourcePacketSha256||
               visible.sourceSceneMutated||
               visible.createsNewEvidence||
               visible.scientificWritebackAllowed||
               !visible.appearanceApplied||
               !visible.displayEncoded){
                return status(env,-107,"Appearance audit resolve failed");
            }

            if(!appearanceStateKnown){
                appearanceState=visible.appearanceStateSha256;
                appearanceStateKnown=true;
            }else if(appearanceState!=visible.appearanceStateSha256){
                return status(env,-108,"Appearance state changed within audit raster");
            }

            bool sourceCensored=false;
            for(const auto& support:query.pixel.support){
                sourceCensored=
                    sourceCensored||
                    support.authority==
                        free_world::ResolvedAuthority::Censored;
            }

            audit::Sample sample{};
            sample.sourceLuminanceNits=visible.sourceLuminanceNits;
            sample.mappedLuminanceNits=visible.mappedLuminanceNits;
            sample.gamutOrDisplayClampApplied=
                visible.gamutOrDisplayClampApplied;
            sample.sourceCensored=sourceCensored;
            input.samples.push_back(sample);
        }
    }

    if(!appearanceStateKnown){
        return status(env,-109,"Appearance state missing");
    }
    input.binding.appearanceStateSha256=appearanceState;

    audit::Report report{};
    if(!audit::run(input,report)||
       report.sourceSceneMutated||
       report.createsNewEvidence||
       report.scientificWritebackAllowed){
        return status(env,-110,"Appearance highlight detail audit failed");
    }

    if(!writeAll(destinationFd,report.json)){
        return status(env,-111,"Appearance highlight audit write failed");
    }

    sha::Digest writtenSha{};
    if(!readbackSha(
            destinationFd,
            report.json.size(),
            writtenSha)||
       writtenSha!=report.jsonSha256){
        return status(env,-112,"Appearance highlight audit post-write SHA mismatch");
    }

    if(!pipeline::reverify(ctx)){
        return status(env,-113,"source changed during appearance highlight audit");
    }

    const auto reportBinding=scene.report();
    if(!reportBinding.fieldSchemaValidated||
       !reportBinding.valueIdentityVerifiedForLoadedRecords||
       reportBinding.masterFieldValueBitMismatches!=0u||
       reportBinding.createsNewEvidence||
       reportBinding.scientificWritebackAllowed){
        return status(env,-114,"scene binding changed during appearance audit");
    }

    std::ostringstream o;
    o<<"{\"status\":0";
    o<<",\"width\":"<<targetWidth;
    o<<",\"height\":"<<targetHeight;
    o<<",\"fileBytes\":"<<report.json.size();
    o<<",\"sampleCount\":"<<report.sampleCount;
    o<<",\"sourceAboveReferenceWhite\":"
      <<report.sourceAboveReferenceWhite;
    o<<",\"mappedAtPeak\":"<<report.mappedAtPeak;
    o<<",\"sourceCensored\":"<<report.sourceCensored;
    o<<",\"gamutOrDisplayClamp\":"<<report.gamutOrDisplayClamp;
    o<<",\"adjacentPairs\":"<<report.adjacentPairs;
    o<<",\"sourceDistinctAdjacentPairs\":"
      <<report.sourceDistinctAdjacentPairs;
    o<<",\"peakCollapsedDistinctAdjacentPairs\":"
      <<report.peakCollapsedDistinctAdjacentPairs;
    o<<",\"brightPeakCollapsedDistinctAdjacentPairs\":"
      <<report.brightPeakCollapsedDistinctAdjacentPairs;
    o<<",\"sourceGradientSum\":"
      <<report.sourceAbsGradientSum;
    o<<",\"mappedGradientSum\":"
      <<report.mappedAbsGradientSum;
    o<<",\"collapsedSourceGradientSum\":"
      <<report.collapsedSourceAbsGradientSum;
    o<<",\"maxCollapsedSourceGradient\":"
      <<report.maxCollapsedSourceAbsGradient;
    o<<",\"peakCollapseFractionOfDistinctPairs\":"
      <<fraction(
            report.peakCollapsedDistinctAdjacentPairs,
            report.sourceDistinctAdjacentPairs);
    o<<",\"noHighlightHeadroom\":"
      <<(report.noHighlightHeadroom?"true":"false");
    o<<",\"mappedPeakCollapseObserved\":"
      <<(report.mappedPeakCollapseObserved?"true":"false");
    o<<",\"sourceSha256\":\""
      <<sha::hex(ctx.sourceSeal.sha256)<<"\"";
    o<<",\"scientificMasterSha256\":\""
      <<sha::hex(ctx.scientific.scientificMasterHash)<<"\"";
    o<<",\"authorityFieldSha256\":\""
      <<sha::hex(ctx.authorityField.contentSha256)<<"\"";
    o<<",\"truthNegativeStateSha256\":\""
      <<sha::hex(ctx.truthNegativeState.stateSha256)<<"\"";
    o<<",\"appearanceStateSha256\":\""
      <<sha::hex(appearanceState)<<"\"";
    o<<",\"auditSha256\":\""
      <<sha::hex(report.auditSha256)<<"\"";
    o<<",\"jsonSha256\":\""
      <<sha::hex(report.jsonSha256)<<"\"";
    o<<",\"postWriteVerified\":true";
    o<<",\"sourceSceneMutated\":false";
    o<<",\"createsNewEvidence\":false";
    o<<",\"scientificWritebackAllowed\":false}";
    return env->NewStringUTF(o.str().c_str());
}
