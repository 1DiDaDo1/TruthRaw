# D.RAW Free World Radiometric / Noise / Optics / Temporal Integration v0.1

Date: 2026-09-30

Status: **IMPLEMENTED PREVALIDATION ARCHITECTURE — NOT TESTED, NOT BUILT, NOT SCIENTIFICALLY PROMOTED**

Parent checkpoint: PR #101 Free World Observation Geometry Foundation v0.1, exact green head `0006b56d3261cad9a13fec560f7ddcd39b0844db`.

## Purpose

This successor preserves D.RAW's sealed-source evidence law while integrating the multidisciplinary research direction from photography, radiometry, colour science, camera calibration, optics, 3D/animation, illumination, dark/noise measurement, stop-motion/temporal imaging and human visual appearance.

The scientific objective is not a generic denoise filter. It is to explain observation differences by physical or reconstruction cause before any residual may become a noise candidate.

## Permanent universal-input law

D.RAW remains camera-, lens-, vendor- and RAW-format independent at the scientific level.

- RAW/container identity may select a decoder or parser only.
- camera/lens/vendor/physical-camera identity may remain provenance only.
- none of those identities may select a scientific reconstruction model, denoise rule, colour truth, field calibration, optical truth or world geometry.
- sealed source SHA-256 roots and explicitly admitted observation relations remain the scientific identity foundation.
- optional calibration observations are relation-based and never required for normal intake.
- one optical route may share world structure with another without sharing one calibration.
- UNKNOWN remains valid.

## New runtime contracts

### PhysicalObservationNoiseContextV01

Universal Intake now binds every source to a source-SHA-rooted physical noise context.

For supported measured CFA payloads it reuses already-admitted sparse backside summaries (normalized statistics, CFA-phase summary and measured field context). For opaque/unsupported RAWs the object still exists but keeps measured payload support UNKNOWN.

A single observation is explicitly forbidden from turning its observed standard deviation, chroma variation, CFA-phase difference or field variation into a noise/lens/sensor claim.

### RadiometricResponseAtlasV01

Separates:

- focal-plane exposure relation;
- effective gain;
- black/offset behavior;
- saturation/censor bounds;
- response linearity;
- OECF or equivalent controlled response evidence.

Permanent boundaries:

- BlackLevel != Zero-Line;
- metadata ISO != measured gain;
- exposure metadata does not prove linearity;
- WhiteLevel alone does not prove a scene clipping point;
- no radiometric correction is enabled.

### NoiseComponentAtlasV01

Declares separate future components:

- photon shot;
- read;
- dark-current shot;
- DSNU/dark fixed pattern;
- PRNU/signal-dependent fixed pattern;
- row/column or correlated readout pattern;
- quantization;
- CFA-phase response;
- spatial-frequency/NPS-like residual;
- UNKNOWN residual.

Temporal noise is not fixed-pattern noise. Metadata NoiseProfile is not a complete noise calibration. Frontside texture may not be declared noise merely because it is irregular.

### ScientificNoiseTransportV01 + ScientificNoiseMathV01

Future scientific transforms must preserve uncertainty rather than only values.

Deterministic numeric primitives are now implemented for:
- scalar variance propagation through gain;
- 3x3 covariance propagation through scalar gain;
- 3x3 linear/Jacobian covariance transport `C_out = J C_in J^T`;
- diagonal covariance construction;
- bounded inverse-transfer noise-gain calculation that fails closed near zero optical transfer.

The primitives transform admitted uncertainty only. They do not synthesize missing covariance, classify residuals as noise or authorize deconvolution.

Contracted representations include:

- per-channel variance;
- cross-channel covariance;
- spatial covariance/kernel;
- noise power spectrum;
- CFA-phase-conditional uncertainty.

For a scalar gain, variance transport follows the `gain^2` relation. A linear colour transform must transport covariance. Reconstruction adds reconstruction uncertainty. Inverse optics must account for frequency-dependent noise amplification.

No numeric covariance is synthesized in this version.

