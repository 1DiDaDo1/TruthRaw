# 2026-10-01 active extension — controlled rotation, measured support, optical topography

This section is the current extension of the Free World Observation Geometry Foundation and supersedes older frontier notes below where they conflict.

Continuation code: `44485`

Current branch:

`research/registration-aware-rotation-audit-v01-2026-10-01`

Current Android/scientific source-code checkpoint:

`faac21e2e2fe97477c846609b8dc4e71af29b3c1`

Current additions integrated into `FreeWorldObservationGeometryFoundationV01`:

- `ControlledRotationConstrainedGeometryV01`
- `GeometryAdjustedFieldMappingDryRunV01`
- `ControlledRotationFieldCoordinateBridgeAuditV01`
- `MeasuredFieldSupportCoordinateBridgeAuditV01`
- `OpticalFieldTopographyAuditV01`

The controlled-rotation campaign uses the same four 0/90/180/270 observations and keeps 270° held out. Residual rotation alone is identical to the nominal relation at the present 12-sector discretization. Translation and especially the full residual similarity can reduce training RMSE while worsening held-out RMSE, so the project does not promote the appearance similarity as physical field truth.

The measured-support successor reconstructs field-cell geometry from the actual `BacksideSignalSupportAudit.sparse_measured_sample_grid` source positions rather than from ideal bin centres.

The topography successor provides X/Y field coordinates plus independent Z layers for measured/derived observables and explicit top/side/oblique view semantics.

Current authority boundary:

- sparse source positions: measured source evidence;
- support centroids/footprints: derived diagnostics;
- topographic height: selected observable only;
- source-grid -> frontside coordinate bridge: unproven;
- literal physical lens curvature: unproven;
- physical scene depth: unproven;
- automatic winner/problem cause: disabled;
- world registration/calibration/correction/writeback: not promoted.

Current detailed docs:

- `docs/research/measured-field-support-coordinate-bridge-v0.1/README.md`
- `docs/research/optical-field-topography-v0.1/README.md`
- `state/CURRENT_PROJECT_STATE_2026-10-01.json`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-01.md`

---

# D.RAW Free World Observation Geometry Foundation v0.1

Date: 2026-09-30

Status: **large implementation wave, research-only, read-only/fail-closed, no physical promotion required to exist**

## Purpose

This branch deliberately changes the development rhythm.

Instead of requiring a phone test after every new idea, D.RAW now implements as much of the universal architecture as can safely exist without claiming physical truth. Every component that still needs device/scene validation remains a candidate, UNKNOWN, or explicit non-promoting contract.

The branch is stacked on PR100 Observation-World Field Separation v0.1 and includes its 5-second Universal Physical Capture timer for ultra-wide, wide/main and tele.

## Implemented modules

### 1. Deterministic Local Feature Geometry v0.1

`DeterministicLocalFeatureGeometryV01.kt`

The frontside inspector now extracts local classical features from the already-decoded analysis bitmap:

- Harris-style 5x5 structure tensor response;
- deterministic response sorting and spatial suppression;
- maximum 96 keypoints;
- local orientation;
- orientation-normalized fixed 128-bit binary intensity descriptor;
- no randomness;
- no AI/ML/neural/generative model;
- no camera/lens/vendor identity.

Authority remains:

`APPEARANCE_DERIVED_ONLY`

A feature is never sensor evidence and is never world-registration proof.

### 2. Deterministic Local Feature Pair Geometry v0.1

`DeterministicLocalFeaturePairGeometryV01.kt`

For every pair of profiled observations:

- 128-bit Hamming matching;
- deterministic nearest/second-nearest ratio gate;
- mutual-best requirement;
- maximum descriptor-distance gate;
- least-squares affine candidate;
- deterministic MAD-based residual rejection;
- affine refit on inliers;
- residual RMS/median/p95;
- normalized inlier coverage;
- approximate rotation/area-scale diagnostics.

A successful fit is explicitly:

`AFFINE_2D_APPEARANCE_HYPOTHESIS`

It does not prove that the two files depict the same physical world structure.

### 3. Free World Observation Graph v0.1

`FreeWorldObservationGraphV01.kt`

Every distinct source SHA-256 is an immutable observation node.

Pair geometry becomes a separate relation edge.

The graph does not:

- merge source evidence;
- increase independent evidence count;
- use camera/lens/vendor identity as a graph key;
- prove same camera/lens/world scene;
- authorize correction.

Graph identity is a derived hash over the sorted source roots plus pair-geometry content. It is an identity for the derived graph state, not a new evidence root.

### 4. Relative World Coordinate Hypothesis v0.1

`RelativeWorldCoordinateHypothesisV01.kt`

D.RAW can now construct a connected relative 2D graph-gauge hypothesis from available pair-affine candidates.

The lexicographically first source SHA may be chosen as a **numeric gauge only**.

It is not:

- the photographer;
- the camera centre;
- a tripod origin;
- a panorama centre;
- an absolute physical world origin.

The result remains appearance-derived and unpromoted.

### 5. Multi-Observation Campaign v0.1

`MultiObservationCampaignV01.kt`

The selected source set can represent:

- ordinary overlap;
- multi-lens overlap;
- rotation sequence;
- original-RAW 360 sequence;
- stop-motion sequence;
- controlled calibration sequence.

Important laws:

- UI selection order is not automatically capture order;
- different optical routes may share world structure;
- different optical routes may not automatically share one field calibration;
- a stitched panorama is a derived view, not independent source evidence;
- the original sealed observations stay the roots.

### 6. Natural Self-Calibration Atlas v0.1

`NaturalSelfCalibrationAtlasV01.kt`

The atlas now exists as a runtime object with separate axes:

- field response;
- CFA phase;
- colour;
- optical support;
- dark/noise;
- temporal.

It may report available observations and candidate readiness, but every axis remains non-promoting until its own validation gate succeeds.

No normal user calibration is required.

No device profile key is allowed.

### 7. World-vs-Sensor Field Decomposition Scaffold v0.1

`WorldSensorFieldDecompositionScaffoldV01.kt`

The runtime can now determine which observation pairs are structurally ready for a future controlled decomposition:

- valid appearance geometry candidate;
- measured field chart on both observations.

It does **not** yet estimate:

- world-fixed field component;
- sensor-fixed field component;
- lens-only vignetting;
- sensor angular response;
- scene illumination.

This prevents infrastructure progress from being confused with solved physics.

### 8. Optical Support Atlas contract v0.1

`OpticalSupportAtlasV01.kt`

The runtime now has explicit slots for:

- SFR;
- MTF;
- PSF;
- radial support;
- tangential support;
- chromatic displacement;
- field curvature;
- focus state.

Scientific support propagation requires future MEASURED or CALIBRATED_ESTIMATE authority.

No inverse optics or deconvolution is enabled.

### 9. Colour Relation Atlas contract v0.1

`ColourRelationAtlasV01.kt`

Separate authority axes now exist for:

- source metadata colour;
- visible/frontside colour;
- empirical reference target;
- illuminant characterization;
- spectral calibration.

The contract requires:

- at least two characterized illuminants for a multi-illuminant candidate;
- a reference target for empirical promotion;
- held-out validation;
- no spectral-truth claim from three-channel RGB;
- white balance is not an illuminant spectrum.

No colour correction is enabled.

### 10. Temporal Observation Relation v0.1

`TemporalObservationRelationV01.kt`

The runtime now carries:

- source-bound capture-time hints;
- future physical sequence relation;
- future rolling-shutter relation;
- future motion path;
- stop-motion relation type;
- RAW 360 relation type.

Synthetic frames and virtual exposures never become independent physical evidence.

No temporal fusion is enabled.

### 11. Axis-Separated Uncertainty Transport v0.1

`FreeWorldUncertaintyTransportV01.kt`

Uncertainty is now explicitly separated across:

- source measurement;
- reconstruction;
- pair geometry;
- world relation;
- field response;
- colour;
- optical support;
- temporal;
- restoration;
- appearance.

A single confidence scalar may not silently replace these axes.

Missing uncertainty does not become zero.

High numerical precision does not imply high authority or low uncertainty.

### 12. Free World Continuous Query Contract v0.1

`FreeWorldContinuousQueryContractV01.kt`

The future raster-independent query ABI is now an executable runtime contract.

A future query must return, at minimum:

- value or UNKNOWN;
- value domain;
- source support;
- reconstruction support;
- spatial footprint;
- temporal footprint;
- authority;
- uncertainty;
- censor bounds;
- provenance roots.

The source raster is not the world-resolution limit.

Finer lattice positions begin UNKNOWN.

Interpolation may never create new MEASURED samples.

The actual continuous pixel solver is intentionally still disabled.

### 13. Conservation/Restoration Authority Runtime v0.1

`ConservationRestorationAuthorityRuntimeV01.kt`

The existing conservation research is now represented in runtime vocabulary:

- ORIGINAL_MEASURED_SUPPORT;
- CONDITION_OBSERVED;
- STABILIZED_DERIVED;
- LOSS_COMPENSATION_RECONSTRUCTED;
- AESTHETIC_REINTEGRATION_ONLY;
- UNRESOLVED_LOSS.

Loss compensation never becomes measured.

Valid measured support cannot be overpainted without a separate invalidation reason.

Restoration must remain replayable/removable from the sealed source lineage.

### 14. Unified Foundation export

`FreeWorldObservationGeometryFoundationV01.kt`

The Android Multi-observation panel now offers:

**Export Free World Observation Geometry Foundation v0.1 · JSON**

It bundles:

- campaign;
- observation graph;
- pair geometry;
- relative world coordinate hypothesis;
- Natural Self-Calibration Atlas;
- world-vs-sensor decomposition readiness;
- uncertainty transport;
- temporal relation;
- optical-support contract;
- colour-relation contract;
- continuous-query contract;
- restoration authority contract.

The export is available after at least two Universal Intake profiles exist.

## Current non-promoting firewall

This branch must keep all of the following false:

- `world_registration_promoted`;
- `camera_system_response_proven`;
- `lens_only_vignetting_proven`;
- `calibration_promoted`;
- `correction_authorized`;
- `deconvolution_authorized`;
- `multi_frame_scientific_fusion_applied`;
- `scientific_writeback_allowed`.

No infrastructure completion changes these facts.

## What still needs physical validation later

The following are now implemented as machinery or contracts but still require future real evidence before promotion:

1. local feature repeatability across real overlapping scenes;
2. pair geometry correctness across viewpoint, focal length and rotation changes;
3. relative-world graph consistency over loops;
4. field response world-fixed vs sensor-fixed decomposition;
5. natural self-calibration repeatability;
6. SFR/MTF/PSF measurement admission;
7. multi-illuminant colour calibration;
8. physical temporal relation / rolling-shutter admission;
9. continuous Free World reconstruction solver;
10. any correction, inverse optics or Scientific Master writeback.

## Development principle

> **Implement the safe architecture first; validate promotion later.**

This does not weaken D.RAW's evidence law because every unvalidated component remains explicitly non-authoritative.


### 15. Typed Free World Continuous Query ABI v0.1

`FreeWorldContinuousQueryApiV01.kt`

The continuous-query concept is now also a typed Kotlin ABI rather than only JSON documentation.

A future solver must consume a request with relative world coordinates, optional time/view direction and requested footprint, and return a result that includes:

- value or null/UNKNOWN;
- value domain;
- authority;
- source support;
- reconstruction support;
- axis-separated uncertainty;
- censor bounds;
- provenance source SHA roots;
- spatial and temporal footprint descriptions.

No solver implementation is enabled yet.

### 16. View / Appearance State boundary v0.1

`ViewAppearanceStateV01.kt`

Viewing distance, display luminance, ambient light, adaptation, output size and acutance are explicitly downstream.

Human-vision or pupil/adaptation modelling may change presentation only.

Film/cinema-like density/tone curves, halation, grain or synthesized motion blur remain appearance-only and never become physical capture evidence.

### 17. Calibration Observation Record contract v0.1

`CalibrationObservationRecordV01.kt`

Optional calibration evidence now has a versioned relation-based record contract.

The primary keys are sealed source SHA roots, not camera/lens/vendor names.

The contract supports field response, colour, optics, dark/noise and temporal observations and requires explicit setup, relation evidence, uncertainty and validation status.

It is never required for normal RAW intake.

### 18. Mechanical Research Promotion Firewall v0.1

`ResearchPromotionFirewallV01.kt`

Foundation exports are recursively audited before they may be written.

The firewall blocks any research bundle that accidentally sets a prohibited promotion flag such as world-registration promotion, camera-system response proof, calibration, correction, deconvolution, multi-frame Scientific Master fusion, creation of new evidence or scientific writeback.

This converts the research/non-promotion rule from documentation into executable runtime enforcement.


### 19. Multi-observation feature-track hypotheses v0.1

`FreeWorldFeatureTrackHypothesesV01.kt`

Robust inlier matches from pair geometry are now assembled into deterministic cross-observation track hypotheses.

Each member remains identified by:

- sealed source SHA-256;
- local feature index;
- normalized frontside coordinates.

Tracks spanning two or more observations are reported explicitly. Tracks that accidentally contain multiple different features from the same observation are **not silently repaired or discarded**; the conflict remains visible in the report.

A track remains:

`APPEARANCE_DERIVED_TRACK_HYPOTHESIS_ONLY`

and never proves that its members are one physical world point.

### 20. Observation graph cycle-consistency audit v0.1

`ObservationGraphCycleConsistencyV01.kt`

For every available three-observation closed triangle, D.RAW now compares:

`direct A→C`

against:

`A→B→C`

over a fixed set of normalized canonical points.

The report exposes closure RMS and maximum discrepancy.

There is deliberately no automatic pass threshold or registration winner. A small loop error is useful evidence for later validation design but is not same-world proof.

### 21. Relative-world feature-track projection v0.1

`RelativeWorldFeatureTrackProjectionV01.kt`

Feature-track members can now be projected through the current relative graph-gauge transforms.

For each track the runtime reports:

- projected members;
- graph-gauge centroid;
- RMS dispersion;
- maximum dispersion;
- missing-transform count.

The graph gauge remains numeric convenience only. Low dispersion does not promote a world point or world registration.

### 22. Geometry validation readiness v0.1

`GeometryValidationReadinessV01.kt`

D.RAW now summarizes, without thresholds:

- number of pair-geometry candidates;
- multi-observation tracks;
- tracks spanning three or more observations;
- closed graph cycles;
- projected feature tracks;
- visible track conflicts;
- descriptive cycle and track residuals.

It also permanently records which later physical gates are still required: overlap, cross-focal-length, rotation, graph loops, held-out observations and world-vs-sensor separation.

### 23. Free World query-support ledger v0.1

`FreeWorldQuerySupportLedgerV01.kt`

Every sealed observation now exposes what it could eventually contribute to a continuous query:

- sample-lattice availability;
- measured sensor-field availability;
- frontside local geometry availability;
- capture-time metadata hint;
- exact-query-support resolver state.

The ledger never fabricates support. Unsupported world queries must remain UNKNOWN.

### 24. Exact source-lattice measured-anchor resolver v0.1

`SourceLatticeExactAnchorResolverV01.kt`

The raster-independent 20-bit lattice now has an executable exact-anchor semantic resolver.

A query can be MEASURED only when:

1. its lattice U/V coordinates are exact multiples of the source-pixel lattice unit;
2. an admitted `MeasuredAnchorProviderV01` returns the exact source coordinate;
3. source coordinate and source SHA binding agree.

Any position between source anchors returns UNKNOWN.

No interpolation is performed.

This is deliberately a **source-lattice** resolver, not a world solver.

### 25. Free World ↔ source-lattice bridge contract v0.1

`FreeWorldSourceLatticeBridgeContractV01.kt`

The boundary between world coordinates and exact source-lattice coordinates is now explicit.

A future world query may reach a measured anchor only through a validated explicit world-to-source relation with uncertainty.

The following are insufficient by themselves:

- appearance pair geometry;
- relative graph gauge;
- camera/lens identity;
- stitched panorama geometry.

Current status remains:

`world_to_source_bridge_admitted=false`

### 26. Calibration Observation Record validator v0.1

`CalibrationObservationRecordValidatorV01.kt`

Optional calibration records now have an executable fail-closed validator.

It checks:

- supported axis scope;
- unique 64-hex source SHA roots;
- one observation role per source root;
- setup description;
- allowed relation-evidence class;
- explicit uncertainty;
- validation status;
- absence of camera/lens/vendor/device-profile identity keys.

A valid record is still only a valid record. Record validation never promotes calibration and never authorizes correction.

### 27. Expanded research promotion firewall

The recursive firewall now also blocks accidental activation of:

- registration promotion;
- world-point estimate promotion;
- deconvolution application;
- inverse-optics authorization;
- automatic colour correction;
- temporal fusion;
- restoration application.

This lets the implementation wave continue further while keeping the scientific output fail-closed.

## Implementation-wave checkpoint

At this point all architecture items previously enumerated for the pre-validation wave have executable runtime code or a typed/runtime contract:

- local geometry;
- pair geometry;
- multi-view tracks;
- graph/cycle diagnostics;
- relative world hypotheses;
- multi-lens/360/stop-motion campaigns;
- Natural Self-Calibration axes;
- world-vs-sensor separation scaffold;
- optics/colour/temporal contracts;
- uncertainty transport;
- exact source-lattice anchor semantics;
- Free World query ABI and world/source bridge boundary;
- restoration authority;
- view/appearance separation;
- calibration record validation;
- promotion firewall.

Physical truth is intentionally **not** promoted merely because the architecture now exists.


### 28. Fail-closed Free World world-query runtime

`FailClosedFreeWorldContinuousQuerySolverV01.kt`

The typed Free World query interface now has a real runtime-safe default implementation.

Until both a validated world→source bridge and an admitted continuous reconstruction operator exist, every world-space query returns:

- `values=null`;
- authority `UNKNOWN`;
- no source support;
- no reconstruction support;
- explicit unknown uncertainty axes for world→source relation and continuous reconstruction.

This means the future query path can already exist in code without silently falling back to interpolation.

### 29. Free World Capability Matrix v0.1

`FreeWorldCapabilityMatrixV01.kt`

The unified foundation export now includes a machine-readable distinction between:

- **implemented machinery**;
- **scientifically validated state**;
- **active scientific use permission**.

The matrix covers geometry, tracks, graph cycles, relative-world gauge, field decomposition, Natural Self-Calibration, optics, colour, temporal relations, exact source anchors, continuous queries, restoration, appearance and the promotion firewall.

Its permanent law is:

> **Implemented does not mean validated. Validated does not automatically mean measured.**

This is specifically intended to prevent future chats or development passes from losing the distinction between code progress and scientific evidence.

## Pre-validation architecture checkpoint

The complete safe architecture set discussed before returning to repeated phone testing is now represented in executable code or explicit runtime contracts.

No phone test is required merely to preserve these ideas in the project.

The next engineering gate is therefore only:

1. integrity checks;
2. full Android compile/build;
3. repair compile/integration errors if any.

Only after that checkpoint does D.RAW need to return to bundled physical validation for scientific promotion.


### 30. Component-aware observation graph partitioning v0.1

`FreeWorldObservationComponentsV01.kt`

Selected files are no longer even implicitly treated as one connected world.

The runtime partitions the candidate pair-geometry graph into deterministic connected components.

Each component records:

- its own source SHA-256 roots;
- number of candidate geometry edges;
- whether it is an isolated observation;
- a deterministic component identity;
- a possible numeric gauge root.

Connectivity remains appearance-derived only.

A connected component does **not** prove:

- one physical scene;
- one capture system;
- one lens;
- one camera;
- one physical world origin.

This is important when a user selects unrelated photographs together, or when only part of a large 360/multi-lens campaign overlaps.

### 31. Independent relative-world gauge per component v0.1

`ComponentRelativeWorldCoordinateHypothesesV01.kt`

Every connected appearance-geometry component now receives its own independent relative numeric gauge hypothesis.

No transform is invented between disconnected components.

Therefore D.RAW no longer needs one global pseudo-world origin for an arbitrary selection set.

The original `RelativeWorldCoordinateHypothesisV01` remains available for compatibility, while the unified foundation uses the component-aware hypothesis for multi-view track projection.

Permanent rules:

- `cross_component_transform_exists=false`;
- `disconnected_components_may_share_numeric_gauge=false`;
- component gauge has no physical-origin authority;
- component connectivity is not same-world proof.

This also strengthens the 360°/multi-lens design: overlap can grow a graph component naturally, while unrelated observations remain separate rather than being forced into the same world.


### 32. Deterministic pair-geometry model bank v0.1

`DeterministicPairGeometryModelBankV01.kt`

The same robust appearance-derived inlier correspondences are now fitted with multiple deterministic geometry families:

- 2D translation;
- 2D similarity;
- 2D affine;
- 2D projective homography.

Every valid candidate reports:

- parameter count;
- parameters;
- valid/invalid projection count;
- RMS residual;
- median residual;
- p95 residual.

No candidate is selected as a winner.

The lowest-residual model is explicitly **not** treated as physical truth, because added model freedom can reduce residuals without proving the corresponding camera/world geometry.

A true 3D pure-rotation/spherical model remains deliberately unimplemented until an admitted intrinsic or equivalent ray-geometry relation exists. Focal-length metadata by itself is not sufficient.

### 33. Pair-geometry model-bank set v0.1

`PairGeometryModelBankSetV01.kt`

The model bank is now applied read-only to every candidate edge in the observation graph and included in the unified foundation export.

This allows later bundled device validation to compare whether ordinary overlap, cross-focal-length overlap, rotation sequences and 360° sequences favour different *candidate* geometry families without changing current authority.

There is no automatic ranking, no correction and no world-registration promotion.


### 30. Bundled physical-validation campaign contract

`BundledPhysicalValidationCampaignV01.kt`

The later physical test campaign is now preserved in code so it cannot be lost between chats.

It contains separate future gates for:

- same-route natural overlap;
- cross-optical-route overlap;
- rotation sequences;
- original-RAW 360° sequences;
- optional controlled field rotations;
- colour under multiple characterized illuminants;
- optical SFR/MTF/PSF support;
- dark/noise/offset observations;
- temporal/rolling-shutter relations.

Every gate starts `performed=false`, `passed=false`, `promoted=false`.

The contract does not require normal users to calibrate their camera and does not use camera/lens identity as the relation key.

### 31. Scientific promotion-gate registry v0.1

`ScientificPromotionGateRegistryV01.kt`

The project now records what future evidence is required before candidate machinery may move toward:

- world registration;
- a world→source bridge;
- field-response calibration;
- colour calibration;
- optical-support calibration;
- dark/noise calibration;
- temporal relations;
- a continuous Free World solver;
- any scientific correction/writeback.

The registry is descriptive and versionable. It never grants promotion automatically.

### 32. Evidence lineage manifest v0.1

`FreeWorldEvidenceLineageManifestV01.kt`

Every foundation export can now bind its independent sealed source SHA roots to the derived graph state.

Derived graph or manifest hashes are explicitly **not** physical evidence roots.

This preserves the rule:

> One Free World may relate many sealed observations without merging their evidence identity.

### 33. Android compiler-memory repair

The first full compile after the architecture expansion exposed a build-system limit rather than a scientific failure: the Kotlin JVM bytecode optimizer exhausted its default CI heap while compiling the already-large D.RAW suite.

`suite_android/gradle.properties` now gives the Suite build one bounded 5 GiB Gradle/Kotlin in-process heap and disables competing Gradle parallel/daemon behavior.

This changes build execution only. It does not alter RAW values, scientific algorithms, authority, runtime precision or export semantics.

## Pre-validation architecture implementation complete

The implementation-first objective is now complete in code.

All major ideas identified for the no-phone-test implementation wave either have:

- executable read-only machinery;
- an explicit typed/runtime contract;
- a fail-closed placeholder that returns UNKNOWN;
- or a machine-readable future validation gate.

The project must now pass one final full Android compile/artifact gate. A successful compile does **not** promote any scientific candidate; it only proves the integrated implementation builds.
