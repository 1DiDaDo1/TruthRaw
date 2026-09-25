#include "free_world_deep_scene_contribution_v0_4.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace truthraw::free_world_deep_scene_contribution::v0_4 {
namespace {

bool finite(double v) noexcept {
    return std::isfinite(v);
}

bool finiteNonNegative(double v) noexcept {
    return std::isfinite(v) && v >= 0.0;
}

bool validAuthority(free_world::ResolvedAuthority a) noexcept {
    switch (a) {
        case free_world::ResolvedAuthority::Reconstructed:
        case free_world::ResolvedAuthority::Censored:
        case free_world::ResolvedAuthority::Unknown:
            return true;
    }
    return false;
}

void hashU32(truthraw::sha256_v0_69::Hasher& h, std::uint32_t v) noexcept {
    const std::array<std::uint8_t, 4u> b{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v >> 8u),
        static_cast<std::uint8_t>(v >> 16u),
        static_cast<std::uint8_t>(v >> 24u)};
    h.update(b);
}

void hashU64(truthraw::sha256_v0_69::Hasher& h, std::uint64_t v) noexcept {
    std::array<std::uint8_t, 8u> b{};
    for (unsigned i = 0u; i < 8u; ++i) {
        b[i] = static_cast<std::uint8_t>(v >> (8u * i));
    }
    h.update(b);
}

void hashF64(truthraw::sha256_v0_69::Hasher& h, double v) noexcept {
    hashU64(h, std::bit_cast<std::uint64_t>(v));
}

bool admitted(ContributionClass c, ResolveView view) noexcept {
    switch (view) {
        case ResolveView::ScientificView:
            return c == ContributionClass::EvidenceConstrained;
        case ResolveView::OpenSceneView:
            return c == ContributionClass::EvidenceConstrained ||
                   c == ContributionClass::InferredScene;
        case ResolveView::RestorationView:
            return c == ContributionClass::EvidenceConstrained ||
                   c == ContributionClass::InferredScene ||
                   c == ContributionClass::RestorationHypothesis;
        case ResolveView::CounterfactualRender:
            return true;
    }
    return false;
}

void accumulateClass(
    VisibleContributionSummary& summary,
    ContributionClass c,
    double visibleWeight) noexcept {
    switch (c) {
        case ContributionClass::EvidenceConstrained:
            summary.evidenceWeight += visibleWeight;
            break;
        case ContributionClass::InferredScene:
            summary.inferredWeight += visibleWeight;
            summary.containsInferred = true;
            break;
        case ContributionClass::RestorationHypothesis:
            summary.restorationWeight += visibleWeight;
            summary.containsRestorationHypothesis = true;
            break;
        case ContributionClass::CounterfactualScene:
            summary.counterfactualWeight += visibleWeight;
            summary.containsCounterfactual = true;
            break;
    }
}

free_world::ResolvedAuthority combineAuthority(
    free_world::ResolvedAuthority current,
    free_world::ResolvedAuthority next,
    bool first) noexcept {
    if (first) return next;
    if (current == free_world::ResolvedAuthority::Unknown ||
        next == free_world::ResolvedAuthority::Unknown) {
        return free_world::ResolvedAuthority::Unknown;
    }
    if (current == free_world::ResolvedAuthority::Censored ||
        next == free_world::ResolvedAuthority::Censored) {
        return free_world::ResolvedAuthority::Censored;
    }
    return free_world::ResolvedAuthority::Reconstructed;
}

}  // namespace

bool validateSample(const DeepSample& sample) noexcept {
    if (!finiteNonNegative(sample.zFront) ||
        !finiteNonNegative(sample.zBack) ||
        sample.zBack < sample.zFront ||
        !finite(sample.opacity) ||
        sample.opacity < 0.0 || sample.opacity > 1.0 ||
        sample.provenanceId == 0u) {
        return false;
    }

    const auto cls = static_cast<std::uint8_t>(sample.contributionClass);
    if (cls < 1u || cls > 4u) return false;

    for (std::size_t c = 0u; c < 3u; ++c) {
        if (!finite(sample.sceneLinearRgb[c]) ||
            !validAuthority(sample.channelAuthority[c])) {
            return false;
        }
        if (sample.uncertaintyKnown[c]) {
            if (!finiteNonNegative(sample.p95Uncertainty[c])) return false;
        } else if (sample.p95Uncertainty[c] != 0.0) {
            return false;
        }

        if (sample.contributionClass !=
                ContributionClass::EvidenceConstrained &&
            sample.channelAuthority[c] !=
                free_world::ResolvedAuthority::Unknown) {
            return false;
        }
    }
    return true;
}

