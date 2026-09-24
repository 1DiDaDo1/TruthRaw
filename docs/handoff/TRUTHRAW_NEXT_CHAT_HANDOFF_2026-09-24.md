# TruthRaw next-chat handoff — 2026-09-24

## CURRENT MAIN INTEGRATION

Active branch:

`integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master`

The previous 2026-09-21 code freeze is now **historical**. On 2026-09-24 the user explicitly authorized the validated recommendations to be promoted into the main project.

Historical freeze documents and the old frozen commit remain provenance and must not be rewritten as though they never existed.

Production promotion code-bearing commit:

`ad3a1f465fc77f76972c64b3a806838fbeddc310`

The promotion was applied directly on top of the then-current main integration head `905803ff36bce8d2d5ec98167410771d0d741510`, preserving the eight post-freeze main-history commits.

The temporary promotion PR #33 was closed unmerged after the same final file set was installed directly as one Git tree/commit on main.

## F64 SCIENTIFIC-MASTER RECONSTRUCTION IS NOW MAIN CODE

The F64 work is no longer only a research-branch experiment.

Main module:

`docs/research/scientific-master-f64-reconstruction-v0.1/native/`

Policy:

- admitted Stage-2 storage remains Float32;
- branch-sensitive directional gradients, curvature, 0.72 branch choice, inverse-gradient weighting, support bounds and colour-difference interpolation use Float64;
- the physically measured CFA component is re-injected directly from the original Float32 Stage-2 sample;
- measured CFA values must remain bit-exact;
- reconstructed channels cross one explicit Float64 -> Float32 storage boundary;
- higher numerical precision does not create stronger evidence or calibration authority.

The canonical v4.7i `core.h` and `core.cpp` remain byte-frozen. The new F64 backend is a separate versioned module above that reference.

All eight identified Android Scientific-Master consumers are routed to F64 on main:

1. advanced preview;
2. source-bound colour preview;
3. photo export;
4. full-resolution restoration export;
5. restoration projection;
6. linear DNG export;
7. PURE Float32 DNG export;
8. TruthNegative export.

Validation content-equivalent to the promoted main code passed in workflow run `35938923291`:

- GCC precision gates: PASS;
- Clang precision gates: PASS;
- Clang ASan/UBSan: PASS;
- measured CFA bit-exact: PASS;
- 35,624 reconstructed components tested;
- 11,040 reconstructed stored components differ between F32 and F64 calculation;
- maximum absolute final Float32 storage delta: `3.0398368835449219e-06`;
- Android arm64 assembleDebug: PASS.

This proves the F64 change is not cosmetic: near the existing 0.72 directional boundary it can select a different reconstruction branch while leaving measured source samples unchanged.

## MAIN-BRANCH VALIDATION — GREEN

The promoted main code has now passed both the existing integration workflow and the new dedicated F64/color workflows.

Code-bearing production validation:

- workflow: `TruthRaw v0.84.2 Adaptive Compute Router`;
- run: `35939298642`;
- head: `ad3a1f465fc77f76972c64b3a806838fbeddc310`;
- status: **SUCCESS**;
- APK bytes: `6,101,301`;
- APK SHA-256: `ab7efc32561996305cc4884cd7594a1a06259be380f5a0dc5322a42ee9615070`;
- artifact id: `10784082914`;
- artifact: `truthraw-v0-84-2-compute-router-debug-arm64`;
- artifact digest: `sha256:2855311de5bc25ceadf5bf45d2d5f4c6f349680e2f30fd20db08a7660aa59f02`.

Dedicated main F64 + color validation:

- workflow run: `35939324830`;
- status: **SUCCESS**;
- F64 host precision: PASS;
- DNG color v0.2 GCC: PASS;
- DNG color v0.2 Clang: PASS;
- color authority/precision contract: PASS;
- Android arm64 assembleDebug: PASS;
- APK bytes: `6,101,301`;
- APK SHA-256: `d9766f247effe3b64e8734fe34fe87821745ba841d473a5396d6fb689319bb9d`;
- artifact id: `10783889536`;
- artifact digest: `sha256:1c639997838b13131d974c442ffe2eb170548b835d205e001cb58993aafbe6a7`.

