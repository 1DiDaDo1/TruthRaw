# TruthRaw Linear DNG Projection v0.2 — RGB output restoration

**Status:** `HOST_AND_ANDROID_BUILD_VALIDATED__PHYSICAL_LIGHTROOM_PREVIEW_INTEROP_PENDING`

**Branch:** `research/restore-rgb-linearraw-output-v0.2-workbase-2026-09-13`

**Base:** `7c040218f6c5d43c6faeff6ab95d9fec763a1046` (`research/linear-dng-export-v0.1-2026-09-13`)

This module restores important behavior from the earlier validated TruthRaw RGB / LinearRaw DNG line while preserving the newer finalized-release, source-binding and bounded-streaming architecture.

The first Android Linear DNG export implementation reproduced the correct high-level Scientific-Master-to-LinearRaw route, but lost parts of the older finite-range compatibility encoding, camera identity contract and visible-preview integration.

## 1. Non-negotiable scientific boundary

TruthRaw keeps:

`sealed Direct-CFA evidence -> Measurement -> Reconstruction -> Scientific Master -> downstream projection`

The Linear DNG is a `COMPATIBILITY_PROJECTION`.

It is not the original measured CFA, a new physical exposure, new photons, a promotion of reconstructed RGB to measured sensor truth, a color-calibration authority upgrade, or the Scientific Master itself.

The Scientific Master remains camera-native reconstructed scene-linear RGB before `camera_to_xyz()` and before appearance/tone/display processing.

`physicalFrameCount=1` and `independentEvidenceCount=1` remain unchanged.

No export may change Scientific Master hash, zero-line, scene binding, Backplane identity, source seal or color-claim scope.

> **Measured where measured. Reconstructed where necessary. Never invented.**

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## 2. Historical RGB / LinearRaw behavior being restored

Earlier real TruthRaw RGB LinearRaw candidates demonstrated that a compatibility container must not simply clamp reconstructed scene-linear values at 1.0.

Recorded real-scene examples:

- scene maximum `1.013005137` -> finite compatibility window `1.25x` -> `BaselineExposure = +0.321928 EV`;
- scene maximum `1.730840802` -> finite compatibility window `2.0x` -> `BaselineExposure = +1.000000 EV`.

Older validated DNG candidates retained physical source-camera identity, including:

`UniqueCameraModel = BKQ-N49-HONOR-HONOR`

Those historical exact files passed their recorded DNG SDK validation. That is interoperability evidence for those bytes, not Adobe certification and not a physical-truth promotion.

## 3. Regression identified in v0.1 / Work-built Android Linear DNG

The v0.1 writer on the 2026-09-13 Linear-DNG branch:

1. quantized camera-native reconstructed RGB directly into unsigned 16-bit `[0,1]`;
2. clipped values `>1.0` to `65535`;
3. wrote `BaselineExposure = 0 EV`;
4. wrote `UniqueCameraModel = TruthRaw LinearRaw Projection v0.1`;
5. copied source-bound Honor color metadata into that synthetic camera identity;
6. wrote a single primary LinearRaw IFD without an embedded compatibility preview.

The exact Work-produced APK containing this line is bound in:

`evidence/WORK_APK_FORENSIC_2026-09-13.md`

## 4. v0.2 RGB restoration design

v0.2 leaves the historical v0.1 writer intact and wraps it at the downstream representation boundary.

### 4.1 Fixed historical 2x compatibility window

For the first Android restoration candidate, camera-native reconstructed RGB is multiplied by `0.5` only while being passed to the finite DNG representation writer.

The DNG carries:

`BaselineExposure = +1 EV`

Thus scene-linear values up to `2.0` remain representable.

Example:

`scene 1.5 -> encoded 0.75 -> DNG BaselineExposure +1 EV -> nominal downstream scene interpretation 1.5`

The Scientific Master is unchanged.

### 4.2 Fail closed above the validated finite window

If the underlying writer still reports high clipping after the representation has been divided by two, the original scene exceeded `2.0`.

v0.2 rejects the export and truncates the destination rather than silently calling the clipped result faithful.

### 4.3 Restore source camera identity

v0.2 restores source-bound:

- `Make`;
- `Model`;
- `UniqueCameraModel`.

`UniqueCameraModel` is required because copied source DNG color metadata must remain tied to its real source-camera identity. This does not increase color authority beyond source-bound metadata.

## 5. Modern preview architecture is leading

The preview restoration target is **not** defined by one historical TIFF/DNG IFD arrangement.

The later 2026-09-11 TruthRaw Preview Representation and Finalized Scientific Preview architecture is leading:

```text
sealed source
 -> source-bound color
 -> Scientific Master + digest
 -> TruthRange / zero-line binding
 -> Technical Backplane phase 2
 -> finalized reconstructed/color/appearance preview
 -> bounded ARGB_8888 / sRGB surface
      |-> live UI
      |-> portable JPEG / sRGB
      |-> embedded DNG JPEG compatibility preview
      |-> optional HDR display derivative
```

Preview Representation v0.1 explicitly says TruthRaw must not choose one encoded image format as the universal preview representation.

The DNG embedded-preview note defines:

```text
one scientific/raw result
        +--> DNG/raw payload
        +--> embedded JPEG preview (compatibility only)
        +--> standalone JPEG preview (visibility/share only)
```

