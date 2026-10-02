# D.RAW Research Fresh Rerun v0.1

Validation-only lifecycle fix for the v0.2.11 authority-template candidate.

## Observed failure

A second explicit universal-analysis request on the same two RAWs could expose
the previous current-generation profiles while the UI showed a new RUNNING
operation. The durable Research journal also preserved completed job state from
the previous attempt. This could make an old performance snapshot look like a
new measurement and could leave the foreground UI showing stale pending/stage
information until the Activity resumed.

The v0.2.11 scientific/authority algorithm is not changed here.

## Contract

An explicit user request for universal analysis is a **fresh measurement
attempt**:

- selected derived UniversalSourceProfile cache files are removed before the
  fresh run;
- in-memory selected profiles are removed so Foundation export cannot silently
  reuse the previous snapshot while the fresh run is pending;
- the service independently verifies/removes selected derived profile files on
  the initial start;
- only Android service redelivery may resume already-persisted profiles from
  that interrupted attempt;
- a fresh journal attempt resets prior per-job COMPLETED/FAILED/stage facts to
  PENDING and records `run_mode=FRESH_USER_RUN`;
- redelivery records `run_mode=REDELIVERED_RESUME` and preserves resumable job
  state;
- bulk profile restore rejects stale profiler generations;
- a stale RUNNING Research operation is recovered fail-closed using the
  per-operation Research worker liveness, not merely global service liveness.

## Authority boundary

The cleared files are derived diagnostic caches only. This fix does not delete
or modify RAW source bytes, sealed evidence, Scientific Master values,
authority records, canonical 25-byte/75-byte layout, SHA identity,
reconstruction, calibration, correction, candidate promotion or scientific
writeback.

The profiler generation intentionally remains:

`D.RAW/UniversalSourceProfileCache/0.2.11-authority-template-reuse-v1`

This lets the repaired APK repeat the same v0.2.11 algorithm under a genuinely
fresh runtime measurement.
