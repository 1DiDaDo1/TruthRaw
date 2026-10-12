# D.RAW / TruthRaw — 44489 MASTER CAPSULE — CURRENT

Continuation code: **44489**  
Capsule role: **primary readable new-chat handoff and navigation layer**  
Current scientific/runtime integration anchor before this documentation reorganisation: `96be8a23c0d43f5dec304bdeb635824868cc80a7`  
Current audit branch: `audit/source-capability-raw-to-linear-contract-v01`  
Date: **2026-10-12**

> **One Free World. Many sealed observations. One evidence law.**

This file is the first document a new chat should read after receiving `44489`.

It does **not** delete, supersede or rewrite historical handoffs. Old capsules remain provenance. This file restores breathing room by separating:

- what is current now;
- what is binding science/governance;
- what is already completed;
- what is still open;
- what older plans remain relevant later;
- where exact historical evidence lives.

Documentation-only commits may advance the branch beyond `96be8a23...`. A later documentation SHA is **not** a new scientific/runtime integration anchor by itself. Always distinguish commit SHA from tree SHA; `29ac59da6c9af3362c2a01c3d57464f2a5012811` is the tree of `96be8a23...`, not a later commit.

---

# 0. READ THIS FIRST — CURRENT TRUTH

## Current governing research audit

The broad binding research/architecture audit is:

`docs/handoff/DRAW_44489_INTERDISCIPLINARY_OBSERVATION_AUDIT_2026-10-10.md`

Its central question remains:

1. What can physically be learned from the sealed observation?
2. What may only be added downstream as natural human-facing Appearance?

The project must never use visual plausibility as a substitute for stronger evidence.

## Current execution step — do this before older plans

PR #135 has been fully integrated at:

`96be8a23c0d43f5dec304bdeb635824868cc80a7`

From that exact integration anchor a read-only audit branch exists:

`audit/source-capability-raw-to-linear-contract-v01`

The active task is now:

**audit and, only after the audit boundary is accepted, optionally implement one read-only `SourceCapabilityRadiometricRawToLinearContractValidatorV01` that composes existing validator results in DNG order without reading or changing RAW image samples.**

Required normative ordering:

`Linearization -> Black Subtraction -> Rescaling / Normalization -> Clipping`

Exact current audit document:

`docs/handoff/DRAW_44489_RAW_TO_LINEAR_CONTRACT_AUDIT_V0_1_2026-10-12.md`

Hard boundaries for this active step:

- no second parser;
- no second source read or hash path;
- no parser rename of tag 50712;
- no ledger canonicalization change;
- no pixel implementation;
- no LinearizationTable application to image samples;
- no black subtraction/normalization/clipping changes to image samples;
- no Scientific Master or Appearance effect;
- no admission/use promotion;
- `full_raw_to_linear_mapping_validated=false` remains locked;
- `VALIDATED != ADMITTED != USED`.

The current Source Capability layer already has separate validated/read-only contracts for LinearizationTable metadata and Effective Black / Normalization metadata. What is missing is the combined ordering/composition contract. This does **not** mean clipping is absent everywhere in D.RAW; it means that the current Source Capability radiometric contract layer has not yet proven the complete ordered raw-to-linear composition as one source-bound contract.

## PR #135 / LinearizationTable state that must survive unchanged

- PR #135 merge commit: `96be8a23c0d43f5dec304bdeb635824868cc80a7`
- parser physical semantic key: `TAG_50712`
- lifecycle for the locked explicit fixture: `PARSED`
- admission: `UNSUPPORTED_SEMANTICS`
- authority: `UNKNOWN_SEMANTICS`
- consumers: zero
- `ledger_digest_sha256 = 6740e6e04c18797e8b57604f30c1884c6e8428e9a469e53cb139c26abe59afb3`
- `observation_evidence_digest_sha256 = 2bdaa44fe3435929310b79fa38d6ce3b0aa0cc05ef68f9522b4554c6862b38e6`
- `linearization_applied_to_image_samples=false`
- `table_application_to_image_samples_validated=false`
- `full_raw_to_linear_mapping_validated=false`
- `scientific_admission_effect=false`
- `scientific_master_pixel_effect=false`
- `appearance_pixel_effect=false`
- `source_values_modified=false`
- `creates_new_evidence=false`
- `scientific_writeback_allowed=false`

---

<details open>
<summary><strong>1. Permanent Evidence Law — never weaken this</strong></summary>

## Fundamental classes

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

These labels are not cosmetic. They describe different epistemic authority.

## Permanent invariants

- Direct CFA / RAW_SENSOR evidence is immutable and sealed.
- One physical frame remains one physical frame; processing never fabricates another physical observation.
- Scientific Master remains separate from View / Appearance / Export.
- `UNKNOWN` is valid and may not silently become zero, black, white, certainty or guessed colour.
- Unknown covariance is not zero covariance.
- `CENSORED` finite values are not automatically trustworthy scene radiance/chromaticity/material reflectance.
- More bits, more pixels, interpolation, reconstruction, registration, visual similarity, export quality, speed or AI confidence do not create scientific authority.
- Camera/lens/vendor/container identity may route parsing or describe provenance; identity alone may not select scientific truth.
- AI/ML/neural/generative inference is excluded from scientific evidence authority.
- SOURCE/SENSOR, WORLD/SCENE and VIEW/OUTPUT remain distinct.
- Output cannot authorize Scientific Master writeback.
- APK/GCam/computational RAW may be product/reference behavior, never TruthRaw evidence/calibration/promotion authority.
- DNG defaults can define specification dependencies, but absent tags do not become physical source occurrences.
- Parser success proves parseability only.
- Validator success proves only its named contract.
- `VALIDATED != ADMITTED != USED`; `USED` requires explicit admitted authority and a named consumer.
- A 3x3 camera colour transform is not exact human colorimetry for arbitrary spectra.
- Emitted radiance and reflected/transmitted radiance remain physically distinct.
- Free Raster density does not create measured detail.
- Cross-observation fusion requires a proven common gauge/registration/authority relation.
- Inferred geometry, material or illumination may not silently become `MEASURED`.

