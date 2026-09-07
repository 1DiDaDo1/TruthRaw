# TruthRaw

TruthRaw is a post-capture RAW reconstruction research project and software-ISP built around one scientific rule:

> **Measured where measured. Reconstructed where necessary. Never invented.**
>
> **Echt gemeten. Echt gereconstrueerd. Geen verzinsels.**

## Project scope

TruthRaw treats RAW/CFA data as measurement evidence rather than as a finished photograph. The project separates:

- immutable source measurements and capture metadata;
- physically/statistically reconstructed values;
- uncertainty and provenance;
- appearance/rendering decisions.

Pure Truth does not use generative scene content, hallucinated texture, or hidden multi-frame scene evidence. Reconstructed values are never relabelled as newly measured photons.

## Current repository snapshot

This repository is being populated from the current private TruthRaw research state. The first checked-in branch focuses on the experimental **HONOR BKQ-N49 Camera 5 full-sensor RAW path**.

Current device evidence for the tele camera indicates:

- Camera 5 / 22.48 mm tele
- default RAW: 4080 × 3072
- maximum-resolution RAW route: 8160 × 6144
- high-resolution RAW_SENSOR route: **16320 × 12288 (200.54016 MP)**
- Android ultra-high-resolution capability advertised
- app-visible RAW is treated as regular Bayer by the Android contract for this capability set
- `lensShadingApplied=true`, therefore this project does **not** claim untouched photodiode/ADC truth

The 200 MP capability is proven from Camera2 characteristics. A real 16320 × 12288 RAW_SENSOR payload plus matching `TotalCaptureResult` is still required to close the runtime capture gate.

## Scientific boundaries

Non-negotiable rules include:

- Original CFA/sample bytes and capture metadata are immutable evidence.
- Measured, reconstructed, and appearance data remain distinguishable.
- White-level clipping is censored/lower-bound evidence.
- GainMap is applied exactly once; signal and noise transform together.
- Sensor black is not display black.
- Noise-free appearance is not perfect knowledge; uncertainty remains.
- Uncertainty is bound to the exact reconstruction backend/hash.
- Camera-native RGB is never treated as display sRGB without the canonical transform.
- Production DNG output remains subject to DNG SDK/interoperability validation.

## Repository policy

Large measurement/evidence files are intentionally not committed directly to Git:

- DNG/RAW source files
- `.rawsensor`, RAW10/RAW12 payloads
- large JPEG/PNG outputs
- APKs
- NPZ analysis dumps
- full project backup archives

Their provenance should be represented by manifests and hashes instead.

## Current work areas

- `capture/android/camera5-200mp-probe-v07/` — experimental Android Camera2 200 MP RAW_SENSOR capture probe
- `tools/` — host-side RAW normalization and fail-closed runtime gates
- `tests/` — host validation for the capture branch
- `docs/` — scientific and validation documentation
- `evidence/` — small JSON manifests/hashes only, not source RAW payloads

## Status

TruthRaw is research software. The general project provenance class remains **derived/reconstructed RAW**, not a claim of full physical ground truth.

The Camera 5 full-sensor branch is experimental and does not silently alter the canonical reconstruction engine.
