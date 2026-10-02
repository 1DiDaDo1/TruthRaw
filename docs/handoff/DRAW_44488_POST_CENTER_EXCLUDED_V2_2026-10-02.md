# D.RAW 44488 continuation checkpoint — post Center-Excluded v2

Date: 2026-10-02

This file **supersedes the performance frontier described in `DRAW_NEXT_CHAT_HANDOFF_2026-10-02.md`** while preserving that older handoff as provenance.

## Continuation code

`44488`

A future chat receiving 44488 must restore the complete prior D.RAW/TruthRaw context and then apply this newer checkpoint before changing code.

## Latest promoted performance result

PR #125 — **Center-Excluded fixed-topology fast path v0.1** — was real-device validated and merged.

- source head: `2260f9be52666939243c769f91ce2fed90a5748b`
- merge commit: `92b56f44a04313901ad1e430901ac3d0e3d884c8`
- merged into: `fix/research-fresh-rerun-v01`
- Android validation build versionCode: `26100120`
- validation build versionName: `0.53-v0.84.2-authority-template-reuse-center-excluded-fixed-topology-v01`

The fixed-topology specialization applies only to the already-established Center-Excluded geometry `{radii 2,4,8} × {4 directions} × {2 symmetric sides}`. The generic canonical predictor remains the fail-closed fallback.

An exact IEEE-754 `Result` parity oracle is compiled and executed in CI. Thresholds, accumulation order, center exclusion and scientific semantics are unchanged.

## Real-device benchmark

Dataset: same two established RAW observations used throughout the current performance line.

Previous Center-Excluded profile baseline (PR #124 / observation export 24):

- aggregate profile elapsed: `31,963.150301 ms`
- RAW1 Center-Excluded: `4,088.13 ms`
- RAW1 predictor: `3,021.54 ms`
- RAW2 Center-Excluded: `5,675.69 ms`
- RAW2 predictor: `4,467.30 ms`

Promoted fixed-topology result (observation export 25):

- aggregate profile elapsed: `26,169.369782 ms`
- RAW1 Center-Excluded: `1,985.62 ms`
- RAW1 predictor: `1,020.15 ms`
- RAW2 Center-Excluded: `2,870.54 ms`
- RAW2 predictor: `1,628.82 ms`

Derived improvement:

- aggregate profile elapsed: about `-18.13%`
- combined Center-Excluded: `9,763.82 -> 4,856.16 ms`, about `-50.26%`
- combined predictor: `7,488.84 -> 2,648.97 ms`, about `-64.63%`

The complete Foundation JSON outside `performance_diagnostics_v0_1` is parsed-structure identical between the baseline and fixed-topology run.

## Safety / authority state

Still required and observed:

- `source_values_modified=false`
- `candidate_applied=false`
- `creates_new_evidence=false`
- `scientific_writeback_allowed=false`
- timing is diagnostic-only
- performance changes may not change measurement authority
- center sample remains excluded from its predictor
- no reconstruction/calibration/correction promotion
- no AI/ML/neural/generative runtime

## Authority line status

The authority optimization sequence through pixel-triplet encoding remains scientifically exact. Canonical template/suffix reuse was proven safe but did **not** show meaningful real-device authority speedup, so it was not promoted as a performance win. Do not resume authority micro-optimization blindly.

The current authority hot path remains around 10 seconds combined for the two established RAWs and is now again the largest single named block, but the project previously reached diminishing returns there.

## Next performance frontier

After Center-Excluded v2 promotion, do **not** jump directly to SIMD/NEON.

The next step is to re-profile the remaining shared scientific preparation / Scientific Master binding work and separate:

1. authority direct-record stream cost;
2. non-authority Scientific Master binding cost;
3. source sealing / pre-open reverify;
4. DNG adapter work;
5. any repeated per-pixel or per-tile work outside the already-optimized authority stream.

Only after the new profile identifies a dominant block should a new specialization be designed.

## Scientific frontier remains separate

The earlier Anchor-Constrained Local Reconstruction affine candidate remains **NOT PROMOTED** because it had worse average true-CFA holdout error despite better coverage. The scientific next direction remains deterministic local model selection using structure support, direction, CFA phase and uncertainty, with `no suitable model` as a valid outcome.

Free World work may continue in parallel through additional real observations and admitted relations. Existing pair geometry remains fail-closed when evidence is insufficient.

## Permanent laws

- `MEASURED != RECONSTRUCTED != APPEARANCE`
- Seal the evidence, not the thinking.
- Representation can exceed the source. Knowledge claims cannot exceed the evidence.
- Direct-CFA evidence remains immutable.
- Scientific Master remains separate from export/presentation.
- SOURCE/SENSOR, WORLD/SCENE and VIEW/OUTPUT spaces remain distinct.
- A faster path may reproduce an existing scientific result; it may not create a new implicit truth.
- Fast-path architecture remains: **general semantic route -> versioned specialization -> exact parity oracle -> fail-closed fallback**.
