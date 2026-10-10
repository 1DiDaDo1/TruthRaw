# D.RAW 44489 — Deep-Censor Chroma Guard v0.2 Detail Candidate — 2026-10-10

Status: **CI-GREEN APPEARANCE CANDIDATE / REAL-DEVICE DETAIL VALIDATION PENDING**.

This record continues the accepted Censored Chroma Fallback v0.1, Residual Near-Censor Chroma Shoulder v0.1 and Deep-Censor Chroma Guard v0.1 work. It does not alter Scientific Master, sealed CFA evidence, scientific promotion state or source authority.

## User/device evidence that motivated v0.2

Latest user bundle:

`DRAWPROJPEGtelepurpletest.zip`

- exact bytes: `26,779,992`;
- SHA-256: `f01957a30d60dadcc1774e53daf0b30fd9c10bb6769469f06e2044f85ec19ded`.

Contained PRO JPEG:

`DRAW_CAPTURE_1791647288671_tele_4080x3072_draw_pro_fullres.jpg`

- exact bytes: `17,952,362`;
- SHA-256: `59111a7b474147f42ee344d900744448ac8c12f0b964398ad03eea82ddbdac66`;
- geometry: `4080 x 3072`.

User judgement after Deep-Censor Chroma Guard v0.1: **the result is already very good; increasing detail preservation would make it perfect**.

The important interpretation is not that v0.1 destroyed luminance detail: v0.1 is a common-scalar contraction around Rec.709 luminance, so luminance is algebraically preserved. The remaining perceptual flattening is most consistent with loss of subtle local chroma / colour microcontrast in deeply censored regions.

A generic sharpening pass is therefore rejected. A simple luminance-edge exemption is also rejected because the residual purple failure can coincide with real luminance edges; allowing more chroma merely because an edge exists could reopen the purple regression.

## v0.2 design

New runtime file:

`suite_android/app/src/main/cpp/presentation_deep_censor_chroma_guard_v0_2.h`

The CENSOR-authority response remains identical to v0.1:

- `kCensorFractionStart = 0.50`;
- `kCensorFractionFull = 0.80`;
- `kMaxRemainingChromaContraction = 0.78`.

v0.2 adds only a bounded low-amplitude chroma-detail relief:

- relative chroma is measured as Euclidean chroma around Rec.709 luminance divided by `max(abs(Y), 0.05)`;
- relief begins fully only below relative chroma `0.02`;
- relief smoothly disappears by relative chroma `0.12`;
- maximum relief is `0.35` of the **extra deep-guard contraction only**;
- upstream accepted Censored Chroma Fallback remains unchanged;
- clearly chromatic residuals at or above relative chroma `0.12` receive the exact v0.1 deep-guard response;
- the stage never expands chroma versus its input;
- Rec.709 luminance remains unchanged by construction;
- chroma-vector direction is preserved by a common scalar;
- no sharpening, blur, resampling, Sobel/Laplacian edge logic, hue detector, semantic/object detector, camera/vendor identity or brightness threshold is introduced.

The strong-purple regression sample `(0.337, 0.216, 0.855)` is explicitly required to produce the same deep-guard output as v0.1. A low-amplitude sample `(0.52, 0.50, 0.48)` must retain more chroma than v0.1 while remaining below its input chroma.

Runtime order remains:

`Natural Light Local Field -> Near-Censor Shoulder -> accepted Censored Chroma Fallback -> Deep-Censor Chroma Guard v0.2 -> Warm Illuminant Retention -> historical highlight observer -> gamut fit`.

PURE bypass remains intact.

## Exact implementation lineage

- v0.2 header commit: `0ba2ecc50df5c2c072aa6028d4b7e83e527c964d`;
- v0.2 applicator commit: `06b8c137c24aa8383d3f076ec6b9093009d3eb5c`;
- initial regression commit: `951082efa855eb1933bc63b8d359b9ac4071842d`;
- dedicated v0.2 workflow commit: `494d14236fb2e77d7c5d07e720054ac08e55971c`;
- main APK workflow binding commit: `5dbed408ee977a0783c83694b74ad1cf2a07bc21`;
- corrected regression-harness commit: `51e4bf5ae2a875e790b02d30568d76b027397116`;
- exact CI-generated runtime wiring commit: `bd6ae77eba0314c9d87130931e0771a91ce749fb`.

The first main-workflow attempt on `5dbed408...` stopped at the new regression because its static forbidden-token check incorrectly matched the word `semantic` inside a comment explaining that semantic inference is forbidden. All prior Appearance regressions in that attempt were green. The build did not reach APK assembly. This is **HARNESS_FAILURE / NOT_RUNTIME_FAILURE** and did not produce an accepted candidate APK.

The corrected harness checks executable-like mechanism tokens rather than explanatory prose.

## Exact successful main-project CI

Workflow: `D.RAW Free Raster v0.3 Finish APK`

- run: `38067210344`;
- job: `114257180765`;
- trigger head: `51e4bf5ae2a875e790b02d30568d76b027397116`;
- conclusion: **SUCCESS**.

Green steps include:

- v0.2 applicator;
- v0.2 deep-censor detail-preservation regression;
- presentation gamut fit;
- historical highlight observer;
- Warm Illuminant Retention;
- Natural Light Local Field;
- accepted Censored Chroma Fallback;
- Residual Near-Censor Shoulder;
- PURE headroom and explicit headroom wiring;
- sealed Full-Frame Streaming integrity;
- strict High-Fidelity Q100/true-4:4:4 JPEG codec regression;
- Android SDK/NDK/Gradle setup;
- unit tests and full Android debug assembly;
- APK verification;
- runtime wiring commit;
- artifact upload.

GitHub Actions artifact:

- artifact ID: `11675980316`;
- name: `draw-free-raster-v03-fullres-candidate-apk`;
- archive digest SHA-256: `ec0aa13ef39e53a9ee01f7934de11ed5ded4915af24551243b485eb1c2b8b208`.

Exact extracted APK:

- bytes: `8,888,139`;
- SHA-256: `a77459fa4e8af1faace3165a1c29ae8b61aea987de47c21ab0d081b9e41192ff`.

## Physical acceptance gate

v0.2 is **not yet physically accepted**. A comparable PRO tele/backlit/sky round must show:

1. purple/magenta suppression at least as good as Deep-Censor Guard v0.1;
2. visibly improved subtle colour texture / microcontrast and perceived detail in clouds, foliage or similarly structured bright regions;
3. no luminance-detail loss, halo, seam, oversharpening or ringing;
4. no reopening of strong purple on real edges;
5. no visible washout of legitimate saturated colour;
6. accepted Warm Illuminant behavior preserved;
7. accepted Censored Chroma Fallback behavior preserved;
8. Q100/true-4:4:4 output remains valid.

Until that device round, classification remains:

**CI-GREEN APPEARANCE CANDIDATE / REAL-DEVICE DETAIL VALIDATION PENDING**.

Scientific boundaries remain unchanged: `creates_new_evidence=false`, no Scientific-Master writeback, no source mutation, no calibration/reconstruction promotion and no new MEASURED authority.