### Optical support and inverse optics

The existing Optical Support Atlas remains the authority source for SFR/MTF/PSF, field dependence, chromatic displacement, field curvature and focus state.

New rule:

> Output raster density is not optical resolution.

Near-zero optical transfer may not be blindly inverted. Deconvolution requires both admitted optical support and admitted noise transport.

### TemporalFootprintV01

Separates:

- exposure integration duration;
- capture-time metadata hint;
- sequence order;
- readout start/end;
- rolling-shutter row time;
- physical motion path;
- occlusion-time relation.

An exposure is not treated as an instantaneous world sample merely because it has one timestamp. Synthetic or interpolated frames never become physical observations.

### GeometryDepthSupportAtlasV01

Keeps existing 2D appearance geometry separate from future:

- intrinsic ray model;
- camera pose;
- parallax;
- depth;
- surface normal;
- visibility;
- occlusion;
- non-rigid state.

Photogrammetric/geometric confidence cannot upgrade radiometric authority.

### LightTransportAuthorityContractV01

Carries forward the older Deep Scene / Light Transport separation:

- observed radiometry;
- geometry/normal;
- material;
- illumination;
- spectral hypothesis;
- visibility

remain separate authority axes.

A physically plausible renderer is not a measurement system. RGB is not a spectrometer. A Lambertian model can be a bounded hypothesis but does not prove a real surface is Lambertian.

### MultiObservationResidualRelationV01

A read-only multi-observation residual scaffold now binds independent source roots and their source-level physical noise contexts. No alignment or residual calculation occurs until world-to-source, radiometric, temporal, visibility/occlusion and uncertainty relations are admitted.

### ScientificDenoiseAdmissionV01

Scientific denoise now has three separate future admission routes:

1. single-frame component-aware;
2. optics-aware / inverse-optics;
3. multi-observation world-space.

The single-frame route does not require 3D or multiple frames. Stronger routes require the extra physical evidence they actually use. All routes remain blocked in this branch.

### WorldSpaceNoiseSeparationV01

Future multi-observation decomposition distinguishes candidate classes:

- world-fixed radiometric structure;
- sensor-fixed pattern;
- temporal random residual;
- view-dependent reflection/specular behavior;
- motion/occlusion;
- optical-field dependence;
- UNKNOWN residual.

This route stays disabled until world-to-source relations, noise calibration, multiple independent observations and held-out validation exist.

### PerceptualNoiseAppearanceV01

Human-vision/noise visibility is explicitly downstream.

Viewing distance, display luminance/black, pixel density, ambient illumination, adaptation, output size and spatial-frequency/channel sensitivity may influence APPEARANCE only.

Perceptual denoise may never rewrite Scientific Master or become MEASURED evidence.

## Existing PR101 modules strengthened

The following existing runtime contracts now reference the new architecture:

- NaturalSelfCalibrationAtlasV01;
- FreeWorldObservationGeometryFoundationV01;
- FreeWorldCapabilityMatrixV01;
- BundledPhysicalValidationCampaignV01;
- ScientificPromotionGateRegistryV01;
- ResearchPromotionFirewallV01;
- FreeWorldUncertaintyTransportV01;
- ColourRelationAtlasV01;
- OpticalSupportAtlasV01;
- TemporalObservationRelationV01;
- CalibrationObservationRecordV01 and validator;
- UniversalObservationCalibrationAtlasV01;
- FreeWorldContinuousQueryContractV01;
- FailClosedFreeWorldContinuousQuerySolverV01;
- FreeWorldQuerySupportLedgerV01;
- ViewAppearanceStateV01.

## New physical validation gates

No gate was executed in this branch.

Added future gates include:

- radiometric response / linearity;
- noise-component separation;
- geometry/depth/visibility;
- world-space residual separation.

Existing field, colour, optics, dark/noise and temporal gates remain.

## Promotion firewall

The recursive research firewall now additionally rejects accidental activation of:

- radiometric calibration promotion;
- noise-component calibration promotion;
- geometry promotion;
- automatic radiometric correction;
- scientific noise reduction;
- world-space denoise;
- light-transport application to Scientific Master.

Existing prohibitions on world registration, calibration, correction, deconvolution, colour correction, temporal fusion, restoration, evidence creation and scientific writeback remain.

## Scientific denoise direction

The intended future chain is:

```
sealed physical observations
  -> radiometric response context
  -> component-separated noise model
  -> optical support
  -> world/sensor separation
  -> temporal/multi-view support
  -> geometry/depth/visibility where proven
  -> reconstruction + axis-separated uncertainty
  -> scientific colour with uncertainty transport
  -> downstream perceptual appearance
```

The intended logic is:

> Do not first ask which pixels look noisy. First ask which part of the observation can be explained by world structure, sensor-fixed response, optical support, temporal sampling, view dependence, motion/occlusion, reconstruction uncertainty or known stochastic components. Only the unresolved supported residual may become a noise candidate.

## Completed implementation checkpoint

Code checkpoint before documentation updates: `caf4c0e39f3a74c94ed00802ff8a84c1f10013d1` — 115 commits ahead of the exact green PR101 parent `0006b56d3261cad9a13fec560f7ddcd39b0844db`.

The recommendation wave is now implemented end-to-end as **candidate runtime**, not as scientific promotion:

- Android can optionally import one or more Calibration Observation Record JSON files/bundles.
- normal RAW intake remains independent of those records;
- every record is recursively checked for forbidden camera/lens/vendor/RAW scientific identity keys;
- record validity does not equal numerical relation admission;
- `USER_GROUPING_HINT_ONLY` and `NONE` cannot drive numeric candidate solvers;
- explicit admitted relation records can feed radiometric, field, colour, optics, dark/noise, NPS, temporal, geometry and world-space candidate runtimes;
- the measured backside audit exports an exact sparse CFA grid with source x/y, CFA phase and normalized measured value;
- repeated admitted DARK/FLAT/scene relations can produce temporal/fixed-pattern summaries and a signal-dependent sparse-grid noise-model candidate;
- radiometric candidates can be fitted from explicit measurement points or controlled RAW-profile exposure relations;
- optical SFR/MTF/PSF observations, colour 3x3 relation candidates, temporal sequence/readout candidates and calibrated-ray 3D triangulation candidates are executable;
- controlled rotation can produce a world-vs-sensor field candidate without claiming lens-only vignetting;
- world-space residual candidates preserve world-fixed, sensor-fixed, temporal/view/motion and UNKNOWN separation boundaries;
- deterministic covariance/variance transport and noise-aware inverse-optics gain candidates are available;
- exact measured anchors pass through unchanged; non-anchor reconstruction can only become `RECONSTRUCTED_CANDIDATE`;
- the scientific denoise operator remains gated and cannot modify Scientific Master;
- perceptual noise visibility remains VIEW/APPEARANCE only;
- older executable Deep Scene / Light Transport research is referenced under the current authority firewall instead of being silently promoted.

### Android relation-record path

The Multi-observation panel now contains an optional JSON picker for Calibration Observation Records. Imported valid records are retained separately from sealed RAW observations and are passed into the Free World Foundation export. The record set is re-read when the export is committed so a stale report cannot silently survive a changed relation set.

This path does **not** make calibration mandatory for ordinary users. It exists only to let controlled or explicitly related physical observations reach the candidate solvers.

### Validation status

Per explicit user instruction, this branch has not been compiled, tested, sent through CI, built as an APK, or device-validated after this implementation wave. Therefore no claim is made that the new branch compiles or that any candidate model is physically correct. The parent PR101 green checkpoint remains the last tested baseline.


## Test/build status

By explicit instruction, this implementation was committed **without running tests, CI, Android build, device validation or promotion experiments**.

Therefore:

- implementation status may be reported;
- compilation success may NOT be reported;
- physical correctness may NOT be reported;
- no scientific promotion is implied.


