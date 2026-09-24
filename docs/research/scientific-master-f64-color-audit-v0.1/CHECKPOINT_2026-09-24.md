# TruthRaw Scientific Master F64 + Formal Color Audit checkpoint

Date: 2026-09-24

Research branch:

`research/scientific-master-f64-color-audit-v01-2026-09-24`

Frozen production code base:

`172a100786eb18d4b08564bbfb025a44f42cfa1e`

Frozen production branch remains untouched.

## 1. Mixed-precision reconstruction result

A parallel research backend now exists under:

`docs/research/scientific-master-f64-reconstruction-v0.1/native/`

The frozen canonical v4.7i `core.h` and `core.cpp` remain byte-identical to the frozen production versions.

Policy:

- Stage-2 input/storage: Float32;
- directional gradients and curvature: Float64;
- 0.72 directional decision: Float64;
- inverse-gradient weights: Float64;
- support limits: Float64;
- red/blue colour-difference interpolation: Float64;
- measured CFA component: direct original Float32 reinjection;
- reconstructed channels: one explicit Float64 -> Float32 storage boundary.

Measured host validation:

- GCC: PASS;
- Clang: PASS;
- Clang ASan/UBSan: PASS;
- deterministic threshold case changes branch:
  - F32 choice = 2;
  - F64 choice = 1;
- reconstructed components tested = 35,624;
- stored reconstructed components differing between F32/F64 = 11,040;
- maximum absolute stored delta = 3.0398368835449219e-06;
- measured CFA component bit-exact = PASS.

This proves that the precision change can alter reconstruction decisions without altering the measured source samples.

## 2. Android research integration

The following Android/native consumers are routed to the isolated F64 reconstruction backend on this branch:

- advanced preview;
- source-bound color preview;
- photo export;
- full-resolution restoration export;
- restoration projection;
- linear DNG export;
- Pure Float32 DNG export;
- TruthNegative export.

No production branch was changed.

Dedicated integration workflow:

`Scientific Master F64 + Color Audit v0.1`

Successful run:

`35937940660`

Result:

- F64 host precision: PASS;
- DNG color v0.2 GCC: PASS;
- DNG color v0.2 Clang: PASS;
- formal color authority/precision contract: PASS;
- Android arm64 assembleDebug: PASS;
- APK verification: PASS;
- artifact upload: PASS.

APK:

- bytes: 6,102,885;
- SHA-256: `30e9bee38bdfd8091a7cc4750b9d9e2c367224c28fc5b252aafcf1186e20b23a`;
- artifact: `truthraw-f64-scientific-master-color-audit-v01-debug-arm64`;
- artifact digest: `sha256:9fde7d2424b774ac1d8058dc5a38709697605090256bf2cc1467270ab1be8bb1`.

## 3. Formal color/calibration audit result

Machine-readable state:

`docs/research/formal-color-calibration-audit-v0.1/STATE_v0_1.json`

Detailed audit:

`docs/research/formal-color-calibration-audit-v0.1/README.md`

Current color authority remains:

`SOURCE_METADATA_BOUND`

Confirmed strengths for the supported DNG 1/2-calibration domain:

- ColorMatrix direction consistent with DNG processing model;
- CameraCalibration direction consistent;
- exact calibration-signature gate;
- inverse-CCT dual-illuminant interpolation;
- iterative CameraNeutral -> xy solve;
- fail closed on non-convergence;
- Bradford D50 fallback;
- dual ForwardMatrix path when both endpoints exist;
- internal matrix solve uses Float64.

Explicit remaining boundaries:

- one ForwardMatrix reused across a dual-calibration range is a project policy and remains REVIEW_REQUIRED for strict scientific use;
- third DNG calibration set is fail-closed unsupported;
- CalibrationIlluminant=255 / IlluminantData is fail-closed unsupported;
- the derived cameraToXyzD50 matrix is stored as Float32 after an internal Float64 solve;
- frozen runtime camera-RGB -> XYZ multiplication is Float32;
- Camera-5 field metadata consistency remains unresolved because the real blue-cast diagnostic showed that metadata-conformant processing is not the same as physical correctness;
- independent target/spectral/lens/unit calibration is NOT established;
- FULL_PHYSICAL color authority remains blocked.

## 4. Authority invariant

The F64 work changes numerical reconstruction quality only.

It does not:

- create source evidence;
- turn reconstructed samples into measured samples;
- alter Direct-CFA evidence;
- grant independent calibration authority;
- make a DNG matrix physically true merely because its arithmetic is higher precision.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
