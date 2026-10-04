# D.RAW 44489 — current knowledge capsule

Purpose: compact but binding recovery state for a new chat. This capsule records what is proven, what remains candidate/research, the current Android/runtime baseline, and the exact point where the next chat should start testing.

Project: **D.RAW** (`TruthRaw` is historical/repository naming)  
Repository: `1DiDaDo1/TruthRaw`  
Continuation code: **44489**  
Recovery marker: **`[KCR-44489-2026-10-03-A]`**  
Active candidate branch: **`fix/android-exact-gauge-pass-artifact-v03`**  
Active PR: **#130**, open/draft/mergeable at the 2026-10-04 refresh  
Runtime/scientific baseline before this documentation refresh: **`2766aca5f8acf0f441c7b4f7fb6d892de9ae5778`**

Always resolve the live PR #130 head before mutation or testing. Documentation commits may move HEAD beyond the runtime/scientific baseline. Never force-reset to a remembered SHA merely to make documentation match.

## 1. Permanent scientific law

D.RAW is deterministic and provenance-bound. Its scientific path does not use AI/ML/neural/generative inference as evidence.

- **Seal the evidence, not the thinking.**
- **MEASURED != RECONSTRUCTED != APPEARANCE.**
- **Representation may become richer than the source; the knowledge claim may never become richer than the evidence.**
- **One Free World. Many sealed observations. One evidence law.**
- Direct-CFA / RAW_SENSOR evidence is immutable and sealed.
- One physical frame remains one physical frame. Derived/virtual views do not create captures.
- Scientific Master is scene-linear scientific state, separate from presentation, export and appearance. Signed values and values above 1 are valid when scientifically meaningful.
- `UNKNOWN` is valid and must never silently become zero, certainty or an estimate.
- Precision, resolution, reconstruction, registration and performance never create authority.
- APK/GCam/computational-RAW/presentation behavior may not define source evidence, calibration or Scientific-Master truth.
- SOURCE/SENSOR SPACE, WORLD/SCENE SPACE and VIEW/OUTPUT SPACE remain distinct.

Canonical architecture:

`readable source -> sealed Observation -> structural inspection -> Source Capability Envelope -> admitted measurement/calibration/reconstruction -> Scientific Master -> Dynamic Authority + uncertainty -> Observation-bound TruthNegative -> Free World Observation Graph -> Deep Scene / Light Transport -> Room Capsule -> View / Appearance -> finite/free raster projection`

## 2. Evidence and authority

Authority/state classes include at least:

`MEASURED`, `CALIBRATED_ESTIMATE`, `RECONSTRUCTED`, `CENSORED`, `UNKNOWN`, `COUNTERFACTUAL`, `APPEARANCE`.

Authority is local, monotone with respect to evidence, and provenance-bound. Registration/interpolation/reconstruction may never auto-upgrade a value to `MEASURED`.

Clipping may be `CENSORED`: a bound can be known while the latent signal remains unknown. Unknown RGB covariance terms remain UNKNOWN/NaN rather than convenience-zero; full covariance requires all terms to be known/consistent. Optics, colour, geometry, material and illumination claims require explicit proven binding.

## 3. Observation and gauge law

A lens/sensor/CFA/readout/capture route belongs to an Observation. TruthNegative is Observation-bound. Multiple observations meet only above that boundary in the Free World layer.

`source capability != proven sample domain`.

The existence of a 200 MP sensor/capability does not prove that a particular frame contains 200 MP measured samples. Camera/vendor identity may route parsing, but it cannot select scientific truth.

TruthRange coordinate family: `T = log2(L/L0)`. Shared coordinate notation is not proof of shared radiance. Cross-observation radiometric fusion requires an admitted common-gauge relation; otherwise it fails closed.

## 4. Current Android/runtime identity

Real application: `suite_android`.

Current branch configuration at the runtime baseline:

