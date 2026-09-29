# D.RAW — ACTIVE RESEARCH OVERLAY — OBSERVATION OPTICAL FIELD CHART v0.1 — 2026-09-29

**Current optical-field branch:** `research/observation-optical-field-chart-v01-2026-09-29`

Read first:

1. `state/OBSERVATION_OPTICAL_FIELD_CHART_STATE_2026-09-29.json`
2. `docs/research/observation-optical-field-chart-v0.1/README.md`
3. v0.3 selector remains frozen on PR #95 and is not modified by this branch.

Purpose:

- implement the user's "flat lens" idea as a deterministic flat field-coordinate chart;
- source/active-area positions are mapped to rho + azimuth + radial/tangential basis;
- geometric center is a declared coordinate origin only, not a proven physical optical axis;
- existing measured CFA samples are grouped into 12 radial annuli and 12 azimuth sectors;
- per-annulus CFA-phase medians and azimuthal dispersion are recorded;
- observed outer/inner signal falloff is descriptive scene+lens+sensor evidence, not automatically lens vignetting;
- no cos^4 model, lens profile, camera model or vendor map is assumed;
- DNG OpcodeList2 headers may be reported as provenance hints; GainMap payloads are not interpreted/applied and cannot calibrate truth;
- no correction gain, measured-sample mutation or Scientific Master writeback is allowed.

Current measured topology is inherited from BacksideSignalSupportAudit: classic TIFF/DNG, uncompressed unsigned 16-bit, single sample/pixel, strips. Unsupported layouts fail closed for the measured field signal while the geometry chart can still exist when raster geometry is known.

Next gate: green Android build, then inspect a real-device radial/azimuthal field profile without applying any correction.

---

# D.RAW — ACTIVE RESEARCH OVERLAY — UNIVERSAL LOCAL MODEL BANK v0.3 — 2026-09-29

**Current stacked successor:** `research/universal-local-model-bank-holdout-v03-2026-09-29`

Read first:

1. `state/UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_STATE_2026-09-29_V03.json`
2. `state/UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_V03_INDEPENDENT_RESULT_2026-09-29.json`
3. `docs/research/universal-local-model-bank-holdout-v0.3/README.md`
4. `docs/research/universal-local-model-bank-holdout-v0.3/DEVICE_VALIDATION_PROTOCOL_2026-09-29.md`
5. `state/UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_V02_PAIRED_RESULT_2026-09-29.json`
6. v0.2 remains the frozen candidate-geometry predecessor.


First independent v0.3 paired device result is now available on `IMG_20260403_182145.dng`:

- original DNG byte SHA-256 `999216c4dfbb21180381a7621ef0d32e5faa16d1fcd2049f0185302760fa28ef` exactly matches v0.1/v0.2/v0.3 `source_sha256`;
- all three versions used the exact same 11,844 holdout coordinates, targets and baseline predictions;
- v0.2/v0.3 candidate estimates, support geometry and validation RMS values are identical; this is a selector-only comparison;
- all-holdout MAE: v0.1 ≈ 0.00554706, v0.2 ≈ 0.00570195, v0.3 ≈ 0.00552648;
- all-holdout RMSE: v0.1 ≈ 0.01008783, v0.2 ≈ 0.01039450, v0.3 ≈ 0.00995411;
- v0.3 improves MAE ≈ 3.08% and RMSE ≈ 4.24% versus v0.2;
- v0.3 is numerically ≈ 0.37% lower MAE than v0.1 over all holdouts, but the 512x512 block-bootstrap interval for v0.3-v0.1 crosses zero;
- on the 11,012 reference-comparable holdouts, v0.1 MAE ≈ 0.00487253, v0.3 ≈ 0.00487868, reference ≈ 0.00494619;
- v0.3 selector regret improves materially versus v0.2 but remains worse than v0.1;
- conclusion: v0.3 selector fix successfully recovers the v0.2 regression, but superiority over v0.1 is not established; no promotion/writeback.

Why v0.3 exists:

- v0.2 improved candidate-bank capacity but its selector regressed;
- v0.3 freezes every v0.2 candidate fit and direction choice;
- cross-family selection now uses only common target-blind validation RMS;
- no second BIC/AIC/parameter-count penalty is applied;
- model complexity is only a deterministic numerical tie-break;
- each holdout reports second-best validation RMS and selection margin;
- source raster remains full-resolution MEASURED support, never world-resolution authority;
- 20-bit raster-independent lattice remains the scientific solution domain;
- no lens/camera/vendor routing, no AI/ML, no writeback.

Retrospective development replay on the already-observed paired source predicts v0.3 MAE ≈ 0.00442126 vs v0.1 ≈ 0.00446583 and v0.2 ≈ 0.00460044. This is development evidence only, not validation.

New APK test export:

`Export Universal Local Model Bank Holdout v0.3 · JSON`

Green v0.3 Android build checkpoint:

- code SHA: `57754bf573bf964d042964843f080282d23d9cab`;
- D.RAW Android DngCreator Compatibility run: `36620946032` — SUCCESS;
- artifact: `11058269102`;
- artifact digest: `sha256:9e599ffa625f92a3c5443ce7714cbe529f8cf5673eff13725503ed3ccf81fa5b`;
- APK SHA-256: `fb08d089b71cf367106fae798703e36ba108a947a759d68f1fbeb31ef05a7752`;
- an earlier Universal Intake retry failed only because the Android SDK CMake 3.22.1 download arrived as an invalid archive; this was external to the code.

Validation law: use one NEW, previously unscored sealed RAW/DNG and export v0.1, v0.2 and v0.3 on that exact source. Holdout coordinates must match. v0.2/v0.3 per-model estimates must be bit-identical; only selected_model may differ.

---

# D.RAW — ACTIVE RESEARCH OVERLAY — UNIVERSAL LOCAL MODEL BANK v0.2 — 2026-09-29

**Current stacked successor:** `research/universal-local-model-bank-holdout-v02-2026-09-29`

Read first:

1. `state/UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_STATE_2026-09-29_V02.json`
2. `state/UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_V02_PAIRED_RESULT_2026-09-29.json`
3. `docs/research/universal-local-model-bank-holdout-v0.2/README.md`
4. `docs/research/universal-local-model-bank-holdout-v0.2/DEVICE_VALIDATION_PROTOCOL_2026-09-29.md`
5. `state/UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_REAL_DEVICE_RESULT_2026-09-29.json`
6. PR #92 remains the frozen v0.1 real-device predecessor.


Paired v0.1/v0.2 result now available on source `IMG_BNC_TRUTHRAW20260907_094414_423`:

- exact same source/scientific/TruthNegative hashes for v0.1 and v0.2;
- exact same 11,844 holdout coordinates and targets;
- v0.2 successfully removed directional/affine redundancy: 0/11,844 bit-identical center estimates;
- v0.1 selected MAE ≈ 0.00446583; v0.2 ≈ 0.00460044 over all holdouts;
- on 11,031 reference-comparable holdouts: v0.1 MAE ≈ 0.00386913, v0.2 ≈ 0.00399589, reference ≈ 0.00397154;
- v0.2 selector regret worsened, but v0.2 oracle MAE improved to ≈ 0.00338673 vs v0.1 oracle ≈ 0.00364386;
- conclusion: v0.2 candidate geometry improved, selector regressed; no promotion/writeback;
- original source was supplied afterwards as `IMG_BNC_TRUTHRAW20260907_094414_423.dng` (25,106,108 bytes); computed SHA-256 `578fad42dad6819b1d3f9a1f9cbfcc5c547b63ae01f3951f26dca988d655d10e` exactly matches the paired v0.1/v0.2 `source_sha256`, so byte-level source provenance is now closed.

Why v0.2 exists:

- PR #92 v0.1 beat the historical reference on its first independent tele test, but no promotion/writeback was allowed;
- selector regret remained material;
- v0.1 DIRECTIONAL_LINE and AFFINE_PLANE center predictions were identical on all 11,844 hold-outs;
- v0.2 therefore uses target-blind support cross-fit for model selection;
- v0.2 replaces DIRECTIONAL_LINE with DIRECTIONAL_STRIP_LINE using observation-derived directional support only;
- tele identity remains provenance only and is not a selector/calibration input;
- the source raster remains exact full-resolution MEASURED support, not world-resolution authority;
- the 20-bit sparse lattice remains the free scientific solution domain;
- target numeric Stage-2 value remains unread until selector and reference predictions are frozen;
- candidate_applied=false and scientific_writeback_allowed=false.