bool makeCameraPlaneContribution(
    const free_world::ResolvedPixel& pixel,
    double depth,
    std::uint64_t provenanceId,
    DeepSample& out) noexcept {
    out = DeepSample{};
    if (!finiteNonNegative(depth) || provenanceId == 0u ||
        pixel.createsNewEvidence ||
        pixel.measuredTargetClaimCount != 0u ||
        pixel.physicalFrameCount != 1u ||
        pixel.independentEvidenceCount != 1u) {
        return false;
    }

    out.zFront = depth;
    out.zBack = depth;
    out.sceneLinearRgb = pixel.sceneLinear;
    out.opacity = 1.0;
    out.contributionClass = ContributionClass::EvidenceConstrained;
    out.provenanceId = provenanceId;

    for (std::size_t c = 0u; c < 3u; ++c) {
        out.channelAuthority[c] = pixel.support[c].authority;
        out.uncertaintyKnown[c] = pixel.support[c].uncertaintyKnown;
        out.p95Uncertainty[c] =
            pixel.support[c].uncertaintyKnown
                ? pixel.support[c].p95Uncertainty
                : 0.0;
    }
    return validateSample(out);
}

bool finalizePacket(DeepPixelPacket& packet) noexcept {
    try {
        if (packet.finalized || packet.samples.empty() ||
            packet.physicalFrameCount != 1u ||
            packet.independentEvidenceCount != 1u ||
            packet.createsNewEvidence ||
            packet.scientificWritebackAllowed) {
            return false;
        }

        for (std::size_t i = 0u; i < packet.samples.size(); ++i) {
            packet.samples[i].insertionOrdinal =
                static_cast<std::uint32_t>(i);
            if (!validateSample(packet.samples[i])) return false;
        }

        std::stable_sort(
            packet.samples.begin(),
            packet.samples.end(),
            [](const DeepSample& a, const DeepSample& b) {
                if (a.zFront != b.zFront) return a.zFront < b.zFront;
                if (a.zBack != b.zBack) return a.zBack < b.zBack;
                return a.insertionOrdinal < b.insertionOrdinal;
            });

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[] =
            "D_RAW_FREE_WORLD_DEEP_SCENE_PACKET_V0_4";
        h.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain) - 1u);
        hashU32(h, packet.physicalFrameCount);
        hashU32(h, packet.independentEvidenceCount);
        hashU64(h, static_cast<std::uint64_t>(packet.samples.size()));

        for (const auto& sample : packet.samples) {
            hashF64(h, sample.zFront);
            hashF64(h, sample.zBack);
            for (double v : sample.sceneLinearRgb) hashF64(h, v);
            hashF64(h, sample.opacity);
            const std::uint8_t cls =
                static_cast<std::uint8_t>(sample.contributionClass);
            h.update(&cls, 1u);
            for (auto a : sample.channelAuthority) {
                const std::uint8_t av = static_cast<std::uint8_t>(a);
                h.update(&av, 1u);
            }
            for (bool known : sample.uncertaintyKnown) {
                const std::uint8_t kv = known ? 1u : 0u;
                h.update(&kv, 1u);
            }
            for (double u : sample.p95Uncertainty) hashF64(h, u);
            hashU64(h, sample.provenanceId);
            hashU32(h, sample.insertionOrdinal);
        }

        packet.packetSha256 = h.finalize();
        packet.finalized = true;
        return true;
    } catch (...) {
        packet.packetSha256 = {};
        packet.finalized = false;
        return false;
    }
}

