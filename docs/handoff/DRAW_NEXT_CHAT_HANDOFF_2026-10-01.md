# D.RAW next-chat handoff — 2026-10-01

## Continuation code: 44485

When a new chat receives only **44485**, it must treat that code as a request to restore the complete D.RAW/TruthRaw project context through this checkpoint before making further scientific or code changes.

The code is a recognition anchor, not a magical transport mechanism. The durable recovery source is this repository state plus the current project-state JSON and the latest source branch.

## Current repository checkpoint

Repository: `1DiDaDo1/TruthRaw`

Active branch:

`research/registration-aware-rotation-audit-v01-2026-10-01`

Open PR:

`#104 — Add constrained rotation, measured support, and optical-field topography audits`

Latest Android/source-code head covered by this handoff:

`4fbc9767d178903ffcf70f5318b9b0706dadf187`

Documentation commits may advance beyond that hash without changing the Android/scientific source checkpoint. Use the source-code head above when comparing APK behavior.

## Latest green Android build

Workflow:

`D.RAW Suite Universal Intake v0.1`

Run:

`36840564859`

Artifact:

`11151017383 — DRAW_Full_Suite_Universal_Intake_v0.1_debug_arm64`

Artifact ZIP digest:

`sha256:9c0221c0bcd91467a65923dbdf1068847cb88169f5db18f5dc71f9930d17c0ab`

Extracted APK:

`DRAW_research_multiraw_resilience_fix_v0_1_debug_arm64.apk`

APK bytes:

`8,368,975`

APK SHA-256:

`35e82830ace915c01328241819ed584d24d82048427d6ae86a36dade4bac02c2`

Green gates at this source head:

- Free World Observation Geometry Foundation v0.1 Integrity — run `36831352861`
- Canonical Integrity — run `36831352840`
- Tile-Native DNG Source v0.2 Android Compatibility — run `36831352745`
- D.RAW Suite Universal Intake v0.1 — run `36831352713`
- D.RAW Universal Physical Capture v0.3 Live Preview Macro — run `36831352655`
- D.RAW Android DngCreator Compatibility v0.1 — run `36831352685`

## Read first

1. `state/CURRENT_PROJECT_STATE_2026-10-01.json`
2. this handoff
3. `START_HERE_NEW_CHAT.md`
4. `docs/research/optical-field-topography-v0.1/README.md`
5. `docs/research/measured-field-support-coordinate-bridge-v0.1/README.md`
6. `docs/research/free-world-observation-geometry-foundation-v0.1/README.md`
7. older dated handoffs only for historical provenance

## Permanent scientific laws

- **MEASURED != RECONSTRUCTED != APPEARANCE**
- **Seal the evidence, not the thinking.**
- **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
- Camera/lens/vendor/RAW identity may route parsing or decoding, but may not select scientific truth.
- Held-out data may evaluate a frozen candidate; it may not fit or select it.
- A lower training residual is not physical truth.
- D.RAW uses no AI/ML/neural/generative runtime.
- Scientific Master writeback from this research wave is forbidden.
- A derived topographic height is not physical scene depth or literal lens-surface curvature unless separately proven.

## Research workbench resilience fix

The 2026-10-01 device test exposed a practical failure mode: pressing **Analyseer alle geselecteerde bronnen universeel** could return to the Research & JSON hub and the four selected RAW handles were then gone.

The code had two concrete vulnerabilities matching that symptom:

1. the selected `RawJob` list lived only in `MainActivity` memory and was not restored after Activity/process recreation;
2. the batch button started every `UniversalSourceProfiler` in parallel with `force=true`, while the first source could already be profiling automatically, allowing duplicate heavy work and an avoidable memory-pressure spike.

The fix at source head `4fbc9767d178903ffcf70f5318b9b0706dadf187`:

- adds `ResearchWorkbenchSessionStoreV01`, storing lightweight URI/metadata handles only — never RAW payload bytes;
- restores the selected research RAW set after recreation/re-entry;
- profiles multi-source research sets sequentially;
- joins an already-running source profile via completion waiters rather than starting a duplicate;
- suppresses automatic heavy preview/profile startup for multi-source research selections so the explicit batch operation owns the workload.

This is an engineering resilience fix only. It changes no scientific authority, evidence, calibration, geometry, topography or promotion state.

## Current controlled-rotation campaign

The active experiment uses the same four DNG observations throughout:

| Capture | Role | Source SHA-256 |
| --- | --- | --- |
| 2026:10:01 01:51:08 | 0° TRAIN | `f2e25aa807598f3a1cd6dfa321535abb5f7350f4b2511e0eecfcff79b221ca29` |
| 2026:10:01 01:51:36 | 90° TRAIN | `58e6595108be099074ac88c9e237f19a4fb6cf336299cab3e0110afda4adfebb` |
| 2026:10:01 01:52:07 | 180° TRAIN | `d46047482f444e42eabbab2ad016b73391825cf85f9de7cfaee97ba291655fa3` |
| 2026:10:01 01:52:23 | 270° HELD_OUT | `4f9d38389816ab04f2d2530e1e45d9b3224824b1ef626f75b45c818c7d3a38e2` |

Selected quarter-turn shift sign:

`-1`

The Calibration Observation Record remains immutable and identity-bound. Derived diagnostic relabels are in-memory only and are not replacement calibration records.

## Constrained rotation geometry

The current constrained geometry model:

- uses the explicit 0/90/180/270 relation as the experiment constraint;
- estimates residual rotation, translation and uniform scale only;
- forbids shear, anisotropic scale, reflection and projective terms;
- uses deterministic appearance-derived feature support;
- does not promote world registration;
- does not authorize image transformation or correction.

