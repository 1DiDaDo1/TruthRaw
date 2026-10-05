# Scientific Master Tile-Read Attribution v0.1

## Purpose

This is a diagnostic-only attribution layer for the Scientific Master binder. It does not optimize, cache, reuse, modify, promote, reconstruct, correct, or write back any scientific value.

The layer explains aggregate Scientific-Master source-tile read counts **after** the active binder route has already been identified by the separate, Scientific-Master-hash-bound PassArtifact diagnostics.

**Read counts do not choose the route.** They only verify that the independently proven route produced the expected read footprint.

## Two admitted route shapes

### Canonical v0.2 fallback — two-pass

`scientific_master_streaming_binding_v0_2.cpp` remains a deterministic two-pass canonical binder:

1. **Canonical pass 1** calls `for_each_canonical_tile(...)`. The helper performs `fill_stage2(source, tile, workspace)` once per canonical tile. Pass 1 performs Scientific Master digest work, reconstruction, the authority observer, and collects the high 16 bits for exact self-gauge resolution.
2. **Canonical pass 2** calls the same helper again. It performs one further Stage-2 source fill per canonical tile, resolves the low 16 bits for the exact self-gauge median, and does not call reconstruction or the authority observer.

For a verified `CANONICAL_V0_2_FALLBACK` route the expected call footprint is therefore:

`source_read_raw_call_count == 2 * reconstruction_call_count`

A reconciled canonical fallback reports:

`CANONICAL_V0_2_TWO_PASS_RECONCILED`

with:

- pass 1 RAW calls = `reconstruction_call_count`;
- pass 2 RAW calls = `reconstruction_call_count`;
- avoided pass-2 RAW calls = `0`;
- unattributed RAW calls = `0`.

### Exact Gauge v0.3 — one active source-read pass

Exact Gauge Retained Artifact v0.3 preserves the exact eligible Float32 gauge bits from the first pass so the second Stage-2 source-tile reread is unnecessary.

The route itself must already be proven by `D.RAW/ScientificMasterPassArtifactAttribution/0.1`, including:

- telemetry available;
- Scientific-Master hash binding verified;
- no attribution contradiction;
- route attribution `EXACT_GAUGE_RETAINED_V0_3`;
- `optimization_applied=true`;
- `stage2_gauge_scan_passes_actually_used=1`;
- `pass2_stage2_tile_reads_avoided == reconstruction_call_count`;
- `candidate_applied=false`;
- `source_values_modified=false`;
- `creates_new_evidence=false`;
- `scientific_writeback_allowed=false`.

Only after those route facts are accepted may Tile-Read Attribution compare the independent source-read counter with the expected one-pass footprint:

`source_read_raw_call_count == reconstruction_call_count`

A reconciled retained route reports:

`EXACT_GAUGE_V0_3_ONE_PASS_RECONCILED`

with:

- pass 1 RAW calls = `reconstruction_call_count`;
- pass 2 RAW calls = `0`;
- avoided pass-2 RAW calls = `reconstruction_call_count`;
- unattributed RAW calls = `0`.

The historical two-pass count is still exported as `expected_two_pass_raw_call_count` for comparison, while `expected_active_route_raw_call_count` records the footprint expected for the actually proven route.

## Fail-closed behavior

If the PassArtifact route telemetry is absent, unbound, contradictory, unsafe, or does not match the independent read-count footprint, Tile-Read Attribution reports:

`UNKNOWN_FAIL_CLOSED`

It does not infer Exact Gauge merely because the read count happens to equal one pass, and it does not infer canonical fallback merely because the count happens to equal two passes.

This separation is intentional:

`explicit PassArtifact diagnostics -> route attribution -> independent read-count reconciliation`

not:

`read count -> guessed route`.

## What is and is not measured

The native Scientific Master Bind Profile remains the measurement source for aggregate RAW-read elapsed time and call count. Tile-Read Attribution v0.1 only adds deterministic route-aware schedule reconciliation to Foundation performance diagnostics.

It deliberately does **not** split aggregate RAW-read milliseconds between passes. The current instrumentation has no independent per-pass timer, so v0.1 always reports:

- `per_pass_source_read_timing_available=false`
- `per_pass_timing_inferred=false`

There is **no 50/50 timing assumption**.

## Export contract

Each available `D.RAW/ScientificMasterBindProfile/0.1` in Free World performance diagnostics receives:

`tile_read_attribution_v0_1`

with schema:

`D.RAW/ScientificMasterTileReadAttribution/0.1`

Important fields include:

- `route_attribution`
- `route_attribution_verified`
- `attribution_basis`
- `aggregate_source_read_raw_call_count`
- `expected_two_pass_raw_call_count`
- `expected_active_route_raw_call_count`
- `pass_1_raw_call_count`
- `pass_2_raw_call_count`
- `pass_2_raw_calls_avoided`
- `unattributed_raw_call_count`
- `raw_call_count_reconciles`
- `optimization_applied`

## Scientific boundary

This remains diagnostic runtime metadata only. It is not scientific evidence and cannot change measurement authority. It does not alter source samples, reconstruction, Scientific Master identity, TruthNegative/D.RAWnegative state, authority hashes, calibration state, Exact Gauge scientific meaning, or Free World promotion state.

Permanent invariants:

- `source_values_modified=false`
- `candidate_applied=false`
- `creates_new_evidence=false`
- `scientific_writeback_allowed=false`
- `per_pass_source_read_timing_available=false`
- `per_pass_timing_inferred=false`

Exact Gauge v0.3 optimization may be **described** by this object only after its separate explicit PassArtifact attribution is valid. This diagnostic layer never grants promotion or writeback authority.
