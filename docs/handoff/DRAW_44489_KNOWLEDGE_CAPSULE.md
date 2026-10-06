# D.RAW 44489 — current knowledge capsule

Purpose: binding recovery state for a new chat. Restore this file plus the live PR #130 state before changing or testing anything.

Project: **D.RAW** (`TruthRaw` remains repository/history naming)  
Repository: `1DiDaDo1/TruthRaw`  
Continuation code: **44489**  
Recovery marker: **`[KCR-44489-2026-10-05-FOUNDATION-DEVICE-ROUND]`**  
Frozen UI/CI audit reference: **PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`**  
Current UI implementation branch from that exact SHA: **`feat/draw-workspace-free-raster-v01`**  
Scientific candidate branch at the frozen audit point: **`fix/android-exact-gauge-pass-artifact-v03`**  
Active scientific PR at the frozen audit point: **#130** — open, draft, not merged  
Latest detailed device-round handoff: `docs/handoff/DRAW_44489_FOUNDATION_DEVICE_ROUND_2026-10-05.md`

Always resolve live PR #130 HEAD before mutating that PR. The UI implementation branch above is deliberately forked from the frozen `4e4f358a...` audit reference so UI work cannot silently rewrite the meaning of the exact-head CI audit. Documentation/checker commits may be newer than the exact runtime source used to build a tested APK; never force-reset to a remembered SHA.

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

### Exact-head CI audit correction (2026-10-05)

The UI audit is anchored to PR #130 head `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f` and must not inherit status from any older SHA.

- `Documentation Governance`: exact-head run exists via `push` and is **SUCCESS**.
- `Research Integrity Guard`: **NO PROVEN RUN ON THIS EXACT HEAD**. Audit state is `NOT RUN / UNPROVEN`, not red and not green.
- `Lifecycle Contract`: **NO PROVEN RUN ON THIS EXACT HEAD**. Audit state is `NOT RUN / UNPROVEN`, not red and not green.
- On the older PR head `d2c26de69ea192ba23ade949aa2656ed6907e9ec`, `integrity` and `lifecycle-contract` were genuinely failing gates. Those failures belong only to that earlier SHA and may not be projected onto `4e4f358a...`.
- The exact `4e4f358a...` PR diff contains **43 changed files**, including workflow files. The remaining governance question is causal only: compare exact workflow YAML `on:`, branch filters, `paths`, `paths-ignore` and job-level `if:` against those 43 files to explain why a workflow was or was not dispatched.
- An absent workflow run is not scientific evidence and does not change any authority state. CI may validate contracts/provenance/cables; it may never manufacture MEASURED evidence or scientific authority.

## 12. Recovery pointers

Read in this order after `44489`:

1. `docs/handoff/DRAW_44489_KNOWLEDGE_CAPSULE.md`
2. `docs/handoff/DRAW_44489_FOUNDATION_DEVICE_ROUND_2026-10-05.md`
3. `docs/handoff/DRAW_44489_T5_FOUNDATION_TELEMETRY_READY_2026-10-05.md`
4. `docs/handoff/DRAW_44489_T5_RUNTIME_AUDIT_READY_2026-10-05.md`
5. `docs/handoff/DRAW_44489_TEST_START_2026-10-04.md`
6. live PR #130 state

Historical negative experiments and older handoffs remain provenance and must not be rewritten away.

## 13. Global UI / product audit — saved before Free Raster implementation

Audit basis: the known D.RAW vision image **“D.RAW – van Lens naar Vrije Raster-Weergave (Eenvoudig Overzicht)”** plus the Android UI/runtime present on frozen head `4e4f358a...`.

Vision law for the UI:

`Werkelijke wereld -> Lens & Sensor -> Sealed Observation -> Kalibratie & Meting -> Scientific Master / reconstructie -> continue wetenschappelijke negatieve representatie -> Vrije Raster Projectie -> Free World Observation Graph -> View / Appearance`

The UI may expose and navigate those layers, but it must not collapse their authority classes or imply that a view/projection created new evidence.

### 13.1 What is already substantially present

- Launcher: Bestand/Camera, PURE/ADVANCED/PRO, Research & JSON, settings and implementation guide.
- Universal camera: live preview, dynamically discovered ultra-wide/main/tele acquisition roles, focus marker, AF lock, macro loupe/pinch, five-second capture timer, `RAW_SENSOR seal -> derived DNG -> Universal Intake`.
- ADVANCED: downstream exposure/light balance, shadow recovery, display-HDR, detail, colourfulness and restoration controls.
- PRO: Open Scene / Light Transport / provenance / professional export and research entry points using the same upstream scientific core.
- Research Hub: multi-observation/JSON workbench, Calibration Observation Records, Free World Foundation and Global Research Snapshot.
- Main workbench: unified output-preview state, long-running restoration/projectie job recovery and research-session persistence already exist.

### 13.2 UI gaps against the vision

1. **D.RAW Workspace / Vrije Raster-weergave — highest priority.**
   One central image/workspace should become the natural operational surface after source selection. It should bring source, route, projection intent, output raster, authority visibility and export navigation together without building a second scientific pipeline.
2. **Evidence / Authority Inspector.**
   User-visible distinction of `MEASURED`, `CALIBRATED_ESTIMATE`, `RECONSTRUCTED`, `CENSORED`, `UNKNOWN`, `APPEARANCE`, with source/provenance context. It must display existing authority, never infer stronger authority from UI state.
3. **Visual Observation Graph.**
   Multiple sealed observations should be shown as separate source nodes, with only proven relations between them. A RAW_SENSOR plus its derived DNG remains one physical observation, not two independent measurements.
4. **Live Appearance around the image.**
   ADVANCED controls should eventually operate around the active preview with reset/before-after while remaining downstream.
5. **Compact pipeline indicator.**
   A user-readable `SOURCE -> SEALED -> SCIENTIFIC -> CONTINUOUS/RECONSTRUCTED -> FREE RASTER -> APPEARANCE` map should expose state without turning a green UI status into scientific authority.

### 13.3 Important UI-status classification

- Layers 1–2 (world/lens/sensor ingress): substantially represented.
- Sealed Observation: technically present, but needs a simple visible evidence card.
- Calibration/measurement: strong in Research, but normal UI needs a compact “what is actually known?” view.
- Scientific Master/reconstruction: strong internally, but authority classes are not yet sufficiently visible to normal users.
- Continuous scientific-negative representation: runtime routes exist, but product identity is not yet clear enough in normal UI.
- Free Raster: **largest immediate product/UI gap**.
- Observation Graph: data/research concepts exist; graphical productization remains incomplete.
- Appearance: controls exist; integrated image-centric workflow remains incomplete.

### 13.4 “Should everything work now?” — exact audit answer

Do **not** record “everything works” yet.

The normal route is sufficiently connected that most of it is expected to function, but whole-product operation remains unproven until a real-device end-to-end run covers at least:

`launcher/workspace -> physical camera -> sealed RAW_SENSOR -> derived DNG / Universal Intake -> Scientific Master -> PURE preview -> ADVANCED preview -> PRO -> restoration/projectie -> export -> reopen`

Additional intentional limitations are not bugs:

- several scientific runtimes remain `IMPLEMENTED CANDIDATE / NOT PROMOTED`;
- DNG is the best-proven admitted full route;
- NEF remains a limited measurement-only decoder path;
- proprietary RAW formats without admitted decoder adapters stay immutable/fail-closed rather than being guessed.

### 13.5 Workspace implementation contract

The immediate next implementation on `feat/draw-workspace-free-raster-v01` is **D.RAW Workspace / Vrije Raster-weergave v0.1**.

Non-negotiable rules:

- reuse the existing proven MainActivity/Scientific/Open-Scene/preview/export machinery; do **not** create a second decoder or scientific core;
- workspace controls are navigation/presentation/projection intent unless an existing scientific runtime explicitly supplies stronger state;
- arbitrary output raster may change representation but never create `MEASURED` samples;
- no UI zoom, crop, resolution, appearance control or status colour may change authority;
- source SHA/provenance remains the authority anchor;
- UNKNOWN stays visible/fail-closed;
- no AI/ML scientific inference;
- keep the current direct Bestand/Camera paths available while introducing the workspace so existing proven paths remain recoverable;
- first implementation may expose a safe orchestration shell around existing workbench functions, but it must label unavailable runtime bindings honestly rather than simulate them.

### 13.6 Planned validation after implementation

First validate compilation and static routing. Then perform a real-device UI round proving:

1. Workspace opens from the application without breaking existing direct routes.
2. Existing RAW/DNG picker still reaches MainActivity.
3. Universal camera still reaches the same sealed RAW_SENSOR -> derived DNG route.
4. PURE/ADVANCED/PRO route selection remains shared and does not mutate source/scientific state.
5. Authority legend/inspector does not report runtime authority unless actually bound.
6. Free-raster controls affect only view/projection intent until a proven runtime bridge is attached.
7. Existing export/restoration/research routes remain reachable.
8. No candidate is promoted and no Scientific Master writeback is enabled by the workspace.

After that user journey is physically proven, return to the exact workflow causality audit for Research Integrity Guard / Lifecycle Contract only if still relevant to the then-current head. Do not resurrect old red states by memory.

## 14. Workspace / Free Raster device acceptance — superseding product status

The first real-device Workspace / Free Raster round is now accepted for product continuation. The full evidence record is:

`docs/handoff/DRAW_44489_WORKSPACE_DEVICE_ACCEPTANCE_2026-10-05.md`

The accepted device output family is bound to sealed source SHA-256:

`fe88a0acd2f95d35353ef9e4c925e50923e100a257bd43f545df4c09b09340b8`

Observed acceptance facts:

- full-colour Scientific Master DNG is a three-channel IEEE Float32 LinearRaw representation at 4080×3072;
- its private contract reports one physical frame and one independent evidence source;
- `scientific_master_modified=0`, `appearance_applied=0`, `counterfactual_observation_created=0`;
- embedded JPEG remains a non-authority preview and cannot write back scientifically;
- Universal Observation Calibration Atlas is source-bound, keeps frontside inspection `APPEARANCE_DERIVED_ONLY`, and refuses unsupported automatic radiometric/colour/noise/light/optical correction;
- Observation Optical Field Chart exports a measured composite scene/lens/sensor field signal but does not claim lens-only vignetting, separated illumination, sensor angular response or a proven optical axis;
- Global Research Snapshot remains `IMPLEMENTATION_MAP_NOT_PHOTO_EVIDENCE`; all promotion gates remain non-automatic and scientific writeback remains disabled;
- camera/lens/vendor identity does not become a scientific-model or calibration key;
- no source sample mutation, new measured sample, new evidence or scientific writeback is admitted by the supplied outputs.

This section supersedes the pre-test wording in **13.4** for Workspace v0.1 product acceptance only. It does **not** promote research candidates, make UNKNOWN quantities known, or claim that future multi-observation/world-space functionality is already scientifically validated.

Accepted Workspace runtime code checkpoint:

`c85b9805a56681b1adbc39f58b95724c4810907c`

Documentation-only commits may follow this checkpoint without changing APK runtime identity. The final APK must still be tied to a successful assemble/verify workflow and must record its exact runtime SHA, artifact ID, byte size, APK SHA-256 and stable signing-certificate SHA-256 before delivery.

## 15. Workspace / Free Raster v0.2 continuation — flexible cables and non-destructive workbench

This section records the v0.2 continuation after the accepted v0.1 device round. It is a **product/architecture/runtime-candidate status**, not scientific promotion.

### 15.1 Exact PR / branch boundary

Immediately before this capsule-only documentation commit, PR #131 had implementation/documentation checkpoint:

`f5d70040cb965fb727d3679f5412a4970c53dd4d`

State at that checkpoint:

- branch: `feat/draw-workspace-free-raster-v01`;
- PR #131: open, draft, not merged, mergeable=true;
- base/frozen scientific-audit reference: PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`;
- PR #130 was not modified by this Workspace continuation.