The earlier 270° failure was recovered with a denser diagnostic-only rotation-support feature set. That support path does not replace the primary pair geometry.

## Coordinate-bridge result already learned

The first field-coordinate bridge audit decomposed the appearance similarity into independent diagnostic components on an identical predictable sample set.

Exact common-predictable sample support:

- training: `378`
- held-out: `122`

RMSE in EV:

| Variant | Train | Held-out |
| --- | ---: | ---: |
| Nominal relation | 0.3115382851 | 0.4952042896 |
| Residual rotation only | 0.3115382851 | 0.4952042896 |
| Uniform scale only | 0.3146035982 | 0.4979402780 |
| Translation only | 0.3015349201 | 0.5187642227 |
| Residual rotation + scale | 0.3146035982 | 0.4979402780 |
| Residual rotation + translation | 0.3005933060 | 0.5190954674 |
| Full residual similarity | 0.1995020644 | 0.7163537541 |

Interpretation:

- residual rotation alone does not move any current 30° sector bins and produces the same RMSE as nominal;
- scale alone is slightly worse;
- translation modestly improves training but worsens independent hold-out;
- full residual similarity greatly improves training and substantially worsens hold-out;
- therefore the current weak point is the source-field/frontside coordinate bridge plus hard re-binning, not evidence that the controlled rotation relation itself should be replaced.

No automatic winner was selected.

## Measured Field Support Coordinate Bridge v0.1

Current implementation:

`MeasuredFieldSupportCoordinateBridgeAuditV01.kt`

This successor removes the assumption that a field-cell's geometric support is located only at the theoretical polar bin centre.

It reads the existing:

`BacksideSignalSupportAudit.sparse_measured_sample_grid`

and reconstructs the support of each `(radial_bin, sensor_sector)` cell from actual measured source-grid `source_x/source_y` positions using the exact same 12x12 field binning used by the measured optical-field signal.

For each support cell it exposes:

- point count;
- source-space centroid;
- isotropic centroid;
- rho and azimuth;
- centroid displacement from the theoretical bin centre;
- source-pixel footprint width/height;
- RMS footprint radius.

Authority boundary:

- individual sparse source positions are measured source evidence;
- the support centroid and footprint are derived;
- photometric values are not used to construct support geometry;
- source-grid -> frontside-isotropic mapping is still unproven;
- decoder crop/orientation binding is still unproven;
- no correction or promotion follows from the audit.

Device result for this new measured-support successor is still pending.

## Optical Field Topography v0.1

Current implementation:

`OpticalFieldTopographyAuditV01.kt`

The purpose is to inspect the same optical-field data not only from above but as a topographic surface whose Z-height can be changed between independent observables.

Coordinate plane:

`MEASURED_SUPPORT_CENTROID_UNIT_DISK`

Current height layers:

- `RELATIVE_SIGNAL_EV`
- `NOMINAL_MODEL_SIGNED_RESIDUAL_EV`
- `NOMINAL_MODEL_ABS_RESIDUAL_EV`
- `SUPPORT_POINT_COUNT`
- `SUPPORT_CENTROID_OFFSET_NORMALIZED`
- `SUPPORT_FOOTPRINT_RMS_SOURCE_PX`

Current view semantics:

- `TOP_DOWN`
- `SIDE_X`
- `SIDE_Y`
- `OBLIQUE`

The audit also reports descriptive Pearson associations, separately for TRAIN and HELD_OUT, between nominal absolute residual and:

- support-centroid offset;
- support footprint;
- support count.

These correlations are descriptive only. Correlation is not causation, is not a scientific promotion threshold, and does not select a problem cause automatically.

The current APK exports the topographic data and view semantics in JSON. An interactive on-screen 3D renderer is not yet implemented; that should come only after the exported scientific topology is validated.

### Topography authority rule

A visible peak, basin, ridge or slope means only that the chosen observable varies over field position.

It is **not** automatically:

- physical scene depth;
- lens glass surface sag;
- field curvature;
- vignetting;
- distortion;
- aberration;
- sensor angular response;
- scene illumination.

Those physical interpretations require their own evidence axis.

## Why the topographic view exists

The user proposed an important diagnostic idea:

- top-down field maps show where a quantity is located;
- side/oblique views make height differences, ridges and local discontinuities much easier to spot;
- aligning ridge locations across independent layers may help localize a shared problem faster.

D.RAW now preserves that idea in code without converting visual alignment into physical proof.

The intended next experiment is to rerun the same four RAWs and same Calibration Observation Record, export a fresh **Free World Observation Geometry Foundation v0.1 JSON**, and inspect the new:

- `measured_field_support_coordinate_bridge_audit`
- `optical_field_topography_audit`

sections.

## Safety state

Current research output must remain:

- `automatic_variant_winner_selected=false`
- `automatic_problem_cause_selected=false`
- `world_registration_promoted=false`
- `field_response_calibration_promoted=false`
- `correction_authorized=false`
- `candidate_applied=false`
- `image_transform_applied=false`
- `source_sample_values_modified=false`
- `source_sample_positions_modified=false`
- `creates_new_evidence=false`
- `scientific_writeback_allowed=false`

The promotion firewall must remain clean.

## 44485 recovery instruction

A future chat receiving **44485** should not ask the user to reconstruct this context manually.

It should first recover the current repository state and this handoff, verify the latest branch/head/build if further code work is requested, preserve all authority boundaries above, and continue from the pending measured-support/topography device validation.

Do not redo the older coarse bin-centre experiment as if it were the current frontier. It remains useful historical diagnostic evidence, but the current frontier is measured source support plus optical-field topography.
