# DNG color transform study note

TruthRaw reviewed Adobe DNG Specification 1.7.1.0 (September 2023) before implementing a source-metadata color-binding producer.

Key normative points used by the research implementation:

- DNG maps camera color-space linear reference values to CIE XYZ with D50 white.
- For one/two/three calibrations, the effective matrices are defined from ColorMatrix, CameraCalibration, AnalogBalance, ReductionMatrix and ForwardMatrix data.
- The DNG model defines `XYZtoCamera = AB * CC * CM`.
- If ForwardMatrix is absent and camera dimensionality is 3, `CameraToXYZ = inverse(XYZtoCamera)` and a chromatic-adaptation matrix maps the selected white balance to D50; Adobe recommends linear Bradford.
- If ForwardMatrix is present, the DNG model defines `CameraToXYZ_D50 = FM * D * inverse(AB * CC)`, with `D` derived from the reference-camera neutral.
- CameraCalibration matrices are only used when CameraCalibrationSignature matches ProfileCalibrationSignature; otherwise identity is preferred.
- With two or three calibrations, DNG requires interpolation based on selected white balance; this is not silently approximated by v0.1.

## v0.1 deliberate subset

The first producer is intentionally restricted to a single 3-channel calibration set. It accepts source metadata only when it can derive the transform without guessing:

- one `ColorMatrix1` (required);
- `AsShotNeutral` with three positive finite components (required);
- optional `AnalogBalance`, default identity;
- optional `CameraCalibration1`, used only when calibration/profile signatures match; otherwise identity;
- optional `ForwardMatrix1`;
- second/third calibration tags cause fail-closed `MULTIPLE_CALIBRATIONS_UNSUPPORTED` in v0.1;
- no profile HueSatMap/LookTable is folded into the scientific color binding;
- no lens correction or lens calibration is inferred from camera-color metadata.

This producer yields authority `SOURCE_METADATA_BOUND`: a source-bound DNG rendering transform suitable for the reconstructed preview path. It is not independent camera/lens calibration and is not `FULL_PHYSICAL` color truth.

Reference: Adobe DNG Specification 1.7.1.0, especially the section “Mapping Camera Color Space to CIE XYZ Space” (pp. 100–103 in the document numbering).
