# D.RAW 44489 — Foundation device round and route-aware tile attribution — 2026-10-05

Continuation code: **44489**  
Project: **D.RAW**  
Repository: `1DiDaDo1/TruthRaw`  
PR: **#130**  
Candidate branch: `fix/android-exact-gauge-pass-artifact-v03`

## Physical input inspected

The 2026-10-05 device round produced four related machine-readable exports from the same four active source roots:

- `DRAW_GLOBAL_RESEARCH_SNAPSHOT_v0_1 (3).json`
- `draw_field_response_repeatability_v0_1_4_observations.json`
- `draw_observation_world_field_separation_v0_1_4_observations.json`
- `draw_free_world_observation_geometry_foundation_v0_1_4_observations (19).json`

The four source roots are consistent across the multi-observation Foundation and relation audits:

- `4cb86b5f965b0cdfbe7e272304950dc8a15bda41802bda0ba2c1a85cc1184af5`
- `f1f5158fad120f3bc1c8e2f5b12b9b55e0a32d9ee27c8c6ae99f90eeac7b1d15`
- `7bc0db97b50a7ce4a7bafeb8262d9e1bb572bf02fb7b6f22ac6713b87f917d4f`
- `31b21f4aa15ea54f92b74ae004699186c9f19bc53c2836b5a94114423b964431`

## T5 Foundation telemetry result — fail-closed path physically proven

The Foundation export contains the newly wired field `t5_corridor_audit_binding_v0_1` for all four observations.

For all four roots it reports:

- `status = UNKNOWN_FAIL_CLOSED`
- `reason = NO_PRECOMPUTED_RUNTIME_T5_AUDIT_FOR_SOURCE`
- `source_binding_verified = false`
- `profile_run_binding_verified = false`
- `telemetry_origin = UNAVAILABLE`
- `t5_corridor_recomputed_by_binding = false`
- `t5_corridor_audit = null`
- `cross_observation_reuse_allowed = false`
- `creates_new_evidence = false`
- `scientific_writeback_allowed = false`

This is the correct physical result for a Foundation export made without first computing a matching T5 `TruthNegativeContinuousPreview` audit for those exact source SHA-256 roots in the same app process.

This device round therefore **proves the fail-closed branch of the new T5→Foundation plumbing**. It does not yet prove the positive `SOURCE_BOUND_PRECOMPUTED_T5_AUDIT_AVAILABLE` branch.

Foundation must not be changed to recompute T5 automatically. The next positive-path test must deliberately produce a matching T5 audit first and then export Foundation without killing/restarting the app process.

## Authority/promotion audit from the same device round

No authority leak was found.

The field-response repeatability export remains descriptive only:

- repeatable camera-system response not proven;
- lens-only vignetting not proven;
- scene illumination not separated;
- sensor angular response not separated;
- optical axis not proven;
- calibration not promoted;
- correction gain not allowed;
- no source mutation/new evidence/writeback.

Observation/world field separation also remains unpromoted and read-only:

- camera-system response not proven;
- lens-only vignetting not proven;
- scene illumination not separated;
- sensor angular response not separated;
- calibration not promoted;
- correction not authorized;
- no scientific writeback.

Foundation `ScientificPromotionState/0.1` remains `NOT_PROMOTED_FAIL_CLOSED` with `NO_INTERNAL_PROMOTION_DECISION`; world registration, world→source bridge, radiometric/field/colour/noise/optical/temporal/geometry/world-space-noise and scientific denoise promotions remain false.

## Additional real-device finding: stale v0.2 tile-read interpretation

The same Foundation export revealed a separate diagnostic inconsistency.

For every source, the existing hash-bound PassArtifact telemetry correctly reports:

- `route_attribution = EXACT_GAUGE_RETAINED_V0_3`
- `binding_verified = true`
- `telemetry_origin = CURRENT_PROFILE_RUN`
- no route contradiction;
- `stage2_gauge_scan_passes_actually_used = 1`
- `pass2_stage2_tile_reads_avoided = 3072`
- `optimization_applied = true`
- `candidate_applied = false`
- `source_values_modified = false`.

At the same time, the older `ScientificMasterTileReadAttribution/0.1` still assumed the historical canonical-v0.2 two-pass schedule and therefore compared the measured `3072` RAW calls against a hardcoded expected `6144`. It reported `UNKNOWN_FAIL_CLOSED` even though explicit PassArtifact diagnostics had already proven the active Exact Gauge v0.3 route.

This was a **diagnostic attribution defect**, not a Scientific-Master or Exact-Gauge scientific defect. The exported safety state remained closed.

## Route-aware tile-read repair

Only the diagnostic aggregator was changed. Scientific core, canonical v0.2, Exact Gauge v0.3 core, sealed CFA, Scientific Master, N2 reconstruction, T5 runtime and Room Capsule were not modified.

