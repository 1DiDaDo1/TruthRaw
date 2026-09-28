# D.RAW Dark Chroma Stability v0.4 — same-observation local N2 spatial binding

Date: 2026-09-29

## Purpose

v0.4 is the next audit-only step above Dark Chroma v0.3.

v0.3 proved that a globally degenerate near-black frontside must remain
`DARK_UNINFORMATIVE` even when source-payload signal is measurable. v0.4 now
connects each deterministic frontside tile to the already existing local N2
Factored Confidence v0.3.1 field from **the exact same sealed DNG observation**.

This is a provenance/support binding step, not a correction step.

## Runtime path

```text
one sealed DNG observation
 -> frontside Dark Chroma v0.1/v0.2/v0.3
 -> existing N2 Factored Confidence v0.3.1 computed from the same source
 -> proportional frontside/source spatial binding
 -> Dark Chroma v0.4 local support state
 -> one D.RAWnegative diagnostic context

NO correction
NO Scientific-Master writeback
NO private chroma A/B/Delta
```

The N2 factored JSON is generated into an app-cache temporary file through the
existing native bridge and is deleted after parsing. The source remains
read-only.

## Exact source binding

v0.4 requires all of the following before a local binding is admitted:

- N2 factored export status success;
- `postWriteVerified=true`;
- N2 source SHA exactly equals the selected DNG source SHA;
- schema = `D.RAW/TruthNegative/N2FactoredConfidenceState/0.3.1`;
- `exact_confidence_field_binding_verified=true`;
- `promotion_eligible=false`;
- `candidate_applied=false`;
- `scientific_writeback_allowed=false`;
- `support_distance_admitted=false`.

Any mismatch is UNKNOWN/fail-closed.

## Spatial binding

Dark Chroma v0.1 uses deterministic 16×16 tiles on the frontside analysis
raster. N2 Factored Confidence uses 64×64 tiles in source coordinates.

For every frontside tile, v0.4 maps its rectangle proportionally into the source
raster and binds all overlapping N2 tiles.

The binding records separately:

- all-candidates-predictable;
- center-outlier-free;
- pair-rejection-free;
- scale-rejection-free;
- predictor-variance support;
- structure protection;
- censor/censor-boundary protection.

These remain separate axes. v0.4 does not collapse them into a probability.

## Strict local vector

For diagnostics only, v0.4 can report a `strict_local_support_vector` when all
overlapping N2 tiles satisfy every conservative factored axis and contain no
structure/censor protection.

This is **not** promotion.

Even if a strict vector is present:

```text
chroma_correction_supported = false
private_ab_delta_allowed = false
candidate_applied = false
```

The purpose is to learn where local factors agree before any chroma candidate
exists.

## DARK_UNINFORMATIVE remains stronger

A local N2 vector may never override a v0.3 `DARK_UNINFORMATIVE` state.

This is explicit in v0.4:

```text
dark_uninformative_can_be_overridden_by_local_n2 = false
```

Therefore the validated near-black tele regression remains blocked even if some
local N2 tiles look statistically well behaved.

## Current real-device regression target

The first required device validation is the same near-black tele scene that
produced:

- 432 / 432 v0.1 visible candidates;
- v0.3 = `DARK_UNINFORMATIVE_BY_DEGENERACY`;
- backside signal = `MEASURED_SIGNAL_PRESENT_OR_MIXED`;
- correction-supported = 0.

Expected v0.4 behavior:

- frontside tiles bind to N2 source tiles;
- the global v0.3 dark-uninformative block remains intact;
- correction-supported remains 0;
- private A/B/Delta remains disabled.

A second regression should use the earlier dark main/wide source with selective
frontside candidates so the local factor counts can be compared without global
degeneracy.

## Performance note

Universal Intake now performs the factored N2 audit automatically in its
background source-profiler thread for native DNG sources. This is deliberately
more expensive than the earlier frontside-only profile. It does not block the
UI thread.

## Single-shot law

Unchanged:

- source observation count = 1;
- no other physical lens;
- no temporal frames;
- no burst;
- no multi-observation fusion;
- no AI/ML/neural/generative/learned model.

## Next gate

After real-device validation of the local binding itself, the next research
question is whether a **private chroma-only A/B/Delta candidate gate** can be
defined without collapsing the separate N2 factors or violating
DARK_UNINFORMATIVE. That candidate does not exist in v0.4.
