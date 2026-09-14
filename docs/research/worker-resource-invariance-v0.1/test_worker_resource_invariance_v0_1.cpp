#include "truthraw/core.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

[[noreturn]] void fail(const std::string& message) {
    std::cerr << "WORKER_RESOURCE_INVARIANCE_V0_1_FAIL: " << message << "\n";
    std::exit(1);
}

void require(bool condition, const std::string& message) {
    if (!condition) fail(message);
}

truthraw::DecodedDngFrame make_frame() {
    truthraw::DecodedDngFrame frame;
    auto& m = frame.meta;
    m.width = 130;
    m.height = 98;
    m.cfa = truthraw::CfaPattern::BGGR;
    m.orientation = truthraw::Orientation::Normal;
    m.whiteLevel = 1023.0f;
    m.blackPhase = {64.0f, 65.0f, 63.0f, 64.0f};
    m.noiseProfile = {
        0.0012f, 0.0000010f,
        0.0010f, 0.0000008f,
        0.0014f, 0.0000013f,
    };
    m.cameraToXyzD50 = {
        0.92f, 0.05f, 0.03f,
        0.04f, 0.93f, 0.03f,
        0.02f, 0.08f, 0.90f,
    };
    m.hasNoiseProfile = true;
    m.hasGainField = false;
    m.hasResidualBlack = false;
    m.sourceId = "worker-resource-invariance-synthetic-v0.1";

    const std::size_t n = static_cast<std::size_t>(m.width) * static_cast<std::size_t>(m.height);
    frame.raw.resize(n);
    for (int y = 0; y < m.height; ++y) {
        for (int x = 0; x < m.width; ++x) {
            std::uint16_t value = static_cast<std::uint16_t>(
                64 + ((x * 37 + y * 53 + (x * y) % 211) % 900));
            if (((x + 3 * y) % 157) == 0) value = 1023;
            if (((2 * x + y) % 173) == 0) value = 1050;
            frame.raw[static_cast<std::size_t>(y) * static_cast<std::size_t>(m.width) +
                      static_cast<std::size_t>(x)] = value;
        }
    }
    return frame;
}

truthraw::ProcessResult run_case(const truthraw::DecodedDngFrame& frame, int threads) {
    auto reconstruction = std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<truthraw::SkinSafeDetailedCrispAppearance>();
    truthraw::TruthRawProcessor processor(reconstruction, appearance);

    truthraw::ProcessOptions options;
    options.tile = {32, 7};
    options.threads = threads;
    options.hdrEnabled = true;
    options.appearance = truthraw::AppearanceProfile::SkinSafeDetailedCrisp;
    options.keepScientificDiagnostics = true;
    options.sdrLutSize = 4096;

    truthraw::ProcessResult result;
    const auto status = processor.processFrame(frame, options, result);
    require(static_cast<bool>(status),
            "processFrame failed for threads=" + std::to_string(threads) + ": " + status.message);
    require(static_cast<bool>(result.status),
            "result.status failed for threads=" + std::to_string(threads));
    return result;
}

template <class T>
void require_exact_vector(const std::vector<T>& baseline,
                          const std::vector<T>& candidate,
                          const std::string& field,
                          int threads) {
    require(baseline.size() == candidate.size(),
            field + " size changed at threads=" + std::to_string(threads));
    if (!baseline.empty()) {
        const auto bytes = baseline.size() * sizeof(T);
        require(std::memcmp(baseline.data(), candidate.data(), bytes) == 0,
                field + " bytes changed at threads=" + std::to_string(threads));
    }
}

void require_exact_exposure(const truthraw::ExposurePlan& a,
                            const truthraw::ExposurePlan& b,
                            int threads) {
    require(a.anchorsX == b.anchorsX, "anchorsX changed at threads=" + std::to_string(threads));
    require(a.anchorsY == b.anchorsY, "anchorsY changed at threads=" + std::to_string(threads));
    require(a.noiseSigmaAt2Pct == b.noiseSigmaAt2Pct,
            "noiseSigmaAt2Pct changed at threads=" + std::to_string(threads));
    require(a.clipFraction == b.clipFraction,
            "clipFraction changed at threads=" + std::to_string(threads));
    require(a.blackFactor == b.blackFactor,
            "blackFactor changed at threads=" + std::to_string(threads));
    require(a.midGain == b.midGain,
            "midGain changed at threads=" + std::to_string(threads));
    require(a.sdrHighlightGain == b.sdrHighlightGain,
            "sdrHighlightGain changed at threads=" + std::to_string(threads));
    require(a.evidenceConfidence == b.evidenceConfidence,
            "evidenceConfidence changed at threads=" + std::to_string(threads));
    require(a.sceneToDisplayScalar == b.sceneToDisplayScalar,
            "sceneToDisplayScalar changed at threads=" + std::to_string(threads));
    require(a.hdrGateStartY == b.hdrGateStartY,
            "hdrGateStartY changed at threads=" + std::to_string(threads));
    require(a.hdrGateFullY == b.hdrGateFullY,
            "hdrGateFullY changed at threads=" + std::to_string(threads));
    require(a.hdrMaxGain == b.hdrMaxGain,
            "hdrMaxGain changed at threads=" + std::to_string(threads));
    require(a.stage2Over1Count == b.stage2Over1Count,
            "stage2Over1Count changed at threads=" + std::to_string(threads));
}