Project governance lines:

- **Seal the evidence, not the thinking.**
- **Representation may become richer than the source; knowledge claims may not exceed evidence.**
- **Stable outside. Flexible inside. Evidence law unchanged.**

</details>

---

<details open>
<summary><strong>2. Canonical architecture / cable map</strong></summary>

Current conceptual chain:

`Readable Source`
`-> Sealed Observation`
`-> Structural Inspection`
`-> Source Capability / Observation Evidence Ledger`
`-> Calibration + Reconstruction with explicit authority and uncertainty`
`-> Scientific Master`
`-> Dynamic Authority + uncertainty`
`-> observation-bound continuous scientific representation`
`-> Free World Observation Graph`
`-> Deep Scene / Light Transport`
`-> Room Capsule`
`-> View / Appearance`
`-> Unified Output`
`-> Free Raster / Export`

Canonical vision from the D.RAW “Lens to Free Raster” architecture:

1. Werkelijke Wereld
2. Lens & Sensor
3. Sealed Observation / D.RAW Observation Record
4. Kalibratie & Meting
5. Reconstructie / Scientific Master
6. continue wetenschappelijke negatieve representatie per observatie
7. Vrije Raster Projectie op vrije x,y-posities en resoluties
8. Vrije Wereld Observation Graph waarin meerdere observaties alleen als afzonderlijk bewezen bronnen worden gekoppeld
9. View / Appearance

Cross-cutting layers:

- Source Capability Envelope;
- Gauge & Calibration;
- Scientific State;
- Authority & Validation;
- generic PassArtifact architecture including Exact Gauge Retained Artifact v0.3;
- project governance / contracts / tests / CI / Android/platforms.

Important: this architecture is a vision and contract map. A box in the diagram is not proof that every runtime path has already been implemented or scientifically promoted.

### Output cable

`Input -> Universal Intake -> Scientific Core / Scientific Master -> PURE / ADVANCED / PRO -> Unified Output State -> Free Raster / Export`

Full-resolution presentation principle:

`admitted source/scientific cable -> full-resolution presentation render -> exact display-oriented RGB24 sibling -> { Free Raster backing artifact | High-Fidelity JPEG | future sibling outputs }`

Preview is never scientific authority and should not become the hidden master for 4K/8K export.

### Non-destructive workbench

`IMMUTABLE_SOURCE + REVERSIBLE_EDIT_STATE -> PRESENTATION / OUTPUT`

`EXPORT -> NEW_DERIVED_OUTPUT`

Never:

`SOURCE -> EDIT -> OVERWRITE SOURCE`

### T5 / Deep Scene / Room Capsule cable

Use the existing cable; do not build a duplicate:

`Scientific Master -> v0.4 Deep Scene Contribution -> v0.5 Deep Scene Binding -> v0.6 Light Transport -> existing Room Capsule host -> v0.7 Appearance Resolve`

No admitted geometry/material/illumination means exact-preserving bypass. T5 remains source-SHA/process-bound and fail-closed where authority is absent.

</details>

---

<details open>
<summary><strong>3. Source Capability — current contract inventory</strong></summary>

## Schema identities

Do not collapse these into one name:

- canonical historical Physical Knowledge Map: `D.RAW/SourceCapabilityEnvelope/0.2`
- runtime parser-bound evidence envelope: `D.RAW/SourceCapabilityRuntimeEvidenceEnvelope/0.1`
- physical occurrence/evidence ledger: `D.RAW/ObservationEvidenceLedger/0.1`

Schema separation migration: `e0527b374d6477a0b239125f7aa8d3f8750c5d32`.

The old runtime label `D.RAW/SourceCapabilityEnvelope/0.2` survives only as non-authoritative legacy provenance; it is not the same serialization as the canonical Physical Knowledge Map.

## Existing layers / checkpoints

### Physical occurrence/traversal provenance

Checkpoint: `ca8530404fed9fd02ce6b9e2d8d7fd30a638826e`

Already binds source SHA-256, byte length, IFD path/offset, tag/type/count, parser/decode status, entry/value offsets, inline/external storage, byte range, raw source value/offset bytes, occurrence index and traversal anomalies.

**Do not rebuild this parser/provenance layer.**

### Raw IFD domain

`SourceCapabilityRawIfdDomainValidatorV01`

Checkpoint: `02b611b246fb4db1ad3c3261ded7a7a484f7ae2f`

**Do not add a second Raw-IFD detector.**

### Radiometric semantic validator

`SourceCapabilityRadiometricSemanticValidatorV01`