The same bounded finalized preview representation must therefore be the source of both portable and embedded JPEG forms. The DNG container transports the preview; it does not define its scientific authority.

Detailed restoration source:

`evidence/MODERN_FINALIZED_PREVIEW_ARCHITECTURE_2026-09-13.md`

The exact old-chat phase label remembered as approximately `1.3` / `1.4` has not been proven by an exact source and is intentionally not assigned.

## 6. Historical v0.7 container is evidence, not the modern architecture

The earlier Android-preview fix used:

- reduced IFD0 thumbnail;
- full RGB LinearRaw SubIFD;
- JPEG preview SubIFD.

It proved that JPEG preview packaging could coexist with unchanged LinearRaw bytes and pass the historical validator gates.

That layout remains a valuable **container/interoperability candidate and regression reference**, but it is not automatically the final v0.2/vNext container topology.

Historical evidence:

`evidence/HISTORICAL_ANDROID_PREVIEW_RESTORE_SOURCE_2026-09-13.md`

## 7. Current Android implementation already contains the modern preview path

The current UI uses:

`NativeTilePreviewBridge.buildFinalizedScientificColorPreview(...)`

which runs source seal/color binding, Scientific Master v0.2, TruthRange, Technical Backplane phase 2 and finalized Scientific Preview release before returning bounded ARGB pixels.

`TilePreviewLoader` then creates an explicit sRGB `ARGB_8888` bitmap.

`PortablePreviewEncoder` already implements the later representation design:

- source must be ARGB_8888;
- source color space must be sRGB;
- baseline JPEG;
- default quality `92`;
- caller-owned `OutputStream`.

The UI currently saves this same finalized bitmap as standalone JPEG.

Current limitation: the live UI is capped at `384 px` (`512 px` native hard cap), whereas Preview Representation v0.1 suggests a portable display preview edge up to `2048 px`. A portable/export representation therefore needs its own explicit bounded size policy without becoming a second scientific pipeline.

## 8. Files and Android integration in v0.2

New v0.2 module files:

- `native/linear_dng_projection_v0_2.h`
- `native/linear_dng_projection_v0_2.cpp`
- `tests/test_linear_dng_projection_v0_2.cpp`
- `CMakeLists.txt`

Android integration points the Work-derived export route at v0.2 through:

- `app/android/truthraw-adaptive-ui-v01/app/src/main/cpp/CMakeLists.txt`
- `app/android/truthraw-adaptive-ui-v01/app/src/main/cpp/linear_dng_export_bridge.cpp`
- `app/android/truthraw-adaptive-ui-v01/app/src/main/java/com/truthraw/adaptiveui/LinearDngExport.kt`
- app identity `versionCode=6`, `versionName=0.6-linear-dng-restore`

The historical v0.1 module remains preserved.

## 9. Host validation — PASS

GitHub Actions run `34780375681` passed:

- GCC Release;
- Clang Release;
- Clang ASan/UBSan.

Evidence:

`evidence/HOST_CI_34780375681_2026-09-13.md`

The synthetic regression proves the 2x window, `BaselineExposure=+1 EV`, source camera identity retention, value `1.5` preservation and fail-closed behavior above `2.0`.

## 10. Android arm64 build — PASS

GitHub Actions run `34780463331` passed for built head `e7bcb103215f97bc0ca68e526bd07453ade46a1c`.

Exact APK:

- bytes: `4335889`;
- SHA-256: `93e51245e0950c5c3140b83f2f3429d2f52ad48adcd5630409134e4c430b5654`;
- version `0.6-linear-dng-restore`;
- ABI `arm64-v8a`.

Evidence:

`evidence/ANDROID_CI_34780463331_2026-09-13.md`

This build proof does not equal physical Honor, Lightroom or embedded-preview proof.

## 11. Next implementation gate

The next preview work must follow this order:

1. keep the finalized Scientific Preview authority path unchanged;
2. keep the v0.2 RGB LinearRaw finite-headroom path unchanged;
3. derive the portable JPEG from the same bounded finalized sRGB preview representation;
4. embed that JPEG downstream in DNG without using it as science;
5. permit the same representation to remain available as standalone JPEG;
6. prove that changing/removing/re-encoding the embedded preview changes no Scientific Master, TruthRange, Backplane or RGB LinearRaw payload identity;
7. then validate the exact DNG container topology on Honor/Android, Lightroom/ACR, LibRaw and DNG SDK.

The historical multi-IFD topology may be reused as a candidate only after it is evaluated under this modern representation contract.

## 12. Still open

Not yet claimed complete:

1. physical execution on Honor BKQ-N49;
2. Lightroom open/edit behavior for a newly produced v0.2/vNext DNG;
3. independent parser / LibRaw / DNG SDK validation of the new exact bytes;
4. embedded finalized-JPEG integration in the DNG writer;
5. portable-preview size/performance policy above the 384 px live UI surface;
6. reconstructed CFA DNG and rawsensor outputs, which remain separate projection classes.

## 13. Promotion rule

Do not call the restored route Lightroom-compatible, production-ready or physically proven until the exact produced bytes pass the independent physical-device and reader gates.

Host and Android build validation are green. They do not close the remaining interoperability gates.

Failures remain evidence and must not be hidden by retuning labels or thresholds.
