# D.RAW Dark Chroma Stability v0.1 — single-shot frontside audit

Date: 2026-09-28

## Purpose

Dark Chroma Stability v0.1 is an **audit-only**, deterministic frontside research layer for one selected source observation.

It implements this contract:

```
1 sealed camera observation
  -> multiple deterministic frontside analyses
  -> structure / chroma / noise-like constraints
  -> one D.RAWnegative observation binding
```

It does **not** use the other physical lenses, a burst, temporal frames, AI, ML, neural or learned models.

It does **not** change source bytes, Direct CFA, Scientific Master, D.RAWnegative or appearance pixels.

The v0.1 frontside is a helper and veto layer. It can say that a rendered dark region shows visible chroma instability and that a region looks structurally protected. It cannot claim that a fluctuation is sensor noise and cannot choose a replacement scene colour.

## Authority boundary

Every result has authority:

`APPEARANCE_DERIVED_ONLY`

The module explicitly records:

- `source_observation_count = 1`
- `other_physical_lenses_used = false`
- `temporal_frames_used = false`
- `burst_used = false`
- `multi_observation_fusion_allowed = false`
- `drawnegative_target_count = 1`
- `candidate_applied = false`
- `creates_new_evidence = false`
- `scientific_writeback_allowed = false`

A frontside chroma candidate always requires later backside/noise evidence before any correction could ever be considered.

## Deterministic analyses

The frontside preview is already downscaled to at most 384 px on its longest edge. Dark Chroma Stability partitions that same preview into 16 x 16 tiles.

Per tile it measures:

- mean visible luminance;
- luminance standard deviation;
- fraction of dark pixels;
- R-G opponent mean;
- B-G opponent mean;
- opponent-chroma standard deviation;
- chroma/luma standard-deviation ratio;
- edge density from direct neighbour luminance differences;
- mean absolute luminance gradient.

From those measurements it keeps three concepts separate.

### Dark support

A tile can be marked `dark_eligible` from visible-preview luminance only. This is not a scene-radiance or sensor-exposure claim.

### Structure protection

Edges and high luminance variation can set `structure_protected=true`. This is a frontside veto: a potential future chroma stabilizer must not use a flat-region shortcut there.

### Chroma-instability candidate

A dark, locally flat, non-protected tile with sufficiently strong visible opponent-chroma variation can be marked:

`frontside_chroma_instability_candidate=true`

This means only:

> the rendered frontside contains a dark, locally flat region whose visible chroma variation deserves backside verification.

It does not mean:

> this is proven sensor noise.

No replacement RGB/chroma value is calculated in v0.1.

## Heuristic thresholds

v0.1 contains explicit research heuristics for dark-preview classification, flatness, structure veto and chroma variation. They are serialized into the JSON result as:

`RESEARCH_APPEARANCE_HEURISTIC_ONLY`

They are not calibration constants and may not become scientific authority merely because a device test looks good.

## Difference-image principle

The intended next validation stage follows the existing D.RAW A/B/Delta discipline and professional film/VFX practice:

- candidate correction, if ever implemented, remains private;
- the removed component is inspected separately;
- recognizable object edges, lettering, hair, material texture, geometry or highlight structure in Delta cause rejection;
- only chroma instability supported by backside measurement may advance.

v0.1 stops before this correction stage.

## D.RAWnegative relationship

The frontside audit is source-SHA-bound and declares itself bindable only as:

`D.RAWNEGATIVE_DIAGNOSTIC_CONSTRAINT_ONLY`

This does not mutate or upgrade the D.RAWnegative scientific state. It supplies additional same-observation diagnostic context.

## Files

Runtime implementation:

- `DarkChromaStabilityAudit.kt`
- `FrontsideSceneInspector.kt`
- `MainActivity.kt` Universal Intake display

The Universal Intake screen reports tile counts, dark/flat-dark regions, candidate tiles, structure veto count, the audit digest and the single-observation contract.
