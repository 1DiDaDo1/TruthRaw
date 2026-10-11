# D.RAW 44489 — Highlight Default + Appearance References — 2026-10-10

Status: **CENSORED CHROMA FALLBACK v0.1 ACCEPTED AS STANDARD DOWNSTREAM APPEARANCE BEHAVIOR**  
Scope: ADVANCED/PRO only; PURE remains isolated.  
Scientific status: **NO SCIENTIFIC PROMOTION / NO SOURCE OR SCIENTIFIC-MASTER WRITEBACK**

Permanent evidence law remains:

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

## 1. User decision — highlight work closed for now

The real-device purple/magenta highlight failure is considered solved for the current project line. The user explicitly does not want further highlight-specific testing unless a later regression reappears.

The accepted default behavior is the already implemented authority-bound CENSORED chroma fallback v0.1 in the shared ADVANCED/PRO downstream presentation path.

This is not a scientific reconstruction promotion. It is a conservative Appearance response to chromaticity whose reconstruction support is explicitly CENSORED.

## 2. Exact accepted implementation identity

Candidate input commit:

`da500581c8828b2e82375be12a5266ae8b08db87`

Validated generated runtime commit:

`3d33af73436925853b3e59d40bec82733c9234fd`

Workflow:

`D.RAW Free Raster v0.3 Finish APK`

Run:

`37989925633`

Job:

`114021172549`

Result:

**SUCCESS**

Exact candidate APK physically tested on device:

- bytes: `8,869,803`
- SHA-256: `0d7015caf8481a0c031d25b05707204d60490ba72359cc1cf70c491f30176b10`

The implementation is already wired as default behavior in the ADVANCED/PRO non-PURE branch. No new runtime flag or second renderer is required to make it standard.

## 3. Physical acceptance evidence retained

Targeted real-device bundle:

`DRAWPROfullsetfixed.zip`

- ZIP bytes: `184,184,686`
- ZIP SHA-256: `3a7274d468d63ff2f36edb2fa976fc03aebbbe9f4322539e752d3ad900bc35ca`

Exact PRO output:

`DRAW_CAPTURE_1791580113812_tele_4080x3072_draw_pro_fullres.jpg`

- bytes: `21,791,106`
- SHA-256: `cddef010bdf700f9ef53158106e5ec5dee45894bd07420970c32498056359272`

Matching full-colour Scientific Master projection:

`DRAW_CAPTURE_1791580113812_tele_4080x3072_draw_full_colour_scientific_master_float32_v0_1.dng`

- bytes: `173,244,458`
- SHA-256: `20d18d8caab2bff02ccd92d5c1ff84a6116eabce33041b0457dc874bd5a20e7f`

Physical/visual result:

- broad purple lamp/highlight body removed;
- same historical 192x192 audit position moved from approximately encoded `(156,128,238)` to approximately `(227,179,114)`;
- strong-purple classification in that audit region fell from about `62.4%` to `0%`;
- in the old dominant-problem bbox it fell from about `67.2%` to `0%`;
- warm source-light impression remained visible;
- lamp structure remained visible;
- no obvious new hard chroma seam/halo was observed;
- JPEG Q100 / true 4:4:4 path remained intact;
- Scientific Master remained unchanged by the Appearance correction.

Classification:

**PASS — CENSORED_CHROMA_FALLBACK_V0_1_TARGETED_REAL_DEVICE_TELE_PURPLE_HIGHLIGHT**

The accepted runtime is now standard project behavior for this downstream failure class. Do not keep re-testing the same highlight issue merely to reconfirm it. Reopen only on a demonstrated regression or a materially different failure class.

## 4. Additional white-highlight supporting bundle

User supplied:

`DRAWJPGpurehighlightwhite.zip`

- ZIP bytes: `16,079,782`
- ZIP SHA-256: `b5c8614e7da8746894ec601d76488f9f4fac17f0a71cec93ead9edccf7891d78`

Contained PURE full-resolution JPEG:

`DRAW_CAPTURE_1791585026524_tele_4080x3072_draw_pure_fullres.jpg`

- bytes: `15,241,013`
- SHA-256: `5b2ac182c7a5504438bd2081f9ef6059d3531fce4049519f3f328722e846e46a`

Associated screenshot:

`Screenshot_20261010_003346_com_truthraw_adaptiveui_MainActivity.jpg`

- SHA-256: `c65e7e95d67889d01900a33ea23b3de1561ef59c56fd70359d0571b41b134325`

This is retained only as additional appearance/output provenance. It is not required as another highlight acceptance gate because the user has closed highlight-specific testing for now.

## 5. Lightroom lamp appearance reference

The user supplied an eye-matched Lightroom example of how the lamp should look in reality.

File:

`IMG_20261009_235112.jpg`

- geometry: `3072 x 4080`
- bytes: `8,891,229`
- SHA-256: `ccb3b8d743e2ee554ab7eaa79e94db1fdda5c4d83077f0354325590fe5a9c53e`
- EXIF Make: `HONOR`
- EXIF Model: `BKQ-N49`
- EXIF Software: `Adobe Lightroom 11.6.01 (Android)`
- EXIF DateTime: `2026:10:10 00:21:06`

Interpretation:

- this image is a **human-adjusted Appearance reference**;
- it may guide qualitative decisions about warmth, local tone, lamp-shade transparency, emitted-light impression and wall-light appearance;
- it is **not** a RAW measurement, calibration chart, illuminant SPD measurement, colorimetric truth source, DNG profile, scientific ground truth or promotion evidence;
- no project calibration may be fit solely to this image;
- no camera/vendor/Lightroom transform may become scientific authority because the reference looks correct.

