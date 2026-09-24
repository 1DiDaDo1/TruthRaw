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
