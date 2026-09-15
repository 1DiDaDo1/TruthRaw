# TruthRaw precision + 200MP next-chat handoff — 2026-09-15

Status: **AUTHORITATIVE CONTINUITY HANDOFF FOR THIS RESEARCH BRANCH**

Branch: `research/history-code-audit-2026-09-15`

Handoff baseline before this document was written: `1ee1673030948245652c00e487f67299a08e0b3c`. The live branch HEAD after later documentation commits is the current repository state.

This handoff exists so a new chat does not need to reconstruct the precision work, the uncertainty blocker, the Free Scientific Space correction, or the 200MP continuation point from conversation history.

## 1. Current project vision — Free Scientific Space

The original RAW/CFA bytes and capture metadata are immutable **Source Evidence Record** material. They are not a numerical prison for TruthRaw.

TruthRaw reconstructs a separate **Free Scientific Scene Space**. Its representation may exceed the source in numerical precision, dynamic-range representation, colour coordinates, spatial representation and working datatype. The representation is not forced to inherit RAW10 limits, source WhiteLevel as an output ceiling, `[0,1]`, DNG container limits, the original CFA lattice as the only possible master lattice, or float32.

The evidential rule remains strict:

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

> **Measured where measured. Reconstructed where necessary. Never invented.**

The older sealed-house metaphor is historical provenance only. Preserve the source evidence, but do not describe the reconstructed Scientific Master as sealed inside the RAW.

## 2. Precision architecture now established

The current research architecture is:

`exact integer/packed RAW evidence`

`-> stage-specific working precision`

`-> F64 branch-sensitive reconstruction reference`

`-> F64 calibration / reductions / optimization / covariance`

`-> controlled F32 storage only after F64 compute where admitted`

`-> arbitrary/high precision reference oracle for validation`

Important: numerical precision is an execution/scientific-numerics property, never evidence authority. Higher precision creates no new photons and cannot upgrade reconstructed data to measured data.

### 2.1 Exact evidence

Original integer/packed CFA data remain exact evidence. Do not rewrite source evidence as floating-point merely because later processing uses floating point.

### 2.2 Stage-2 normalization

For the tested 4080x3072 real RAW sources, F32 black/white normalization remained far below the physical/model uncertainty. It is therefore a **hot-path candidate in the tested scope**, with F64 retained as reference. This is not a universal permission for every future source or algorithm.

### 2.3 DNG GainMap

For the tested HONOR vendor GainMap family, SDK-equivalent F32 interpolation versus F64 interpolation also remained far below the physical/model uncertainty. The stored map samples themselves remain the authority; F64 must not pretend to invent more precise GainMap measurements than the DNG supplied.

### 2.4 Branch-sensitive reconstruction

This is where the precision decision changed materially.

The v4.7i-class reconstruction contains direction/branch decisions around a `0.72` threshold. Tiny F32/F64 input differences can cross that threshold and select different reconstruction paths. Once the wrong branch is selected, casting the result to double cannot recover the lost decision.

Therefore:

- `F32 compute -> cast to F64 later` is **not admitted** as scientific reference for the tested branch-sensitive reconstruction;
- F64 compute is required as the current scientific reference for that tested topology;
- measured CFA components remain protected; the large differences occur in reconstructed channels.

Three deterministic real-data C++ fixtures guard this hazard, including MotionCam Direct-CFA and HONOR vendor sources.

## 3. Authoritative eight-file mixed-precision gate

The authoritative retained v0.4 evidence is:

`docs/research/precision-independent-scientific-master-v0.1/evidence/MIXED_PRECISION_RECONSTRUCTION_GATE_v0_4.json`

Authority scope:

`EXACT_REPOSITORY_V0_4_REAL_FILE_EVALUATOR_ON_EIGHT_HASH_VERIFIED_SOURCES`

Aggregate authoritative result:

- files: `8`
- source classes: `2`
- direction-checked sites: `50,135,040`
- direction differences: `3,545`
- green clamp differences: `0`
- colour clamp differences: `0`
- measured-channel violations: `0`
- qualified reconstructed samples: `300,810,240`
- maximum F32-vs-F64 reconstruction difference: `0.03754056890225277`
- maximum branch amplification factor: `204871.6928905374`
- maximum F64-compute -> F32-storage absolute error: `5.960464477539063e-08`
- storage half-ULP violations: `0`

