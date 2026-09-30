# D.RAW Field Response Repeatability v0.1

Date: 2026-09-30

Status: **research-only, read-only, descriptive multi-observation audit, no calibration/correction/writeback**

## Purpose

This is the first axis-specific experiment built on:

- PR96 Observation Optical Field Chart v0.1;
- PR97 Universal Observation & Calibration Atlas v0.1;
- PR98 Universal Multi-Observation Relation Protocol v0.1.

The goal is to ask a narrow question:

> When at least three independent measured field observations are expressed in the same PR96 rho/azimuth/radial/tangential coordinate system, how repeatable is the *shape* of the observed field signal after removing only one scalar level per observation?

This is not a lens-vignetting calibration.

It is not a camera-profile lookup.

It does not prove that the observations came from the same physical camera or lens.

## Input

The Android UI operates on the currently selected sources.

The user can select multiple RAW files and choose:

**Analyseer alle geselecteerde bronnen universeel**

The experiment becomes exportable only when at least three selected observations have:

- a PR96 `FIELD_CHART_AVAILABLE`;
- a `MEASURED_COMPOSITE_FIELD_SIGNAL_AVAILABLE`;
- distinct sealed source SHA-256 roots.

Opaque/unsupported RAW sources remain valid Universal Intake observations, but they do not contribute measured field charts until their backside topology is scientifically admitted.

## Why three observations

PR98 freezes three independent observations as the minimum for a first field-repeatability candidate.

Three is still not enough to identify a physical cause. It is only enough to stop treating one scene as the entire basis of a repeatability statement.

## Per-observation normalization

For every eligible observation, v0.1 reads the twelve annular PR96 p50 measurements.

Only positive finite annular values participate in the scalar reference:

```text
S = median(positive annulus p50 values)
```

The radial shape is then represented as:

```text
R_bin = log2(annulus_p50 / S)
```

This removes one scalar brightness/exposure-like level from each observation.

It does **not** remove scene content.

It does **not** use ExposureTime, ISO, f-number, white balance or DNG GainMap.

## Radial repeatability

For each of the 12 radial annuli, the report records:

- valid observation count;
- cross-observation median relative signal in EV;
- cross-observation median absolute deviation (MAD) in EV.

It also reports pairwise Pearson correlation between complete radial shape vectors where enough common finite bins exist.

These are descriptive metrics only.

No threshold chooses a winner and no automatic pass/fail promotes a camera-system response.

## Azimuth repeatability

Within each annulus and sector:

```text
A_bin,sector = log2(sector_p50 / annulus_p50)
```

Across observations, v0.1 records the median and MAD for every radial/azimuth cell.

This is useful because a radial pattern with unstable azimuth structure can indicate strong scene dependence.

It still does not identify the cause.

## CFA-phase repeatability

For every CFA phase and radial annulus:

```text
P_phase,bin = log2(phase_p50 / annulus_p50)
```

The report records cross-observation median and MAD.

This creates a future observation basis for studying position-dependent CFA/sensor response without calling it lens-only behaviour.

## Relation authority

The current Android set-level audit uses:

`USER_GROUPING_HINT_ONLY`

with authority:

`NON_AUTHORITY_GROUPING_HINT`

That is deliberate.

Selecting three files together is not proof that they came from the same physical camera or lens.

A later controlled experiment may attach stronger PR98 relation evidence such as:

- `SEALED_CAPTURE_SESSION_PROVENANCE`;
- `EXPLICIT_CALIBRATION_CAPTURE_RECORD`.

v0.1 does not silently create either.

## Interpretation boundary

Even strong repeatability may result from repeated scene structure, acquisition conditions or other coupled effects.

Therefore the output always keeps:

- `repeatable_camera_system_response_proven=false`;
- `lens_only_vignetting_proven=false`;
- `scene_illumination_separated=false`;
- `sensor_angular_response_separated=false`;
- `optical_axis_proven=false`;
- `calibration_promoted=false`;
- `correction_gain_allowed=false`.

## Android workflow

On compact/mobile, medium and expanded layouts there is a new:

**Multi-observation · Field Response v0.1**

panel.

It reports:

- number of selected sources;
- number already universally profiled;
- number with measured PR96 field charts.

Actions:

1. **Analyseer alle geselecteerde bronnen universeel**
2. **Export Field Response Repeatability v0.1 · JSON**

The export button remains disabled until at least three measured field charts exist.

The export re-checks the source-SHA set before writing. Changing the observation set while the document picker is open blocks the write.

## Permanent safety

- no camera identity required;
- no lens identity required;
- no vendor mapping;
- no device profile key;
- no AI/ML/neural/generative runtime;
- no ExposureTime/ISO/f-number normalization;
- no white-balance normalization;
- no DNG GainMap application;
- no source sample mutation;
- no sample-position mutation;
- no new measured samples;
- no calibration promotion;
- no correction gain;
- no Scientific Master writeback.

## Next physical gate

Use at least three substantially different real DNG scenes that each produce a measured PR96 field chart.

Export one `Field Response Repeatability v0.1` JSON.

The first device result should be interpreted descriptively. Thresholds for a stronger controlled relation experiment must not be invented from the same three scenes after seeing the answer.
