# D.RAW 44489 — T5 Foundation telemetry plumbing ready — 2026-10-05

Continuation code: **44489**  
Project: **D.RAW**  
Repository: `1DiDaDo1/TruthRaw`  
PR: **#130**  
Candidate branch: `fix/android-exact-gauge-pass-artifact-v03`

## Why this checkpoint exists

The T5 runtime corridor audit already existed and was already computed from the native single-observation Appearance path. The remaining defect was narrower: the precomputed `t5CorridorAudit` did not reach the normal Foundation `performance_diagnostics_v0_1` JSON export.

No second Room Capsule, no second T5 evaluation and no new scientific calculation were required.

## Implemented route

The connected telemetry route is now:

`existing native T5 runtime metrics -> TruthNegativeContinuousPreview.Ready -> existing T5CorridorAuditV01 -> source-SHA-bound process-local diagnostic snapshot -> FreeWorldPerformanceDiagnosticsV01 -> Foundation performance_diagnostics_v0_1 JSON`

The binding layer does not invoke the native preview/corridor, does not call `T5CorridorAuditV01.from(...)`, and does not evaluate `evaluate_room_capsule_relative(...)`.

## Exact code checkpoints

Starting 44489 handoff head before this plumbing round:

`0d80c36174f6da5937ab7673e6de9a47a5e2ed6d`

Foundation-T5 plumbing code head that was compiled into the new APK:

`90aeee32166c571ff12aa812bfe854e9e4f95d6a`

Direct T5 integrity-workflow head that proved the dedicated plumbing contract:

`245e6b9a3bee6b101b81edcee9426694ec9c272d`

Always resolve live PR #130 head before any later mutation; documentation-only commits may be newer than the validated runtime code head.

## New binding contract

New file:

`suite_android/app/src/main/java/com/truthraw/adaptiveui/ResearchPerformanceT5CorridorBindingV01.kt`

The binding is telemetry-only and process-local. It:

- indexes snapshots by exact 64-hex `source_sha256`;
- only accepts `D.RAW/Runtime/T5CorridorAudit/0.1`;
- requires `audit_authority=DIAGNOSTIC_RUNTIME_ONLY`;
- requires `mutation_authority=NONE`;
- rejects source-SHA mismatch;
- rejects new-evidence authority;
- rejects scientific writeback;
- rejects sealed-CFA, measured-anchor, Scientific-Master, Exact-Gauge or reconstruction modification claims;
- rejects `candidate_applied=true`;
- disallows cross-observation reuse;
- reports missing/invalid/contradictory telemetry as `UNKNOWN_FAIL_CLOSED`;
- explicitly reports `profile_run_binding_verified=false`, because source-bound process-local telemetry must not be promoted into a claim that it was computed inside the same profiler run.

A valid bound snapshot is exported as:

`t5_corridor_audit_binding_v0_1`

inside each Foundation performance-diagnostic observation.

## Scientific boundary remains unchanged

This round did **not** modify:

- sealed CFA evidence;
- measured anchors;
- Scientific Master semantics or writeback;
- Exact Gauge Retained Artifact v0.3 core;
- canonical v0.2 core;
- reconstruction behavior;
- v0.4/v0.5/v0.6/Room-Capsule/v0.7 scientific/runtime evaluation;
- exposure calculation.

The existing T5 contract remains authoritative for diagnostic interpretation: one exposure, one physical frame/evidence source, exact-preserving Room-Capsule bypass when world evidence is absent, and `candidate_applied=false`.

## Dedicated integrity contract

Added:

`tools/check_t5_corridor_performance_plumbing_v01.py`

and workflow:

`.github/workflows/t5-corridor-performance-plumbing-v01.yml`

The checker verifies, among other things:

- Room Capsule exact-preserving bypass contract remains present;
- exposure application count remains exactly one;
- physical-frame and independent-evidence counts remain one;
- candidate/writeback/evidence firewalls remain closed;
- the new binding cannot call the T5/native corridor again;
- cross-source snapshots fail closed;
- contradiction/writeback/new-evidence/candidate cases are rejected;
- Foundation continues to aggregate diagnostics rather than re-evaluate T5.

Direct workflow proof:

- run: `37291751779`
- workflow: `T5 Corridor Performance Plumbing v0.1 Integrity`
- exact head: `245e6b9a3bee6b101b81edcee9426694ec9c272d`
- conclusion: **SUCCESS**

## Android/APK build proof for the plumbing code

GitHub Actions run:

`37291164092`

Workflow:

`D.RAW Free World Research APK`

Exact runtime/plumbing source head:

`90aeee32166c571ff12aa812bfe854e9e4f95d6a`

Result: **SUCCESS**.

The job completed checkout, SDK/NDK setup, stable signing materialization, `:app:assembleDebug`, APK/signing verification and artifact upload.

Artifact:

`DRAW-free-world-research-debug-arm64-stable-signed`

Artifact ID:

`11336456810`

Artifact ZIP size:

`3,236,767 bytes`

Artifact ZIP SHA-256:

`3fd7554710e1925500ff82b736dede98473610a6c55a69232a3e7dd11023f3e2`

APK:

`app-debug.apk`

APK size:

`8,588,087 bytes`

APK SHA-256:

`bc20369453694e42826de8440ae8dd2c78a79d72b4d8066f4a942337bafd76b4`

Signing certificate SHA-256:

`a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`

Version code remains:

`26100127`

## Superseded device-test candidate for this specific export test

The older T5 runtime APK bound to runtime commit `6a557cda...` remains valid historical evidence for the original T5 runtime-audit candidate, but it predates this Foundation telemetry plumbing.

For the next **Foundation T5 JSON export validation**, use the APK above from code head `90aeee...`, SHA-256 `bc203694...afd76b4`.

## Exact next real-device test

1. install/update the stable-signed APK above;
2. keep package/signing continuity explicit;
3. use one real admitted DNG/RAW_SENSOR-derived observation;
4. execute the current Foundation/Open-World/T5 path;
5. export the Foundation observation JSON;
6. locate `performance_diagnostics_v0_1.observations[*].t5_corridor_audit_binding_v0_1` for the exact source SHA;
7. require `SOURCE_BOUND_PRECOMPUTED_T5_AUDIT_AVAILABLE` when the matching T5 runtime preview was computed in the process;
8. verify the nested `t5_corridor_audit` still reports the expected stage lineage, Room Capsule exact bypass, exposure count one, one frame/evidence source and closed candidate/writeback firewalls;
9. if no matching precomputed source-bound audit exists, require `UNKNOWN_FAIL_CLOSED`; never synthesize or recompute it during Foundation export;
10. preserve the exported JSON as physical evidence before any promotion.

## Promotion state

This checkpoint proves implementation, compilation and static/integrity-contract success. It does **not** yet prove the new field on a real-device Foundation JSON export.

PR #130 remains candidate/draft. Do not merge/promote solely from these two green workflows; re-evaluate the wider PR governance/lifecycle/check set and inspect real-device JSON evidence first.

**One Free World. Many sealed observations. One evidence law.**
