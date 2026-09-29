# TruthNegative N2 Sample Support Distance v0.1

Status: research-only, audit-only.

## Purpose

Dark Chroma v0.5 proved that reducing the structure-support reporting tile from
64×64 to 32×32 did not separate selective main/wide chroma candidates from
N2 structure protection. The measured structure density around candidates was
essentially the same as the whole-observation density.

This module therefore stops asking only whether a tile contains any protected
sample and measures the geometry of the **exact deterministic N2 sampled
support points**.

## Source domain

The module reuses the same deterministic N2 CFA evaluation:

- sampling period = 8 source pixels;
- evaluation tile edge = 64 for streaming/workspace locality;
- same structure-preservation gate;
- same NoiseProfile/Stage-2 inputs;
- same PreserveReason values;
- same source-local DNG scientific route.

The resulting N2 candidate SHA and audit SHA use the predecessor v0.1 hash
domains so parity can be checked against existing N2 sidecars.

## Exact coordinate record

For PreserveReason values:

- Structure;
- Censored;
- CensorBoundary;

the exact sampled source-native x/y coordinates are recorded.

The sidecar stores them compactly as:

`BASE64_LE_U32_XY_PAIRS_SOURCE_NATIVE`

and also seals the ordered protected-point stream with
`support_point_stream_sha256`.

This is still sample-grid evidence only:

- unsampled pixels are never inferred;
- no interpolation creates "structure-free" pixels;
- coordinate density is not a probability.

## Candidate query geometry

The Android bridge maps each visible Dark-Chroma frontside tile to its exact
source rectangle and supplies those rectangles as audit queries.

For each query the audit records:

### Center radii

At radii 8, 16, 32 and 64 source pixels from the mapped candidate center:

- sampled N2 points;
- structure-protected points;
- structure fraction;
- censored points;
- censor-boundary points.

### Rectangle margins

At distance 0, 8, 16 and 32 source pixels from the mapped candidate rectangle:

- sampled N2 points;
- structure-protected points;
- structure fraction;
- censored points;
- censor-boundary points.

Margin 0 means sampled points inside the candidate rectangle.

### Exact nearest support

Separately for Structure, Censored and CensorBoundary:

- nearest sampled support point from candidate center;
- nearest sampled support point to candidate rectangle;
- exact nearest source x/y coordinate.

## No promotion semantics

Hard invariants:

- `exact_sample_coordinates_recorded=true`;
- `sample_grid_evidence_only=true`;
- `unsampled_pixels_inferred=false`;
- `scalar_probability_created=false`;
- `can_reduce_protection=false`;
- `can_enable_correction=false`;
- `promotion_eligible=false`;
- `candidate_applied=false`;
- `creates_new_evidence=false`;
- `scientific_writeback_allowed=false`.

No radius or distance threshold is admitted in v0.1.
