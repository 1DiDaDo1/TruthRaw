#pragma once
#include "dng_stage2_v0_4.h"
#include "truthrange_dense_uncertainty_v0_3.h"
#include "uncertainty_runtime_v5_0g.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace truthraw_v05 {

enum class BayerRoleV05 : std::uint8_t { R=0, G1=1, G2=2, B=3 };

struct DngExtraV05 {
    std::array<float,6> noiseProfileRgb{}; // R:(S,O), G:(S,O), B:(S,O) in CFAPlaneColor order 0,1,2
    std::array<std::uint8_t,3> cfaPlaneColor{};
    int iso = 0;
};

struct BackendAnchorV05 {
    int y=0, x=0;
    BayerRoleV05 role=BayerRoleV05::R;
    int rgbChannel=0;
    std::array<float,18> features{};
    float predictedHidden=0.f;
    float sigmaFeature=0.f;
    float snr=0.f;
    float p50Abs=0.f;
    float p95Abs=0.f;
};

struct DenseStreamSummaryV05 {
    std::uint64_t totalRgbEntries=0;
    std::uint64_t measuredUncensored=0;
    std::uint64_t measuredHighCensored=0;
    std::uint64_t reconstructedProxyValid=0;
    std::uint64_t reconstructedProxyUnresolved=0;
    std::uint64_t truthrangeFiniteEstimate=0;
    std::uint64_t darkP95LowerInfinity=0;
    std::uint64_t brightEvidenceUpperInfinity=0;
    double estimateEvMin=0.0;
    double estimateEvMax=0.0;
};

struct RealLatentSummaryV05 {
    int width=0,height=0,iso=0;
    double stage2ParityMaxAbs=0.0;
    std::uint64_t stage2FloatBitsSum=0;
    std::uint32_t stage2FloatBitsXor=0;
    double measuredCfaReinjectionMaxAbs=0.0;
    double selfGaugeL0=0.0;
    std::string selfGaugeId;
    std::array<std::uint64_t,4> anchorsByRole{};
    std::uint64_t anchorsSkippedSourceClip=0;
    float anchorP50Min=0.f,anchorP50Max=0.f;
    float anchorP95Min=0.f,anchorP95Max=0.f;
    DenseStreamSummaryV05 dense;
};

DngExtraV05 read_dng_extra_v0_5(const std::string& path);

truthraw::Status adapt_classic_dng_to_decoded_v0_5(
    const truthraw_v04::ClassicDng& dng,
    const DngExtraV05& extra,
    truthraw::DecodedDngFrame& out);

const char* role_name_v0_5(BayerRoleV05 role);
int role_rgb_channel_v0_5(BayerRoleV05 role);
int role_runtime_code_v0_5(BayerRoleV05 role);
BayerRoleV05 role_at_v0_5(int y,int x);

std::array<float,18> build_v5g_features_exact_v0_5(
    const truthraw::DecodedDngFrame& frame,
    const truthraw::LatentCameraSceneV02& scene,
    BayerRoleV05 role,
    int y,int x,
    float& predictedHidden,
    float& sigmaFeature,
    float& snr);

truthraw::Status build_v5g_measured_role_anchors_v0_5(
    const truthraw::DecodedDngFrame& frame,
    const truthraw::LatentCameraSceneV02& scene,
    std::vector<BackendAnchorV05>& anchors,
    std::array<std::uint64_t,4>& countsByRole,
    std::uint64_t& skippedSourceClip);

truthraw::Status stream_dense_truthrange_v0_5(
    const truthraw::DecodedDngFrame& frame,
    const truthraw::LatentCameraSceneV02& scene,
    const truthraw::TruthRangeGaugeV02& gauge,
    const std::vector<BackendAnchorV05>& anchors,
    int transportRadius,
    int tileSize,
    DenseStreamSummaryV05& out);

truthraw::Status run_real_latent_bridge_v0_5(
    const std::string& dngPath,
    truthraw::IReconstructionBackend& reconstruction,
    RealLatentSummaryV05& summary,
    std::vector<BackendAnchorV05>* anchorsOut=nullptr,
    int transportRadius=40,
    int tileSize=256);

} // namespace truthraw_v05
