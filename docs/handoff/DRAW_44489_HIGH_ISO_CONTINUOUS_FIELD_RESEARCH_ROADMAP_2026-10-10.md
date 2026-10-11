# D.RAW 44489 — High-ISO + Continuous Scientific Field Research Roadmap — 2026-10-10

Status: **RESEARCH ROADMAP / DOCUMENTATION ONLY — NOT IMPLEMENTED, NOT PROMOTED**

This document turns the 2026-10-10 interdisciplinary audit into a falsifiable research sequence for high-ISO noise reduction, reconstruction and a representation richer than a fixed raster.

---

## 1. Permanent boundary

Do not define success as “looks cleaner”. Define success as:

**lower estimation error / better calibrated uncertainty on genuinely withheld measurements, while preserving measured anchors, structure and provenance.**

`clean image != perfect knowledge`

No candidate in this roadmap may become Scientific Master authority without its own promotion evidence.

---

## 2. High-ISO model

A RAW observation can contain multiple uncertainty sources, including photon statistics, read/electronic noise, dark/fixed-pattern components, quantization and later reconstruction uncertainty.

D.RAW must not assume these are a single additive RGB noise texture.

Preferred research chain:

`sealed CFA`
`-> admitted black/linearity/reliability`
`-> censor/UNKNOWN state`
`-> source-bound noise likelihood`
`-> local CFA structure + phase + optical support`
`-> deterministic latent-radiance candidate`
`-> explicit uncertainty`
`-> colour formation`
`-> Natural Appearance`

If the source does not identify noise parameters well enough, the correct state is wider uncertainty or UNKNOWN parameters — not invented coefficients.

---

## 3. Noise-likelihood milestone

Build research diagnostics before denoising:

- source/Scientific-Master binding;
- per-channel signal strata;
- dark/near-dark behavior where valid;
- variance versus signal diagnostics where repeated/held-out support permits;
- censoring exclusions;
- fixed-pattern/repeated-pattern diagnostics only if evidence supports them;
- residual distribution diagnostics;
- uncertainty calibration metrics.

EMVA 1288 photon-transfer concepts are a useful measurement reference, but D.RAW must not claim EMVA-style calibrated sensor parameters from a single uncontrolled frame.

Reference: https://www.emva.org/standards-technology/emva-1288/

---

## 4. Deterministic local model bank

Research-only candidates may include:

- constant/local robust estimate;
- affine/local plane;
- directional/edge-aware basis;
- local polynomial/spline basis;
- periodic/texture basis only when support is sufficient;
- `NO_SUITABLE_MODEL`.

Selection inputs may include:

- support count/geometry;
- directionality;
- CFA phase/channel;
- local gradients/curvature;
- censoring/UNKNOWN mask;
- source-bound noise likelihood;
- local optical-frequency support;
- estimated uncertainty.

The target sample must be excluded from model choice and fitting whenever it is used as a hold-out.

The earlier Anchor-Constrained affine experiment remains an important negative result: more coverage did not justify promotion because comparable-point aggregate error was worse and uncertainty coverage was too optimistic.

---

## 5. Required hold-out battery

Each scientific candidate must report at minimum:

- number of true target-excluded CFA hold-outs;
- valid-prediction count;
- baseline-comparable count;
- MAE/RMSE/bias aggregate;
- per-CFA-channel MAE/RMSE/bias;
- candidate-vs-baseline win counts;
- 1σ/2σ/3σ uncertainty coverage;
- normalized residual / z-score tails;
- scene strata: flat, edge, texture, dark, midtone, near-censor, emissive-like where identifiable without semantic authority;
- measured anchors changed = 0;
- target used by solver = false;
- Scientific-Master writeback = false;
- candidate applied to production = false during research.

A candidate that improves average MAE but destroys texture or is overconfident does not pass.

---

## 6. Observation-Bound Continuous Scientific Field

Free Raster stays downstream. Research may represent the best admitted scientific image as a continuous/adaptive field rather than a permanently fixed output raster.

Conceptual state:

```text
mu(x,y,c)              best-supported local estimate
Sigma(x,y,c)           uncertainty/covariance
Authority(x,y)         MEASURED / CALIBRATED_ESTIMATE / RECONSTRUCTED / CENSORED / UNKNOWN
OpticalSupport(x,y,f)  supported spatial-frequency content
Model(x,y)             local deterministic basis/model identity
Provenance(x,y)        sealed-source/support lineage
```

