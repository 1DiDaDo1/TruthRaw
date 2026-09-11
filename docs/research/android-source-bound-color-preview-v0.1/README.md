# Android Source-Bound Color Preview v0.1

Status: **RESEARCH PROTOTYPE — ANDROID ARM64 APK CI PASS**

Date: 2026-09-11

Validated implementation SHA: `65818f529d4588c2ed907fb11787e3dd83d4432c`

Validation workflow run: `34613390230`

## Purpose

This candidate replaces the Android UI's active gray CFA parser-sentinel path with a real source-bound color preview path while preserving TruthRaw's evidence and authority boundaries.

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

The old gray CFA sentinel bridge remains present only as a separately governed diagnostic path. `TilePreviewLoader` does not silently fall back to it.

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

- `ResearchEdgeAwareMeasuredPreservingReconstruction` from frozen canonical v4.7i;
- `NeutralReferenceAppearance`;
- the existing two-pass Full-Frame Streaming processor;
- one worker;
- 128-pixel core / 16-pixel halo;
- bounded preview long edge of 384 px in Kotlin;
- no scientific diagnostic full frame.

The preview sink owns only the bounded ARGB preview plus write-ownership bookkeeping. The streaming adapter must report that it owns no full RAW, SDR, half-gain, or diagnostic frame.

Canonical v4.7i bytes remain unchanged. Android NDK Clang 18 reports one `-Wmisleading-indentation` warning in the frozen compact `core.cpp`; the Android CMake integration makes only that one warning non-fatal for that one canonical translation unit. Integration/research sources remain under target-wide `-Wall -Wextra -Werror`. The contract verifier prohibits broadening this exception.

## Source and memory

The source file is never copied into a UI byte array. SHA-256 verification reads the source through bounded chunks. `TileNativeDngSource` reads CFA payload by requested tiles/striles. The research UI currently supplies:

- source resident ceiling: 8 MiB;
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

## Validated Android build

The exact implementation at `65818f529d4588c2ed907fb11787e3dd83d4432c` passed workflow run `34613390230`.

Observed proof:

- Android source-bound color-preview contract: PASS;
- Building Runtime v0.1 integrity: PASS;
- Technical Backplane v0.1 integrity: PASS;
- Tile-Native DNG Source v0.1 integrity: PASS;
- Adaptive UI Ingress v0.1 contract: PASS;
- source/color authority boundary: PASS;
- no camera permission: PASS;
- Kotlin compilation: PASS, with only the already-known `setDecorFitsSystemWindows` deprecation warning;
- Android NDK/CMake arm64 compilation and JNI link: PASS;
- APK assembly: PASS (`BUILD SUCCESSFUL in 2m 22s`);
- packaged native bridge: `lib/arm64-v8a/libtruthraw_ui_preview_bridge.so`, 931,048 bytes;
- APK size: 3,356,541 bytes;
- APK SHA-256: `b377ba11f6dff14ddf0375e7f1146650e62e3d28b3f4e4608de0289e6604af66`;
- artifact name: `truthraw-android-source-bound-color-preview-v0.1-debug-arm64`;
- artifact ID: `10269945881`;
- uploaded artifact ZIP size: 1,078,019 bytes;
- uploaded artifact ZIP SHA-256: `7f72f3fae7f28756f2e34be3e56bb3fe4c9206a54d10ab06049c1d0ed6b76780`;
- CI reports `color_authority=SOURCE_METADATA_BOUND`;
- CI reports `appearance_release=ALLOWED_LABELED`;
- CI reports `scientific_preview_release=BLOCKED_PRE_MASTER`;
- CI reports `scientific_claim=BLOCKED_PRE_MASTER`;
- CI reports `source_raw_full_materialization=0_by_contract`;
- CI reports `physical_frame_count=1 independent_evidence_count=1`.

## Preserved failure history

The first Android integration candidate at `3ea45d9e65a3281b862378e9affefec643b4af19` failed workflow run `34612805148` during `assembleDebug` because Android NDK Clang 18 promoted the frozen canonical v4.7i `-Wmisleading-indentation` style diagnostic to an error under target-wide `-Werror`.

That run remains **FAIL**. The repair did not modify canonical v4.7i bytes and did not globally weaken `-Werror`; it introduced one source-scoped toolchain-compatibility exception for frozen `core.cpp`. Full details are preserved in `FAILURE_HISTORY_v0_1.md`.

## Proof boundary

This proves build/package/static-contract integration for the arm64 Android candidate. It does **not** yet prove:

- successful execution on the physical Honor target;
- successful preview of a real MotionCam/Honor DNG on-device;
- on-device RSS, thermal behavior, latency or frame time;
- arbitrary provider compatibility beyond the existing seekable-fd contract;
- independently calibrated camera/lens color;
- `FULL_PHYSICAL` color truth;
- a deterministic Scientific Master digest;
- a real phase-2 Technical Backplane finalization;
- finalized Scientific Preview authority;
- multi-capture fusion/HDR science.

A real MotionCam/Honor DNG and physical-device run are therefore the next empirical validation boundary, while Scientific Master digest/Backplane phase 2 remains a separate architecture step.
