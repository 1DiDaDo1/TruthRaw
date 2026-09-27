#pragma once

#include "truthraw_sha256_v0_69.h"

#include <cstdint>
#include <string>
#include <vector>

namespace truthraw::free_world_observation_graph::v0_1 {

using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "D.RAW/FreeWorldObservationGraphNative/0.1";
inline constexpr const char* kObservationMethod =
    "D_RAW_FREE_WORLD_OBSERVATION_NODE_V0_1";
inline constexpr const char* kInformationMethod =
    "D_RAW_FREE_WORLD_INFORMATION_RECORD_V0_1";
inline constexpr const char* kRelationMethod =
    "D_RAW_FREE_WORLD_RELATION_RECORD_V0_1";
inline constexpr const char* kFusionMethod =
    "D_RAW_FREE_WORLD_COMPOSITE_FUSION_ADMISSION_V0_1";

enum class KnowledgeFloor : std::uint8_t {
    Evidence = 1u,
    ScientificDerived = 2u,
    Relation = 3u,
    SceneModel = 4u,
    Appearance = 5u,
};

enum class InformationDomain : std::uint8_t {
    SourceEvidence = 1u,
    SamplingGeometry = 2u,
    GeometryPose = 3u,
    Radiometry = 4u,
    Colorimetry = 5u,
    Spectral = 6u,
    OpticalSupport = 7u,
    NoiseUncertainty = 8u,
    Temporal = 9u,
    Material = 10u,
    Illumination = 11u,
    Provenance = 12u,
    Restoration = 13u,
    Appearance = 14u,
};

enum class RelationAxis : std::uint8_t {
    Geometry = 1u,
    RadiometricGauge = 2u,
    Colorimetric = 3u,
    Spectral = 4u,
    OpticalSupport = 5u,
    UncertaintyCorrelation = 6u,
    Temporal = 7u,
    Provenance = 8u,
};

enum class RelationStatus : std::uint8_t {
    Unknown = 1u,
    Hypothesis = 2u,
    SourceBound = 3u,
    Calibrated = 4u,
    Admitted = 5u,
    Rejected = 6u,
};

enum class FusionKind : std::uint8_t {
    Radiometric = 1u,
    Color = 2u,
    SpatialDetail = 3u,
};

enum class LensRole : std::uint8_t {
    Main = 1u,
    UltraWide = 2u,
    Telephoto = 3u,
    Front = 4u,
    External = 5u,
    Unknown = 6u,
};

enum class TemporalModel : std::uint8_t {
    Unknown = 1u,
    CaptureTimestampOnly = 2u,
    ExposureInterval = 3u,
    GlobalShutterInterval = 4u,
    RollingShutterModel = 5u,
};

struct ObservationNodeInput final {
    std::string observationId{};
    Digest drawNegativeStateSha256{};
    Digest sourceEvidenceSha256{};
    LensRole lensRole = LensRole::Unknown;
    std::string sourceLocalGaugeId{};
    TemporalModel temporalModel = TemporalModel::Unknown;
    bool exposureIntervalKnown = false;
    bool rollingShutterModelKnown = false;
    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;
};

struct ObservationNode final {
    ObservationNodeInput input{};
    Digest stateSha256{};
    bool finalized = false;
    bool sourceEvidenceMutable = false;
    bool observationIdentityMutable = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
    std::string methodId = kObservationMethod;
};

struct InformationRecordInput final {
    std::string informationId{};
    std::string observationId{};
    InformationDomain domain = InformationDomain::SourceEvidence;
    KnowledgeFloor floor = KnowledgeFloor::ScientificDerived;
    std::string authorityTag{};
    Digest identitySha256{};
    bool identityKnown = false;
    bool createsNewEvidence = false;
};

struct InformationRecord final {
    InformationRecordInput input{};
    Digest stateSha256{};
    bool finalized = false;
    bool promotesSourceEvidence = false;
    bool scientificWritebackAllowed = false;
    std::string methodId = kInformationMethod;
};

struct RelationRecordInput final {
    std::string relationId{};
    std::string observationA{};
    std::string observationB{};
    RelationAxis axis = RelationAxis::Geometry;
    RelationStatus status = RelationStatus::Unknown;
    Digest certificateSha256{};
    bool certificateKnown = false;
    bool coordinateTransformAllowed = false;
    bool equalityAllowed = false;
    bool fusionAllowed = false;
    bool calibrationTransferAllowed = false;
};

struct RelationRecord final {
    RelationRecordInput input{};
    Digest stateSha256{};
    bool finalized = false;
    bool relationAxisIndependent = true;
    bool promotesSourceEvidence = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
    std::string methodId = kRelationMethod;
};

struct FusionAdmissionInput final {
    FusionKind kind = FusionKind::Radiometric;
    std::string observationA{};
    std::string observationB{};
    std::vector<RelationRecord> relations{};
    Digest certificateSha256{};
};

struct FusionAdmission final {
    FusionKind kind = FusionKind::Radiometric;
    std::string observationA{};
    std::string observationB{};
    std::vector<Digest> relationStateSha256{};
    Digest certificateSha256{};
    Digest stateSha256{};
    bool finalized = false;
    bool admitted = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
    std::string methodId = kFusionMethod;
};

bool finalizeObservation(
    const ObservationNodeInput& input,
    ObservationNode& out) noexcept;

bool finalizeInformation(
    const InformationRecordInput& input,
    InformationRecord& out) noexcept;

bool finalizeRelation(
    const RelationRecordInput& input,
    RelationRecord& out) noexcept;

bool finalizeFusionAdmission(
    const FusionAdmissionInput& input,
    FusionAdmission& out) noexcept;

const char* schema_name() noexcept;

}  // namespace truthraw::free_world_observation_graph::v0_1
