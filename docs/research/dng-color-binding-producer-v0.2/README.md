# DNG Color Binding Producer v0.2

Status: **RESEARCH PASS — SOURCE-METADATA-BOUND DUAL-ILLUMINANT COLOR**

Date: 2026-09-11

Validated implementation SHA: `820b76bc289b70944cc1986c3908179dfc1f4263`

Validation workflow run: `34645455055`

## Purpose

DNG Color Binding Producer v0.2 extends the existing fail-closed v0.1 producer with a bounded dual-illuminant path while preserving the TruthRaw authority boundary. It does not promote source DNG metadata to independently calibrated physical color truth.

The producer remains source-bound:

`sealed DNG bytes -> IFD0 color metadata -> bounded dual-illuminant solve -> ScientificColorBindingRecord(SOURCE_METADATA_BOUND)`

It does not create evidence, does not modify Direct-CFA evidence, does not modify the Scientific Master, and does not change `physicalFrameCount = 1` / `independentEvidenceCount = 1`.

## Single-illuminant compatibility

A DNG without a second calibration set is delegated to DNG Color Binding Producer v0.1. The v0.2 test requires exact equality for the resulting color matrix, binding ID, source evidence ID, and authority. v0.2 therefore does not silently perturb the already-validated single-illuminant path.

## Dual-illuminant model

The research implementation follows the DNG/Adobe reference model for the supported subset:

- `CalibrationIlluminant1` and `CalibrationIlluminant2` are mapped to reference correlated color temperatures using the bounded standard-light mapping implemented in v0.2.
- Endpoint calibration sets are sorted by temperature before interpolation.
- `AsShotNeutral -> xy` is solved iteratively, beginning from D50.
- the solve is limited to 30 iterations;
- convergence requires `abs(dx) + abs(dy) < 1e-7`;
- unlike Adobe's permissive final-pass averaging fallback, TruthRaw v0.2 fails closed when the solve does not converge;
- the interpolation factor is linear in inverse correlated color temperature (`1/T`), not linear temperature;
- ColorMatrix, CameraCalibration, and ForwardMatrix endpoint state is interpolated consistently for the resolved white point;
- if only one ForwardMatrix endpoint exists, use across the supported temperature range is explicitly audited rather than hidden.

## Calibration signatures

CameraCalibration matrices are applied only when CameraCalibrationSignature and ProfileCalibrationSignature match. A mismatch leaves the camera-calibration contribution at identity, preserving the existing DNG rule and v0.1 authority boundary.

## Explicit fail-closed limits

v0.2 rejects rather than guesses when:

- the second calibration set is incomplete;
- a third calibration is present;
- calibration illuminant `255` requires IlluminantData support not yet implemented;
- a supported calibration illuminant cannot be mapped to a bounded reference temperature;
- a matrix is malformed, singular, or produces non-finite state;
- the iterative neutral-to-xy solve fails to converge within 30 iterations;
- the exact sealed source changes before or during production.

Triple-illuminant support and custom IlluminantData are therefore not claimed by v0.2.

## Preserved negative evidence

The first v0.2 validation run, `34645106756`, remains a **FAIL**. Its GCC test exposed that an intentionally extreme synthetic dual CameraCalibration pair (`1.10` / `0.90` first-channel endpoint scale) can drive the neutral-to-xy fixed-point iteration into a non-convergent/two-point-oscillation regime.

The repair did not loosen the convergence threshold and did not introduce Adobe's averaging fallback. Instead:

- the positive signature/application test now uses a mild convergent `1.01` / `0.99` synthetic calibration pair;
- the original `1.10` / `0.90` case remains as an explicit negative test;
- that case must return `NeutralSolveDidNotConverge`.

This preserves the failure as falsification evidence rather than relabeling it as success.

## Validated test surface

Exact-head workflow run `34645455055` passed on implementation SHA `820b76bc289b70944cc1986c3908179dfc1f4263` with:

- GCC Release: PASS;
- Clang Release: PASS;
- Clang AddressSanitizer + UndefinedBehaviorSanitizer: PASS;
- exact v0.1 delegation for single-illuminant input: PASS;
- D50/D65 inverse-CCT interpolation: PASS;
- calibration-order swap invariance: PASS;
- CameraCalibration signature mismatch -> identity contribution: PASS;
- matching mild CameraCalibration -> applied: PASS;
- extreme CameraCalibration non-convergence -> fail closed: PASS;
- one-ForwardMatrix explicit audit: PASS;
- incomplete dual calibration -> fail closed: PASS;
- triple calibration -> fail closed: PASS;
- custom illuminant 255 -> fail closed until IlluminantData support: PASS;
- source tamper -> source-seal mismatch: PASS.

## Authority boundary

Successful output is still:

`ColorBindingAuthority::SourceMetadataBound`

It is not:

- independent camera/lens spectral calibration;
- `FULL_PHYSICAL` color truth;
- proof that the embedded/vendor color matrices are physically optimal;
- proof of a specific Honor/MotionCam DNG until such a file is exercised on the physical-device route.

The Scientific Master and immutable source evidence remain separate from this color-binding producer.

## Next integration boundary

The next step is to replace the Android finalized-preview branch's v0.1 producer call with v0.2 while preserving:

1. exact single-illuminant v0.1 behavior;
2. source-seal binding;
3. finalized Scientific Preview phase-2 gate;
4. `SOURCE_METADATA_BOUND` authority unless an independent camera/lens calibration binding exists;
5. pixel/authority equivalence for the already-supported single-illuminant fixtures;
6. fail-closed behavior for unsupported dual/triple/custom profiles.

Physical Honor/MotionCam execution, real-device RSS/thermal/latency, and independent per-lens color calibration remain separate empirical blockers.