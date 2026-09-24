# TruthRaw next-chat handoff — 2026-09-24

## CURRENT MAIN INTEGRATION

Active branch:

`integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master`

The previous 2026-09-21 code freeze is now **historical**. On 2026-09-24 the user explicitly authorized the validated recommendations to be promoted into the main project.

Historical freeze documents and the old frozen commit remain provenance and must not be rewritten as though they never existed.

Current code-bearing checkpoint for the architecture described in this handoff:

`675265b084577aa0c005905363ab9f6ba39dfc7f`

This checkpoint includes the F64/Open-Scene/TN-4 work, synchronized product UI, per-output Unified Output Preview/orientation handling, and the JPG-L migration from an internal TN-3 scientific layer to TN-4.

Historical F64 + formal-colour production-promotion commit:

`ad3a1f465fc77f76972c64b3a806838fbeddc310`

That promotion was applied directly on top of the then-current main integration head `905803ff36bce8d2d5ec98167410771d0d741510`, preserving the eight post-freeze main-history commits.

The temporary promotion PR #33 was closed unmerged after the same final file set was installed directly as one Git tree/commit on main.

## CURRENT RAW/DNG INGRESS + OUTPUT PREVIEW — MAIN CODE

Read the dedicated current-state document:

`docs/CURRENT_RAW_DNG_INGRESS_AND_OUTPUT_PREVIEW_2026-09-24.md`

### RAW/DNG ingress

The product UI now reflects the actual backend rather than implying universal proprietary RAW decoding.

Current support:

- **DNG:** full scientific route when the strict DNG profile is admitted;
- **Nikon NEF:** strict uncompressed-16 CFA **measurement-only** subset; no Scientific Master until calibration/authority requirements are admitted;
- **CR3/CR2, ARW/SRF/SR2, RAF, RW2, ORF, PEF, RWL, 3FR/FFF, IIQ and other proprietary RAW:** immutable source handle only, decoder adapter pending, fail-closed before Scientific Master;
- **generic .raw:** no universal scientific decoder.

For an admitted DNG the active route is:

`independent source seal -> source-bound colour -> MultiVendorRawSourceAdapter -> TileNativeDngSource -> Stage-2 -> F64 branch-sensitive reconstruction -> Float32 canonical Scientific Master -> Dynamic Authority/Open Scene -> Open Scene Field v0.85 -> local policy v0.86 -> outputs`.

A camera-origin processing DNG is independently resealed/admitted. It may retain upstream Camera2 acquisition provenance, but it does not inherit scientific authority from that upstream capture.

The strict DNG source currently requires classic TIFF/DNG, uncompressed 16-bit unsigned one-sample CFA storage, an admitted 2x2 RGB Bayer pattern, valid BlackLevel/WhiteLevel and exactly one strip/tile storage model. Unsupported compression, BigTIFF, packed 10/12/14-bit TIFF sample storage and unsupported topology remain fail-closed.

### Unified Output Preview v0.1

UOP1 is now the current output-preview contract.

Rule:

**same stored/selected output primary route, smaller display projection.**

Current route bindings include:

- PURE Float32 DNG -> random-access F64 Scientific-Master primary;
- Full Colour Scientific Master DNG -> random-access F64 Scientific-Master primary;
- Advanced Render/Edit DNG -> final Render/Edit primary;
- TruthNegative 200MP DNG -> dense TruthNegative primary;
- TruthNegative TN-4 scientific negative -> TN-4 Scientific-Master primary;
- Full-res Restoration / restoration projections -> restoration derivative primary;
- compatibility Linear DNG -> exact bounded-U16 primary representation;
- saved JPEG -> the actually committed JPEG bytes.

UOP1 carries the stored output orientation in `displayQuarterTurns`. The UI now renders each result with its own stored-orientation contract instead of blindly reusing editor rotation.

The preview is presentation-only: no new HDR/detail/restoration/relight, no scientific authority, no writeback.

### UI synchronization

Visible stale wording has been removed:

- launcher: `DNG volledig · proprietary RAW adapter-afhankelijk`;
- launcher core line: active F64 reconstruction + Open Scene v0.85 + TN-4 + UOP v0.1;
- Advanced: `Canonical Open Scene v0.85 · gedeeld met TN-4/TRR/projecties`;
- Advanced Restoration names Open Scene Field v0.85 + local policy v0.86;
- PRO Precision: `F32 canonical storage · F64 compute actief`;
- PRO explicitly exposes Open Scene v0.85 / TN-4 v0.4 / local policy v0.86 / UOP v0.1 and the real RAW-ingress matrix;
- Scientific Negative button says TN-4;
- Settings no longer labels the current app as the old v0.84.2 architecture.

