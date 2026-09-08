# START HERE — TruthRaw new-chat bootstrap

This file is the first conceptual entry point for a new TruthRaw chat/session.

## Mandatory reading order

1. `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md`
2. `state/CURRENT_CANONICAL_STATE_2026-09-08.json`
3. `docs/PROJECT_STATE_AUDIT_2026-09-08.md`
4. relevant canonical module documentation for the task being continued

## Non-negotiable core vision

The original RAW is the **sealed original house**: immutable measurement evidence.

TruthRaw builds a **new house, stone by stone**: a separate Latent Scene Truth / Scene Master.

The new master is not required to inherit the source container's arbitrary representation limits such as RAW10 range, WhiteLevel as output ceiling, source ISO as working scale, source gamut, SDR range, integer storage, or DNG compatibility constraints.

**ISO remains capture provenance but does not define the identity or numerical scale of the reconstructed scene.** TruthRaw may normalize capture gain into an ISO-neutral scene master while preserving the original ISO/readout state in provenance and uncertainty.

TruthRaw may reconstruct beyond source-representation limits, but may never relabel reconstructed information as newly measured photons.

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

FULL_PHYSICAL is an evidence/certification scope, not permission to start using the richer architecture. Calibration improves physical support and reduces uncertainty; it does not force the Latent Scene Master back inside the original RAW container.

## Permanent scientific rules

- Original CFA/sample bytes + capture metadata are immutable evidence.
- Measured, reconstructed, censored/unknown and rendered quantities remain distinguishable.
- WhiteLevel clipping is censored/lower-bound evidence.
- GainMap exactly once; corresponding noise/uncertainty transforms with signal.
- Sensor black != display black.
- Scene-linear master may contain values below 0 or above 1 where meaningful.
- Noise-free appearance != zero uncertainty.
- Uncertainty remains bound to exact backend/domain/hash.
- Camera-native RGB != display sRGB without a documented transform.
- DNG is an export/compatibility projection, not the scientific master.
- No generative semantic scene invention.
- No APK-derived canonical changes unless the user explicitly reverses that policy.
- Do not retune the frozen v5.0g model on the black/white-dog holdout failure.
- Do not silently add a LICENSE.

## One-sentence project definition

**TruthRaw preserves the original RAW as sealed evidence and constructs a new, richer, uncertainty-aware scene estimate whose representation is free to exceed the original file's limits while every scientific claim remains bounded by evidence.**
