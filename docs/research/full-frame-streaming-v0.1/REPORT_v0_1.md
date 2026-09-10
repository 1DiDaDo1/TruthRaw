# TruthRaw Full-Frame Streaming v0.1 — Implementation Report

## Candidate goal

Prove that the canonical v4.7i image result can be reproduced without the streaming adapter owning any full-frame RAW, SDR, half-gain or scientific-diagnostic buffers.

## Architecture

The adapter is deliberately outside `canonical/reconstruction/v4.7i/`. It consumes the existing reconstruction and appearance backend ABIs and the existing public exposure/LUT functions. Canonical source bytes are not modified.

The principal low-memory route is two-pass recomputation. This trades CPU for RAM on cheap devices. Pass 1 persists only fixed-size histograms; no half-resolution scratch image is persisted between passes. A stronger device may later provide bounded caching source/sink implementations while preserving the same values and interface.

## Test gates

The repository test compares streaming v0.1 against canonical `TruthRawProcessor::processFrame` on a synthetic Bayer frame with:

- BGGR CFA;
- per-phase black levels;
- residual row/column black correction;
- GainField enabled;
- NoiseProfile enabled;
- research edge-aware measured-preserving reconstruction;
- SkinSafeDetailedCrisp appearance;
- HDR enabled;
- scientific Stage-2 diagnostic enabled.

It compares exposure-plan fields, final SDR RGB, half-resolution log gain and Stage-2 diagnostic.

The planning test also uses 16320x12288 dimensions and verifies that logical workspace size is the same as for a smaller frame when tile/halo and feature configuration are unchanged. Only tile count grows with megapixels.

## Promotion rule

Do not promote from research until the GitHub test compiles against the exact current canonical v4.7i source and passes under GCC, Clang and ASan/UBSan. Promotion does not delete the old full-frame compatibility API.
