# TruthRaw Mixed-Precision Promotion Gate v0.4

Date: 2026-09-15  
Status: **RESEARCH / NUMERICAL GATE / NOT CANONICAL PROMOTION**  
Authority: numerical implementation only; no photographic-evidence upgrade

## Decision

For the tested v4.7i-equivalent branch-sensitive reconstruction:

- **Float32 branch-sensitive compute: FAIL as scientific reference.**
- **Float64 branch-sensitive compute: PASS as the scientific numerical reference.**
- **Float64 compute -> Float32 output storage: PROVISIONAL NUMERICAL PASS for the tested 4080x3072 scope only.**
- Exact RAW/CFA evidence remains exact integer/packed evidence and is never replaced by either float representation.

This does not promote a new canonical Scientific Master and does not claim Float32 storage is universally safe.

## Why Float32 compute fails

The frozen v4.7i reconstruction contains a hard `0.72` directional decision. A tiny F32/F64 Stage-2 perturbation can cross that threshold and select a different reconstruction hypothesis. The resulting reconstructed-channel error can be many orders larger than the upstream arithmetic difference.

Three real deterministic C++ fixtures now retain this class of failure:

1. `IMG_BNC_TRUTHRAW20260906_151129_256.dng`, SHA-256 `f900c9d8911072530d43c532a02328c62ed1e380ab6659c36bc2ed0f2dc9b229`, `(1408,884)`: F32 `Vertical`, F64 `WeightedBlend`, reconstructed red difference about `1.95e-2`.
2. MotionCam Direct-CFA `IMG_260816_134122_304_005.dng`, SHA-256 `04068eabc681f241b9fa102e46b27b964a636be13361e1d8bacabe0db60471e5`, `(3623,95)`: F32 `WeightedBlend`, F64 `Horizontal`.
3. `IMG_BNC_TRUTHRAW20260906_142950_144.dng`, SHA-256 `f640875800adf4aeca131dabe0845f10bbda07886396d562ca16324421e9a3ce`, `(332,2164)`: F32 `Horizontal`, F64 `WeightedBlend`, reconstructed maximum error about `3.754e-2`.

CI run `34987242806` at commit `996a132e0ad2ea958205a9aa416afa3ba5967ac8` passed **8/8 C++ tests**, including all three real branch-hazard fixtures and the mixed-storage primitive.

## Eight-file full-frame host sweep

Two source classes were used: MotionCam Direct-CFA and HONOR vendor DNG. The sweep covered:

- **8 real files**;
- **100,270,080 CFA samples**;
- **50,135,040 red/blue branch-decision sites**;
- **3,549 direction divergences** between F32 and F64;
- maximum per-file direction-divergence rate: **0.030334%**;
- maximum F32-vs-F64 reconstructed RGB absolute difference: **0.0375405553384**;
- maximum reconstructed RGB RMS difference: **1.75119598753e-05**;
- measured-channel violations: **0** in both reference routes.

| Source | direction divergences | rate | max F32/F64 RGB abs | RGB RMS | max F64->F32 storage abs | storage RMS |
|---|---:|---:|---:|---:|---:|---:|
| `IMG_260816_134122_304_005.dng` | 1,901 | 0.030334% | 0.00411972809 | 6.11284298e-06 | 5.89519498e-08 | 2.67951503e-09 |
| `IMG_260816_134204_911_008.dng` | 1,509 | 0.024079% | 0.0156049084 | 1.75119599e-05 | 5.93457281e-08 | 3.47575563e-09 |
| `IMG_BNC_TRUTHRAW20260906_142935_568.dng` | 4 | 0.000064% | 0.0153143728 | 4.76272644e-06 | 5.95256338e-08 | 4.78758187e-09 |
| `IMG_BNC_TRUTHRAW20260906_142941_853.dng` | 23 | 0.000367% | 0.0374897109 | 6.29313200e-06 | 5.96043788e-08 | 6.74920706e-09 |
| `IMG_BNC_TRUTHRAW20260906_142950_144.dng` | 1 | 0.000016% | 0.0375405553 | 6.69512318e-06 | 5.93703742e-08 | 3.26990711e-09 |
| `IMG_BNC_TRUTHRAW20260906_151110_490.dng` | 12 | 0.000191% | 0.00250225620 | 7.17908137e-07 | 5.94172942e-08 | 2.81191296e-09 |
| `IMG_BNC_TRUTHRAW20260906_151129_256.dng` | 91 | 0.001452% | 0.0194820315 | 4.04756906e-06 | 5.96044865e-08 | 8.55403059e-09 |
| `IMG_BNC_TRUTHRAW20260906_151349_043.dng` | 8 | 0.000128% | 0.0130863943 | 3.75169070e-06 | 2.98023151e-08 | 6.52635747e-09 |

The full-file sweep is **locator/research evidence**, not independent photographic authority. Critical worst cases are frozen separately as C++ CI fixtures.

## Storage result

Across all eight files, F64-compute -> F32-storage produced:

- worst maximum absolute quantization error: **5.96044864576e-08** normalized;
- worst per-file RMS quantization error: **8.55403058687e-09** normalized.

This is drastically smaller than the largest errors produced by executing the branch-sensitive reconstruction itself in F32.

Therefore the preferred research architecture is now:

`exact RAW integer/packed evidence`
-> `validated F32 or F64 pre-reconstruction stages`
-> **`F64 branch-sensitive reconstruction`**
-> `F64 calibration / covariance / optimization`
-> **optional F32 Scientific-Master storage after F64 compute, only when its storage gate passes**
-> `higher-precision offline oracle where required`

## Provisional numerical budget

For v0.4 only:

- max absolute F64->F32 storage error <= `1e-7` normalized;
- RMS F64->F32 storage error <= `1e-8` normalized;
- zero measured-channel violations in the F64 compute reference;
- at least 8 real files across at least 2 source classes;
- all real C++ branch-hazard fixtures must remain green.

These are **engineering research limits**, not physical uncertainty limits and not evidence thresholds.

## What is not proven

This gate does **not** yet prove:

- F32 storage is below local covariance/noise at every pixel;
- Android runtime, memory or thermal fitness;
- the 16320x12288 maximum-resolution route;
- future reconstruction algorithms;
- canonical project promotion.

## 200MP consequence

The 200MP path inherits the doctrine, not the untested conclusion. Once FotoGraaf physically delivers and binds a real `16320x12288 RAW_SENSOR` payload, repeat:

1. exact integer/packed source preservation;
2. F32/F64 Stage-2 comparison;
3. GainMap/sample-domain comparison if applicable;
4. F64 branch-sensitive reconstruction;
5. F64->F32 storage quantization;
6. uncertainty-relative storage test;
7. tile-memory/runtime/thermal audit.

Until then, `F64 compute -> F32 storage` is only a provisional result for the tested 4080x3072 sources.

## Next gate

Propagate local uncertainty/covariance through the F64 reconstruction and compare F64->F32 storage quantization against the **local scientific uncertainty**. Storage should only progress beyond research when its numerical error is demonstrably negligible relative to that uncertainty, rather than merely passing a fixed absolute threshold.
