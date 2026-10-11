# D.RAW 44489 — Interdisciplinary Observation Audit — 2026-10-10

Status: **BINDING RESEARCH / ARCHITECTURE AUDIT, DOCUMENTATION-ONLY**  
Project: D.RAW / TruthRaw repository history  
Continuation code: **44489**  
Scope: sealed RAW intake, colour, high-ISO inference, optics/resolution, continuous scientific representation, human vision, 3D/projective geometry, artificial light, stop-motion/temporal sampling, architecture and conservation/restoration.

This document records research direction and promotion gates. It does **not** modify runtime code, sealed source evidence, Scientific Master, candidate state, product defaults or scientific promotion.

Permanent motto: **One Free World. Many sealed observations. One evidence law.**

---

## 1. Audit question

Every future image-quality decision must be separated along two axes:

1. **What can physically be learned from the sealed observation?**
2. **What may only be added downstream as a natural human-facing Appearance?**

The project must never use better visual plausibility as a substitute for stronger evidence.

A RAW observation is not merely a raster of pixel values. It is a sealed measurement carrying several coupled information layers:

- radiometric samples and censoring/clipping state;
- CFA/sampling geometry and phase;
- source/container calibration information;
- optical/spatial support;
- exposure/timing context;
- noise/uncertainty information;
- provenance and parser semantics.

The scientific representation may become richer than the source representation; authority may not become richer than the evidence.

---

## 2. Interdisciplinary foundation

### Photography / sensor metrology

Raw sensel values are measurements affected by photon statistics, read/dark/electronic components, quantization, black-reference behavior, saturation/censoring and the source's acquisition chain. Scientific denoise/reconstruction must therefore be formulated as evidence-weighted estimation with explicit uncertainty, not as generic smoothing.

EMVA 1288 provides an important metrology reference for measured signal/noise characterization and photon-transfer reasoning. It is a source for test methodology and physical modeling concepts, not a license to infer unavailable sensor parameters from one image.

### Optics / real resolution

Pixel dimensions are not synonymous with resolved scene detail. Optical resolution depends on the combined imaging chain, including lens, sampling aperture, sensor sampling and processing. ISO 12233 explicitly distinguishes addressable photoelements from resolution and spatial-frequency response.

Therefore an output may contain 8K samples without claiming 8K independently observed optical detail.

### Colour science / human vision

Natural-looking colour is not equivalent to maximum saturation. Appearance depends on scene-referred relationships plus viewing luminance, surround, adaptation, local contrast and perceptual colourfulness/chroma. CIECAM16 is explicit that colour appearance is viewing-condition-specific.

Scientific colour formation and natural Appearance must remain distinct.

### Radiometry / 3D / physically based rendering

Emission and reflection are physically distinct. The light-transport equation separates emitted radiance from reflected/scattered incident radiance. A TV/OLED/LED pixel is not physically equivalent to a reflective yellow surface merely because both can result in an RGB triplet.

### Stop-motion / temporal sampling

A geometrically static scene does not imply time-invariant radiance during exposure. PWM, LED drivers, display refresh/scanout and rolling shutter may change the integrated colour/luminance recorded by the camera. A sealed frame represents an exposure integral over its acquisition process, not an instantaneous universal scene truth.

### Projective geometry / architecture / depth

Perspective, vanishing points, lines, planes and homographies can provide strong structural evidence. A single image generally does not uniquely determine metric scale, complete depth, materials or illumination. World-space authority must remain fail-closed where those quantities are not supported.

### Conservation / restoration principle

The project should treat the sealed observation like an original conservation object: preserve the original, document every intervention, keep interventions minimal and non-destructive where practical, and make their provenance explicit. This is an architectural analogy; it does not imply that every digital transformation is mathematically reversible.

---

## 3. RAW intake — richer without vendor authority

“Source-agnostic” does **not** mean “ignore metadata”. It means information is admitted because it is present, parsed, semantically validated and source-bound — never because a camera/vendor/model name is assumed to imply scientific truth.

Immediately downstream of sealing, future intake should support a richer:

**Source Capability Envelope v0.2 + RAW Evidence Harvest + Observation Evidence Ledger**.

