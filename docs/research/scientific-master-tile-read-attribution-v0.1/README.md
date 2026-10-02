# Scientific Master Tile-Read Attribution v0.1

## Purpose

This is a diagnostic-only attribution layer for the Scientific Master binder. It does not optimize, cache, reuse, modify, promote, reconstruct, correct, or write back any scientific value.

The real-device Scientific Master Bind Profile v0.1 run showed 6,144 RAW source reads and 3,072 reconstruction calls per tested RAW. Before any source-tile reuse is considered, D.RAW must explain that ratio without guessing.

## Source-code finding

`scientific_master_streaming_binding_v0_2.cpp` is a deterministic two-pass binder:

1. **Canonical pass 1** calls `for_each_canonical_tile(...)`. That helper calls `fill_stage2(source, tile, workspace)` once for every canonical tile. Pass 1 then performs Scientific Master digest work, reconstruction, the canonical authority observer, and collects the high 16 bits needed by the exact self-gauge median.
2. **Canonical pass 2** calls the same `for_each_canonical_tile(...)` helper again. It therefore performs one more Stage-2 source fill per canonical tile. Pass 2 resolves the low 16 bits for the exact self-gauge median and does not call reconstruction or the authority observer.

For a successful v0.2 bind, the established counters therefore provide a strict fail-closed schedule reconciliation:

`source_read_raw_call_count == 2 * reconstruction_call_count`

When this equality holds, the count attribution is:

- pass 1 RAW reads = `reconstruction_call_count`;
- pass 2 RAW reads = `reconstruction_call_count`;
- unattributed RAW reads = 0.

When it does not hold, v0.1 reports `UNKNOWN_FAIL_CLOSED`; it does not force the two-pass interpretation.

## What is and is not measured

The existing native Scientific Master Bind Profile remains the measurement source for aggregate RAW-read elapsed time and call count. Tile-Read Attribution v0.1 only adds a deterministic schedule interpretation to the Foundation performance diagnostics.

It deliberately does **not** split aggregate RAW-read milliseconds between pass 1 and pass 2. The current instrumentation has no independent per-pass timer, so v0.1 reports:

- `per_pass_source_read_timing_available=false`
- `per_pass_timing_inferred=false`

No 50/50 timing assumption is allowed. A later timing experiment may measure the two passes separately if that is needed before an optimization decision.

## Export contract

Each available `D.RAW/ScientificMasterBindProfile/0.1` in the Free World performance diagnostics receives:

`tile_read_attribution_v0_1`

with schema:

`D.RAW/ScientificMasterTileReadAttribution/0.1`

Key status for the proven current route is:

`TWO_PASS_BINDER_SCHEDULE_RECONCILED`

The object records the two pass roles, aggregate RAW-read count/time, expected two-pass count, pass 1/pass 2 counts, unattributed count, and explicit timing limitations.

## Scientific boundary

This layer is diagnostic runtime metadata only. It is not scientific evidence and it cannot change measurement authority. It does not alter source samples, reconstruction, Scientific Master identity, TruthNegative/D.RAWnegative state, authority hashes, calibration state, or Free World promotion state.

Required invariants remain:

- `source_values_modified=false`
- `candidate_applied=false`
- `creates_new_evidence=false`
- `scientific_writeback_allowed=false`
- `optimization_applied=false`

The next performance step is not authorized by this diagnostic. Any proposed reuse of pass-1 material by pass 2 must first have its own exact-parity proof, bounded-memory argument, fail-closed fallback, and real-device measurement.