### JPG-L scientific layer

The layered JPG-L container previously embedded a TN-3 scientific payload. That was stale after TN-4 promotion and also caused a native-signature compile break once UOP arguments were added.

It is now migrated to:

- TN-4 packet contract;
- `magic=TRUTHNEGATIVE_V0_4_TN4`;
- Open Scene Field v0.85 markers;
- local authority projection v0.4 marker;
- TN-4 manifest role/hash naming;
- TN-4 science-chunk post-write verification.

JPG-L remains a JPEG-compatible layered photograph; the front JPEG is presentation/compatibility and the embedded TN-4 scientific layer remains the separate scientific payload.

### Final validation for this handoff checkpoint

General ARM64 main-integration validation:

- workflow: `TruthRaw v0.84.2 Adaptive Compute Router`;
- run: `35963545440`;
- code head: `675265b084577aa0c005905363ab9f6ba39dfc7f`;
- status: **SUCCESS**;
- Android arm64 assembleDebug: PASS;
- APK bytes: `6,217,309`;
- APK SHA-256: `009f04a471f3cdaafc40bd10117e031606c46936e40d64034c55aebbcfd4a4d2`;
- artifact id: `10793630629`;
- artifact: `truthraw-v0-84-2-compute-router-debug-arm64`;
- artifact digest: `sha256:9d01e11b63f41d9f97094aff9bf81029efb5e957d24f9551d3544a65452333aa`.

Dedicated UOP1/output-route validation:

- workflow: `Unified Output Preview v0.1`;
- run: `35963625751`;
- workflow head: `b1a0a52c802da20354397ff3a2a98b3a1ec7b948` (same code plus CI contract);
- status: **SUCCESS**;
- GCC UOP1 gates: PASS;
- Clang UOP1 gates: PASS;
- direct-primary route contract: PASS;
- Android main integration: PASS;
- APK bytes: `6,217,309`;
- APK SHA-256: `8fd3092785d2b84309a14ef4414ff74c40450582519ba68a7bb831b5824b2865`;
- artifact id: `10793695531`;
- artifact: `truthraw-unified-output-preview-v01-main-debug-arm64`;
- artifact digest: `sha256:b9bd4defa7b248240e9089234f4b59081bffb557070934900507c121308ae8c9`.

The APK hashes differ between the two workflows although the code payload is equivalent because they are separate APK build invocations; use the workflow/run/artifact identity when reproducing a specific package.

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

## TN-4 POST-WRITE VERIFIER FIX — GREEN

A real-device test exposed a verifier mismatch after TN-4/Open Scene v0.85 promotion.

The writer succeeded, but `PureFloat32DngExport.kt` still required the old authority markers:

- `schema=TruthNegativeOutputChannelAuthority/0.1`;
- `mapping_mode=RESAMPLED_UNIFORM_UNKNOWN_IMPLICIT`;
- `target_support_role=RECONSTRUCTED_DENSE_SUPPORT`.

This was a verifier bug, not a 200MP writer failure.

The main verifier now requires the TN-4 contract:

- `schema=TruthNegativeOutputChannelAuthority/0.2`;
- `mapping_mode=RESAMPLED_PROCEDURAL_LOCAL_FIELD_V04`;
- `open_scene_field_schema=TruthRawOpenSceneField/0.85`;
- `local_projection_schema=TruthNegativeLocalAuthorityProjection/0.4`;
- projected-raster and Open Scene SHA-256 values;
- local-field policy/artifact SHA-256 values;
- downstream local-authority projection artifact SHA-256;
- equality of the authority-binding and edit-binding local-field artifact identity.

Fail-closed behavior is retained; only the expected contract was migrated.

Final validation:

- workflow run: `35958961941`;
- head: `77f7d49660d5629e79af550d0e4a61d272b4caa2`;
- GCC local-field gates: PASS;
- Clang local-field gates: PASS;
- main authority invariants: PASS;
- Android arm64 build: PASS;
- APK bytes: `6,151,421`;
- APK SHA-256: `79eadbc2cfa16c64f7f896fd054e3608e4f637d020342acf58b5ecb55afb60f7`;
- artifact id: `10791476986`;
- artifact digest: `sha256:c7136c272454d9915287f0331661ad463dbbb942d2d0ffab614fbd8a72a4f391`.

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
