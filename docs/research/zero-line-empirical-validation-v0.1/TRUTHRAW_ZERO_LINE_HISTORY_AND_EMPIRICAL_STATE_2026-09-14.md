# TruthRaw Zero-Line / Nul-lijn — history + empirical integration — 2026-09-14

**Status: ACTIVE DESIGN RATIONALE + SOURCE-BOUND RESEARCH EVIDENCE**

This document integrates the recovered TruthRaw history with the 2026-09-14 Honor/MotionCam ISO, response and dark-frame experiments. It does not replace the canonical Zero-Line / TruthRange architecture. Its purpose is to make the historical intent and the newest empirical constraints impossible to lose in a future session.

## 1. Terminology correction: zero-line / nul-lijn

The intended project concept is the **zero-line / nul-lijn**.

The zero-line is the chosen scene gauge/reference `L0` in:

`T = log2(L / L0)`

with `L > 0` for the positive physical-light quantity represented by TruthRange.

`T = 0` means `L = L0`.

It is not:

- sensor black;
- DNG `BlackLevel`;
- zero photons;
- clipping;
- display black;
- middle grey by definition;
- the lower boundary of the reconstructed house.

## 2. Original house intent: unbounded in both directions

The recovered house vision was deliberately symmetric around the zero-line as an address-space concept:

`darkness <- ... <- -EV <- 0-line -> +EV -> ... -> brighter light`

For the positive-light TruthRange coordinate:

- if `L -> +infinity`, then `T -> +infinity`;
- if `L -> 0+`, then `T -> -infinity`.

Therefore the **new house was designed without a finite representational ceiling and without a finite representational floor**. In the user's original metaphor, the house can be extended indefinitely upward and indefinitely downward around the zero-line.

This is a representational/architectural statement. It is not a claim that a physical sensor measures infinite dynamic range.

The sealed RAW still exposes only a finite, noisy, quantized and sometimes censored evidence window inside this larger coordinate space.

Canonical distinction:

**The house can be unbounded in both directions. The evidence inside it is finite.**

## 3. Why the lower side requires two coordinated representations

The downward side cannot be implemented by pretending negative numerical estimates are negative physical light.

TruthRaw therefore keeps two complementary domains:

1. a **signed scene-linear estimator** for unbiased reconstruction, residuals, noise, black-level uncertainty and color/forward-model operations;
2. a **positive-light TruthRange companion** for `T = log2(L/L0)`, bounds, uncertainty and support classes.

Small negative post-black or reconstructed linear values are valid numerical estimates around an uncertain zero level. They must be preserved in the signed domain. They must not be passed naively through `log2` and must not be hard-clipped to zero just to make the positive-light coordinate convenient.

On the dark side, when the measurement no longer resolves a positive latent signal reliably, the scientifically correct state is a bound such as:

`T <= T_dark_upper`

with a lower tail that may remain open toward `-infinity`.

On the bright side, saturation is analogously represented as:

`T >= T_clip_lower`

with the upper tail open toward `+infinity` until valid reconstruction/model support narrows it.

This is the intended two-sided meaning of the zero-line architecture.

## 4. Historical genealogy that must remain attached to the zero-line

The project history now resolves as:

`immutable sealed Direct-CFA evidence`

`-> new reconstructed house not numerically bounded by RAW10/WhiteLevel`

`-> zero-line / T = log2(L/L0)`

`-> finite sensor range separated from unbounded scene address space`

`-> clipping represented as censoring/bounds rather than a scene endpoint`

`-> signed floating-point Scene/Scientific Master work`

`-> TruthRange v0.1 provisional ISO/exposure gauge`

`-> TruthRange v0.2 SELF_GAUGE / multiplicative invariance`

`-> explicit uncertainty/support roles`

`-> tiled/streaming evaluation with one coherent scene/gauge identity`

`-> Technical Backplane carrying source/master/zero-line/scene-scale identity`

`-> downstream float32 TRUTHRAW PURE projection preserving negative and greater-than-one numerical values`

The key historical correction remains: the zero-line predates the self-gauge implementation. `SELF_GAUGE` refined how `L0` is chosen; it did not invent the zero-line or the two-sided unbounded-house idea.