New APK test export:

`Export Universal Local Model Bank Holdout v0.2 · JSON`

Validation law: use one NEW sealed RAW/DNG and export both v0.1 and v0.2 on the same source. Their hold-out coordinates should match exactly; compare v0.2 directly against v0.1 and the same baseline.

---

# D.RAW — ACTIVE RESEARCH OVERLAY — UNIVERSAL LOCAL MODEL BANK — 2026-09-29

**Current stacked successor:** `research/universal-local-model-bank-holdout-v01-2026-09-29`

Read first:

1. `state/UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_STATE_2026-09-29.json`
2. `state/UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_REAL_DEVICE_RESULT_2026-09-29.json`
3. `docs/research/universal-local-model-bank-holdout-v0.1/README.md`
4. `docs/research/universal-local-model-bank-holdout-v0.1/DEVICE_VALIDATION_PROTOCOL_2026-09-29.md`
5. `state/UNIVERSAL_OBSERVATION_MODEL_SELECTION_STATE_2026-09-29.json`
6. `docs/research/universal-observation-model-selection-v0.1/README.md`
7. PR #90 real-device result remains negative evidence for universal affine promotion.


PR #92 first independent device result is now available on capture
`DRAW_CAPTURE_1790703829897_tele_4080x3072`:

- 11,844 full-resolution real CFA hold-outs, exactly 2,961 per CFA phase;
- selected model valid on 11,844/11,844; baseline valid on 11,667/11,844;
- directly comparable MAE: selected ≈ 0.02082264 vs baseline ≈ 0.02143966;
- directly comparable RMSE: selected ≈ 0.02706725 vs baseline ≈ 0.02784103;
- wins selected/baseline = 6,227 / 5,440;
- selected bias ≈ -0.00032525 vs baseline ≈ -0.00315449;
- post-reveal oracle MAE ≈ 0.01833255; mean selector regret ≈ 0.00247127;
- directional-line and affine-plane center estimates were exactly identical on all 11,844 hold-outs under the current symmetric 80-sample support, revealing a model-bank redundancy;
- all target-leakage/lens/camera/vendor/writeback safety flags stayed false;
- no promotion/writeback is allowed from this result.

Review-package limitation: all JSON sidecars share the same source/scientific/TruthNegative hashes, but the original sealed source RAW/DNG bytes were not included in the uploaded ZIP, so external byte-level source-SHA reverification remains pending.

New in this successor:

- original source resolution is used as full-resolution MEASURED support for CFA values, phase, detail/structure, geometry, censor paths and hold-out validation — not only noise;
- audit scope is a deterministic stratified set of real CFA anchors across the full source raster, not only Dark-Chroma regions;
- Dark-Chroma/N2 candidate state and the PR #91 query-policy are optional context, **not admission gates** for this full-resolution audit;
- the existing 20-bit fixed-point sample lattice remains the free scientific coordinate domain;
- model bank: robust median constant, directional line, affine plane, quadratic surface, NO_RECONSTRUCTION;
- model selection is frozen before target reveal;
- target value, target error and post-reveal oracle are forbidden selector inputs;
- the post-reveal oracle exists only to measure selector regret;
- model selection does not require NoiseProfile; the legacy baseline is simply unavailable where its own variance contract cannot be satisfied;
- no lens calibration, camera model, physical Camera2 ID or vendor map is a model-selection dependency;
- no model output is applied to Scientific Master/D.RAWnegative;
- new independent real-device RAW is required for scientific validation.

APK test export:

`Export Universal Local Model Bank Holdout v0.1 · JSON`

Permanent laws remain:

> The source raster determines where D.RAW measured. It does not determine the raster on which D.RAW must think.

> A new raster may refine the measurement space without replacing the measurement history.

---

# D.RAW — ACTIVE RESEARCH OVERLAY — 2026-09-29

**Current stacked research successor:** `research/universal-observation-model-selection-v01-2026-09-29`

Read first for this branch:

1. `state/UNIVERSAL_OBSERVATION_MODEL_SELECTION_STATE_2026-09-29.json`
2. `docs/research/universal-observation-model-selection-v0.1/README.md`
3. `state/ANCHOR_CONSTRAINED_LOCAL_RECONSTRUCTION_STATE_2026-09-29.json`
4. `docs/research/anchor-constrained-local-reconstruction-v0.1/README.md`
5. `state/RASTER_INDEPENDENT_SAMPLE_LATTICE_STATE_2026-09-29.json`

Latest proven PR #90 real-device evidence is no longer "pending": tele capture
`DRAW_CAPTURE_1790686814757_tele_4080x3072` produced 21,760 real CFA hold-outs.
The affine solver had complete coverage but was worse than the existing baseline on
the 19,560 directly comparable hold-outs (MAE ≈ 0.00154976 vs 0.00147486;
RMSE ≈ 0.00205484 vs 0.00190590). Affine is therefore **not promoted**.
Diagnostic uncertainty was too optimistic. Safety/writeback invariants remained intact.

The active successor is intentionally universal and observation-conditioned:

- no lens-specific noise calibration;
- no camera-model/device routing;
- no vendor lookup table;
- no AI/ML/neural/generative runtime;
- exact source CFA samples remain immutable MEASURED anchors;
- the original source resolution is full-resolution evidence support for values,
  structure, geometry, CFA phase, censoring and provenance — **not only noise**;
- the source raster is measurement sampling, not world-resolution authority;
- the existing 20-bit fixed-point sample lattice is the free scientific solution domain;
- every unanchored coordinate begins UNKNOWN;
- model-bank selection uses only pre-target observation geometry/support;
- held-out target values and hold-out errors are forbidden selector inputs;
- the already-seen 2026-09-29 tele hold-out is development evidence only;
- a new independent capture is required to validate the selector.

Green code-bearing PR #91 checkpoint:

- code SHA: `4f1b5c1e99a115fa4b63d49b02d1913908d5a8ec`
- D.RAW Suite Universal Intake run: `36593498025` — SUCCESS
- artifact: `11044977856`
- artifact digest: `sha256:c76bff4277b64914fdc2760af17fead6ebbcda2bba1b365e946a5fb4ded1a5e4`
- APK SHA-256: `bb303089805dc271da238d8a6ee41b1ed124f16e036deacb99976d6ce325c90d`

The APK exposes `Export Universal Observation Model Selection v0.1 · JSON`.
This selector sidecar is computed before the anchor hold-out audit and contains no
held-out target/error metrics. Export/freeze it before interpreting a new
independent hold-out result.

Permanent law for this line:

> The source raster determines where D.RAW measured. It does not determine the raster on which D.RAW must think.

And:

> A new raster may refine the measurement space without replacing the measurement history.

---

# D.RAW — CURRENT PROJECT IDENTITY — 2026-09-28

**Official product name: `D.RAW`.**

Primary current repository line: `main`.

Latest canonical main integration:

`a46d438145427e09e9c09db8e6cf3e443abe5a51` — PR #81

Main includes the RAW_SENSOR-first universal physical-capture foundation plus live preview, tap focus, focus lock and display-only Macro Loupe v0.3. Camera2 remains transport/provenance only. The normal ultra-wide, wide/main and tele routes use separate physical-camera acquisition paths; the existing special 4K→200MP Camera-5 route remains separate and preserved.

## Current core vision

- The sealed RAW/DNG/source is the immutable evidence foundation, not the boundary of the reconstructed world.
- Universal intake reads both the technical/source **back side** and the visible photographic **front side** from the beginning.
- Frontside inspection is deterministic classical vision only and remains `APPEARANCE_DERIVED_ONLY` unless a separate scientific path proves a stronger fact.
- **No AI/ML/neural/generative/learned models are allowed in D.RAW.**
- Device-specific maps are optional hints, never prerequisites for universal source entry or scientific truth.
- Camera and file sources converge after source sealing: physical capture → sealed app-visible RAW_SENSOR evidence → derived compatibility DNG → the same Universal Intake as imported RAW.
- Ultra-wide / wide-main / tele are dynamically discovered physical acquisition routes. Current PR #82 real-device tests prove distinct physical 4/2/5 runtime routes on the tested device, while role names/FoV ordering remain acquisition/UI knowledge rather than universal scientific truth.
- Float64 compute where needed and controlled Float32 Scientific-Master/RAW storage remain the intended precision architecture.
- Provenance/admission/authority are guardrails around claims; they do not bound the Free World itself.
- The source raster is now explicitly **measurement sampling geometry, not the world-resolution boundary**. Active successor work introduces a sparse raster-independent sample lattice: measured source samples remain exact anchors; all positions between them begin `UNKNOWN` and are not upscaled/interpolated pixels.
- The canonical Android product is `suite_android`. The geometry/universal-capture APK is a historical/test capture assistant.

