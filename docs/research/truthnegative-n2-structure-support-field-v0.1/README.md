# TruthNegative N2 Structure Support Field v0.1

Status: research-only, audit-only.

This module reuses the existing TruthNegative N2 CFA candidate/preservation
pipeline without changing its thresholds or preserve decisions. It only changes
the **reporting tile size** from the historical 64×64 N2 spatial field to a
32×32 source-domain field so structure-protection density can be inspected at
finer spatial resolution.

## Fixed runtime options

- tile edge: 32 source pixels
- sampling period: 8 source pixels
- same N2 structure-preservation gate
- same source-local DNG scientific route

The period=8 audit samples the same deterministic CFA grid as the existing N2
spatial audit. The smaller reporting tile does not create new sensor samples.

## Authority

The field reports only measured preserve outcomes on that deterministic sample
grid.

Hard invariants:

- `sample_grid_evidence_only=true`
- `unsampled_pixels_inferred=false`
- `can_reduce_protection=false`
- `can_enable_correction=false`
- `promotion_eligible=false`
- `candidate_applied=false`
- `creates_new_evidence=false`
- `scientific_writeback_allowed=false`

It therefore cannot say that an unsampled pixel is structure-free.

## Reported per-tile values

Each 32×32 tile reports:

- sampled N2 points
- structure-protected sampled points
- structure-protection fraction
- censored-protected sampled points
- censor-protection fraction
- censor-boundary-protected sampled points
- censor-boundary-protection fraction
- whether at least one sampled point was structure-protected

These quantities are diagnostics. They are not probabilities and they do not
replace the existing N2 Factored Confidence axes.

## Why this successor exists

Real-device Dark Chroma v0.4 testing on a selective main/wide observation found
that only about 16.56% of all sampled points were structure-protected while
`structure_protection_present=true` occurred in all 3072 historical 64×64 N2
tiles. The old any-presence boolean was therefore safe but too coarse for a
candidate-level veto.

This field exists to measure the spatial density of that already-existing
protection before any change to promotion logic is considered.
