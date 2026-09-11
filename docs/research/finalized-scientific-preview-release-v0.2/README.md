# Finalized Scientific Preview Release v0.2

Status: research candidate. v0.1 remains immutable historical evidence.

## Purpose

This stage integrates `scientific-master-streaming-binding-v0.2` into the finalized Scientific Preview release path without changing scientific identity, Technical Backplane phase-2 semantics, preview authority, appearance processing or output pixels.

The only intended execution change is the exact self-gauge median schedule:

- v0.1: four complete Stage-2 gauge scans using 8-bit radix refinement;
- v0.2: two complete Stage-2 gauge scans using 16-bit radix refinement.

For positive finite IEEE-754 `float` Stage-2 samples, unsigned bit order equals numeric order. v0.2 therefore preserves the same exact lower/upper median values and the same even-count average used by v0.1.

## Required equivalence

The end-to-end test runs the same prepared source through finalized release v0.1 and v0.2 and requires:

- identical Scientific Master SHA-256;
- bit-identical `TruthRangeGaugeV02::L0` and identical gauge mode/id/authority flags;
- identical `LatentSceneBindingV02`, including `sceneScaleId`;
- byte-for-byte identical serialized 180-byte Technical Backplane phase 2;
- identical finalized preview authority;
- byte-for-byte identical ARGB8888 preview pixels;
- unchanged single-frame/provenance invariants;
- exactly `4 -> 2` Scientific Master gauge scan passes;
- total RAW tile requests reduced by exactly two Scientific-Master tile traversals in the instrumented test source.

A second test requires v0.2 to accept a persisted Backplane produced from the v0.1 scientific identity, proving there is no serialized lineage drift.

## Memory/resource tradeoff

The optimization intentionally exchanges bounded auxiliary histogram memory for fewer source scans. `scientific-master-streaming-binding-v0.2` uses two 65,536-bin `uint64_t` histograms at most. This changes execution resources only; it does not change truth authority or scientific values.

## What is not claimed

This module does not yet prove physical Honor wall-time, RSS, thermal, energy or storage-I/O improvements. Those claims require a rebuilt Android route and a fresh physical-device empirical run. The expected logical read reduction on the previously measured Honor workload remains a prediction until that run is performed.

It also does not change color authority, create physical calibration, create additional scene evidence, change the zero-line into an absolute physical zero, or make appearance output part of the Scientific Master.