## Active unmerged 2026-09-28 device research

The following work is deliberately **draft/unmerged** and must not be confused with canonical `main`:

- **PR #82** · `fix/universal-camera-input-v04-2026-09-28` — real-device validated normal physical routing: ultra-wide route/physical 4 at 1.82 mm, main route/physical 2 at 6.55 mm, tele route/physical 5 at 22.48 mm; derived DNG Orientation compatibility fix validated through the scientific pipeline.
- **PR #83** · `research/dark-chroma-stability-v01-2026-09-28` — same-observation frontside dark-chroma instability audit + structure veto; no correction/writeback.
- **PR #84** · `research/dark-chroma-stability-v02-2026-09-28` — first information-support gate. Real-device near-black tele testing proved the entropy hard-cutoff too brittle, while correction safety still held.
- **PR #85** · `research/dark-chroma-stability-v03-2026-09-28` — device-validated successor: non-entropy-hinged frontside degeneracy blocker plus sparse measured backside signal-support from the selected DNG CFA payload. Global backside support may **block** hidden-colour reconstruction but may never enable correction.
- **PR #86** · `research/dark-chroma-stability-v04-2026-09-29` — device-validated in three cases: near-black tele remains `DARK_UNINFORMATIVE`; structured ultra-wide has zero visible candidates; selective main/wide `1790637640867` binds all 6 visible candidates but exposes a new limitation: `structure_protection_present` is true in 3072/3072 N2 tiles although only ~16.56% of sampled points are structure-protected. Therefore v0.4 binding is validated, but the 64x64 any-structure-presence veto is too coarse for candidate promotion. Correction-supported and private chroma A/B/Delta remain false.
- **PR #87** · `research/dark-chroma-stability-v05-2026-09-29` — parent device-validated on selective main/wide `1790662450477`: 15/15 visible candidates fine-bound, overlap/interior structure fraction ≈0.1677/0.1678, fine tiles structure/free=558/0, zero-interior-structure=0, correction-supported=0. The 32×32 refinement therefore does not separate candidates; next gate is exact sampled structure-support distance/density, still audit-only: unchanged N2 structure gate, but 32×32 source reporting tiles expose sampled structure-protection density and fully-contained interior support for selective Dark-Chroma regions. It is measurement-only: no unsampled-pixel inference, no protection reduction, no correction enable and no private chroma A/B/Delta.

Current Dark Chroma law:

```text
one sealed observation
 -> deterministic frontside structure/chroma diagnostics
 -> measured selected-DNG backside signal support where topology is provable
 -> blocking/uncertainty constraints
 -> one D.RAWnegative diagnostic binding

NO hidden-colour reconstruction from DARK_UNINFORMATIVE
NO CHROMA_CORRECTION_SUPPORTED without same-observation local N2/backside binding
NO private chroma A/B/Delta yet
NO Scientific-Master writeback
```

Read `state/DARK_CHROMA_RESEARCH_STATE_2026-09-29_V06.json` and
`docs/research/dark-chroma-stability-v0.6/README.md` before continuing this line.

For the current coordinate-world successor also read:
- `state/RASTER_INDEPENDENT_SAMPLE_LATTICE_STATE_2026-09-29.json`
- `docs/DRAW_RASTER_INDEPENDENT_SAMPLE_WORLD_2026-09-29.md`
- `docs/research/raster-independent-sample-lattice-v0.1/README.md`

Latest v0.4 device evidence: `docs/research/dark-chroma-stability-v0.4/DEVICE_EVIDENCE_2026-09-29_ULTRAWIDE_1790636281140.md`.
Selective main/wide evidence: `docs/research/dark-chroma-stability-v0.4/DEVICE_EVIDENCE_2026-09-29_MAIN_WIDE_1790637640867.md`.

v0.5 green test APK checkpoint: artifact `11005636260`, APK SHA-256 `615e75323005d079a2a5650dba035128e6d6a40153a1f7bd45ba927d6173e46e`. The v0.5 device target remains the selective main/wide case; correction-supported stays false.

v0.5 device evidence: `docs/research/dark-chroma-stability-v0.5/DEVICE_EVIDENCE_2026-09-29_MAIN_WIDE_1790662450477.md`. Important caveat: the uploaded package contains predecessor N2 JSON sidecars, but not the new fine-field JSON; the 32×32 aggregate values are grounded in the device UI screenshot.

- **PR #88** · `research/dark-chroma-stability-v06-2026-09-29` — parent active support-distance successor; code checkpoint `ae93117bf492e5ebd81a2411bf9f6bf592738cff` is fully green. It records exact deterministic N2 sampled Structure/Censored/CensorBoundary coordinates + center/radius and rectangle-margin support geometry and now exposes a machine-readable `Export N2 Sample Support Distance v0.1 · JSON` sidecar. No distance threshold, no probability, no protection reduction, correction-supported=false.

v0.6 green APK checkpoint: artifact `11020341011`, APK SHA-256 `b84093fdb428f724d58dc07665054537aa97cad45634748e540624e5898ac4fd`, bytes `7311423`. Device validation must include the exported support-distance JSON sidecar so exact sampled support can be independently recomputed.

- **PR #89 · Raster-Independent Sample Lattice v0.1** · `research/raster-independent-sample-lattice-v01-2026-09-29` — current active successor above v0.6; code checkpoint `f75025e0d2bbb3345653e1e6365becf28cb8f7d4` is fully green: exact source CFA samples become measured anchors in a sparse fixed-point coordinate world with 2^20 coordinate units per source pixel. No dense replacement raster, no interpolation, no upscaling and no new measurement. The current v0.6 exact support geometry is projected into this world as `D.RAW/N2RasterIndependentSampleGeometry/0.1`; Dark Chroma v0.7 binds to it audit-only.

Sample-lattice green APK checkpoint: artifact `11022051681`, APK SHA-256 `615ea1146f0a647fd3cd8d8100d6297dc7f56c2b1e7b88b1841a72e01269ada3`, bytes `7327807`. The APK is built from the fully green code checkpoint above; later branch commits only record state/documentation.

First device evidence now exists: tele `1790673150696`, 4080×3072. Universal Intake reports exactly `12,533,760` measured anchors = 4080×3072, `dense=false`, `upscaling=false`, unanchored positions UNKNOWN. Two Dark-Chroma candidates bind into the lattice. Exact support geometry separates them: candidate 0 nearest sampled Structure support = 2.6926 px with 5/12 structure samples inside r8; candidate 1 nearest sampled Structure support = 8.5147 px with 0/12 inside r8 but 7/44 by r16. This does **not** enable correction; it proves exact sample geometry carries useful local information that coarse any-structure tiles lost. Evidence: `docs/research/raster-independent-sample-lattice-v0.1/DEVICE_EVIDENCE_2026-09-29_TELE_1790673150696.md`.

- **PR #90 · Anchor-Constrained Local Reconstruction v0.1** · `research/anchor-constrained-local-reconstruction-v01-2026-09-29` — current active successor above the device-validated sample lattice; code checkpoint `ae1f322a134a6c7b29fb3f983ceb8985aadba454` is fully green. It performs deterministic period-8 holdout validation inside selective Dark-Chroma regions: the real CFA target remains immutable MEASURED evidence, is hidden from the solver, and is predicted from other same-phase measured anchors by an inverse-variance local affine plane. The existing center-excluded multiscale v0.2 predictor is retained only as a reference. Output authority is `RECONSTRUCTED_PRIVATE_AUDIT_ONLY`; lower holdout error does not prove denoising or scene truth. No correction, no private A/B/Δ and no Scientific-Master/D.RAWnegative writeback.

Read before continuing this successor:
- `state/ANCHOR_CONSTRAINED_LOCAL_RECONSTRUCTION_STATE_2026-09-29.json`
- `docs/research/anchor-constrained-local-reconstruction-v0.1/README.md`

