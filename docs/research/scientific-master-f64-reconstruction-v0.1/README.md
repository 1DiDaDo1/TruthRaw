# Scientific Master F64 Reconstruction Research v0.1

## Purpose

This research line closes a precision mismatch in the frozen v4.7i Scientific-Master reconstruction path without modifying the frozen production branch.

The current v4.7i research backend stores Stage-2 as Float32 and also performs its branch-sensitive reconstruction arithmetic in Float32. The sensitive points include:

- Hamilton-Adams directional curvature and gradient estimates;
- the 0.72 directional branch threshold;
- inverse-gradient directional blending;
- red/blue colour-difference weighting;
- local support-bound derivation.

A Float32 rounding change at those points can change a branch, not merely the last decimal place.

## v0.1 mixed-precision policy

v0.1 adds a parallel research backend:

`ResearchEdgeAwareMeasuredPreservingReconstructionF64`

The policy is deliberately narrow:

1. admitted Stage-2 remains Float32 storage;
2. branch-sensitive directional calculations are promoted to Float64;
3. reconstructed green and red/blue colour-difference interpolation are Float64;
4. support-limit calculations are Float64;
5. the physically measured CFA component is re-injected directly from the original Float32 input, bit-exact;
6. only reconstructed channels cross the explicit Float64 -> Float32 storage boundary;
7. no new evidence or calibration authority is created by greater arithmetic precision.

This is not a claim that Float64 reconstructs information that the sensor did not measure.

## Promotion gates

The host test requires:

- a deterministic neighbourhood where the legacy Float32 and new Float64 arithmetic select different sides of the existing 0.72 directional decision;
- measured CFA values bit-exact for all four Bayer layouts;
- finite reconstruction for both backends;
- at least one stored reconstructed component differs on a deterministic stress tile, proving the comparison is active rather than degenerate;
- strict floating-point compiler semantics: no fast-math and FP contraction disabled;
- GCC and Clang passes;
- ASan/UBSan pass.

The old F32 backend remains available for A/B comparison.

## Storage and memory

The full Scientific Master is not converted to Float64. Float64 is local reconstruction working precision only. The public backend ABI remains Float32 input and Float32 camera-RGB output, so the large image buffers and downstream Float32 Scientific-Master representation do not double in size.

## Authority boundary

Precision is representation quality, not evidence authority.

- measured remains measured;
- reconstructed remains reconstructed;
- appearance remains appearance;
- no Direct-CFA evidence is modified;
- no source sample is promoted to a stronger knowledge class;
- no physical colour/calibration authority is granted.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
