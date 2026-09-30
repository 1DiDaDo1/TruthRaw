package com.truthraw.adaptiveui

import org.json.JSONObject

/**
 * One exportable read-only bundle for the current Free World implementation
 * wave. It binds implemented infrastructure without promoting any unvalidated
 * physical interpretation.
 */
object FreeWorldObservationGeometryFoundationV01 {
    const val SCHEMA = "D.RAW/FreeWorldObservationGeometryFoundation/0.1"

    fun build(
        profiles: List<JSONObject>,
        fieldRepeatability: JSONObject? = null,
    ): JSONObject {
        val campaign =
            MultiObservationCampaignV01.describe(profiles)
        val graph =
            FreeWorldObservationGraphV01.build(profiles)
        val atlas =
            NaturalSelfCalibrationAtlasV01.build(
                profiles = profiles,
                graph = graph,
                fieldRepeatability = fieldRepeatability,
            )
        val relativeWorld =
            RelativeWorldCoordinateHypothesisV01.build(graph)
        val observationComponents =
            FreeWorldObservationComponentsV01.build(graph)
        val componentRelativeWorld =
            ComponentRelativeWorldCoordinateHypothesesV01.build(
                graph = graph,
                components = observationComponents,
            )
        val featureTracks =
            FreeWorldFeatureTrackHypothesesV01.build(graph)
        val cycleConsistency =
            ObservationGraphCycleConsistencyV01.evaluate(graph)
        val pairGeometryModelBanks =
            PairGeometryModelBankSetV01.build(graph)
        val trackProjection =
            RelativeWorldFeatureTrackProjectionV01.build(
                tracks = featureTracks,
                relativeWorld = componentRelativeWorld,
            )
        val geometryValidationReadiness =
            GeometryValidationReadinessV01.describe(
                graph = graph,
                tracks = featureTracks,
                cycles = cycleConsistency,
                trackProjection = trackProjection,
            )
        val decomposition =
            WorldSensorFieldDecompositionScaffoldV01.describe(
                profiles = profiles,
                graph = graph,
            )
        val uncertainty =
            FreeWorldUncertaintyTransportV01.describe(
                graph = graph,
                atlas = atlas,
            )
        val temporal =
            TemporalObservationRelationV01.describe(profiles)
        val querySupportLedger =
            FreeWorldQuerySupportLedgerV01.build(profiles)
        val queryPlanner =
            FreeWorldContinuousQueryPlannerV01.plan(
                components = observationComponents,
                querySupportLedger = querySupportLedger,
            )
        val axisAuthorityMatrix =
            ObservationAxisAuthorityMatrixV01.build(profiles)
        val unknownPropagation =
            UnknownPropagationGuardV01.describe()
        val fieldSeparationCandidateSet =
            FieldResponseSeparationCandidateSetV01.build(
                profiles = profiles,
                graph = graph,
                modelBankSet = pairGeometryModelBanks,
            )
        val capabilityMatrix =
            FreeWorldCapabilityMatrixV01.describe(
                graph = graph,
                tracks = featureTracks,
                cycles = cycleConsistency,
                decomposition = decomposition,
            )
        val lineage =
            FreeWorldEvidenceLineageManifestV01.build(
                profiles = profiles,
                graph = graph,
            )
        val validationCampaign =
            BundledPhysicalValidationCampaignV01.describe()
        val gateRegistry =
            ScientificPromotionGateRegistryV01.describe()

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (profiles.size >= 2) {
                    "FREE_WORLD_FOUNDATION_AVAILABLE"
                } else {
                    "UNKNOWN_FAIL_CLOSED"
                },
            )
            .put("campaign", campaign)
            .put("observation_graph", graph)
            .put("observation_components", observationComponents)
            .put("relative_world_coordinate_hypothesis", relativeWorld)
            .put(
                "component_relative_world_coordinate_hypotheses",
                componentRelativeWorld,
            )
            .put("feature_track_hypotheses", featureTracks)
            .put("graph_cycle_consistency", cycleConsistency)
            .put("pair_geometry_model_banks", pairGeometryModelBanks)
            .put("relative_world_feature_track_projection", trackProjection)
            .put("geometry_validation_readiness", geometryValidationReadiness)
            .put("natural_self_calibration_atlas", atlas)
            .put("world_sensor_decomposition", decomposition)
            .put("uncertainty_transport", uncertainty)
            .put("temporal_relation", temporal)
            .put("query_support_ledger", querySupportLedger)
            .put("continuous_query_planner", queryPlanner)
            .put("observation_axis_authority_matrix", axisAuthorityMatrix)
            .put("unknown_propagation_guard", unknownPropagation)
            .put(
                "field_response_separation_candidate_set",
                fieldSeparationCandidateSet,
            )
            .put("capability_matrix", capabilityMatrix)
            .put("evidence_lineage_manifest", lineage)
            .put("bundled_physical_validation_campaign", validationCampaign)
            .put("scientific_gate_registry", gateRegistry)
            .put(
                "optical_support_atlas_contract",
                OpticalSupportAtlasV01.describe(),
            )
            .put(
                "colour_relation_atlas_contract",
                ColourRelationAtlasV01.describe(),
            )
            .put(
                "continuous_query_contract",
                FreeWorldContinuousQueryContractV01.describe(),
            )
            .put(
                "continuous_query_typed_abi",
                JSONObject()
                    .put(
                        "request_type",
                        "FreeWorldContinuousQueryRequestV01",
                    )
                    .put(
                        "result_type",
                        "FreeWorldContinuousQueryResultV01",
                    )
                    .put(
                        "solver_interface",
                        "FreeWorldContinuousQuerySolverV01",
                    )
                    .put(
                        "source_lattice_exact_anchor_resolver",
                        "SourceLatticeExactAnchorResolverV01",
                    )
                    .put(
                        "measured_anchor_provider_interface",
                        "MeasuredAnchorProviderV01",
                    )
                    .put(
                        "fail_closed_world_solver",
                        "FailClosedFreeWorldContinuousQuerySolverV01",
                    )
                    .put(
                        "query_planner",
                        "FreeWorldContinuousQueryPlannerV01",
                    )
                    .put("world_solver_implemented", false)
                    .put(
                        "fail_closed_world_runtime_implemented",
                        true,
                    )
                    .put(
                        "exact_source_anchor_semantics_implemented",
                        true,
                    ),
            )
            .put(
                "world_source_lattice_bridge_contract",
                FreeWorldSourceLatticeBridgeContractV01.describe(),
            )
            .put(
                "calibration_observation_record_contract",
                CalibrationObservationRecordV01.describeContract()
                    .put(
                        "validator",
                        "CalibrationObservationRecordValidatorV01",
                    ),
            )
            .put(
                "view_appearance_state_contract",
                ViewAppearanceStateV01.describe(),
            )
            .put(
                "restoration_authority_contract",
                ConservationRestorationAuthorityRuntimeV01.describe(),
            )
            .put(
                "architecture_law",
                JSONObject()
                    .put(
                        "measured_reconstructed_appearance_separated",
                        true,
                    )
                    .put(
                        "source_raster_is_world_resolution_authority",
                        false,
                    )
                    .put(
                        "seal_the_evidence_not_the_thinking",
                        true,
                    )
                    .put(
                        "representation_may_exceed_source_knowledge_claims_may_not",
                        true,
                    ),
            )
            .put(
                "promotion_boundary",
                JSONObject()
                    .put("world_registration_promoted", false)
                    .put("camera_system_response_proven", false)
                    .put("lens_only_vignetting_proven", false)
                    .put("calibration_promoted", false)
                    .put("correction_authorized", false)
                    .put("deconvolution_authorized", false)
                    .put("multi_frame_scientific_fusion_applied", false)
                    .put("scientific_writeback_allowed", false),
            )
            .put("ai_ml_neural_generative_used", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
            .also { report ->
                report.put(
                    "promotion_firewall",
                    ResearchPromotionFirewallV01.audit(report),
                )
            }
    }
}