Worst source:

`IMG_BNC_TRUTHRAW20260906_142950_144.dng`

SHA-256:

`f640875800adf4aeca131dabe0845f10bbda07886396d562ca16324421e9a3ce`

Worst coordinate:

`(332, 2164)`

### 3.1 Resolved discrepancy

An older/provisional status block contained `3549 / 44 / 2580` for direction/green-clamp/colour-clamp divergence. Those numbers are **not authoritative retained evidence**.

`DISCREPANCY_AND_UNCERTAINTY_BINDING_RESOLUTION_v0_9.md` resolved this by rerunning the retained v0.4 evaluator against all eight hash-verified sources and reproducing the authoritative evidence artifact. The correct retained result is:

`3545 / 0 / 0`.

Treat `3549 / 44 / 2580` only as superseded provisional locator/instrumentation output, never as the current gate result.

## 4. F64 compute -> F32 Scientific-Master storage

The eight-file gate supports a clear distinction between **compute precision** and **storage precision**.

F32 compute can alter branch decisions and generate large local reconstructed-channel differences. In contrast, after the reconstruction has been computed in F64, storing the completed Scientific-Master value as F32 produced a maximum tested absolute storage error of only `5.960464477539063e-08`, with no half-ULP violations in the retained v0.4 gate.

Current interpretation:

- F64 compute is required for the tested branch-sensitive reconstruction reference;
- F32 storage after F64 compute has a **tested numerical pass in the 4080x3072 mixed-precision scope**;
- this is not yet a universal canonical promotion;
- uncertainty-relative end-to-end admission and 200MP admission remain separate gates.

This separation is especially important for 200MP because it allows tiled F64 compute without requiring repeated full-frame F64 RGB allocations.

## 5. Uncertainty state — exact boundary

### 5.1 v0.7 uncertainty-relative storage gate

v0.7 is a PASS only for a **synthetic analytic camera-space sigma reference**. It shows that the tested F64->F32 storage error is tiny relative to that analytic sigma model, but it does **not** certify canonical per-pixel v5.0g/p1 uncertainty binding.

Do not promote this synthetic result to canonical local uncertainty.

### 5.2 v0.8 Scientific-Master conversion audit

v0.8 correctly exposes:

- pre-storage F64 Scientific-Master values;
- post-storage F32 values;
- reconstruction trace/provenance.

It intentionally reports `uncertaintyApplied=false` / `NOT_BOUND`.

This fail-closed state is correct.

### 5.3 v0.9 native uncertainty runtime

A v0.9 research runtime/evaluator exists and passes its current host tests. It can evaluate the frozen model specification when supplied with the required feature vector and has same-quantity quantile semantics. Its real-input smoke test used a **synthetic proxy feature/floor** and therefore does not constitute canonical reconstruction-feature binding.

### 5.4 The remaining canonical uncertainty blocker

The frozen v5.0g/p1 model expects exactly 18 historical features:

1. `mean_sigma`
2. `mean_abs_demosaic_extra`
3. `std_abs_demosaic_extra`
4. `p90_abs_demosaic_extra`
5. `max_abs_demosaic_extra`
6. `mean_unseen_color_error`
7. `p90_unseen_color_error`
8. `max_unseen_color_error`
9. `mean_abs_r`
10. `mean_abs_g`
11. `mean_abs_b`
12. `std_r`
13. `std_g`
14. `std_b`
15. `support_std_over_sigma`
16. `spread_p90_over_sigma`
17. `spread_max_over_sigma`
18. `propagated_spread_over_sigma`

The canonical manifest records a historical extractor named `uncertainty_core_v5_0g.py` and its hash, but that extractor is absent from the current repository tree. Direct recovery attempts from the current branch, the archived 4f842d8f state, and the older project-handoff commit did not recover the file at its expected path.

The current reconstruction precision trace contains branch/clamp provenance and RGB outputs but is **not sufficient to reconstruct all 18 historical hidden-CFA/support-spread features with proven identical semantics and no leakage**.

Therefore the blocker is:

`OPEN_NEEDS_EXACT_V5G_FEATURE_EXTRACTOR_RECOVERY_OR_HASH_VERIFIED_EQUIVALENT_FEATURE_DEFINITION`

