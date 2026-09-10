# TruthRaw Full-Frame Streaming v0.1

Status: **RESEARCH PASS — canonical v4.7i equivalence validated on GCC, Clang and Clang ASan/UBSan; canonical v4.7i bytes remain unchanged**.

## Purpose

Remove full-frame ownership from the execution corridor without changing TruthRaw's scientific authority. The adapter turns the current full-frame public ownership pattern into:

`IRawTileSource -> bounded tile/halo workspace -> tile-local recomputation -> IStreamingSink`

The sealed source remains evidence. The adapter does not create a second observation, does not change the zero-line, and does not let appearance modify the scientific master.

## Why two image passes

Canonical v4.7i chooses one global exposure plan from whole-image histograms. A low-memory implementation cannot apply that plan before the global statistics exist.

v0.1 therefore uses:

1. **Pass 1:** raw tile -> Stage-2 -> measured-preserving reconstruction -> XYZ/neutral RGB -> fixed-size global histograms only -> release tile.
2. Choose the exact existing canonical exposure plan and LUT.
3. **Pass 2:** re-read tile -> Stage-2 -> reconstruction -> neutral RGB -> appearance -> LUT -> stream SDR + optional diagnostic + half-gain -> release tile.

Appearance is intentionally absent from pass 1 because canonical exposure statistics use neutral RGB and XYZ scene Y, not the appearance output.

## Full-frame state removed from the adapter

The adapter itself owns no full-frame RAW, SDR, half-gain or diagnostic vector. Input and output are external interfaces with declared resident-memory bounds; no per-image half-resolution scratch store is required.

The current canonical v4.7i `DecodedDngFrame.raw` and `ProcessResult.sdrRgb` remain untouched for compatibility and for equivalence testing.

## Half-resolution HDR state

Canonical v4.7i keeps `halfSceneMax` and `halfCensor` in RAM. v0.1 does not persist them at all. During pass 2, half-scene maxima are recomputed for the active tile and censor dilation is derived directly from the RAW halo. The scientific values remain the same while both RAM and scratch-disk ownership are avoided.

## Device flexibility

v0.1 establishes the correctness-first single-worker streaming path. Device tiers can already vary tile size and the bounded cache strategy of source/sink implementations. Multi-worker concurrent streaming is deliberately OPEN because source/sink thread-safety and deterministic ownership need a separate gate.

A stronger phone may retain/reuse more data to avoid recomputation later. That is an execution optimization only and cannot change exposure, evidence confidence or scientific claims.

## Important bound

`logicalResidentUpperBound` includes:

- logical adapter tile workspace;
- source-reported resident upper bound;
- sink-reported resident upper bound.

It is not an OS-RSS theorem: STL allocator metadata, C runtime state, code pages and kernel page cache are outside this logical accounting. When a memory budget is supplied, v0.1 additionally compares observed vector capacities with that budget and fails closed if they exceed it.

## Claim boundary

PASS means streaming equivalence for the tested v4.7i path and bounded adapter ownership. It does **not** mean the DNG decoder itself is already tile-native on Android, nor that every downstream canonical module has already migrated to StreamedSink.


## Real repository validation

GitHub Actions run `34528065758` compiled this adapter against the exact bound canonical v4.7i `core.h`/`core.cpp` and passed under GCC Release, Clang Release and Clang ASan/UBSan. On the synthetic canonical-equivalence fixture, final SDR RGB, half-resolution log gain and Stage-2 diagnostic each matched the canonical `TruthRawProcessor::processFrame` output with maximum absolute difference `0`.

This validates the tested synthetic v4.7i path. It does not yet prove tile-native DNG input, every camera metadata topology, every downstream room, or real-device OS RSS.
