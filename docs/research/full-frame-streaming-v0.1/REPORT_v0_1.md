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

## Repository validation result

GitHub Actions run `34528065758` passed all three real-repository jobs against the exact bound canonical v4.7i source:

- GCC Release: PASS;
- Clang Release: PASS;
- Clang ASan/UBSan: PASS.

For the synthetic canonical-equivalence fixture the reported maximum absolute differences were exactly `0` for SDR RGB, half-resolution log gain and Stage-2 diagnostic. The 16320x12288 planning fixture reported `458920` bytes logical adapter workspace and `983208` bytes total logical resident upper bound with the test's 256 KiB source + 256 KiB sink bounds.

The first real Clang attempt was blocked before execution by a pre-existing `-Wmisleading-indentation` warning in byte-frozen v4.7i. That failure is preserved in `FAILURE_HISTORY_v0_1.md`. Canonical source was not edited; the frozen core is compiled separately so the warning remains visible but does not force a canonical byte change. New adapter/test sources remain strict `-Werror`.

## Promotion boundary

The module has passed the required real canonical equivalence gate. Promotion must still use an add-only clean commit above the current main and repeat CI on those final bytes. Promotion does not delete or rewrite the old full-frame compatibility API.
