# D.RAW 44489 — Censored Chroma Fallback v0.1 — CI Candidate — 2026-10-09

Status: **CI/BUILD PASS — PHYSICAL REAL-DEVICE ACCEPTANCE PENDING**

Project: D.RAW / TruthRaw  
Continuation code: `44489`  
PR: #131 (`feat/draw-workspace-free-raster-v01`)  
PR #131 remains **open, draft, unmerged**.  
Frozen PR #130 remains unchanged at `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`.

Permanent evidence law remains:

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

No source/RAW/CFA evidence, Scientific Master value, scientific promotion state or scientific writeback authority was changed.

## 1. Why this candidate exists

The source-bound causal trace on exact capture `1791566217859` falsified the earlier negative-green-clamp hypothesis for the broad purple highlight:

- exact same-capture ADVANCED JPEG and PURE Float32 XYZ-D50 projection were compared;
- after the exact current XYZ-D50 -> linear-sRGB matrix, the full 4080x3072 frame contained **zero negative G components**;
- in the 192x192 problematic highlight crop, median linear RGB was approximately `(1.1236, 0.8279, 3.8560)` with zero negative G;
- the same crop in the actual ADVANCED JPEG was approximately `(156,128,238)`;
- a dominant `B > 3.0` component of about `83,471` pixels mapped to a broad purple body, while its immediate exterior ring was warm;
- Scientific-Master/output authority reported `120,956` censored-support pixels and `362,868` censored output channels.

The strongest current interpretation is therefore:

`source clipping/censoring -> finite but chromatically unreliable reconstructed/full-colour value -> CENSORED authority retained -> ADVANCED tone/presentation exposes that unreliable ratio as purple`.

This is an Appearance problem under explicit CENSORED authority, not proof that the clipped chromaticity is physically known.

Detailed causal record:

`docs/handoff/DRAW_44489_PURPLE_HIGHLIGHT_CAUSAL_TRACE_2026-10-09.md`

Documentation-only causal-trace commit:

`764763fb1a04d1ccbd070eb0b3ee4f78899f8606`

## 2. Candidate implementation

Input candidate commit:

`da500581c8828b2e82375be12a5266ae8b08db87`

Commit message:

`feat: add authority-bound censored chroma fallback candidate`

Validated generated runtime commit from the successful workflow:

`3d33af73436925853b3e59d40bec82733c9234fd`

Commit message:

`android: update downstream presentation output guards`

The generated runtime commit changes only downstream `photo_export_bridge.cpp` wiring for this stage; source/scientific evidence is untouched.

New candidate header:

`suite_android/app/src/main/cpp/presentation_censored_chroma_fallback_v0_1.h`

New deterministic patcher:

`tools/apply_presentation_censored_chroma_fallback_v0_1.py`

New regression:

`tools/test_presentation_censored_chroma_fallback_v0_1.py`

## 3. Candidate semantics

Classification:

**APPEARANCE_ONLY / DERIVED_PRESENTATION_OUTPUT / CI-VALIDATED CANDIDATE**

The stage does not detect “purple”, hue, camera model, vendor, object class or semantic content.

It derives a continuous `highlightCensorFraction` from the already-existing reconstruction-support CENSOR integral:

`support-censored samples in reconstruction support / support area`

No additional RAW read is introduced.

The chroma contraction is controlled only by this authority fraction:

- `fraction <= 0.15`: exact no-op;
- smooth transition for increasing support censoring;
- full candidate strength by `fraction >= 0.85`;
- maximum contraction = `0.84`, retaining approximately 16% of the incoming chroma vector rather than forcing exact grayscale.

For a valid finite RGB triplet with luminance `Y`, each channel is transformed as:

`channel_out = Y + (channel_in - Y) * chroma_scale`

This preserves Rec.709 luminance within the deterministic numerical tolerance tested by CI.

The neutral-axis fallback is **not** claimed to be recovered scene colour. It is only a conservative presentation response to chromaticity whose support is explicitly CENSORED.

## 4. Ordering and isolation

The validated downstream ADVANCED/PRO ordering is:

`historical reconstruction/output -> HDR/detail/restoration under historical centre-only authority -> colour fullness -> Natural Light Local Field Tone -> CENSORED chroma fallback v0.1 -> accepted Warm Illuminant Retention v0.1 -> historical support-aware highlight observer/pass-through -> final luminance-preserving gamut fit -> sRGB output`

