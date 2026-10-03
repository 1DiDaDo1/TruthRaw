# D.RAW Exact Gauge Retained Artifact v0.3 — Source Inspection Checkpoint

Date: 2026-10-03

Status: **SOURCE GATE COMPLETE; SCALAR RESEARCH CANDIDATE IMPLEMENTED; NOT PROMOTED**

This checkpoint records the exact merged v0.2 behavior that the v0.3 retained-artifact candidate is allowed to accelerate. It does not redefine D.RAW, Truth Zero, Scientific Master authority, calibration, reconstruction, restoration, free precision, free resolution/raster, or RAW/JPG interpretation.

## Open-world rule

D.RAW seals source evidence and provenance, not the scientific world built above them.

A current RAW encoding, JPG encoding, Stage-2 representation, canonical tile grid, self-gauge, Float32 path, Float64 path, Truth Zero representation, or reconstruction model is never a universal boundary on future interpretation. Specialized implementations are replaceable computational routes behind general semantics.

The retained ledger in this experiment is therefore temporary computation, not evidence.

## Canonical source checkpoint

Scientific runtime checkpoint inspected:

- PR #127 runtime commit: `b82ada319ede5a87b0da0cdfe8732a19a3e74ff6`
- v0.2 binder: `docs/research/scientific-master-streaming-binding-v0.2/native/scientific_master_streaming_binding_v0_2.cpp`
- Stage-2 helper: `docs/research/full-frame-streaming-v0.1/native/full_frame_streaming_v0_1_internal.h`

The later integration head `c4a0f5883f6e9bafcceed2a9d60bb79a7829f12f` contains documentation/project-state synchronization; it does not supersede the PR #127 scientific runtime checkpoint.

## Exact Stage-2 meaning before the gauge

`fill_stage2` performs, per sample:

1. select CFA-phase black level;
2. add row/column residual black when present;
3. compute `(raw - black) / max(WhiteLevel - black, 1.0f)`;
4. apply the gain field exactly once when present;
5. store the result as Float32 Stage-2.

The self-gauge does not recalibrate these values. The retained artifact may preserve their exact Float32 bit patterns only after canonical eligibility has admitted them.

This is compatible with Truth Zero only because the artifact does not redefine the zero line, black level, white level, gain, calibration, or evidence state.

## Exact v0.2 eligibility predicate

For a canonical core sample `(x,y)`, v0.2 admits Stage-2 into the self-gauge only when all of the following hold:

- the sample lies inside the canonical 10% border exclusion;
- the tile-local RAW and Stage-2 indices are valid;
- `float(raw) < WhiteLevel` using the original RAW code value;
- `Stage2 > 0.0f`;
- `Stage2` is finite.

Consequences:

- `+0.0f` is excluded;
- `-0.0f` is excluded by the same strict-positive comparison;
- NaN is excluded;
- positive and negative infinity are excluded;
- source samples at or above `WhiteLevel` are censored before gauge admission.

In the complete binder, a non-finite Stage-2 value may be rejected even earlier when camera-native reconstruction causes the Scientific Master digest to encounter NaN/Inf. The v0.3 candidate must preserve that earlier failure behavior rather than broadening tolerance.

## Exact rank and median convention

For eligible count `N`:

- lower rank = `(N - 1) / 2`;
- upper rank = `N / 2`;
- ranks are zero-based.

v0.2 first selects the high 16 bits of each rank by histogram, then performs a second canonical Stage-2 traversal to resolve the low 16 bits for the selected prefix(es).

For odd `N`, lower and upper ranks are equal and the selected Float32 value is the median.

For even `N`, the two selected Float32 values are each converted to `double`, then combined as:

`L0 = lowerDouble + (upperDouble - lowerDouble) * 0.5`

The v0.3 candidate must reproduce this exact operation, not an algebraically rearranged Float32 average.

## Why unsigned Float32 bit radix order is valid here

The exact v0.2 eligibility contract leaves only strictly positive finite Float32 values. For that restricted domain, unsigned IEEE-754 binary32 bit-pattern order is monotonic with numerical order. The retained route must not reuse this ordering proof if eligibility is broadened in the future.

## Canonical order and determinism

Canonical cores are traversed deterministically in row-major tile order with a fixed 64x64 canonical core. Samples inside each core are visited row-major.

The median result depends on counts/ranks, not encounter order. Nevertheless, the first candidate retains canonical append order because changing the pass-artifact contract is unnecessary and would weaken auditability.

The established correctness path is single-thread deterministic. The first v0.3 candidate introduces no new concurrency or SIMD/NEON path.

## Pass-2 consumer audit

In the inspected v0.2 binder, pass 2:

- calls `fill_stage2` again for each canonical tile;
- applies the same self-gauge eligibility predicate;
- extracts exact Float32 high/low 16-bit words;
- increments the low-word histograms only for the already-selected high-word prefix(es);
- performs resident-budget accounting.

No second-pass reconstruction, Scientific Master digest addition, canonical observer callback, authority mutation, calibration change, source write, restoration, appearance operation, or scientific writeback was found.

Therefore a retained exact-bit ledger is sufficient for the currently proven low-16 consumer. If any future pass-2 consumer is added, this artifact contract is no longer automatically eligible.

## Cardinality and memory bound

For dimensions `W x H`, define the same v0.2 borders:

- `borderX = floor(0.10 * W)`, clamped by the canonical v0.2 dimension rule;
- `borderY = floor(0.10 * H)`, clamped likewise.

The geometric maximum retained count is:

`(W - 2*borderX) * (H - 2*borderY)`

Actual retained count is less than or equal to this bound because RAW clipping, non-positive Stage-2 and non-finite Stage-2 remain excluded.

For the established `4080 x 3072` geometry:

- `borderX = 408`;
- `borderY = 307`;
- geometric maximum = `3264 * 2458 = 8,022,912` samples;
- exact `uint32_t` ledger payload maximum = `32,091,648` bytes;
- this is approximately `30.60 MiB`.

This is a dimension-specific bound, not a universal D.RAW memory constant. Other sensors/resolutions derive their own bound.

The scalar candidate reserves against the geometric upper bound, accounts retained capacity in the logical resident budget, and falls back to the complete canonical v0.2 route if the bound cannot be represented, admitted, or allocated safely.

## Candidate route identity

Artifact contract:

- type: `EXACT_GAUGE_FLOAT32_BITS`;
- version: `0.3`;
- producer semantics: `SMSB_V0_2_STAGE2_SELF_GAUGE`;
- consumer semantics: `EXACT_GAUGE_LOW16_V0_3`;
- eligibility identity: `SMSB_V0_2_POSITIVE_FINITE_RAW_LT_WHITE_CENTER80`.

The optimized route truthfully reports one Stage-2 gauge scan. Complete fallback truthfully retains v0.2's two scans. Route, memory, read counts and timing are diagnostics only and are excluded from Scientific Master identity.

## Permanent safety state

`source_values_modified=false`

`creates_new_evidence=false`

`scientific_writeback_allowed=false`

`restoration_authority_created=false`

`appearance_authority_created=false`

`truth_zero_redefined=false`

`free_resolution_or_raster_restricted=false`

`float32_declared_universal=false`

`float64_declared_universal=false`

The artifact may remember a computation. It cannot become an observation or close D.RAW's open scientific world.
