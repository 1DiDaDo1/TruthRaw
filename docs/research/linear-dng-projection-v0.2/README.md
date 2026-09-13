# TruthRaw Linear DNG Projection v0.2 — RGB output restoration

**Status:** `DEVELOPMENT_CANDIDATE__HOST_TEST_AND_ANDROID_WIRING_PENDING`

**Branch:** `research/restore-rgb-linearraw-output-v0.2-workbase-2026-09-13`

**Base:** `7c040218f6c5d43c6faeff6ab95d9fec763a1046` (`research/linear-dng-export-v0.1-2026-09-13`)

This module restores important behavior from the earlier validated TruthRaw RGB / LinearRaw DNG line while preserving the newer finalized-release, source-binding and bounded-streaming architecture.

It exists because the first Android Linear DNG export implementation (`linear-dng-projection-v0.1`) reproduced the correct high-level Scientific-Master-to-LinearRaw route, but lost parts of the older compatibility encoding and camera identity contract. The result could be structurally DNG-like while still behaving poorly in downstream RAW applications.

## 1. Non-negotiable scientific boundary

TruthRaw keeps the following order and authority separation:

`sealed Direct-CFA evidence -> Measurement -> Reconstruction -> Scientific Master -> downstream projection`

The Linear DNG is a `COMPATIBILITY_PROJECTION`.

It is not:
- the original measured CFA;
- a new physical exposure;
- a new source of photons;
- a promotion of reconstructed RGB to measured sensor truth;
- a color-calibration authority upgrade;
- the Scientific Master itself.

The Scientific Master remains camera-native reconstructed RGB before `camera_to_xyz()` and before appearance/tone/display processing.

`physicalFrameCount=1` and `independentEvidenceCount=1` remain unchanged.

No export is allowed to change the Scientific Master hash, zero-line, scene binding, Backplane identity, source seal or color-claim scope.

Canonical law:

> **Measured where measured. Reconstructed where necessary. Never invented.**

Secondary law:

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## 2. Historical RGB / LinearRaw behavior being restored

Earlier real TruthRaw RGB LinearRaw candidates demonstrated that the compatibility container must not simply clamp reconstructed scene-linear values at 1.0.

Recorded real-scene examples from the prior v0.6 validation:

- scene maximum `1.013005137` -> finite compatibility window `1.25x` -> `BaselineExposure = +0.321928 EV`;
- scene maximum `1.730840802` -> finite compatibility window `2.0x` -> `BaselineExposure = +1.000000 EV`.

The older release-candidate line also retained the physical source camera identity. For the Honor tele examples the validated DNGs reported:

`UniqueCameraModel = BKQ-N49-HONOR-HONOR`

Those exact v0.4 candidates passed a source-built DNG SDK 1.7.1/2611 `dng_validate` run with `Validation complete`. This is interoperability evidence for those exact historical files, not Adobe certification and not a physical-truth promotion.

## 3. Regression identified in v0.1 / Work-built Android Linear DNG

The v0.1 writer on the 2026-09-13 Linear-DNG branch:

1. quantizes camera-native reconstructed RGB directly into unsigned 16-bit `[0,1]`;
2. clips values `>1.0` to `65535`;
3. writes `BaselineExposure = 0 EV`;
4. writes `UniqueCameraModel = TruthRaw LinearRaw Projection v0.1`;
5. copies source-bound Honor color metadata into that new synthetic camera identity;
6. writes a single primary LinearRaw IFD and does not yet restore the older embedded-preview/container behavior.

This is a compatibility/export regression, not a reason to change the scientific reconstruction or the finalized preview authority model.

The Work-produced APK containing this line is recorded separately under `evidence/WORK_APK_FORENSIC_2026-09-13.md`.

## 4. v0.2 restoration design

v0.2 deliberately leaves the v0.1 writer intact and wraps it at the downstream representation boundary.

### 4.1 Fixed historical 2x compatibility window for the first restoration candidate

For this first Android restoration candidate, reconstructed camera-native RGB is multiplied by `0.5` only while being passed to the DNG representation writer.

The DNG then carries:

`BaselineExposure = +1 EV`

