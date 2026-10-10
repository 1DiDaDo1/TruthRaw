# D.RAW 44489 — Deep-Censor Chroma Guard v0.1 — real-device sky regression + CI-green candidate — 2026-10-10

## Purpose

This handoff records a new real-device tele/PRO sky regression discovered after `Residual Near-Censor Chroma Shoulder v0.1`, explains why that first shoulder cannot solve the dominant failure zone, and binds the follow-up `Deep-Censor Chroma Guard v0.1` candidate.

The accepted `Censored Chroma Fallback v0.1` remains accepted standard ADVANCED/PRO Appearance behavior. This document does not demote it, alter Scientific Master, create new evidence or promote scientific colour recovery.

## 1. Real-device test input

User-supplied bundle:

`DRAWdirecttelepurpleskytest.zip`

- bytes: `19,180,969`;
- SHA-256: `68a68a2daf634ef0aff54716483206eca9061ec97619d8eacd5bd36bca14b2fa`.

Contained full-resolution PRO JPEG:

`DRAW_CAPTURE_1791645320252_tele_4080x3072_draw_pro_fullres.jpg`

- bytes: `10,550,559`;
- SHA-256: `23dd8d719915f84e6bc5e258b3cb756a3e6459e022061bb67fd7f1c5a9f79a9d`;
- geometry: `4080 x 3072`;
- route shown by the app: `PRO`;
- orientation shown by the app: `0°` source-metadata orientation.

The image shows a clearly visible pink/lila cast in the strongly backlit/censored cloud region. Luminance structure and cloud-edge detail remain visible, so a detail-destructive blur or tone flattening is neither necessary nor allowed.

## 2. Decisive authority evidence from the same capture

The app's `N2 · 1:1 Full-colour SAFE A/B/Δ` diagnostic selected:

`Censor/highlight-zone · bron x=3136, y=1088 · 192x192`

Reported for this exact zone:

- sampled: `36,864`;
- candidate: `1,530`;
- preserved: `35,334`;
- structure: `8,034`;
- censored: `26,591`;
- boundary: `709`;
- changed: `2 / 36,864` pixels;
- candidate-applied: `false`.

The measured reconstruction-support CENSOR fraction is therefore:

`26591 / 36864 = 0.7213270399` ≈ **72.13%**.

This is the key discriminator: the dominant purple sky region is not merely a low-fraction near-censor shoulder case. It is a **deeply censored support region**.

## 3. Why Residual Near-Censor Chroma Shoulder v0.1 was insufficient

`Residual Near-Censor Chroma Shoulder v0.1` was deliberately bounded to:

- start at CENSOR fraction `0.02`;
- reach max shoulder strength at `0.15`;
- begin fade at `0.35`;
- be fully zero again by `0.55`;
- max chroma contraction `0.12`.

At the observed sky authority `f = 0.721327`, that shoulder is therefore exactly inactive by design.

The accepted `Censored Chroma Fallback v0.1` remains active. With its frozen constants:

- start `0.15`;
- full `0.85`;
- max contraction `0.84`;

its contraction at `f = 0.721327` is approximately:

`0.765286`,

leaving approximately:

`1 - 0.765286 = 0.234714` = **23.47% residual chroma**.

The new real-device sky proves that this residual can still be visibly pink/lila when the pre-fallback censored chromatic ratio is extreme.

Classification of the first shoulder round:

**NOT SUFFICIENT AS COMPLETE RESIDUAL-PURPLE SOLUTION FOR DEEP-CENSOR SKY / LOW-FRACTION SHOULDER ROLE NOT INVALIDATED**.

Do not rewrite this as a scientific failure: this is downstream Appearance behavior around already-CENSORED chromaticity.

## 4. New candidate — Deep-Censor Chroma Guard v0.1

Runtime file:

`suite_android/app/src/main/cpp/presentation_deep_censor_chroma_guard_v0_1.h`

This is a separate downstream Appearance layer. It does not change the accepted fallback constants.

Non-PURE ordering:

`Natural Light Local Field -> Residual Near-Censor Shoulder -> accepted Censored Chroma Fallback -> Deep-Censor Chroma Guard -> Warm Illuminant Retention -> historical highlight observer -> gamut fit`

Constants:

- `kCensorFractionStart = 0.50`;
- `kCensorFractionFull = 0.80`;
- `kMaxRemainingChromaContraction = 0.78`.

Hard behavior:

