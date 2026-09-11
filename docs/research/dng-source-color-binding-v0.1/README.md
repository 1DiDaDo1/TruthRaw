# DNG Source Color Binding v0.1

Status: RESEARCH CANDIDATE — CI proof pending

Base: Scientific Preview Source Binding v0.1 @ `badf78ac578495425cf8ece0eef686944b3468b2`.

## Purpose

Produce the first real `SOURCE_METADATA_BOUND` camera-to-XYZ(D50) color binding from the exact sealed DNG source bytes, so a selected native DNG can eventually enter the already-validated Main-House reconstructed-color preview without the old identity/sentinel shortcut.

This module is deliberately narrower than a normal RAW converter. A file is admitted only when the DNG color model falls inside a subset whose semantics are reproduced explicitly and tested.

## v0.1 admitted profile

- classic TIFF/DNG, little- or big-endian;
- DNG major version 1, up to 1.7;
- embedded primary profile tags in IFD0;
- exactly 3 color channels represented by 3x3 `ColorMatrix1` and `ForwardMatrix1`;
- single-illuminant profile only;
- standard (non-unknown, non-custom) `CalibrationIlluminant1`;
- exactly one of `AsShotNeutral` or `AsShotWhiteXY`;
- `AnalogBalance` optional, identity when absent;
- `CameraCalibration1` optional, but if present both CameraCalibrationSignature and ProfileCalibrationSignature must be non-empty and identical;
- scene-referred ColorimetricReference only (default/0).

## Fail-closed exclusions

v0.1 rejects rather than approximates:

- BigTIFF;
- ColorMatrix2 / ForwardMatrix2 / CameraCalibration2 / CalibrationIlluminant2;
- DNG 1.6 triple-illuminant tags (`CalibrationIlluminant3`, `CameraCalibration3`, `ColorMatrix3`, `ForwardMatrix3`, `ReductionMatrix3`);
- alternate/extra camera profiles or `AsShotProfileName` profile selection;
- custom/unknown CalibrationIlluminant1 requiring IlluminantData;
- output-referred ColorimetricReference;
- missing ForwardMatrix1 fallback paths;
- unprovable CameraCalibration signature compatibility;
- singular/non-finite matrices;
- malformed/zero-denominator rationals;
- duplicate relevant color tags;
- source mutation after the original SHA-256 seal.

These exclusions are compatibility gaps, not scientific downgrades. Future v0.2+ work can add each class with its own tests.

## DNG SDK semantics reproduced

The implementation follows the Adobe DNG SDK model for the admitted subset:

1. Normalize `ColorMatrix1` as the SDK profile setter does: scale only when `max(ColorMatrix1 * PCS_D50)` lies outside 0.99..1.01, then round to four decimal places.
2. Round `ForwardMatrix1` to four decimal places, require its equal-camera-value mapping to be within the SDK 0.01 XYZ-D50 tolerance, then normalize it so `[1,1,1]` maps exactly to PCS D50.
3. Form the effective XYZ-to-individual-camera transform:
   `AnalogBalance * CameraCalibration1 * ColorMatrix1`.
4. If `AsShotNeutral` is present, solve white xy from
   `inverse(XYZtoCamera) * AsShotNeutral` (single-profile `NeutralToXY` case).
   If `AsShotWhiteXY` is present, use that chromaticity directly.
5. Compute and normalize/pin the camera white exactly in the same role as `dng_color_spec::SetWhiteXY`.
6. Compute:
   `individualToReference = inverse(AnalogBalance * CameraCalibration1)`
   `refCameraWhite = individualToReference * cameraWhite`
   `CameraToPCS = normalizedForwardMatrix * inverse(diag(refCameraWhite)) * individualToReference`.
7. PCS is XYZ chromatically adapted to D50, which is the matrix form expected by TruthRaw `cameraToXyzD50`.

The produced binding is explicitly `SourceMetadataBound`, never `IndependentCalibration` and never `FULL_PHYSICAL`.

## Source identity and TOCTOU boundary

The producer requires the pre-existing `SourceSeal` from Scientific Preview Source Binding v0.1. After parsing/deriving the matrix it re-hashes the source through the bounded SHA-256 path before returning. If any source byte changed after the seal, the producer fails closed.

Binding ID:

`dng-source-color-v0.1:single-forward@sha256:<source-digest>`

Thus the binding identifies both algorithm policy and exact source lineage.

## Memory model

Only the root IFD directory entries and individual required tag payloads are read. Required tag payloads are capped at 512 bytes and root IFD count at 512 entries. The final source re-verification uses the already-governed <=64 KiB SHA-256 chunk path. No RAW pixel payload or full image is materialized by this module.

## Tests

Synthetic valid DNG fixtures cover:

- little-endian single-profile path;
- big-endian single-profile path;
- `AsShotNeutral` route;
- `AsShotWhiteXY` route;
- hand-computable D50 fixture yielding identity CameraToXYZ(D50) within tolerance;
- output binding admitted by Scientific Preview Source Binding + Backplane source hash;
- dual profile rejection;
- triple profile rejection;
- missing ForwardMatrix rejection;
- output-referred ColorimetricReference rejection;
- zero SRATIONAL denominator rejection;
- CameraCalibration1 without compatible signatures rejection;
- CameraCalibration1 with matching signatures acceptance;
- source mutation after seal rejection;
- physical/evidence counts remain 1/1.

## External references reviewed 2026-09-11

- Adobe DNG 1.7.1.0 / DNG SDK landing page (SDK build 2724 dated 2026-09-08):
  https://helpx.adobe.com/camera-raw/desktop/dng-and-file-formats/digital-negative.html
- Adobe DNG SDK `dng_color_spec.cpp` reference implementation:
  https://github.com/aizvorski/dng_sdk/blob/master/source/dng_color_spec.cpp
- Android mirror of DNG SDK `dng_camera_profile.cpp` for `NormalizeColorMatrix`, `NormalizeForwardMatrix`, and forward-matrix validity:
  https://android.googlesource.com/platform/external/dng_sdk/+/refs/heads/android14-prebuilt-test/source/dng_camera_profile.cpp
- Android DNG tag codes / DNG 1.6 third-illuminant tag family.

## Non-claims / next step

This does not yet prove that the user's actual Honor/MotionCam DNG falls inside this single-profile subset. It does not provide independent lens/spectral/target calibration. It does not yet connect Android's selected `ParcelFileDescriptor` all the way through this producer and the Main-House streaming preview in one APK call.

After CI passes, the next proof is an Android-native composition:

```text
selected URI / borrowed fd
 -> PosixFdByteSource
 -> SourceSeal SHA-256
 -> DNG Source Color Binding v0.1
 -> Scientific Preview admission + Backplane source match
 -> TileNativeDngSource
 -> StreamingTruthRawProcessor v4.7i
 -> BoundedSrgbPreviewSink
 -> ARGB_8888 / JPEG sRGB
```