## 5. 2026-09-14 empirical campaign A — real-scene ISO ladder

Seven HONOR BKQ-N49 MotionCam DNGs were measured as one source-bound ISO-response series:

- `IMG_20260908_194910.dng`
- `IMG_20260908_194916.dng`
- `IMG_20260908_194924.dng`
- `IMG_20260908_194929.dng`
- `IMG_20260908_194939.dng`
- `IMG_20260908_194947.dng`
- `IMG_20260908_194951.dng`

Observed ISO ladder:

`200 -> 400 -> 800 -> 1600 -> 3200 -> 6400 -> 12800`

Shared source properties included 4080x3072 BGGR CFA and `WhiteLevel=1023`.

Important findings:

- ISO 200 and ISO 400 used the same approximately `0.09940137 s` exposure and aligned smooth-region signal at ISO 200 was about half ISO 400 for B/G, consistent with a one-stop response change.
- At ISO 1600/3200/6400/12800, the within-frame green checker measured/predicted NoiseProfile ratios were approximately `1.009 / 1.024 / 1.033 / 1.053`.
- Low-ISO checker estimates were quantization/scene-gradient limited and were not promoted to a NoiseProfile failure.
- Below-black samples rose from 109 at ISO 3200 to 4,974 at ISO 6400 and 59,573 at ISO 12800, where they represented about `0.4753%` of all sensels.
- The most negative ISO12800 black-corrected sample observed in that audit was `-22 DN`.

Project meaning:

**Real physical source data demonstrated that signed post-black values below zero are not merely a mathematical convenience. Hard clipping them to zero would destroy measured shadow-side fluctuations.**

This supports the signed lower side of the zero-line architecture while still not interpreting negative estimator values as negative photons.

Status:

- source-bound ISO/noise regression: **PASS**;
- independent physical calibration: **OPEN**;
- certified gain-switch point: **OPEN**.

## 6. 2026-09-14 empirical campaign B — response/repeatability follow-up

A second seven-file series added repeat opportunities:

- `IMG_260914_104212_969_001.dng`
- `IMG_260914_104222_376_003.dng`
- `IMG_260914_104225_763_005.dng`
- `IMG_260914_104228_111_007.dng`
- `IMG_260914_104230_455_009.dng`
- `IMG_260914_104232_099_011.dng`
- `IMG_260914_104243_236_013.dng`

The sequence contained ISO 100, 100, 200, 800, 800, 1600 and 6444.

Direct same-exposure ISO100 -> ISO200 signal ratios after affine alignment were:

- B: `1.9087x`;
- G1: `1.9679x`;
- G2: `1.9907x`;
- R: `1.9994x`.

The expected ideal one-stop ratio was `2.000x`.

The two ISO800 captures reproduced after alignment at approximately:

- B: `0.9905x`;
- G1: `1.0000x`;
- G2: `1.0000x`;
- R: `1.0000x`.

The second ISO100 capture changed focus/geometry metadata, and framing moved enough that these repeats were not promoted to temporal-noise calibration pairs.

Status:

- source-bound gain response: **PASS**;
- scene-signal repeatability: **PASS**;
- clean temporal calibration: **OPEN**.

## 7. 2026-09-14 empirical campaign C — covered/dark series

Ten additional DNGs were captured as dark/no-scene measurements:

- `IMG_260914_104858_011_015.dng`
- `IMG_260914_104916_112_017.dng`
- `IMG_260914_104919_216_019.dng`
- `IMG_260914_104921_224_021.dng`
- `IMG_260914_104923_408_023.dng`
- `IMG_260914_104925_197_025.dng`
- `IMG_260914_104927_269_027.dng`
- `IMG_260914_104944_692_029.dng`
- `IMG_260914_104948_936_031.dng`
- `IMG_260914_104952_817_033.dng`

The first nine formed a dark ladder at one common exposure of approximately `0.0327347964 s` with ISO values:

`100, 100, 400, 400, 800, 1600, 3200, 6397, 12522`.

The duplicate pairs enabled direct temporal-noise estimation with `(A-B)/sqrt(2)`.

Measured temporal sigma across CFA channels was approximately:

- ISO100: `0.789-0.790 DN`;
- ISO400: `0.867-0.870 DN`.