Anchor-holdout green APK checkpoint: artifact `11032268027`, APK SHA-256 `c6c864e2985a0a916a390beee36794be7ce6cdec7c0ac4532fdbbe34c9ecf5e6`, bytes `7370911`. Device validation must include `Export Anchor-Constrained Reconstruction v0.1 · JSON`; lower holdout error remains prediction-of-noisy-target evidence only, not denoise/scene-truth proof.

## Current physical observations

- Camera-5 / telephoto remains an existing source-local proof anchor.
- Physical Camera-2 / main is now **ADMITTED_SOURCE_LOCAL**.
- User-captured ultra-wide / project Camera-4 lens route is now **ADMITTED_SOURCE_LOCAL** through direct physical-source ingress.

Main physical Observation ID:

`DRAW_PHYSICAL_OBS_a85cac58601d6cc8138bf8f9372e5161b85ab1e89afa70372f97c46461dcea79`

Main final admission state:

`c20104981a0373f0d2f7c03216272213ff614f0dc732b13247cf2a0c0c6a9b7a`

Ultra-wide physical Observation ID:

`DRAW_PHYSICAL_OBS_14757aaac784b17421598121a232c22e044f217531571697a73e4c90bff28133`

Ultra-wide final admission state:

`629edde9697387e87c39c66846f21457954d258d831464140eda42ed912b655a`

## Mandatory current reading order

1. `state/CURRENT_PROJECT_STATE_2026-09-28.json`
2. `state/DARK_CHROMA_RESEARCH_STATE_2026-09-29_V06.json` — active draft/unmerged Dark Chroma line
3. `docs/research/dark-chroma-stability-v0.6/README.md`
4. `docs/DRAW_CORE_VISION_REALIGNMENT_2026-09-28.md`
5. `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-09-27.md` — historical pre-resumption handoff
6. `state/CURRENT_PROJECT_STATE_2026-09-27.json` — parent state
7. `docs/DOCUMENT_STATUS_INDEX_2026-09-27.md`
8. `docs/research/draw-direct-physical-source-admission-v0.1/README.md`
9. `docs/research/draw-observation-record-v0.4/README.md`
10. `docs/research/draw-source-capability-envelope-v0.2/README.md`
11. `docs/research/draw-source-admission-package-v0.2/README.md`
12. `docs/research/draw-scientific-ingress-lineage-v0.1/README.md`
13. `docs/research/free-world-observation-graph-v0.1/README.md`
14. `docs/research/drawnegative-v0.1/SEAL_MANIFEST_v0_1.json`
15. `docs/DRAW_KNOWLEDGE_GROWTH_INTEGRATION_2026-09-27.md`

## Current laws

- **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
- **Measured where measured. Reconstructed where necessary. Never invented.**
- **One Free World. Many sealed observations. One evidence law.**
- **Evidence stays what it was. Knowledge can grow through admitted relations.**

## Current identity architecture

```text
immutable physical source
 -> optional compatibility-ingress lineage
 -> unchanged common scientific route
 -> Scientific Master
 -> D.RAWnegative v0.1 computational lineage
 -> physical-identity Observation Record v0.4
 -> physical Source Capability Envelope v0.2
 -> Source Admission Package v0.2
 -> physical Free World Observation node
 -> independently admitted relations
 -> optional composite fusion
 -> Deep Scene / Light Transport
 -> Appearance
 -> finite projection
```

Compatibility representation may change pipeline identity. It may **not**
create a new physical Observation.

D.RAWnegative v0.1 remains sealed and unchanged.

## Main-camera current identities

- immutable physical source:
  `a85cac58601d6cc8138bf8f9372e5161b85ab1e89afa70372f97c46461dcea79`
- compatibility ingress:
  `7c8eb85c568f6bc0ec3ae007de1658ec86d38b1144059d0fe0bc294a3e17bf08`
- Scientific Master:
  `c26939efe5e58a0d32846e905d156789e35b674bb2d1c03288bbabc53e243202`
- D.RAWnegative v0.1:
  `87955cae86a3c9318b24990018208a4982379366d4bb4ae78e830c8d1cf0ccf7`
- physical graph node:
  `029b845daf0bb20ebed4e7ce6d66eb61a172dd89e9977f996d20d604c2703353`
- Observation Record v0.4:
  `aaf2dcf0b81ebe5debcc2fa6a0b40057e5f1cf20e0d6e2df67a54fd825853fc5`
- Capability Envelope v0.2:
  `057c627bf172521d9336386d4f74ae1e26664c70c4a24ed7993b6c16c33a3986`
- Final Admission v0.2:
  `c20104981a0373f0d2f7c03216272213ff614f0dc732b13247cf2a0c0c6a9b7a`

Capture sample domain, readout domain and sensor pixel mode remain UNKNOWN.
Stored-sample sensel semantics are not certified; direct sensor ADC claims are
not allowed. Main has zero cross-observation relations and zero fusion
admissions.

## Ultra-wide current identities

The private source `IMG_20260927_084938.dng` is user-attested as a self-captured ultra-wide RAW and passes the unchanged common scientific route directly.

- immutable physical source: `14757aaac784b17421598121a232c22e044f217531571697a73e4c90bff28133`;
- serialized CFA payload: `87e56296cd51a076d921c5a529dfd64784dfc8b9789869b64644b672767a89a5`;
- Scientific Master: `949777edb5541064e190d148775ce27d303ce1e0d35f57d7a837fc092ad9f6d9`;
- D.RAWnegative v0.1: `b0ee1322ff6260eb30072533c2b3e294be46375778ac4622b908d6c746e172e0`;
- physical graph node: `85d8ef0b9b3e769f30c4d62a114e70c94369fc3250ae5c4f664c04223980a64e`;
- final Admission v0.3: `629edde9697387e87c39c66846f21457954d258d831464140eda42ed912b655a`;
- validation run `36302484185`: **SUCCESS**.

Project mapping associates the ultra-wide route with Camera 4, but this file does not carry a runtime Camera2 active-physical-result proof. That distinction stays explicit. Capture-sample domain, readout domain and sensor pixel mode remain UNKNOWN. Calibration bindings, graph relations and fusion admissions remain zero.

## Main ↔ Ultra-Wide Geometry Relation Campaign v0.1

The first inter-lens relation campaign is now main-integrated.

- merge: `753a8048e5fbb97cd6bbaca95403664aeb756d31`
- validation run: `36304289379` — **SUCCESS**
- state: `77d490b0d190b39736aa124add6f1fae9548c132c26d11d1da6b3e8c5aaccd5b`

Current graph state is GEOMETRY = UNKNOWN. The existing admitted MAIN and
ULTRA_WIDE observations are not a geometry measurement pair because they are
different capture sessions/scenes.

Promotion requires a new matched metric-target campaign with at least 12
training lens-pairs plus 4 disjoint holdout lens-pairs. Keep the phone rigid,
keep the target static between each MAIN/ULTRA_WIDE pair, admit every source
independently, fit per-lens projection/distortion plus the rigid inter-lens
transform on training data only, then freeze model/thresholds before holdout.

No geometry result may automatically grant radiometric, colour, optical,
uncertainty, temporal, provenance or fusion authority.

### Open-world evolution

Permanent rule:

**Seal the evidence, not the thinking.**

Sealed source/evidence objects remain immutable. Models and interpretations do
not become final merely because the source is sealed. New scientific models
must be added through versioned successors while preserving parent evidence.

### Direct RAW geometry-pair intake

Main-integrated:

- `docs/research/draw-geometry-direct-raw-pair-intake-v0.1/**`
- validation `36312425248`: SUCCESS
- Camera2 required: **false**
- direct original RAW/DNG pair intake: **true**
- source-local admission required before feature extraction: **true**
- relation/transform/fusion granted by pair intake: **false**

The pair tool is intentionally semantically forward-compatible with future
`D.RAW/SourceAdmissionPackage/*` successors.

### Geometry Capture-Set Gate v0.1

Main-integrated and host validated.

- `docs/research/draw-geometry-capture-set-gate-v0.1/**`
- dedicated run `36312992699`: SUCCESS
- 12 TRAINING + 4 HOLDOUT matrix: proven
- training model selection after complete set: allowed
- HOLDOUT model selection: forbidden
- final holdout scoring before model freeze: forbidden
- relation/transform/fusion authority: not granted
- projection/distortion model selected by this gate: no

