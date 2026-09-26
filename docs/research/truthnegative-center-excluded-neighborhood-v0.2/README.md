# TruthNegative Center-Excluded Neighborhood v0.2

Status: **EXECUTABLE N2 RESEARCH PREDICTOR — DIAGNOSTIC ONLY**

## Purpose

This module tests a stricter local-estimate contract for the D.RAW N2 path.
It does **not** replace the current v0.1 N2 route and is not connected to normal
PRO output.

The predictor is center-excluded by construction: its input contains neighbour
samples only. The observed center CFA value is not available to neighbour
selection, directional agreement, scale agreement, or the predicted local
signal. The center may only re-enter later in a separate residual test.

This prevents the exact noisy observation under test from deciding which
neighbours are accepted as its own predictor support.

## Support geometry

Compatible same-channel support may arrive as symmetric pairs in four source
lattice directions:

- horizontal;
- vertical;
- diagonal down;
- diagonal up.

Any positive source-pixel radius is representable. The first Android research
adapter should use admitted same-CFA-phase radii only; the module itself does
not infer CFA topology.

A symmetric pair is admitted only when:

- both samples have MEASURED or CALIBRATED_ESTIMATE authority;
- variance is known and positive;
- channel identity is compatible;
- no censor boundary is crossed;
- a known object identity does not cross objects;
- the two opposite observations agree within the fixed research bound.

At least two agreeing directions are required for one scale. Larger scales are
accepted only when they agree statistically with the already-admitted finer
support. A conflicting direction or coarse scale is rejected rather than
pulling the estimate across a likely edge/texture transition.

## Why this remains diagnostic

Center exclusion is not proof that camera noise is spatially independent.
DNG NoiseProfile models the white shot/read component and does not by itself
admit fixed-pattern, row/column, PRNU/DSNU or other correlated noise as
independent samples. D.RAW therefore keeps predictor support and noise-authority
as separate claims.

This module creates no evidence and allows no scientific writeback.

## Permanent invariants

- sealed Direct CFA unchanged;
- Scientific Master unchanged;
- TruthNegative unchanged;
- no appearance candidate is applied by this module;
- `createsNewEvidence=false`;
- `scientificWritebackAllowed=false`;
- existing reconstruction-support closure remains authoritative for protected
  full-colour output pixels.

## Promotion sequence

1. host-test center exclusion and directional/multiscale fail-closed behavior;
2. add an audit-only adapter over real Stage-2 CFA without changing v0.1;
3. compare v0.1 versus v0.2 predictor residuals in quiet/texture/highlight
   regions;
4. measure spatial residual correlation and CFA-phase behavior;
5. only after real-device evidence consider feeding v0.2 into a temporary
   appearance-only candidate;
6. reconstruction-support closure and exact Scientific-Master baseline gate
   remain mandatory.

## External knowledge anchors

- Batson & Royer, *Noise2Self: Blind Denoising by Self-Supervision* (2019):
  center/blind-spot evaluation is useful only under explicit statistical
  assumptions about noise independence.
- Adobe DNG 1.7.1.0 `NoiseProfile`: shot/read noise model for linear RAW; the
  specification explicitly states the white, spatially-independent assumption
  and excludes fixed-pattern/spatial effects from that model.
- EMVA 1288 Release 4.0: spatial non-uniformity is separately characterized
  from temporal noise, including row/column/pixel and DSNU/PRNU behavior.

These are constraints and comparison points, not authority upgrades for D.RAW.
