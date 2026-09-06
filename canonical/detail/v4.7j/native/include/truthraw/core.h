#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace truthraw {

enum class StatusCode : int {
    Ok = 0,
    InvalidArgument,
    UnsupportedTopology,
    DecoderFailed,
    OutputTooSmall,
    BackendFailed,
};

enum class CfaPattern : int { BGGR=0, RGGB=1, GRBG=2, GBRG=3 };
enum class Orientation : int { Normal=1, Rotate180=3, Rotate90CW=6, Rotate90CCW=8 };
enum class ReconstructionQuality : int { ReferenceFallback=0, ResearchBackend=1, ProductionBackend=2 };
enum class AppearanceProfile : int { NeutralReference=0, SkinSafeDetailedCrisp=1, AdaptiveDetailedCrisp=2, ExternalProfile=3 };

struct Status {
    StatusCode code = StatusCode::Ok;
    std::string message;
    explicit operator bool() const { return code == StatusCode::Ok; }
    static Status ok() { return {}; }
    static Status error(StatusCode c, std::string m) { Status s; s.code=c; s.message=std::move(m); return s; }
};

struct DngMetadata {
    int width = 0;
    int height = 0;
    CfaPattern cfa = CfaPattern::BGGR;
    Orientation orientation = Orientation::Normal;
    float whiteLevel = 1023.0f;
    std::array<float,4> blackPhase = {64.f,64.f,64.f,64.f};
    std::array<float,6> noiseProfile = {0.f,0.f,0.f,0.f,0.f,0.f};
    std::array<float,9> cameraToXyzD50 = {1,0,0, 0,1,0, 0,0,1};
    bool hasNoiseProfile = false;
    bool hasGainField = false;
    bool hasResidualBlack = false;
    std::string sourceId;
};

struct DecodedDngFrame {
    DngMetadata meta;
    std::vector<std::uint16_t> raw;
    std::vector<float> gainField;
    std::vector<float> rowBias;
    std::vector<float> colBias;
};

class IDngDecoder {
public:
    virtual ~IDngDecoder() = default;
    virtual Status decode(const std::string& path, DecodedDngFrame& out) = 0;
};

struct TilePolicy { int core = 512; int halo = 16; };

struct ProcessOptions {
    TilePolicy tile{};
    int threads = 1;
    bool hdrEnabled = true;
    AppearanceProfile appearance = AppearanceProfile::NeutralReference;
    bool keepScientificDiagnostics = false;
    int sdrLutSize = 4096;
};

// Read-only frame context supplied to appearance backends. It contains only
// evidence/scene descriptors already known before appearance processing.
struct AppearanceContext {
    float noiseSigmaAt2Pct = 0.0f;
};

struct ExposurePlan {
    std::array<float,5> anchorsX = {0,0.01f,0.18f,0.60f,1};
    std::array<float,5> anchorsY = {0,0.004f,0.18f,0.72f,1};
    float noiseSigmaAt2Pct = 0.f;
    float clipFraction = 0.f;
    float blackFactor = 1.f;
    float midGain = 1.f;
    float sdrHighlightGain = 1.f;
    float evidenceConfidence = 1.f;
    float sceneToDisplayScalar = 1.f;
    float hdrGateStartY = 0.1f;
    float hdrGateFullY = 0.7f;
    float hdrMaxGain = 1.f;
    std::uint64_t stage2Over1Count = 0;
};

struct TimingInfo {
    double stage2ReconstructColorMs = 0.0;
    double exposurePlanMs = 0.0;
    double appearanceMs = 0.0;
    double sdrApplyMs = 0.0;
    double gainMapMs = 0.0;
    double totalMs = 0.0;
};

struct MemoryInfo {
    std::size_t peakTileBytes = 0;
    std::size_t finalSdrBytes = 0;
    std::size_t halfGainBytes = 0;
    std::size_t diagnosticBytes = 0;
};

struct Provenance {
    bool scientificMasterModifiedByAppearance = false;
    bool gainMapAppliedExactlyOnce = true;
    bool censoredHighlightRecoveryClaimed = false;
    bool scenePlanSharedAcrossLooks = true;
    bool perFilePerPhaseBlackUsed = true;
    bool residualBlackApplied = false;
    ReconstructionQuality reconstructionQuality = ReconstructionQuality::ReferenceFallback;
    std::string reconstructionBackend = "reference_measured_preserving_bilinear";
    std::string appearanceBackend = "neutral_reference_cpu";
    std::string residualBlackStatus = "metadata_only_no_dark_pack";
    std::string colorFidelityPolicy = "none";
};

struct ProcessResult {
    Status status;
    int width = 0;
    int height = 0;
    Orientation orientation = Orientation::Normal;
    ExposurePlan exposure;
    TimingInfo timing;
    MemoryInfo memory;
    Provenance provenance;
    std::vector<float> sdrRgb;
    std::vector<float> halfLogGain;
    std::vector<float> stage2Diagnostic;
};

struct TileRect {
    int x0=0,y0=0,x1=0,y1=0;
    int hx0=0,hy0=0,hx1=0,hy1=0;
};

std::vector<TileRect> make_tiles(int width, int height, const TilePolicy& policy);

