# Shared Scientific Context / Android 17 performance baseline v0.1

Status: **real-device baseline established 2026-10-01**.

This checkpoint formalizes the bounded same-RAW shared scientific-preparation
optimization that was validated through PR #107 and merged as
`9c74740bc8174d7e29d2312e342c1f3c8c943e16`.

## Real-device result

The same four DNG sources were analyzed before and after the optimization. The
optimized run was performed after clearing app data, so persisted profile/stage
cache from the earlier run could not explain the result.

- previous cold run: **15:31** (931 s)
- shared-context cold run: **05:18** (318 s)
- saved time: **10:13** (613 s)
- elapsed-time reduction: **65.84%**
- observed speedup: **2.93x**
- attempt count: **1**
- redelivery count: **0**
- terminal ANR: **not observed**
- navigation out/back after completion: **passed**
- Free World Foundation export after completion: **passed**

This is a real-device performance observation, not a universal performance
guarantee for other phones, RAW topologies or workloads.

## What changed

The expensive immutable source / Scientific-Master / authority preparation is
constructed once for the currently profiled RAW and may be reused by later
heavy N2/Anchor audits of that **same** RAW.

The process-local cache is deliberately bounded:

- maximum one prepared context;
- context keyed to the freshly sealed source and identical memory budgets;
- owned duplicated source FD keeps lifetime explicit;
- use is serialized;
- every audit still runs its own audit logic;
- every audit still re-verifies the source before accepting output;
- the cache is released at the profile boundary;
- cross-observation context reuse is forbidden.

No GPU path is introduced by this checkpoint.

## Runtime telemetry added after validation

The follow-up baseline adds diagnostic-only telemetry without changing the
validated compute path:

- monotonic per-stage elapsed time;
- derived-stage cache-hit marker;
- per-audit `shared_pipeline_prepare_cache_hit`;
- shared prepare hit/miss counts;
- first reported native audit miss / later same-RAW reuse diagnostics;
- cache-release attempted/succeeded state;
- Foundation-level aggregation per observation.

All telemetry is marked `DIAGNOSTIC_RUNTIME_ONLY`,
`creates_new_evidence=false`, and
`scientific_writeback_allowed=false`.

## Android update lineage

Distributable APKs now read `versionCode` from the repository-owned
`suite_android/VERSION_CODE` file. The baseline moves from legacy
`versionCode=51` to `26100101`, while keeping:

- application ID `com.truthraw.adaptiveui`;
- stable development signing certificate SHA-256
  `a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`.

For the next distributable baseline, `VERSION_CODE` must increase. This makes
the update lineage explicit and reproducible in source control rather than
depending on an ephemeral CI runner.

## Scientific invariants

This performance work does **not**:

- modify sealed source samples;
- change MEASURED / RECONSTRUCTED / APPEARANCE authority;
- alter the Scientific Master;
- promote the anchor solver;
- authorize correction or calibration;
- create new evidence;
- enable scientific writeback;
- introduce AI/ML/neural/generative runtime.

The optimization changes redundant preparation work, not scientific meaning.
