# D.RAW 44489 — current knowledge capsule

Purpose: binding recovery state for a new chat. Restore this file plus the live PR #130 state before changing or testing anything.

Project: **D.RAW** (`TruthRaw` remains repository/history naming)  
Repository: `1DiDaDo1/TruthRaw`  
Continuation code: **44489**  
Recovery marker: **`[KCR-44489-2026-10-05-FOUNDATION-DEVICE-ROUND]`**  
Active candidate branch: **`fix/android-exact-gauge-pass-artifact-v03`**  
Active PR: **#130** — open, draft, not merged; last observed `mergeable=false`  
Latest detailed device-round handoff: `docs/handoff/DRAW_44489_FOUNDATION_DEVICE_ROUND_2026-10-05.md`

Always resolve live PR #130 HEAD before mutation. Documentation/checker commits may be newer than the exact runtime source used to build a tested APK; never force-reset to a remembered SHA.

## 1. Permanent scientific law

- **Seal the evidence, not the thinking.**
- **MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE.**
- Representation may become richer than the source; knowledge claims may never become richer than evidence.
- Direct CFA / RAW_SENSOR evidence is immutable and sealed.
- One physical frame remains one physical frame; derived views never create additional captures.
- Scientific Master is separate from export/presentation/Appearance.
- `UNKNOWN` is valid and must never silently become zero, certainty or an estimate.
- Precision, resolution, reconstruction, registration and performance never create authority.
- Camera/lens/vendor/container identity may describe provenance or route parsing; it may not select scientific truth or calibration by name alone.
- AI/ML/neural/generative inference is not allowed as scientific evidence.
- SOURCE/SENSOR SPACE, WORLD/SCENE SPACE and VIEW/OUTPUT SPACE remain distinct.
- **One Free World. Many sealed observations. One evidence law.**

Canonical architecture:

`readable source -> sealed Observation -> structural inspection -> Source Capability Envelope -> calibration/reconstruction with explicit authority -> Scientific Master -> Dynamic Authority + uncertainty -> Observation-bound TruthNegative -> Free World Observation Graph -> Deep Scene / Light Transport -> Room Capsule -> View / Appearance -> free raster projection`

## 2. Scientific Master / Exact Gauge v0.3

Scientific Master remains the downstream scientific authority source. Exact Gauge Retained Artifact v0.3 is an execution/performance artifact, not a new evidence class.

Permanent v0.3 rules:

- canonical v0.2 remains complete fallback before candidate semantic start;
- v0.3 retains exact eligible Float32 gauge bits so the second Stage-2 reread can be avoided without changing the canonical result;
- after semantic processing starts there is no replay into another scientific route;
- explicit PassArtifact diagnostics are bound to Scientific-Master SHA-256;
- route attribution is never inferred from timing or raw-read counts;
- `candidate_applied=false`;
- `source_values_modified=false`;
- `creates_new_evidence=false`;
- `scientific_writeback_allowed=false`.

Real-device exports in the 2026-10-05 device round proved Exact Gauge v0.3 active for all four tested sources: explicit route attribution `EXACT_GAUGE_RETAINED_V0_3`, hash binding verified, one Stage-2 gauge pass, and 3072 avoided second-pass tile reads per source. Scientific firewalls remained closed.

## 3. T5 / Room Capsule runtime corridor

Existing route:

`Scientific Master -> v0.4 Deep Scene Contribution -> v0.5 Deep Scene Binding -> v0.6 Light Transport -> existing Room Capsule host -> v0.7 Appearance Resolve`

Room Capsule was never a missing architecture. Without admitted geometry/material/illumination world evidence it must be an **exact-preserving bypass**.

`T5CorridorAuditV01` already interprets native corridor telemetry read-only/fail-closed. `TruthNegativeContinuousPreview.Ready` computes that existing audit once. `ResearchPerformanceT5CorridorBindingV01` carries only that precomputed object into Foundation diagnostics by exact source SHA.

Permanent T5 binding rules:

- process-local diagnostic transport only;
- exact `source_sha256` binding;
- no cross-observation reuse;
- no second T5/Room-Capsule evaluation by Foundation;
- `profile_run_binding_verified=false` unless independently proven — source binding is not equivalent to same-profiler-run evidence;
- missing/mismatch/contradiction -> `UNKNOWN_FAIL_CLOSED`;
- no new evidence, no writeback, no candidate application.

## 4. 2026-10-05 real-device Foundation T5 result

Four uploaded real-device exports used the same four source roots:

- `4cb86b5f965b0cdfbe7e272304950dc8a15bda41802bda0ba2c1a85cc1184af5`
- `f1f5158fad120f3bc1c8e2f5b12b9b55e0a32d9ee27c8c6ae99f90eeac7b1d15`
- `7bc0db97b50a7ce4a7bafeb8262d9e1bb572bf02fb7b6f22ac6713b87f917d4f`
- `31b21f4aa15ea54f92b74ae004699186c9f19bc53c2836b5a94114423b964431`

Foundation physically exported `t5_corridor_audit_binding_v0_1` for all four roots. All four correctly reported:

`UNKNOWN_FAIL_CLOSED / NO_PRECOMPUTED_RUNTIME_T5_AUDIT_FOR_SOURCE`

with no recomputation, no cross-source reuse and no writeback/evidence creation.

Conclusion: **the fail-closed branch of T5→Foundation plumbing is physically validated.**

The positive branch `SOURCE_BOUND_PRECOMPUTED_T5_AUDIT_AVAILABLE` remains unproven because the device round did not first execute a matching `TruthNegativeContinuousPreview` T5 audit for those exact sources in the same app process.

Do not “fix” this by running T5 automatically during Foundation export. The next test must precompute T5 intentionally and then export Foundation in the same living process.

## 5. Route-aware tile-read attribution — defect found and repaired

The same device Foundation export revealed a stale diagnostic assumption:

- explicit PassArtifact telemetry correctly said Exact Gauge v0.3 was active;
- measured RAW source calls were 3072 for 3072 reconstruction calls;
- Exact Gauge reported one Stage-2 gauge pass and 3072 avoided second-pass reads;
- old `ScientificMasterTileReadAttribution/0.1` still expected canonical-v0.2 `6144 = 2 × 3072`, so it reported `UNKNOWN_FAIL_CLOSED`.

This was **diagnostic telemetry drift**, not a Scientific-Master or Exact-Gauge scientific failure.

Repair code head:

`6a8e01763ca8eb0f6d371c1588a7fbeb91f0af50`

Only `FreeWorldPerformanceDiagnosticsV01` runtime diagnostic aggregation changed. It now follows:

`explicit hash-bound PassArtifact route -> independent raw-read count reconciliation`

Never:

`raw-read count -> route inference`.

Admitted diagnostic outcomes:

- Exact Gauge v0.3 + one-pass footprint -> `EXACT_GAUGE_V0_3_ONE_PASS_RECONCILED`;
- explicit canonical v0.2 fallback + two-pass footprint -> `CANONICAL_V0_2_TWO_PASS_RECONCILED`;
- anything missing/contradictory/inconsistent -> `UNKNOWN_FAIL_CLOSED`.

The historical two-pass expected count remains exportable for comparison, while `expected_active_route_raw_call_count` represents the route actually proven by PassArtifact diagnostics.

No change was made to canonical v0.2, Exact Gauge v0.3 core, sealed CFA, Scientific Master, reconstruction, T5 native runtime or Room Capsule.

## 6. Validation/build identity after tile-attribution repair

Route-aware integrity gate:

- workflow: `Scientific Master Tile-Read Attribution v0.1 Integrity`
- run: `37298021441`
- head: `665be2419b2d652206c624864edb864f86cdac4b`
- result: **SUCCESS**

Android/APK build of runtime patch:

- workflow: `D.RAW Free World Research APK`
- run: `37297805632`
- exact runtime source head: `6a8e01763ca8eb0f6d371c1588a7fbeb91f0af50`
- result: **SUCCESS**
- artifact: `DRAW-free-world-research-debug-arm64-stable-signed`
- artifact ID: `11339942673`
- artifact ZIP SHA-256: `9eafa0490115f9ca3480beeb380a7dbb8dbe0637ae27a1c02dc565a20486a67c`
- APK bytes: `8,588,087`
- APK SHA-256: `8ded03cc38375afc2b41d49b7150ae757dac71039aa94f40f18f60fa23d45141`
- signing certificate SHA-256: `a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`
- versionCode: `26100127`

This APK supersedes `90aeee... / bc203694...` specifically for the next Foundation diagnostics test.

## 7. Same device round: authority/promotion remained closed

Field-response repeatability remains descriptive only. Four observations are sufficient to compute metrics, but the export explicitly does **not** prove camera-system response, lens-only vignetting, separated illumination, sensor angular response or optical axis. Calibration remains unpromoted and correction gain unauthorized.

Observation/world field separation remains read-only. World structure is not yet scientifically registered; camera/lens identity is not used as a calibration key; calibration and correction remain unpromoted.

Foundation `ScientificPromotionState/0.1` remains:

`NOT_PROMOTED_FAIL_CLOSED / NO_INTERNAL_PROMOTION_DECISION`

World registration, world→source bridge, radiometric response, field response, colour, noise, optical support, temporal relation, geometry, world-space noise separation and scientific-denoise approvals remain false. Validated world→source mapping is not attached. No scientific writeback is allowed.

## 8. Reconstruction result that must not be rediscovered

Anchor-Constrained Local Reconstruction v0.1 is **NOT PROMOTED**. The real tele hold-out showed worse aggregate MAE/RMSE/bias than baseline on directly comparable points and over-optimistic uncertainty despite better coverage and a small channel-2 benefit. Safety remained closed.

Future reconstruction direction remains deterministic local model selection from structural support, direction, CFA phase and uncertainty. Affine is only one optional model; `no suitable model` is valid.

## 9. Architecture status that remains open

- Universal source admission beyond the best-proven DNG path is incomplete.
- Canonical Source Capability Envelope is not yet fully unified.
- First-class standalone D.RAWnegative write→close→read→re-import→verify round-trip remains incomplete.
- Free Raster productization at arbitrary full-resolution output remains partial.
- Active multi-observation Free World Observation Graph remains the largest incomplete scientific block.
- True metric 3D geometry is not generally admitted.
- Material/illumination authority remains research-only without admitted evidence.
- User-visible authority/uncertainty still trails internal telemetry.
- Full-resolution/multi-observation memory/runtime scaling still needs physical proof.

## 10. Exact next device test

Use only the APK from runtime head `6a8e017...`, SHA-256:

`8ded03cc38375afc2b41d49b7150ae757dac71039aa94f40f18f60fa23d45141`

### Test A — route-aware tile-read attribution

Run/export Foundation normally for the four sources. For each Exact Gauge v0.3 profile with 3072 reconstruction calls, expect:

- `route_attribution = EXACT_GAUGE_RETAINED_V0_3`
- `tile_read_attribution_v0_1.status = EXACT_GAUGE_V0_3_ONE_PASS_RECONCILED`
- `expected_active_route_raw_call_count = 3072`
- `aggregate_source_read_raw_call_count = 3072`
- `pass_1_raw_call_count = 3072`
- `pass_2_raw_call_count = 0`
- `pass_2_raw_calls_avoided = 3072`
- `raw_call_count_reconciles = true`
- `optimization_applied = true`
- all candidate/source/writeback/evidence firewalls remain closed.

### Test B — positive T5→Foundation binding

For one exact DNG/source SHA:

1. keep the app process alive;
2. execute the source through PRO / D.RAWnegative / `TruthNegativeContinuousPreview` so T5 audit is actually computed and published;
3. **do not restart or kill the app**;
4. run/export Foundation including the same exact source;
5. inspect that observation's `t5_corridor_audit_binding_v0_1`.

Expected positive state:

- `SOURCE_BOUND_PRECOMPUTED_T5_AUDIT_AVAILABLE`
- exact source SHA match;
- `source_binding_verified=true`
- `profile_run_binding_verified=false` remains intentionally false;
- `t5_corridor_recomputed_by_binding=false`
- nested T5 audit present;
- Room Capsule exact-preserving bypass if no admitted world evidence;
- exposure application count = 1;
- physical frame count = 1;
- independent evidence count = 1;
- `candidate_applied=false`;
- no new evidence/writeback.

If no matching in-process T5 preview was executed, `UNKNOWN_FAIL_CLOSED / NO_PRECOMPUTED_RUNTIME_T5_AUDIT_FOR_SOURCE` remains correct.

## 11. Promotion/governance boundary

Do **not** call PR #130 promoted or globally green from the local successful gates above. PR remains draft and wider governance/lifecycle/mergeability state must be re-evaluated before merge. Real-device positive T5 binding is still outstanding.

No scientific promotion follows automatically from implementation availability, performance, geometric hypotheses, visual similarity or user-imported records.

## 12. Recovery pointers

Read in this order after `44489`:

1. `docs/handoff/DRAW_44489_KNOWLEDGE_CAPSULE.md`
2. `docs/handoff/DRAW_44489_FOUNDATION_DEVICE_ROUND_2026-10-05.md`
3. `docs/handoff/DRAW_44489_T5_FOUNDATION_TELEMETRY_READY_2026-10-05.md`
4. `docs/handoff/DRAW_44489_T5_RUNTIME_AUDIT_READY_2026-10-05.md`
5. `docs/handoff/DRAW_44489_TEST_START_2026-10-04.md`
6. live PR #130 state

Historical negative experiments and older handoffs remain provenance and must not be rewritten away.