The evidence membership can be sealed while scientific model hypotheses remain open to versioned successors.

### Geometry Model Freeze Gate v0.1

Main-integrated and host validated.

- `docs/research/draw-geometry-model-freeze-gate-v0.1/**`
- dedicated run `36313383767`: SUCCESS
- TRAINING-only model selection required
- HOLDOUT access during model selection: forbidden
- model vocabulary: extensible, not closed
- freeze scope: one reproducible certificate candidate
- frozen candidate = final scientific truth: false
- final HOLDOUT scoring after freeze: allowed
- relation/transform/fusion authority after freeze: not granted
- real MAIN ↔ ULTRA_WIDE frozen model: none yet

A changed model, parameterization or threshold policy becomes a new versioned
candidate rather than a rewrite of the old candidate.

### Geometry HOLDOUT Validation Gate v0.1

Main-integrated and host validated.

- `docs/research/draw-geometry-holdout-validation-gate-v0.1/**`
- dedicated run `36313705179`: SUCCESS
- exact HOLDOUT member coverage: required
- partial HOLDOUT: rejected
- refit/model/threshold changes during HOLDOUT: rejected
- PASS -> validated candidate awaiting endpoint applicability
- FAIL -> rejected candidate preserved as knowledge
- PASS grants graph relation: no
- PASS grants coordinate-transform authority: no
- endpoint applicability before relation admission: required
- real MAIN ↔ ULTRA_WIDE HOLDOUT evaluation: none yet

## Validation boundary

Main has executed the unchanged common scientific C++ route on host. This does
not replace Android/device validation or upgrade sensor provenance.

Latest fully host + Android validated D.RAWnegative code checkpoint:

`3e150afeb36cbb68fd318b0143a315e64d5a637f`

Android/NDK/JNI run `36260305613`: **SUCCESS**.

## Next empirical step

Main, ultra-wide and telephoto now exist as separate source-local physical anchors. The next empirical work is **relation measurement**, not source admission.

Measure geometry, radiometric gauge, colorimetric/spectral, optical support, uncertainty/correlation, temporal compatibility and provenance independently between the physical Observation IDs. No calibration transfers by lens/device identity, and no fusion is allowed until the required relation certificates are separately ADMITTED.

Optional: use the preserved FotoGraaf ultra-wide probe later to obtain a runtime Camera2 active-physical-result proof for Camera 4. This would strengthen route provenance only.

---

# D.RAW — HISTORICAL PROJECT IDENTITY — 2026-09-26

**Official current product name: `D.RAW`.**

Current active architecture/code branch:

`architecture/drawnegative-v01-2026-09-26`

Current public scientific-negative identity:

**D.RAWnegative v0.1**

Historical `TruthNegative` remains only as computational/compatibility
ancestry where its existing bytes, schemas, symbols or hashes must stay stable.

Latest fully host + Android validated D.RAWnegative code checkpoint:

`3e150afeb36cbb68fd318b0143a315e64d5a637f`

Validation:

- host GCC/Clang/ASan/UBSan run `36260305512`: SUCCESS;
- signed Android/NDK/JNI run `36260305613`: SUCCESS;
- APK SHA-256:
  `b116339ffc4c65a571b533a7103a9560043852cd239380420c877f0001e92317`;
- artifact ID: `10911843774`.

Default GitHub `main` is **not current** and remains at:

`514f2f4bde6aba5a6709e176c03b22c3b9aea912`

**Read first:**

1. `state/CURRENT_PROJECT_STATE_2026-09-26.json`
2. `docs/DRAW_MAIN_PROJECT_STATE_2026-09-26.md`
3. `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-09-26.md`
4. `docs/DOCUMENT_STATUS_INDEX_2026-09-26.md`
5. `docs/CORE_VISION_LENS_INDEPENDENT_FREE_WORLD_OBSERVATION_ARCHITECTURE_v0_2_DRAWNEGATIVE.md`
6. `docs/research/drawnegative-v0.1/README.md`
7. `docs/research/drawnegative-v0.1/SEAL_MANIFEST_v0_1.json`
8. `docs/research/draw-observation-record-v0.2/README.md`
9. `docs/research/draw-observation-record-v0.2/examples/CAMERA5_LAMP_SCENE_DRAWNEGATIVE_OBSERVATION_v0_2.json`
10. `docs/CORE_VISION_LENS_INDEPENDENT_FREE_WORLD_OBSERVATION_ARCHITECTURE.md`
11. `docs/research/lens-independent-free-world-observation-contract-v0.1/README.md`
12. `docs/research/lens-independent-free-world-observation-contract-v0.1/DRAW_OBSERVATION_CONTRACT_v0_1.json`
13. `docs/research/lens-independent-free-world-observation-contract-v0.1/SEAL_MANIFEST_v0_1.json`
14. `docs/research/draw-observation-record-v0.1/README.md`
15. `docs/research/draw-observation-record-v0.1/examples/CAMERA5_LAMP_SCENE_OBSERVATION_v0_1.json`
16. `docs/research/truthnegative-n2-factored-confidence-state-v0.3.1/README.md`
14. `docs/research/truthnegative-appearance-highlight-detail-audit-v0.1/evidence/DEVICE_RESULT_2026-09-26.md`
15. `docs/research/truthnegative-appearance-highlight-headroom-sweep-v0.2/evidence/DEVICE_RESULT_LAMP_SCENE_2026-09-26.md`

Current architecture:

```text
sealed physical observation / Source Evidence
 -> D.RAW Observation Contract
 -> Source Capability Envelope
 -> Float64 measurement/calibration/reconstruction
 -> Scientific Master
 -> validated Float32 scientific storage where admitted
 -> legacy TruthNegative Continuous parent
 -> D.RAWnegative per observation
 -> Free World Observation Graph
 -> Deep Scene / Light Transport
 -> View Contract / Appearance
 -> finite projection
```

## 2026-09-26 sealed lens-independent Free World v0.1

The Free World is now formally lens-independent.

Canonical law:

**One Free World. Many sealed observations. One evidence law.**

Main, ultra-wide, telephoto and future admitted cameras use the same world
architecture. Lens/sensor/readout specifics belong to the Observation Contract
and Source Capability Envelope.

The Zero-Line remains `T=log2(L/L0)`. A shared coordinate family does not
automatically prove a shared radiometric gauge. Cross-observation radiometric
equality/fusion requires an admitted gauge relation.

TruthNegative is explicitly one raster-independent scientific-negative state
per admitted observation lineage. Multiple observations compose in the Free
World Observation Graph, not by silently merging TruthNegative identities.

The v0.1 canonical files are SHA-256 byte-sealed. Any semantic change requires
a versioned successor.

## Current D.RAWnegative identity

D.RAWnegative v0.1 is now a real runtime state, not only a naming decision.

The Android pipeline constructs:

```text
TruthNegative Continuous v0.5 parent
 + D.RAW Observation ID
 + source-local Zero-Line gauge ID
 + precision/storage contract
 -> D.RAWnegative v0.1 state SHA-256
```

The runtime defaults to a source-local gauge and therefore keeps
cross-observation radiometric equality/fusion disabled. A shared gauge must be
separately admitted.

The PRO UI now exposes `PRO · D.RAWnegative v0.1`. The preview packet carries
the D.RAWnegative state SHA-256 in addition to the legacy parent SHA.

The old `.tnc` container remains a legacy TruthNegative container because its
bytes do not contain the new D.RAWnegative state. It is not being mislabeled.

## D.RAWnegative v0.1 seal

D.RAWnegative v0.1 is now byte- and semantics-sealed.

Seal enforcement run `36260886983`: **SUCCESS**.

Changing the v0.1 canonical core requires a versioned successor.

The current Observation Record successor is v0.2. It explicitly stores both:

- `legacy_truthnegative_parent`;
- `drawnegative`.

The Camera-5 lamp-scene migration deterministically binds parent
`73c908…002c5` to D.RAWnegative state
`7fabfd…3e9a5` without creating new evidence.

Observation Record v0.2 validation run `36260712364`: **SUCCESS**.

## First executable observation record

`DRAWObservationRecord v0.1` is now the first machine-level implementation of
the sealed architecture.

It binds one real Camera-5 lamp-scene source to:

- sealed Source Evidence;
- acquisition/lens provenance;
- source topology;
- Source Capability Envelope;
- source-local Zero-Line gauge;
- Scientific Master identity;
- TruthNegative identity;
- one-frame/one-evidence counts;
- separate geometry/radiometry authority.

CI run `36258955138` is green and also proves fail-closed rejection when
cross-observation radiometric fusion is enabled without an admitted common
gauge.

Main and ultra-wide should receive separate records next. No calibration is
borrowed from telephoto.

Current route semantics remain:

```text
D.RAW PURE     = Scientific View
D.RAW ADVANCED = Appearance / Restoration View
D.RAW PRO      = Open Scene / Light Transport
```

## 2026-09-26 N2 status

N2 remains diagnostic/appearance research only.

The current SAFE full-colour path requires:

- exact Scientific-Master source-pixel baseline identity;
- `baseline-mismatch=0`;
- reconstruction-support closure over backend `requiredHalo()`;
- `protected-changed=0`;
- no Direct-CFA, Scientific-Master or TruthNegative writeback.

Center-excluded v0.2, whole-frame v0.2.1, vector Confidence Field v0.3 and
Factored Confidence State v0.3.1 are integrated as audit layers. No scalar
confidence probability and no automatic denoise promotion exist.

## 2026-09-26 Appearance finding

Real-device Appearance Highlight Detail v0.1 proved a many-to-one property in
the existing PRO 100-nit-reference / 100-nit-peak display mapping:

- 502 source samples above reference white;
- all 502 mapped exactly to display peak;
- source CENSORED count = 0;
- 851 distinct neighboring source-luminance pairs collapsed to the same peak.

The Honor camera-app comparison image is `VISUAL_REFERENCE_ONLY`; it is not
D.RAW evidence or calibration.

Appearance Highlight Headroom Sweep v0.2 is Android-green and now has a real
lamp-scene device result in addition to the earlier white-exterior scene.
For the lamp scene the 100/100 baseline produced 941 collapsed distinct pairs
with source_censored=0; 90/100 reduced the audit's exact peak-collapse to zero
while leaving all samples at/below the 90-nit knee unchanged. This does not
promote 90/100 automatically. The unchanged 1058 gamut/display clamps remain a
separate issue.

Current v0.2 validation:

- standalone GCC/Clang/ASan/UBSan run `36254075689`: SUCCESS;
- signed Android run `36254294042`: SUCCESS;
- APK SHA-256:
  `d46b40c7650ad0b1102438bc1a31b1c4f8ee7c4c14050af633c435c5b0405e8b`.

Permanent rule:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

---

# D.RAW — CURRENT PROJECT IDENTITY — 2026-09-25

**Official current product name: `D.RAW`.**

Current code-bearing integration checkpoint:

`c7d7cef05502aca6f22f0d049987aad3a9f37b0a`

Active integration:

`integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master`

**Read first:**

1. `state/CURRENT_PROJECT_STATE_2026-09-25.json`
2. `docs/DRAW_MAIN_PROJECT_STATE_2026-09-25.md`
3. `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-09-25.md`
4. `docs/research/free-world-output-pixel-v0.1/README.md`
5. `docs/research/free-world-appearance-resolve-v0.7/README.md`
6. `docs/research/truthnegative-continuous-v0.5/README.md`
7. `docs/research/truthnegative-roundtrip-oracle-v0.6/README.md`
8. `docs/research/truthnegative-optics-support-v0.7/README.md`
9. `docs/research/truthnegative-deep-scene-bridge-v0.8/README.md`
10. `docs/research/truthnegative-camera5-color-highlight-oracle-v0.1/README.md`
11. `docs/research/truthnegative-native-container-v0.1/README.md`

Current main architecture:

```text
sealed Source Evidence
 -> Scientific Master
 -> Continuous Scene Field
 -> Deep Scene
 -> geometry/radiometry authority
 -> Light Transport
 -> Appearance / Viewing / Display Resolve
 -> finite output
```

Current route semantics:

```text
D.RAW PURE     = Scientific View
D.RAW ADVANCED = Appearance / Restoration View
D.RAW PRO      = Open Scene / Light Transport
```

The v0.2-v0.7 modules, TruthNegative Continuous v0.5, Round-Trip Oracle v0.6, Optics Support v0.7, Deep Scene Bridge v0.8, Camera-5 Color/Highlight Oracle v0.1 and TruthNegative Native Container v0.1 are integrated into the Android main line. PRO now has the continuous preview, Camera-5 diagnostic oracle and .tnc export/import round-trip bridge. PURE and existing validated exports are not rerouted.

Current green main validation:

- Android signed ARM64 + current A-D integration: `36115128965` — SUCCESS
- Scientific Master F64 + Color Audit: `36112964482` — SUCCESS
- Open Scene Field/local authority: `36112964500` — SUCCESS
- Unified Output Preview + Android main integration: `36115129004` — SUCCESS
- TruthNegative Continuous v0.5 GCC/Clang/ASan/UBSan: `36107899950` — SUCCESS
- TruthNegative Round-Trip + Optics gates: `36111981454` — SUCCESS
- TruthNegative Deep Scene Bridge v0.8: `36112847260` — SUCCESS
- Camera-5 Oracle + Native Container: `36114391772` — SUCCESS

Current installable APK:

- artifact ID `10855057536`
- APK SHA-256 `dd2ca57e0de292dbc06617d9efce39cff7d6b242a1b20de1ebf74808b1ed5273`
- stable development certificate SHA-256 `a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`

Permanent rule: **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

---

# D.RAW — CURRENT PROJECT IDENTITY — 2026-09-24

**Official project/product name from this point forward: `D.RAW`.**

`TruthRaw` remains only where required for historical provenance, sealed evidence,
legacy schema/wire identifiers, stable Android package/class identities, and
backward compatibility. Historical documents are not rewritten.

See: `docs/PROJECT_RENAME_DRAW_2026-09-24.md`.

---

# CURRENT MAIN NOTICE — 2026-09-24

**Read first:**

1. `state/CURRENT_PROJECT_STATE_2026-09-24.json`
2. `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-24.md`
3. `docs/CURRENT_RAW_DNG_INGRESS_AND_OUTPUT_PREVIEW_2026-09-24.md`
4. `docs/research/scientific-master-f64-reconstruction-v0.1/README.md`
5. `docs/research/formal-color-calibration-audit-v0.1/README.md`
6. `docs/research/open-scene-field-v0.85/README.md`
7. `docs/research/unified-output-preview-v0.1/README.md`

Active main integration:

`integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master`

Current green code-bearing checkpoint:

`675265b084577aa0c005905363ab9f6ba39dfc7f`

Final general Android validation: run `35963545440` — **SUCCESS**.

Final Unified Output Preview validation: run `35963625751` — GCC/Clang/route-contract/Android **SUCCESS**.

Current 2026-09-24 architecture checkpoint includes:

- active F64 branch-sensitive Scientific-Master reconstruction with Float32 canonical storage;
- Open Scene Field v0.85;
- TruthNegative TN-4 + local authority projection v0.4;
- local authority policy v0.86;
- Unified Output Preview v0.1 with per-output stored-orientation binding;
- DNG full scientific ingress, NEF measurement-only subset, proprietary RAW decoder-pending fail-closed handling;
- visible UI synchronized away from stale TN-3 / v0.84.2 wording;
- JPG-L scientific payload migrated from TN-3 to TN-4.

F64 + formal-color production promotion code commit:

`ad3a1f465fc77f76972c64b3a806838fbeddc310`

The user's 2026-09-21 **no-more-code-changes freeze was explicitly lifted on 2026-09-24**. Validated recommendations may now be integrated directly into the main project. Sealed source evidence and byte-frozen scientific reference modules remain immutable; new science must use explicit versioned successors rather than silently rewriting historical reference code.

The 2026-09-21 freeze notice below is preserved as historical provenance only.

---

# HISTORICAL FINAL CODE FREEZE NOTICE — 2026-09-21

**Historical reference:** `docs/handoff/TRUTHRAW_FINAL_CODE_FREEZE_V0843_2026-09-21.md`

The historical frozen code-bearing head was:

`172a100786eb18d4b08564bbfb025a44f42cfa1e`

Historical frozen APK SHA-256:

`5805d291b163d66e68d5ab98aa4d2971e38b062b724325469d6002fbd8b1917e`

---

# START HERE — TruthRaw current bootstrap

## CURRENT ACTIVE INTEGRATION — 2026-09-21

