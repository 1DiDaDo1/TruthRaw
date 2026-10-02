# D.RAW Center-Excluded v2 Profile v0.1

Diagnostic-only profiling pass for the existing center-excluded v0.2.1 scientific
contract, executed through the sparse-reference v0.2.2 runtime path.

## Why

Real-device v0.2.10/v0.2.11 measurements show Center-Excluded is now one of the
largest remaining native hot paths. The existing telemetry only separates:

- Stage2 tile fill;
- whole candidate loop;
- whole predictor estimate;
- final audit hash.

That is not enough to decide which optimization is justified.

## What this pass measures

The scientific algorithm is unchanged. A deterministic 1-in-64 candidate sample
adds low-overhead diagnostic timing for:

- center/channel/index/variance setup;
- neighbor index/channel/value acquisition;
- censor-path traversal;
- neighbor variance evaluation;
- predictor estimate;
- valid-residual metric math.

Inside the predictor, the same sampled candidates are split into:

- admissibility + slot construction;
- symmetric-pair discovery/gating;
- directional scale consistency + pair combination;
- cross-scale consistency + running combination;
- final scale combination.

Structural counters accompany those timings: neighbor samples, censor-path
checks/steps, slot duplicates, map pair lookups, z-distance evaluations and
inverse-variance combines.

## Sampling contract

The profile stride is fixed at 64 candidates. Sampling is deterministic from the
established sparse corrected-sample order. Timings are raw sampled timings; they
must not be silently multiplied into a claimed full-run duration. Their purpose
is hotspot attribution and relative phase comparison.

The existing full-run metrics remain authoritative for runtime diagnostics:

- center_excluded_candidate_loop_ms
- center_excluded_predictor_estimate_ms
- center_excluded_total_instrumented_ms

## Authority boundary

All new fields are DIAGNOSTIC_RUNTIME_ONLY.

They do not enter the v0.2.1 audit hash, do not change candidate acceptance,
do not change measured/reconstructed/censored/unknown classification, and do
not authorize reconstruction, calibration, correction or Scientific Master
writeback.

Required invariants remain:

- source_values_modified=false
- candidate_applied=false
- creates_new_evidence=false
- scientific_writeback_allowed=false
- timing_is_scientific_evidence=false
- timing_may_change_scientific_authority=false

The active Research derived-stage id is
`N2_LOCAL_SPATIAL_V01_R12_CENTER_EXCLUDED_PROFILE` so the first explicit run
after installing the profiling APK recomputes the N2_LOCAL_SPATIAL stage without
requiring app-data deletion. The profile cache generation remains the existing
v0.2.11 generation because this pass changes diagnostics, not scientific
identity.

## Next decision

Use one fresh real-device two-RAW run to identify the dominant Center-Excluded
subphase. Only after that measurement should a performance optimization be
implemented. No SIMD/NEON is introduced by this profiling pass.
