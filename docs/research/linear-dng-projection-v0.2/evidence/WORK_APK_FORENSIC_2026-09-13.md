# Work APK forensic binding — 2026-09-13

This file binds the RGB / LinearRaw restoration work to the exact Work-produced Android APK that was inspected during the regression investigation.

## Exact APK

- file: `TruthRaw-linear-dng-v0.1-debug-arm64.apk`
- bytes: `4300505`
- SHA-256: `00236c2166d909ed1d2589cf66355ccd022f7080312e0b1c54491fa98cd38331`

The APK itself is not committed to the repository.

## Native evidence recovered from the exact APK

The APK contains `lib/arm64-v8a/libtruthraw_ui_preview_bridge.so`.

The arm64 shared library exports:

- `Java_com_truthraw_adaptiveui_LinearDngNativeBridge_exportFinalizedLinearDng`
- `truthraw::linear_dng_projection::v0_1::write_finalized_linear_dng(...)`

Relevant embedded strings include:

- `TruthRaw Linear DNG Projection v0.1`
- `Scientific Master Linear Projection`
- `LinearRaw projection requires a finalized Scientific Preview admission`
- `LinearRaw projection requires a real Scientific Master digest`
- `source ColorMatrix1/AsShotNeutral required for LinearRaw compatibility projection`
- `cannot write LinearRaw pixel row`

This establishes that the Work APK contains the actual finalized-release-to-RGB-LinearRaw v0.1 native writer, rather than only UI scaffolding.

## Repository correspondence

The inspected APK behavior corresponds to the 2026-09-13 Linear DNG branch family based on:

- branch: `research/linear-dng-export-v0.1-2026-09-13`
- recorded head used as v0.2 restoration base: `7c040218f6c5d43c6faeff6ab95d9fec763a1046`

The restoration branch is:

`research/restore-rgb-linearraw-output-v0.2-workbase-2026-09-13`

The restoration deliberately starts from this Work-compatible branch rather than from the separate 12-september three-output research branch because the Work line already contains:

- Android JNI export wiring;
- finalized Scientific Preview admission;
- exact source SHA re-verification;
- source/backplane identity comparison;
- bounded native streaming;
- Android SAF save flow.

Those are retained. The downstream DNG representation is what is being repaired.

## Regression relevance

The v0.1 native writer represented reconstructed camera-native RGB directly in unsigned 16-bit `[0,1]`, counted/clipped values above 1.0, wrote `BaselineExposure = 0 EV`, and used a synthetic TruthRaw `UniqueCameraModel` while copying source camera color metadata.

The historical TruthRaw RGB / LinearRaw line had already demonstrated finite compatibility headroom above scene value 1.0 and source-camera identity retention.

Therefore the Work APK is treated as a valuable executable negative/reference point for the export regression, not as scientific authority.

## Authority boundary

APK inspection may establish which software path was built and packaged. It may not determine TruthRaw:

- physical evidence;
- sensor topology;
- noise authority;
- camera/lens calibration;
- source color truth;
- scientific-master content;
- reconstruction authority.

The scientific source of truth remains the sealed RAW/DNG evidence and the validated TruthRaw modules.

## Validation status

This forensic binding does **not** prove the Work APK's output is Lightroom-compatible or scientifically correct. The user-observed failure — exported RAW without useful preview and not practically editable in Lightroom — remains a regression symptom to reproduce and close with the new v0.2 branch.
