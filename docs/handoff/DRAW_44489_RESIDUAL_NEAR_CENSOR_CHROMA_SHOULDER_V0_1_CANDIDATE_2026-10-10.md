# D.RAW 44489 — Residual Near-Censor Chroma Shoulder v0.1 — CI candidate — 2026-10-10

## Purpose

This handoff records the first bounded follow-up to the already accepted `Censored Chroma Fallback v0.1` after a new PRO real-device output showed that a smaller residual lila/magenta near-censor fringe remains in some bright/backlit edge regions.

This is **not** a rollback of the accepted fallback and does not reopen the original broad-purple failure as unsolved. The accepted fallback still removes the former large purple highlight component. This candidate addresses a narrower residual authority gap below/around the accepted fallback's `0.15` reconstruction-support CENSOR-fraction start.

Classification:

**CI-GREEN APPEARANCE CANDIDATE / REAL-DEVICE VALIDATION PENDING**

No scientific promotion, evidence creation or Scientific-Master writeback is authorized.

---

## New real-device regression evidence

User supplied:

`DRAW_CAPTURE_1791642512412_tele_4080x3072_draw_pro_fullres.zip`

- ZIP bytes: `17,179,195`;
- ZIP SHA-256: `47b7d6e3ec88f8cd2b402e0bb46aa1f5d875f878625abdd016f16a1cbb3b79ea`.

Contained PRO JPEG:

`DRAW_CAPTURE_1791642512412_tele_4080x3072_draw_pro_fullres.jpg`

- bytes: `17,329,222`;
- SHA-256: `f45102ab7fdf8bb23922283680cf9a1edff3c21f6d11f4100b2c68fffa40ae5a`;
- geometry: `4080 x 3072`.

Observed visually and by bounded image inspection:

- the former broad purple highlight failure is not back;
- residual lila/magenta remains in several very bright/backlit exterior edge/highlight regions, especially the upper window strip and some high-contrast exterior boundaries;
- the residual is spatially narrower and weaker than the historical failure;
- fine structure remains visible, so the follow-up must preserve luminance/detail rather than use a stronger indiscriminate highlight desaturation.

The inspection heuristic used to localize candidate regions is not scientific authority and is not part of runtime selection. Runtime selection remains solely reconstruction-support CENSOR authority.

---

## Candidate design

Name:

**Residual Near-Censor Chroma Shoulder v0.1**

Runtime file:

`suite_android/app/src/main/cpp/presentation_near_censor_chroma_shoulder_v0_1.h`

Position in downstream non-PURE Appearance:

`Natural Light Local Field -> Residual Near-Censor Chroma Shoulder -> accepted Censored Chroma Fallback -> Warm Illuminant Retention -> historical highlight observer -> gamut fit`

PURE bypasses the complete branch.

### Authority and constants

- `kCensorFractionStart = 0.02`;
- `kCensorFractionFullShoulder = 0.15`;
- `kCensorFractionFadeStart = 0.35`;
- `kCensorFractionFadeEnd = 0.55`;
- `kMaxChromaContraction = 0.12`.

Rules:

- `0.00 .. 0.02` reconstruction-support CENSOR fraction is exact no-op;
- the shoulder rises smoothly to at most 12% chroma contraction by fraction `0.15`;
- it remains bounded while the accepted fallback starts taking over;
- it fades away by fraction `0.55`, where the stronger accepted fallback already carries the authority;
- combined shoulder + accepted fallback chroma response is regression-tested to be monotonic with increasing CENSOR authority;
- Rec.709 luminance is preserved;
- no spatial blur, resampling, geometry change or acutance/detail-stage mutation;
- no hue-specific trigger, purple detector, semantic/object detector, camera/vendor identity or brightness threshold;
- no source mutation or Scientific-Master writeback;
- no claim that contracted chroma is recovered scene truth.

This is intentionally more conservative than changing the accepted fallback's constants. The accepted fallback remains frozen and independently regression-checked.

