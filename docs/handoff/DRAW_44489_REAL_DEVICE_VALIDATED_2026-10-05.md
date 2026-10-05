# D.RAW 44489 — real-device T5 / Exact Gauge validation — 2026-10-05

Continuation code: **44489**  
Project: **D.RAW**  
Repository: `1DiDaDo1/TruthRaw`  
PR: **#130**  
Candidate branch: `fix/android-exact-gauge-pass-artifact-v03`

## What is now physically proven

A real-device Foundation export (`draw_free_world_observation_geometry_foundation_v0_1_2_observations8.json`) contains two observations and proves both the positive and negative-control sides of the source-bound T5 transport contract.

Positive-control source:

`4cb86b5f965b0cdfbe7e272304950dc8a15bda41802bda0ba2c1a85cc1184af5`

After that exact RAW was processed through `PRO · D.RAWnegative v0.1` in the same app process, Foundation exported:

- `SOURCE_BOUND_PRECOMPUTED_T5_AUDIT_AVAILABLE`;
- `source_binding_verified=true`;
- `profile_run_binding_verified=false`;
- `t5_corridor_recomputed_by_binding=false`;
- exact source/Scientific-Master binding verified;
- Room Capsule `EXACT_PRESERVING_BYPASS`;
- exposure application count exactly one;
- physical frame count one;
- independent evidence count one;
- all mutation/writeback firewalls closed;
- `candidate_applied=false`;
- no new evidence and no scientific writeback.

The exported stage chain is:

`v0.4 OBSERVED -> v0.5 OBSERVED -> v0.6 READY -> ROOM_CAPSULE EXACT_PRESERVING_BYPASS -> v0.7 OBSERVED`

The authorities remain separated: observed radiometry is preserved; v0.5 is image-plane bound; v0.6 geometry/material/illumination is inferred rather than promoted into measured radiometry; v0.7 is Appearance-only downstream.

Negative-control source:

`7bc0db97b50a7ce4a7bafeb8262d9e1bb572bf02fb7b6f22ac6713b87f917d4f`

No matching precomputed T5 preview existed for this source in-process, and Foundation correctly exported:

- `UNKNOWN_FAIL_CLOSED`;
- reason `NO_PRECOMPUTED_RUNTIME_T5_AUDIT_FOR_SOURCE`;
- `source_binding_verified=false`;
- `t5_corridor_recomputed_by_binding=false`;
- `cross_observation_reuse_allowed=false`;
- no new evidence / no scientific writeback.

This is direct physical evidence that process-local T5 telemetry does not leak between observations and is not recomputed to fill gaps.

## Exact Gauge route-aware tile attribution also proven

Both observations report:

`EXACT_GAUGE_V0_3_ONE_PASS_RECONCILED`

with explicit PassArtifact route attribution `EXACT_GAUGE_RETAINED_V0_3`.

For each observation:

- aggregate source RAW calls: 3072;
- expected active-route RAW calls: 3072;
- pass 1 RAW calls: 3072;
- pass 2 RAW calls: 0;
- pass 2 RAW calls avoided: 3072;
- `raw_call_count_reconciles=true`;
- `optimization_applied=true`;
- source values unchanged;
- `candidate_applied=false`;
- no new evidence;
- no scientific writeback.

This validates the repaired route-aware diagnostic attribution on real hardware. Read counts verify the explicitly bound route; they do not select route authority.

## Build / source identity

Validated runtime code head:

`6a8e01763ca8eb0f6d371c1588a7fbeb91f0af50`

Route-aware attribution integrity head:

`665be2419b2d652206c624864edb864f86cdac4b`

Android build workflow:

- run `37297805632` — SUCCESS;
- artifact ID `11339942673`;
- APK bytes `8,588,087`;
- APK SHA-256 `8ded03cc38375afc2b41d49b7150ae757dac71039aa94f40f18f60fa23d45141`;
- stable signer SHA-256 `a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`;
- versionCode `26100127`.

Dedicated route-aware tile attribution integrity run:

`37298021441` — SUCCESS.

Earlier T5 plumbing integrity run:

`37291751779` — SUCCESS.

## What this does NOT promote

The Foundation-wide scientific promotion state remains `NOT_PROMOTED_FAIL_CLOSED`.

This device test does not promote:

- world registration;
- world-to-source bridge;
- radiometric calibration;
- field-response calibration;
- colour calibration;
- noise-component calibration;
- optical-support calibration;
- temporal relation;
- geometry;
- world-space noise separation;
- scientific single-frame denoise.

No sealed CFA, measured anchor, Scientific Master, Exact Gauge scientific semantics or reconstruction behavior was modified by the T5 audit/export path.

## Exact next task

The device proof is now sufficient for the narrow T5/Foundation and Exact-Gauge route-attribution questions. Do **not** add more runtime/scientific code on that basis.

Next task is repository governance:

1. resolve live PR #130 head;
2. inspect all required governance / lifecycle / integrity gates;
3. fix stale documentation authority or genuinely failing contracts without weakening the gates;
4. rerun and require green/understood status;
5. only then consider taking PR #130 out of draft or merging.

A real-device pass cannot override a required red governance/lifecycle gate.

**One Free World. Many sealed observations. One evidence law.**