class IReconstructionBackend {
public:
    virtual ~IReconstructionBackend() = default;
    virtual ReconstructionQuality quality() const = 0;
    virtual const char* name() const = 0;
    virtual int requiredHalo() const = 0;
    virtual Status reconstructTile(
        const float* stage2FullTile, int tileW, int tileH,
        int globalHx0, int globalHy0,
        int coreX0, int coreY0, int coreW, int coreH,
        CfaPattern cfa, float* coreCameraRgb) = 0;
};

class ReferenceMeasuredPreservingReconstruction final : public IReconstructionBackend {
public:
    ReconstructionQuality quality() const override { return ReconstructionQuality::ReferenceFallback; }
    const char* name() const override { return "reference_measured_preserving_bilinear"; }
    int requiredHalo() const override { return 1; }
    Status reconstructTile(const float*,int,int,int,int,int,int,int,int,CfaPattern,float*) override;
};

// v4.7i deterministic research backend: directional, edge-aware interpolation
// with exact reinjection of the physically measured CFA component.
class ResearchEdgeAwareMeasuredPreservingReconstruction final : public IReconstructionBackend {
public:
    ReconstructionQuality quality() const override { return ReconstructionQuality::ResearchBackend; }
    const char* name() const override { return "research_edge_aware_support_limited_measured_preserving_v47i"; }
    int requiredHalo() const override { return 3; }
    Status reconstructTile(const float*,int,int,int,int,int,int,int,int,CfaPattern,float*) override;
};

class IAppearanceBackend {
public:
    virtual ~IAppearanceBackend() = default;
    virtual AppearanceProfile profile() const = 0;
    virtual const char* name() const = 0;
    // Halo is in already reconstructed/color-converted RGB pixels.
    virtual int requiredHalo() const = 0;
    virtual const char* colorFidelityPolicy() const = 0;
    virtual Status applyTile(
        const float* neutralRgbTile, int tileW, int tileH,
        int coreX0, int coreY0, int coreW, int coreH,
        const AppearanceContext& context,
        float* coreAppearanceRgb) const = 0;
};

class NeutralReferenceAppearance final : public IAppearanceBackend {
public:
    AppearanceProfile profile() const override { return AppearanceProfile::NeutralReference; }
    const char* name() const override { return "neutral_reference_cpu"; }
    int requiredHalo() const override { return 0; }
    const char* colorFidelityPolicy() const override { return "neutral_reference_no_appearance_displacement"; }
    Status applyTile(const float*,int,int,int,int,int,int,const AppearanceContext&,float*) const override;
};

// v4.7i production-port of the v4.6b principle. It deliberately avoids skin
// segmentation: local detail is applied in luminance and profile-induced OKLab
// chroma/hue displacement is constrained relative to Neutral Reference.
class SkinSafeDetailedCrispAppearance final : public IAppearanceBackend {
public:
    AppearanceProfile profile() const override { return AppearanceProfile::SkinSafeDetailedCrisp; }
    const char* name() const override { return "skin_safe_detailed_crisp_color_fidelity_guard_v47i"; }
    int requiredHalo() const override { return 4; }
    const char* colorFidelityPolicy() const override { return "oklab_color_fidelity_guard_no_skin_segmentation"; }
    Status applyTile(const float*,int,int,int,int,int,int,const AppearanceContext&,float*) const override;
};


// v4.7j Detail Truth backend. Unlike the legacy detailed look, it separates
// micro/fine/texture bands, adapts strength to NoiseProfile-derived confidence,
// suppresses smooth-region over-acutance without semantic/skin segmentation,
// and changes luminance only (RGB direction is preserved exactly).
class AdaptiveDetailedCrispAppearance final : public IAppearanceBackend {
public:
    AppearanceProfile profile() const override { return AppearanceProfile::AdaptiveDetailedCrisp; }
    const char* name() const override { return "adaptive_detailed_crisp_multiband_hard_edge_guard_v47j"; }
    int requiredHalo() const override { return 5; }
    const char* colorFidelityPolicy() const override { return "luminance_only_rgb_direction_preserved_no_semantic_segmentation"; }
    Status applyTile(const float*,int,int,int,int,int,int,const AppearanceContext&,float*) const override;
};

class TruthRawProcessor {
public:
    TruthRawProcessor();
    explicit TruthRawProcessor(std::shared_ptr<IReconstructionBackend> reconstruction);
    TruthRawProcessor(std::shared_ptr<IReconstructionBackend> reconstruction,
                      std::shared_ptr<IAppearanceBackend> appearance);

    Status processDng(const std::string& path, IDngDecoder& decoder,
                      const ProcessOptions& options, ProcessResult& result) const;
    Status processFrame(const DecodedDngFrame& frame,
                        const ProcessOptions& options, ProcessResult& result) const;

private:
    std::shared_ptr<IReconstructionBackend> reconstruction_;
    std::shared_ptr<IAppearanceBackend> appearance_;
};

float luminance709(float r,float g,float b);
std::vector<float> build_monotone_lut(const ExposurePlan& plan, int n=4096);
ExposurePlan choose_exposure_plan_from_histograms(
    const std::vector<std::uint64_t>& displayHist,
    const std::vector<std::uint64_t>& sceneHist,
    float sceneHistMax,
    float noiseSigmaAt2Pct,
    float clipFraction,
    std::uint64_t stage2Over1Count);

} // namespace truthraw