- fraction `<= 0.50` -> exact no-op;
- smooth authority-only rise from `0.50` to `0.80`;
- at/above `0.80`, contract at most 78% of the **remaining** chroma after the accepted fallback;
- Rec.709 luminance preserved exactly within numerical tolerance;
- no spatial blur or resampling;
- no geometry/acutance/detail mutation;
- no hue/purple detector;
- no brightness trigger;
- no object semantics;
- no camera/vendor identity;
- PURE bypasses the stage;
- Warm Illuminant remains downstream;
- no source mutation;
- no Scientific-Master writeback;
- no new evidence;
- no claim of recovered scene colour.

At the observed real-device sky fraction `0.721327`, the complete shoulder + accepted fallback + deep-guard chain retains approximately **8.28%** of the original chroma rather than ~23.47%.

At fully deep-censored support the chain retains approximately **3.52%** original chroma. This is deliberately not mathematically exact gray; downstream bounded Warm Illuminant may still restore its already accepted source-white appearance cue.

## 5. Regression requirements

The dedicated regression proves:

- deep guard exact no-op through `0.50`;
- max remaining-chroma contraction ~78% at/above `0.80`;
- exact Rec.709 luminance preservation within numerical tolerance;
- hue-independent authority response;
- observed `26591/36864` real-device authority reduces total residual chroma below 9% and above 7%;
- combined shoulder + fallback + deep-guard chroma scale is monotonically non-increasing for CENSOR fraction 0..1;
- at fully censored support total residual chroma is approximately 3.52%;
- accepted fallback constants unchanged;
- near-censor shoulder constants unchanged;
- no leakage into restoration/detail/acutance preparation;
- downstream ADVANCED/PRO Appearance isolation preserved.

## 6. Exact implementation / CI lineage

Candidate code commit:

`81e7f255aede165dbee1e687e6a02aa0fbf2fff0`

Dedicated CI workflow commit:

`81638fdf37925e8e428a7b16548207c2aa1cb239`

Exact CI-generated runtime-wiring commit:

`05a41eb4a4362fce134c72c90294f969cf39655b`

Workflow:

`D.RAW Deep-Censor Chroma Guard v0.1`

- run: `38064184948`;
- job: `114248374631`;
- conclusion: **SUCCESS**.

Green steps include:

- deep-censor applicator;
- downstream ordering verification;
- deep-censor regression;
- existing near-censor shoulder regression;
- accepted Censored Chroma Fallback regression;
- Warm Illuminant regression;
- Natural Light Local Field regression;
- presentation gamut regression;
- sealed Full-Frame Streaming integrity;
- strict High-Fidelity JPEG codec regression;
- Android SDK/NDK/Gradle setup;
- unit tests + APK build;
- APK verification;
- runtime wiring commit;
- artifact upload.

## 7. Candidate APK

Artifact:

- id: `11674890747`;
- name: `draw-deep-censor-chroma-guard-v01-apk`;
- archive digest SHA-256: `2b6776c7ab8930d3b2f9a50ab071d6ab4a8b01a149c4b05829a1f5cb03539b35`.

Extracted APK:

- bytes: `8,887,019`;
- SHA-256: `ed1659999c4a292a399515bde01bce98b88dad6e3795bb1ea9be9dc414e911cf`.

Classification:

**CI-GREEN APPEARANCE CANDIDATE / REAL-DEVICE SKY VALIDATION PENDING**.

## 8. Required physical validation

Preferred test: repeat a direct tele/PRO backlit cloud/sky scene comparable to capture `1791645320252` using this candidate APK.

PASS requires:

- strongly pink/lila deep-censored cloud regions materially reduced;
- cloud luminance structure and edge detail retained;
- no gray seam/halo around censor boundaries;
- non-censored blue sky and ordinary cloud color not globally washed out;
- tree/building silhouettes unchanged in geometry/detail;
- Warm Illuminant behavior unchanged where applicable;
- accepted broad-purple fallback behavior not regressed;
- Q100/true-4:4:4 output remains valid.

FAIL/revise if:

- deep-censored pink/lila remains clearly dominant;
- luminance detail/acutance is reduced;
- neutral halos/seams appear;
- ordinary saturated non-censored colour is washed out;
- Warm Illuminant or accepted fallback behavior regresses.

## 9. Scientific/project boundary

This test strengthens one already-known law:

**CENSORED finite RGB is not automatically trustworthy chromaticity.**

It does not establish the physical pre-saturation linearity boundary, sensor spectral truth, scene colour, or a new Scientific-Master value. `Radiometric Reliability Envelope v0.1` remains a fail-closed scientific contract; this deep-censor stage is only an Appearance fallback for chromaticity already known to lack full authority.

PR #130 remains frozen. PR #131 remains open, draft and unmerged. No scientific promotion occurred.