bool resolve(
    const DeepPixelPacket& packet,
    ResolveView view,
    DeepResolvedPixel& out) noexcept {
    out = DeepResolvedPixel{};
    try {
        if (!packet.finalized || packet.samples.empty() ||
            packet.createsNewEvidence ||
            packet.scientificWritebackAllowed ||
            packet.physicalFrameCount != 1u ||
            packet.independentEvidenceCount != 1u) {
            return false;
        }

        const auto vv = static_cast<std::uint8_t>(view);
        if (vv < 1u || vv > 4u) return false;

        double transmittance = 1.0;
        std::array<bool, 3u> firstAuthority{true, true, true};
        std::array<bool, 3u> uncertaintyKnown{true, true, true};
        std::array<double, 3u> uncertaintyAccum{0.0, 0.0, 0.0};
        bool sawAdmitted = false;

        for (const auto& sample : packet.samples) {
            if (!validateSample(sample) ||
                !admitted(sample.contributionClass, view)) {
                continue;
            }

            sawAdmitted = true;
            const double visibleWeight =
                transmittance * sample.opacity;
            if (visibleWeight > 0.0) {
                accumulateClass(
                    out.visibility,
                    sample.contributionClass,
                    visibleWeight);

                for (std::size_t c = 0u; c < 3u; ++c) {
                    out.sceneLinearRgb[c] +=
                        visibleWeight * sample.sceneLinearRgb[c];

                    if (sample.contributionClass ==
                        ContributionClass::EvidenceConstrained) {
                        out.channelAuthority[c] = combineAuthority(
                            out.channelAuthority[c],
                            sample.channelAuthority[c],
                            firstAuthority[c]);
                        firstAuthority[c] = false;
                    } else {
                        out.channelAuthority[c] =
                            free_world::ResolvedAuthority::Unknown;
                        firstAuthority[c] = false;
                    }

                    if (!sample.uncertaintyKnown[c] ||
                        sample.contributionClass !=
                            ContributionClass::EvidenceConstrained) {
                        uncertaintyKnown[c] = false;
                    } else {
                        uncertaintyAccum[c] +=
                            visibleWeight * sample.p95Uncertainty[c];
                    }
                }
            }

            transmittance *= (1.0 - sample.opacity);
            if (transmittance <= 0.0) {
                transmittance = 0.0;
                break;
            }
        }

        if (!sawAdmitted) return false;

        out.visibility.residualTransmittance = transmittance;
        out.visibility.scientificObservation =
            view == ResolveView::ScientificView &&
            !out.visibility.containsInferred &&
            !out.visibility.containsRestorationHypothesis &&
            !out.visibility.containsCounterfactual;

        for (std::size_t c = 0u; c < 3u; ++c) {
            if (firstAuthority[c]) {
                out.channelAuthority[c] =
                    free_world::ResolvedAuthority::Unknown;
                uncertaintyKnown[c] = false;
            }
            out.uncertaintyKnown[c] = uncertaintyKnown[c];
            out.p95Uncertainty[c] =
                uncertaintyKnown[c] ? uncertaintyAccum[c] : 0.0;
        }

        out.sourcePacketSha256 = packet.packetSha256;
        out.view = view;
        out.appearanceApplied = false;
        out.displayEncoded = false;
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;
        out.physicalFrameCount = 1u;
        out.independentEvidenceCount = 1u;
        return true;
    } catch (...) {
        out = DeepResolvedPixel{};
        return false;
    }
}

const char* toString(ContributionClass contributionClass) noexcept {
    switch (contributionClass) {
        case ContributionClass::EvidenceConstrained:
            return "EVIDENCE_CONSTRAINED";
        case ContributionClass::InferredScene:
            return "INFERRED_SCENE";
        case ContributionClass::RestorationHypothesis:
            return "RESTORATION_HYPOTHESIS";
        case ContributionClass::CounterfactualScene:
            return "COUNTERFACTUAL_SCENE";
    }
    return "INVALID";
}

const char* toString(ResolveView view) noexcept {
    switch (view) {
        case ResolveView::ScientificView: return "SCIENTIFIC_VIEW";
        case ResolveView::OpenSceneView: return "OPEN_SCENE_VIEW";
        case ResolveView::RestorationView: return "RESTORATION_VIEW";
        case ResolveView::CounterfactualRender:
            return "COUNTERFACTUAL_RENDER";
    }
    return "INVALID";
}

}  // namespace truthraw::free_world_deep_scene_contribution::v0_4