Repeat-frame correlation was effectively zero at ISO100 and around `0.004-0.005` at ISO400 after excluding extreme outliers, so the stored variation in those tested pairs was overwhelmingly temporal.

The dark frames also contained genuine samples below metadata BlackLevel. This directly reinforces the rule that the signed scientific domain must not clamp post-black values at zero.

### NoiseProfile semantics finding

Under the ordinary DNG R/G/B color-plane interpretation, the embedded middle NoiseProfile pair predicted much less stored green dark noise than measured at low ISO.

This is recorded as an **OPEN metadata-semantics/calibration issue**, not as permission to rewrite source metadata. MotionCam/device-specific NoiseProfile channel ordering or coordinate semantics remain unverified independently.

## 8. The special exact-ISO8192 sample domain

The last file of campaign C, `IMG_260914_104952_817_033.dng`, was ISO8192 at approximately `0.0163673982 s` and behaved very differently from the preceding ordinary dark ladder.

Its observed dark sigma was roughly `54-55 DN`, and about `7.1204%` of all sensels were already serialized at raw code zero.

Relative to the preceding ISO12522 dark file, the embedded NoiseProfile changed by approximately:

- `S`: about `16x`;
- `O`: about `267x`;
- `sqrt(O)`: about `16.34x`.

That scaling has the algebraic signature expected when a variance model is expressed after a large multiplicative coordinate/sample scaling.

The files did not identify the physical/software cause.

## 9. 2026-09-14 empirical campaign D — threshold hypothesis falsified

A later ten-file dark series used one common exposure of approximately `0.01648959145 s` and the ISO sequence:

`8192, 10244, 8184, 6500, 3200, 1600, 800, 400, 200, 100`.

Key files include:

- exact ISO8192: `IMG_260914_110550_363_001.dng`, SHA-256 `2dae19d2e9d144c4d257808432e58a7246764e1259d6a5033de2e1eff76c644e`;
- nearby ISO8184: `IMG_260914_110600_897_005.dng`;
- higher ISO10244: `IMG_260914_110556_463_003.dng`.

The previous exact-ISO8192 sample was:

- `IMG_260914_104952_817_033.dng`, SHA-256 `06bf9afdf6a71d4c0b3995dec641f33a8d205cd022139440960131b348650ec3`.

Across these two independent sessions, exact ISO8192 reproduced:

- identical BlackLevel values;
- identical six-value DNG NoiseProfile;
- approximately `54-55 DN` trimmed dark sigma;
- approximately `7.1%` raw-zero censoring.

At the same exposure, ISO8184 instead showed:

- `0%` raw-zero censoring;
- about `2.6 DN` trimmed dark sigma.

ISO8192 versus ISO8184 NoiseProfile scaling was roughly:

- `S`: about `23.6-24.4x`;
- `O`: about `584-624x`;
- `sqrt(O)`: about `24.2-25.0x`.

ISO10244, despite being numerically higher than 8192, stayed in the ordinary domain with:

- `0%` raw-zero censoring;
- about `3.2 DN` trimmed dark sigma.

Therefore the hypothesis of a simple monotonic `ISO >= 8192` threshold is **REJECTED**.

The correct current name is:

**reproducible discrete ISO8192-associated capture/sample domain**.

Its physical cause remains **OPEN**. Do not call it a DCG switch, analog-gain switch, sensor defect or decoder bug without additional causal evidence.

## 10. New calibration-domain rule

The empirical results add an important rule to the measurement/calibration layer:

**Noise calibration must be bound to the exact capture/sample-domain identity, not to ISO magnitude alone.**

Consequences:

- do not interpolate a calibrated noise model across an observed capture/sample-domain discontinuity;
- preserve source metadata as evidence/provenance even when its semantics are under investigation;
- fail closed when the capture/sample domain cannot be matched to the calibration domain;
- a future empirical calibration identity should include source/camera/mode/sample-domain information in addition to ISO/gain/exposure;
- any future Backplane/certificate field for such a domain must be introduced through an explicit versioned schema change rather than rewriting a frozen serialization silently.

This rule changes how calibration locates evidence in the house. It does **not** change how high or deep the house is allowed to exist.

## 11. Zero-line consequences of the empirical evidence