Covers `SamplesPerPixel`, `BlackLevelRepeatDim`, `BlackLevel`, `WhiteLevel`.

Historical generated/runtime checkpoint: `1534d321f56a7f57b1b34e309282b1d9ead4579b`.

WhiteLevel is not a missing feature.

### Radiometric encoding consistency

`SourceCapabilityRadiometricEncodingConsistencyValidatorV01`

Validates `BitsPerSample`, `SampleFormat`, `SamplesPerPixel`, `PlanarConfiguration` and related domain consistency. It does not normalize pixels.

### DNG version compatibility

Exists as a separate validator. Do not merge its meaning into parser success.

### Spatial black-level dependencies

`SourceCapabilityRadiometricSpatialBlackLevelValidatorV01`

Validator checkpoint: `3d59441a113a35b0efbf33e2c20aff687f8dae76`

Tests: `6f9f0fea11ea2fb817720a9b408dfc0a143de399`

Exposure: `919272ef5ad2cad5985a74c455dc2ffbcb0ee1a0`

Validates `ImageWidth`, `ImageLength`, `ActiveArea`, `BlackLevelDeltaH/V`, defaults/fail-closed behavior; no pixel effect.

### Effective Black / Normalization

`SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01`

Source checkpoint: `fba30bc23e340d3a9d31c4b7f6eba7c904f03671`

Tests: `a0aa8d926dfabe11d9713a1fa144decf55538c1b`

Corrected green checkpoint: `bc93fe6693140262a52740b20d2e297b386b11f5`

Metadata relation:

`effective_black = BlackLevel[row mod repeatRows][col mod repeatCols][sample] + BlackLevelDeltaH[col] + BlackLevelDeltaV[row]`

Normalization dependency:

`normalization_denominator = WhiteLevel[sample] - max_effective_black[sample]`

Denominator must be finite and > 0. This validator does not read or normalize image samples.

Read-only runtime exposure was integrated by PR #133.

### LinearizationTable

`SourceCapabilityRadiometricLinearizationTableValidatorV01`

Schema: `D.RAW/SourceCapabilityRadiometricLinearizationTable/0.1`

Standalone status: `SHADOW_ONLY_LINEARIZATION_TABLE_METADATA_CONTRACT`

PR #134 merged; final PR head `c4474904ec9bd00552f1378fd50896dac0d7be61`; merge commit `f9547cfe9b04c732d5543d2b42cd4513e83ab9ab`.

Rules:

- consumes numeric tag 50712 from existing parsed entries;
- no second parser/source read/hash;
- absent table -> DNG identity default without physical occurrence;
- explicit table -> TIFF SHORT, count `2..65536`;
- explicit table + floating-point raw -> fail closed;
- explicit table + BitsPerSample >16 -> fail closed;
- count need not equal `2^BitsPerSample`;
- duplicate occurrence -> unresolved fail-closed;
- parser semantic rename is deliberately deferred.

Read-only runtime exposure was merged by PR #135 at `96be8a23...`.

### CURRENT missing layer

No combined Source Capability contract yet binds the existing dependencies into:

`Linearization -> Black Subtraction -> Rescaling / Normalization -> Clipping`

That is the active audit described at the top of this capsule.

</details>

---

<details open>
<summary><strong>4. Scientific foundation — grouped by discipline</strong></summary>

External knowledge improves contracts, models and tests. It never becomes source-specific evidence merely because it is true in general.

## Photography / RAW / CFA / sensor metrology

- Sensel values are measurements influenced by photon statistics, read/electronic noise, dark/fixed-pattern behavior, quantization, black reference, gain chain and saturation/censoring.
- Scientific denoise/reconstruction therefore belongs in an evidence-weighted estimation framework with explicit uncertainty, not generic RGB smoothing.
- CFA phase, sampling geometry and measured-channel identity matter; demosaicing/reconstruction cannot be treated as if every output channel were directly measured.
- BlackLevel, effective black, WhiteLevel, clipping and display black/white are different concepts.
- Source-contained calibration can be used only within its validated semantics/domain.

Primary metrology anchor: **EMVA 1288 Release 4.0**. Release 4.0 includes Linear and General modules and is useful for objective signal/noise characterization. It is methodology, not permission to invent unavailable camera parameters from one image.

https://www.emva.org/standards-technology/emva-1288/

## DNG / raw-to-linear semantics

Normative DNG anchor: **Adobe DNG Specification 1.7.1.0**.

Current Adobe resource page verified 2026-10-12 also lists DNG SDK **1.7.1 Build 2724, September 8 2026**. The SDK is an implementation cross-check; specification semantics still do not create D.RAW admission/use automatically.

https://www.adobe.com/support/downloads/dng/dng_sdk.html

## Optics / lens / PSF / OTF / MTF / sampling

- Pixel dimensions are not equal to independently resolved scene detail.
- Real resolution depends on lens, aperture/diffraction, aberrations, optical PSF/OTF/MTF, sensor aperture, sample lattice, CFA and processing.
- Resampling to a denser raster may improve representation/compatibility but cannot retroactively create measured spatial bandwidth.
- Sharpening should follow demonstrated optical/support structure rather than act as a generic truth-enhancing slider.

Current standard anchor: **ISO 12233:2024**, methods for measuring camera resolution and spatial frequency response.

https://www.iso.org/standard/88626.html