Do **not** invent an approximate adapter. Do not call the v0.9 runtime canonically bound until the exact feature semantics are recovered and verified.

Required recovery order:

1. recover the exact historical extractor bytes from a retained artifact, old repository object/branch, handoff archive or Library source;
2. verify the recovered bytes against the hash recorded by the canonical manifest;
3. inspect and freeze the exact output-coordinate and leakage semantics;
4. only then implement the F64-reconstruction -> 18-feature adapter;
5. evaluate v5.0g/p1 locally only where all required inputs exist;
6. preserve unresolved/unknown samples as unresolved rather than synthesizing missing features;
7. bind those canonical uncertainty outputs into the v0.8 pre/post-storage probe;
8. re-evaluate F64->F32 storage error relative to the actual admitted local uncertainty.

Until then, canonical uncertainty binding remains OPEN.

## 6. 200MP / FotoGraaf continuation point

The next physical acquisition experiment remains the real Camera-5 maximum-resolution capture.

Known static route:

`logical camera 0 -> physical camera 5 -> RAW_SENSOR maximum-resolution route`

The route authority is **v0.8**, not the older standalone v0.7 direct-open assumption.

Static capability/session support is not frame proof.

The physical Step 3B capture must establish at minimum:

- requested and **applied** `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION`;
- `OutputConfiguration` bound to physical camera `5` where required;
- matching physical `TotalCaptureResult` for camera 5;
- delivered `RAW_SENSOR` image of `16320x12288`;
- `Image.timestamp == SENSOR_TIMESTAMP` binding;
- exact row stride, pixel stride, buffer length and padding semantics;
- exact original `.rawsensor` payload hash;
- CFA arrangement;
- dynamic/static black and white information as available;
- NoiseProfile as available;
- ISO, exposure time, focal/focus state, stabilization state and relevant capture controls;
- DNG only as a derived convenience container from the same bound Image/Result pair.

The initial successful classification should be bounded, e.g.:

`APP_VISIBLE_MAXIMUM_RESOLUTION_RAW_SENSOR_CFA_PROVEN`

Do **not** automatically claim:

`UNTOUCHED_NATIVE_200MP_ADC`

because Camera2 already reports `SENSOR_INFO_LENS_SHADING_APPLIED=true`, and app-visible maximum-resolution RAW is not automatically proof of untouched photodiode/ADC output or untouched optical falloff.

`SENSOR_INFO_BINNING_FACTOR` (`Size`) and `SENSOR_RAW_BINNING_FACTOR_USED` (optional Boolean result key) must not be conflated. Absence of the optional result key is not by itself a capture failure for this HONOR route.

## 7. Precision plan for 200MP

Do not allocate repeated full-frame F64 RGB images merely because F64 is the reconstruction reference.

Use:

`exact 16320x12288 rawsensor evidence`

`-> tiled Stage-2`

`-> F64 compute inside branch-sensitive reconstruction tiles`

`-> F64 calibration/statistics/covariance`

`-> post-compute F32 storage only if the 200MP storage gate passes`

`-> arbitrary/high precision only for selected reference/oracle validation`

At 16320x12288, approximate storage for one scalar plane is:

- F32: `0.747 GiB`
- F64: `1.494 GiB`
- 16-byte values: `2.988 GiB`

Therefore tiling/streaming is a scientific execution requirement for practical mobile 200MP work; resource pressure must never weaken evidence/provenance rules.

The existing 4080x3072 mixed-precision PASS must be **repeated on a physically proven 16320x12288 source** before any 200MP precision promotion.

## 8. Calibration after physical 200MP proof

Treat 4080x3072, 8160x6144 and 16320x12288 as separate sample/readout domains until measured equivalence is proven.

For each mode, characterize independently:

1. dark/black behavior;
2. gain/readout/noise;
3. exposure linearity;
4. saturation/clipping;
5. temporal noise and spatial non-uniformity/PRNU/DSNU where measurable;
6. residual app-visible shading;
7. optical SFR/MTF across centre/mid/edge;
8. colour/illuminant response using independent calibrated targets/known spectra;
9. held-out repeatability.

Because `SENSOR_INFO_LENS_SHADING_APPLIED=true`, distinguish **app-visible residual shading** from uncorrected physical optical shading.

Do not equate 200.54 million samples with 200.54 million independent optical details. The 200MP programme must measure effective optical information via SFR/MTF and same-scene 12.5MP/50MP/200MP comparisons.

