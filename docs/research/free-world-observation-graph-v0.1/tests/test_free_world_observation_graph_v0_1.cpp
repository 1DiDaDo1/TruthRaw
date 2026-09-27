#include "free_world_observation_graph_v0_1.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace graph = truthraw::free_world_observation_graph::v0_1;

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)

namespace {

graph::Digest digest(std::uint8_t seed) {
    graph::Digest d{};
    for (std::size_t i = 0u; i < d.size(); ++i) {
        d[i] = static_cast<std::uint8_t>(seed + i + 1u);
    }
    return d;
}

graph::ObservationNodeInput camera5() {
    graph::ObservationNodeInput i{};
    i.observationId = "DRAW_OBS_CAMERA5";
    i.drawNegativeStateSha256 = digest(1u);
    i.sourceEvidenceSha256 = digest(40u);
    i.lensRole = graph::LensRole::Telephoto;
    i.sourceLocalGaugeId = "DRAW_SOURCE_LOCAL_GAUGE_CAMERA5";
    i.temporalModel = graph::TemporalModel::CaptureTimestampOnly;
    i.exposureIntervalKnown = false;
    i.rollingShutterModelKnown = false;
    return i;
}

graph::RelationRecord admitted(
    const char* id,
    graph::RelationAxis axis,
    std::uint8_t seed,
    bool coordinate = false,
    bool equality = false) {
    graph::RelationRecordInput i{};
    i.relationId = id;
    i.observationA = "OBS_A";
    i.observationB = "OBS_B";
    i.axis = axis;
    i.status = graph::RelationStatus::Admitted;
    i.certificateSha256 = digest(seed);
    i.certificateKnown = true;
    i.coordinateTransformAllowed = coordinate;
    i.equalityAllowed = equality;
    graph::RelationRecord out{};
    REQUIRE(graph::finalizeRelation(i, out));
    return out;
}

void test_observation_is_immutable_one_frame_node() {
    graph::ObservationNode out{};
    REQUIRE(graph::finalizeObservation(camera5(), out));
    REQUIRE(out.finalized);
    REQUIRE(!out.sourceEvidenceMutable);
    REQUIRE(!out.observationIdentityMutable);
    REQUIRE(!out.createsNewEvidence);
    REQUIRE(!out.scientificWritebackAllowed);
    REQUIRE(out.stateSha256 != graph::Digest{});
}

void test_temporal_claim_fails_closed() {
    auto i = camera5();
    i.temporalModel = graph::TemporalModel::CaptureTimestampOnly;
    i.exposureIntervalKnown = true;
    graph::ObservationNode out{};
    REQUIRE(!graph::finalizeObservation(i, out));

    i = camera5();
    i.temporalModel = graph::TemporalModel::RollingShutterModel;
    i.exposureIntervalKnown = true;
    i.rollingShutterModelKnown = false;
    REQUIRE(!graph::finalizeObservation(i, out));

    i.rollingShutterModelKnown = true;
    REQUIRE(graph::finalizeObservation(i, out));
}

void test_information_knows_its_floor() {
    graph::InformationRecordInput source{};
    source.informationId = "SOURCE";
    source.observationId = "OBS";
    source.domain = graph::InformationDomain::SourceEvidence;
    source.floor = graph::KnowledgeFloor::Evidence;
    source.authorityTag = "SEALED";
    source.identitySha256 = digest(3u);
    source.identityKnown = true;

    graph::InformationRecord out{};
    REQUIRE(graph::finalizeInformation(source, out));
    REQUIRE(!out.promotesSourceEvidence);
    REQUIRE(!out.scientificWritebackAllowed);

    source.floor = graph::KnowledgeFloor::ScientificDerived;
    REQUIRE(!graph::finalizeInformation(source, out));

    graph::InformationRecordInput appearance{};
    appearance.informationId = "APP";
    appearance.observationId = "OBS";
    appearance.domain = graph::InformationDomain::Appearance;
    appearance.floor = graph::KnowledgeFloor::ScientificDerived;
    appearance.authorityTag = "APPEARANCE_ONLY";
    REQUIRE(!graph::finalizeInformation(appearance, out));

    appearance.floor = graph::KnowledgeFloor::Appearance;
    REQUIRE(graph::finalizeInformation(appearance, out));
}

void test_relation_axis_does_not_grant_fusion_or_calibration_transfer() {
    graph::RelationRecordInput i{};
    i.relationId = "GEOM";
    i.observationA = "OBS_A";
    i.observationB = "OBS_B";
    i.axis = graph::RelationAxis::Geometry;
    i.status = graph::RelationStatus::Admitted;
    i.certificateSha256 = digest(8u);
    i.certificateKnown = true;
    i.coordinateTransformAllowed = true;

    graph::RelationRecord out{};
    REQUIRE(graph::finalizeRelation(i, out));
    REQUIRE(out.relationAxisIndependent);
    REQUIRE(!out.promotesSourceEvidence);

    i.fusionAllowed = true;
    REQUIRE(!graph::finalizeRelation(i, out));
    i.fusionAllowed = false;
    i.calibrationTransferAllowed = true;
    REQUIRE(!graph::finalizeRelation(i, out));
}

void test_admission_requires_certificate() {
    graph::RelationRecordInput i{};
    i.relationId = "GAUGE";
    i.observationA = "OBS_A";
    i.observationB = "OBS_B";
    i.axis = graph::RelationAxis::RadiometricGauge;
    i.status = graph::RelationStatus::Admitted;
    i.equalityAllowed = true;

    graph::RelationRecord out{};
    REQUIRE(!graph::finalizeRelation(i, out));

    i.certificateSha256 = digest(10u);
    i.certificateKnown = true;
    REQUIRE(graph::finalizeRelation(i, out));
}

void test_nonadmitted_relation_has_no_operational_capability() {
    graph::RelationRecordInput i{};
    i.relationId = "HYP";
    i.observationA = "OBS_A";
    i.observationB = "OBS_B";
    i.axis = graph::RelationAxis::Geometry;
    i.status = graph::RelationStatus::Hypothesis;
    i.coordinateTransformAllowed = true;

    graph::RelationRecord out{};
    REQUIRE(!graph::finalizeRelation(i, out));

    i.coordinateTransformAllowed = false;
    REQUIRE(graph::finalizeRelation(i, out));
}

void test_radiometric_fusion_requires_composite_axes() {
    const auto gauge = admitted(
        "GAUGE", graph::RelationAxis::RadiometricGauge, 20u, false, true);
    const auto uncertainty = admitted(
        "UNCERTAINTY", graph::RelationAxis::UncertaintyCorrelation, 40u);
    const auto temporal = admitted(
        "TEMPORAL", graph::RelationAxis::Temporal, 60u);

    graph::FusionAdmissionInput i{};
    i.kind = graph::FusionKind::Radiometric;
    i.observationA = "OBS_A";
    i.observationB = "OBS_B";
    i.certificateSha256 = digest(80u);
    i.relations = {gauge, uncertainty};

    graph::FusionAdmission out{};
    REQUIRE(!graph::finalizeFusionAdmission(i, out));

    i.relations.push_back(temporal);
    REQUIRE(graph::finalizeFusionAdmission(i, out));
    REQUIRE(out.admitted);
    REQUIRE(!out.createsNewEvidence);
    REQUIRE(!out.scientificWritebackAllowed);
    REQUIRE(out.relationStateSha256.size() == 3u);
}

void test_color_and_spatial_fusion_need_different_relations() {
    const auto gauge = admitted(
        "GAUGE", graph::RelationAxis::RadiometricGauge, 20u, false, true);
    const auto color = admitted(
        "COLOR", graph::RelationAxis::Colorimetric, 30u, false, true);
    const auto uncertainty = admitted(
        "UNCERTAINTY", graph::RelationAxis::UncertaintyCorrelation, 40u);
    const auto temporal = admitted(
        "TEMPORAL", graph::RelationAxis::Temporal, 60u);
    const auto geometry = admitted(
        "GEOMETRY", graph::RelationAxis::Geometry, 70u, true, false);
    const auto optics = admitted(
        "OPTICS", graph::RelationAxis::OpticalSupport, 90u);

    graph::FusionAdmissionInput i{};
    i.observationA = "OBS_A";
    i.observationB = "OBS_B";
    i.certificateSha256 = digest(100u);

    graph::FusionAdmission out{};

    i.kind = graph::FusionKind::Color;
    i.relations = {gauge, uncertainty, temporal};
    REQUIRE(!graph::finalizeFusionAdmission(i, out));
    i.relations.push_back(color);
    REQUIRE(graph::finalizeFusionAdmission(i, out));

    i.kind = graph::FusionKind::SpatialDetail;
    i.relations = {geometry, uncertainty, temporal};
    REQUIRE(!graph::finalizeFusionAdmission(i, out));
    i.relations.push_back(optics);
    REQUIRE(graph::finalizeFusionAdmission(i, out));
}

void test_wrong_pair_cannot_enter_fusion() {
    const auto gauge = admitted(
        "GAUGE", graph::RelationAxis::RadiometricGauge, 20u, false, true);
    const auto uncertainty = admitted(
        "UNCERTAINTY", graph::RelationAxis::UncertaintyCorrelation, 40u);
    auto temporal = admitted(
        "TEMPORAL", graph::RelationAxis::Temporal, 60u);
    temporal.input.observationB = "OBS_C";

    graph::FusionAdmissionInput i{};
    i.kind = graph::FusionKind::Radiometric;
    i.observationA = "OBS_A";
    i.observationB = "OBS_B";
    i.certificateSha256 = digest(80u);
    i.relations = {gauge, uncertainty, temporal};

    graph::FusionAdmission out{};
    REQUIRE(!graph::finalizeFusionAdmission(i, out));
}

}  // namespace

int main() {
    test_observation_is_immutable_one_frame_node();
    test_temporal_claim_fails_closed();
    test_information_knows_its_floor();
    test_relation_axis_does_not_grant_fusion_or_calibration_transfer();
    test_admission_requires_certificate();
    test_nonadmitted_relation_has_no_operational_capability();
    test_radiometric_fusion_requires_composite_axes();
    test_color_and_spatial_fusion_need_different_relations();
    test_wrong_pair_cannot_enter_fusion();

    std::cout << graph::kSchemaName << " PASS\n";
    std::cout << "relation_axis_independent=1\n";
    std::cout << "single_relation_direct_fusion=0\n";
    std::cout << "composite_fusion_admission_required=1\n";
    std::cout << "scientific_writeback_allowed=0\n";
    return 0;
}