University grounding: Stanford CS348B material covers radiometry, cameras/film, sampling/reconstruction, aliasing, BRDFs and the rendering equation; Stanford EE367/CS448I covers computational imaging, digital cameras, denoising/deconvolution and inverse problems.

https://www.graphics.stanford.edu/courses/cs348b-02/
https://web.stanford.edu/class/ee367/

## Radiometry / photometry / physically based rendering / artificial light

- Emission and reflection/transmission are physically distinct.
- Light transport must keep emitted radiance separate from reflected/scattered incident radiance.
- Radiometric scene formation must remain separate from display appearance.
- Narrowband/LED/OLED spectra can create camera-vs-human observer mismatch; an RGB triplet is not a unique spectrum.
- Small emissive glyphs can interact with optical PSF, display subpixel geometry, sensor aperture and CFA sampling differently from large uniform fields.

The rendering equation, BRDF/BSDF concepts and spectral light transport are useful physical models; they do not make unsupported material/light parameters observable from one frame.

Useful technical anchor: PBRT 4th edition, plus Stanford CS348B.

https://www.pbr-book.org/4ed/contents

## Human vision / colour appearance / depth perception

- Human appearance depends on adaptation, luminance, surround, local contrast, colour relationships and viewing conditions.
- Scene-referred scientific colour formation and display-referred appearance must stay separate.
- Single-view depth has ambiguities; perspective, shading and other cues can support structure without uniquely determining metric world truth.

Current CIE appearance anchor: **CIE 248:2022 — CIECAM16**, a viewing-condition-specific colour appearance model.

https://www.cie.co.at/publications/cie-2016-colour-appearance-model-colour-management-systems-ciecam16

University grounding: MIT OpenCourseWare Sensory Systems includes dedicated material on **adaptation and color** and **depth perception**.

https://ocw.mit.edu/courses/9-04-sensory-systems-fall-2013/resources/lecture-videos/

## 3D / projective geometry / architecture

- Lines, planes, vanishing points, homographies and multiple-view constraints can provide strong geometric evidence.
- A single view does not normally determine unique metric depth, material, scale and illumination.
- Multi-observation world enrichment is permitted only when each observation remains separately sealed and the inter-observation relation is proven with uncertainty/authority.

University/book anchor: Oxford Visual Geometry Group, Hartley & Zisserman, **Multiple View Geometry in Computer Vision**.

https://www.robots.ox.ac.uk/~vgg/hzbook/

## Stop-motion / exposure integration / temporal aliasing / rolling shutter

- A geometrically static scene need not have time-constant radiance.
- Camera exposure integrates over time.
- PWM, LED drivers, display refresh/scanout and rolling shutter can alter measured colour/luminance depending on exposure duration/phase and row timing.
- Do not solve temporal/spectral phenomena using a semantic “screen” recolour detector.

Current CIE temporal-light anchor: **CIE 249:2022-Cor1**, released **2026-10-05**, which corrects and supersedes the original CIE 249:2022 publication regarding the SVM calculation ambiguity.

https://www.cie.co.at/publications/visual-aspects-time-modulated-lighting-systems-incl-corrigendum-1

## Restoration / conservation analogy

The sealed observation should be treated like an original conservation object:

- preserve the original;
- document intervention;
- minimize destructive intervention;
- keep provenance explicit;
- make reconstructions distinguishable from original evidence;
- prefer reversible/retreatable downstream work where practical.

This is governance inspiration, not imaging authority.

ICOMOS Venice Charter Article 16 strongly emphasizes precise documentation of preservation/restoration interventions.

https://www.icomos.org/charters-and-doctrinal-texts/

</details>

---

<details>
<summary><strong>5. Broader image-quality route map — still relevant after the current audit</strong></summary>

The older but still active long-horizon quality plan is subordinate to the current Source Capability raw-to-linear audit. Do not invert this order.

Central aim:

**MAXIMISE THE NATURAL, PHYSICALLY SUPPORTED SCENE ESTIMATE FROM THE SEALED OBSERVATION, WHILE KEEPING UNCERTAINTY AND AUTHORITY EXPLICIT.**

Guiding lines:

- **REMOVE NOISE BY BETTER EVIDENCE-BOUND ESTIMATION, NOT BY ERASING DETAIL.**
- **MAKE SHARPNESS FOLLOW PHYSICS, NOT A SHARPENING SLIDER.**
- **MAKE BLACK AND COLOUR NATURAL IN APPEARANCE, NOT BY REWRITING SCIENTIFIC TRUTH.**
- **LET FREE RASTER CHOOSE OUTPUT DENSITY WITHOUT PRETENDING DENSITY CREATES NEW EVIDENCE.**

Preferred long route:

`SEALED MEASUREMENT`
`-> MEASUREMENT LIKELIHOOD / UNCERTAINTY`
`-> EVIDENCE-WEIGHTED LOCAL SCENE ESTIMATE`
`-> OPTICAL SUPPORT`
`-> EXPLICIT AUTHORITY`
`-> NATURAL APPEARANCE`
`-> FREE RASTER`
`-> USER-SELECTED RESOLUTION`
`-> HIGH-FIDELITY EXPORT`

### Phase ordering

**P0 — scientific foundation**

- baseline/provenance audit;
- black/white semantic separation;
- observation-bound noise model;
- evidence-weighted local radiance reconstruction;
- true hold-out + uncertainty calibration;
- optical support.

