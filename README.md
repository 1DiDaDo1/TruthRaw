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

## Core architecture: sealed house -> new house

The original RAW is the **sealed original house**: immutable evidence of what the camera actually measured.

TruthRaw builds a **separate new house, stone by stone**: the Latent Scene Truth / Scene Master. That new master is not required to inherit arbitrary representation limits of the source container such as RAW10 code range, `WhiteLevel` as an output ceiling, source ISO as the working image scale, source gamut, SDR range, integer storage, or DNG compatibility constraints.

**ISO remains immutable capture provenance, but does not define the identity or numerical scale of the reconstructed scene.** TruthRaw may normalize capture gain into an ISO-neutral scene-domain master while preserving the original gain/readout state in provenance and uncertainty.

This freedom applies to representation, not to evidence claims: reconstruction may exceed source-container limits, but reconstructed values are never relabelled as newly measured photons. `FULL_PHYSICAL` is an evidence/certification level, not the permission boundary of the reconstruction architecture.

The canonical formulation is documented in `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md`.

## Core dynamic-range architecture: zero line / TruthRange

TruthRaw also treats dynamic range as a property of **two different worlds**.

For positive latent scene light, the new house may use:

`T = log2(L/L0)`

with a chosen zero-line reference `L0`.

The TruthRange address space is allowed to continue toward `+infinity` for increasing light and toward `-infinity` as positive light approaches zero. Source RAW10, WhiteLevel, ISO, DNG and display range do not define the ceiling/floor of that address space.

At the same time, any particular RAW exposes only a **finite evidence-supported interval** inside that space. Clipping is a lower-bound censor toward `+infinity`; noise-limited darkness may have an open tail toward `-infinity`.

TruthRaw therefore permanently distinguishes:

- unbounded TruthRange address space;
- finite captured/evidence-supported dynamic range;
- reconstructed-support range with explicit bounds/uncertainty;
- finite presentation/export dynamic range.

**The axis can be infinite. The evidence is finite. Reconstruction beyond the evidence remains reconstruction.**

Calibration places evidence on the axis and reduces uncertainty; it does not determine how high or deep the new house is allowed to exist.

The canonical formulation is in `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`. New sessions should begin with `START_HERE_NEW_CHAT.md`.

## Current repository snapshot

The repository contains the canonical reconstruction/detail/output-acutance/PTC/uncertainty stack, the canonical sealed-house + zero-line conceptual architecture, and separate experimental/research branches including the HONOR BKQ-N49 Camera 5 full-sensor path and the zero-line implementation prototype.

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
- Measured, reconstructed, censored/unknown, and appearance data remain distinguishable.
- White-level clipping is censored/lower-bound evidence.
- GainMap is applied exactly once; signal and noise transform together.
- Sensor black is not display black and is not the TruthRange `-infinity` point.
- Signed scene-linear reconstruction remains distinct from the positive-light TruthRange/log coordinate.
- Noise-free appearance is not perfect knowledge; uncertainty remains.
- Uncertainty is bound to the exact reconstruction backend/hash.
- Camera-native RGB is never treated as display sRGB without the canonical transform.
- Exact LinearRaw v0.4 release-candidate DNGs have passed `dng_validate` 1.7.1 (2611); every changed/new DNG candidate must repeat the full interoperability/validator gates.
- DNG SDK validation establishes container/interoperability validity; it does not turn a derived reconstruction into untouched sensor RAW or Adobe certification.

## Repository policy

Large measurement/evidence files are intentionally not committed directly to Git:

- DNG/RAW source files
- `.rawsensor`, RAW10/RAW12 payloads
- large JPEG/PNG outputs
- APKs
- NPZ analysis dumps
- full project backup archives

Their provenance should be represented by manifests and hashes instead.

APK reverse-engineering evidence is non-canonical side evidence under the current project rule. No APK-derived code, topology, noise, color, metadata, or architecture change is merged into canonical TruthRaw unless that policy is explicitly reversed.

## Current work areas

- `canonical/reconstruction/v4.7i/` — byte-exact production reconstruction reference core
- `canonical/detail/v4.7j/` — Adaptive Detail Truth backend and selected release evidence
- `canonical/output-acutance/v4.7k/` — Output Acutance CPU reference, build support and validation evidence
- `canonical/ptc/v1.1/` — Pure Truth Certificate implementation and fail-closed export boundary
- `canonical/uncertainty/v5.0g/` — backend-bound tele uncertainty model/runtime
- `canonical/uncertainty/v5.0g-p1/` — prospective tele holdout PASS evidence
- `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md` — canonical source-vs-master architecture
- `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md` — canonical unbounded TruthRange architecture
- `docs/research/zero-line-dynamic-range-v0.1/` — research implementation/validation, not yet canonical master implementation
- `capture/android/camera5-200mp-probe-v07/` — experimental Android Camera2 200 MP RAW_SENSOR capture probe
- `tools/` — host-side RAW normalization, integrity verification and fail-closed runtime gates
- `tests/` — host validation
- `evidence/` — compact manifests/hashes, not source RAW payloads

## Current status

TruthRaw remains research software. The general project provenance class is **PURE_TRUTH_DERIVED / derived reconstructed RAW**, not full physical ground truth.

The **zero-line / unbounded TruthRange address-space architecture is canonical**. Its current v0.1 `ISO * exposure` implementation remains a research prototype and is not treated as physical gain calibration.

LinearRaw v0.4 is a **DNG_SDK_VALIDATED_RELEASE_CANDIDATE** for its exact validated hashes. It remains `DERIVED_RECONSTRUCTED_RAW` and is not labelled original sensor RAW or Adobe-certified.

The Camera 5 full-sensor branch is experimental and does not silently alter the canonical reconstruction engine.
