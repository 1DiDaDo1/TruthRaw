# Android Finalized Dual-Illuminant Color v0.2

Status: **RESEARCH PASS — ANDROID ARM64 APK CI PASS**

Date: 2026-09-11

Validated Android integration SHA: `42b49ba16a6c5a0d2d6dbc407330acde3e161a46`

Validation workflow run: `34646078196`

Validated DNG color-science core SHA: `820b76bc289b70944cc1986c3908179dfc1f4263`

DNG color-science validation workflow run: `34645455055`

## Purpose

This candidate integrates DNG Color Binding Producer v0.2 into the Android finalized Scientific Preview route while preserving TruthRaw's evidence, Scientific Master, and color-authority boundaries.

Android route:

`ParcelFileDescriptor(fd)`
→ exact SHA-256 source seal
→ DNG Color Binding Producer v0.2
→ Scientific Preview Source Binding v0.2 phase-1 preparation
→ `TileNativeDngSource`
→ v4.7i streaming reconstruction / Scientific Master streaming binding
→ deterministic Scientific Master identity + TruthRange self-gauge
→ Technical Backplane phase 2
→ Finalized Scientific Preview release gate
→ bounded sRGB `Bitmap`
→ optional presentation export.

DNG Color Binding Producer v0.2 itself is source-metadata-bound. Its successful dual-illuminant output does not become independent physical camera/lens color calibration.

## Exact implementation chain

The Android integration was deliberately small and separated into auditable commits:

- `5c1b277f6c7ea9096e6ad8155972a5cf7e963142` — link both DNG Color Producer v0.1 and v0.2 into the Android CMake target. v0.1 remains required because v0.2 delegates the already-validated single-illuminant case exactly to v0.1.
- `9f9e61ae2613f8fb66747b42b312ed5c2cd6bfee` — switch both Android source-bound and finalized color producer calls from v0.1 to v0.2 without changing the downstream renderer/finalization route.
- `42b49ba16a6c5a0d2d6dbc407330acde3e161a46` — align Kotlin native-status descriptions with the v0.2 enum after an integration review found that the old v0.1 error-code text would misdescribe new v0.2 fail-closed states.

## Color model

For supported dual-illuminant DNG metadata, v0.2 uses the separately validated research implementation:

- standard calibration illuminants are mapped to bounded correlated color temperatures;
- calibration endpoints are sorted by temperature;
- `AsShotNeutral -> xy` is solved iteratively from D50;
- maximum 30 iterations;
- convergence threshold `abs(dx) + abs(dy) < 1e-7`;
- failure to converge is fail-closed rather than averaged into an accepted answer;
- matrix interpolation is linear in inverse correlated color temperature (`1/T`);
- ColorMatrix / CameraCalibration / ForwardMatrix state is resolved consistently at that white point;
- CameraCalibration is applied only when CameraCalibrationSignature and ProfileCalibrationSignature match;
- source bytes are SHA-256 rebound/reverified as required by the surrounding Android route.

Single-illuminant DNG input remains an exact v0.1 delegation and is tested for exact equality of matrix, binding ID, source evidence ID, and authority.

## Authority boundary

For the embedded/source-metadata color path:

- `ColorBindingAuthority = SOURCE_METADATA_BOUND`;
- finalized preview authority may be `FINALIZED_SOURCE_BOUND_SCIENTIFIC_PREVIEW`;
- `scientificClaimAllowed` remains false for source-metadata-only color;
- only a separately admitted independent camera/lens calibration may justify the stronger independently-calibrated preview authority / scientific color claim.

The route does not manufacture a `FULL_PHYSICAL` color claim from DNG metadata.

Evidence invariants remain:

- `physicalFrameCount = 1`;
- `independentEvidenceCount = 1`;
- no second capture is invented;
- appearance does not modify the Scientific Master;
- counterfactual observation is not evidence;
- sealed source bytes remain the evidence root.

## Preserved negative evidence

DNG Color Producer v0.2 first validation run `34645106756` remains **FAIL**. The extreme synthetic dual CameraCalibration pair (`1.10` / `0.90` first-channel endpoint scale) exposed non-convergence/two-point oscillation in the neutral-to-xy solve.

The repair did not loosen the convergence threshold. The extreme case remains a negative test and must return `NeutralSolveDidNotConverge`; the positive signature/application case uses a mild convergent calibration pair (`1.01` / `0.99`).

