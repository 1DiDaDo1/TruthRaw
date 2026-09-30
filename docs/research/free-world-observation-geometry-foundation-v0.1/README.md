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
