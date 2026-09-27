#include "free_world_observation_graph_v0_1.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace truthraw::free_world_observation_graph::v0_1 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(d.begin(), d.end(), [](std::uint8_t v) {
        return v != 0u;
    });
}

void hashU8(truthraw::sha256_v0_69::Hasher& h, std::uint8_t v) noexcept {
    h.update(&v, 1u);
}

void hashU32(truthraw::sha256_v0_69::Hasher& h, std::uint32_t v) noexcept {
    std::array<std::uint8_t, 4u> b{};
    for (std::size_t i = 0u; i < b.size(); ++i) {
        b[i] = static_cast<std::uint8_t>(v >> (8u * i));
    }
    h.update(b);
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
    const std::string& value) noexcept {
    hashU64(h, static_cast<std::uint64_t>(value.size()));
    h.update(
        reinterpret_cast<const std::uint8_t*>(value.data()),
        value.size());
}

bool validTemporal(const ObservationNodeInput& input) noexcept {
    switch (input.temporalModel) {
        case TemporalModel::Unknown:
        case TemporalModel::CaptureTimestampOnly:
            return !input.exposureIntervalKnown &&
                   !input.rollingShutterModelKnown;
        case TemporalModel::ExposureInterval:
        case TemporalModel::GlobalShutterInterval:
            return input.exposureIntervalKnown &&
                   !input.rollingShutterModelKnown;
        case TemporalModel::RollingShutterModel:
            return input.exposureIntervalKnown &&
                   input.rollingShutterModelKnown;
    }
    return false;
}

bool validPlacement(const InformationRecordInput& input) noexcept {
    if (input.domain == InformationDomain::SourceEvidence) {
        return input.floor == KnowledgeFloor::Evidence;
    }
    if (input.domain == InformationDomain::Appearance) {
        return input.floor == KnowledgeFloor::Appearance;
    }
    if (input.floor == KnowledgeFloor::Evidence) {
        return false;
    }
    if (input.floor == KnowledgeFloor::Appearance &&
        input.domain != InformationDomain::Appearance) {
        return false;
    }
    return true;
}

bool validRelationCapabilities(
    const RelationRecordInput& input) noexcept {
    if (input.fusionAllowed || input.calibrationTransferAllowed) {
        return false;
    }
    if (input.status != RelationStatus::Admitted &&
        (input.coordinateTransformAllowed || input.equalityAllowed)) {
        return false;
    }
    if (input.equalityAllowed &&
        input.axis != RelationAxis::RadiometricGauge &&
        input.axis != RelationAxis::Colorimetric) {
        return false;
    }
    return true;
}

bool relationRequiresCertificate(RelationStatus status) noexcept {
    return status == RelationStatus::Calibrated ||
           status == RelationStatus::Admitted;
}

bool hasAxis(
    const std::vector<RelationRecord>& relations,
    RelationAxis axis) noexcept {
    return std::any_of(
        relations.begin(), relations.end(),
        [axis](const RelationRecord& r) {
            return r.finalized &&
                   r.input.status == RelationStatus::Admitted &&
                   r.input.axis == axis;
        });
}

bool requiredAxesPresent(
    FusionKind kind,
    const std::vector<RelationRecord>& relations) noexcept {
    const bool uncertainty =
        hasAxis(relations, RelationAxis::UncertaintyCorrelation);
    const bool temporal =
        hasAxis(relations, RelationAxis::Temporal);

    if (!uncertainty || !temporal) {
        return false;
    }

    switch (kind) {
        case FusionKind::Radiometric:
            return hasAxis(relations, RelationAxis::RadiometricGauge);
        case FusionKind::Color:
            return hasAxis(relations, RelationAxis::RadiometricGauge) &&
                   hasAxis(relations, RelationAxis::Colorimetric);
        case FusionKind::SpatialDetail:
            return hasAxis(relations, RelationAxis::Geometry) &&
                   hasAxis(relations, RelationAxis::OpticalSupport);
    }
    return false;
}

bool relationMatchesPair(
    const RelationRecord& relation,
    const std::string& a,
    const std::string& b) noexcept {
    return
        (relation.input.observationA == a &&
         relation.input.observationB == b) ||
        (relation.input.observationA == b &&
         relation.input.observationB == a);
}

}  // namespace

bool finalizeObservation(
    const ObservationNodeInput& input,
    ObservationNode& out) noexcept {
    out = ObservationNode{};
    try {
        if (input.observationId.empty() ||
            !nonzero(input.drawNegativeStateSha256) ||
            !nonzero(input.sourceEvidenceSha256) ||
            input.sourceLocalGaugeId.empty() ||
            input.physicalFrameCount != 1u ||
            input.independentEvidenceCount != 1u ||
            !validTemporal(input)) {
            return false;
        }

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[] = "D_RAW_FREE_WORLD_OBSERVATION_NODE_V0_1";
        h.update(reinterpret_cast<const std::uint8_t*>(domain),
                 sizeof(domain) - 1u);
        hashString(h, input.observationId);
        h.update(input.drawNegativeStateSha256);
        h.update(input.sourceEvidenceSha256);
        hashU8(h, static_cast<std::uint8_t>(input.lensRole));
        hashString(h, input.sourceLocalGaugeId);
        hashU8(h, static_cast<std::uint8_t>(input.temporalModel));
        hashU8(h, input.exposureIntervalKnown ? 1u : 0u);
        hashU8(h, input.rollingShutterModelKnown ? 1u : 0u);
        hashU32(h, input.physicalFrameCount);
        hashU32(h, input.independentEvidenceCount);

        out.input = input;
        out.stateSha256 = h.finalize();
        if (!nonzero(out.stateSha256)) {
            out = ObservationNode{};
            return false;
        }
        out.finalized = true;
        out.sourceEvidenceMutable = false;
        out.observationIdentityMutable = false;
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;
        out.methodId = kObservationMethod;
        return true;
    } catch (...) {
        out = ObservationNode{};
        return false;
    }
}

