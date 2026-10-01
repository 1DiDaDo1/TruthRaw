# D.RAW main project state — 2026-10-01

## Active continuation checkpoint

Continuation code:

`44485`

Repository:

`1DiDaDo1/TruthRaw`

Active branch:

`research/registration-aware-rotation-audit-v01-2026-10-01`

Open PR:

`#104 — Add constrained rotation, measured support, and optical-field topography audits`

Android/scientific source-code checkpoint:

`faac21e2e2fe97477c846609b8dc4e71af29b3c1`

The repository documentation head may be newer than this hash. That does not imply a different APK or scientific source implementation.

## Current scientific direction

D.RAW is no longer treating the source raster or a flat 12x12 field chart as the final geometric representation.

The current research direction is:

`sealed CFA/source evidence -> measured source support -> optical-field coordinates -> diagnostic topography -> future continuous optical/ray-space bridge -> Free World reconstruction`

The project still refuses to create information merely by changing representation.

A finer or more flexible coordinate representation may expose or organize existing measured support more effectively, but it does not create new photons, new measured samples or new independent evidence.

## Controlled-rotation experiment

The active controlled relation contains four observations:

- 0° TRAIN
- 90° TRAIN
- 180° TRAIN
- 270° HELD_OUT

The explicit relation record remains the experiment authority.

Appearance-derived constrained geometry may estimate residual rotation, translation and uniform scale, but it may not overwrite the relation record or become physical field truth automatically.

## Key empirical result

The first component-isolation audit established:

- residual rotation alone: no current 12-sector cell change and same RMSE as nominal;
- scale alone: slightly worse;
- translation: small training gain with held-out loss;
- rotation+translation: same pattern;
- full residual similarity: large training gain with large held-out loss.

Therefore the project currently treats **coarse coordinate bridging + hard re-binning** as the weak link under investigation.

It does not interpret lower training error as proof that the full similarity is physically correct.

## Measured Field Support Coordinate Bridge v0.1

The project now uses the existing sparse measured RAW source-grid positions to reconstruct the spatial support of each aggregate field cell.

For every populated radial/azimuth support cell the runtime exports:

- measured source-point count;
- derived source-space centroid;
- isotropic centroid;
- rho and azimuth;
- deviation from the theoretical bin centre;
- footprint width/height;
- RMS footprint radius.

Important authority distinction:

**measured sparse points = source evidence**

**support centroid / footprint = derived diagnostic**

Photometric values are not used to construct the support geometry.

## Optical Field Topography v0.1

The measured-support field can now be represented as X/Y with a selectable Z observable.

Available Z layers:

- relative signal EV;
- nominal-model signed residual EV;
- nominal-model absolute residual EV;
- support point count;
- centroid offset from theoretical bin centre;
- support footprint RMS.

View semantics:

- top-down;
- side-X;
- side-Y;
- oblique.

This implements the idea that a side view can reveal ridges, basins, local peaks and discontinuities that are hard to recognize in a flat map.

## Physical interpretation boundary

A topographic Z value is **not automatically a physical height**.

It does not automatically prove:

- object depth;
- literal curvature/sag of lens glass;
- field curvature;
- distortion;
- vignetting;
- optical aberration;
- sensor angular response;
- scene illumination.

Those interpretations require their own measured/calibrated evidence axes.

## Permanent architecture

Still mandatory:

- sealed source evidence remains immutable;
- MEASURED, RECONSTRUCTED and APPEARANCE stay distinct;
- Scientific Master is separate from export/presentation;
- camera/lens/vendor/RAW identity cannot select scientific truth;
- no AI/ML/neural/generative runtime;
- held-out data cannot train or select the tested bridge;
- no automatic model/problem winner;
- no correction or scientific writeback from these audits.

## Current green build

Latest APK-producing workflow:

`36831352713`

Artifact:

`11146499757`

APK:

`DRAW_optical_field_topography_v0_1_debug_arm64.apk`

Bytes:

`8,368,975`

SHA-256:

`a892a9220c554f5ad655cfaffe23f580dbba37f8eba1ec9fe34f2fe9c00571d3`

The following gates were green at the source-code checkpoint:

- Free World Observation Geometry Foundation v0.1 Integrity
- Canonical Integrity
- Tile-Native DNG Source v0.2 Android Compatibility
- D.RAW Suite Universal Intake v0.1
- D.RAW Universal Physical Capture v0.3 Live Preview Macro
- D.RAW Android DngCreator Compatibility v0.1

## Immediate next device test

Do not make a new capture yet.

Reuse:

- the same four controlled-rotation RAWs;
- the same immutable Calibration Observation Record.

Export:

**Free World Observation Geometry Foundation v0.1 JSON**

The expected new research sections are:

- `measured_field_support_coordinate_bridge_audit`
- `optical_field_topography_audit`

The next analysis should ask whether residual-error ridges align with support-centroid displacement, support footprint or sparse-support density, while keeping correlation and visual alignment descriptive rather than causal proof.

## Recovery

If a future chat receives only:

`44485`

it should recover this state first and continue from the measured-support/topography validation frontier rather than restarting older coarse bin-centre experiments.