- application ID / namespace: **`com.truthraw.adaptiveui`**
- versionCode: **`26100127`**
- versionName: **`0.54-v0.84.2-open-world-authority-corridor-v01`**
- minSdk 31
- targetSdk 37
- ARM64 native target
- C++20 with warnings-as-errors in the Android native build

Do not substitute stale identities from older 44489 snapshots. An APK is a valid current test candidate only after its package/version/signing identity and SHA-256 are checked against the artifact actually being installed.

## 5. Scientific Master and Exact Gauge Retained Artifact v0.3

Scientific Master remains the immutable scientific authority source for downstream layers. Exact Gauge v0.3 is an execution/performance artifact; it does not create evidence or change scientific meaning.

Binding rules:

- canonical v0.2 remains the admitted complete fallback before candidate semantic start;
- v0.3 retains exact eligible Float32 gauge bit patterns so the second Stage-2 reread can be avoided without changing the canonical result;
- after semantic processing starts there is **no replay** into canonical v0.2;
- Exact Gauge is one registered/versioned `PassArtifact`, not a hard-coded truth exception;
- telemetry/route attribution must be bound to the exact Scientific-Master identity rather than inferred from timings/read counts;
- telemetry contradiction/mismatch fails closed to `UNKNOWN_FAIL_CLOSED`;
- `candidate_applied=false`;
- `creates_new_evidence=false`;
- `scientific_writeback_allowed=false`.

Performance work follows one permanent rule: **same scientific meaning, less work**.

## 6. Reconstruction result that must NOT be rediscovered as a missing feature

Anchor-Constrained Local Reconstruction v0.1 is **NOT PROMOTED**. Real tele hold-out used 21,760 CFA holdouts. On the 19,560 directly comparable samples, the affine solver was worse than baseline in aggregate MAE/RMSE/bias; uncertainty was too optimistic. It did provide better coverage and a small channel-2 benefit, but that is insufficient for promotion.

Safety remained closed: target value not used by the solver, measured anchors unmodified, no Scientific-Master solver writeback, candidate not applied.

Future reconstruction direction is deterministic local model selection from structural support, direction, CFA phase and uncertainty. Affine is only one optional model; `no suitable model` is a valid result.

N2 sparse/local-spatial remains reconstruction/performance support only and never becomes `MEASURED` by reuse or speed.

## 7. Open-World corridor: what is now connected

The historical project contained two highly developed worlds:

A. `real RAW -> sealing -> calibration -> Scientific Master -> authority/uncertainty`

B. `Open Scene -> geometry/world representation -> restoration/light transport -> appearance/rendering`

The current branch has connected the corridor far enough that the modern route is conceptually and in runtime wiring:

`Scientific Master -> v0.4 Deep Scene Contribution / geometry bridge -> v0.5 Deep Scene Binding / ObservationGeometryEnvelope -> v0.6 Light Transport / World Observation Contract -> existing Room Capsule host -> v0.7 Appearance Resolve`.

Important commit landmarks in this lineage include:

- v0.4 geometry bridge: `f1004b6...`
- v0.5 geometry envelope: `e27d478...`
- v0.6 world observation contract: `33fad60...`
- v0.7 appearance layer: `205754e...`
- duplicate user-exposure correction: `0053e25...`

These abbreviated landmarks are navigation aids, not a substitute for resolving the live branch.

### Room Capsule rule

Room Capsule architecture was **not missing**. It already existed in the flexible Open-World host. The missing runtime connection was the issue; the corridor is now wired through it.

Without admitted geometry/material/illumination evidence, Room Capsule must remain an **exact-preserving bypass**. It may not change pixels merely because the host exists. More useful relighting requires an admitted world-evidence package first.

## 8. Nine-block architecture status

### Blocks 1–6 — largely present

The source/seal/calibration/Scientific-Master/authority scientific core is architecturally mature, though universal source admission and broader real-device validation remain incomplete.

### Block 7 — Free Raster Projection: PARTIAL

