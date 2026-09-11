# Android Finalized Scientific Preview v0.1

Status: **RESEARCH CANDIDATE — HOST + ANDROID ARM64 CI PASS; PHYSICAL HONOR EXECUTION NOT YET PROVEN**

Date: 2026-09-11

Validated implementation SHA before this proof-document commit: `f24cba41b3987bd735296ff0c186e8390cc0c5b0`

Validation workflow run: `34643813402`

Host job: `103409675848` — PASS

Android arm64 APK job: `103409675467` — PASS

## Purpose

This candidate moves the Android UI from the pre-master `SOURCE_BOUND_APPEARANCE_PREVIEW` stage to a finalized Scientific Preview release path without changing the established bounded sRGB/ARGB/JPEG rendering role.

Finalization path:

`ParcelFileDescriptor(fd)`
→ SHA-256 source seal
→ source-metadata DNG color binding
→ Scientific Preview Source Binding phase-1 preparation
→ pre-render SHA-256 re-verification
→ `TileNativeDngSource` opened from the same shared random-access source
→ streaming Scientific Master identity
→ TruthRange self-gauge / scene binding
→ Technical Backplane phase 2
→ finalized Scientific Preview admission
→ existing v4.7i streaming reconstruction + Neutral appearance
→ `BoundedSrgbPreviewSink`
→ Android explicit-sRGB ARGB_8888 `Bitmap`
→ optional JPEG presentation export
→ post-render SHA-256 re-verification.

The older source-bound color JNI route remains present as a separate diagnostic/pre-master path. `TilePreviewLoader` now uses the finalized route by default and does not silently downgrade a failed finalization into a weaker preview label.

## Authority boundary

A successful source-metadata route is labeled:

`FINALIZED_SOURCE_BOUND_SCIENTIFIC_PREVIEW`

This means the preview release lineage has closed through a real Scientific Master identity, TruthRange/scene binding and Technical Backplane phase 2. It does **not** mean the DNG source metadata has become an independently calibrated physical color measurement.

For source-metadata color:

- `scientificPreviewReleaseAllowed = true` after successful finalization;
- `scientificClaimAllowed = false`;
- `physicalFrameCount = 1`;
- `independentEvidenceCount = 1`.

Only `FINALIZED_INDEPENDENTLY_CALIBRATED_SCIENTIFIC_PREVIEW` is permitted to carry the stronger scientific color-claim flag. The Android default path does not manufacture that authority.

## Source identity

The direct Android route uses one shared random-access byte source for source sealing, DNG metadata production and `TileNativeDngSource` opening.

The sealed SHA-256 source identity is re-verified immediately before the finalized processing path and again after rendering. A mismatch fails closed.

This is stronger than trusting only a caller-supplied source ID string. It does not claim cryptographic proof that a hostile provider could not mutate bytes during processing and then restore them before the post-check. The current candidate assumes the normal read-only Android document-provider/file-descriptor contract.

## Scientific Master and Backplane

The finalized route no longer supplies fixture or placeholder Scientific Master identity to the preview gate.

The in-process release function computes/binds:

- deterministic Scientific Master identity through the streaming scientific binding;
- zero-line self-gauge;
- scene-scale binding;
- canonical Technical Backplane phase-2 state;
- preview authority derived from the admitted color claim scope.

The release path rejects source/color identity drift between the prepared source, opened tile source and recomputed phase-2 admission.

## Reconstruction and appearance

The Android proof continues to use:

- `ResearchEdgeAwareMeasuredPreservingReconstruction` from frozen canonical v4.7i;
- `NeutralReferenceAppearance`;
- existing two-pass Full-Frame Streaming;
- one worker;
- 128-pixel core / 16-pixel halo;
- bounded preview long edge of 384 px in Kotlin;
- no scientific diagnostic full frame.

The authority integration does not intentionally introduce a new look, tone curve or color style.

An explicit host equivalence test compares the new in-process finalized route with the already validated persisted-Backplane release route on the same synthetic source. The test requires equality of:

- authority;
- Scientific Master hash;
- zero-line `L0` and gauge ID;
- scene-scale ID;
- serialized Technical Backplane bytes;
- preview dimensions;
- the complete ARGB_8888 pixel vector;
- written pixel count.

That fixture passes. This establishes tested-path pixel equivalence; it is not a universal proof over every possible RAW input.

## Provenance and resource invariants

The release gate requires:

- `physicalFrameCount == 1`;
- `independentEvidenceCount == 1`;
- appearance does not modify the Scientific Master;
- no counterfactual observation is created;
- GainMap application is reported exactly once where applicable;
- streaming adapter owns no full RAW, SDR, half-gain or diagnostic frame;
- bounded preview sink is complete.

