# D.RAW Dark Chroma Stability v0.2 — information-support gate

Date: 2026-09-28

## Why v0.2 exists

Dark Chroma Stability v0.1 correctly stayed audit-only, but a real-device near-black tele test exposed an important ambiguity.

Observed v0.1 frontside result:

- 432 / 432 tiles dark
- 432 / 432 tiles flat-dark
- 432 / 432 tiles visible chroma-instability candidates
- 0 structure-veto tiles
- frontside edgeDensity about 0.0007
- frontside entropy about 1.04 bits

That result means the rendered frontside is dominated by visible chroma instability. It does **not** mean the hidden scene colour is known.

The successor v0.2 therefore adds an explicit information-support gate without changing v0.1.

## Immutable predecessor

`DarkChromaStabilityAudit.kt` remains v0.1.

v0.2 is implemented separately in:

`DarkChromaStabilityV02Audit.kt`

The v0.2 result binds to the exact v0.1 source SHA and schema. v0.1 candidate logic is not silently rewritten.

## State model

Per tile v0.2 can emit:

- `STRUCTURE_PROTECTED`
- `DARK_UNINFORMATIVE`
- `CHROMA_INSTABILITY_VISIBLE`
- `NO_CHROMA_INSTABILITY`

`CHROMA_CORRECTION_SUPPORTED` is intentionally **not available from frontside analysis alone**.

A future correction requires the intersection:

```
v0.1 visible chroma instability
AND enough same-observation information support
AND no frontside structure veto
AND local backside/noise support bound to the same observation
```

Until the last term exists, the state is `BACKSIDE_CONFIRMATION_PENDING`.

## Global DARK_UNINFORMATIVE gate

v0.2 detects the specific failure mode shown by the full-dark tele observation:

- almost all tiles are dark;
- frontside entropy is extremely low;
- frontside edge density is extremely low.

The current research thresholds are serialized into the audit and have authority:

`RESEARCH_APPEARANCE_HEURISTIC_ONLY`

They are not calibration constants.

The observed full-dark tele profile is expected to cross this gate:

```
dark tile fraction = 1.0
entropy ~= 1.04
edgeDensity ~= 0.0007
=> DARK_UNINFORMATIVE
```

For contrast, the earlier dark main/wide observation had approximately:

```
dark tile fraction = 414 / 432 ~= 0.958
entropy ~= 2.33
edgeDensity ~= 0.0499
=> information remains present; do not globally classify DARK_UNINFORMATIVE
```

This contrast is used only as a regression sanity check for the current research gate.

## Backside metadata versus local confirmation

Universal Intake passes source-bound hints for:

- NoiseProfile presence;
- BlackLevel presence;
- WhiteLevel presence.

These remain `SOURCE_METADATA_BOUND_HINT_ONLY`.

A DNG NoiseProfile being present is **not** local proof that a specific frontside tile is removable noise. Therefore v0.2 records:

- `local_noise_confirmation_available = false`
- `n2_local_support_bound = false`
- `can_support_correction_by_itself = false`
- `chroma_correction_supported = false`

A later version may bind local N2/backside measurements to the same observation and then construct a private chroma-only A/B/Delta candidate.

## Single-shot contract

Unchanged:

- one sealed source observation;
- no other physical lens;
- no temporal frame;
- no burst;
- no multi-observation fusion;
- one D.RAWnegative diagnostic target;
- no AI/ML/neural/learned model.

## Scientific writeback

Always false in v0.2:

- `candidate_applied = false`
- `creates_new_evidence = false`
- `scientific_writeback_allowed = false`
- `replacement_colour_estimated = false`
- `pixel_value_replacement_proposed = false`

DARK_UNINFORMATIVE therefore means exactly:

> D.RAW may observe instability, but it does not claim to know the hidden colour and does not reconstruct one.

