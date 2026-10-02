package com.truthraw.adaptiveui

import org.json.JSONObject

/**
 * One exportable read-only bundle for the current Free World implementation
 * wave. It binds implemented infrastructure without promoting any unvalidated
 * physical interpretation.
 */
object FreeWorldObservationGeometryFoundationV01 {
    const val SCHEMA = "D.RAW/FreeWorldObservationGeometryFoundation/0.1"

    fun buildWithCalibrationBundle(
        profiles: List<JSONObject>,
        fieldRepeatability: JSONObject? = null,
        calibrationRecordBundleJson: String,
    ): JSONObject =
        build(
            profiles = profiles,
            fieldRepeatability = fieldRepeatability,
            calibrationRecords =
                CalibrationObservationRecordBundleV01.validRecords(
                    calibrationRecordBundleJson,
                ),
        )

    fun build(
        profiles: List<JSONObject>,
        fieldRepeatability: JSONObject? = null,
        calibrationRecords: List<JSONObject> = emptyList(),
        promotionState: JSONObject =
            ScientificPromotionStateV01.blocked(),
        validatedWorldToSourceMapping: JSONObject? = null,
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
        val radiometricResponse =
            RadiometricResponseAtlasV01.build(profiles)
        val noiseComponents =
            NoiseComponentAtlasV01.build(profiles)
        val scientificNoiseTransport =
            ScientificNoiseTransportV01.describe(promotionState)
        val temporalFootprint =
            TemporalFootprintV01.build(profiles)
        val geometryDepth =
            GeometryDepthSupportAtlasV01.describe(
                graph = graph,
                tracks = featureTracks,
                cycles = cycleConsistency,
            )
        val residualRelation =
            MultiObservationResidualRelationV01.build(
                profiles = profiles,
                graph = graph,
                temporalFootprint = temporalFootprint,
            )
        val worldSpaceNoise =
            WorldSpaceNoiseSeparationV01.describe(
                decomposition = decomposition,
                temporal = temporalFootprint,
                geometry = geometryDepth,
            )

        val calibrationBinding =
            CalibrationObservationSessionBindingV01.bind(
                profiles = profiles,
                records = calibrationRecords,
            )
        val boundCalibrationRecords =
            calibrationBinding.records
        val activeSourceRoots =
            linkedSetOf<String>().apply {
                for (profile in profiles) {
                    profile.optString("source_sha256")
                        .trim()
                        .lowercase()
                        .takeIf { it.matches(Regex("[0-9a-f]{64}")) }
                        ?.let(::add)
                    profile.optString(
                        "upstream_sealed_source_sha256",
                    )
                        .trim()
                        .lowercase()
                        .takeIf { it.matches(Regex("[0-9a-f]{64}")) }
                        ?.let(::add)
                }
            }
        val worldSourceBridge =
            FreeWorldSourceLatticeBridgeContractV01.describe(
                promotionState = promotionState,
                validatedMapping = validatedWorldToSourceMapping,
                activeSourceRoots = activeSourceRoots,
            )

        val radiometricResponseCandidate =
            RadiometricResponseCandidateSolverV01.evaluate(
                boundCalibrationRecords,
            )
        val radiometricProfileCandidate =
            RadiometricProfileRelationCandidateV01.evaluate(
                profiles = profiles,
                records = boundCalibrationRecords,
            )
        val noiseComponentCandidate =
            NoiseComponentDecompositionCandidateV01.evaluate(boundCalibrationRecords)
        val noiseSpectrumCandidate =
            NoiseSpectrumMeasurementCandidateV01.evaluate(boundCalibrationRecords)
        val repeatedSparseGridNoiseCandidate =
            RepeatedSparseGridNoiseCandidateV01.evaluate(
                profiles = profiles,
                records = boundCalibrationRecords,
            )
        val sparseGridNoiseModelCandidate =
            SparseGridNoiseModelCandidateV01.evaluate(
                repeatedSparseGrid = repeatedSparseGridNoiseCandidate,
            )
        val fieldResponseSeparationCandidate =
            FieldResponseRotationSeparationCandidateV01.evaluate(boundCalibrationRecords)
        val registrationAwareControlledRotationAudit =
            RegistrationAwareControlledRotationAuditV01.evaluate(
                graph = graph,
                records = boundCalibrationRecords,
            )
        val controlledRotationConstrainedGeometryAudit =
            ControlledRotationConstrainedGeometryV01.evaluate(
                profiles = profiles,
                records = boundCalibrationRecords,
            )
        val geometryAdjustedFieldMappingDryRun =
            GeometryAdjustedFieldMappingDryRunV01.evaluate(
                profiles = profiles,
                records = boundCalibrationRecords,
                constrainedAudit =
                    controlledRotationConstrainedGeometryAudit,
            )
        val controlledRotationFieldCoordinateBridgeAudit =
            ControlledRotationFieldCoordinateBridgeAuditV01.evaluate(
                profiles = profiles,
                records = boundCalibrationRecords,
                constrainedAudit =
                    controlledRotationConstrainedGeometryAudit,
            )
        val measuredFieldSupportCoordinateBridgeAudit =
            MeasuredFieldSupportCoordinateBridgeAuditV01.evaluate(
                profiles = profiles,
                records = boundCalibrationRecords,
                constrainedAudit =
                    controlledRotationConstrainedGeometryAudit,
            )
        val opticalFieldTopographyAudit =
            OpticalFieldTopographyAuditV01.evaluate(
                records = boundCalibrationRecords,
                fieldResponse =
                    fieldResponseSeparationCandidate,
                measuredSupportAudit =
                    measuredFieldSupportCoordinateBridgeAudit,
            )
        val colourRelationCandidate =
            ColourRelationCandidateSolverV01.evaluate(boundCalibrationRecords)
        val opticalSupportCandidate =
            OpticalSupportMeasurementCandidateV01.evaluate(boundCalibrationRecords)
        val temporalSequenceCandidate =
            TemporalSequenceCandidateSolverV01.evaluate(boundCalibrationRecords)
        val geometryDepthCandidate =
            GeometryDepthCandidateSolverV01.evaluate(boundCalibrationRecords)
        val colourCovarianceTransportCandidate =
            ColourCovarianceTransportCandidateV01.evaluate(
                colourRelation = colourRelationCandidate,
                records = boundCalibrationRecords,
            )
        val noiseOpticsJointCandidate =
            NoiseOpticsJointCandidateV01.evaluate(
                noiseSpectrum = noiseSpectrumCandidate,
                opticalSupport = opticalSupportCandidate,
                records = boundCalibrationRecords,
            )
        val worldSpaceResidualCandidate =
            WorldSpaceResidualCandidateSolverV01.evaluate(
                records = boundCalibrationRecords,
                worldSourceBridge = worldSourceBridge,
                promotionState = promotionState,
            )

        val scientificDenoiseAdmission =
            ScientificDenoiseAdmissionV01.describe(
                radiometric = radiometricResponseCandidate,
                noiseComponents = noiseComponentCandidate,
                noiseTransport = scientificNoiseTransport,
                opticalSupport = opticalSupportCandidate,
                temporalFootprint = temporalSequenceCandidate,
                geometryDepth = geometryDepthCandidate,
                worldSpaceNoise = worldSpaceResidualCandidate,
                promotionState = promotionState,
                allowPromotionProjection = false,
            )
        val lightTransportAuthority =
            LightTransportAuthorityContractV01.describe()
        val perceptualNoiseAppearance =
            PerceptualNoiseAppearanceV01.describe()
        val querySupportLedger =
            FreeWorldQuerySupportLedgerV01.build(profiles)
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
        val universalIdentityIndependence =
            UniversalIdentityIndependenceV01.describe()

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
            .put(
                "performance_diagnostics_v0_1",
                FreeWorldPerformanceDiagnosticsV01.build(profiles),
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
            .put("radiometric_response_atlas", radiometricResponse)
            .put("noise_component_atlas", noiseComponents)
            .put("scientific_noise_transport", scientificNoiseTransport)
            .put("temporal_footprint", temporalFootprint)
            .put("geometry_depth_support_atlas", geometryDepth)
            .put("multi_observation_residual_relation", residualRelation)
            .put("world_space_noise_separation", worldSpaceNoise)
            .put(
                "numeric_candidate_runtime",
                JSONObject()
                    .put("calibration_observation_record_count", calibrationRecords.size)
                    .put("radiometric_response_candidate", radiometricResponseCandidate)
                    .put("radiometric_profile_candidate", radiometricProfileCandidate)
                    .put("noise_component_candidate", noiseComponentCandidate)
                    .put("noise_spectrum_candidate", noiseSpectrumCandidate)
                    .put("repeated_sparse_grid_noise_candidate", repeatedSparseGridNoiseCandidate)
                    .put("sparse_grid_noise_model_candidate", sparseGridNoiseModelCandidate)
                    .put("field_response_separation_candidate", fieldResponseSeparationCandidate)
                    .put(
                        "registration_aware_controlled_rotation_audit",
                        registrationAwareControlledRotationAudit,
                    )
                    .put(
                        "controlled_rotation_constrained_geometry_audit",
                        controlledRotationConstrainedGeometryAudit,
                    )
                    .put(
                        "geometry_adjusted_field_mapping_dry_run",
                        geometryAdjustedFieldMappingDryRun,
                    )
                    .put(
                        "controlled_rotation_field_coordinate_bridge_audit",
                        controlledRotationFieldCoordinateBridgeAudit,
                    )
                    .put(
                        "measured_field_support_coordinate_bridge_audit",
                        measuredFieldSupportCoordinateBridgeAudit,
                    )
                    .put(
                        "optical_field_topography_audit",
                        opticalFieldTopographyAudit,
                    )
                    .put("colour_relation_candidate", colourRelationCandidate)
                    .put(
                        "colour_covariance_transport_candidate",
                        colourCovarianceTransportCandidate,
                    )
                    .put("optical_support_candidate", opticalSupportCandidate)
                    .put(
                        "noise_optics_joint_candidate",
                        noiseOpticsJointCandidate,
                    )
                    .put("temporal_sequence_candidate", temporalSequenceCandidate)
                    .put("geometry_depth_candidate", geometryDepthCandidate)
                    .put("world_space_residual_candidate", worldSpaceResidualCandidate)
                    .put(
                        "noise_aware_inverse_optics_operator",
                        "NoiseAwareInverseOpticsCandidateV01",
                    )
                    .put(
                        "scientific_reconstruction_candidate_operator",
                        "ScientificReconstructionCandidateV01",
                    )
                    .put(
                        "scientific_denoise_operator",
                        "ScientificDenoiseOperatorV01",
                    )
                    .put(
                        "perceptual_noise_visibility_candidate",
                        "PerceptualNoiseVisibilityCandidateV01",
                    )
                    .put("candidates_are_promotions", false)
                    .put("candidate_application_enabled", false),
            )
            .put(
                "calibration_observation_session_binding",
                calibrationBinding.report,
            )
            .put("scientific_promotion_state", promotionState)
            .put(
                "validated_world_to_source_mapping_attached",
                validatedWorldToSourceMapping != null,
            )
            .put("scientific_denoise_admission", scientificDenoiseAdmission)
            .put("light_transport_authority_contract", lightTransportAuthority)
            .put("perceptual_noise_appearance_contract", perceptualNoiseAppearance)
            .put("query_support_ledger", querySupportLedger)
            .put("capability_matrix", capabilityMatrix)
            .put("evidence_lineage_manifest", lineage)
            .put("bundled_physical_validation_campaign", validationCampaign)
            .put("scientific_gate_registry", gateRegistry)
            .put("universal_identity_independence", universalIdentityIndependence)
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
                worldSourceBridge,
            )
            .put(
                "calibration_observation_record_contract",
                CalibrationObservationRecordV01.describeContract()
                    .put(
                        "attached_record_count",
                        calibrationRecords.size,
                    )
                    .put(
                        "session_bound_record_count",
                        boundCalibrationRecords.size,
                    )
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
                    )
                    .put(
                        "camera_lens_vendor_or_raw_identity_may_route_decoding_not_scientific_truth",
                        true,
                    )
                    .put(
                        "noise_reduction_requires_explained_physical_or_reconstruction_residual",
                        true,
                    )
                    .put(
                        "radiometry_geometry_illumination_material_view_are_separate_authority_axes",
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
                    .put("radiometric_calibration_promoted", false)
                    .put("noise_component_calibration_promoted", false)
                    .put("world_space_denoise_applied", false)
                    .put("geometry_promoted", false)
                    .put("temporal_fusion_applied", false)
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
