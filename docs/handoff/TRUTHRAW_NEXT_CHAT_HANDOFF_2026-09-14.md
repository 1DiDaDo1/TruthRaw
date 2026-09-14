# TruthRaw next-chat handoff — 2026-09-14

Status: **ACTIVE SESSION HANDOFF / IMPLEMENTATION + ZERO-LINE EMPIRICAL OVERLAY**

This file is the shortest path for the next chat to continue the current implementation without reconstructing the session from conversation history.

It does **not** replace the project-wide canonical state or the sealed scientific architecture. Read it together with `START_HERE_NEW_CHAT.md`, the 2026-09-13 project map/fact-check/house documents, the updated zero-line documents, and the module-local tests/manifests named below.

## 1. Repository / PR snapshot

Repository: `1DiDaDo1/TruthRaw`

Active implementation branch:

`research/ui-output-modes-certificate-v0.1-2026-09-14`

Draft PR:

`#23 — Integrate four-mode output policy + TRUTHRAW PURE float32 DNG`

PR base:

`research/project-factcheck-optimization-v0.1-2026-09-13`

The last branch head immediately before this handoff refresh was:

`614864b3f58d82aea676ca02ddd42062744223be`

That head includes documentation-only integration of the recovered nul-lijn history and the 2026-09-14 Honor ISO/response/dark-frame evidence. The next chat must re-fetch the live PR head and workflow state before making a fresh CI/head claim because this handoff update itself moves the branch head.

Earlier validated implementation snapshot:

`ef5175707d2b69973601408d6053fd234b2c6906`

At that implementation head the then-current PR-triggered workflow set was observed green. Later documentation and verifier commits moved the branch head without intentionally changing the frozen scientific reconstruction/export algorithms.

## 2. Non-negotiable scientific laws

Keep these exact project laws:

**Measured where measured. Reconstructed where necessary. Never invented.**

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

The original Direct-CFA/source RAW remains immutable sealed evidence. Normal scientific processing remains single-frame with:

- `physicalFrameCount = 1`
- `independentEvidenceCount = 1`

Measured, reconstructed, censored/unknown, counterfactual, appearance and projection state remain different authority classes.

The current project-level Scientific Master is reconstructed **camera-native RGB before the normal `camera_to_xyz()` route and before appearance**. It is not original sensor evidence and it is not a display image.

## 3. Nul-lijn / zero-line history that must not be lost

The intended term is the **zero-line / nul-lijn**.

For positive latent scene light:

`T = log2(L / L0)`

`L0` is a reference/gauge, not sensor black, DNG BlackLevel, absolute darkness, clipping or display middle grey.

The recovered house intent is explicitly two-sided:

`darkness <- ... <- -EV <- nul-lijn -> +EV -> ... -> brighter light`

The **new house was designed without a finite representational ceiling above the nul-lijn and without a finite representational floor below it**.

This is a mathematical/representational property. The source RAW still exposes only a finite noisy/quantized/censored evidence window. Do not turn the house metaphor into an infinite physical-sensor dynamic-range claim.

TruthRaw must keep both:

1. a signed scene-linear estimator for unbiased reconstruction/noise/residual calculations;
2. a positive-light TruthRange companion with support/bounds/uncertainty.

Negative post-black numerical values are not negative physical light. They must not be hard-clipped merely because `log2` applies only to positive physical-light quantities.

Read before any zero-line/noise/calibration work:

- `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
- `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_EVOLUTION_2026-09-13.md`
- `docs/research/zero-line-empirical-validation-v0.1/TRUTHRAW_ZERO_LINE_HISTORY_AND_EMPIRICAL_STATE_2026-09-14.md`
- `docs/research/zero-line-empirical-validation-v0.1/TRUTHRAW_ZERO_LINE_EMPIRICAL_STATE_2026-09-14.json`

## 4. House / history context that must not be lost

The current architecture is the convergence of four recovered design concepts:

- **gezegelde woning / sealed house** — preserve the original RAW as immutable evidence;
- **alle vrijheid / new house** — allow a richer reconstructed scene representation without claiming stronger evidence; the two-sided nul-lijn address space belongs here;
- **achterkant van de foto / Technical Backplane** — bind source/master/zero-line/scene-scale/provenance behind the visible projection;
- **tussenwoning / Gatehouse** — isolate external decode complexity and detach it after a sealed handoff before Main-House processing.

The Technical Backplane is a compact technical backside, not hidden image evidence and not a second Scientific Master. The Gatehouse is not a second truth source.

## 5. Four-mode product contract

The public launcher has four primary modes:

1. `JPG` — presentation output.
2. `JPG XL` — high-quality presentation slot; currently fail-closed until an actual JPEG XL encoder is independently validated.
3. `TRUTHRAW PURE` — scientific output; no creative appearance toggles.
4. `TRUTHRAW ADVANCED` — same scientific foundation, with downstream appearance/export controls.

The option chips are:

- `Colourful`
- `Detailed`
- `Soft`
- `HDR`

They are selectable only for `JPG`, `JPG XL`, and `TRUTHRAW ADVANCED`.

`TRUTHRAW PURE` remains appearance-neutral. Missing/unknown mode handoff fails safe to PURE. Unsupported appearance options sanitize to neutral.

Current profile selections are **appearance intent only**. They do not alter the source evidence, reconstruction authority, Scientific Master identity, TruthRange zero-line, evidence counts or Technical Backplane.

## 6. Language / product identity contract

English is the canonical fallback language.

Current Android resource localizations:

- English
- Dutch / Nederlands
- German / Deutsch
- French / Français

The product names `TRUTHRAW PURE`, `TRUTHRAW ADVANCED` and the option identifiers `Colourful`, `Detailed`, `Soft`, `HDR` remain stable across languages.

Android prototype identity remains:

`0.7-four-mode-certificate`

The branch is research/implementation state, not a production release.

## 7. TRUTHRAW PURE scientific DNG path

The high-fidelity PURE RAW/DNG path is the restored float32 Scientific Master Linear DNG writer, not the older 16-bit compatibility window.

Current scientific export sequence:

`sealed source -> source-bound color -> Scientific Master v0.2 -> Technical Backplane phase 2 -> canonical 64x64 Scientific Master replay -> exact master digest gate -> cameraToXyzD50 -> float32 XYZ-D50 LinearRaw DNG`

The float32 writer:

- stores 32-bit IEEE float samples;
- uses `PhotometricInterpretation = LinearRaw`;
- writes three samples per pixel;
- retains negative and `>1` values;
- does not apply tone mapping, appearance or gamut mapping;
- validates Scientific Master identity before artifact commit;
- preserves one physical frame / one independent evidence root.

The old 16-bit projections remain compatibility outputs. They may clip to a finite compatibility interval and therefore must never redefine TRUTHRAW PURE.

The preservation of negative and >1 output values is representation fidelity, not evidence promotion.

## 8. TruthRaw Certificate v0.1

The certificate belongs **inside the file / technical backside**, never as a visible watermark.

Certificate v0.1 binds source evidence, Scientific Master, zero-line/scene-scale/backplane identity, projection class, color binding and single-frame evidence counts.

Current security rule:

**No private TruthRaw brand-signing key may be embedded in the APK or repository.**

Until a trusted issuer key and verifier trust distribution are provisioned, file/UI state remains:

`UNSIGNED DEVELOPMENT`

and must not display a verified certificate badge.

Certificate v0.1 does not independently serialize the exporter-internal `appearanceApplied` and `counterfactualObservationCreated` booleans. Artifact-level validation must report that limitation rather than upgrading code-path guarantees into stronger file-byte claims.

## 9. Completed supplemental Honor PURE source-bound run

A supplemental real-device Honor smoke run is now closed end-to-end from exact source bytes to the saved PURE artifact. It is **not one of the formal five regression sources**.

Exact source:

- `IMG_260908_193753_205_015.dng`
- bytes: `25,899,870`
- dimensions: `4080x3072`
- SHA-256: `757f6aaa0b45a0e68270ee8073d338895d2df284d8ae57bca5683a5ca2ba2573`

The empirical pre/post probes both reported the same source hash and byte length.

Saved PURE artifact:

- `IMG_260908_193753_205_015_truthraw_pure_float32_v0_1.dng`
- bytes: `151,021,352`
- SHA-256: `a80390deb51e315e73973782f151555521eed819383ebe45a2906a9b321dc788`
- `4080x3072`
- float32 XYZ-D50 LinearRaw
- 37,601,280 real serialized components, all finite
- 288,739 negative components
- 218,566 components greater than 1
- observed real-component range approximately `-0.0091582136 .. 1.45196819`
- 147,456 tile-padding components checked as zero

Source SHA agrees across the actual source bytes, DNGPrivateData and `TRCERT01`. Scientific Master identity also agrees between private projection metadata and certificate:

`dc747bc3bc9d7ab24614560ead67008455742c6732faf6f1a3167d56670eae97`

Result:

**PASS — exact-source artifact/projection integrity + device/app cross-binding.**

This does not close the formal five-source gate, Adobe interoperability, independent physical color calibration or trusted signing.

## 10. Formal five-file physical regression set remains open

Manifest:

`docs/research/physical-dng-test-set-v0.1/TRUTHRAW_PHYSICAL_TESTSET_2026-09-14.json`

Formal sources:

- `IMG_260816_134122_304_005.dng`
- `IMG_260816_134204_911_008.dng`
- `IMG_260816_143716_085_041.dng`
- `IMG_260830_143012_297_014.dng`
- `IMG_260908_193845_662_027.dng`

These files remain **test evidence, not automatically calibration evidence**.

The supplemental `IMG_260908_193753_205_015.dng` PASS must not be counted as one of these five.

## 11. 2026-09-14 Honor tele ISO / response evidence

A separate source-bound research campaign was intentionally kept distinct from the PR #23 five-file artifact gate.

### Real-scene ISO ladder

ISO sequence:

`200 -> 400 -> 800 -> 1600 -> 3200 -> 6400 -> 12800`

Key findings:

- same-exposure ISO200 versus ISO400 signal was about half in B/G, consistent with one stop;
- high-ISO within-frame green NoiseProfile observed/predicted ratios were approximately `1.009 / 1.024 / 1.033 / 1.053` for ISO1600/3200/6400/12800;
- below-black samples reached 59,573 at ISO12800, about `0.4753%` of all sensels;
- minimum observed black-corrected source sample was `-22 DN`.

Interpretation: source-bound response/noise evidence and strong empirical support for signed post-black preservation; not independent physical calibration.

### Response/repeatability follow-up

Same-exposure ISO100 -> ISO200 aligned signal ratios:

- B `1.9087x`
- G1 `1.9679x`
- G2 `1.9907x`
- R `1.9994x`

Expected one-stop ratio: `2.000x`.

The duplicated ISO800 captures reproduced after alignment at approximately `1.000x` in G1/G2/R and `0.9905x` in B.

Interpretation: source-bound gain response and repeatability **PASS**; strict temporal calibration remained open because framing/focus were not sufficiently fixed.

## 12. 2026-09-14 covered dark-frame evidence

A covered/no-scene dark ladder provided the first direct same-condition temporal pairs.

First-nine ISO sequence:

`100, 100, 400, 400, 800, 1600, 3200, 6397, 12522`

Common exposure:

approximately `0.0327347964 s`.

Direct `(A-B)/sqrt(2)` temporal sigma:

- ISO100: about `0.789-0.790 DN` across CFA channels;
- ISO400: about `0.867-0.870 DN` across CFA channels.

Repeat-frame correlation was effectively zero at ISO100 and around `0.004-0.005` at ISO400 after extreme-outlier exclusion.

The dark data also contain genuine samples below metadata BlackLevel. This reinforces the canonical rule that the signed scientific estimator must not hard-clip post-black values to zero.

Under the ordinary DNG R/G/B interpretation, the embedded middle NoiseProfile pair underpredicted measured low-ISO green dark variation. Because exact MotionCam/device NoiseProfile channel/coordinate semantics have not been independently certified, this remains an **OPEN metadata-semantics/calibration issue**, not permission to rewrite source metadata.

## 13. Reproducible discrete exact-ISO8192-associated domain

The first unusual exact-ISO8192 dark file was:

- `IMG_260914_104952_817_033.dng`
- SHA-256 `06bf9afdf6a71d4c0b3995dec641f33a8d205cd022139440960131b348650ec3`
- about `54-55 DN` trimmed dark sigma
- about `7.1204%` raw-zero censoring

A later same-exposure ten-file dark series used ISO sequence:

`8192, 10244, 8184, 6500, 3200, 1600, 800, 400, 200, 100`

with common exposure approximately `0.01648959145 s`.

The second exact-ISO8192 capture:

- `IMG_260914_110550_363_001.dng`
- SHA-256 `2dae19d2e9d144c4d257808432e58a7246764e1259d6a5033de2e1eff76c644e`

reproduced:

- identical BlackLevel values;
- identical six-value DNG NoiseProfile;
- about `54-55 DN` trimmed dark sigma;
- about `7.13%` raw-zero censoring.

At the same exposure:

- ISO8184 remained ordinary: `0%` raw-zero censoring, about `2.6 DN` sigma;
- ISO10244 also remained ordinary: `0%` raw-zero censoring, about `3.2 DN` sigma.

Therefore:

- simple monotonic `ISO >= 8192` threshold: **REJECTED**;
- reproducible discrete ISO8192-associated capture/sample domain: **PASS AS OBSERVATION**;
- physical/software cause: **OPEN**.

Do not call it a DCG switch, analog-gain switch, sensor defect or decoder bug without causal evidence.

## 14. New calibration-domain rule

The empirical dark work adds one important project rule:

**Noise calibration must be keyed to exact capture/sample-domain identity, not ISO magnitude alone.**

A future calibrated noise model must fail closed rather than interpolate across an observed domain discontinuity.

This rule affects source interpretation/uncertainty only. It does not change the nul-lijn, Scientific Master identity or reconstruction authority.

If a future Backplane/certificate/calibration record needs a capture-domain identifier, add it only through an explicit versioned schema. Do not silently mutate a frozen serialization.

## 15. What is already proven vs still open

Already implemented/validated or empirically supported:

- four-mode launcher contract;
- fail-safe PURE policy sanitization;
- float32 Scientific Master Linear DNG writer + Android export route;
- preservation of negative and `>1` float components;
- TruthRaw Certificate v0.1 + embedding;
- supplemental exact-source Honor PURE artifact PASS;
- nul-lijn two-sided unbounded representational address-space semantics;
- signed post-black preservation empirically reinforced;
- source-bound gain/response repeatability;
- ISO100/400 temporal dark-noise estimates for tested pairs;
- reproducible discrete exact-ISO8192-associated capture/sample-domain observation;
- rejection of a simple `ISO >= 8192` threshold model.

Still intentionally open / blocked from stronger claim:

- actual JPEG XL encoder validation;
- final renderer semantics for `Colourful`, `Detailed`, `Soft`, `HDR`;
- trusted cryptographic issuer signing key and verifier trust distribution;
- real-device Honor/MotionCam end-to-end float32 DNG validation on all formal five regression sources;
- Adobe Camera Raw / Lightroom interoperability proof;
- independent physical NoiseProfile semantics/calibration;
- full PTC/conversion-gain/read-noise calibration across regimes;
- physical cause of the exact-ISO8192-associated domain;
- `FULL_PHYSICAL` color/light/material authority without independent calibration evidence.

## 16. Recommended next execution order

Unless the user gives a new priority:

1. Re-fetch PR `#23`, branch head and current workflows.
2. Treat the updated nul-lijn history/evidence synthesis as mandatory context for noise/calibration work.
3. Keep the already-validated Scientific Master/PURE export route frozen while continuing the formal five-source device artifact gate.
4. For physical noise calibration, acquire 3-5 repeated dark frames per target capture/sample domain, especially exact ISO8192, nearby ordinary ISO8184 and ordinary ISO10244 under matched shutter/thermal conditions.
5. Acquire controlled flat-field frames at multiple signal levels before fitting a physical PTC/conversion-gain/read-noise model.
6. Independently validate MotionCam/device NoiseProfile color-plane/coordinate semantics.
7. Keep multi-capture calibration evidence separate from the normal one-frame photographic evidence path.
8. Test Adobe Camera Raw / Lightroom independently from scientific correctness.
9. Implement `Colourful`, `Detailed`, `Soft`, `HDR` only downstream with master/backplane identity regression tests.
10. Keep JPEG XL fail-closed until a real encoder is validated.
11. Design trusted signing only with an external/protected issuer key.
12. Do not promote/merge PR #23 merely because documentation is complete; preserve all remaining gates.

