# TruthRaw Highlight Truth v0.1 — Experimental Evidence

**Status:** EXPERIMENTAL_NOT_CANONICAL

Core rule: **censored CFA samples are lower bounds, not exact latent radiance.** The guard is appearance-only and does not modify source CFA, Stage-2, the reconstruction master, or uncertainty state.

## Sun stress case

- Source: `IMG_BNC_TRUTHRAW20260906_151129_256.dng`
- Censored samples: **481,294 (3.83998%)**
- Bayer cells with any censoring: **4.66210%**
- Fully chroma-unsupported cells: **4.61394%**
- Synthetic BGGR mapping: **PASS** (`[3000, 2000, 1000]`)
- AsShotNeutral -> D50 max error: **1.11e-16**
- Linear luminance max |delta| after display-chroma guard: **1.79e-7**
- Preview area affected: **5.0877%**

Within the affected preview area, normalized linear chroma median changed from **1.0634** to **0.0**. This does not claim the true clipped highlight was neutral; it intentionally refuses to present an unsupported hue as measured truth.

## Four-scene activation regression

| File | censored samples | sample fraction | Bayer cells any censor |
|---|---:|---:|---:|
| `IMG_BNC_TRUTHRAW20260906_151110_490.dng` | 179 | 0.001428% | 0.002394% |
| `IMG_BNC_TRUTHRAW20260906_151129_256.dng` | 481,294 | 3.839981% | 4.662097% |
| `IMG_BNC_TRUTHRAW20260907_094414_423.dng` | 0 | 0.000000% | 0.000000% |
| `IMG_BNC_TRUTHRAW20260907_094449_565.dng` | 217 | 0.001731% | 0.002808% |

The black/white dog frame `094414_423` contains zero censored samples, so the source-defined guard is exactly inactive. The street and white-dog frames activate it only at trace levels.

## Promotion blockers

1. Full-resolution implementation and regression, not only preview-resolution evaluation.
2. Synthetic clipped-CFA boundary fixtures.
3. Colored-light-source validation.
4. Fully censored regions must remain explicitly non-identifiable in latent hue.

**No canonical TruthRaw reconstruction code is changed by this evidence record.**
