# Dark Chroma Stability v0.4 — selective main/wide device evidence

Date: 2026-09-29  
Capture: `DRAW_CAPTURE_1790637640867_wide_main_4096x3072.dng`  
User evidence package: `DRAWmainwide4.zip`  
Evidence package SHA-256: `0a53fb86f6a11e62adfacc2b262fd479371c53230b1a59508b855049eab09074`

## Why this case matters

This is the selective regression that v0.4 was waiting for.

The previous two device cases established:

1. near-black tele: 432/432 visible Dark-Chroma candidates and global
   `DARK_UNINFORMATIVE`; local N2 binding must never override that block;
2. structured ultra-wide: 0/432 visible Dark-Chroma candidates; local binding
   must not invent any candidate.

This main/wide source lands between those extremes: a structured scene with a
small, non-zero Dark-Chroma candidate set.

## Source / frontside result

Observed in Universal Intake:

- raster: `4096x3072`
- focal length: `6.55 mm`
- ISO metadata: `4325`
- CFA: `[2,1,1,0]`
- 16-bit, Compression=1, SamplesPerPixel=1, STRIPS
- WhiteLevel: `1023`
- BlackLevel values around `64.00 .. 64.03`
- frontside analysis: `384x287`
- edgeDensity: `0.1161`
- entropy: `4.28`
- naturalGeometryCandidate: `true`

Dark Chroma v0.1:

- tiles: 432
- dark tiles: 74
- flat-dark tiles: 25
- visible chroma candidates: 6
- structure-veto tiles: 136
- candidateApplied=false

This is the first v0.4 device case with a sparse non-zero visible candidate set.

## Backside Signal Support v0.1

Observed:

- state: `MEASURED_SIGNAL_PRESENT_OR_MIXED`
- sample count: `12288`
- p50: `0.05002`
- p90: `0.11363`
- p99: `0.19917`
- fraction <= 0.01: `0.112`

The conservative near-black blocker does not fire.

## Dark Chroma v0.2 / v0.3

v0.2:

- state: `FRONTSIDE_INFORMATION_PRESENT`
- visible instability: 6
- dark-uninformative: 5
- backside-pending: 6
- correction-supported: 0

v0.3:

- state: `FRONTSIDE_INFORMATION_PRESENT`
- degenerate=false
- backsideNearBlack=false
- visible=6
- dark-uninformative=5
- backside-pending=6
- correction-supported=0

Degeneracy factors:

- dark fraction ~= `0.171`
- visible-candidate fraction ~= `0.014`
- structure fraction ~= `0.315`
- edgeDensity = `0.1161`

The global degeneracy gate correctly remains off.

## N2 Local Spatial Binding v0.1

Observed:

- frontside-bound = `432 / 432`
- visible-bound = `6`
- structure-blocked = `6`
- censor-blocked = `0`
- all-predictable = `0`
- center-outlier-free = `0`
- pair-free = `0`
- scale-free = `0`
- strict-vector = `0`

Dark Chroma v0.4:

- global-v0.3 = `FRONTSIDE_INFORMATION_PRESENT`
- localBinding = `true`
- visible = 6
- locally-bound = 6
- dark-blocked = 0
- protection-blocked = 6
- strict-vector = 0
- correction-supported = 0

The geometric/source binding therefore works in the selective case: all six
visible candidates are locally bound. No correction is promoted.

## N2 sidecar evidence

All four N2 sidecars in the package bind to the same lineage.

Source SHA-256 recorded by N2:

`93417851e6375b5803e1d5e19b8dcb50b1415356b07a7b9ac0aac66eece0bf77`

Scientific Master lineage SHA-256:

`7aa4a83b1ea6637d486809a828b53968281be7abba13cbf987e566421ec70171`

TruthNegative state SHA-256:

`18d6b1ec87e32190a455989f057cd90e9841db344ef43712cc088ce85acc3999`

N2 Spatial Audit v0.1:

- sampled: 786432
- candidate-corrected: 652389 (~82.96%)
- preserved: 134043
- structure-protected samples: 130240 (~16.56% of sampled points)
- censored-protected: 89
- censor-boundary-protected: 91
- removed residual energy fraction: `0.048470865416`
- max abs Stage-2 candidate correction: `0.006823039105`
- candidate_applied=false
- scientific_writeback_allowed=false