This field is **not** permission to hallucinate values between samples. Every local basis must be restricted by the evidence and its supported bandwidth.

---

## 7. Candidate representations

Possible deterministic representations for research:

- piecewise local polynomial fields;
- splines;
- wavelet/multiscale bases;
- edge-aware anisotropic local bases;
- other deterministic bases that expose interpolation error and support.

Do not choose one by elegance. Choose only through real hold-outs and failure analysis.

Generative super-resolution, neural implicit fields and NeRF-style scene completion are not admitted as scientific authority.

---

## 8. Optical support / resolution honesty

ISO 12233 distinguishes camera resolution/SFR from mere addressable pixel count. D.RAW should therefore eventually make output density and supported scene detail separate concepts.

Reference: https://www.iso.org/standard/88626.html

Desired rule:

`arbitrary raster density != arbitrary scientific bandwidth`

An output sampler may generate 1080p, 4K, 8K or custom dimensions. It may not label interpolated sample density as newly measured scene detail.

Where a local SFR/PSF/bandwidth estimate is not evidenced, `OpticalSupport` must remain conservative/UNKNOWN rather than invented from lens identity.

---

## 9. Output route

Long-term conceptual cable:

`sealed observation`
`-> Source Capability Envelope / Evidence Ledger`
`-> Scientific Master + authority/uncertainty`
`-> observation-bound continuous/adaptive scientific field`
`-> Natural Appearance field/state`
`-> ONE final sampling operation`
`-> Original / 1080p / 4K / 8K / Custom`

Free Raster therefore remains valuable as the final sampling/projection interface.

---

## 10. Human-facing quality and Natural Appearance

After scientific validation, a lower-noise latent estimate may look more natural, but perceptual tuning remains separate.

Natural Appearance may use:

- viewing-condition adaptation;
- tone shaping;
- local luminance/contrast;
- bounded perceptual chroma/colourfulness;
- gamut mapping;
- display/output profile.

CIECAM16 is a relevant colour-appearance anchor because its transforms are viewing-condition specific:

https://www.cie.co.at/publications/cie-2016-colour-appearance-model-colour-management-systems-ciecam16

Perceptual preference tests must never substitute for scientific hold-outs.

---

## 11. Architecture / 3D / temporal boundary

The continuous image field is 2D observation-bound unless more evidence exists.

Perspective lines, planes and vanishing structure may constrain projective geometry, but one observation must not silently become metric 3D truth. Hartley/Zisserman is the canonical geometry anchor:

https://www.robots.ox.ac.uk/~vgg/hzbook/

Likewise, local brightness does not prove physical irradiance/material/light transport. Physically based rendering distinguishes emitted and reflected radiance:

https://www.pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/The_Light_Transport_Equation

For stop-motion/static scenes, temporal modulation from displays/LEDs and rolling shutter must remain possible unless exposure evidence rules it out.

---

## 12. Promotion sequence

### R0 — diagnostics only
Noise-likelihood and support diagnostics, no pixel changes.

### R1 — offline/hold-out local models
True withheld-CFA tests; no Scientific-Master application.

### R2 — continuous-field research artifact
Generate a separate research representation with full authority/uncertainty/provenance; compare against baseline.

### R3 — arbitrary sampling tests
Sample at multiple raster sizes and prove that higher sample density does not change scientific authority or claimed bandwidth.

### R4 — Appearance integration candidate
Only after R0–R3 pass; keep downstream and non-authoritative.

### R5 — promotion review
Requires explicit evidence review, regressions, safety/firewall proof and user/project authorization. No automatic promotion.

---

## 13. Success criterion

The roadmap succeeds only if D.RAW can truthfully say:

- more of the sealed observation is understood;
- less stochastic uncertainty is visible where inference is supported;
- real detail is preserved better than baseline;
- uncertainty becomes better calibrated, not merely smaller;
- arbitrary output sampling does not fabricate optical information;
- provenance and authority remain intact end to end.

Guiding line:

> **Niet meer pixels verzinnen, maar meer van de werkelijk aanwezige informatie begrijpen.**
