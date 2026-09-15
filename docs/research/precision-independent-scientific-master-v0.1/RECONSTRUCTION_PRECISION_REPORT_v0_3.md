# TruthRaw Reconstruction Precision Report v0.3

Date: 2026-09-15
Status: RESEARCH / HOST-CI VALIDATED / NOT CANONICAL PROMOTION
Authority: numerical-reference only; no photographic-evidence upgrade

## Question

Can the frozen v4.7i edge-aware measured-preserving reconstruction remain a Float32 scientific path merely because the upstream black/white normalization and DNG GainMap stages showed errors far below the physical noise budget?

Answer: **not without an additional branch-stability argument.**

The linear/mostly-linear upstream stages can have tiny Float32 errors while a later hard reconstruction threshold converts a tiny input perturbation into a different algorithmic branch and a materially larger reconstructed-channel difference.

## Frozen algorithm under test

The precision reference reproduces the existing v4.7i `ResearchEdgeAwareMeasuredPreservingReconstruction` topology without modifying canonical v4.7i:

- measured CFA component is reinjected exactly;
- green at red/blue sites uses horizontal/vertical directional estimates;
- direction selection uses the frozen `0.72` ratio threshold;
- otherwise a weighted blend is used;
- reconstructed values remain support-limited;
- red/blue are reconstructed as local colour differences around the edge-aware green plane.

The Float32 reference is CI-checked against frozen canonical v4.7i before Float64 comparisons are admitted.

## Real HONOR fixture

Source file:

`IMG_BNC_TRUTHRAW20260906_151129_256.dng`

Source SHA-256:

`f900c9d8911072530d43c532a02328c62ed1e380ab6659c36bc2ed0f2dc9b229`

A 7x7 real Stage-2 neighbourhood around global CFA coordinate `(x=1408, y=884)` is retained as a deterministic host-CI fixture.

The fixture stores two executions from the **same photographic source evidence**:

- Float32 Stage-2 execution;
- Float64 Stage-2 execution.

They are numerical representations, not additional photographic observations.

## Result

At the centre of this real neighbourhood:

- Float32 green-direction decision: `Vertical`;
- Float64 green-direction decision: `WeightedBlend`;
- the Stage-2 perturbation that triggers the change is below `2e-7` in normalized signal;
- measured CFA-channel violations: `0` in both routes;
- the reconstructed red-channel end-to-end difference is greater than `1.9e-2` and less than `2.0e-2` normalized;
- when the **same Float32 Stage-2 samples** are merely promoted to double before reconstruction, the red-channel difference remains below `1e-5`.

Therefore the large local discrepancy is not explained by ordinary Float32 arithmetic inside reconstruction alone. The dominant mechanism in this fixture is:

`tiny upstream F32/F64 Stage-2 difference -> hard 0.72 direction threshold crossing -> different reconstruction branch -> amplified reconstructed-channel difference`

## Scientific interpretation

This does **not** mean Float64 discovers new scene evidence. It means Float32 can change which deterministic reconstruction hypothesis is selected from the same evidence.

That distinction is central to TruthRaw:

- evidence authority is unchanged;
- measured CFA samples remain measured;
- reconstructed channels remain reconstructed;
- higher precision only reduces numerical ambiguity in the reconstruction computation.

## Precision policy after v0.3

### Exact source evidence

Preserve exact integer/packed RAW bytes. No float format replaces source evidence.

### Black/white normalization

For the tested HONOR sources, Float32 remains a valid hot-path candidate with a Float64 reference path because measured numerical error remains far below the modeled physical uncertainty.

### DNG GainMap

For the tested HONOR vendor GainMap family, Float32 remains a hot-path candidate with Float64 reference because the six-file / 75,202,560-sample audit remains far below the modeled physical uncertainty, including highlight stress cases.

### Branch-sensitive reconstruction

**Float64 becomes the scientific reference precision for branch-sensitive reconstruction.**

Float32 reconstruction is not rejected as a runtime optimization, but it may not be assumed equivalent merely from the small upstream error budget. It must pass branch-stability and output-error gates against the Float64 reference.

### Calibration / optimization / covariance

Float64 remains the default scientific reference.

### Float128 / arbitrary precision

Offline validator/oracle only unless a later experiment proves a specific need.

## Important mixed-precision consequence

Promoting an already-rounded Float32 Stage-2 tile to Float64 is insufficient to recover the Float64 branch decision in the real fixture. A branch-stable mixed-precision design must preserve enough precision **before** the branch-sensitive reconstruction decision.

Candidate future designs include:

1. full Float64 Stage-2 + Float64 reconstruction per tile;
2. a dual-lane tile where Float32 remains the compact working signal but a Float64 decision/reference lane is preserved for branch-sensitive operations;
3. an adaptive scheme that escalates numerically ambiguous pixels/neighbourhoods to Float64 while retaining a validated Float32 fast path elsewhere.

None of these candidates is promoted yet.

## 200MP consequence

This finding does not require a full-frame 200MP Float64 RGB allocation. TruthRaw is already tile-oriented.

For a 512x512 tile:

- one Float64 scalar plane is 2 MiB;
- one Float64 RGB plane is 6 MiB.

This makes Float64 scientific reconstruction feasible as a tiled reference path even when a full-frame Float64 representation would be wasteful on Android.

Resource adaptation may change tile size, concurrency, cache and backend. It may not change scientific authority silently.

## CI evidence

Workflow `precision-independent-scientific-master-v0.1`, run `34984159027`, passed all five C++ tests plus the arbitrary-precision Python regressions and oracle report.

The five C++ tests include:

1. precision policy;
2. GainMap precision;
3. v4.7i Stage-2 adapter;
4. F32/F64 reconstruction precision reference with frozen-v4.7i F32 parity;
5. the real HONOR reconstruction precision fixture described above.

## Next gate

Build and test a mixed-precision reconstruction policy against the Float64 scientific reference on multiple real HONOR files. Required measurements:

- direction-branch divergence count;
- support-clamp divergence count;
- reconstructed RGB max/RMS error;
- measured-channel exactness;
- error relative to local modeled uncertainty;
- memory and runtime cost per tile;
- behaviour in dark, textured, edge, saturated and GainMap-amplified regions.

Until that gate closes, the scientific reference for branch-sensitive reconstruction is Float64.