Candidate source facts, only when present and semantically valid, include:

- CFA pattern and exact phase;
- active/masked sensor regions;
- effective sample geometry;
- packing and bit depth;
- BlackLevel structure and black-level deltas;
- WhiteLevel and observed clipping/censoring structure;
- linearization information;
- exposure time;
- aperture;
- gain/ISO as acquisition context, not universal truth;
- source-contained noise profile when valid;
- colour/calibration matrices;
- AsShotNeutral / source white-point information;
- calibration illuminants;
- lens-shading/distortion or other source geometry information;
- standardized DNG/RAW operation metadata only where semantics are proven.

For every datum, record the lifecycle:

`PRESENT -> PARSED -> VALIDATED -> ADMITTED -> USED`

and preserve:

- exact source location/tag/IFD or equivalent;
- raw representation where appropriate;
- parser/version identity;
- admitted semantic meaning;
- uncertainty/limitations;
- contradictions;
- rejection reason when not admitted;
- downstream consumers.

Proprietary MakerNotes or opaque vendor blocks remain provenance-only until independently understood and validated. Camera/lens identity may route parsing or describe provenance; it may not silently select a scientific calibration.

---

## 4. Fuller natural colour without forgetting origin

The target is not “more saturation”. The target is a better separation between **scientifically supported colour formation** and **human-facing natural appearance**.

### 4.1 Scientific Colour Formation

This layer should remain scene/source-referred and authority-bound:

- preserve negative intermediate components and values above nominal display range where scientifically meaningful;
- preserve censoring and UNKNOWN rather than cosmetically clipping them into certainty;
- propagate colour uncertainty/covariance where available;
- treat a 3x3 camera-to-XYZ or equivalent transform as authoritative only over its proven calibration domain;
- do not claim exact human colorimetry for arbitrary narrowband spectra from three camera channels.

### 4.2 Natural Appearance

Only downstream of Scientific Master / scientific output state:

- chromatic adaptation;
- viewing-condition context;
- tone mapping;
- local contrast/luminance shaping;
- perceptual chroma/colourfulness;
- gamut mapping;
- output encoding/profile.

These are `APPEARANCE` operations and must never be written back as `MEASURED`.

### 4.3 Natural Appearance Graph

Existing Appearance modules such as Warm Illuminant Retention, Natural Light Local Field, Censored Chroma Fallback, Deep-Censor/Hue-Floor candidates and future operators should converge architecturally toward a declarative graph in which every node states:

- input authority accepted;
- working colour/transfer space;
- exact no-op region;
- maximum intervention;
- provenance/configuration;
- interaction/order dependencies;
- reversibility/undo relationship where practical;
- explicit prohibition on Scientific-Master writeback.

This allows fuller natural colour while retaining exactly where every transformation came from.

---

## 5. High-ISO noise — scientific direction

Conventional RGB blur/denoise must not become the scientific core.

A high-ISO observation may contain photon shot noise, read/electronic noise, dark/fixed-pattern components, quantization effects and reconstruction uncertainty. These are not a single separable “grain layer”.

The correct research question is:

**Which latent radiance is best supported by these sealed sensor measurements, with what uncertainty?**

Preferred chain:

`sealed CFA -> black/linearity/reliability -> censor state -> source-bound noise likelihood -> CFA phase + local structure + optical support -> evidence-weighted latent estimate -> uncertainty -> colour -> Appearance`

Where calibration/evidence permits, a noise likelihood may model signal-dependent variance plus a read/dark/quantization floor. If one RAW cannot identify those components reliably, D.RAW must widen uncertainty or mark parameters UNKNOWN rather than fabricate coefficients.

Permanent rule:

**clean image != perfect knowledge**.

The existing Anchor-Constrained Local Reconstruction affine candidate remains NOT PROMOTED because true target-excluded tele CFA hold-outs were worse overall than baseline and its uncertainty was too optimistic.

The next scientific model direction may use a deterministic local model bank, for example:

- constant/local robust estimate;
- affine/local plane;
- directional/edge-aware basis;
- periodic/texture basis only when supported;
- `NO_SUITABLE_MODEL`.