Dedicated main F64 compiler/sanitizer run `35939327728` is also fully green on GCC, Clang and Clang ASan/UBSan.

## OPEN SCENE FIELD v0.85 / TRUTHNEGATIVE TN-4 — MAIN CODE

The Open Scene / "new house" work is now implemented in the main integration.

Main additions:

- `TruthRawOpenSceneField/0.85`: local per-pixel/per-RGB-channel creation role, authority, uncertainty class, optional p95, support, censor bound/domain and contribution provenance;
- compact canonical 64x64 tile encoding with uniform / 2-bit / 4-bit / dense fallback;
- `TruthNegativeLocalAuthorityProjection/0.4`: exact 4x footprint-local authority/provenance projection without creating target measurements;
- `TruthRawOpenSceneLocalPolicy/0.86`: one local policy for Restoration, Scientific HDR gating and future detail support;
- TN-4 embeds the v0.85 source-grid field physically per cell;
- the 16320x12288 Full Colour TruthNegative DNG now carries a procedural per-target-channel local field bound to the exact `PROJECTED_RASTER_SHA256`, avoiding a multi-gigabyte metadata duplication.

Current conservative Camera-5 interpretation remains:

- directly measured uncensored CFA channel -> calibrated-estimate authority at the source coordinate;
- clipped measured CFA channel -> censored with source-RAW-code bound;
- reconstructed source RGB channels -> numeric value may exist while scientific authority remains UNKNOWN until admitted uncertainty exists;
- dense 4x target channels -> `DENSE_PROJECTION + UNKNOWN + UNRESOLVED` unless a future projection uncertainty model is independently admitted.

The local field does not create new evidence, does not promote uncertainty through resampling and does not allow scientific writeback.

Final production validation:

- workflow run: `35943061088`;
- head: `dbe66ee9f43d92b5412c12d1c4e17a6db9e1d1d5`;
- status: **SUCCESS**;
- GCC local-field gates: PASS;
- Clang local-field gates: PASS;
- main-project authority invariants: PASS;
- Android arm64 assembleDebug: PASS;
- APK bytes: `6,151,421`;
- APK SHA-256: `a9e5bc9417b80e53ad4d1e08823fac0ecef3e355b64f5de3684db5595a7129fc`;
- artifact id: `10785173626`;
- artifact: `truthraw-open-scene-v085-tn4-main-debug-arm64`;
- artifact digest: `sha256:f87921dd69470bd7e133f30f6e85a7d6a317641a8ffda83137c4e38102c94c17`.

## FORMAL COLOUR / CALIBRATION AUDIT IS NOW MAIN PROJECT STATE

Read:

- `docs/research/formal-color-calibration-audit-v0.1/README.md`
- `docs/research/formal-color-calibration-audit-v0.1/STATE_v0_1.json`

Current authority remains:

`SOURCE_METADATA_BOUND`

For the supported one/two-calibration DNG domain, the current audit passes ColorMatrix direction, CameraCalibration direction/signature gating, inverse-CCT interpolation, iterative neutral-to-xy solving, fail-closed non-convergence, Bradford D50 fallback and dual ForwardMatrix processing when both endpoints exist.

Explicit boundaries remain:

- one ForwardMatrix reused over a dual-calibration range remains source-bound and REVIEW_REQUIRED for strict scientific use;
- a third DNG calibration set remains fail-closed unsupported;
- CalibrationIlluminant 255 / IlluminantData remains fail-closed unsupported;
- the producer performs its matrix solve in Float64 but stores `cameraToXyzD50` as Float32;
- the old runtime camera-RGB -> XYZ multiply remains a future precision target;
- Camera-5 colour metadata remains unresolved by physical calibration;
- independent target/spectral/lens/unit calibration has not been established;
- FULL_PHYSICAL colour authority is therefore not granted.

## MAIN-PROJECT POLICY FROM THIS POINT

Validated improvements may now land directly on the main integration branch. A separate branch is no longer required merely because a change improves the Scientific Master.

This does **not** repeal scientific immutability:

- sealed source evidence stays immutable;
- measured / reconstructed / appearance remain separate;
- byte-frozen reference modules stay frozen unless a new explicitly versioned successor replaces them;
- unsupported cases remain fail-closed;
- precision improvements never increase evidence authority.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