**P1 — Natural Appearance**

- clean reconstruction + natural tone/colour;
- avoid plastic/oversharpened output;
- keep all appearance downstream.

**P2 — output density/export**

- real resolution selector;
- one-step resampling from the best admitted representation;
- high-fidelity JPEG/PNG/profile path.

**P3 — acceptance/governance/performance**

- automated regression;
- real-device acceptance;
- performance only after correctness;
- 44489 governance and handoff.

### Absolute do-not-do list

- no conventional blur filter as the primary scientific denoise;
- no generic AI denoiser as scientific reconstruction;
- no unsharp-mask core detail solution;
- no promotion of UNKNOWN because the image looks prettier;
- no camera model identity as scientific authority;
- no rewriting Zero-Line/BlackLevel for prettier blacks;
- no Appearance baked into Scientific Master;
- no preview bitmap as the 4K/8K scientific/export source;
- no duplicate full-resolution renderer when a sibling output can reuse the same render state;
- no silent 4:2:0-only master;
- no claim that denser raster equals more measured detail;
- no hidden uncertainty.

</details>

---

<details>
<summary><strong>6. Reconstruction research state</strong></summary>

## Anchor-Constrained Local Reconstruction v0.1 — NOT PROMOTED

Real CFA hold-out result:

- total true hold-outs: `21,760`
- affine solver valid: `21,760/21,760`
- baseline valid: `19,560/21,760`
- directly comparable: `19,560`
- affine MAE ~`0.00154976`
- baseline MAE ~`0.00147486`
- affine RMSE ~`0.00205484`
- baseline RMSE ~`0.00190590`
- affine bias ~`-0.00049664`
- baseline bias ~`-0.00009471`
- affine wins `9,363`
- baseline wins `10,197`
- affine supplied `2,200` extra valid estimates
- channel 2 slightly better under affine
- channel 1 clearly worse under affine
- uncertainty 1σ/2σ/3σ coverage ~`47.69% / 80.19% / 93.54%`
- max combined z ~`16.29`

Safety remained intact:

- target not used by solver;
- measured anchors unchanged;
- solver not applied to Scientific Master;
- candidate not applied;
- scientific writeback forbidden.

Conclusion:

The experiment was scientifically useful, but the solver is **not** promoted.

Next preferred reconstruction direction:

A deterministic local model bank selected from target-excluded structural support, CFA phase, direction and uncertainty, where affine plane is only one candidate. `NO_SUITABLE_MODEL` is a valid outcome.

</details>

---

<details>
<summary><strong>7. Performance / Exact Gauge Retained Artifact</strong></summary>

Exact Gauge Retained Artifact v0.3 is an execution/performance mechanism, not a new estimator.

Core meaning:

- canonical fallback remains available;
- exact eligible Float32 bits may be retained to avoid a second Stage-2 reread;
- no semantic authority change;
- diagnostics bind Scientific-Master SHA;
- timing alone is never authority;
- `candidate_applied=false`;
- `source_values_modified=false`;
- `creates_new_evidence=false`;
- `scientific_writeback_allowed=false`.

Real-device evidence established four sources with one Stage-2 gauge pass and **3072 avoided second-pass tile reads per source**.

Historical telemetry-repair runtime checkpoint:

`6a8e01763ca8eb0f6d371c1588a7fbeb91f0af50`

Historical frozen scientific/audit reference remains:

PR #130 exact checkpoint `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`

At that exact historical head:

- Documentation Governance = proven SUCCESS;
- Research Integrity Guard = no proven exact-head run -> `NOT RUN / UNPROVEN`;
- Lifecycle Contract = no proven exact-head run -> `NOT RUN / UNPROVEN`.

Later live PR #130 advanced because work merged into its branch and is now closed superseded/unmerged. Never call the later live PR itself the frozen reference; only the exact checkpoint above is frozen.

</details>

---

<details>
<summary><strong>8. Product/output and image-quality checkpoints</strong></summary>

## PR #131 / Workspace / Free Raster

PR #131 final head: `c28b8bacfead4787fdc14cd65e9e00ff4bc7abb8`

Merged into integration branch `fix/android-exact-gauge-pass-artifact-v03` on 2026-10-11.

Merge commit: `28343adf054ccb8f4c741edff187604cf2b0c88a`

This did not itself advance `main`.

## High-Fidelity JPEG accepted path

Accepted runtime: `f42cd79ef5060e78c1c6e6bcc1e7d9d94a07d588`

Regression lock: `e4742bf048a160b65398d93461f9fa3799faa638`

Accepted APK SHA-256:

`4c6a794b252a43a6bf38405b053e667738e779c0fe014bdbe02713765cfdeb22`

Accepted tele/PURE JPEG:

`DRAW_CAPTURE_1791409962721_tele_4080x3072_draw_pure_fullres.jpg`

SHA-256:

`7d37016b7dbf6277ce5e196ec778bb33aa927f78009dd07c49b2891f14cd47ff`

Properties:

- 4080x3072;
- baseline SOF0;
- 8-bit;
- true 4:4:4;
- Q100;
- FFmpeg/FFprobe/Pillow/ImageMagick accepted;
- Lightroom compatibility user-confirmed.

Still separate/pending from that acceptance:

- broader arbitrary Original/1080p/4K/8K/Custom scaling;
- truthful ICC/profile strategy;
- PNG/lossless/high-bit-depth sibling output;
- broader sources/devices where not separately accepted.

## Natural Appearance checkpoints

### Warm Illuminant Retention v0.1

Real-device PASS runtime:

`8ad695bad5be4fbc799fa781d401124e466ca48e`

ADVANCED/PRO only, PURE bypass, bounded warm retention, no source/master mutation.

### Natural Light Local Field Tone v0.1

Checkpoint:

`0a489190c474fa765431064513c0f6e52f7b63fd`

Downstream perceived luminous-field local brightness impression only; not reconstructed physical light transport, not WB/glow. Uses rendered luminance context, excludes censored support, common RGB gain, protected black/near-white, bounded approximately +0.14 EV.

### Deep-Censor Chroma Guard v0.2

Checkpoint:

`0db27c2cfc6eab768d41dcdbb1014b43311bb5a2`

APK SHA:

`32ec63317e9cbdfecb353b1b98a30ef0770a742370da44503a7132278b454756`

`APPEARANCE_ONLY`; bounded chroma relief/contraction behavior; preserves luminance target/chroma direction; PURE bypass; no evidence effect.

### Censored Illuminant Hue Floor v0.1

Workflow `38073401245` SUCCESS.

APK SHA:

`abec7589b9da32a1fbc2ff0674c8f83db93440274a00d2ffc3777cc92e68e85b`

Warm-highlight candidate; CI-green but physical warm-highlight acceptance remains separate unless later exact evidence proves it.

### Purple/magenta tele highlight problem

The former broad purple/magenta highlight problem is materially solved and is **not** the current upstream task. Do not reopen it without a new real regression.

### Yellow TV/subtitle/emissive-display problem

Still open research. Must consider:

- emitted spectrum vs reflected light;
- camera spectral sensitivity vs human observer;
- metameric mismatch;
- RGB subpixel lattice;
- lens PSF;
- sensor aperture;
- CFA/reconstruction;
- PWM/refresh/scanout;
- exposure duration/phase;
- rolling shutter / temporal aliasing.

Do not use a semantic yellow/screen recolour patch or weaken evidence authority.

</details>

---

<details>
<summary><strong>9. Ordered project history / major transitions</strong></summary>

This is a navigation chronology, not a replacement for the preserved handoff documents.

### 2026-09-10 — formal sealed handoff era

Formal transfer artifact established with immutable/sealed Direct-CFA evidence law and strict separation of measured/reconstructed/appearance.

### 2026-09-16 to 2026-09-24 — scientific architecture and Camera5/TruthNegative foundations

Core scientific architecture, RAW route, code freeze and detailed next-chat handoffs were established. These files remain historical provenance under `docs/handoff/TRUTHRAW_*` and state documents.

### 2026-09-25 to 2026-09-28 — D.RAW naming, knowledge growth, Free World realignment

Project governance and Free World Observation Graph direction became explicit. Observation-bound truth, uncertainty and world-space authority were formalized further.

### 2026-09-29 — Anchor-Constrained Local Reconstruction real-device hold-out

Experiment completed; affine candidate not promoted because baseline was better overall and uncertainty too optimistic. This remains an important negative scientific result.

### 2026-10-01 to 2026-10-03 — Exact Gauge / performance and continuous-field groundwork

Performance optimization proceeded while preserving byte/semantic authority. Exact Gauge Retained Artifact v0.3 became a performance artifact, not a truth estimator. Free World/optical/measured-field support research continued.

### 2026-10-04 to 2026-10-05 — T5 / Deep Scene / Room Capsule and CI audit

Existing T5/Deep Scene/Room Capsule cable was traced and bound rather than duplicated. Historical PR #130 exact-head CI interpretation was corrected: absence of runs is not failure.

### 2026-10-05 to 2026-10-08 — Workspace / Free Raster / high-fidelity output

Free Raster and preview-independent full-resolution output were productized downstream. High-Fidelity JPEG real-device acceptance established Q100 true 4:4:4 output without turning presentation into scientific authority.

### 2026-10-09 to 2026-10-10 — Natural Appearance and emissive-source research

Warm illuminant, local field tone and censored-highlight Appearance work advanced. Purple highlight issue became materially solved. Emissive-display/yellow subtitle issue was separated into spectral/spatial/temporal research rather than semantic recolouring.

### 2026-10-10 — Interdisciplinary Observation Audit

Binding audit established the present broad research direction: richer source-bound evidence harvesting, noise likelihood, evidence-weighted local reconstruction, optical support, continuous scientific field, Natural Appearance graph, Free Raster as sampling interface and cautious multi-observation world enrichment.

### 2026-10-11 — Source Capability contract expansion

- PR #132 merged: runtime Source Capability evidence-envelope schema separation.
- PR #133 merged: Effective Black / Normalization read-only runtime diagnostic.
- PR #134 merged: LinearizationTable shadow validator.
- PR #135 merged: LinearizationTable read-only runtime diagnostic.

Current exact integration anchor after PR #135:

`96be8a23c0d43f5dec304bdeb635824868cc80a7`

### 2026-10-12 — current capsule + raw-to-linear audit

The project now pauses before any sample-domain Linearization/Black/Normalization/Clipping implementation and first audits the missing combined Source Capability ordering/composition contract.

