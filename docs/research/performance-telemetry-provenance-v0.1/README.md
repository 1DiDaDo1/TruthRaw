# Performance Telemetry Provenance v0.1

Status: **candidate validation**.

This checkpoint separates timing measured in the current Universal Source Profile
run from native timing that may have been carried inside a deterministic derived
stage cache entry created by an earlier run.

## Why this is needed

On a cold v0.2.11 device run, both RAW profiles performed their heavy N2 work in
the current run. Subsequent warm runs completed in about one second because the
derived N2 stages were cache hits. The profile-level and stage-level elapsed
times correctly measured those warm runs, but the cached N2 JSON still contained
the native phase/subphase timings recorded when that cached result was originally
computed.

Those two timing domains are both useful, but they must not be presented as if
they describe the same execution.

## Provenance contract

The diagnostic telemetry now states explicitly:

- `profile_elapsed_ms` has scope `CURRENT_PROFILE_RUN`;
- every stage `elapsed_ms` has scope `CURRENT_PROFILE_RUN`;
- each heavy audit reports whether its result was a current-run derived-stage
  cache hit;
- embedded native shared-preparation timing reports one of:
  - `CURRENT_PROFILE_RUN`;
  - `CACHED_DERIVED_STAGE_ORIGIN_COMPUTE`;
  - `UNKNOWN_CURRENT_STAGE_TRACE_MISSING`;
  - `UNAVAILABLE`;
- native shared-prepare cache hit/miss counts are explicitly scoped to
  `ORIGIN_NATIVE_EXECUTION`;
- Foundation aggregation separately counts current-run derived-stage hits/misses,
  current-run native audit computes, and cached-origin native audit telemetry.

The existing timing fields remain available for compatibility. Their provenance
is now explicit so a warm cache-hit run cannot be mistaken for a multi-second
native recomputation.

## Scientific invariants

This work is telemetry/provenance only. It does not change RAW bytes, sealed
source identity, Scientific Master values, MEASURED/RECONSTRUCTED/APPEARANCE
authority, reconstruction decisions, calibration, correction, promotion, or
scientific writeback.

The Universal Source Profile cache generation remains
`D.RAW/UniversalSourceProfileCache/0.2.11-authority-template-reuse-v1`.

## Android validation build

- versionCode: `26100116`
- versionName:
  `0.53-v0.84.2-authority-template-reuse-telemetry-provenance`

The build is intended to install over the validated Research live-status build
without clearing app data, so both current-run and cached-origin timing domains
can be exercised on-device.