Model selection must be based on support that excludes the target sample and must be validated on genuine target-excluded hold-outs. The objective is to reduce stochastic variance while preserving hair, fabric, brick, foliage, seams and other real high-frequency structure.

---

## 6. Beyond raster as the primary scientific representation

Free Raster remains useful and should **not** be removed. Its role should become clearer:

**Free Raster is a sampling/export interface, not the definition of the scientific image itself.**

Research direction:

### Observation-Bound Continuous Image Field

A deterministic, non-generative representation may conceptually expose:

- `mu(x,y,c)` — best-supported local scene estimate;
- `Sigma(x,y,c)` — uncertainty/covariance;
- `Authority(x,y)` — MEASURED / CALIBRATED_ESTIMATE / RECONSTRUCTED / CENSORED / UNKNOWN;
- `OpticalSupport(x,y,f)` — locally supported spatial-frequency range;
- `Model(x,y)` — admitted local deterministic basis/model;
- `Provenance(x,y)` — sealed observation and support lineage.

Candidate representations may include splines, wavelets, local polynomials or other deterministic basis functions, but only after true hold-outs prove they improve reconstruction without inventing detail.

Preferred conceptual output route:

`Continuous Scientific Field -> Natural Appearance Field -> one final sampling -> Original / 1080p / 4K / 8K / Custom raster`

An 8K raster may therefore contain 8K samples while provenance honestly records that optical support is lower where the observation cannot resolve corresponding frequencies.

Scientific use of generative NeRF-like world completion or hallucinated detail is explicitly rejected. Multi-view geometry may enrich the Free World only from separately sealed observations with preserved provenance and appropriate uncertainty.

---

## 7. Artificial light, emissive sources and stop-motion

Future colour/light decisions must preserve these distinctions:

- self-emitted radiance vs reflected/transmitted radiance;
- spectral power distribution vs trichromatic camera response;
- large uniform colour field vs small subpixel/glyph sampling;
- scene geometry being static vs radiance being temporally constant;
- rolling-shutter row timing vs global exposure assumptions.

For displays/LED/OLED and other temporally modulated emitters, a useful test matrix includes:

- large red/green/blue/yellow/white fields;
- same colours as small glyphs/lines;
- repeated sealed RAW captures;
- materially different exposure durations where practical;
- row/position comparison when rolling-shutter interaction is plausible.

This can discriminate spectral/matrix, spatial CFA/PSF and temporal modulation hypotheses without semantic “screen detection”.

---

## 8. Black and white values

Never collapse scientific sensor limits into display aesthetics.

Measurement concepts:

- sensor/source BlackLevel and black reference;
- zero-line/gauge;
- WhiteLevel/saturation;
- clipping/censoring;
- radiometric uncertainty.

Appearance concepts:

- display black;
- display white;
- contrast curve;
- highlight roll-off;
- shadow rendering.

A prettier JPEG may never authorize rewriting the scientific BlackLevel or saturation state.

---

## 9. Audit outcome

| Area | Current foundation | Highest-value next step |
|---|---|---|
| RAW intake | architecture/evidence law strong | richer source-bound Evidence Harvest + Ledger |
| Colour | Science/Appearance split strong | calibration-domain-aware colour + declarative Natural Appearance Graph |
| High ISO | philosophy correct; reconstruction unpromoted | source-bound noise likelihood + deterministic local model selection + true hold-outs |
| Resolution | Free Raster correctly downstream | continuous/adaptive scientific field before rasterization |
| Optics/detail | authority contracts exist | local PSF/SFR/bandwidth support where evidence permits |
| 3D/light | cautious/fail-closed | use structural evidence without inventing metric depth/material/light |
| Provenance | very strong | carry Evidence Ledger and authority/uncertainty to export |

---

## 10. Recommended execution order

1. **Source Capability Envelope v0.2 + Observation Evidence Ledger** — documentation/schema + parser introspection first; zero pixel changes.
2. **Source-bound noise likelihood** — research-only; no denoise promotion.
3. **Evidence-Weighted Local Radiance Reconstruction / Continuous Field** — research-only with true CFA hold-outs and calibrated uncertainty.
4. **Optical-support binding** — only where source/test evidence justifies spatial-frequency claims.
5. **Natural Appearance Graph** — compose existing downstream operators declaratively; Scientific Master invariant.
6. **Free Raster single-sampling output** — sample arbitrary output dimensions from the best admitted representation without claiming invented detail.
7. **Free World / 3D enrichment** — only from separately sealed observations or explicit calibrated evidence; unsupported dimensions remain UNKNOWN/fail-closed.

