# TruthRaw Worker Resource Invariance v0.1

**Status: RESEARCH REGRESSION / CANONICAL v4.7i EXECUTION-EQUIVALENCE TEST / NO CANONICAL BYTE CHANGE**

This harness tests the execution claim behind the FotoGraaf metaphor **"one room, multiple photographers"** against the existing byte-frozen canonical v4.7i in-memory processor.

Important historical fact: canonical v4.7i already contains real multithreaded tile workers, parallel SDR application and parallel half-resolution HDR-gain calculation. The later bounded streaming route deliberately returned to one worker as a correctness/memory baseline; multithreading itself was not removed from the scientific design.

Full source-grounded history and forward plan:

`V47I_MULTITHREADING_HISTORY_AND_VNEXT_PLAN_2026-09-14.md`

It does not promote the current bounded Android streaming route to multiple workers. `full-frame-streaming-v0.1` remains intentionally single-worker until its source/audit, sink ordering and memory contracts are evolved separately.

## Contract under test

For one fixed synthetic RAW frame and one fixed scientific/appearance configuration, changing only `ProcessOptions::threads` from 1 to 2 or 4 must not change authoritative output bytes or provenance semantics.

The harness uses:

- canonical `ResearchEdgeAwareMeasuredPreservingReconstruction`;
- canonical `SkinSafeDetailedCrispAppearance`;
- even 32-pixel tile cores with halo 7;
- HDR enabled;
- scientific diagnostics enabled;
- deterministic synthetic BGGR input containing ordinary, saturated and >WhiteLevel samples.

The one-worker result is the baseline. The two-worker and four-worker results must match it exactly for:

- dimensions/orientation;
- the complete `ExposurePlan` including `stage2Over1Count`;
- scientific/provenance fields;
- every float byte in `sdrRgb`;
- every float byte in `halfLogGain`;
- every float byte in `stage2Diagnostic`.

The four-worker case is repeated three additional times to expose schedule-dependent drift.

Timing and execution-resource telemetry are intentionally excluded from identity because worker count is allowed to change speed and resource use, not truth.

## Existing v4.7i execution model

The full canonical processor already:

- derives a worker count from `ProcessOptions::threads` and tile count;
- assigns tiles dynamically through an atomic next-tile counter;
- gives each worker private scratch buffers, private histograms and private counters;
- joins workers before deriving the one global `ExposurePlan`;
- parallelizes SDR application by row ranges;
- parallelizes HDR `halfLogGain` generation by half-resolution row ranges;
- requires an even tile core when multiple workers are used so 2x2 HDR cells retain one tile owner.

This is the historical implementation basis for the newer room/photographer metaphor.

## vNext improvement direction

Do not rewrite frozen v4.7i just to modernize threading. Carry the proven idea forward through the Building Runtime:

- persistent worker pool instead of repeated thread creation/join;
- explicit tile/half-cell ownership;
- bounded per-worker `WorkerScratchLease`;
- deterministic reductions and canonical commit order;
- producer -> compute workers -> ordered sink when source/sink cannot be concurrent;
- resource governor chooses worker count from CPU, RAM and thermal budget only;
- separate room-level concurrency after shared prerequisites are sealed;
- ISO/gain interpretation remains upstream; HDR remains downstream and must not trigger a second ISO normalization.

## Authority boundary

A PASS proves only that this canonical synthetic in-memory path is resource-invariant for the tested 1/2/4 worker configurations and compilers/runtimes exercised by CI.

It does **not** prove:

- that `TileNativeDngSource` is currently thread-safe;
- that streaming sinks are concurrently safe;
- that Android four-worker streaming is production-ready;
- a performance gain on a phone;
- thermal suitability;
- universal race freedom outside the tested path.

Those remain separate gates for the future bounded streaming candidate.

## Principle

**One truth, one room contract, as many validated workers as the hardware can use safely.**

**More cores may give more photographers. They may never give more truth.**