The resolver mathematics and free-evaluation idea exist, but the product path does not yet prove a complete first-class arbitrary full-resolution projection flow (for example 1080p/4K/8K/arbitrary raster) from the same scientific state with all authority/provenance preserved.

### Block 8 — Free World Observation Graph: LARGEST INCOMPLETE SCIENTIFIC BLOCK

Foundations exist: world-field separation, pair geometry, observation-manifold and related experiments. What is still missing is a mature active graph that binds multiple sealed observations through proven registration/transforms, radiometric relations and uncertainty, then allows controlled joint evaluation without inventing shared truth.

### Block 9 — View / Appearance: SINGLE-OBSERVATION ROUTE CONNECTED

v0.7 Appearance is in the runtime corridor and duplicate user exposure has been corrected. Richer geometry/light/material-driven appearance remains gated by admitted world evidence. Dynamic display-target/HDR behavior is still product work.

## 9. Missing / incomplete components found by historical audit

### 9.1 Universal source admission

DNG is the best-proven admitted route. Professional non-DNG RAW families have recognition/research foundations, but a broad container-independent end-to-end chain from arbitrary professional RAW container -> sensor payload -> evidence contract -> Scientific Master is not yet proven.

### 9.2 Camera -> true source validation

Camera2 `RAW_SENSOR -> rawsensor/seal -> DNG/intake` lineage, calibration binding, SHA/provenance and promotion firewalls are substantially repaired. They still require real-device validation across modes/devices/vendors; one HONOR route is not universal proof.

### 9.3 Canonical Source Capability Envelope

Many individual capability facts exist (CFA, packing, black/white limits, optics, metadata, NPS, calibration). A single canonical source-agnostic capability layer that consistently exposes only what can be proven and governs downstream authority is still incomplete.

### 9.4 First-class D.RAWnegative container

TruthNegative Continuous/native-container research exists, including high-resolution/native experiments. What remains unproven is a single first-class standalone D.RAWnegative with complete write -> close -> read -> re-import -> verify -> export round-trip and preserved scientific identity/provenance.

### 9.5 Free Raster productization

Free continuous evaluation is not the same as pixel enlargement. The unresolved product task is to evaluate the same scientific/world state on a requested raster/footprint while preserving state/uncertainty/provenance and without pretending added samples are new measurements.

### 9.6 Active multi-observation graph

This is the main next scientific frontier after device validation: multiple real sealed observations -> proven inter-observation relations -> world representation -> controlled free evaluation.

### 9.7 True 3D geometry

Current safe route may use `ImagePlaneBound`/limited geometry authority. A metrically meaningful 3D geometry state has not yet been broadly admitted from real evidence. Estimated depth/normals must never silently become `MEASURED`.

### 9.8 Material / illumination authority

BRDF/PBR/material/illumination/light-transport structures are architectural/research tools until supported by admitted evidence. They may influence Appearance only under explicit authority and may not rewrite Scientific Master.

### 9.9 Restoration / Render-Edit consolidation

Censor-aware restoration, `.trr`, and DNG/EXR/TIFF projection research exist in older lines. They need consolidation into the modern D.RAW flow with explicit provenance/uncertainty. Restoration hypotheses remain hypotheses and never contaminate measured source truth.

### 9.10 User-visible authority

Internal authority/telemetry is much richer than the current user presentation. A coherent UI should eventually show what is MEASURED, RECONSTRUCTED, CENSORED, UNKNOWN, INFERRED/estimated or APPEARANCE, why, and with what uncertainty.

### 9.11 Scale/performance proof

N2 reuse, direct SHA, Pixel-Triplet work and Exact Gauge reduce redundant work, but true full-resolution free-raster and multi-observation workloads still need real-device memory/runtime proof.

## 10. Current CI truth at runtime baseline `2766aca...`

Do **not** call the baseline fully green.

Known check state on 2026-10-04:

- exact-parity checks are succeeding;
- **Research Integrity Guard / integrity** is failing;
- **Lifecycle Contract Test / lifecycle-contract** is failing;
- many other scientific/Android checks are green.