## 9. Authority map

### Proven / retained evidence

- exact source hashes and source evidence provenance;
- v0.4 eight-file precision gate and `3545 / 0 / 0` authoritative aggregate;
- deterministic C++ real-source branch-divergence fixtures;
- F64 branch-sensitive reconstruction reference for the tested topology;
- tested F64-compute -> F32-storage numerical pass in the 4080x3072 v0.4 scope;
- static Camera-5 maximum-resolution capability/session observations already retained by FotoGraaf research.

### Research/provisional

- v0.7 synthetic uncertainty-relative storage semantics;
- v0.9 uncertainty runtime before canonical feature binding;
- F32 hot-path candidacy for Stage-2/GainMap outside the tested source family;
- transfer of the mixed-precision policy to 200MP before a real 200MP payload exists.

### Explicitly not proven

- byte-for-byte recovery/parity of the missing historical v5.0g feature extractor/runtime;
- exact end-to-end generation of all 18 canonical v5.0g/p1 features from the current F64 reconstruction path;
- canonical per-pixel uncertainty binding to v0.8;
- universal mixed-precision safety for other reconstruction algorithms;
- physically delivered/bound `16320x12288 RAW_SENSOR` payload;
- untouched native 200MP photodiode/ADC semantics;
- FULL_PHYSICAL calibration for the 200MP mode;
- canonical promotion of this research branch.

## 10. Exact next order of operations

For the next chat, do not restart the history audit. Continue in this order:

1. Read this handoff, `START_HERE_NEW_CHAT.md`, the Free Scientific Space core vision, and `STATUS_v0_1.json`.
2. If possible, recover and hash-verify the missing historical `uncertainty_core_v5_0g.py` / exact feature semantics from retained project artifacts. Never recreate it from guesses.
3. If recovered, bind the exact 18-feature extractor to F64 reconstruction and then bind canonical uncertainty into the v0.8 pre/post-storage audit.
4. If not recoverable, leave that gate OPEN and do not block unrelated acquisition work.
5. Continue FotoGraaf Step 3B: obtain the real physical-ID5 `16320x12288 RAW_SENSOR` payload using the v0.8 logical0->physical5 route.
6. Seal payload/result/timestamp/stride/hash/CFA metadata before any reconstruction.
7. Run the precision suite on the real 200MP source: Stage-2, GainMap if applicable, branch-sensitive F64 reconstruction, F64->F32 storage, and high-precision spot/oracle tests.
8. Build independent 200MP noise/PTC, shading, colour and SFR/MTF calibration evidence before transferring models from lower-resolution modes.
9. Promote nothing to canonical merely because CI is green; require module-specific scientific admission and physical evidence.

## 11. Reading order for a fresh chat

1. `docs/TRUTHRAW_PRECISION_200MP_NEXT_CHAT_HANDOFF_2026-09-15.md`
2. `START_HERE_NEW_CHAT.md`
3. `docs/CORE_VISION_FREE_SCIENTIFIC_SPACE_ARCHITECTURE.md`
4. `docs/TRUTHRAW_EXPERTISE_FOUNDATION_FREE_SPACE_200MP_2026-09-15.md`
5. `docs/TRUTHRAW_EXPERTISE_200MP_ROUTE_v0_8_ADDENDUM_2026-09-15.md`
6. `docs/research/precision-independent-scientific-master-v0.1/STATUS_v0_1.json`
7. `docs/research/precision-independent-scientific-master-v0.1/DISCREPANCY_AND_UNCERTAINTY_BINDING_RESOLUTION_v0_9.md`
8. `docs/research/precision-independent-scientific-master-v0.1/evidence/MIXED_PRECISION_RECONSTRUCTION_GATE_v0_4.json`
9. relevant FotoGraaf/Camera2 v0.8 capture documentation before modifying acquisition code.

## 12. Final continuity rule

A new chat must not collapse the project to “make a 200MP camera app” and must not collapse it to “use float64 everywhere”.

The project goal remains a provenance-preserving, uncertainty-aware, physically calibrated reconstruction system in a **Free Scientific Space**. FotoGraaf proves and calibrates the origin of evidence. Precision is selected per numerical stage. Reconstruction may exceed source representation but never source evidential authority.
