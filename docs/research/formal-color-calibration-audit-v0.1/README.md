# TruthRaw Formal Color / Calibration Audit v0.1

Date: 2026-09-24

## Scope

This audit follows the Scientific-Master F32/F64 reconstruction review. It asks four separate questions and does not merge them:

1. Is the source-metadata color transform structurally consistent with the DNG color model?
2. Is the numerical precision boundary explicit and appropriate?
3. Does the project have independent physical camera/lens color calibration?
4. Does any observed device-specific inconsistency require the source metadata to be rejected or downgraded?

The audit does not modify Direct-CFA evidence and does not grant stronger color authority.

## Normative references checked

- Adobe Digital Negative (DNG) Specification 1.7.1.0, September 2023:
  https://helpx.adobe.com/content/dam/help/en/camera-raw/digital-negative/jcr_content/root/content/flex/items/position/position-par/download_section_733958301/download-1/DNG_Spec_1_7_1_0.pdf
- Android Camera / NDK metadata reference:
  https://developer.android.com/ndk/reference/group/camera

The repository implementation reviewed is:

- `docs/research/dng-color-binding-producer-v0.2/`
- `docs/research/scientific-preview-source-binding-v0.1/`
- `canonical/reconstruction/v4.7i/`
- `suite_android/app/src/main/java/com/truthraw/adaptiveui/CameraCalibrationTelemetry.kt`
- `docs/research/camera5-color-metadata-diagnostic-v0.1/`
- `docs/calibration/tele-full-physical-v0.1/`

## A. DNG model audit

### A1. ColorMatrix direction — PASS

DNG 1.7.1 defines ColorMatrix1/2 as transforms from CIE XYZ to reference camera native color under the corresponding calibration illuminant.

TruthRaw v0.2 treats ColorMatrix as the XYZ-to-reference-camera component and forms:

`XYZtoCamera = AB * CC * CM`

This matches the DNG Chapter 6 model for the three-channel path.

### A2. CameraCalibration direction and signature gate — PASS

DNG defines CameraCalibration as reference-camera-native to individual-camera-native and requires it to be used only when CameraCalibrationSignature exactly matches ProfileCalibrationSignature.

TruthRaw v0.2 applies CameraCalibration only after exact signature equality. Mismatch leaves the CameraCalibration contribution at identity. This is correct and fail-closed.

### A3. Dual-illuminant interpolation — PASS for supported 2-set domain

DNG 1.2+ requires inverse correlated-color-temperature interpolation between two calibration sets, with the closest set used outside the interval.

TruthRaw v0.2 sorts the two calibration sets by temperature and performs inverse-temperature interpolation. Its existing tests cover D50/D65 and calibration-order invariance.

### A4. CameraNeutral -> white xy iteration — PASS for supported convergent domain

DNG specifies an iterative solve from camera neutral coordinates to white-balance xy because the interpolated XYZ-to-camera matrix itself depends on the white point.

TruthRaw v0.2 performs this iterative solve and fails closed when it does not converge. The existing extreme synthetic CameraCalibration test intentionally exercises non-convergence.

### A5. No-ForwardMatrix path — PASS

DNG specifies inversion of XYZtoCamera followed by chromatic adaptation from the selected white to D50, with linear Bradford recommended.

TruthRaw v0.2 implements this structure and validates the derived matrix for finite values and singularity.

### A6. ForwardMatrix path — PASS when both dual endpoints exist

DNG defines:

`CameraToXYZ_D50 = FM * D * Inverse(AB * CC)`

where `D` is derived from the reference-camera neutral.

TruthRaw v0.2 follows this structure when both dual ForwardMatrix endpoints are present and interpolates the ForwardMatrix at the resolved white.

### A7. Only one ForwardMatrix in a dual-calibration file — PROJECT POLICY / REVIEW REQUIRED

TruthRaw currently reuses the single available ForwardMatrix across the entire supported temperature range and records:

`usedSingleForwardMatrixAcrossTemperatures = true`

This behavior is explicit rather than hidden, which is good provenance. However, DNG Chapter 6 describes `FM` as the matrix interpolated from the calibration ForwardMatrix tags. The specification does not establish TruthRaw's one-endpoint-across-two-calibrations policy as an independently equivalent scientific transform.

Audit decision:

- keep this state `SOURCE_METADATA_BOUND` only;
- do not promote it to independent calibration;
- add a future strict-scientific option that can fail closed on a partial dual ForwardMatrix set rather than silently extrapolating.

This is a compatibility/policy issue, not evidence that the current source-bound preview is invalid.

### A8. Three calibration sets — FAIL-CLOSED / INCOMPLETE COMPATIBILITY

DNG 1.6+ supports one, two, or three color calibration sets. If a third calibration is included, DNG defines additional consistency requirements.

TruthRaw v0.2 detects a third calibration and returns `TripleCalibrationUnsupported`.

This is scientifically safe because it fails closed, but it is not complete DNG 1.7.1 support.

### A9. Custom illuminant 255 / IlluminantData — FAIL-CLOSED / INCOMPLETE COMPATIBILITY

DNG 1.6+ allows CalibrationIlluminant value 255 for a custom illuminant, paired with IlluminantData that can provide x-y chromaticity or spectral power distribution information.

TruthRaw v0.2 returns `UnsupportedCalibrationIlluminant` for 255 because IlluminantData parsing is not implemented.

Again, this is safe but incomplete.

## B. Numerical precision audit

### B1. Color solve internal precision — PASS

The v0.2 matrix algebra, white solve, interpolation and matrix inversion use `double` internally.

