#include <jni.h>

#include "free_world_appearance_resolve_v0_7.h"
#include "free_world_scientific_open_scene_binding_v0_3.h"
#include "truthnegative_deep_scene_bridge_v0_8.h"
#include "truthnegative_n2_cfa_audit_v0_1.h"
#include "truthnegative_pipeline_bridge_common.h"
#include "truthraw_sha256_v0_69.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {

namespace pipeline = truthraw::android_truthnegative_pipeline::v0_1;
namespace appearance = truthraw::free_world_appearance_resolve::v0_7;
namespace binding = truthraw::free_world_scientific_open_scene_binding::v0_3;
namespace deep = truthraw::free_world_deep_scene_contribution::v0_4;
namespace free_world = truthraw::free_world_pixel_resolve_2d::v0_2;
namespace tn = truthraw::truthnegative_continuous::v0_5;
namespace tn_deep = truthraw::truthnegative_deep_scene_bridge::v0_8;
namespace n2 = truthraw::truthnegative_n2_cfa_audit::v0_1;
namespace sha = truthraw::sha256_v0_69;

constexpr jint kMagic = 0x31424132; // "2AB1" little-endian view.
constexpr std::size_t kCropCount = 3u;
constexpr std::size_t kCropMetaInts = 32u;
constexpr std::size_t kHeaderInts = 48u + kCropCount * kCropMetaInts;
constexpr std::uint32_t kCropEdge = 192u;
constexpr double kDeltaGain = 32.0;

jint clamp_metric(std::uint64_t value) noexcept {
    const auto cap=static_cast<std::uint64_t>(std::numeric_limits<jint>::max());
    return static_cast<jint>(std::min(value,cap));
}

jint scaled_metric(double value,double scale) noexcept {
    if(!std::isfinite(value)||value<=0.0||!std::isfinite(scale)||scale<=0.0)return 0;
    const double cap=static_cast<double>(std::numeric_limits<jint>::max());
    return static_cast<jint>(std::llround(std::min(value*scale,cap)));
}

jintArray status_packet(JNIEnv* env,jint status) {
    std::array<jint,kHeaderInts> values{};
    values[0]=kMagic; values[1]=status;
    jintArray out=env->NewIntArray(static_cast<jsize>(values.size()));
    if(out) env->SetIntArrayRegion(out,0,static_cast<jsize>(values.size()),values.data());
    return out;
}

void digest_to_words(const sha::Digest& digest,jint* out) noexcept {
    for(std::size_t word=0u;word<8u;++word){
        const std::size_t i=word*4u;
        const std::uint32_t v=
            static_cast<std::uint32_t>(digest[i]) |
            (static_cast<std::uint32_t>(digest[i+1u])<<8u) |
            (static_cast<std::uint32_t>(digest[i+2u])<<16u) |
            (static_cast<std::uint32_t>(digest[i+3u])<<24u);
        out[word]=static_cast<jint>(v);
    }
}

sha::Digest labeled_digest(
    const char* label,
    const sha::Digest* parent=nullptr,
    const std::string* text=nullptr) noexcept {
    sha::Hasher h;
    const std::size_t n=std::char_traits<char>::length(label);
    h.update(reinterpret_cast<const std::uint8_t*>(label),n);
    if(parent)h.update(*parent);
    if(text)h.update(reinterpret_cast<const std::uint8_t*>(text->data()),text->size());
    return h.finalize();
}

