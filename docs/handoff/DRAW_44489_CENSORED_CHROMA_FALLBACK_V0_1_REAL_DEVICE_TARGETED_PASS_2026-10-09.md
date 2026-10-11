# D.RAW 44489 — Censored Chroma Fallback v0.1 — Real-Device Targeted PASS — 2026-10-09

Status: **TARGETED REAL-DEVICE PASS FOR THE KNOWN TELE PURPLE-HIGHLIGHT FAILURE SCENE**

Project: D.RAW / TruthRaw  
Continuation code: `44489`  
PR: #131 (`feat/draw-workspace-free-raster-v01`)  
PR #131 remains **open, draft, unmerged**.  
Frozen PR #130 remains unchanged at `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`.

Permanent evidence law remains unchanged:

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

This PASS is an **Appearance/output acceptance for one known physical failure scene**. It is not a scientific reconstruction promotion, not a claim that clipped colour has been recovered, and not proof for every possible clipped/highlight scene.

## 1. Candidate under test

CI/build candidate input commit:

`da500581c8828b2e82375be12a5266ae8b08db87`

Validated generated runtime commit:

`3d33af73436925853b3e59d40bec82733c9234fd`

Candidate definition / CI handoff:

`docs/handoff/DRAW_44489_CENSORED_CHROMA_FALLBACK_V0_1_CI_CANDIDATE_2026-10-09.md`

The candidate is APPEARANCE_ONLY. It contracts chroma only as a function of reconstruction-support CENSOR fraction, preserves Rec.709 luminance, leaves PURE isolated, and keeps accepted Warm Illuminant retention downstream.

## 2. Real-device evidence bundle

User-provided bundle:

`DRAWPROfullsetfixed.zip`

- bytes: `184,184,686`
- SHA-256: `3a7274d468d63ff2f36edb2fa976fc03aebbbe9f4322539e752d3ad900bc35ca`

The bundle contains 8 UI screenshots plus a source-matched PRO JPEG and Full Colour Scientific Master DNG for capture id:

`1791580113812`

### PRO full-resolution JPEG

`DRAW_CAPTURE_1791580113812_tele_4080x3072_draw_pro_fullres.jpg`

- bytes: `21,791,106`
- SHA-256: `cddef010bdf700f9ef53158106e5ec5dee45894bd07420970c32498056359272`
- geometry: `4080 x 3072`
- route shown in UI: `PRO`
- output is visually the same lamp / tele failure scene used to reproduce the broad purple clipped-highlight defect.

### Full Colour Scientific Master DNG

`DRAW_CAPTURE_1791580113812_tele_4080x3072_draw_full_colour_scientific_master_float32_v0_1.dng`

- bytes: `173,244,458`
- SHA-256: `20d18d8caab2bff02ccd92d5c1ff84a6116eabce33041b0457dc874bd5a20e7f`
- primary: `3072 x 4080 x 3`, Float32, uncompressed tiled LinearRaw
- `UniqueCameraModel=TruthRaw Full Colour Scientific Master`
- private role: `TRUTHRAW_FULL_COLOUR_SCIENTIFIC_MASTER_FLOAT32_CAMERA_NATIVE_LINEAR_DNG_V0_1`
- `representation_only=1`
- `scientific_master_modified=0`
- `appearance_applied=0`
- `primary_raster_equals_scientific_master=1`
- `appearance_baked_into_primary=0`
- `scientific_writeback_allowed=0`
- `creates_new_evidence=0`
- sealed source SHA-256: `06319dbaa46f15f206627c9de070830447bf3f51c2a2297f48e789cb36df5500`
- Scientific Master SHA-256: `57603e1cb621464a76aab1f36d7601ad53a1b2d475e703ab549edfda40d6dc57`
- `censored_support_pixels=115432`
- `censored_channels=346296`
- `unknown_channels=37254984`

`346296 == 3 * 115432`, consistent with the conservative full-colour output authority assigning all three output channels CENSORED for these support-censored pixels.

## 3. Direct comparison against the previous purple failure

Historical exact bad-output reference used during causal isolation:

`DRAW_CAPTURE_1791566217859_tele_4080x3072_draw_advanced_fullres.jpg`

The new capture is a new physical frame, not the same exposure, so this is a **same-scene failure-class comparison**, not a bit-exact frame A/B.

At the same previously used 192 x 192 audit position:

`x=1920, y=1344, width=192, height=192`

historical bad-output median encoded RGB was approximately:

`(156,128,238)`

The new PRO output median at the same position is approximately:

`(227,179,114)`

Using the conservative display-purple predicate:

`B-R > 40 && B-G > 70 && B > 180`

results in:

- historical bad frame, 192x192 audit region: **62.37% purple-class pixels**
- new PRO frame, same audit region: **0.00%**

For the historical dominant purple-component bounding region:

`x=1961..2311, y=1312..1663`

- historical bad frame: **67.22% purple-class pixels**
- new PRO frame: **0.00%**

For a broader lamp/highlight region:

`x=1800..2449, y=1150..1799`

- historical bad frame: about **20.96% purple-class pixels**
- new PRO frame: **0.00%**

The broad solid violet/magenta interior visible in the prior failure is absent in the new full-resolution PRO JPEG.

## 4. Visual acceptance observations

The new output preserves the key desired properties:

- the broad purple clipped-highlight body is gone;
- the lamp remains visibly warm rather than being globally neutralized;
- the wall illumination remains warm and spatially smooth;
- the shade texture / mesh structure remains visible;
- the clipped/top highlight retains spatial structure rather than becoming one uniform flat gray replacement;
- no obvious purple ring, hard chroma seam or new halo is visible around the corrected region at full-resolution inspection;
- the surrounding room colour is not globally desaturated;
- the correction is localized to the relevant CENSORED-support presentation class.

This matches the intended design: **do not invent clipped scene colour; reduce unsupported chroma in Appearance while keeping luminance/structure and downstream source-bound Warm Illuminant behavior.**

## 5. Runtime diagnostic consistency

The new full-resolution JPEG embeds the existing non-mutating gamut/highlight diagnostic:

`D.RAW_GAMUT_DIAG_V01`

Observed values include:

- `pixels=12533760`
- `early_negative_any=0`
- `early_g_negative_rb_positive=0`
- `early_g_boundary_input_rb_positive=136458`
- `early_g_boundary_input_rb_high=0`
- `early_g_boundary_after_fit=0`
- `highlight_censored_calls=230864`
- `highlight_censored_green_min_in=31824`
- `highlight_white_candidates=135696`
- `highlight_severe_applied=0`
- `highlight_neutral_applied=17388`
- `sealed_streaming_module_modified=0`
- `pixel_mutation_by_diagnostics=0`
- `scientific_writeback=0`

Notably:

`230864 == 2 * 115432`

where `115432` is the Full Colour Scientific Master `censored_support_pixels` count for this exact capture. This is consistent with the downstream support-censor-bound output path being exercised while the scientific authority manifest itself remains unchanged.

The diagnostic remains observation-only and is not itself a pixel source.

## 6. High-Fidelity JPEG regression remains intact

The new PRO JPEG independently verifies as:

- SOI valid;
- terminal EOI exactly at file end;
- baseline SOF0, 8-bit;
- geometry `4080 x 3072`;
- component sampling `1x1 / 1x1 / 1x1` = true 4:4:4;
- DQT table 0: all 64 values are `1`;
- DQT table 1: all 64 values are `1`;
- quality reported as 100;
- FFprobe pixel format: `yuvj444p`;
- canonical SOS payload: `03 01 00 02 11 03 11 00 3F 00`;
- entropy start offset: `1278`;
- valid `FF 00` byte-stuffings: `16861`;
- unexpected unstuffed entropy markers: `0`;
- terminal EOI offset: `21791104` = exactly file length minus 2.

Therefore the Appearance fix did not regress the previously accepted Q100 / true-4:4:4 JPEG contract.

## 7. N2 / scientific-side observations remain isolated

The screenshots confirm the N2 Full-colour SAFE A/B/Delta tooling remains audit-only:

- `candidate-applied=false`;
- source / Scientific Master / D.RAWnegative remain unchanged;
- protected core remains preserved;
- the Censor/highlight crop reports zero A/B change in the audit view;
- the exported Full Colour Scientific Master contract independently reports `scientific_master_modified=0` and `appearance_applied=0`.

This is the desired separation: the visible correction belongs to downstream PRO Appearance, not to a silent scientific reconstruction rewrite.

## 8. Acceptance classification

**PASS — CENSORED_CHROMA_FALLBACK_V0_1_TARGETED_REAL_DEVICE_TELE_PURPLE_HIGHLIGHT**

What this proves:

- the known broad purple tele highlight failure reproduced in this lamp scene is materially corrected on-device;
- warm source-light appearance is retained;
- luminance/structure are not visibly collapsed into a flat replacement;
- the correction remains downstream Appearance;
- Scientific Master / sealed evidence remain immutable;
- Q100 / true-4:4:4 full-resolution JPEG output remains valid.

What this does **not** prove:

- universal correctness for all cameras, lenses, illuminants, clipping geometries or partial-channel saturation cases;
- recovered physical chromaticity inside a censored region;
- scientific promotion of the fallback;
- permission to alter CENSORED -> MEASURED/RECONSTRUCTED authority;
- permission to merge PR #131 solely on the basis of this one scene.

## 9. Next safe step

Keep the candidate architecture as-is for this failure class. Do not reintroduce hue-specific purple detection or global desaturation.

The next image-quality validation should deliberately broaden physical coverage while keeping the same implementation frozen:

1. neutral/white clipped source;
2. warm tungsten-like clipped source;
3. cool LED clipped source;
4. saturated red/green/blue non-clipped objects near highlight brightness;
5. partially clipped one-/two-channel cases;
6. hard highlight boundary / specular edge;
7. skin/wood/paint near but outside CENSOR authority;
8. PURE control for isolation.

Any failure should first be classified by authority/support geometry before changing the contraction curve. No Scientific Master, sealed CFA evidence, N2 candidate state or writeback rule should be broadened by this acceptance.

## 10. Current project state

**GREEN:** targeted real-device purple-highlight defect fixed in the tested tele/lamp scene.  
**GREEN:** Scientific Master immutability / Appearance separation intact.  
**GREEN:** Q100 true-4:4:4 output contract intact.  
**YELLOW:** broader cross-scene / cross-illuminant physical validation still pending.  
**UNCHANGED:** PR #131 stays open, draft and unmerged; PR #130 stays frozen.