The capsule commit itself is documentation-only; it advances the branch head without changing Android runtime semantics.

### 15.2 v0.2 product architecture reference

The user-supplied image **“D.RAW – Volledige Project Architectuur, Kabels, Input, Opties en Output (v0.2)”** is the current concrete product/architecture reference. It does not prove runtime implementation or scientific admission of every depicted block.

Canonical product cable:

`Input -> Universal Intake -> Scientific Core -> Unified Output State -> PURE / ADVANCED / PRO -> Vrije Raster / Output -> Export`

Scientific/world architecture remains evidence-bound. The UI diagram may guide placement and responsibility, but it may never manufacture authority.

Free Raster is downstream output/projection. It must consume an already admitted/rendered output state; it is not a second RAW decoder, second Scientific Master, second reconstruction route or second source of truth.

### 15.3 Permanent flexible-inside cable rule

New binding architecture rule:

**Stable outside. Flexible inside. Evidence law unchanged.**

Every cable must keep its outer evidence/provenance/safety contract explicit while permitting internal stages, adapters, algorithms and capability routing to evolve.

Stable outer boundary includes, where applicable:

- source/observation binding;
- payload/type contract;
- provenance contract;
- authority contract;
- uncertainty/censoring state;
- writeback permission;
- UNKNOWN/failure semantics.