N2 Center-Excluded v0.2.1:

- predictor coverage: `638439 / 652389 ~= 0.97862`
- pair acceptance: `7085051 / 7819656 ~= 0.90606`
- scale acceptance: `1378673 / 1951594 ~= 0.70643`
- mean abs residual: `0.007304103899`
- max abs residual: `0.363971341186`
- max directional disagreement: `26.836657304411 sigma`
- max cross-scale disagreement: `16.12349432359 sigma`
- noise_independence_admitted=false
- candidate_applied=false

N2 Confidence Field v0.3:

- candidate fraction: `0.829555511475`
- predictor coverage: `0.978617052096`
- pair acceptance: `0.90605660914`
- scale acceptance: `0.706434330091`
- mixed tiles: 3072
- fully coherent tiles: 0
- promotion_eligible=false

N2 Factored Confidence v0.3.1:

- tile count: 3072
- has candidates: 3072
- all-candidates-predictable tiles: 63
- center-outlier-free tiles: 17
- predictable AND center-outlier-free: 2
- pair-rejection-free: 0
- scale-rejection-free: 0
- structure-protection-present tiles: **3072 / 3072**
- censor-protection-present tiles: 16
- censor-boundary-protection-present tiles: 23
- max-predictor-variance <= center-variance tiles: 3068
- promotion_eligible=false
- candidate_applied=false
- scientific_writeback_allowed=false

## Important new finding: structure-presence is too coarse as a local veto

This main/wide test exposes a limitation that the tele test already hinted at.

Only about **16.56% of sampled source points** are structure-protected, yet the
Factored Confidence field marks **structure-protection-present in all 3072
64x64 N2 tiles**.

The current v0.4 local binding uses a conservative tile-presence rule:
if any overlapping N2 tile reports structure protection, the frontside region
is protection-blocked.

Because every N2 tile reports structure presence, all six selective Dark-Chroma
candidates become structure-blocked.

That result is safe, but it is not spatially discriminative enough to justify
the next correction gate.

This is not evidence that all six frontside candidates themselves are
structure. It proves only that their overlapping 64x64 N2 tiles contain at
least some protected source samples.

Therefore the next research step must **not** be private chroma A/B/Delta yet.

The next gate is now:

`LOCAL_STRUCTURE_PROTECTION_RESOLUTION_REFINEMENT`

It must preserve the existing protection law while resolving structure support
at a finer spatial level, for example by binding protected sample/core geometry
or conservative distance-to-protected-support rather than using only an
any-protected-sample-per-64x64-tile boolean.

No implementation choice is promoted by this evidence document; the required
scientific property is simply that valid structure remains protected without
turning a globally sparse protection mask into a universal local veto.

## Scientific Master package check

The supplied full-colour Scientific Master is:

- primary raster: `4096x3072`
- RGB Float32
- all values finite
- observed minimum: about `-0.02766565`
- observed maximum: about `1.38000798`
- mean: about `0.05898074`
- negative RGB components: 456708 / 37748736 (~1.21%)
- components > 1: 3748

These are properties of the supplied Scientific-Master artifact, not claims
about untouched sensor ADC values.

## Package artifact hashes

- full-colour Scientific Master Float32 DNG:
  `e015c9284efe9ee181c80d59dac4af72db5ad4a213744e76a6cd486f14032506`
- PRO full-resolution JPEG:
  `5528411b8dca32376e7355ab5f68207056957cfc6d86ecb01acc0eeeb0fd0f65`
- N2 Factored Confidence v0.3.1 JSON:
  `20f5de794c647665897629eb4d5311c5a114b044eeededb222d087485deef3ba`

## Conclusion

The selective main/wide regression validates the **binding** part of v0.4:

- 432/432 frontside tiles bind;
- all 6 visible Dark-Chroma candidates bind to the same-observation N2 field;
- the source is non-degenerate and has measured signal;
- correction-supported remains 0;
- no Scientific-Master or D.RAWnegative writeback occurs.

It simultaneously shows that v0.4's current 64x64
`structure_protection_present` boolean is too coarse to serve as a selective
candidate-level structure veto because that flag is present in 3072/3072 N2
tiles while only ~16.56% of sampled source points are structure-protected.

The project should therefore keep v0.4 as an audit-only validated binding layer
and refine local structure-support resolution before defining any private
chroma correction candidate.
