#pragma once

#include "free_world_pixel_resolve_2d_v0_2.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace truthraw::free_world_deep_scene_contribution::v0_4 {

namespace free_world = truthraw::free_world_pixel_resolve_2d::v0_2;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "FreeWorldDeepSceneContribution/0.4";
inline constexpr const char* kResolveMethodId =
    "ORDERED_TRANSMITTANCE_SCENE_LINEAR_F64_V0_4";

enum class ContributionClass : std::uint8_t {
    EvidenceConstrained = 1u,
    InferredScene = 2u,
    RestorationHypothesis = 3u,
    CounterfactualScene = 4u,
};

enum class ResolveView : std::uint8_t {
    ScientificView = 1u,
    OpenSceneView = 2u,
    RestorationView = 3u,
    CounterfactualRender = 4u,
};

struct DeepSample final {
    double zFront = 0.0;
    double zBack = 0.0;
    std::array<double, 3u> sceneLinearRgb{};
    double opacity = 0.0;

    ContributionClass contributionClass =
        ContributionClass::EvidenceConstrained;
    std::array<free_world::ResolvedAuthority, 3u> channelAuthority{
        free_world::ResolvedAuthority::Unknown,
        free_world::ResolvedAuthority::Unknown,
        free_world::ResolvedAuthority::Unknown};

    std::array<bool, 3u> uncertaintyKnown{};
    std::array<double, 3u> p95Uncertainty{};

    std::uint64_t provenanceId = 0u;
    std::uint32_t insertionOrdinal = 0u;
};

struct DeepPixelPacket final {
    std::vector<DeepSample> samples;
    Digest packetSha256{};
    bool finalized = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;
};

struct VisibleContributionSummary final {
    double evidenceWeight = 0.0;
    double inferredWeight = 0.0;
    double restorationWeight = 0.0;
    double counterfactualWeight = 0.0;
    double residualTransmittance = 1.0;

    bool containsInferred = false;
    bool containsRestorationHypothesis = false;
    bool containsCounterfactual = false;
    bool scientificObservation = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

struct DeepResolvedPixel final {
    std::array<double, 3u> sceneLinearRgb{};
    std::array<free_world::ResolvedAuthority, 3u> channelAuthority{
        free_world::ResolvedAuthority::Unknown,
        free_world::ResolvedAuthority::Unknown,
        free_world::ResolvedAuthority::Unknown};
    std::array<bool, 3u> uncertaintyKnown{};
    std::array<double, 3u> p95Uncertainty{};

    VisibleContributionSummary visibility{};
    Digest sourcePacketSha256{};
    std::string resolveMethodId = kResolveMethodId;
    ResolveView view = ResolveView::ScientificView;

    bool appearanceApplied = false;
    bool displayEncoded = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;
};

bool validateSample(const DeepSample& sample) noexcept;

bool makeCameraPlaneContribution(
    const free_world::ResolvedPixel& pixel,
    double depth,
    std::uint64_t provenanceId,
    DeepSample& out) noexcept;

bool finalizePacket(DeepPixelPacket& packet) noexcept;

bool resolve(
    const DeepPixelPacket& packet,
    ResolveView view,
    DeepResolvedPixel& out) noexcept;

const char* toString(ContributionClass contributionClass) noexcept;
const char* toString(ResolveView view) noexcept;

}  // namespace truthraw::free_world_deep_scene_contribution::v0_4