Flexible internal structure may include deterministic adapters, alternate implementations behind the same contract, optional telemetry, faster exact-preserving implementations and future source/output capabilities.

Flexibility never allows:

- creation of new `MEASURED` evidence;
- silent promotion of `UNKNOWN`;
- mutation of sealed evidence;
- hidden Scientific Master writeback;
- conversion of appearance into scientific authority;
- AI/ML as a scientific inference layer.

Implementation/documentation anchors now present:

- `suite_android/app/src/main/java/com/truthraw/adaptiveui/DrawFlexibleCableContractV01.kt`;
- `docs/DRAW_FLEXIBLE_CABLE_INTERNALS_v0_1.md`;
- refactored `PresentationRasterLoader.kt` with a stable `load(...)` boundary and internally extensible `Decoder` adapter chain.

The first presentation decoder is `android.bitmap_factory.v1`. This is an implementation adapter, not scientific authority.

### 15.4 External raster loading status

The external JPG/PNG/WebP path remains explicitly:

`EXTERNAL_PRESENTATION_RASTER / PRESENTATION_ONLY`

`PresentationRasterLoader` now has an internally extensible decoder cable while retaining provider-safe Android loading:

- best-effort persistable read permission;
- `openFileDescriptor(uri, "r")` preferred;
- fresh `openInputStream(uri)` fallback;
- separate fresh handles for bounds and actual decode;
- sampled preview bounded by the configured display dimension;
- distinct permission/provider/type/decode/memory failures;
- no RAW parsing;
- no Scientific Master mutation;
- no authority creation.