Therefore scene-linear values up to `2.0` remain representable inside the normal finite DNG range.

Example:

`scene 1.5 -> encoded 0.75 -> DNG BaselineExposure +1 EV -> downstream scene interpretation 1.5`

This representation transform does not change the Scientific Master.

### 4.2 Fail closed above the validated finite window

The underlying v0.1 quantizer still reports high clipping. Because the wrapper presents it with `scene / 2`, any high clipping now means the original scene value exceeded `2.0`.

v0.2 rejects that export and truncates the destination to zero bytes rather than silently claiming that clipped output is a faithful finite-window projection.

A future adaptive window may be considered only after it has an explicit validated contract. v0.2 does not invent arbitrary new scaling from the observed result.

### 4.3 Restore source camera identity

v0.2 reads the source TIFF/DNG identity and restores source-bound:

- `Make`;
- `Model`;
- `UniqueCameraModel`.

`UniqueCameraModel` is required for v0.2 admission because the copied DNG color calibration should remain bound to the actual source-camera identity rather than to a synthetic TruthRaw camera model.

The source color metadata does not acquire stronger authority. It remains source-bound metadata.

## 5. Files added in this candidate

- `native/linear_dng_projection_v0_2.h`
- `native/linear_dng_projection_v0_2.cpp`
- `tests/test_linear_dng_projection_v0_2.cpp`

The historical v0.1 module remains preserved unchanged.

## 6. Required tests

The v0.2 host regression test is designed to prove at minimum:

- a synthetic reconstructed RGB value `1.5` does not clip;
- its encoded 16-bit value decodes back to approximately `1.5` under the 2x window;
- `BaselineExposure` is exactly `+1/1 EV`;
- source `Make = HONOR` survives;
- source `Model = BKQ-N49` survives;
- source `UniqueCameraModel = BKQ-N49-HONOR-HONOR` survives;
- an original reconstructed value `>2.0` fails closed and leaves no completed DNG;
- scientific frame/evidence invariants remain inherited from the finalized release.

## 7. Work / Android relationship

The separate Work-built APK is useful as executable forensic evidence for the v0.1 Android path. It is not allowed to determine scientific evidence, calibration, color truth, noise authority or topology.

The good parts of that Android line to retain are:

- finalized Scientific Preview admission before export;
- exact source SHA re-verification;
- source/backplane identity checks;
- native bounded streaming;
- Android SAF destination handling;
- no full Scientific Master materialization merely for export.

The compatibility container behavior is what v0.2 is restoring.

## 8. Still open after the current code additions

The following are **not yet claimed complete** at this README revision:

1. host compile/test execution of v0.2;
2. Android CMake wiring from v0.1 to v0.2;
3. JNI status/result wiring for the v0.2 failure modes;
4. arm64 APK build of the v0.2 branch;
5. physical run on the Honor BKQ-N49;
6. Lightroom open/edit test on the resulting file;
7. independent parser / LibRaw / DNG SDK validation of the new exact v0.2 bytes;
8. embedded JPEG preview / multi-IFD restoration;
9. final decision whether a per-image `1x/1.25x/2x` selector can be obtained without an unacceptable extra reconstruction pass;
10. reconstructed CFA DNG and rawsensor outputs, which remain separate projection classes.

## 9. Preview restoration remains a separate container task

The finite-window and camera-identity repair does not by itself add an embedded JPEG preview.

The desired final compatibility container should be investigated as a multi-IFD DNG design in which:

- the primary image remains the RGB LinearRaw compatibility projection;
- an embedded preview is a display derivative only;
- the preview cannot feed back into science;
- the preview cannot alter source/master authority;
- orientation and source-bound color remain coherent;
- downstream readers such as Lightroom see an immediately usable RAW container.

Do not solve preview absence by replacing the RGB LinearRaw primary image with a rendered JPEG or by rebayering the Scientific Master.

## 10. Promotion rule

Do not call v0.2 validated, Lightroom-compatible, production-ready or physically proven until its exact built bytes pass the relevant host, Android, parser/LibRaw/DNG validation and physical-device gates.

Failures remain evidence and must not be hidden by retuning labels or thresholds.
