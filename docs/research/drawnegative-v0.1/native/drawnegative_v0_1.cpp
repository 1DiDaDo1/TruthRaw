#include "drawnegative_v0_1.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace truthraw::drawnegative::v0_1 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(d.begin(), d.end(), [](std::uint8_t v) {
        return v != 0u;
    });
}

void hashU8(truthraw::sha256_v0_69::Hasher& h, std::uint8_t v) noexcept {
    h.update(&v, 1u);
}

void hashU64(truthraw::sha256_v0_69::Hasher& h, std::uint64_t v) noexcept {
    std::array<std::uint8_t, 8u> b{};
    for (std::size_t i = 0u; i < b.size(); ++i) {
        b[i] = static_cast<std::uint8_t>(v >> (8u * i));
    }
    h.update(b);
}

void hashString(
    truthraw::sha256_v0_69::Hasher& h,
    const std::string& s) noexcept {
    hashU64(h, static_cast<std::uint64_t>(s.size()));
    h.update(
        reinterpret_cast<const std::uint8_t*>(s.data()),
        s.size());
}

bool validLegacyState(const legacy_tn::State& s) noexcept {
    return s.finalized &&
           nonzero(s.stateSha256) &&
           s.isRasterIndependent &&
           !s.createsNewEvidence &&
           !s.scientificWritebackAllowed &&
           s.input.physicalFrameCount == 1u &&
           s.input.independentEvidenceCount == 1u &&
           nonzero(s.input.sourceEvidenceSha256) &&
           nonzero(s.input.scientificMasterSha256) &&
           nonzero(s.input.authorityFieldSha256);
}

bool validGauge(const Input& input) noexcept {
    if (input.scaleGaugeId.empty()) return false;

    switch (input.gaugeRelation) {
        case GaugeRelation::SourceLocalOnly:
            return input.sharedFreeWorldGaugeId.empty();
        case GaugeRelation::SharedRelative:
        case GaugeRelation::SharedAbsolute:
            return !input.sharedFreeWorldGaugeId.empty();
    }
    return false;
}

} // namespace

bool finalize(const Input& input, State& out) noexcept {
    out = State{};
    try {
        if (!validLegacyState(input.truthNegativeState) ||
            input.observationId.empty() ||
            !validGauge(input) ||
            !input.truthRangeCoordinateFamilyDeclared) {
            return false;
        }

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[] =
            "D_RAW_NEGATIVE_OBSERVATION_BOUND_STATE_V0_1";
        h.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain) - 1u);
        h.update(input.truthNegativeState.stateSha256);
        hashString(h, input.observationId);
        hashString(h, input.scaleGaugeId);
        hashString(h, input.sharedFreeWorldGaugeId);
        hashU8(h, static_cast<std::uint8_t>(input.gaugeRelation));
        hashU8(h, static_cast<std::uint8_t>(input.canonicalStorage));
        hashU8(h, input.truthRangeCoordinateFamilyDeclared ? 1u : 0u);
        hashU8(h, input.perSampleTruthRangeMaterialized ? 1u : 0u);

        out.stateSha256 = h.finalize();
        if (!nonzero(out.stateSha256)) {
            out = State{};
            return false;
        }

        out.parentTruthNegativeStateSha256 =
            input.truthNegativeState.stateSha256;
        out.observationId = input.observationId;
        out.scaleGaugeId = input.scaleGaugeId;
        out.sharedFreeWorldGaugeId = input.sharedFreeWorldGaugeId;
        out.gaugeRelation = input.gaugeRelation;
        out.canonicalStorage = input.canonicalStorage;
        out.finalized = true;
        out.isRasterIndependent = true;
        out.isPerObservationLineage = true;
        out.commonGaugeAdmitted =
            input.gaugeRelation != GaugeRelation::SourceLocalOnly;
        out.crossObservationRadiometricEqualityAllowed =
            out.commonGaugeAdmitted;
        out.crossObservationRadiometricFusionAllowed =
            out.commonGaugeAdmitted;
        out.truthRangeCoordinateFamilyDeclared =
            input.truthRangeCoordinateFamilyDeclared;
        out.perSampleTruthRangeMaterialized =
            input.perSampleTruthRangeMaterialized;
        out.branchSensitiveComputeFloat64 = true;
        out.precisionUpgradesAuthority = false;
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;
        out.methodId = kStateMethod;
        return true;
    } catch (...) {
        out = State{};
        return false;
    }
}

const char* schema_name() noexcept {
    return kSchemaName;
}

} // namespace truthraw::drawnegative::v0_1