Permanent classification:

**APPEARANCE_REFERENCE_ONLY / USER_EYE_MATCHED / NON_AUTHORITY**

## 6. New TV yellow-subtitle appearance discrepancy

User states that the TV subtitles in the supplied scene are **yellow in reality**.

Bundle:

`DRAWpuregeelondertitelingtv.zip`

- ZIP bytes: `199,485,240`
- ZIP SHA-256: `6dd57c5965434af830f07e85827d7e0bab24e98531e5a1948caee47608fa6f6c`

Same-capture PURE files:

`DRAW_CAPTURE_1791583374795_tele_4080x3072_draw_pure_float32_v0_63.dng`

- bytes: `178,536,988`
- SHA-256: `7913ff23f3549f93792c776d72bde9d3716e7eb806c1cef5edfa56c73a4ee83b`

`DRAW_CAPTURE_1791583374795_tele_4080x3072_draw_pure_fullres.jpg`

- bytes: `25,984,512`
- SHA-256: `08f772fe7c24bc71bf0054d457aaee7cc442bbd3bd266aca7e927b01daaea40b`

Associated screenshot:

`Screenshot_20261010_000623_com_truthraw_adaptiveui_MainActivity.jpg`

- bytes: `1,117,883`
- SHA-256: `d78cbfa1c6216f77586f5afac50a0f780b46d7cc68d5d5f133b1dc054c6ec118`

### 6.1 What is visibly wrong

The current PURE full-resolution output renders the bright subtitle text approximately white / yellow-white rather than the user-verified yellow scene appearance.

This is a **separate colour-rendering problem** from the now-accepted purple-highlight fix. Do not modify or weaken Censored Chroma Fallback v0.1 to solve it.

### 6.2 Same-capture scientific projection facts

The DNG private contract identifies the primary as:

`TRUTHRAW_PURE_FLOAT32_XYZ_D50_LINEAR_DNG_PROJECTION`

with:

- `representation_only=1`
- `scientific_master_modified=0`
- `appearance_applied=0`
- `sealed_source_sha256=ea26d47e41a2fb0163452f28561a387c0f22f8a9ce872770fc2e6fbee242d0bf`
- `scientific_master_sha256=d666643207f536f53e7f06b7bb732bdb559d695bd58277a9feb8a7f2bda40baf`
- `output_channel_authority_bound=1`
- `censored_channels=250239`
- `unknown_channels=37351041`
- `censored_support_pixels=83413`
- `scientific_writeback_allowed=0`
- `creates_new_evidence=0`

The per-pixel authority class of the subtitle letters is not independently resolved from this bundle, so do **not** infer that their scene chromaticity is scientifically known.

### 6.3 Source-bound diagnostic observation

Using the exact current XYZ-D50 -> linear-sRGB matrix from `unified_output_preview_v0_1.cpp`, bright rendered subtitle pixels in the same capture map before final display clipping to a strongly B-dominant current RGB representation. For the sampled bright text pixels, the median was approximately:

`linear sRGB = (0.885, 0.915, 3.581)`

The portrait/landscape coordinate mapping was independently verified against the same output JPEG: the correct +90-degree mapping reproduced the JPEG with about `1.49` mean absolute 8-bit channel levels on random samples, versus about `36.87` for the opposite rotation.

Therefore the yellow-to-near-white discrepancy is not explained solely by the final `clamp(linear,0,1)` display step; the current chromatic relationship at those coordinates is already inconsistent with a simple yellow appearance before final display clipping.

This does **not** yet prove a camera-color-matrix defect, reconstruction defect or calibration defect because:

- the scene is an emissive TV display;
- high-frequency RGB subpixel structure, CFA sampling, clipping/censoring, display temporal modulation/PWM, exposure interval and uncertainty may all matter;
- exact per-pixel authority for the subtitle text has not yet been established;
- user-observed yellow is an appearance/scene-reference statement, not an instrumented colorimetric measurement.

Correct next interpretation:

**NEW APPEARANCE/COLOR INVESTIGATION TARGET — EMISSIVE YELLOW TV SUBTITLES**

not:

**HIGHLIGHT REGRESSION**.

## 7. Project rule from these experiences

Two useful project lessons are now explicit:

1. **CENSORED finite RGB is not automatically trustworthy chromaticity.** The purple-highlight case proved that downstream presentation may need authority-bound conservative handling without inventing recovered colour.
2. **A visually wrong colour need not be a highlight problem.** The yellow-TV-subtitle example must be traced separately through source/CFA support, authority, camera-native colour, XYZ/linear RGB, display clipping and emissive-display temporal/sampling effects before any new correction is designed.

Do not build broad hue-specific hacks. Do not calibrate from Lightroom. Do not globally desaturate. Preserve all accepted highlight, Warm Illuminant, Natural Light, detail/HDR and Scientific-Master boundaries while investigating the new yellow-TV case.

## 8. Current state after this handoff

- Censored Chroma Fallback v0.1: **ACCEPTED DEFAULT / STANDARD ADVANCED-PRO APPEARANCE BEHAVIOR**.
- Highlight-specific testing: **CLOSED FOR NOW BY USER DECISION**.
- PURE: remains isolated from the fallback.
- Warm Illuminant Retention v0.1: accepted real-device behavior remains in force.
- Natural Light Local Field Tone v0.1: separate candidate status unchanged.
- Lightroom lamp image: **appearance reference only**.
- Yellow TV subtitle discrepancy: **new separate appearance/color investigation target**.
- Scientific promotion: unchanged and closed.
- PR #130: frozen.
- PR #131: remains open, draft and unmerged.