## Completed numeric candidate implementation wave

The multidisciplinary recommendations are now represented by executable, fail-closed candidate runtimes rather than contracts alone.

Implemented candidate runtimes:

- `RadiometricResponseCandidateSolverV01`: deterministic linear/piecewise response candidate with held-out residual reporting.
- `RadiometricProfileRelationCandidateV01`: builds radiometric candidates directly from controlled sealed RAW profile relations using exposure time plus measured backside signal while refusing to treat ISO metadata as measured gain.
- `NoiseComponentDecompositionCandidateV01`: shot/read variance candidate plus fixed-pattern summary candidates.
- `RepeatedSparseGridNoiseCandidateV01`: direct repeated-frame comparison of exact sparse measured CFA coordinates for DARK/FLAT/REPEATED_SCENE relation sets.
- `NoiseSpectrumMeasurementCandidateV01`: controlled NPS/spatial-frequency measurement ingestion.
- `FieldResponseRotationSeparationCandidateV01`: robust additive world-cell versus sensor-cell field candidate under explicitly admitted controlled rotation relations.
- `OpticalSupportMeasurementCandidateV01`: SFR/MTF/PSF candidate measurement ingestion with field/focus context.
- `NoiseAwareInverseOpticsCandidateV01`: bounded Wiener-like frequency-gain candidate using explicit transfer, signal PSD and noise PSD; it never transforms an image or authorizes deconvolution.
- `ColourRelationCandidateSolverV01`: identity-independent 3x3 camera-RGB to reference-XYZ candidate fitted over at least two illuminants with held-out patches.
- `TemporalSequenceCandidateSolverV01`: explicit sequence/readout candidate from relation records; metadata timing alone remains insufficient.
- `GeometryDepthCandidateSolverV01`: calibrated-ray triangulation candidate from supplied pose/ray relations; geometry authority remains separate from radiometry.
- `WorldSpaceResidualCandidateSolverV01`: world-fixed plus sensor-fixed residual candidate decomposition after admitted world/source and radiometric relations.
- `ScientificReconstructionCandidateV01`: uncertainty-weighted reconstructed-value candidate with exact MEASURED anchor passthrough.
- `ScientificDenoiseOperatorV01`: route-gated derived-output operator; it cannot run scientifically until the corresponding admission gate is promoted.
- `PerceptualNoiseVisibilityCandidateV01`: appearance-only conversion from cycles/pixel to cycles/degree and optional use of an explicitly validated appearance sensitivity curve.
- `ResearchMathV01` and `ScientificNoiseMathV01`: deterministic fitting, matrix, variance and covariance primitives.

The existing backside audit now also exports the exact sparse measured sample grid:
`source_x, source_y, cfa_phase, normalized_above_black`.
No interpolation is performed. This enables later relation-based repeated-frame statistics at identical sensor coordinates.

Quantization context is now derived from the source coding bounds as normalized code-step candidates, but is explicitly not treated as a complete noise model.

## Optional relation-record ingestion

`CalibrationObservationRecordBundleV01` accepts JSON observation-record bundles, validates every record fail-closed, and exposes only valid records to:

`FreeWorldObservationGeometryFoundationV01.buildWithCalibrationBundle(...)`

Normal RAW intake does not require such a bundle. Camera, lens, vendor and RAW-format identity remain forbidden as scientific calibration/model-selection keys.

## Important state after this implementation wave

All numeric outputs above remain **CANDIDATE / UNVALIDATED**.

In particular, the implementation does not set any of the following true:

- radiometric calibration promoted;
- field-response calibration promoted;
- noise-component calibration promoted;
- optical-support calibration promoted;
- colour calibration promoted;
- temporal relation promoted;
- geometry promoted;
- world-space noise separation promoted;
- scientific denoise admitted;
- deconvolution authorized;
- correction authorized;
- Scientific Master writeback allowed.

The research firewall explicitly rejects those promotion/application flags in the current foundation export.
