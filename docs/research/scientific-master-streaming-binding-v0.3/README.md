# Scientific Master Streaming Binding v0.3 — bounded stripe execution

Status: **RESEARCH CANDIDATE — v0.2 scientific identity must remain bit-exact**

Parent scientific contract: `scientific-master-streaming-binding-v0.2`.

## Purpose

The physical HONOR v0.73 camera run measured about 87.96 s wall / 87.70 s worker CPU for the finalized Scientific Preview at 4080x3072 while memory, thermals and UI pacing remained stable.

The same run reported 7,680 logical RAW tile reads. v0.2 already cut the older v0.1 self-gauge schedule from four scientific scans to two, so the next safe optimization target is runtime tile granularity rather than another change to the scientific algorithm.

v0.3 changes only **how many source calls are used to compute the same pixels**.

## Invariants

v0.3 must not change:

- source bytes or admitted topology;
- Stage-2 math;
- v4.7i reconstruction;
- canonical 64x64 Scientific Master digest cells;
- Scientific Master SHA-256;
- exact L0 binary64 bits;
- Zero-Line mode/id/flags;
- scene-scale binding;
- eligible sample count;
- physical frame count / independent evidence count;
- color, appearance, HDR or authority;
- PURE v0.63 output semantics.

## Runtime stripe policy

Digest/reconstruction pass:

- runtime core width: 1024;
- runtime core height: 64;
- halo: exact v4.7i required halo;
- the digest accumulator splits each runtime stripe back into the same canonical 64x64 leaf cells.

Exact low-16 self-gauge refinement:

- runtime core width: 1024;
- runtime core height: 256;
- halo: 0;
- this pass consumes only per-sample RAW/Stage-2 values and no reconstruction neighbourhood.

The runtime stripe shape is not part of Scientific Master identity.

## 4080x3072 request-count expectation

v0.2 scientific binding:
- 3072 canonical cells x 2 scans = **6144 logical source tile requests**.

v0.3 scientific binding:
- digest pass: ceil(4080/1024) x ceil(3072/64) = 4 x 48 = **192 requests**;
- gauge refinement: ceil(4080/1024) x ceil(3072/256) = 4 x 12 = **48 requests**;
- scientific total = **240 requests**.

The existing preview still uses 768 tiles x 2 passes = **1536 requests**.

Predicted complete finalized-preview logical request count after integration:
**1776 instead of 7680**, a 76.875% reduction in logical source calls.

This is not yet a latency claim. Reconstruction still evaluates the same scientific pixels, and physical device timing must be measured after integration.

## Falsification gates

Before Android integration, v0.3 must equal v0.2 on pseudo-random, constant, split-median, sparse-evidence, GainMap, residual-black and multi-stripe fixtures.

Required exact equality:

- Scientific Master SHA-256;
- L0 double bits;
- Zero-Line semantics;
- scene-scale semantics;
- eligible sample count;
- canonical master tile count;
- frame/evidence 1/1.

It must also:
- perform fewer logical source tile calls;
- not read more RAW samples than v0.2 on the test matrix;
- fail before source reads when the radix-state memory budget is impossible;
- pass GCC, Clang and ASan/UBSan.

Only after those gates pass may the finalized Android route be switched from v0.2 to v0.3.
