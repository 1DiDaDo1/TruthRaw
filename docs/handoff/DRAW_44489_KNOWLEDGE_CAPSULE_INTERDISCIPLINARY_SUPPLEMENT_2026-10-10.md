# D.RAW 44489 — Binding Knowledge Capsule Supplement — Interdisciplinary Observation Audit — 2026-10-10

Status: **BINDING 44489 RECOVERY SUPPLEMENT / DOCUMENTATION ONLY**

Read this file **immediately after** `docs/handoff/DRAW_44489_KNOWLEDGE_CAPSULE.md` whenever continuation code **44489** is used for work touching RAW intake, colour, high-ISO/noise, reconstruction, optical resolution, Free Raster, Free World/3D, artificial light, temporal sampling or Natural Appearance.

Reason for a supplement rather than destructive rewriting: the canonical capsule already contains newer live checkpoints through section 23, including active Evidence/Uncertainty/Authority integration, residual-purple/deep-censor work and the Censored Illuminant Hue Floor candidate. Those checkpoints are preserved verbatim. This supplement extends them; it does not supersede or delete them.

No runtime, sealed source, Scientific Master, accepted Appearance default, scientific promotion state or PR merge state is changed by this supplement.

---

## 1. Binding new audit direction

The 2026-10-10 interdisciplinary audit establishes the following permanent research direction:

1. treat a sealed RAW as a multi-layer measurement, not merely a raster;
2. improve intake first with richer source-bound evidence extraction and explicit lifecycle/provenance;
3. pursue fuller natural colour by strengthening Scientific Colour Formation vs Natural Appearance separation, not by indiscriminate saturation;
4. pursue high-ISO quality through source-bound noise likelihood + deterministic evidence-weighted reconstruction + calibrated uncertainty, not generic RGB blur;
5. retain Free Raster, but treat it as the final sampling/projection/export surface rather than the fundamental scientific image representation;
6. research an observation-bound deterministic continuous/adaptive scientific field carrying estimate, uncertainty, authority, optical support, model identity and provenance;
7. keep 3D/depth/material/light claims fail-closed where a single observation cannot support them;
8. preserve emission vs reflection and exposure-time/rolling-shutter/PWM distinctions for artificial/emissive light;
9. preserve sensor black/white/censoring as measurement concepts and display black/white/tone as Appearance concepts;
10. use conservation/restoration principles as governance: original sealed evidence untouched, interventions documented, minimal/non-destructive where practical, provenance retained.

Guiding line:

> **Niet meer pixels verzinnen, maar meer van de werkelijk aanwezige informatie begrijpen.**

---

## 2. Preferred first implementation — zero pixel changes

The preferred next broad scientific implementation is:

**Source Capability Envelope v0.2 + RAW Evidence Harvest + Observation Evidence Ledger**.

Every candidate source datum should follow:

`PRESENT -> PARSED -> VALIDATED -> ADMITTED -> USED`

and preserve source location/tag/IFD or equivalent, parser/version, semantic meaning, uncertainty/limitations, contradictions, rejection reason and downstream consumers.

Candidate fields, only when present and semantically valid, include CFA pattern/phase, active/masked regions, packing/bit depth, BlackLevel/deltas, WhiteLevel, linearization, observed clipping/censoring, exposure/aperture/gain context, valid source-contained noise information, colour/calibration matrices, source white/calibration illuminants, optical/geometric metadata and standardized RAW/DNG operation metadata.

MakerNotes/private opaque blocks remain `PROVENANCE_ONLY` / `UNKNOWN_SEMANTICS` until independently validated. Vendor/camera/lens/app identity may route parsing or describe provenance; it may not select scientific truth by name.

**First milestone must prove zero pixel changes** to Scientific Master and product output until a separately admitted consumer explicitly uses a newly admitted datum.

---

## 3. Fuller natural colour

Future architecture should explicitly preserve two layers:

### Scientific Colour Formation

- scene/source-referred;
- calibration-domain-aware;
- negative and >1 intermediates retained where meaningful;
- censoring/UNKNOWN retained;
- uncertainty/covariance propagated where supported;
- a fixed 3x3 transform is not declared exact human colorimetry for arbitrary spectra.

### Natural Appearance

Downstream only:

- chromatic adaptation/viewing context;
- tone;
- local luminance/contrast;
- perceptual chroma/colourfulness;
- gamut mapping;
- output encoding/profile.

Existing Appearance operators should converge toward a declarative **Natural Appearance Graph**. Every node should state input authority, working space, no-op region, maximum intervention, order dependencies, provenance/configuration and Scientific-Master-writeback prohibition.

This is how D.RAW may obtain fuller/natural colour **without forgetting origin**.

---

## 4. High-ISO / scientific noise direction

Do not add conventional RGB blur/denoise as the scientific core.

Preferred research chain:

`sealed CFA -> black/linearity/reliability -> censor state -> source-bound noise likelihood -> CFA phase + local structure + optical support -> evidence-weighted latent estimate -> uncertainty -> colour -> Appearance`

Where evidence supports it, noise likelihood may include signal-dependent variance plus read/dark/quantization floor. If a single observation cannot identify components reliably, widen uncertainty or retain UNKNOWN parameters instead of inventing coefficients.

Permanent rule:

**clean image != perfect knowledge**.

The deterministic local model bank may contain constant/local robust, affine/local plane, directional/edge-aware and supported texture/periodic bases plus `NO_SUITABLE_MODEL`. Genuine target-excluded CFA hold-outs remain mandatory. Aggregate/per-channel error, bias and uncertainty coverage must be reported. The historical affine hold-out result remains a negative promotion result and must not be overwritten by visual plausibility or coverage.

