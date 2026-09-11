# Android Source-Bound Color Preview v0.1

Status: RESEARCH PROTOTYPE — CI PENDING

Date: 2026-09-11

## Purpose

This candidate replaces the Android UI's gray CFA parser-sentinel path with a real source-bound color preview path while preserving TruthRaw's evidence and authority boundaries.

Execution path:

`ParcelFileDescriptor(fd)`
→ bounded SHA-256 source seal
→ DNG Color Binding Producer v0.1
→ Scientific Preview Source Binding v0.2 phase-1 preparation
→ `TileNativeDngSource`
→ v4.7i `StreamingTruthRawProcessor`
→ `BoundedSrgbPreviewSink`
→ Android sRGB `Bitmap`
→ optional JPEG appearance export.

## Authority boundary

The displayed result is **SOURCE_BOUND_APPEARANCE_PREVIEW**, not a finalized Scientific Preview and not a Scientific Master.

Required phase-1 state:

- `mainHouseComputeAllowed = true`
- `sourceBoundAppearanceReleaseAllowed = true`
- `scientificPreviewReleaseAllowed = false`
- `scientificClaimAllowed = false`
- `physicalFrameCount = 1`
- `independentEvidenceCount = 1`

No dummy `scientificMasterHash`, zero-line hash, scene-scale hash, identity color matrix, or PreviewSentinel is manufactured by this bridge.

## Color

Color authority is `SOURCE_METADATA_BOUND` from the exact sealed DNG bytes. DNG Color Binding Producer v0.1 remains deliberately fail-closed for unsupported/ambiguous profiles, including dual/triple calibration sets that require interpolation not implemented in v0.1.

This is source-faithful DNG rendering metadata. It is not independent camera/lens spectral calibration and does not justify a `FULL_PHYSICAL` color claim.

## Reconstruction / appearance

The Android proof uses:

- `ResearchEdgeAwareMeasuredPreservingReconstruction` (v4.7i),
- `NeutralReferenceAppearance`,
- the existing two-pass Full-Frame Streaming processor,
- one worker,
- 128-pixel core / 16-pixel halo,
- bounded preview long edge (384 px in Kotlin),
- no scientific diagnostic full frame.

The preview sink owns only the bounded ARGB preview plus write-ownership bookkeeping. The streaming adapter must report that it owns no full RAW, SDR, half-gain, or diagnostic frame.

## Source and memory

The source file is never copied into a UI byte array. SHA-256 verification reads the source through bounded chunks. `TileNativeDngSource` reads CFA payload by requested tiles/striles. The research UI currently supplies:

- source resident ceiling: 8 MiB,
- logical streaming resident ceiling: 64 MiB.

These are v0.1 preview proof limits, not the final Building Runtime resource policy. Scientific output authority cannot depend on these limits.

## JPEG export

The optional JPEG export is made from the already bounded sRGB appearance preview using Android's JPEG encoder. It is explicitly presentation-only. Saving a JPEG does not create evidence, does not modify the sealed source, and does not finalize the Scientific Master.

## Fail-closed behavior

The color preview is blocked when any of the following occurs:

- source SHA-256 cannot be produced/reverified;
- DNG color metadata is missing, malformed, unsupported, ambiguous, or source-mismatched;
- phase-1 authority is not exactly the permitted appearance-only state;
- TileNative DNG parsing/binding fails;
- streaming reconstruction/sink fails;
- memory ceiling is exceeded;
- a full-frame adapter/source materialization flag becomes true;
- frame/evidence counts differ from 1/1;
- appearance is reported as mutating the Scientific Master.

There is no automatic fallback to the gray sentinel preview inside `TilePreviewLoader`.

## Proof boundary

Until CI and physical-device testing exist, this branch proves only source-level integration intent. APK build success will not equal physical Honor/MotionCam DNG proof. A real MotionCam/Honor DNG, on-device RSS/thermal/frame-time observations, and the future deterministic Scientific Master digest/Backplane phase-2 finalization remain separate validation steps.