The two red governance/lifecycle checks must be diagnosed from their exact current output before promotion. Do not guess their cause and do not treat a device pass as permission to ignore them.

## 11. Exact next-chat starting point: TEST, not new architecture

Read `docs/handoff/DRAW_44489_TEST_START_2026-10-04.md` and begin there.

The next chat should first establish a reproducible test baseline:

1. resolve live PR #130 HEAD and record it;
2. read current CI and classify every red/skipped required gate;
3. identify or build the exact candidate APK, record SHA-256/package/version/signing identity;
4. install/update on the real device without silently changing app identity;
5. capture one real Camera2 RAW_SENSOR observation and preserve raw/seal/DNG/manifest lineage;
6. run it through intake -> Scientific Master -> v0.4 -> v0.5 -> v0.6 -> Room Capsule -> v0.7 Appearance;
7. verify provenance, calibration binding, Scientific-Master identity, authority monotonicity, duplicate-exposure absence and exact-preserving Room-Capsule bypass where world evidence is absent;
8. save an evidence bundle before any new research or optimization.

### Stop immediately if any of these occurs

- installed APK SHA/package/version/signing identity is not the intended candidate;
- source lineage/hash or calibration binding disagrees;
- crash/OOM or non-deterministic scientific output;
- user exposure is applied twice;
- Room Capsule changes scientific/presentation values without admitted world evidence;
- inferred/estimated geometry becomes `MEASURED`;
- authority increases without new evidence;
- non-DNG admission silently guesses sensor truth;
- Scientific Master, sealed CFA or measured anchors are mutated by a candidate/performance/appearance path.

No promotion until CI + real-device evidence support it.

## 12. What comes after a clean device test

Do not immediately add another local reconstruction heuristic. The highest-value scientific continuation is:

`multiple real sealed observations -> proven registration/gauge/uncertainty relations -> active Free World Observation Graph -> free raster/world evaluation`.

In parallel, horizontal engineering tracks are:

- universal source admission;
- first-class D.RAWnegative round-trip;
- user-visible authority/uncertainty;
- dynamic appearance/HDR targets;
- scale/performance proof for full-resolution and multi-observation workloads.

## 13. Recovery pointers

Current recovery/test layer:

- `START_HERE_NEW_CHAT.md`
- `docs/handoff/DRAW_44489_KNOWLEDGE_CAPSULE.md`
- `docs/handoff/DRAW_44489_TEST_START_2026-10-04.md`
- `state/DRAW_PROJECT_STATE_2026-10-04.json`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-04.md`

Historical but still important provenance:

- `docs/handoff/DRAW_44489_RECOVERY_2026-10-03.md`
- `state/DRAW_PROJECT_STATE_2026-10-03.json`
- `docs/research/scientific-master-exact-gauge-retained-artifact-v0.3/README.md`
- older handoffs/state/research files referenced by the document-status index.

When the user says **44489**, restore this capsule plus the live repository/PR state first. Do not ask the user to manually reconstruct old chats when the repository recovery layer is sufficient.

## 14. Continuous maintenance rule

44489 is a living recovery artifact. Refresh it whenever a scientific conclusion changes, a candidate is promoted/rejected, real-device evidence arrives, architecture/authority changes, Android identity/build state changes, or the exact next task moves.

Maintenance discipline:

1. record scientific conclusions, not only code changes;
2. preserve falsified/negative experiments because they constrain future work;
3. distinguish measured fact, derived fact, estimate, candidate and appearance;
4. preserve provenance for important measurements;
5. keep the capsule compact enough for cold-start recovery and move deep detail to dedicated state/research documents;
6. never rewrite history to make a newer approach look inevitable;
7. study authoritative photography, optics, radiometry, colour science, signal processing, calibration, 3D/photogrammetry, rendering/PBR and human-vision literature when those domains affect semantics, then translate that knowledge into explicit contracts, provenance, uncertainty and tests rather than assumptions.
