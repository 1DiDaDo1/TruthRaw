# Precision-Independent Scientific Master v0.1 — real evidence report

**Date:** 2026-09-15  
**Status:** RESEARCH / REAL-FILE NUMERICAL AUDIT  
**Authority:** `NUMERICAL_EXECUTION_AUDIT_ONLY_NO_EVIDENCE_UPGRADE`

## Sources

Two real HONOR 4080x3072 DNGs were audited. Source identity was verified by SHA-256 before analysis.

### A. MotionCam Direct-CFA

- file: `IMG_260816_134122_304_005.dng`
- SHA-256: `04068eabc681f241b9fa102e46b27b964a636be13361e1d8bacabe0db60471e5`
- CFA: BGGR
- WhiteLevel: 1023
- BlackLevel 2x2: `[64.0, 63.75; 64.0, 64.0]`
- samples: 12,533,760 uint16 codes
- OpcodeList2: four GainMap opcodes; all 884 gain values are exactly `1.0`

Because the GainMaps are unity and no BlackLevelDeltaH/V tags were observed, the audited black/white normalization is also equivalent to this file's Stage-2 result after GainMap multiplication.

### B. HONOR vendor-tele prospective holdout

- file: `IMG_BNC_TRUTHRAW20260906_151349_043.dng`
- SHA-256: `7f64a628ff431272fc76f17b63179c93af8e496c40cdbd3b45d7dd67fd58b344`
- CFA: BGGR
- WhiteLevel: 1023
- BlackLevel 2x2: `[64.0, 63.75; 63.98, 63.75]`
- samples: 12,533,760 uint16 codes
- OpcodeList2: four non-unity GainMaps, observed gain range approximately `1.0009765625 .. 2.365234375`

For this second source, v0.1's real-file result below is intentionally **pre-GainMap normalization only**. The C++ compatibility adapter already supports a decoded `gainField`; the next real-evidence gate is to compare the full decoded non-unity GainMap path in F32 versus F64.

## Method

The same exact integer CFA codes were evaluated with the historical Stage-2 black/white equation in two numerical execution modes:

- genuine float32 arithmetic;
- float64 arithmetic.

No CFA samples were changed. No new evidence was introduced. The difference is purely arithmetic representation error.

The numerical difference was also compared to the per-channel DNG `NoiseProfile` model. This is not used to claim that the DNG model is FULL_PHYSICAL noise truth; it is used only as the source-declared uncertainty scale for judging whether arithmetic error is materially large.

## Results

| source | max F32-vs-F64 normalized error | max DN-equivalent error | max error / modeled sigma |
|---|---:|---:|---:|
| MotionCam Direct-CFA | `2.9771245913e-08` | `2.8550624831e-05 DN` | `5.7257684634e-06 sigma` |
| HONOR vendor tele, pre-GainMap | `4.5386840619e-08` | `4.3526887891e-05 DN` | `2.2400328613e-06 sigma` |

MotionCam Direct-CFA region checks:

- below black: max ~`1.61e-08 DN`;
- <=4 DN above/below black: max ~`8.75e-08 DN`;
- normalized midtones 0.1..0.9: max ~`2.86e-05 DN`;
- normalized >=0.9: max ~`2.83e-05 DN`;
- codes at WhiteLevel: F32/F64 difference exactly zero in this comparison.

## Decision

### Stage-2 black/white normalization

For these two real 10-bit-range HONOR sources, float32 arithmetic is supported as a **hot-path candidate** for this specific operation because the observed arithmetic error is many orders below the source-declared modeled measurement uncertainty.

This is deliberately not a statement that float32 is universally sufficient for TruthRaw.

### Calibration, optimization and covariance

Keep float64 as the scientific reference/default for:

- calibration fitting;
- covariance propagation;
- large reductions/accumulators;
- matrix optimization;
- numerical reference implementations.

The arbitrary-precision oracle already showed materially lower arithmetic error for float64 than float32 in deterministic covariance tests.

### Higher precision

Float128/arbitrary precision remains a **reference validator**, not a per-pixel production default and not an evidence upgrade.

## Precision-selection principle established by real evidence

TruthRaw should not choose precision from ideology (`higher is always better`) or from performance alone (`float32 is always enough`).

The rule becomes:

`choose the cheapest precision whose measured numerical error is safely below the physical/model uncertainty budget of that operation`

while preserving a higher-precision reference path.

For the tested black/white Stage-2 normalization, F32 currently satisfies that rule. For covariance/calibration, F64 remains preferred. Later reconstruction stages remain **UNTESTED** under this gate.

## Next gate

1. Decode the real non-unity HONOR vendor GainMaps through the admitted DNG path.
2. Run identical gain-field Stage-2 tiles in F32 and F64.
3. Add a double-precision reference implementation of the measured-preserving reconstruction/demosaic stage without modifying frozen canonical v4.7i.
4. Compare reconstruction errors against propagated scene/model uncertainty, not merely against F64 numerically.
5. Only after that define the runtime precision policy for 200MP tiles.

Machine-readable evidence: `evidence/REAL_DNG_STAGE2_PRECISION_AUDIT_v0_1.json`.

**Precision changes arithmetic accuracy, not epistemic authority.**