void require_exact_provenance(const truthraw::Provenance& a,
                              const truthraw::Provenance& b,
                              int threads) {
    const std::string suffix = " at threads=" + std::to_string(threads);
    require(a.scientificMasterModifiedByAppearance == b.scientificMasterModifiedByAppearance,
            "scientificMasterModifiedByAppearance changed" + suffix);
    require(a.gainMapAppliedExactlyOnce == b.gainMapAppliedExactlyOnce,
            "gainMapAppliedExactlyOnce changed" + suffix);
    require(a.censoredHighlightRecoveryClaimed == b.censoredHighlightRecoveryClaimed,
            "censoredHighlightRecoveryClaimed changed" + suffix);
    require(a.scenePlanSharedAcrossLooks == b.scenePlanSharedAcrossLooks,
            "scenePlanSharedAcrossLooks changed" + suffix);
    require(a.perFilePerPhaseBlackUsed == b.perFilePerPhaseBlackUsed,
            "perFilePerPhaseBlackUsed changed" + suffix);
    require(a.residualBlackApplied == b.residualBlackApplied,
            "residualBlackApplied changed" + suffix);
    require(a.reconstructionQuality == b.reconstructionQuality,
            "reconstructionQuality changed" + suffix);
    require(a.reconstructionBackend == b.reconstructionBackend,
            "reconstructionBackend changed" + suffix);
    require(a.appearanceBackend == b.appearanceBackend,
            "appearanceBackend changed" + suffix);
    require(a.residualBlackStatus == b.residualBlackStatus,
            "residualBlackStatus changed" + suffix);
    require(a.colorFidelityPolicy == b.colorFidelityPolicy,
            "colorFidelityPolicy changed" + suffix);
}

void require_finite(const std::vector<float>& values, const std::string& field) {
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (!std::isfinite(values[i])) {
            fail(field + " contains non-finite value at index " + std::to_string(i));
        }
    }
}

std::uint64_t fnv1a64(const std::vector<float>& values) {
    constexpr std::uint64_t offset = 14695981039346656037ull;
    constexpr std::uint64_t prime = 1099511628211ull;
    std::uint64_t h = offset;
    const auto* bytes = reinterpret_cast<const unsigned char*>(values.data());
    const std::size_t n = values.size() * sizeof(float);
    for (std::size_t i = 0; i < n; ++i) {
        h ^= static_cast<std::uint64_t>(bytes[i]);
        h *= prime;
    }
    return h;
}

void require_exact_authoritative_result(const truthraw::ProcessResult& baseline,
                                        const truthraw::ProcessResult& candidate,
                                        int threads) {
    require(baseline.width == candidate.width,
            "width changed at threads=" + std::to_string(threads));
    require(baseline.height == candidate.height,
            "height changed at threads=" + std::to_string(threads));
    require(baseline.orientation == candidate.orientation,
            "orientation changed at threads=" + std::to_string(threads));
    require_exact_exposure(baseline.exposure, candidate.exposure, threads);
    require_exact_provenance(baseline.provenance, candidate.provenance, threads);
    require_exact_vector(baseline.sdrRgb, candidate.sdrRgb, "sdrRgb", threads);
    require_exact_vector(baseline.halfLogGain, candidate.halfLogGain, "halfLogGain", threads);
    require_exact_vector(baseline.stage2Diagnostic, candidate.stage2Diagnostic,
                         "stage2Diagnostic", threads);
}

} // namespace

int main() {
    const auto frame = make_frame();
    const auto baseline = run_case(frame, 1);

    require_finite(baseline.sdrRgb, "baseline.sdrRgb");
    require_finite(baseline.halfLogGain, "baseline.halfLogGain");
    require_finite(baseline.stage2Diagnostic, "baseline.stage2Diagnostic");
    require(!baseline.sdrRgb.empty(), "baseline SDR output is empty");
    require(!baseline.halfLogGain.empty(), "baseline HDR half-gain output is empty");
    require(!baseline.stage2Diagnostic.empty(), "baseline diagnostic output is empty");
    require(baseline.exposure.stage2Over1Count > 0,
            "synthetic frame did not exercise >1 stage2 samples");

    for (const int threads : {2, 4}) {
        const auto candidate = run_case(frame, threads);
        require_exact_authoritative_result(baseline, candidate, threads);
    }

    // Re-run the four-worker case to catch schedule-dependent byte drift.
    for (int repeat = 0; repeat < 3; ++repeat) {
        const auto candidate = run_case(frame, 4);
        require_exact_authoritative_result(baseline, candidate, 4);
    }

    std::cout << "WORKER_RESOURCE_INVARIANCE_V0_1_PASS\n";
    std::cout << "threads_tested=1,2,4\n";
    std::cout << "four_worker_repeats=3\n";
    std::cout << "authoritative_vector_equivalence=EXACT_BYTES\n";
    std::cout << "timing_and_resource_telemetry_excluded_from_identity=1\n";
    std::cout << "stage2_over1_count=" << baseline.exposure.stage2Over1Count << "\n";
    std::cout << std::hex << std::setfill('0');
    std::cout << "sdr_fnv1a64=0x" << std::setw(16) << fnv1a64(baseline.sdrRgb) << "\n";
    std::cout << "half_gain_fnv1a64=0x" << std::setw(16) << fnv1a64(baseline.halfLogGain) << "\n";
    std::cout << "stage2_diag_fnv1a64=0x" << std::setw(16) << fnv1a64(baseline.stage2Diagnostic) << "\n";
    return 0;
}
