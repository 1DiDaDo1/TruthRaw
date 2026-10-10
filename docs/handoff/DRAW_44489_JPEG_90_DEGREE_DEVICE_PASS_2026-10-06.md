# D.RAW 44489 — Full-resolution JPEG +90° device acceptance — 2026-10-06

Continuation code: **44489**

This record adds real-device evidence for the non-zero downstream orientation path of the preview-independent full-resolution JPEG output. It does not promote scientific authority and does not alter sealed RAW/CFA evidence or the Scientific Master.

## Repository boundary

- repository: `1DiDaDo1/TruthRaw`
- active PR: **#131**
- branch: `feat/draw-workspace-free-raster-v01`
- tested runtime source remains the preview-independent JPEG implementation built from `de0d49ef9f9abda777a8126a2a98a028030eafa1`
- frozen scientific/audit reference remains PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`

## Uploaded physical-device evidence

Archive supplied from the real device:

`DRAW90degree.zip`

Archive properties from independent inspection:

- bytes: `3,671,051`
- SHA-256: `26fd2eec9e5856694fa58447f973512f32943266823e0eb04be41a0cc3569696`

Contents:

- `Screenshot_20261006_205252_com_truthraw_adaptiveui_MainActivity.jpg`
- `Screenshot_20261006_205426_com_truthraw_adaptiveui_MainActivity.jpg`
- `IMG_260830_143010_214_008_draw_pro_fullres (1).jpg`

## Runtime orientation evidence

The device UI visibly reports:

`Oriëntatie-override: +90° met de klok mee · alleen presentatie/projectie; sealed RAW en Scientific Master blijven ongewijzigd.`

The displayed Advanced/PRO preview is visibly rotated clockwise relative to the earlier 0° presentation.

One screenshot also records a cancelled JPEG file-dialog round-trip (`JPG-export geannuleerd.`). That cancellation is not counted as the successful export. The independently supplied JPEG file below is the acceptance artifact.

## Exported JPEG inspection

Independent decode of:

`IMG_260830_143010_214_008_draw_pro_fullres (1).jpg`

shows:

- format: JPEG
- dimensions: **3072 × 4080**
- mode: RGB
- bytes: **1,994,803**
- SHA-256: `9ac32855452494ec8b755d187f6c5f99006284a842123cc8fa80426643154b11`
- no EXIF-orientation transform is required to obtain the portrait geometry; the output pixels themselves are stored in the rotated geometry.

The corresponding previously accepted 0° JPEG was:

- dimensions: `4080 × 3072`
- bytes: `1,991,838`
- SHA-256: `115354213c3fd828bd2708414a423dba33e39c644f8aa6ea8a251b1c042f1bbc`

The total pixel count is unchanged:

`4080 × 3072 = 3072 × 4080 = 12,533,760 pixels`

so this device test shows rotation with dimension swap rather than a lower-resolution output.

## Direct image-to-image rotation check

The accepted 0° JPEG was decoded, rotated exactly **90° clockwise**, and compared pixel-for-pixel with the new +90° JPEG decode.

Results:

- clockwise comparison MAE: `0.6985928936` code values/channel
- clockwise RMSE: `1.0416217438`
- clockwise maximum absolute difference: `8` code values
- clockwise PSNR: `47.7766 dB`
- clockwise decoded-sample correlation: `0.9997739056`
- exact decoded channel samples equal: about `46.16%`

For the opposite 90° direction, MAE is about `62.82`, which decisively rejects counter-clockwise rotation.

Interpretation: the +90° output is overwhelmingly consistent with the same rendered presentation raster being physically rotated **clockwise** and JPEG-encoded again. The small residual differences are consistent with lossy JPEG re-encoding; there is no evidence here of a crop, rescale, opposite-direction rotation or preview-size substitution.

## Acceptance result

**PASS — NON_ZERO_ORIENTATION_FULL_RESOLUTION_JPEG_CLOCKWISE_90**

This establishes on the tested DNG / PRO path:

- downstream +90° orientation control is honored by full-resolution JPEG output;
- output geometry swaps from `4080×3072` to `3072×4080`;
- pixel correspondence is consistent with a clockwise 90° rotation;
- the operation remains downstream presentation/projective behavior;
- the test does not create new `MEASURED` samples or scientific authority;
- sealed RAW and Scientific Master are not claimed to be rotated or rewritten.

This device test does **not** independently prove byte-exact source immutability because the original source DNG was not included for a before/after hash comparison.

## Remaining physical acceptance

The next highest-value fail-closed tests are:

1. **route mismatch during Android document-picker round trip** — start a JPEG export on one PURE/ADVANCED/PRO route, change the route before completion if the UI/lifecycle allows it, and require export to block rather than silently retarget;
2. **source switch/reprocess stale-state clearing** — create a pending output request and then change/reprocess the source; stale output binding must clear/fail closed;
3. independent byte-exact source before/after comparison only if stronger immutability evidence is required;
4. optional cold-start/process-death Free Raster restoration;
5. optional installed APK hash readback.

Keep PR #131 draft until the required physical acceptance boundary is complete.

## Permanent scientific boundary

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

**Stable outside. Flexible inside. Evidence law unchanged.**

Rotation changes downstream presentation/projective coordinates only. It does not rotate sensor evidence in place, fabricate optical information, modify sealed CFA samples, or increase scientific authority.