---

## Exact implementation lineage

Candidate/header/applicator/regression commit:

`91b0660b7336d5ae5fc7f62bde95c790f46393b0`

Workflow integration commit:

`2f532545be9ff81e23151b582937279d70c603aa`

CI-generated runtime wiring commit:

`564398857965ab4560c5bb3a6341a2de167fd2e8`

The runtime commit changes only `photo_export_bridge.cpp` wiring for this stage: include, namespace and one call between Natural Light Local Field and the accepted Censored Chroma Fallback.

PR #130 remains frozen. PR #131 remains open, draft and unmerged.

---

## CI validation

Workflow:

`D.RAW Free Raster v0.3 Finish APK`

Run:

`38061533998`

Job:

`114240619677`

Conclusion:

**SUCCESS**

Successful relevant steps include:

- all existing Appearance applicators;
- new `Apply residual near-censor chroma shoulder v0.1`;
- full-resolution cable verification;
- gamut-fit regression;
- historical highlight-observer regression;
- Warm Illuminant regression;
- Natural Light Local Field regression;
- accepted Censored Chroma Fallback regression;
- new Residual Near-Censor Chroma Shoulder regression;
- PURE headroom regression;
- sealed Full-Frame Streaming integrity;
- strict Q100/true-4:4:4 JPEG codec regression;
- Android SDK/NDK/Gradle setup;
- `:app:testDebugUnitTest :app:assembleDebug`;
- APK verification;
- runtime wiring commit;
- artifact upload.

The new regression explicitly verifies:

- exact no-op at and below the 0.02 start;
- approximately 12% maximum shoulder contraction at 0.15;
- Rec.709 luminance preservation;
- fade-out of the shoulder where the accepted fallback has sufficient authority;
- hue-independent authority behavior;
- monotonic combined chroma contraction from shoulder + accepted fallback as CENSOR fraction rises;
- no leakage into pre-acutance/restoration/detail stages;
- PURE branch isolation;
- accepted fallback constants remain unchanged.

---

## Candidate APK

GitHub Actions artifact:

- artifact id `11673332546`;
- name `draw-free-raster-v03-fullres-candidate-apk`;
- archive digest SHA-256 `65cacca9a30f2d0c6685b472a4d135fbc708e92fd3a364b6eff444c5b94b2998`.

Extracted APK:

- member `app-debug.apk`;
- bytes `8,886,667`;
- SHA-256 `835b672de55a65236f950ec3b4801146b6d5d2299cd4611ca78f719c0d37f8b8`.

The workflow trigger head is `2f532545...`; the exact corresponding committed runtime wiring is `5643988579...`. The APK was built in the same CI workspace after the applicator inserted that runtime wiring and before the bot committed the identical wiring to the branch.

---

## Required real-device validation

This candidate is not physically accepted until a new real-device PRO output demonstrates the intended improvement.

Primary validation target:

- reproduce the same type of backlit/window scene where `1791642512412` showed residual lila;
- verify residual lila/magenta is materially reduced;
- verify fine luminance structure, edge detail and acutance remain intact;
- verify no new neutral/gray halo appears around bright edges;
- verify legitimate saturated colours are not visibly washed out;
- verify accepted Warm Illuminant appearance remains intact;
- verify the former broad-purple failure does not regress;
- Q100/true-4:4:4 output remains structurally valid.

A matching PURE frame is useful as an isolation reference but is not required to promote this downstream Appearance candidate if the PRO regression target is clearly resolved and no collateral colour/detail failure appears.

Fail the candidate if the residual purple remains materially unchanged, if legitimate colour is visibly desaturated, if detail/acutance is reduced, if halos/seams appear, or if accepted Warm Illuminant/Censored Chroma behavior regresses.

---

## Permanent boundary

This candidate must never be reinterpreted as a measured pre-saturation sensor calibration or chromatic recovery algorithm. It is a bounded downstream Appearance response to already-known reconstruction-support CENSOR authority.

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`