The Android UI additionally fails closed if full RAW materialization is reported, if logical memory exceeds its current research ceiling, or if either streaming pass processes zero tiles.

Current research UI ceilings remain:

- source resident ceiling: 8 MiB;
- logical streaming resident ceiling: 64 MiB.

These are execution/resource limits, not scientific authority parameters.

## Android build proof

Exact implementation head `f24cba41b3987bd735296ff0c186e8390cc0c5b0` passed workflow run `34643813402` before this proof-document commit.

Observed CI evidence:

- host finalized-release CMake build: PASS;
- original finalized release test: PASS;
- in-process vs persisted-Backplane equivalence test: PASS;
- Android authority-integration static contract: PASS;
- no `android.permission.CAMERA`: PASS;
- Kotlin/Gradle build: PASS;
- Android NDK/CMake arm64 compile/link: PASS;
- APK assembly: PASS (`BUILD SUCCESSFUL`);
- packaged bridge: `lib/arm64-v8a/libtruthraw_ui_preview_bridge.so`;
- packaged bridge size: 1,682,624 bytes;
- APK size: 4,108,117 bytes;
- APK SHA-256: `81e583fefcadd23bcb15a2fa09b90b0a7672e9059f82ef3971e39e8642b06434`;
- artifact name: `truthraw-android-finalized-scientific-preview-v0.1-debug-arm64`;
- artifact ID: `10280934886`;
- artifact ZIP size: 1,296,577 bytes;
- artifact ZIP SHA-256: `391897129da49aa314be80b83e1fc6ad3821ac5f4afd06c4f8bdfd6c77bfe9d8`;
- CI records `preview_role=FINALIZED_SOURCE_BOUND_SCIENTIFIC_PREVIEW_or_stronger`;
- CI records `source_reverification=PRE_AND_POST_RENDER`;
- CI records `scientific_claim=INDEPENDENT_CALIBRATION_ONLY`;
- CI records `physical_frame_count=1 independent_evidence_count=1`.

The APK artifact is a research/debug artifact, not a promoted production release.

## Toolchain notes

Frozen canonical v4.7i remains byte-unchanged. Android NDK Clang reports the already-known `-Wmisleading-indentation` warning in the frozen compact canonical translation unit; only that source file carries the existing source-scoped non-fatal exception. Research/integration sources remain under target-wide warning-as-error policy.

The Android Kotlin build also reports the existing `setDecorFitsSystemWindows` deprecation warning. It is not a scientific or rendering-authority failure.

## Fail-closed behavior

The finalized preview is blocked if, among other cases:

- source SHA-256 seal or re-verification fails;
- DNG color metadata is missing, malformed, source-mismatched or unsupported;
- the pre-master state already claims finalized authority;
- TileNative parsing/binding fails;
- Scientific Master/TruthRange identity cannot be produced;
- canonical phase 2 cannot be finalized;
- opened source/color identity differs from prepared or phase-2 identity;
- finalized authority is `None`;
- streaming/provenance/resource invariants fail;
- bounded preview output is incomplete;
- Android receives an unknown authority code.

There is no automatic relabeling of a weaker preview as finalized.

## Known real-DNG blocker: multiple color calibrations

DNG Color Binding Producer v0.1 deliberately fails closed for dual/triple calibration sets that require illuminant-dependent interpolation. It does not arbitrarily choose `ColorMatrix1` or `ColorMatrix2`.

Therefore real Honor/MotionCam DNGs containing multiple calibration sets can still be rejected before a finalized color preview is produced. Correct dual-illuminant interpolation is the next color-science implementation boundary for those sources.

## Proof boundary / non-claims

This candidate proves host scientific-gate integration, tested in-process/persisted-route identity and Android arm64 build/package integration on the exact CI head named above. It does **not** yet prove:

- successful execution on the physical Honor Magic 8 Pro target;
- successful finalized preview of a real MotionCam/Honor DNG on-device;
- on-device RSS, thermal behavior, latency or frame time;
- arbitrary Android document-provider behavior;
- independently calibrated camera/lens spectral color;
- `FULL_PHYSICAL` color truth for source-metadata color;
- correct dual-illuminant interpolation;
- Gatehouse/vendor-decoded RAW paths beyond this direct `TileNativeDngSource` route;
- Lightroom-readable DNG compatibility projection;
- canonical promotion of this Android candidate;
- multi-frame/multi-capture evidence or HDR fusion science.

The next empirical boundary is physical-device execution with real MotionCam/Honor material. The next color-science boundary is correct multi-illuminant DNG color interpolation. The two must remain separate.