### 15.5 Unified Output -> Free Raster bridge progress

New runtime file:

`suite_android/app/src/main/java/com/truthraw/adaptiveui/UnifiedOutputPresentationBridge.kt`

Purpose:

`existing Unified Output Ready bitmap -> lifetime-safe presentation snapshot -> Workspace / Free Raster consumer`

The bridge is process-local and presentation-only. It does **not** render, reconstruct, run T5, modify Scientific Master or infer scientific authority.

Important bitmap-lifetime rule: MainActivity owns and may recycle its own `UnifiedOutputPreviewResult.Ready.bitmap`. Workspace must therefore never retain that direct bitmap reference. The bridge publishes an owned copy and returns a separate consumer-owned copy.

The bridge itself follows the flexible-inside rule through an internal `Transport` adapter. Current default transport:

`process_memory_owned_copy.v1`

Current bridge authority/safety constants remain presentation-only and no-scientific-writeback.

**Status boundary:** the bridge class exists, but the complete publisher/consumer wiring is not yet proven complete. Do not report the internal D.RAW-output Free Raster path as finished until the existing `UnifiedOutputPreviewResult.Ready` creation/publication path and Workspace consume action are actually connected and compiled/tested.

Exact remaining cable:

`UnifiedOutputPreviewResult.Ready -> UnifiedOutputPresentationBridge.publish(...) -> Workspace consume -> Free Raster`

No second renderer or T5 evaluation may be introduced to complete this connection.

### 15.6 Permanent non-destructive workbench rule

The D.RAW Workbench must be non-destructive for **RAW/DNG scientific sources and ordinary JPEG/PNG/WebP source images**.

Canonical model:

`IMMUTABLE_SOURCE + REVERSIBLE_EDIT_STATE -> PRESENTATION / OUTPUT`

`EXPORT -> NEW_DERIVED_OUTPUT`

Never:

`SOURCE -> EDIT -> OVERWRITE SOURCE`

Hard consequences:

- sealed RAW/CFA evidence remains immutable;
- D.RAW Observation Records remain unchanged by workbench edits;
- Scientific Master remains unchanged by appearance/output controls;
- source RAW/DNG is never overwritten by workbench editing;
- imported JPEG/PNG/WebP remains read-only as the base image;
- appearance operations are stored separately and remain individually reversible;
- crop/framing, rotation, pan, zoom, Free Raster x/y and scale are downstream view/output transforms;
- reset returns to the same original source with no edit operations;
- previews are render results, never replacement source truth;
- JPEG is not repeatedly decode -> edit -> re-encode -> overwrite as working state;
- explicit export creates a new derived file with lineage/provenance;
- PURE / ADVANCED / PRO share the same admitted source/scientific core and differ downstream only.

New runtime contract anchor:

`suite_android/app/src/main/java/com/truthraw/adaptiveui/NonDestructiveWorkbenchStateV01.kt`

It stores an immutable/read-only `SourceBinding` plus open, reversible downstream `EditOperation` records. Operation and parameter identifiers remain extensible instead of being frozen into a closed list. The contract permits only downstream domains `APPEARANCE_ONLY`, `OUTPUT_TRANSFORM_ONLY` and `VIEW_ONLY` and hard-codes these safety states false:

- `SOURCE_MUTATION_ALLOWED=false`;
- `SCIENTIFIC_MASTER_WRITEBACK_ALLOWED=false`;
- `OVERWRITE_SOURCE_ON_EXPORT_ALLOWED=false`.

Documentation anchor:

`docs/DRAW_NON_DESTRUCTIVE_WORKBENCH_v0_1.md`

**Status boundary:** this contract now exists, but every existing ADVANCED/Workspace control has not yet been migrated to it. Do not claim global non-destructive runtime proof until integration and tests demonstrate that all relevant edit/export paths obey it.

### 15.7 Appearance reference — dog JPEG

The supplied black/white dog JPEG is retained only as an **appearance stress-test/reference**, never as calibration evidence.

Desired downstream qualities include:

- deeper black fur without crushed strand detail;
- retained texture in white fur/highlights;
- smooth highlight roll-off instead of hard display clipping;
- useful midtone separation without halos;
- restrained warmth/colorfulness increases when the render is dull/cool;
- neutral white/black fur protection;
- subtle detail/texture enhancement.

No observation-specific scientific calibration values may be learned from this single JPEG.

### 15.8 Current APK / validation boundary

The v0.1 accepted APK identity from section 14 remains historical evidence only. Runtime code has changed since that checkpoint.

Therefore:

- no old APK SHA/artifact ID may be presented as the build identity of the v0.2 continuation;
- PR #131 remains draft;
- fresh current-head compilation/CI is required;
- a new APK must be tied to its exact runtime SHA, artifact ID, byte count, APK SHA-256 and signing-certificate SHA-256;
- real-device validation is required for both external presentation raster and internal D.RAW Unified Output consumption;
- none of these product validations promotes a scientific candidate.

### 15.9 Exact next implementation / acceptance order

1. Trace the common existing creation/publication boundary of `UnifiedOutputPreviewResult.Ready` without duplicating render logic.
2. Publish that existing Ready result through `UnifiedOutputPresentationBridge` using source/route binding already known upstream.
3. Add Workspace action/state to consume the bridge snapshot fail-closed; absence of a current publication must remain “no current D.RAW output available”.
4. Keep external JPG/PNG/WebP as a separate `PRESENTATION_ONLY` input path.
5. Integrate `NonDestructiveWorkbenchStateV01` into Workspace/ADVANCED state so source binding and edit recipe are separate.
6. Extend the existing Appearance cable — not a duplicate renderer — with reversible black point, white point, highlight roll-off, shadow/midtone and colour/detail controls.
7. Add static/runtime checks that workbench edits cannot mutate source/Scientific Master and export cannot overwrite the source by default.
8. Compile/build on the exact new runtime head and record exact build provenance.
9. Real-device test a normal JPEG plus one D.RAW observation through internal Unified Output -> Free Raster.
10. Keep PR #131 draft until those checks are green; keep PR #130 frozen and scientifically unmodified.

This section supersedes any earlier implication that the internal Unified Output -> Free Raster bridge or fully non-destructive edit-state integration was already complete. The architecture/contract pieces exist; their remaining wiring and physical validation are explicitly open.
