#pragma once

#include "free_world_appearance_resolve_v0_7.h"
#include "free_world_light_transport_state_v0_6.h"
#include "open_world_native_v03.h"
#include "truthraw_sha256_v0_69.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

namespace truthraw::open_world::v0_3 {

namespace appearance = truthraw::free_world_appearance_resolve::v0_7;
namespace deep = truthraw::free_world_deep_scene_contribution::v0_4;
namespace light_transport = truthraw::free_world_light_transport_state::v0_6;
namespace room = truthraw::room_capsule::v0_1;
namespace sha = truthraw::sha256_v0_69;

// Stable outside, extensible inside: callers hand one AppearanceInput to this
// corridor. Optional pre-Appearance stages are ordered hooks. A stage may apply,
// take an exact-preserving bypass, or reject fail-closed. New stage types can be
// added without changing the corridor ABI or the scientific source path.
enum class AppearanceStageDecision : std::uint8_t {
    Bypassed = 0u,
    Applied = 1u,
    Rejected = 2u,
};

struct LightTransportAttachment final {
    const light_transport::LightTransportState* state = nullptr;
    sha::Digest expectedParentBoundDeepPacketSha256{};
};

using AppearanceStageHook = AppearanceStageDecision (*)(
    const deep::DeepResolvedPixel& inputScene,
    const LightTransportAttachment* lightTransport,
    deep::DeepResolvedPixel& outputScene,
    const void* stageContext) noexcept;

struct AppearanceStage final {
    const char* stageId = nullptr;
    AppearanceStageHook hook = nullptr;
    const void* context = nullptr;
    bool required = false;
};

struct AppearanceCorridorAudit final {
    std::uint32_t configuredStages = 0u;
    std::uint32_t appliedStages = 0u;
    std::uint32_t bypassedStages = 0u;
    bool lightTransportAttached = false;
    bool exactPreservingBypass = true;
    bool sourceSceneMutated = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

struct AppearanceCorridorEnvelope final {
    bool valid = false;
    Status status = Status::UpstreamRejected;
    appearance::AppearanceResolvedPixel result{};
    AppearanceCorridorAudit audit{};
};

struct RoomCapsuleAppearanceStageContext final {
    room::RoomSampleRuntime sample{};
    room::BoundaryIlluminationEnvelope boundary{};
    std::span<const room::LightState> lights{};
    IlluminationBinding illumination{};
};

namespace detail {

inline bool nonzero_digest(const sha::Digest& digest) noexcept {
    return std::any_of(
        digest.begin(), digest.end(),
        [](std::uint8_t value) { return value != 0u; });
}

inline bool same_scene(
    const deep::DeepResolvedPixel& a,
    const deep::DeepResolvedPixel& b) noexcept {
    return a.sceneLinearRgb == b.sceneLinearRgb &&
           a.channelAuthority == b.channelAuthority &&
           a.uncertaintyKnown == b.uncertaintyKnown &&
           a.p95Uncertainty == b.p95Uncertainty &&
           a.visibility.evidenceWeight == b.visibility.evidenceWeight &&
           a.visibility.inferredWeight == b.visibility.inferredWeight &&
           a.visibility.restorationWeight == b.visibility.restorationWeight &&
           a.visibility.counterfactualWeight == b.visibility.counterfactualWeight &&
           a.visibility.residualTransmittance == b.visibility.residualTransmittance &&
           a.visibility.containsInferred == b.visibility.containsInferred &&
           a.visibility.containsRestorationHypothesis ==
               b.visibility.containsRestorationHypothesis &&
           a.visibility.containsCounterfactual ==
               b.visibility.containsCounterfactual &&
           a.visibility.scientificObservation ==
               b.visibility.scientificObservation &&
           a.visibility.createsNewEvidence == b.visibility.createsNewEvidence &&
           a.visibility.scientificWritebackAllowed ==
               b.visibility.scientificWritebackAllowed &&
           a.sourcePacketSha256 == b.sourcePacketSha256 &&
           a.resolveMethodId == b.resolveMethodId &&
           a.view == b.view &&
           a.appearanceApplied == b.appearanceApplied &&
           a.displayEncoded == b.displayEncoded &&
           a.createsNewEvidence == b.createsNewEvidence &&
           a.scientificWritebackAllowed == b.scientificWritebackAllowed &&
           a.physicalFrameCount == b.physicalFrameCount &&
           a.independentEvidenceCount == b.independentEvidenceCount;
}

inline bool validate_light_transport_attachment(
    const LightTransportAttachment& attachment) noexcept {
    if (attachment.state == nullptr ||
        !attachment.state->finalized ||
        attachment.state->createsNewEvidence ||
        attachment.state->scientificWritebackAllowed ||
        !nonzero_digest(attachment.state->stateSha256) ||
        !nonzero_digest(attachment.expectedParentBoundDeepPacketSha256)) {
        return false;
    }
    return attachment.state->parentBoundDeepPacketSha256 ==
           attachment.expectedParentBoundDeepPacketSha256;
}

inline bool authority_matches(
    light_transport::ParameterAuthority stateAuthority,
    IlluminationAuthority bindingAuthority) noexcept {
    switch (stateAuthority) {
        case light_transport::ParameterAuthority::Measured:
            return bindingAuthority == IlluminationAuthority::Measured;
        case light_transport::ParameterAuthority::CalibratedEstimate:
            return bindingAuthority == IlluminationAuthority::CalibratedEstimate;
        case light_transport::ParameterAuthority::Inferred:
            return bindingAuthority == IlluminationAuthority::Inferred;
        case light_transport::ParameterAuthority::Counterfactual:
            return bindingAuthority == IlluminationAuthority::Counterfactual;
        case light_transport::ParameterAuthority::Hypothetical:
        case light_transport::ParameterAuthority::Unknown:
            return false;
    }
    return false;
}

inline bool close_enough(double a, float b) noexcept {
    return std::isfinite(a) && std::isfinite(static_cast<double>(b)) &&
           std::abs(a - static_cast<double>(b)) <= 1.0e-5;
}

inline void hash_u64(sha::Hasher& hasher, std::uint64_t value) noexcept {
    std::array<std::uint8_t, 8u> bytes{};
    for (std::size_t i = 0u; i < bytes.size(); ++i) {
        bytes[i] = static_cast<std::uint8_t>(value >> (8u * i));
    }
    hasher.update(bytes);
}

inline void hash_f64(sha::Hasher& hasher, double value) noexcept {
    hash_u64(hasher, std::bit_cast<std::uint64_t>(value));
}

inline sha::Digest room_stage_digest(
    const deep::DeepResolvedPixel& source,
    const light_transport::LightTransportState& transport,
    const RoomCapsuleEnvelope& roomEnvelope,
    const std::array<double, 3u>& transformedRgb) noexcept {
    sha::Hasher hasher;
    constexpr char domain[] =
        "D_RAW_OPEN_WORLD_ROOM_CAPSULE_APPEARANCE_STAGE_V0_3";
    hasher.update(
        reinterpret_cast<const std::uint8_t*>(domain),
        sizeof(domain) - 1u);
    hasher.update(source.sourcePacketSha256);
    hasher.update(transport.stateSha256);
    for (float multiplier : roomEnvelope.result.relativeMultiplier) {
        hash_f64(hasher, static_cast<double>(multiplier));
    }
    for (double value : transformedRgb) hash_f64(hasher, value);
    return hasher.finalize();
}

}  // namespace detail

inline AppearanceStageDecision room_capsule_appearance_stage(
    const deep::DeepResolvedPixel& inputScene,
    const LightTransportAttachment* lightTransport,
    deep::DeepResolvedPixel& outputScene,
    const void* stageContext) noexcept {
    outputScene = inputScene;

    // No authoritative v0.6 state means neutral bypass, never guessed geometry
    // or illumination. This is the normal fail-safe state until such a binding
    // is actually available from the upstream scene route.
    if (lightTransport == nullptr || lightTransport->state == nullptr) {
        return AppearanceStageDecision::Bypassed;
    }
    if (stageContext == nullptr ||
        !detail::validate_light_transport_attachment(*lightTransport)) {
        return AppearanceStageDecision::Rejected;
    }

    const auto& transport = *lightTransport->state;
    const auto& config =
        *static_cast<const RoomCapsuleAppearanceStageContext*>(stageContext);

    if (!detail::authority_matches(
            transport.illumination.authority,
            config.illumination.authority) ||
        !detail::close_enough(transport.surface.normal.x, config.sample.normal[0]) ||
        !detail::close_enough(transport.surface.normal.y, config.sample.normal[1]) ||
        !detail::close_enough(transport.surface.normal.z, config.sample.normal[2]) ||
        !detail::close_enough(transport.visibility, config.sample.visibility)) {
        return AppearanceStageDecision::Rejected;
    }

    RoomCapsuleEnvelope envelope{};
    const auto status = evaluate_room_capsule_relative(
        config.sample,
        config.boundary,
        config.lights,
        config.illumination,
        envelope);
    if (status != Status::Ok || !envelope.valid ||
        envelope.authority.mayModifyScientificMaster ||
        envelope.authority.independentEvidenceAdded ||
        envelope.result.ledger.scientificMasterModified ||
        envelope.result.ledger.zeroLineModified ||
        envelope.result.ledger.lightStatesAreEvidence) {
        return AppearanceStageDecision::Rejected;
    }

    if (envelope.upstreamStatus == room::Status::OutsideRoom) {
        return AppearanceStageDecision::Bypassed;
    }

    bool changed = false;
    for (std::size_t channel = 0u; channel < 3u; ++channel) {
        const double multiplier =
            static_cast<double>(envelope.result.relativeMultiplier[channel]);
        if (!std::isfinite(multiplier) || multiplier < 0.0) {
            return AppearanceStageDecision::Rejected;
        }
        const double value = inputScene.sceneLinearRgb[channel] * multiplier;
        if (!std::isfinite(value)) {
            return AppearanceStageDecision::Rejected;
        }
        outputScene.sceneLinearRgb[channel] = value;
        changed = changed || value != inputScene.sceneLinearRgb[channel];
    }

    if (!changed) {
        outputScene = inputScene;
        return AppearanceStageDecision::Bypassed;
    }

    // The transformed branch is explicitly appearance/counterfactual. It may
    // inherit lineage from the one real frame, but never measured authority,
    // scientific uncertainty, evidence weight, or writeback permission.
    outputScene.channelAuthority.fill(
        truthraw::free_world_pixel_resolve_2d::v0_2::
            ResolvedAuthority::Unknown);
    outputScene.uncertaintyKnown.fill(false);
    outputScene.p95Uncertainty.fill(0.0);
    outputScene.visibility = {};
    outputScene.view = deep::ResolveView::CounterfactualRender;
    outputScene.appearanceApplied = false;
    outputScene.displayEncoded = false;
    outputScene.createsNewEvidence = false;
    outputScene.scientificWritebackAllowed = false;
    outputScene.sourcePacketSha256 = detail::room_stage_digest(
        inputScene,
        transport,
        envelope,
        outputScene.sceneLinearRgb);

    if (!detail::nonzero_digest(outputScene.sourcePacketSha256)) {
        outputScene = inputScene;
        return AppearanceStageDecision::Rejected;
    }
    return AppearanceStageDecision::Applied;
}

inline Status resolve_appearance_corridor(
    const appearance::AppearanceInput& input,
    const LightTransportAttachment* lightTransport,
    std::span<const AppearanceStage> stages,
    AppearanceCorridorEnvelope& out) noexcept {
    out = AppearanceCorridorEnvelope{};
    try {
        if (!appearance::validateInput(input) ||
            input.scene.createsNewEvidence ||
            input.scene.scientificWritebackAllowed) {
            out.status = Status::UpstreamRejected;
            return out.status;
        }
        if (lightTransport != nullptr &&
            !detail::validate_light_transport_attachment(*lightTransport)) {
            out.status = Status::InvalidAuthorityBinding;
            return out.status;
        }

        out.audit.configuredStages =
            static_cast<std::uint32_t>(stages.size());
        out.audit.lightTransportAttached = lightTransport != nullptr;

        deep::DeepResolvedPixel current = input.scene;
        for (const auto& stage : stages) {
            if (stage.stageId == nullptr || stage.stageId[0] == '\0' ||
                stage.hook == nullptr) {
                out.status = Status::InvalidAuthorityBinding;
                return out.status;
            }

            deep::DeepResolvedPixel next = current;
            const auto decision =
                stage.hook(current, lightTransport, next, stage.context);

            if (decision == AppearanceStageDecision::Rejected) {
                out.status = Status::UpstreamRejected;
                return out.status;
            }
            if (decision == AppearanceStageDecision::Bypassed) {
                if (stage.required || !detail::same_scene(current, next)) {
                    out.status = Status::UpstreamRejected;
                    return out.status;
                }
                ++out.audit.bypassedStages;
                continue;
            }

            if (next.createsNewEvidence ||
                next.scientificWritebackAllowed ||
                next.appearanceApplied ||
                next.displayEncoded ||
                next.physicalFrameCount != current.physicalFrameCount ||
                next.independentEvidenceCount !=
                    current.independentEvidenceCount) {
                out.status = Status::UpstreamRejected;
                return out.status;
            }
            ++out.audit.appliedStages;
            out.audit.exactPreservingBypass = false;
            current = std::move(next);
        }

        appearance::AppearanceInput resolvedInput = input;
        resolvedInput.scene = current;
        if (!appearance::resolveAppearance(resolvedInput, out.result) ||
            out.result.sourceSceneSha256 != current.sourcePacketSha256 ||
            out.result.sourceSceneMutated ||
            out.result.createsNewEvidence ||
            out.result.scientificWritebackAllowed ||
            !out.result.appearanceApplied ||
            !out.result.displayEncoded) {
            out = AppearanceCorridorEnvelope{};
            out.status = Status::UpstreamRejected;
            return out.status;
        }

        out.audit.sourceSceneMutated = false;
        out.audit.createsNewEvidence = false;
        out.audit.scientificWritebackAllowed = false;
        out.valid = true;
        out.status = Status::Ok;
        return Status::Ok;
    } catch (...) {
        out = AppearanceCorridorEnvelope{};
        out.status = Status::UpstreamRejected;
        return out.status;
    }
}

}  // namespace truthraw::open_world::v0_3