Important isolation rules remain enforced:

- PURE extended-linear/headroom branch bypasses this candidate;
- restoration remains on historical centre-only `censored`;
- HDR remains on historical centre-only `!censored`;
- Natural Light Local Field Tone remains on historical centre-only `censored`;
- detail/acutance/pre-acutance do not consume the new fraction;
- the support-aware fraction is consumed only in the final ADVANCED/PRO colour-presentation loop;
- Warm Illuminant remains downstream so its already accepted source-white, bounded warmth can remain active after the conservative CENSORED fallback;
- the old near-neutral/severe highlight stage remains observation-only/pass-through and does not modify pixels.

## 5. Regression contract

The new regression proves:

- zero CENSOR fraction is bit-identical;
- the start threshold is bit-identical;
- luminance is preserved across multiple censor fractions;
- chroma decreases monotonically as CENSOR fraction increases;
- full candidate strength retains a bounded nonzero chroma residual;
- behavior is authority-driven rather than hue-specific (a fully CENSORED yellow synthetic sample receives the same contraction law);
- non-finite authority input fails safely;
- candidate wiring is after Natural Light Local Field and before Warm Illuminant;
- candidate wiring is absent from pre-acutance/detail stages;
- existing centre-only HDR/Local-Field authority consumers remain unchanged.

## 6. CI/build evidence

Workflow:

`D.RAW Free Raster v0.3 Finish APK`

Run:

`37989925633`

Job:

`114021172549`

Input head:

`da500581c8828b2e82375be12a5266ae8b08db87`

Result:

**SUCCESS**

The same run proved successful:

- deterministic Free Raster v0.3 patch;
- presentation gamut fit v0.1;
- existing highlight pass-through regression;
- Warm Illuminant Retention v0.1 regression;
- Natural Light Local Field Tone v0.1 regression;
- new Censored Chroma Fallback v0.1 regression;
- PURE Float32 headroom regression;
- explicit presentation headroom wiring regression;
- sealed Full-Frame Streaming integrity;
- strict High-Fidelity JPEG codec regression;
- Android/Kotlin/unit/NDK/C++ build;
- APK structural verification;
- generated runtime wiring commit;
- artifact upload.

No successful test/build step is a scientific promotion or physical image-quality acceptance.

## 7. Exact artifact/APK provenance

Workflow artifact:

- artifact ID: `11644926839`
- artifact name: `draw-free-raster-v03-fullres-candidate-apk`
- ZIP bytes: `3,345,278`
- ZIP digest: `sha256:f5031a06aa01dc520f2277a703e6a92d3465043d153312170df08a1b4e384a59`
- expiry: `2027-01-07T20:52:58Z`

Extracted APK independently re-hashed after download:

- bytes: **`8,869,803`**
- SHA-256: **`0d7015caf8481a0c031d25b05707204d60490ba72359cc1cf70c491f30176b10`**

## 8. Required physical acceptance round

This candidate must not be called PASS until it is tested on the actual device/problem source.

Minimum test:

1. install/update with the exact APK SHA-256 above;
2. rerun the same problematic tele RAW/scene that previously produced the large purple lamp/highlight;
3. use ADVANCED and/or PRO with the same settings that exposed the defect;
4. inspect the UI preview and saved 4080x3072 full-resolution output;
5. confirm whether the broad purple interior is removed/materially corrected;
6. verify the highlight does not become an implausibly flat gray patch;
7. verify its already-proven warm source-light impression remains credible where supported;
8. inspect the boundary for halo, seam, ring or abrupt chroma transition;
9. inspect non-highlight saturated colours elsewhere for unintended desaturation;
10. verify detail/acutance/HDR/Natural-Light behavior did not change unexpectedly;
11. retain Q100/true-4:4:4 output compatibility;
12. optionally repeat PURE: it must remain isolated from this ADVANCED/PRO-only stage.

If physical appearance fails, keep the candidate unaccepted and iterate only in downstream Appearance. Do not broaden restoration/HDR authority and do not rewrite Scientific Master or sealed evidence.

## 9. Current status

**GREEN — causal diagnosis is source-bound and CI/build candidate is internally consistent.**  
**YELLOW — visual/physical acceptance is still pending.**

PR #130 stays frozen. PR #131 stays open, draft and unmerged. No scientific candidate was promoted. No scientific writeback was enabled.
