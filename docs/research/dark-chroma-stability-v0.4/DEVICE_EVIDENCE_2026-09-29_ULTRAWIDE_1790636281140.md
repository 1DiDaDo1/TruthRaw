# Dark Chroma Stability v0.4 — Ultra-wide device evidence

Date: 2026-09-29  
Capture: `DRAW_CAPTURE_1790636281140_ultra_wide_4032x3024.dng`  
User evidence package: `DRAWultrawide4.zip`  
Evidence package SHA-256: `f12d8560ccf9fd4b48a284927297f1c88fe27c2825f6c46c6ffea298efc7f4d6`

## Scope

This is a real-device v0.4 regression using the normal ultra-wide physical route.

Only files belonging to capture ID `1790636281140` are admitted for this record.

The uploaded ZIP also contained two older tele files from capture
`1790609506655` (a Scientific Master DNG and PRO JPEG). Those are explicitly
out-of-scope for this ultra-wide observation and are not used in any metric,
binding or conclusion below.

## Universal Intake / frontside result

Observed source properties:

- raster: `4032x3024`
- focal length: `1.82 mm`
- ISO metadata: `4851`
- CFA: `[2,1,1,0]`
- 16-bit, Compression=1, SamplesPerPixel=1, STRIPS
- WhiteLevel: `1023`
- BlackLevel values around `64.10 .. 64.35`
- frontside analysis: approximately `384x287`
- edgeDensity: `0.1213`
- entropy: `3.86`
- naturalGeometryCandidate: `true`

Dark Chroma v0.1:

- tiles: 432
- dark tiles: 1
- flat-dark tiles: 0
- visible chroma candidates: 0
- structure-veto tiles: 91
- candidateApplied=false

This is a useful non-dark/non-degenerate contrast case.

## Backside Signal Support v0.1

Device output:

- state: `MEASURED_SIGNAL_PRESENT_OR_MIXED`
- sample count: `12096`
- p50: `0.08958`
- p90: `0.20321`
- p99: `0.33783`
- fraction <= 0.01: `0.036`

This does not trigger the conservative near-black blocker.

## Dark Chroma v0.2 / v0.3

v0.2:

- state: `FRONTSIDE_INFORMATION_PRESENT`
- visible instability: 0
- dark-uninformative: 0
- backside-pending: 0
- correction-supported: 0

v0.3:

- state: `FRONTSIDE_INFORMATION_PRESENT`
- degenerate=false
- backsideNearBlack=false
- visible=0
- dark-uninformative=0
- backside-pending=0
- correction-supported=0

Degeneracy factors:

- dark fraction ~= `0.002`
- candidate fraction = `0.000`
- structure fraction ~= `0.211`
- edgeDensity = `0.1213`

This confirms the v0.3 global degeneracy blocker does not fire on a structured
ultra-wide observation.

## N2 local spatial binding v0.1

Device output:

- frontside-bound = `432 / 432`
- visible-bound = `0`
- structure-blocked = `0` among visible Dark Chroma candidates
- censor-blocked = `0`
- all-predictable = `0`
- center-outlier-free = `0`
- pair-free = `0`
- scale-free = `0`
- strict-vector = `0`

Important interpretation: the zero blocked/predictable counts above are scoped
to visible Dark Chroma candidates. This observation has zero such candidates,
so the local-binding layer correctly proves geometry/source binding without
inventing a chroma-correction candidate.

Dark Chroma v0.4:

- global-v0.3 = `FRONTSIDE_INFORMATION_PRESENT`
- localBinding = `true`
- visible = 0
- locally-bound = 0
- dark-blocked = 0
- protection-blocked = 0
- strict-vector = 0
- correction-supported = 0

## N2 factored / confidence evidence

All N2 sidecars are bound to the same source/Scientific Master/authority/
D.RAWnegative lineage.

Source SHA-256 recorded by N2:

`6717d421e977f03738a2031b5f6782e89889db5708bd4784772f64175cf02008`

Scientific Master lineage SHA-256:

`23bf725fe6f3508f4e5d585867d55e83d1d2db91abb2fe73656407d18ef31a36`

TruthNegative state SHA-256:

`fd0c5c67d464e142376e5e9b9dee04bea4735e31dea90ba62a546c9834e71631`

N2 Spatial Audit v0.1:

- sampled: 762048
- candidate-corrected: 581454
- preserved: 180594
- structure-protected: 176974
- censored-protected: 6
- censor-boundary-protected: 5
- removed residual energy fraction: `0.042240340634`
- max abs Stage-2 correction candidate: `0.03999170746`
- candidate_applied=false
- scientific_writeback_allowed=false

N2 Center-Excluded v0.2.1:

- predictor coverage: `558574 / 581454 ~= 0.96065`
- pair acceptance: `6141236 / 6969643 ~= 0.88114`
- scale acceptance: `1081103 / 1741741 ~= 0.62070`
- mean abs residual: `0.034465907039`
- max abs residual: `0.599089157052`
- max directional disagreement: `15.044853396314 sigma`
- max cross-scale disagreement: `9.527850271916 sigma`
- noise_independence_admitted=false
- candidate_applied=false

N2 Confidence Field v0.3:

- candidate fraction: `0.763014928193`
- predictor coverage: `0.960650369591`
- pair acceptance: `0.881140683963`
- scale acceptance: `0.620702503989`
- mixed tiles: 3024
- fully coherent tiles: 0
- promotion_eligible=false

N2 Factored Confidence v0.3.1:

- tile count: 3024
- has candidates: 3024
- all-candidates-predictable tiles: 3
- center-outlier-free tiles: 9
- predictable AND center-outlier-free: 0
- pair-rejection-free: 0
- scale-rejection-free: 0
- structure-protection-present tiles: 3024
- censor-protection-present tiles: 1
- censor-boundary-protection-present tiles: 2
- max-predictor-variance <= center-variance tiles: 3020
- promotion_eligible=false
- candidate_applied=false
- scientific_writeback_allowed=false

## Exported artifact hashes from the evidence package

Container/file SHA-256 values computed from the uploaded package:

- ultra-wide Float32 Scientific Master DNG:
  `3bc021cfbe4427a23bd91ee6795380d4ca9d6ca9412420e1a47ee1fe85d771db`
- ultra-wide PRO full-resolution JPEG:
  `9ff0e1d064f45aec2321ac77660f93db7620329ce4d211d285e9df82d3f1566a`
- N2 Factored Confidence v0.3.1 JSON:
  `d18e0833fa11dd8e5c0f347720125439c9c68415f293c3ebe63ec5e55ad361ab`

These file hashes are package-artifact hashes and must not be confused with
internal source/payload/scientific lineage hashes stored inside D.RAW sidecars.

## Conclusion

This ultra-wide test validates a second important v0.4 case:

1. local frontside-to-N2 spatial binding succeeds for all 432 frontside tiles;
2. a structured/non-dark observation remains non-degenerate;
3. zero visible Dark Chroma candidates remain zero locally-bound correction
   candidates;
4. no local N2 metric is promoted into a scalar probability;
5. correction-supported remains zero;
6. Scientific Master/D.RAWnegative remain immutable;
7. no AI/ML or multi-observation information is used.

Together with the near-black tele regression, v0.4 now has device evidence for
both:

- a globally dark-uninformative case that remains blocked; and
- a structured ultra-wide no-candidate case that remains non-degenerate.

The still-missing high-value regression is the selective dark main/wide source
with a small non-zero Dark Chroma candidate set.