An intermediate Android integration run, `34645951293` at head `9567d77fca42d854d4fbfbcc71bfaafd48c1ba1e`, successfully compiled and assembled the v0.2 route. It is **superseded as final integration evidence**, because review during that run found that Kotlin still described producer status codes according to the old v0.1 enum. Native processing remained fail-closed, but UI error explanations for the new v0.2 statuses could be wrong. The mapping was corrected at `42b49ba16a6c5a0d2d6dbc407330acde3e161a46`, and the full workflow was rerun from that exact head.

## Exact-head validation

Workflow run `34646078196` on exact implementation SHA `42b49ba16a6c5a0d2d6dbc407330acde3e161a46` completed successfully.

Host-science/release evidence:

- DNG Color Binding Producer v0.2 revalidation: PASS;
- finalized Scientific Preview release-gate revalidation: PASS.

Android integration/build evidence:

- v0.2 JNI producer wiring present: PASS;
- v0.1 exact-single delegate still linked: PASS;
- Finalized Scientific Preview gate still present: PASS;
- pre/post source re-verification present: PASS;
- no Android camera permission / no capture authority: PASS;
- Java 17 / Android SDK 35 / Build Tools 35.0.0 / CMake 3.22.1 / NDK 27.2.12479018 route: PASS;
- Kotlin compilation: PASS, with only the already-known `setDecorFitsSystemWindows` Java deprecation warning;
- Android NDK/CMake arm64 compilation and JNI link: PASS;
- frozen canonical v4.7i retains the already-known one source-scoped non-fatal `-Wmisleading-indentation` warning; canonical bytes were not modified;
- APK assembly: PASS (`BUILD SUCCESSFUL in 2m 24s`);
- packaged native bridge: `lib/arm64-v8a/libtruthraw_ui_preview_bridge.so`;
- packaged bridge size: `1,716,584` bytes;
- no preview bridge for armeabi-v7a, x86, or x86_64: PASS;
- APK size: `4,142,077` bytes;
- APK SHA-256: `2c90487384e2243b030f9ed50bc55ab078590a93e7d8a2acf09758e1ed252989`;
- artifact name: `truthraw-android-finalized-dual-illuminant-color-v0.2-debug-arm64`;
- artifact ID: `10281704392`;
- uploaded artifact ZIP size: `1,308,651` bytes;
- uploaded artifact ZIP SHA-256: `3653df8a5ddac838602dcba2a8a6ac0116696e44eac1c07ab251b9c56ddd9bf3`.

The workflow explicitly reported:

- `dng_color_producer=V0_2_WITH_EXACT_V0_1_SINGLE_DELEGATION`;
- `dual_interpolation=INVERSE_CORRELATED_COLOR_TEMPERATURE`;
- `color_authority=SOURCE_METADATA_BOUND`;
- `preview_role=FINALIZED_SOURCE_BOUND_SCIENTIFIC_PREVIEW_or_stronger`;
- `source_reverification=PRE_AND_POST_RENDER`;
- `scientific_claim=INDEPENDENT_CALIBRATION_ONLY`;
- `physical_frame_count=1 independent_evidence_count=1`.

## Fail-closed scope

This Android candidate rejects rather than guesses for unsupported/malformed color metadata, including:

- incomplete dual calibration;
- triple-illuminant calibration;
- custom calibration illuminant `255` that requires IlluminantData support not yet implemented;
- unsupported calibration illuminants;
- malformed/singular/non-finite matrices;
- neutral-to-xy non-convergence;
- source seal mismatch or source mutation;
- invalid finalized preview authority/provenance state.

## Proof boundary

This proves software/build/package integration for the arm64 Android candidate and revalidates the bounded dual-illuminant color science in host CI. It does **not** yet prove:

- successful execution on the physical Honor Magic 8 Pro target;
- successful finalized preview of a real MotionCam/Honor DNG on-device;
- that a particular Honor/MotionCam DNG uses a supported dual-illuminant metadata form;
- physical-device RSS, thermal behavior, latency, frame time, or provider compatibility beyond the current seekable-fd contract;
- independent camera/lens spectral calibration;
- `FULL_PHYSICAL` color truth;
- that embedded vendor matrices are physically optimal;
- arbitrary DNG dual/triple/custom profile support;
- multi-capture fusion/HDR science.

## Next empirical boundary

The next empirical step is a physical Honor/MotionCam run using a real sealed DNG through this exact Android route, recording source SHA-256, admitted metadata form, preview authority, failure/success status, APK identity, RSS, latency/frame time, and thermal observations without changing scientific thresholds.

Independent per-lens camera color calibration remains a separate scientific program and is required before a `FULL_PHYSICAL` color claim can be considered.