The 2026-09-14 measurements strengthen, but do not inflate, the zero-line claim.

They support:

- a signed post-black estimator below the chosen numerical zero;
- explicit dark/noise-side uncertainty rather than black clipping;
- source-domain-aware uncertainty models;
- the separation between finite sensor evidence and the unbounded representational address space;
- the need to retain source ISO/readout/noise provenance even when TruthRange itself is self-gauged.

They do **not** prove:

- negative physical light;
- an infinitely sensitive sensor;
- infinite measured dynamic range;
- exact darkness from a black/zero code;
- exact latent highlights from clipped samples;
- a physical cause for the exact-ISO8192-associated domain.

## 12. Relationship to TRUTHRAW PURE float32

The current TRUTHRAW PURE writer preserves negative and greater-than-one floating-point components in its downstream XYZ-D50 LinearRaw projection.

That is consistent with the scientific-master principle that representation must not silently clip valid signed/overrange numerical state merely to satisfy a conventional bounded image interval.

This does not make the projection new evidence and does not mean negative XYZ components are negative photons.

The real-device PURE validation and the zero-line/noise campaigns are complementary:

- PURE proves the output carriage can preserve the richer scientific representation;
- the dark/ISO experiments constrain how source evidence and uncertainty must be interpreted before or within reconstruction.

## 13. Current project decisions

The following statuses are now active:

- unbounded zero-line/TruthRange address space in both directions: **CANONICAL PASS**;
- finite source evidence window: **CANONICAL PASS**;
- signed post-black preservation: **EMPIRICALLY REINFORCED PASS**;
- source clipping as high-side censoring: **CANONICAL PASS**;
- dark/noise side as uncertainty/bound rather than hard physical zero: **CANONICAL PASS + EMPIRICAL SUPPORT**;
- source-bound ISO/gain-response regression: **PASS**;
- ISO100/400 temporal dark-noise estimates: **PASS FOR TESTED PAIRS**;
- DNG NoiseProfile as a source-bound high-ISO prior: **SUPPORTED**, but exact device/app semantics remain **OPEN**;
- simple monotonic ISO8192 threshold: **REJECTED**;
- reproducible discrete ISO8192-associated capture/sample domain: **PASS AS OBSERVATION**;
- physical cause of that domain: **OPEN**;
- independent physical read-noise/PTC/conversion-gain calibration: **OPEN**;
- full physical color calibration: **BLOCKED** without independent calibration evidence.

## 14. What must not change because of these tests

Do not:

- move the zero-line to sensor BlackLevel;
- treat raw zero as physical darkness;
- clip the signed Scientific Master or Stage-2 estimator at zero;
- turn the unbounded address space into an infinite-sensor claim;
- promote source metadata NoiseProfile to independent calibration without validation;
- interpolate across the exact-ISO8192-associated domain just because ISO values are numerically adjacent;
- change the Scientific Master or reconstruction algorithm merely to make a calibration plot smoother;
- let multi-capture calibration evidence turn final photographic reconstruction into a multi-frame evidence path.

## 15. Next empirical gate

For physical sensor/noise calibration rather than source-bound regression:

- capture at least 3-5 dark repeats per target ISO/sample domain;
- specifically repeat exact ISO8192, nearby ordinary ISO8184 and ordinary ISO10244 under matched shutter/thermal conditions;
- acquire controlled flat-field frames at multiple signal levels;
- keep framing, focus, illumination and thermal state controlled where applicable;
- validate NoiseProfile color-plane/coordinate semantics independently;
- only then fit variance-vs-signal, conversion gain/read-noise and investigate any DCG/readout-mode hypothesis.

These calibration captures may be multi-capture **calibration evidence**. The normal TruthRaw photograph remains one sealed source observation unless a separately authorized architecture says otherwise.

## 16. Permanent continuity sentence

**The TruthRaw nul-lijn was designed so the new house has no finite representational ceiling above it and no finite representational floor below it: positive-light TruthRange can extend toward `+infinity` and `-infinity`, while the sealed RAW contributes only a finite uncertain evidence window, the signed scientific estimator preserves real below-black fluctuations, and every reconstruction outside direct support remains explicitly reconstruction rather than invented measurement.**