## 17. Files the next chat should read first

Read, in order:

1. `START_HERE_NEW_CHAT.md`
2. `docs/TRUTHRAW_PROJECT_MAP_2026-09-13.md`
3. `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
4. `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_EVOLUTION_2026-09-13.md`
5. `docs/research/zero-line-empirical-validation-v0.1/TRUTHRAW_ZERO_LINE_HISTORY_AND_EMPIRICAL_STATE_2026-09-14.md`
6. `docs/research/zero-line-empirical-validation-v0.1/TRUTHRAW_ZERO_LINE_EMPIRICAL_STATE_2026-09-14.json`
7. `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-14.md` (this file)
8. `docs/implementation/OUTPUT_MODES_CERTIFICATE_IMPLEMENTATION_2026-09-14.md`
9. `docs/research/physical-dng-test-set-v0.1/TRUTHRAW_PHYSICAL_TESTSET_2026-09-14.json`
10. `docs/CORE_VISION_HOUSE_GENEALOGY_BACKPLANE_GATEHOUSE_2026-09-13.md`
11. `state/CURRENT_CANONICAL_STATE_2026-09-13.json`
12. module-local README/manifests/tests for the specific feature being changed

## 18. Do not regress these boundaries

Do not:

- move the nul-lijn to sensor BlackLevel;
- call raw code zero physical darkness;
- clip signed post-black/scientific values merely to make the lower side convenient;
- call the unbounded TruthRange address space infinite physical sensor dynamic range;
- edit sealed/canonical files just to make a workflow green;
- change expected hashes to hide an integrity failure;
- call a reconstructed CFA output original measured sensor RAW;
- let JPG/JPG XL/ADVANCED appearance options modify PURE's Scientific Master;
- let a certificate strengthen evidence authority;
- let the Technical Backplane become a second image/master;
- let the Gatehouse become a second scientific source;
- interpolate empirical noise calibration across an observed capture/sample-domain discontinuity;
- call exact ISO8192 a DCG/analog-gain switch without physical causal evidence;
- call JPEG XL production-ready before an encoder is validated;
- call `UNSIGNED DEVELOPMENT` cryptographically verified;
- use GCam/APK/computational-RAW content as TruthRaw evidence/calibration authority.

## 19. Continuity sentence

**TruthRaw preserves one sealed RAW observation as finite evidence, reconstructs a separate scientific scene inside a nul-lijn address space intentionally unbounded both upward toward brighter positive light and downward toward darkness, preserves signed below-black numerical evidence instead of clipping it away, binds empirical noise interpretation to the exact capture/sample domain rather than ISO magnitude alone, and keeps every downstream PURE/ADVANCED/JPG projection from strengthening the underlying evidence claim.**
