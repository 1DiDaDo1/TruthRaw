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

### ScientificNoiseTransportV01

Future scientific transforms must preserve uncertainty rather than only values.

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

## Test/build status

By explicit instruction, this implementation was committed **without running tests, CI, Android build, device validation or promotion experiments**.

Therefore:

- implementation status may be reported;
- compilation success may NOT be reported;
- physical correctness may NOT be reported;
- no scientific promotion is implied.