sha::Digest candidate_scene_digest(
    const sha::Digest& scientificScene,
    const sha::Digest& gridSha,
    std::uint32_t gx,
    std::uint32_t gy,
    const std::array<double,3u>& rgb) noexcept {
    sha::Hasher h;
    constexpr char domain[]="D_RAW_TN_N2_1_TO_1_CROP_CANDIDATE_V0_1";
    h.update(reinterpret_cast<const std::uint8_t*>(domain),sizeof(domain)-1u);
    h.update(scientificScene);h.update(gridSha);
    const std::array<std::uint32_t,2u> xy{gx,gy};
    for(auto v:xy){
        std::array<std::uint8_t,4u> b{
            static_cast<std::uint8_t>(v),
            static_cast<std::uint8_t>(v>>8u),
            static_cast<std::uint8_t>(v>>16u),
            static_cast<std::uint8_t>(v>>24u)};
        h.update(b);
    }
    for(double v:rgb){
        const auto bits=std::bit_cast<std::uint64_t>(v);
        std::array<std::uint8_t,8u> b{};
        for(std::size_t i=0u;i<8u;++i)b[i]=static_cast<std::uint8_t>(bits>>(8u*i));
        h.update(b);
    }
    return h.finalize();
}

appearance::Matrix3 matrix_from_f32(const std::array<float,9u>& source) noexcept {
    appearance::Matrix3 out{};
    for(std::size_t i=0u;i<source.size();++i)out.m[i]=static_cast<double>(source[i]);
    return out;
}

appearance::Matrix3 xyz_d50_to_linear_srgb() noexcept {
    appearance::Matrix3 out{};
    out.m={3.1338561,-1.6168667,-0.4906146,
          -0.9787684,1.9161415,0.0334540,
           0.0719453,-0.2289914,1.4052427};
    return out;
}

std::uint32_t quantize_u8(double encoded) noexcept {
    if(!std::isfinite(encoded))return 0u;
    const long q=std::lround(std::clamp(encoded,0.0,1.0)*255.0);
    return static_cast<std::uint32_t>(std::clamp<long>(q,0l,255l));
}

std::uint32_t argb(
    const std::array<double,3u>& encoded) noexcept {
    const auto r=quantize_u8(encoded[0]);
    const auto g=quantize_u8(encoded[1]);
    const auto b=quantize_u8(encoded[2]);
    return 0xff000000u|(r<<16u)|(g<<8u)|b;
}

struct CropRect final {
    std::uint32_t type=0u;
    std::uint32_t x=0u,y=0u,width=0u,height=0u;
};

std::size_t best_candidate_tile(const n2::Result& audit) noexcept {
    std::size_t best=0u; double bestScore=-1e300;
    for(std::size_t i=0u;i<audit.tiles.size();++i){
        const auto& t=audit.tiles[i];
        if(t.sampled==0u)continue;
        const double corrected=static_cast<double>(t.audit.corrected);
        const double structure=static_cast<double>(t.audit.structureProtected);
        const double censor=static_cast<double>(
            t.audit.censoredProtected+t.audit.censorBoundaryProtected);
        const double score=corrected-1.5*structure-4.0*censor;
        if(score>bestScore){bestScore=score;best=i;}
    }
    return best;
}

std::size_t best_structure_tile(const n2::Result& audit) noexcept {
    std::size_t best=0u; std::uint64_t score=0u;
    for(std::size_t i=0u;i<audit.tiles.size();++i){
        const auto s=audit.tiles[i].audit.structureProtected;
        if(s>score){score=s;best=i;}
    }
    return best;
}

std::size_t best_censor_tile(const n2::Result& audit) noexcept {
    std::size_t best=0u; std::uint64_t score=0u;
    for(std::size_t i=0u;i<audit.tiles.size();++i){
        const auto s=audit.tiles[i].audit.censoredProtected+
                     audit.tiles[i].audit.censorBoundaryProtected;
        if(s>score){score=s;best=i;}
    }
    return best;
}

CropRect crop_around(
    const n2::TileAudit& tile,
    std::uint32_t type,
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight) noexcept {
    const std::uint32_t edge=std::min(
        kCropEdge,std::min(sourceWidth,sourceHeight));
    const std::uint32_t cx=tile.x+tile.width/2u;
    const std::uint32_t cy=tile.y+tile.height/2u;
    const std::uint32_t maxX=sourceWidth-edge;
    const std::uint32_t maxY=sourceHeight-edge;
    const std::uint32_t x=std::min(
        maxX,cx>edge/2u?cx-edge/2u:0u);
    const std::uint32_t y=std::min(
        maxY,cy>edge/2u?cy-edge/2u:0u);
    return {type,x,y,edge,edge};
}