bool finalizeInformation(
    const InformationRecordInput& input,
    InformationRecord& out) noexcept {
    out = InformationRecord{};
    try {
        if (input.informationId.empty() ||
            input.observationId.empty() ||
            input.authorityTag.empty() ||
            input.createsNewEvidence ||
            !validPlacement(input) ||
            (input.identityKnown && !nonzero(input.identitySha256))) {
            return false;
        }

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[] = "D_RAW_FREE_WORLD_INFORMATION_RECORD_V0_1";
        h.update(reinterpret_cast<const std::uint8_t*>(domain),
                 sizeof(domain) - 1u);
        hashString(h, input.informationId);
        hashString(h, input.observationId);
        hashU8(h, static_cast<std::uint8_t>(input.domain));
        hashU8(h, static_cast<std::uint8_t>(input.floor));
        hashString(h, input.authorityTag);
        hashU8(h, input.identityKnown ? 1u : 0u);
        if (input.identityKnown) {
            h.update(input.identitySha256);
        }

        out.input = input;
        out.stateSha256 = h.finalize();
        if (!nonzero(out.stateSha256)) {
            out = InformationRecord{};
            return false;
        }
        out.finalized = true;
        out.promotesSourceEvidence = false;
        out.scientificWritebackAllowed = false;
        out.methodId = kInformationMethod;
        return true;
    } catch (...) {
        out = InformationRecord{};
        return false;
    }
}

bool finalizeRelation(
    const RelationRecordInput& input,
    RelationRecord& out) noexcept {
    out = RelationRecord{};
    try {
        if (input.relationId.empty() ||
            input.observationA.empty() ||
            input.observationB.empty() ||
            input.observationA == input.observationB ||
            !validRelationCapabilities(input)) {
            return false;
        }

        if (relationRequiresCertificate(input.status)) {
            if (!input.certificateKnown ||
                !nonzero(input.certificateSha256)) {
                return false;
            }
        } else if (input.certificateKnown &&
                   !nonzero(input.certificateSha256)) {
            return false;
        }

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[] = "D_RAW_FREE_WORLD_RELATION_RECORD_V0_1";
        h.update(reinterpret_cast<const std::uint8_t*>(domain),
                 sizeof(domain) - 1u);
        hashString(h, input.relationId);
        hashString(h, input.observationA);
        hashString(h, input.observationB);
        hashU8(h, static_cast<std::uint8_t>(input.axis));
        hashU8(h, static_cast<std::uint8_t>(input.status));
        hashU8(h, input.certificateKnown ? 1u : 0u);
        if (input.certificateKnown) {
            h.update(input.certificateSha256);
        }
        hashU8(h, input.coordinateTransformAllowed ? 1u : 0u);
        hashU8(h, input.equalityAllowed ? 1u : 0u);

        out.input = input;
        out.stateSha256 = h.finalize();
        if (!nonzero(out.stateSha256)) {
            out = RelationRecord{};
            return false;
        }
        out.finalized = true;
        out.relationAxisIndependent = true;
        out.promotesSourceEvidence = false;
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;
        out.methodId = kRelationMethod;
        return true;
    } catch (...) {
        out = RelationRecord{};
        return false;
    }
}

bool finalizeFusionAdmission(
    const FusionAdmissionInput& input,
    FusionAdmission& out) noexcept {
    out = FusionAdmission{};
    try {
        if (input.observationA.empty() ||
            input.observationB.empty() ||
            input.observationA == input.observationB ||
            input.relations.empty() ||
            !nonzero(input.certificateSha256) ||
            !requiredAxesPresent(input.kind, input.relations)) {
            return false;
        }

        for (const auto& relation : input.relations) {
            if (!relation.finalized ||
                relation.input.status != RelationStatus::Admitted ||
                relation.input.fusionAllowed ||
                relation.input.calibrationTransferAllowed ||
                !relationMatchesPair(
                    relation, input.observationA, input.observationB)) {
                return false;
            }
        }

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[] =
            "D_RAW_FREE_WORLD_COMPOSITE_FUSION_ADMISSION_V0_1";
        h.update(reinterpret_cast<const std::uint8_t*>(domain),
                 sizeof(domain) - 1u);
        hashU8(h, static_cast<std::uint8_t>(input.kind));
        hashString(h, input.observationA);
        hashString(h, input.observationB);
        h.update(input.certificateSha256);
        hashU64(h, static_cast<std::uint64_t>(input.relations.size()));

        out.relationStateSha256.reserve(input.relations.size());
        for (const auto& relation : input.relations) {
            out.relationStateSha256.push_back(relation.stateSha256);
            h.update(relation.stateSha256);
        }

        out.kind = input.kind;
        out.observationA = input.observationA;
        out.observationB = input.observationB;
        out.certificateSha256 = input.certificateSha256;
        out.stateSha256 = h.finalize();
        if (!nonzero(out.stateSha256)) {
            out = FusionAdmission{};
            return false;
        }
        out.finalized = true;
        out.admitted = true;
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;
        out.methodId = kFusionMethod;
        return true;
    } catch (...) {
        out = FusionAdmission{};
        return false;
    }
}

const char* schema_name() noexcept {
    return kSchemaName;
}

}  // namespace truthraw::free_world_observation_graph::v0_1