Runtime code patch:

`6a8e01763ca8eb0f6d371c1588a7fbeb91f0af50`

`FreeWorldPerformanceDiagnosticsV01.scientificMasterTileReadAttribution(...)` now receives the already-existing `ScientificMasterPassArtifactAttribution/0.1` object.

Permanent rule:

`explicit hash-bound PassArtifact route attribution -> independent source-read-count reconciliation`

Never:

`source-read count -> guessed route`.

### Exact Gauge v0.3 admitted diagnostic footprint

Only when explicit route telemetry is available, bound, contradiction-free and firewall-clean, and reports `EXACT_GAUGE_RETAINED_V0_3`, the tile-read diagnostic expects:

- one active source-read pass;
- `source_read_raw_call_count == reconstruction_call_count`;
- Stage-2 gauge scan passes = 1;
- `pass2_stage2_tile_reads_avoided == reconstruction_call_count`.

A successful reconciliation reports:

`EXACT_GAUGE_V0_3_ONE_PASS_RECONCILED`

with pass 1 = reconstruction count, pass 2 = 0 and avoided pass-2 calls = reconstruction count.

### Canonical v0.2 fallback footprint

Only when explicit route telemetry proves `CANONICAL_V0_2_FALLBACK`, the diagnostic expects:

`source_read_raw_call_count == 2 * reconstruction_call_count`

and reports:

`CANONICAL_V0_2_TWO_PASS_RECONCILED`.

Anything else remains `UNKNOWN_FAIL_CLOSED`.

## Validation of the repair

Route-aware tile-read integrity workflow:

- run: `37298021441`
- exact head: `665be2419b2d652206c624864edb864f86cdac4b`
- conclusion: **SUCCESS**

Android APK build of the actual runtime-code patch:

- workflow run: `37297805632`
- exact code head: `6a8e01763ca8eb0f6d371c1588a7fbeb91f0af50`
- conclusion: **SUCCESS**
- APK size: `8,588,087 bytes`
- APK SHA-256: `8ded03cc38375afc2b41d49b7150ae757dac71039aa94f40f18f60fa23d45141`
- signing certificate SHA-256: `a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`
- versionCode: `26100127`
- artifact: `DRAW-free-world-research-debug-arm64-stable-signed`
- artifact ID: `11339942673`
- artifact ZIP SHA-256: `9eafa0490115f9ca3480beeb380a7dbb8dbe0637ae27a1c02dc565a20486a67c`

## Exact next device test

Use only the APK above for the next round.

### A. Tile-read attribution retest

Run/export Foundation normally for the four sources. For an Exact Gauge v0.3 current-run profile with 3072 reconstruction calls, require:

- `route_attribution = EXACT_GAUGE_RETAINED_V0_3`
- `tile_read_attribution_v0_1.status = EXACT_GAUGE_V0_3_ONE_PASS_RECONCILED`
- `expected_active_route_raw_call_count = 3072`
- `aggregate_source_read_raw_call_count = 3072`
- `pass_1_raw_call_count = 3072`
- `pass_2_raw_call_count = 0`
- `pass_2_raw_calls_avoided = 3072`
- `raw_call_count_reconciles = true`
- `optimization_applied = true`
- `candidate_applied = false`
- `source_values_modified = false`
- no new evidence or scientific writeback.

### B. Positive T5→Foundation binding test

For one exact source SHA:

1. keep the app process alive;
2. open/run the exact source through the PRO/D.RAWnegative `TruthNegativeContinuousPreview` route so `T5CorridorAuditV01` is actually computed and published;
3. without restarting/killing the app process, run/export Foundation including that same exact source;
4. inspect `performance_diagnostics_v0_1.observations[*].t5_corridor_audit_binding_v0_1` for that SHA.

Expected positive result:

- `status = SOURCE_BOUND_PRECOMPUTED_T5_AUDIT_AVAILABLE`
- exact source SHA match;
- `source_binding_verified = true`
- `profile_run_binding_verified = false` remains intentionally false;
- `t5_corridor_recomputed_by_binding = false`
- nested T5 audit present;
- Room Capsule exact-preserving bypass when no admitted world evidence exists;
- exposure application count exactly 1;
- one physical frame and one independent evidence source;
- candidate/writeback/new-evidence firewalls closed.

If the matching T5 preview was not executed in-process, `UNKNOWN_FAIL_CLOSED / NO_PRECOMPUTED_RUNTIME_T5_AUDIT_FOR_SOURCE` remains the correct result.

## Promotion state

No scientific promotion is granted by this round. PR #130 remains a candidate/draft until wider CI/governance state and the positive real-device T5 binding test are resolved.

**One Free World. Many sealed observations. One evidence law.**
