# TruthNegative N2 1:1 A/B/Δ Crop Diagnostics v0.1

Status: **DISPLAY-ONLY VALIDATION — NO SCIENTIFIC WRITEBACK**

This diagnostic extends the low-resolution N2 Appearance A/B experiment with
source-native 1:1 crops. It is intended to reveal fine texture, edge behavior,
highlight/censor protection and the spatial signature of the current N2
candidate before any production appearance denoise is considered.

## Automatic crop selection

A balanced 1/16 whole-frame N2 audit first selects three 64x64 source tiles:

1. a quiet/noise-candidate tile with strong candidate support and penalties for
   structure/censor content;
2. a structure tile with the highest structure-protected count;
3. a censor/highlight tile with the highest CENSORED + censor-boundary count.

A 192x192 source-native crop is centered around each selected tile and clamped
inside the admitted source geometry.

## Full-lattice crop audit

Each diagnostic crop is then re-audited with `samplingPeriod=2`. Because the
CFA phase condition is source-coordinate aware, this covers every CFA site in
the bounded crop. The bounded audit is still read-only and uses the same
Stage-2 NoiseProfile / structure gate / authority-aware neighborhood / bounded
residual candidate pipeline.

The crop result records candidate, preserved, structure, CENSORED,
censor-boundary, residual-outlier and no-neighborhood protections, plus
removed residual energy and maximum proposed Stage-2 correction.

## A / B / Delta

- **A** is the unmodified 1:1 TruthNegative/Deep Scene/Appearance observation
  at the source coordinate.
- **B** is a temporary display-only copy. Only the N2 correction belonging to
  the physically measured CFA channel at that source site is added to the copy.
- **Delta** is `abs(B-A)` in encoded display RGB with a fixed x32 diagnostic
  gain. Black therefore means no visible A/B difference.

B is deliberately **not** described as a reconstructed denoised RAW. The two
non-measured colour components are not silently synthesized from the modified
CFA candidate. This makes the 1:1 view a conservative diagnostic of where the
current measured-CFA correction acts, rather than a claim about the final
production denoise appearance.

## Authority boundary

Before B is passed to Appearance/Display resolve, its scientific authority,
uncertainty and visibility summary are cleared and it receives a separate
candidate-scene identity bound to the N2 crop correction-grid SHA-256.

Permanent invariants:

- Direct CFA source is immutable;
- Scientific Master is immutable;
- TruthNegative state is immutable;
- A remains the primary scientific Appearance reference;
- B and Delta are transient diagnostic UI rasters;
- `createsNewEvidence=false`;
- `scientificWritebackAllowed=false`;
- no candidate is written into any export primary.

The Android PRO UI exposes the experiment through
`N2 · 1:1 A/B/Δ cropdiagnose`.
