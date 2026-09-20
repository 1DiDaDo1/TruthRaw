#include "truthraw/core.h"

#include <bit>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

std::uint64_t fnv1a64(std::uint64_t h, const void* data, std::size_t size) {
    const auto* p = static_cast<const std::uint8_t*>(data);
    for (std::size_t i = 0; i < size; ++i) {
        h ^= p[i];
        h *= 1099511628211ull;
    }
    return h;
}

template <class T>
void hash_value(std::uint64_t& h, const T& value) {
    h = fnv1a64(h, &value, sizeof(value));
}

void hash_floats(std::uint64_t& h, const std::vector<float>& values) {
    for (float value : values) {
        const auto bits = std::bit_cast<std::uint32_t>(value);
        hash_value(h, bits);
    }
}

truthraw::DecodedDngFrame make_frame() {
    truthraw::DecodedDngFrame frame;
    frame.meta.width = 130;
    frame.meta.height = 98;
    frame.meta.cfa = truthraw::CfaPattern::BGGR;
    frame.meta.orientation = truthraw::Orientation::Normal;
    frame.meta.whiteLevel = 1023.0f;
    frame.meta.blackPhase = {64.0f, 65.0f, 66.0f, 67.0f};
    frame.meta.noiseProfile =
        {0.0009f, 1.0e-6f, 0.0010f, 1.2e-6f, 0.0011f, 1.4e-6f};
    frame.meta.hasNoiseProfile = true;
    frame.meta.hasGainField = false;
    frame.meta.hasResidualBlack = false;
    frame.meta.cameraToXyzD50 = {
        0.62f, 0.21f, 0.08f,
        0.18f, 0.71f, 0.07f,
        0.03f, 0.12f, 0.79f,
    };
    frame.meta.sourceId = "truthraw-compiler-equivalence-v0.1";

    const std::size_t pixels =
        static_cast<std::size_t>(frame.meta.width) *
        static_cast<std::size_t>(frame.meta.height);
    frame.raw.resize(pixels);

    for (int y = 0; y < frame.meta.height; ++y) {
        for (int x = 0; x < frame.meta.width; ++x) {
            std::uint32_t v =
                68u +
                static_cast<std::uint32_t>(
                    (x * 37 + y * 53 + x * y * 3 + (x ^ y) * 11) % 920);
            if (((x * 17 + y * 29) % 251) == 0) v = 1023u;
            frame.raw[
                static_cast<std::size_t>(y) *
                    static_cast<std::size_t>(frame.meta.width) +
                static_cast<std::size_t>(x)] =
                static_cast<std::uint16_t>(v);
        }
    }
    return frame;
}

} // namespace

int main() {
    using namespace truthraw;

    auto reconstruction =
        std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance =
        std::make_shared<SkinSafeDetailedCrispAppearance>();

    ProcessOptions options;
    options.tile = {32, 8};
    options.threads = 1;
    options.hdrEnabled = true;
    options.appearance = AppearanceProfile::SkinSafeDetailedCrisp;
    options.keepScientificDiagnostics = true;
    options.sdrLutSize = 4096;

    ProcessResult result;
    TruthRawProcessor processor(reconstruction, appearance);
    const auto status = processor.processFrame(make_frame(), options, result);
    if (!status) {
        std::cerr << "process failed: " << status.message << "\n";
        return 2;
    }

    std::uint64_t hash = 1469598103934665603ull;
    hash_value(hash, result.width);
    hash_value(hash, result.height);
    hash_value(hash, result.orientation);
    hash_floats(hash, result.sdrRgb);
    hash_floats(hash, result.halfLogGain);
    hash_floats(hash, result.stage2Diagnostic);

    hash_value(hash, std::bit_cast<std::uint32_t>(result.exposure.noiseSigmaAt2Pct));
    hash_value(hash, std::bit_cast<std::uint32_t>(result.exposure.clipFraction));
    hash_value(hash, std::bit_cast<std::uint32_t>(result.exposure.blackFactor));
    hash_value(hash, std::bit_cast<std::uint32_t>(result.exposure.midGain));
    hash_value(hash, std::bit_cast<std::uint32_t>(result.exposure.sdrHighlightGain));
    hash_value(hash, std::bit_cast<std::uint32_t>(result.exposure.sceneToDisplayScalar));
    hash_value(hash, result.exposure.stage2Over1Count);

    std::cout << std::hex << std::setfill('0') << std::setw(16) << hash << "\n";
    return 0;
}