struct CropMetrics final {
    std::uint64_t changedPixels=0u;
    std::uint64_t adjustedChannels=0u;
    std::uint64_t clampA=0u;
    std::uint64_t clampB=0u;
    double absDeltaSum=0.0;
    double maxAbsDelta=0.0;
};

bool make_scientific_view(
    const tn::State& state,
    const tn::QueryResult& query,
    std::uint64_t provenance,
    deep::DeepResolvedPixel& out) noexcept {
    tn_deep::CameraPlaneObjectInput cameraPlane{};
    cameraPlane.provenanceId=provenance;
    cameraPlane.regionId=1u;
    cameraPlane.objectId=1u;
    cameraPlane.depth=0.0;
    cameraPlane.geometryAuthority=
        truthraw::free_world_deep_scene_binding::v0_5::
            GeometryAuthority::ImagePlaneBound;
    cameraPlane.parentAncestrySha256=query.querySha256;
    tn_deep::ScenePacket packet{};
    if(!tn_deep::buildCameraPlaneObject(state,query,cameraPlane,packet)||
       packet.createsNewEvidence||packet.scientificWritebackAllowed)return false;
    if(!deep::resolve(packet.deepPacket,deep::ResolveView::ScientificView,out)||
       out.createsNewEvidence||out.scientificWritebackAllowed||
       out.physicalFrameCount!=1u||out.independentEvidenceCount!=1u)return false;
    return true;
}

} // namespace

