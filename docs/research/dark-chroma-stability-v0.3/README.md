# D.RAW Dark Chroma Stability v0.3 — degeneracy + measured backside signal support

Date: 2026-09-28

## Why v0.3 exists

Real-device testing exposed a weakness in the v0.2 information gate.

The same near-black tele source class produced a new frontside run with:

- 432 / 432 tiles dark;
- 432 / 432 tiles flat-dark;
- 432 / 432 v0.1 visible chroma-instability candidates;
- 0 structure-veto tiles;
- edge density about 0.0040;
- entropy about 1.54 bits.

v0.2 used an entropy threshold of 1.50 as one hard condition. A change of about 0.04 bits therefore changed the global state from the intended dark-uninformative interpretation to FRONT_SIDE_INFORMATION_PRESENT, even though every tile was still dark/candidate and no structure veto existed.

The scientific safety boundary remained intact: correction-supported stayed zero and candidateApplied stayed false. The problem was classification robustness, not scientific writeback.

v0.3 fixes the classification without rewriting v0.1 or v0.2.

## 1. Frontside degeneracy gate

The new gate asks whether the frontside analysis itself has stopped being discriminative.

Current conservative blocking factors are serialized as RESEARCH/CONSERVATIVE blocking heuristics:

- dark tile fraction >= 0.98;
- visible chroma-instability candidate fraction >= 0.98;
- structure-protected fraction <= 0.02;
- edge density <= 0.010.

Entropy is retained as a diagnostic measurement but is **not a hard gate**.

For the real-device near-black tele pattern:

```
dark = 432 / 432 = 1.0
candidate = 432 / 432 = 1.0
structure = 0 / 432 = 0.0
edgeDensity ~= 0.0040
entropy ~= 1.54  # diagnostic only
=> DARK_UNINFORMATIVE_BY_DEGENERACY
```

For the earlier dark main/wide observation:

```
dark = 414 / 432 ~= 0.958
candidate = 22 / 432 ~= 0.051
structure = 40 / 432 ~= 0.093
edgeDensity ~= 0.0499
=> not globally degenerate
```

This is a blocking gate only. It cannot enable correction.

## 2. Backside Signal Support v0.1

A new read-only `BacksideSignalSupportAudit.kt` samples the actual selected DNG CFA payload when the topology is provably simple enough:

- classic TIFF/DNG;
- raw CFA IFD;
- Compression = 1;
- BitsPerSample = 16;
- SamplesPerPixel = 1;
- unsigned integer SampleFormat;
- FillOrder = 1;
- strip storage;
- BlackLevel and WhiteLevel available.

Unsupported or incomplete topology remains UNKNOWN/fail-closed.

No full RAW materialization is required. The audit reads sparse source rows and samples all four 2x2 parity phases on a deterministic 64-pixel grid.

Each measured code is normalized as:

```
(raw_code - black_level) / (white_level - black_level)
```

No clamp is applied. Negative values are preserved.

Reported measurements include:

- p01 / p10 / p50 / p90 / p99;
- mean and standard deviation;
- fraction at or below black;
- fraction <= 0.01 above black-normalized range;
- fraction <= 0.02;
- per-CFA-phase p50 and p90.

Authority is:

`SOURCE_PAYLOAD_MEASURED_WITHIN_SELECTED_DNG`

This is **not** a claim of untouched photodiode/ADC truth.

## 3. Conservative backside blocker

Backside Signal Support currently exposes only a conservative blocking classification:

`NEAR_BLACK_DOMINATED`

when both:

- at least 70% of sampled values are <= 0.01 normalized above black;
- p90 <= 0.02 normalized above black.

These thresholds are marked:

`CONSERVATIVE_BLOCKING_HEURISTIC_ONLY`

They may block hidden-colour reconstruction. They may never enable it.

No DNG NoiseProfile formula is interpreted in this version. NoiseProfile presence remains source metadata only.

## 4. Dark Chroma Stability v0.3 states

v0.3 can now produce global states including:

- `DARK_UNINFORMATIVE_BY_DEGENERACY`;
- `DARK_UNINFORMATIVE_BY_BACKSIDE_SIGNAL`;
- `FRONTSIDE_INFORMATION_PRESENT`.

Per-tile states include:

- `STRUCTURE_PROTECTED`;
- `DARK_UNINFORMATIVE_BY_GLOBAL_DEGENERACY`;
- `DARK_UNINFORMATIVE_BY_BACKSIDE_SIGNAL`;
- `DARK_UNINFORMATIVE`;
- `CHROMA_INSTABILITY_VISIBLE`;
- `NO_CHROMA_INSTABILITY`.

## 5. What v0.3 still does not do

v0.3 does not create a chroma correction.

Hard invariants remain:

- `candidate_applied = false`;
- `creates_new_evidence = false`;
- `scientific_writeback_allowed = false`;
- `replacement_colour_estimated = false`;
- `pixel_value_replacement_proposed = false`;
- `chroma_correction_supported = false`;
- private chroma A/B/Delta remains disabled.

A future correction still requires local N2/backside support bound to exactly the same observation and spatial region.

The global backside signal audit can block. It cannot promote.

## 6. Single-shot law

Unchanged:

```
one sealed observation
-> deterministic frontside views
-> source-payload backside measurement
-> structure/chroma/signal constraints
-> one D.RAWnegative diagnostic binding
```

No other physical lens, temporal frame, burst, AI, ML, neural, generative or learned model is used.

## 7. Version lineage

- v0.1: visible dark-chroma instability + structure veto; frontside only; audit-only.
- v0.2: first information-support gate; entropy-sensitive; audit-only.
- v0.3: robust degeneracy blocker + measured selected-DNG backside signal support; audit-only.

v0.1 and v0.2 remain present as predecessors and are not silently rewritten.