### B2. Color binding storage boundary — EXPLICIT F32 BOTTLENECK

`ScientificColorBindingRecord.cameraToXyzD50` is currently:

`std::array<float, 9>`

The producer derives the matrix in Float64 and then explicitly converts each coefficient to Float32.

This is not an authority problem, but it is a precision boundary and must be stated as such.

### B3. Runtime camera-RGB -> XYZ multiplication — F32 COMPUTE / IMPROVEMENT CANDIDATE

Frozen v4.7i performs the camera-RGB -> XYZ(D50) matrix multiply using Float32 coefficients, Float32 reconstructed samples, Float32 multiply/add and Float32 output.

For consistency with the new mixed-precision reconstruction policy, a future research step should evaluate:

- Float32 matrix storage + Float64 accumulation + Float32 XYZ storage; and
- optionally an F64 matrix-binding record or sidecar for the scientific coordinate transform.

This is a numerical-quality change only. It must not change color authority.

### B4. XYZ(D50) -> display RGB — PRESENTATION BOUNDARY

The frozen core converts XYZ(D50) to linear sRGB in Float32 for the neutral/display path. That step is not itself proof of scene color truth and should remain downstream of the scientific color-binding authority.

## C. Authority audit

### C1. Current source-metadata authority — CORRECT

The v0.2 producer emits:

`ColorBindingAuthority::SourceMetadataBound`

This is the correct current authority. Embedded DNG matrices, AsShotNeutral, Camera2 static matrices, runtime gains, and named standard illuminants are not by themselves independent physical calibration.

### C2. Camera2 telemetry — CORRECTLY OBSERVATION-ONLY

`CameraCalibrationTelemetry.kt` records Camera2 black/white/noise/neutral/green-split/lens/color metadata with:

- `CAMERA2_RESULT_OBSERVATION_ONLY`;
- `calibrationAuthorityGranted=false`;
- `scientificWritebackAllowed=false`.

This boundary should remain unchanged.

### C3. AsShotNeutral is not measured SPD — PASS

DNG defines AsShotNeutral as the selected white balance at capture in neutral camera coordinates. It is not an independent spectral measurement of the illuminant.

TruthRaw's physical-calibration campaign already forbids treating AsShotNeutral as measured SPD. Keep this invariant.

## D. Device-specific Camera-5 color audit

The existing Camera-5 diagnostic is important negative evidence:

- the sealed physical Camera-5 DNG contained dual ColorMatrix/ForwardMatrix metadata;
- the DNG AsShotNeutral/ForwardMatrix path produced a strong blue cast in nominally neutral scene regions;
- a manually estimated scene neutral reduced the cast, but the project correctly classified that estimate as diagnostic only, not calibration.

Therefore:

1. metadata-conformant processing is not the same as proof that the vendor metadata is physically correct for that capture;
2. TruthRaw must retain the source metadata rather than silently overwrite it;
3. the source-bound color result may be flagged as unresolved/inconsistent when DNG and physical Camera2 color observations disagree;
4. the diagnostic neutral must never become Scientific-Master calibration evidence.

## E. Independent physical color calibration status

Current status remains NOT ESTABLISHED.

The existing Tele FULL_PHYSICAL campaign correctly requires a separate chain:

`calibration capture -> model fit -> held-out validation -> promotion`

For color, promotion requires at minimum:

- original admitted camera RAWs;
- known physical color target / spectral references;
- characterized illuminant evidence, ideally including measured SPD where the claim requires it;
- explicit camera/lens/unit/readout/focus domain binding;
- model fit separated from held-out validation;
- disjoint physical target IDs for training and final validation;
- uncertainty and out-of-domain behavior;
- no use of TruthRaw-derived DNG as calibration input.

Only after those gates pass may a bounded domain move toward `IndependentCalibration` / `FULL_PHYSICAL`.

## Formal status matrix

| Item | Status | Authority consequence |
|---|---|---|
| ColorMatrix direction | PASS | source-bound transform permitted |
| CameraCalibration direction | PASS | source-bound transform permitted |
| Signature matching | PASS | fail-closed identity on mismatch |
| Dual inverse-CCT interpolation | PASS | supported two-set source-bound domain |
| Neutral-to-xy iterative solve | PASS | fail closed on non-convergence |
| Bradford fallback | PASS | supported source-bound domain |
| Dual ForwardMatrix, both endpoints | PASS | supported source-bound domain |
| Single ForwardMatrix across dual range | REVIEW_REQUIRED | source-bound only; strict mode candidate |
| Third calibration set | FAIL_CLOSED_UNSUPPORTED | no transform produced |
| Custom IlluminantData / 255 | FAIL_CLOSED_UNSUPPORTED | no transform produced |
| Internal producer matrix math | F64_PASS | no authority promotion |
| Stored cameraToXyzD50 matrix | F32_BOUNDARY | precision boundary only |
| Runtime camera->XYZ multiply | F32_RESEARCH_GAP | precision improvement candidate |
| Camera-5 metadata consistency | UNRESOLVED | do not promote beyond source-bound |
| Independent target/spectral calibration | NOT_ESTABLISHED | FULL_PHYSICAL blocked |

## Ordered follow-up

1. validate and integrate the new F64 reconstruction backend on the research branch;
2. add a camera->XYZ Float64 accumulation A/B test while retaining Float32 storage;
3. add a strict dual-ForwardMatrix policy option and test it before any scientific promotion;
4. add DNG third-calibration and IlluminantData support only with new fail-closed tests;
5. build a DNG-vs-Camera2 color consistency sidecar for real physical-camera captures;
6. execute the external target/illuminant held-out physical calibration campaign separately.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