---

## 5. Beyond a fixed raster

Free Raster stays. Its role becomes clearer:

**Free Raster = final sampling / projection / export interface.**

Research direction: **Observation-Bound Continuous Scientific Field**, deterministic and non-generative, conceptually exposing:

- `mu(x,y,c)` — best-supported local estimate;
- `Sigma(x,y,c)` — uncertainty/covariance;
- `Authority(x,y)` — MEASURED / CALIBRATED_ESTIMATE / RECONSTRUCTED / CENSORED / UNKNOWN;
- `OpticalSupport(x,y,f)` — evidenced spatial-frequency support;
- `Model(x,y)` — admitted deterministic basis/model;
- `Provenance(x,y)` — sealed-source/support lineage.

Splines, wavelets, local polynomials or other deterministic bases are research candidates only and must beat/match current baselines on true hold-outs. Generative NeRF/world completion, hallucinated super-resolution and AI/ML scientific inference remain prohibited.

Desired long-term output route:

`Continuous Scientific Field -> Natural Appearance Field -> ONE final sampling -> Original / 1080p / 4K / 8K / Custom`

Raster sample count must never be relabelled as optical information. 8K output may contain 8K samples without claiming 8K independently measured detail.

---

## 6. 3D / architecture / artificial light / stop-motion / vision boundaries

- Projective lines, vanishing points, planes and homographies may constrain structure; one image generally does not uniquely prove metric scale, complete depth, material or illumination.
- Room Capsule / Free World remain fail-closed for unsupported world claims.
- Self-emitted and reflected/transmitted radiance are physically distinct.
- A static scene may still contain time-varying radiance from PWM, LED drivers or display refresh/scanout; rolling shutter gives different rows different integration windows.
- Sensor BlackLevel/zero-line/WhiteLevel/saturation/censoring remain measurement concepts; display black/white/contrast/roll-off remain Appearance concepts.
- Human naturalness depends on luminance/chromatic adaptation, surround and contrast, which justifies downstream Appearance but never Scientific-Master falsification.

---

## 7. New mandatory promotion gates

### Gate A — Intake completeness
Real fixtures; exact source trace; semantic validation; contradiction/fail-closed behavior; no brand-based scientific authority; zero-pixel-change first milestone.

### Gate B — Noise likelihood
Every parameter calibration/source-backed or UNKNOWN; residual/likelihood validation where possible; no fabricated coefficients; no Scientific-Master candidate application.

### Gate C — Reconstruction
True target-excluded CFA hold-outs; aggregate/per-channel error and bias; uncertainty coverage; edges/textures; measured anchors unchanged; deterministic; `NO_SUITABLE_MODEL` fail-closed.

### Gate D — Continuous field
Match/beat current raster baseline on genuine hold-outs; honest bandwidth/optical support; arbitrary raster density creates no MEASURED authority; provenance/uncertainty survive resampling.

### Gate E — Appearance
Scientific Master invariant; exact node order/config/provenance; no-op and max-intervention bounds; perceptual validation separate from scientific validation.

### Gate F — 3D/world
No unsupported metric depth/material/illumination promotion; separately sealed observation binding for multi-view; missing/contradictory support stays UNKNOWN/fail-closed.

No research implementation self-promotes by compiling, building or looking better.

---

## 8. Preferred broad scientific execution order

Without disturbing the current section-18-through-23 product/runtime checkpoints in the canonical capsule:

1. `Source Capability Envelope v0.2 + Observation Evidence Ledger` — zero-pixel-change schema/introspection;
2. source-bound noise likelihood — research-only;
3. Evidence-Weighted Local Radiance Reconstruction + Observation-Bound Continuous Scientific Field — research-only with genuine CFA hold-outs;
4. optical-support/SFR binding only where evidence permits;
5. declarative Natural Appearance Graph while Scientific Master remains invariant;
6. Free Raster as one final arbitrary-resolution sampling interface;
7. Free World/3D authority expansion only from separately sealed/calibrated evidence.

This order is a **broad scientific roadmap**, not permission to discard or silently bypass the newer active PR #131 Appearance candidates already recorded in the canonical capsule.

---

## 9. Detailed documents

This supplement binds these detailed records into the 44489 recovery set:

- `docs/handoff/DRAW_44489_INTERDISCIPLINARY_OBSERVATION_AUDIT_2026-10-10.md`;
- `docs/handoff/DRAW_44489_SOURCE_CAPABILITY_EVIDENCE_LEDGER_V0_2_DESIGN_2026-10-10.md`;
- `docs/handoff/DRAW_44489_HIGH_ISO_CONTINUOUS_FIELD_RESEARCH_ROADMAP_2026-10-10.md`.

Knowledge anchors used include Adobe DNG resources, EMVA 1288, ISO 12233:2024, CIECAM16, Physically Based Rendering radiometry/light transport and Hartley-Zisserman multiple-view/projective geometry. They inform definitions/tests; they do not create observation-specific evidence.

---

## 10. Non-authorizations

Nothing in this supplement authorizes:

- sealed-source/CFA mutation;
- Scientific-Master writeback;
- current candidate promotion;
- PR #131 merge;
- vendor/model-name calibration truth;
- generic RGB blur as scientific denoise;
- AI/ML/generative scientific inference;
- semantic screen/object recolouring;
- inferred spectral truth from three channels alone;
- invented metric 3D/material/light;
- output raster density as measured optical resolution.

All permanent evidence laws and all newer canonical capsule checkpoints remain in force.