**Read first:**

1. `state/CURRENT_PROJECT_STATE_2026-09-21.json`
2. `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-21.md`
3. `docs/research/android-acceleration-v0.1/README.md`
4. `docs/research/android-acceleration-v0.1/APV_PROFESSIONAL_VIDEO_ROUTE.md`
5. `docs/research/android-acceleration-v0.1/PARALLEL_SCIENTIFIC_MASTER_PLAN.md`
6. `docs/TRUTHRAW_V0842_ADVANCED_RENDER_EDIT_FLOAT32_2026-09-21.md`

Active branch:

`integration/truthraw-suite-v0-84-2-adaptive-compute-router`

Current Android integration:

`0.51-v0.84.2-adaptive-compute-router`

Latest fully green code-bearing CI:

`35547319268` at `5a7acf28f90cac82297c2d5af3e81006c438da2f`

Current APK:
- bytes: `5,994,861`
- SHA-256: `9b4184afdec22d6313ba08d7abe8884e812e3b3cadc7f29d492685af4d820295`
- artifact id: `10617071043`

Current scientific boundary:

- v0.84 per-output RGB-channel authority map exists;
- Scientific HDR remains `BLOCKED`;
- blocked reason is now `UNKNOWN_CHANNEL_AUTHORITY_PRESENT`;
- Natural HDR remains `APPEARANCE_ONLY`;
- compute acceleration never increases scientific authority.

Current implementation additions since the old 2026-09-20 bootstrap:

- non-destructive orientation override;
- true Float32 Lightroom JPG-L RAW/Edit DNG + embedded JPEG preview;
- PURE Float32 DNG preview/self-binding improvements;
- Android background `mediaProcessing` execution + wake-lock + truthful timers/status dots;
- strict-FP native `-O2` with exact v4.7i O0/O2 signature gate;
- bounded ordered multicore executor;
- dynamic multicore Full-res Restoration;
- Android thermal + CPU/GPU resource headroom scheduling;
- runtime ADPF worker hints on API33+;
- Vulkan hardware capability discovery;
- APV hardware encoder/decoder discovery in PRO;
- ADVANCED/PRO Render/Edit Float32 DNG: developed extended-linear derivative, deterministic projected-raster SHA replay, embedded non-authority JPEG preview, tile-partition invariance and negative-headroom preservation.

Immediate continuation is documented in the 2026-09-21 handoff. Do not revert to the
old v0.83 assumption that output-channel authority is absent.

## HISTORICAL ACTIVE INTEGRATION SNAPSHOT — 2026-09-20



**Read first:**

1. `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-20.md`
2. `state/CURRENT_PROJECT_STATE_2026-09-20.json`
3. `docs/TRUTHRAW_V083_HDR_AUTHORITY_2026-09-20.md`
4. `docs/research/hdr-authority-v0.83/README.md`
5. `docs/TRUTHRAW_V082_ILLUMINATION_STATE_2026-09-20.md`
6. `docs/TRUTHRAW_V081_OUTPUT_ACUTANCE_V47K_2026-09-20.md`
7. `docs/TRUTHRAW_V080_ADAPTIVE_DETAIL_V47J_2026-09-20.md`

Current active branch:

`integration/truthraw-suite-v0-83-authority-aware-hdr`

Current app:

`0.49-v0.83-authority-aware-hdr`

Current green CI:

`35523343893`

Current APK SHA-256:

`88d8ac29b64ddef67668c026c05711974c645e3fc1ef6da4b8fb942bda4066a7`

Frozen v0.72 baseline references retained for governance/provenance:

- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-19.md`
- `state/CURRENT_PROJECT_STATE_2026-09-19.json`

Current HDR boundary:
presentation HDR remains usable but is explicitly `APPEARANCE_ONLY`. Scientific HDR is `BLOCKED / NO_PER_OUTPUT_CHANNEL_AUTHORITY` until a canonical authority/support map exists at final output RGB coordinates. CENSORED stays a bound, UNKNOWN gets no headroom, v0.82 illumination cannot create headroom.

Immediate next branch:

`integration/truthraw-suite-v0-84-output-channel-authority-map`

Next objective: propagate channel authority/support through reconstruction and resampling without altering HDR gain. Compute tiles must never become authority regions.

## TruthNegative research-branch overlay — 2026-09-19

If the checked-out branch is `research/truthnegative-v0-2-existing-house-binding`, read this overlay before the older bootstrap below:

1. `docs/DOCUMENT_STATUS_INDEX_2026-09-19_TRUTHNEGATIVE_BRANCH.md`
2. `docs/TRUTHNEGATIVE_EXISTING_HOUSE_ALIGNMENT_AUDIT_2026-09-19.md`
3. `docs/CORE_VISION_TRUTHNEGATIVE_SCIENTIFIC_NEGATIVE.md`
4. `docs/CURRENT_TRUTHNEGATIVE_ARCHITECTURE_2026-09-19.md`
5. `docs/research/truthnegative-v0.2/README.md`
6. `state/TRUTHNEGATIVE_V0_2_EXISTING_HOUSE_BINDING_STATE_2026-09-19.json`
7. `docs/handoff/TRUTHNEGATIVE_BRANCH_HANDOFF_2026-09-19.md`

Corrected branch model:

`any admitted RAW -> native ingress OR Gatehouse handoff -> Source Evidence -> Measurement/de-ISP -> Latent Camera Scene -> Scientific Master -> Dynamic Authority/Open Scene`

TruthNegative binds to that existing reconstructed state:

`Scientific Master + lineage + authority -> TruthNegative Core -> optional reconstructed sensor-negative projection`

This is research only, not canonical/main promotion. The HONOR OEM/HAL/RAW14 route remains an independent parallel research line.

This is the living bootstrap entry point for the consolidated TruthRaw research state, updated through the HONOR Camera-5 v0.20 payload-geometry result on 2026-09-17.

It is a navigation/current-state document. It does not rewrite frozen historical evidence, dated handoffs, rejected experiments or canonical bytes.

## Mandatory current reading order

Read in this order:

1. `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
2. `docs/CURRENT_SCENE_PHYSICS_RESTORATION_200MP_2026-09-16.md` — scientific background; its old open Camera-5 physical gate is superseded by the 2026-09-17 Camera-5 documents
3. `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md`
4. `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json`
5. `docs/DOCUMENT_STATUS_INDEX_2026-09-17.md`
6. `docs/DOCUMENT_STATUS_INDEX_2026-09-16.md` — preserved historical integration index; Camera-5 interpretation is superseded by the 2026-09-17 index
7. `state/CURRENT_PROJECT_STATE_2026-09-16.json`
8. `docs/PROJECT_HISTORY_AND_CHANGES_2026-09-16.md`
9. `docs/handoff/TRUTHRAW_CONSOLIDATED_HANDOFF_2026-09-16.md`
10. `docs/handoff/TRUTHRAW_DETAILED_HANDOFF_2026-09-16.md`
11. `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md`
12. only then the exact canonical/research/module documents relevant to the task.

Camera-5 detailed reading order:

- `docs/HONOR_MAGIC8_PRO_TELE_200MP_FULL_RAW_V014_2026-09-17.md`
- `docs/HONOR_CAMERA5_TWO_DOOR_AIRLOCK_V017_2026-09-17.md`
- `docs/HONOR_MAGIC8_PRO_TELE_PAYLOAD_GEOMETRY_V020_2026-09-17.md`.

Historical architecture documents remain background authorities in their own domain:

- `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md`
- `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
- `docs/CORE_VISION_UNCERTAINTY_AWARE_APPEARANCE.md`
- `docs/CORE_VISION_VIRTUAL_OBSERVATION_MANIFOLD.md`.

## One-sentence current definition

**TruthRaw seals app-visible RAW/CFA observations as immutable source evidence, reconstructs a separate uncertainty- and authority-aware Scientific Master in Free Scientific Space, and permits open-world/counterfactual/restoration/appearance projections only without upgrading what the physical evidence actually measured.**

## Permanent scientific laws

1. **Source evidence is immutable.** Original admitted bytes, decoded CFA identity and capture provenance are history, not a workspace to rewrite.
2. **Representation can exceed the source. Knowledge claims cannot exceed the evidence.** Free Scientific Space may exceed RAW code range, source CFA lattice, SDR or DNG, while authority remains bounded.
3. **Measured is not reconstructed.** `MEASURED`, `RECONSTRUCTED`, `CENSORED`, `UNKNOWN`, `COUNTERFACTUAL` and appearance/transport/restoration state remain distinguishable.
4. **Evidence count is physical.** A single physical frame remains one frame/evidence source unless another real modality is explicitly admitted; virtual observations do not create measurements.
5. **Censoring is a bound, not a guessed exact value.**
6. **Uncertainty/support is local and fail-closed.** Wrong identity, coordinates, missing runtime fields or missing semantics cannot silently widen authority.
7. **Precision is stage-specific.** Exact integer/packed evidence first; F64 where branch-sensitive science requires it; controlled F32 storage only after validation.
8. **Counterfactual state never becomes capture evidence.**
9. **Appearance/transport never writes back into science.**
10. **Restoration never overpaints valid measured support in the Scientific Master.**
11. **Compute resources never increase truth authority.**
12. **Vendor metadata and vendor-key names are observations, not calibration/semantic authority by themselves.**
13. **Rejected/failed experiments remain provenance.**

## Current project flow

`Source Evidence`
`-> acquisition/provenance + sample-topology proof`
`-> measurement/de-ISP`
`-> Scientific Master in Free Scientific Space`
`-> Dynamic Authority / uncertainty / support`
`-> Open Scene State`
`-> optional counterfactual/restoration state`
`-> appearance / HDR / transport`
`-> finite export`.

DNG, LinearRaw, reconstructed CFA, restored renders, diagnostic previews and display files are projections/compatibility products. They do not replace the Scientific Master or become original measurement evidence.

## Current Camera-5 result through v0.20

The old 2026-09-16 state said a real physical `16320x12288` Camera-5 RAW_SENSOR delivery still had to be obtained. That gate is now historical.

v0.14 proved one physical-Camera-5 app-visible `16320x12288` RAW_SENSOR delivery through logical camera 0 with:

- physical output binding to camera 5;
- exact Image/physical-result timestamp equality;
- original `Image.Plane[0]` persisted and SHA-256 sealed before interpreting contradictory pixel-mode metadata;
- app-visible source envelope size `401,080,320` bytes, rowStride `32640`, pixelStride `2`.

v0.16/v0.17 then added a two-door airlock:

`request/session fingerprint before HONOR/QTI execution`
`-> vendor pipeline`
`-> delivered HardwareBuffer/Image/physical result envelope`.

The post-HAL probe is descriptor/metadata-only and does not lock, map or write the HardwareBuffer.

v0.19 established that the tested sealed source envelope is not fully populated:

- only declared rows `0..767` contain non-zero source codes;
- rows `768..12287` are all zero;
- populated source prefix = exactly `25,067,520` bytes;
- therefore `16320*768*2 == 4080*3072*2`.

v0.20 then proved that `4080x3072` is the **only runtime-advertised standard Camera-5 RAW_SENSOR geometry** whose U16 byte count matches that populated prefix. The derived `.rawpayload` is an exact no-transform prefix copy and its SHA equals the Stage-3.6 first-band SHA.

The same bytes, indexed as `4080x3072`, produce:

- a coherent full-frame diagnostic image;
- Bayer-like 2x2 phase statistics;
- much stronger same-phase distance-2 correlations than immediate cross-colour distance-1 correlations;
- observed code range `64..1023` in the v0.20 capture.

Current bounded classification:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Descriptive shorthand:

`APP_VISIBLE_4080x3072_BAYER_LIKE_RAW_PAYLOAD_EMBEDDED_IN_16320x12288_HAL_ENVELOPE`.

Neither string proves native physical sensor geometry, untouched ADC, exact binning/remosaic mechanism or 200 MP optical resolving power.

## Current Camera-5 route clues

The untouched v0.20 request/session surface exposes candidate vendor controls including:

- `EnableIdealRAW`
- `RawCbSourceType`
- `EnableXCFAOptimization`
- `HALOutputBufferCombined`
- `EnableInsensorZoom`
- `EnableSnapshotOnlyInsensorZoom`
- `EnableMCXMasterCb`.

v0.20 changed none of them. `EnableXCFAOptimization` was visible as zero; `EnableIdealRAW` and `RawCbSourceType` were present but had null builder current/default values in the captured fingerprint.

Post-HAL route clues include HONOR `binningFactor=4`, an AEC crop near the `4080x3072` domain, ISP crop metadata in the `16320x12288` domain, a 40-byte `sensorCustomMetaData` sidecar and QTI multicamera/AF metadata. These are hypotheses/route observations, not vendor-semantic proof.

## Current next route experiment

Use v0.20 unchanged as the control.

First single-variable candidate:

`org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW`.

Rules:

1. separate branch/build;
2. runtime key lookup and type verification on this device;
3. change exactly one unknown vendor variable;
4. preserve logical0 -> physical5 -> exact 16320x12288 MAX topology;
5. preserve source-first sealing and both airlock sides;
6. repeat Stage 3.6 and Stage 3.7 unchanged;
7. compare populated byte count, geometry, hashes, result pixel mode, binning/crop metadata and full route fingerprints against v0.20.

Only after that should `RawCbSourceType`, XCFA-related controls, in-sensor zoom or other route keys be tested separately.

## Multi-camera / focus status

Camera 5 exposes Android focus-distance results and QTI AF lens-position/phase-detect telemetry. QTI multicamera sidecars are present during the tele capture.

An isolated v0.18 physical-focus branch exists, but it is deliberately not in the trusted v0.19/v0.20 parent chain because source-population/topology had higher priority.

Future multi-camera/multi-focus work must seal each physical RAW separately and keep physical-frame/evidence counts explicit. Simultaneous capture does not automatically mean independent evidence.

## Precision and zero-line continuity

The current precision direction remains:

`exact RAW integer/packed evidence`
`-> integer-exact topology/content audit`
`-> F32 only where proven safe`
`-> F64 branch-sensitive reconstruction/calibration/optimization/covariance`
`-> optional controlled F32 storage after F64 validation`.

The zero-line / TruthRange gauge remains downstream of acquisition and calibration. RAW code 0, BlackLevel, display black and the zero-line are not interchangeable concepts.

## Frozen downstream references

For the previously validated source-bound research state:

- Scientific Master SHA-256: `a86034da7b6f9663640ee3b4478ccf4294d23b720f2b087d083dca38f5fb4640`
- Dynamic Authority v1.9 SHA-256: `7678a0b145f8721347cdcc5177fedb720ba19b91bf34984a3f18bd8216408098`
- source-bound P3 transform SHA-256: `2105712a1be9950089976ba358afd4b062347680d5e6d8c500462c2e8d541533`
- reference `L0 = 0.12564234435558320`.

These are frozen downstream references for their validated source and are not automatically transferred to the newly investigated Camera-5 payload domain.

## Repository/governance rules

- Use `docs/DOCUMENT_STATUS_INDEX_2026-09-17.md` for current-vs-historical interpretation.
- Preserve the 2026-09-16 index/current supplement as historical snapshots where later Camera-5 evidence supersedes their open physical gate.
- Do not rewrite v0.14 history to pretend it already knew the v0.19/v0.20 payload topology.
- v0.15 direct-open Camera-5 remains rejected-route provenance.
- v0.18 focus remains isolated parallel research.
- APK/GCam/computational-RAW material does not determine source authority, calibration, noise, topology or colour truth.
- No silent license changes.
- `canonical/ptc/v1.1` means **Pure Truth Certificate**.

## Immediate continuation

1. keep v0.20 as immutable experimental control;
2. run the first single-variable upstream vendor-route differential only after runtime type verification;
3. continue direct `.rawpayload` bit/CFA/row/column/noise topology analysis without promoting it to native ADC geometry;
4. obtain black/white/noise/shading/colour/optics calibration separately for any readout domain that may eventually be admitted;
5. resume physical focus/multi-camera research only with per-observation sealing and explicit authority counts;
6. keep Scientific Master, Dynamic Authority, TruthRange, HDR, restoration and appearance downstream from correctly established source topology.

v0.71 CI run `35506676249` = **SUCCESS** on GCC, Clang, scientific contracts and Android. Documentation governance run `35506676288` = **SUCCESS**. Artifact ID `10604525855`; APK SHA-256 `5653f33c5bbf3263473ee89cfc70e1d0807ce0a529b258b61e3325b791700e46`.