Guiding line:

> **Niet meer pixels verzinnen, maar meer van de werkelijk aanwezige informatie begrijpen.**

---

## 11. Promotion gates

### Gate A — Intake completeness

Must prove:

- representative real fixtures;
- parser lifecycle and exact source trace;
- semantic validation before admission;
- explicit rejection/UNKNOWN behavior;
- no brand/model-name scientific authority;
- zero source mutation and zero pixel change from the ledger itself.

### Gate B — Noise likelihood

Must prove:

- each parameter is calibration-backed, source-backed or explicitly UNKNOWN;
- normalized residual / likelihood behavior on held-out measurements where possible;
- no fabricated read/shot/fixed-pattern coefficients;
- no candidate application to Scientific Master.

### Gate C — Reconstruction

Must prove on true target-excluded CFA hold-outs:

- aggregate and per-channel error;
- bias;
- uncertainty coverage/calibration;
- edges/textures/high-frequency strata;
- deterministic reproducibility;
- protected measured anchors unchanged;
- `NO_SUITABLE_MODEL` works fail-closed.

### Gate D — Continuous field

Must prove:

- match/beat current raster baseline on genuine hold-outs;
- no unsupported high-frequency claim;
- output sampling does not create MEASURED authority;
- provenance/authority/uncertainty survive arbitrary raster sizes.

### Gate E — Appearance

Must prove:

- Scientific Master invariant;
- exact node order/configuration/provenance;
- no-op regions and maximum intervention;
- perceptual/user validation is classified separately from scientific validation.

### Gate F — 3D/world

Must prove:

- no unsupported metric depth, material, reflectance or illumination promotion;
- single-view ambiguity stays explicit;
- multi-observation links bind to separately sealed sources;
- absent/contradictory support remains UNKNOWN/fail-closed.

All research modules remain inactive for scientific production authority until their own gate passes.

---

## 12. External knowledge anchors

These references inform tests/models but do not themselves create source-specific evidence:

- Adobe, **Digital Negative (DNG) 1.7.1.0 specification / DNG SDK resources**: https://helpx.adobe.com/camera-raw/desktop/dng-and-file-formats/digital-negative.html
- EMVA, **EMVA 1288 — camera/image-sensor characterization**: https://www.emva.org/standards-technology/emva-1288/
- ISO, **ISO 12233:2024 — Digital cameras — Resolution and spatial frequency responses**: https://www.iso.org/standard/88626.html
- CIE, **CIE 248:2022 — CIECAM16**: https://www.cie.co.at/publications/cie-2016-colour-appearance-model-colour-management-systems-ciecam16
- Pharr, Jakob, Humphreys, **Physically Based Rendering, 4th ed. — Radiometry, Spectra, Color and Light Transport**: https://www.pbr-book.org/4ed/contents
- Hartley & Zisserman / Oxford VGG, **Multiple View Geometry in Computer Vision**: https://www.robots.ox.ac.uk/~vgg/hzbook/

The current primary-standard anchors support these specific architectural decisions: pixel count must not be conflated with real resolution; colour appearance depends on viewing conditions; sensor-noise models require measured characterization; emitted and reflected radiance are physically distinct; and projective geometry must not be overinterpreted as unique metric world truth.

---

## 13. Non-authorizations

This audit does **not** authorize:

- scientific candidate promotion;
- changes to sealed CFA/RAW_SENSOR bytes;
- Scientific-Master writeback;
- AI/ML/generative scientific inference;
- vendor-specific calibration by identity;
- generic RGB blur denoise as scientific truth;
- semantic screen/object recolouring;
- inferred SPD from three camera channels without evidence;
- invented depth/material/illumination;
- treating raster density as optical evidence;
- replacing Free Raster with a generative world model.

It authorizes only a documented research/order-of-operations direction under the existing evidence law.
