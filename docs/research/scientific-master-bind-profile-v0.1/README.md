# Scientific Master Bind Profile v0.1

Status: diagnostic runtime profiling only. No scientific authority, reconstruction, correction, calibration, promotion or writeback is added by this profile.

## Purpose

After Center-Excluded fixed-topology v0.1 reduced the established two-RAW runtime, `PREPARE_SCIENTIFIC_CONTEXT_AND_MASTER` became the dominant measured phase. The existing `prepare_bind_scientific_master_ms` timer measures the whole Scientific Master binding call, while the already-established authority observer timer accounts for only one substantial component inside it.

This profile splits that existing bind wall time without changing the binder algorithm. It measures:

- source `readRawTile` wall time and call count;
- source `readRowBias` wall time and call count;
- source `readColBias` wall time and call count;
- reconstruction backend wall time and call count;
- the already-established authority observer/direct-record-stream wall time;
- the remaining unattributed bind wall time.

The unattributed remainder is deliberately not interpreted as one scientific operation. It can include binder-owned work such as stage-2 normalization, canonical digest updates, observer-side raw-core copying outside the authority accumulator, self-gauge histogram work, rank selection and budget/invariant checks. If that remainder is dominant, a later version may profile those binder internals explicitly.

## Instrumentation contract

`ScientificMasterSourceProfilingProxy` implements the same `IRawTileSource` interface and returns the delegate metadata and resident-byte bound unchanged. Its read methods forward the exact arguments and output buffers to the delegate and record only monotonic wall time and call counts.

`ScientificMasterReconstructionProfilingProxy` implements the same reconstruction backend interface. It forwards `quality()`, `name()`, `requiredHalo()` and the exact reconstruction pointers, geometry and CFA arguments to the established backend, recording only monotonic wall time and call count.

The original reconstruction object remains stored in the prepared context and remains the downstream reconstruction authority object. The profiling proxy exists only during the Scientific Master binding call.

Timing fields are diagnostic runtime observations. They are not scientific evidence and may not change scientific authority.

## Fail-closed scientific boundary

This profile does not modify source values, Scientific Master samples, authority records, evidence identity, reconstruction thresholds, colour binding, calibration state or output hashes. It cannot set `candidate_applied`, `creates_new_evidence` or `scientific_writeback_allowed` true.

A real-device performance interpretation is not accepted until the existing two-RAW test is rerun explicitly and the exported Free World Observation Geometry Foundation confirms current-run provenance plus unchanged scientific/firewall content outside performance diagnostics.