extern "C" JNIEXPORT jintArray JNICALL
Java_com_truthraw_adaptiveui_TruthNegativeN2CropAbNativeBridge_buildDiagnosticCrops(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if(sourceFd<0||maxSourceResidentBytes<=0||maxLogicalResidentBytes<=0){
        return status_packet(env,-1);
    }

    pipeline::Context ctx{};
    const auto prepared=pipeline::prepare(
        static_cast<int>(sourceFd),
        static_cast<std::size_t>(maxSourceResidentBytes),
        static_cast<std::size_t>(maxLogicalResidentBytes),
        ctx);
    if(!prepared)return status_packet(env,prepared.code);
    if(ctx.width<32u||ctx.height<32u)return status_packet(env,-2);

    binding::BoundScenePlane scene(
        *ctx.masterSource,*ctx.fieldSource,ctx.width,ctx.height);
    if(!scene.valid())return status_packet(env,-3);

    n2::Binding coarseBinding{};
    coarseBinding.sourceEvidenceSha256=ctx.sourceSeal.sha256;
    coarseBinding.truthNegativeStateSha256=ctx.truthNegativeState.stateSha256;
    n2::Options coarseOptions{};
    coarseOptions.tileEdge=64u;
    coarseOptions.samplingPeriod=8u;
    n2::Result coarse{};
    if(!n2::run(*ctx.openedSource.source,coarseBinding,coarseOptions,coarse)||
       coarse.tiles.empty()||coarse.sourceValuesModified||
       coarse.truthNegativeModified||coarse.createsNewEvidence||
       coarse.scientificWritebackAllowed){
        return status_packet(env,-4);
    }

    const std::array<std::size_t,kCropCount> selected{
        best_candidate_tile(coarse),
        best_structure_tile(coarse),
        best_censor_tile(coarse)};
    const std::array<std::uint32_t,kCropCount> types{1u,2u,3u};
    std::array<CropRect,kCropCount> crops{};
    for(std::size_t i=0u;i<kCropCount;++i){
        crops[i]=crop_around(
            coarse.tiles[selected[i]],types[i],ctx.width,ctx.height);
    }

    tn::RasterResolver fullResolver(
        scene,ctx.truthNegativeState,ctx.width,ctx.height);
    if(!fullResolver.valid())return status_packet(env,-5);

    appearance::SceneColorimetry sceneColor{};
    sceneColor.rgbToXyz=matrix_from_f32(ctx.produced.color.cameraToXyzD50);
    sceneColor.referenceWhiteXyz={0.96422,1.0,0.82521};
    sceneColor.sceneReferenceWhiteNits=100.0;
    sceneColor.identitySha256=labeled_digest(
        "D_RAW_TN_N2_CROP_SCENE_COLOR",
        &ctx.truthNegativeState.stateSha256,
        &ctx.produced.color.bindingId);

    appearance::ViewingConditions viewing{};
    viewing.adaptingWhiteXyz={0.95047,1.0,1.08883};
    viewing.adaptingLuminanceNits=20.0;
    viewing.backgroundLuminanceNits=20.0;
    viewing.surround=appearance::Surround::Average;
    viewing.viewingDistanceMeters=0.5;
    viewing.identitySha256=labeled_digest(
        "D_RAW_TN_N2_CROP_VIEW_AVERAGE_20_NIT");

    appearance::DisplayTarget display{};
    display.xyzToRgb=xyz_d50_to_linear_srgb();
    display.whitePointXyz={0.95047,1.0,1.08883};
    display.referenceWhiteNits=100.0;
    display.peakLuminanceNits=100.0;
    display.blackLuminanceNits=0.0;
    display.transfer=appearance::TransferFunction::Srgb;
    display.identitySha256=labeled_digest(
        "D_RAW_TN_N2_CROP_SRGB_100_NIT");

    appearance::AppearancePolicy policy{};
    policy.exposureEv=0.0;
    policy.colorfulnessScale=1.0;
    policy.highlightCompression=1.0;
    policy.identitySha256=labeled_digest(
        "D_RAW_TN_N2_CROP_NEUTRAL_APPEARANCE");

    const std::uint32_t edge=crops[0].width;
    const std::size_t cropPixels=
        static_cast<std::size_t>(edge)*edge;
    const std::size_t payloadPixels=
        cropPixels*kCropCount*3u;
    if(payloadPixels>
       static_cast<std::size_t>(std::numeric_limits<jsize>::max())-kHeaderInts){
        return status_packet(env,-6);
    }

    std::vector<jint> packet(kHeaderInts+payloadPixels,0);
    packet[0]=kMagic;packet[1]=0;
    packet[2]=static_cast<jint>(kCropCount);
    packet[3]=static_cast<jint>(edge);
    packet[4]=static_cast<jint>(ctx.width);
    packet[5]=static_cast<jint>(ctx.height);
    packet[6]=static_cast<jint>(ctx.openedSource.source->metadata().orientation);
    packet[7]=1;packet[8]=1;
    packet[9]=0;packet[10]=0;packet[11]=0;
    packet[12]=static_cast<jint>(kDeltaGain);
    digest_to_words(ctx.truthNegativeState.stateSha256,packet.data()+14u);
    digest_to_words(ctx.authorityField.contentSha256,packet.data()+22u);
    digest_to_words(coarse.auditSha256,packet.data()+30u);
    digest_to_words(coarse.spatialSha256,packet.data()+38u);

    const std::size_t rasterBase=kHeaderInts;
    for(std::size_t cropIndex=0u;cropIndex<kCropCount;++cropIndex){
        const auto& crop=crops[cropIndex];

        n2::Options cropOptions{};
        cropOptions.tileEdge=64u;
        cropOptions.samplingPeriod=2u;
        cropOptions.regionX=crop.x;
        cropOptions.regionY=crop.y;
        cropOptions.regionWidth=crop.width;
        cropOptions.regionHeight=crop.height;
        cropOptions.appearanceGridWidth=crop.width;
        cropOptions.appearanceGridHeight=crop.height;

        n2::Result audit{};
        if(!n2::run(
                *ctx.openedSource.source,
                coarseBinding,
                cropOptions,
                audit)||
           !audit.appearanceGridDerived||
           audit.appearanceGrid.size()!=cropPixels||
           audit.sampled!=cropPixels||
           audit.sourceValuesModified||
           audit.truthNegativeModified||
           audit.createsNewEvidence||
           audit.scientificWritebackAllowed){
            return status_packet(env,-7);
        }

        CropMetrics metrics{};
        const std::size_t aBase=
            rasterBase+(cropIndex*3u+0u)*cropPixels;
        const std::size_t bBase=
            rasterBase+(cropIndex*3u+1u)*cropPixels;
        const std::size_t dBase=
            rasterBase+(cropIndex*3u+2u)*cropPixels;

        for(std::uint32_t ly=0u;ly<crop.height;++ly){
            for(std::uint32_t lx=0u;lx<crop.width;++lx){
                const std::uint32_t gx=crop.x+lx;
                const std::uint32_t gy=crop.y+ly;
                const std::size_t local=
                    static_cast<std::size_t>(ly)*crop.width+lx;

                tn::QueryResult query{};
                if(!fullResolver.resolvePixel(gx,gy,query)||
                   query.stateSha256!=ctx.truthNegativeState.stateSha256||
                   query.stateIdentityChangedByTargetRaster||
                   query.createsNewEvidence||query.scientificWritebackAllowed){
                    return status_packet(env,-8);
                }

                deep::DeepResolvedPixel scientificView{};
                const std::uint64_t provenance=
                    1u+static_cast<std::uint64_t>(gy)*ctx.width+gx;
                if(!make_scientific_view(
                        ctx.truthNegativeState,query,provenance,scientificView)){
                    return status_packet(env,-9);
                }

                appearance::AppearanceInput aInput{};
                aInput.scene=scientificView;
                aInput.sceneColorimetry=sceneColor;
                aInput.viewing=viewing;
                aInput.display=display;
                aInput.policy=policy;
                appearance::AppearanceResolvedPixel aVisible{};
                if(!appearance::resolveAppearance(aInput,aVisible)||
                   aVisible.sourceSceneMutated||
                   aVisible.createsNewEvidence||
                   aVisible.scientificWritebackAllowed){
                    return status_packet(env,-10);
                }
                if(aVisible.gamutOrDisplayClampApplied)++metrics.clampA;

                auto candidateScene=scientificView;
                const auto& bin=audit.appearanceGrid[local];
                bool adjusted=false;
                for(std::size_t cc=0u;cc<3u;++cc){
                    if(bin.sampled[cc]==0u)continue;
                    const double correction=
                        bin.correctionSum[cc]/
                        static_cast<double>(bin.sampled[cc]);
                    if(!std::isfinite(correction))return status_packet(env,-11);
                    if(correction!=0.0){
                        candidateScene.sceneLinearRgb[cc]+=correction;
                        adjusted=true;
                        ++metrics.adjustedChannels;
                    }
                }
                candidateScene.channelAuthority.fill(
                    free_world::ResolvedAuthority::Unknown);
                candidateScene.uncertaintyKnown.fill(false);
                candidateScene.p95Uncertainty.fill(0.0);
                candidateScene.visibility={};
                candidateScene.appearanceApplied=false;
                candidateScene.displayEncoded=false;
                candidateScene.sourcePacketSha256=candidate_scene_digest(
                    scientificView.sourcePacketSha256,
                    audit.appearanceGridSha256,
                    gx,gy,candidateScene.sceneLinearRgb);
                candidateScene.createsNewEvidence=false;
                candidateScene.scientificWritebackAllowed=false;

                appearance::AppearanceInput bInput{};
                bInput.scene=candidateScene;
                bInput.sceneColorimetry=sceneColor;
                bInput.viewing=viewing;
                bInput.display=display;
                bInput.policy=policy;
                appearance::AppearanceResolvedPixel bVisible{};
                if(!appearance::resolveAppearance(bInput,bVisible)||
                   bVisible.sourceSceneMutated||
                   bVisible.createsNewEvidence||
                   bVisible.scientificWritebackAllowed){
                    return status_packet(env,-12);
                }
                if(bVisible.gamutOrDisplayClampApplied)++metrics.clampB;

                const auto aArgb=argb(aVisible.encodedRgb);
                const auto bArgb=argb(bVisible.encodedRgb);
                packet[aBase+local]=static_cast<jint>(aArgb);
                packet[bBase+local]=static_cast<jint>(bArgb);

                std::array<double,3u> delta{};
                bool displayChanged=false;
                for(std::size_t cc=0u;cc<3u;++cc){
                    const double d=std::abs(
                        bVisible.encodedRgb[cc]-aVisible.encodedRgb[cc]);
                    if(!std::isfinite(d))return status_packet(env,-13);
                    metrics.absDeltaSum+=d;
                    metrics.maxAbsDelta=std::max(metrics.maxAbsDelta,d);
                    delta[cc]=std::clamp(d*kDeltaGain,0.0,1.0);
                    displayChanged=displayChanged||
                        quantize_u8(aVisible.encodedRgb[cc])!=
                        quantize_u8(bVisible.encodedRgb[cc]);
                }
                if(displayChanged)++metrics.changedPixels;
                packet[dBase+local]=static_cast<jint>(argb(delta));
                (void)adjusted;
            }
        }

        const std::size_t m=48u+cropIndex*kCropMetaInts;
        packet[m+0u]=static_cast<jint>(crop.type);
        packet[m+1u]=static_cast<jint>(crop.x);
        packet[m+2u]=static_cast<jint>(crop.y);
        packet[m+3u]=static_cast<jint>(crop.width);
        packet[m+4u]=static_cast<jint>(crop.height);
        packet[m+5u]=clamp_metric(audit.sampled);
        packet[m+6u]=clamp_metric(audit.audit.corrected);
        packet[m+7u]=clamp_metric(audit.audit.preserved);
        packet[m+8u]=clamp_metric(audit.audit.structureProtected);
        packet[m+9u]=clamp_metric(audit.audit.censoredProtected);
        packet[m+10u]=clamp_metric(audit.audit.censorBoundaryProtected);
        packet[m+11u]=clamp_metric(audit.audit.residualOutlierProtected);
        packet[m+12u]=clamp_metric(audit.audit.noNeighborhoodProtected);
        packet[m+13u]=clamp_metric(metrics.changedPixels);
        packet[m+14u]=clamp_metric(metrics.adjustedChannels);
        packet[m+15u]=scaled_metric(
            metrics.absDeltaSum/(static_cast<double>(cropPixels)*3.0),
            1000000000.0);
        packet[m+16u]=scaled_metric(metrics.maxAbsDelta,1000000000.0);
        const double removedFraction=
            audit.audit.totalResidualEnergy>0.0
                ? audit.audit.removedResidualEnergy/audit.audit.totalResidualEnergy
                : 0.0;
        packet[m+17u]=scaled_metric(
            std::clamp(removedFraction,0.0,1.0),1000000.0);
        packet[m+18u]=scaled_metric(
            audit.audit.maxAbsCorrection,1000000000.0);
        packet[m+19u]=clamp_metric(metrics.clampA);
        packet[m+20u]=clamp_metric(metrics.clampB);
        packet[m+21u]=0;
        digest_to_words(audit.appearanceGridSha256,packet.data()+m+22u);
    }

    const auto report=scene.report();
    if(!report.fieldSchemaValidated||
       !report.valueIdentityVerifiedForLoadedRecords||
       report.masterFieldValueBitMismatches!=0u||
       report.createsNewEvidence||report.scientificWritebackAllowed||
       report.physicalFrameCount!=1u||report.independentEvidenceCount!=1u||
       !pipeline::reverify(ctx)){
        return status_packet(env,-14);
    }

    jintArray out=env->NewIntArray(static_cast<jsize>(packet.size()));
    if(!out)return nullptr;
    env->SetIntArrayRegion(
        out,0,static_cast<jsize>(packet.size()),packet.data());
    return out;
}