</details>

---

<details>
<summary><strong>10. Older step plans — status-labelled so they cannot outrank the current step</strong></summary>

## STILL RELEVANT LATER

- Source-bound noise likelihood.
- Deterministic target-excluded local model selection.
- Continuous/adaptive observation-bound scientific image field.
- Optical-support / local spatial-frequency authority.
- Declarative Natural Appearance Graph.
- Arbitrary Free Raster output density from the best admitted representation.
- PNG/lossless/high-bit-depth sibling export.
- Truthful colour-profile/ICC strategy.
- Multi-observation Free World integration only after proven gauge/registration/uncertainty.
- Emissive-display spectral/spatial/temporal test matrix.
- Broader real-device/source acceptance.

## COMPLETED / INTEGRATED BOUNDARIES

- PR #131 Workspace/Free Raster product line merged into integration branch.
- PR #132 runtime Source Capability schema separation.
- PR #133 Effective Black read-only runtime diagnostic.
- PR #134 LinearizationTable shadow validator.
- PR #135 LinearizationTable read-only runtime diagnostic.
- High-Fidelity JPEG Q100 4:4:4 acceptance on the accepted tele/PURE path.
- Historical Exact Gauge v0.3 performance proof.

## COMPLETED SCIENTIFIC EXPERIMENT — NOT PROMOTED

- Anchor-Constrained Local Reconstruction affine candidate.

## HISTORICAL / SUPERSEDED AS “NEXT STEP” BUT PRESERVED AS PROVENANCE

- old PR #131 draft-era route-mismatch-first bootstrap sequence;
- old “merge PR #109 then test N2_LOCAL_SPATIAL” sequence;
- earlier PR #130 live-head assumptions;
- earlier capsule instructions that treat PR #131 as still draft/open;
- older performance micro-optimization sequences that were overtaken by later integration.

These files must not be deleted; they remain evidence of project evolution.

## PAUSED / OPEN RESEARCH

- complete sample-domain raw-to-linear validation;
- Source Capability clipping composition dependency;
- full source-bound noise likelihood;
- promoted reconstruction model beyond baseline;
- optical support authority at local frequencies;
- continuous scientific field implementation;
- emissive display/yellow subtitle issue;
- general arbitrary output size/profile/lossless export;
- broader multi-observation world authority.

</details>

---

<details>
<summary><strong>11. Checkpoint / PR / hash quick index</strong></summary>

| Meaning | Reference |
|---|---|
| Historical frozen PR #130 audit checkpoint | `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f` |
| PR #131 final head | `c28b8bacfead4787fdc14cd65e9e00ff4bc7abb8` |
| PR #131 integration merge | `28343adf054ccb8f4c741edff187604cf2b0c88a` |
| Source Capability schema migration | `e0527b374d6477a0b239125f7aa8d3f8750c5d32` |
| Physical occurrence provenance checkpoint | `ca8530404fed9fd02ce6b9e2d8d7fd30a638826e` |
| Raw-IFD validator | `02b611b246fb4db1ad3c3261ded7a7a484f7ae2f` |
| Spatial black validator | `3d59441a113a35b0efbf33e2c20aff687f8dae76` |
| Effective Black corrected green | `bc93fe6693140262a52740b20d2e297b386b11f5` |
| PR #134 Linearization merge | `f9547cfe9b04c732d5543d2b42cd4513e83ab9ab` |
| PR #135 merge / current scientific integration anchor | `96be8a23c0d43f5dec304bdeb635824868cc80a7` |
| PR #135 commit tree — NOT a commit | `29ac59da6c9af3362c2a01c3d57464f2a5012811` |
| Linearization locked ledger digest | `6740e6e04c18797e8b57604f30c1884c6e8428e9a469e53cb139c26abe59afb3` |
| Linearization locked observation digest | `2bdaa44fe3435929310b79fa38d6ce3b0aa0cc05ef68f9522b4554c6862b38e6` |
| Accepted high-fidelity JPEG runtime | `f42cd79ef5060e78c1c6e6bcc1e7d9d94a07d588` |
| High-fidelity JPEG regression lock | `e4742bf048a160b65398d93461f9fa3799faa638` |
| Warm Illuminant runtime PASS | `8ad695bad5be4fbc799fa781d401124e466ca48e` |
| Natural Light Local Field Tone checkpoint | `0a489190c474fa765431064513c0f6e52f7b63fd` |
| Deep-Censor Chroma Guard v0.2 checkpoint | `0db27c2cfc6eab768d41dcdbb1014b43311bb5a2` |

</details>

---

<details open>
<summary><strong>12. New-chat protocol</strong></summary>

When a new chat receives only `44489`, it should do this in order:

1. Read this file completely: `docs/handoff/DRAW_44489_MASTER_CAPSULE_CURRENT.md`.
2. Resolve the live head of `audit/source-capability-raw-to-linear-contract-v01`; do not assume a remembered documentation SHA is still current.
3. Treat `96be8a23c0d43f5dec304bdeb635824868cc80a7` as the exact current **scientific/runtime integration anchor** unless a later explicitly validated runtime/scientific integration replaces it.
4. Read `docs/handoff/DRAW_44489_RAW_TO_LINEAR_CONTRACT_AUDIT_V0_1_2026-10-12.md` before touching code.
5. Read `docs/handoff/DRAW_44489_INTERDISCIPLINARY_OBSERVATION_AUDIT_2026-10-10.md` for the broad binding research direction.
6. For Source Capability work, inspect existing validators and handoffs before inventing a new parser/validator path.
7. Preserve `VALIDATED != ADMITTED != USED` and all explicit false pixel/writeback flags.
8. If the task moves back to reconstruction, output, T5, Appearance or Free World, use the corresponding sections above and then open the exact historical handoff named there.
9. Refresh external scientific knowledge against current primary/standards/university sources when a decision depends on it.
10. Never delete older capsules simply because this master exists; they are historical provenance.

### Immediate next repository action after this capsule

If continuing the active scientific work, inspect the existing Source Capability validator APIs and create **one isolated read-only composition validator** only if it can be built entirely from existing source-bound reports while preserving parser/ledger/pixel/master/appearance identity.

No pixel-domain raw-to-linear implementation is authorized by this capsule.

</details>

---

# 13. Historical document pointers

The following are especially important preserved handoffs. This is not exhaustive; the repository remains the provenance archive.

- `docs/handoff/DRAW_44489_KNOWLEDGE_CAPSULE.md`
- `docs/handoff/DRAW_44489_KNOWLEDGE_CAPSULE_INTERDISCIPLINARY_SUPPLEMENT_2026-10-10.md`
- `docs/handoff/DRAW_44489_INTERDISCIPLINARY_OBSERVATION_AUDIT_2026-10-10.md`
- `docs/handoff/DRAW_44489_SOURCE_CAPABILITY_EVIDENCE_LEDGER_V0_2_DESIGN_2026-10-10.md`
- `docs/handoff/DRAW_44489_SOURCE_CAPABILITY_OCCURRENCE_LEDGER_V0_1_CHECKPOINT_2026-10-11.md`
- `docs/handoff/DRAW_44489_RAW_IFD_FORMAT_DOMAIN_VALIDATION_V0_1_CHECKPOINT_2026-10-11.md`
- `docs/handoff/DRAW_44489_RADIOMETRIC_ENCODING_CONSISTENCY_V0_1_CHECKPOINT_2026-10-11.md`
- `docs/handoff/DRAW_44489_DNG_RADIOMETRIC_SEMANTIC_VALIDATION_V0_1_CHECKPOINT_2026-10-11.md`
- `docs/handoff/DRAW_44489_DNG_VERSION_COMPATIBILITY_V0_1_CHECKPOINT_2026-10-11.md`
- `docs/handoff/DRAW_44489_EFFECTIVE_BLACK_RUNTIME_DIAGNOSTIC_V0_1.md`
- `docs/handoff/DRAW_44489_LINEARIZATION_TABLE_AUDIT_V0_1_2026-10-11.md`
- `docs/handoff/DRAW_44489_LINEARIZATION_TABLE_RUNTIME_DIAGNOSTIC_V0_1_2026-10-11.md`
- `docs/handoff/DRAW_44489_HIGH_ISO_CONTINUOUS_FIELD_RESEARCH_ROADMAP_2026-10-10.md`
- `docs/handoff/DRAW_44489_EMISSIVE_DISPLAY_SPECTRAL_SPATIOTEMPORAL_RESEARCH_2026-10-10.md`
- `docs/handoff/DRAW_44489_HIGH_FIDELITY_JPEG_REAL_DEVICE_PASS_2026-10-08.md`
- `docs/handoff/DRAW_44489_WARM_ILLUMINANT_REAL_DEVICE_PASS_2026-10-09.md`
- `docs/handoff/DRAW_44489_PURPLE_HIGHLIGHT_CAUSAL_TRACE_2026-10-09.md`
- `docs/handoff/DRAW_44489_HIGHLIGHT_DEFAULT_AND_APPEARANCE_REFERENCES_2026-10-10.md`
- `docs/handoff/DRAW_44489_WORKSPACE_OUTPUT_FREE_RASTER_V0_2.md`
- `docs/handoff/DRAW_44489_T5_RUNTIME_AUDIT_READY_2026-10-05.md`
- `docs/handoff/DRAW_44489_RECOVERY_2026-10-03.md`
- `docs/CORE_VISION_LENS_INDEPENDENT_FREE_WORLD_OBSERVATION_ARCHITECTURE.md`
- `docs/CORE_VISION_TRUTHNEGATIVE_SCIENTIFIC_NEGATIVE.md`
- `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
- `docs/DRAW_INTERDISCIPLINARY_SCIENTIFIC_DECISION_FOUNDATION_v0_1.md`
- `docs/DRAW_INTERDISCIPLINARY_DECISION_MATRIX_v0_2.md`

Older `TRUTHRAW_*`, `DRAW_NEXT_CHAT_*`, project-state JSON files and document-status indexes remain historical provenance and may be consulted when a particular checkpoint must be reconstructed.

---

# Final boundary

The current project is **not** waiting for another appearance heuristic, renderer rewrite or generic denoiser.

The active frontier is narrower and more fundamental:

**prove the Source Capability raw-to-linear dependency composition before allowing any claim that the complete RAW sample mapping is understood or used.**

Until then:

`sealed observation -> read-only evidence contracts -> explicit UNKNOWN where necessary`

remains stronger science than silently filling the missing cable with assumptions.
