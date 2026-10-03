# D.RAW 44489 — compact knowledge capsule

Purpose: smallest practical project-state document that preserves the binding scientific/architectural knowledge needed for a new chat to continue safely. Read this first; use the larger recovery handoff only when detail/provenance is needed.

Project: **D.RAW** (`TruthRaw` is historical/repository naming)  
Repo: `1DiDaDo1/TruthRaw`  
Branch: `fix/research-fresh-rerun-v01`  
Recovery code: **44489**  
Live head immediately before this document was added: `fb0bff795ee991f77c691b0a707a7d91e75aa54d` (`Upgrade machine state to recovery code 44489`). This document-only commit moves the branch head forward; always fetch live head before mutation.

## 1. Permanent scientific law

D.RAW is deterministic, provenance-bound and AI/ML-free in its scientific path.

- **Seal the evidence, not the thinking.**
- **MEASURED != RECONSTRUCTED != APPEARANCE.**
- Richer representation may never imply richer evidence.
- Direct-CFA / RAW_SENSOR evidence is immutable/sealed.
- One physical frame remains one physical frame; virtual/derived views do not create captures.
- Scientific Master is scene-linear scientific state, separate from view/export/appearance; signed values and >1 are valid.
- UNKNOWN is valid and must not silently become zero/certainty/estimate.
- More precision, more resolution, reconstruction, speed or registration never creates authority.
- APK/GCam/computational RAW/presentation may not define source evidence, calibration or Scientific-Master truth.

Canonical chain:
`sealed Observation -> Observation Contract -> Source Capability Envelope -> measurement/calibration/reconstruction -> Scientific Master -> Dynamic Authority+uncertainty -> Observation-bound TruthNegative -> Free World Observation Graph -> view/appearance/projection`

## 2. Evidence / authority

Authority classes include:
`MEASURED`, `CALIBRATED_ESTIMATE`, `RECONSTRUCTED`, `CENSORED`, `UNKNOWN`, `COUNTERFACTUAL`, `APPEARANCE`.

Authority is local and provenance-bound. Registration, interpolation or reconstruction never upgrades to `MEASURED` automatically.

Clipping may be `CENSORED`: a bound is known while the exact latent signal remains unknown.

Unknown RGB covariance terms stay UNKNOWN/NaN, never convenience-zero. Full covariance requires all needed terms known/consistent; transport is conceptually `J Sigma J^T`.

Optics (NPS/signal PSD/MTF/SFR/PSF), geometry, colour and world-space claims require explicit proven binding. World-space information may not rewrite Scientific Master merely because it exists.

## 3. Observation / gauge law

**One Free World. Many sealed observations. One evidence law.**

Lens/sensor/CFA/readout/capture route belong to an Observation. TruthNegative is Observation-bound; multiple observations meet only above that boundary.

`source capability != proven sample domain`.

Proven Camera-5 example: TELE provenance, `4080x3072`, RAW10/BGGR, WhiteLevel `1023`, BlackLevel `64`, source-local gauge. A `16320x12288`/200MP capability is not proof that a captured frame measured 200MP.

TruthRange family: `T = log2(L/L0)`. Shared coordinate notation is not shared radiance. Cross-observation radiometric fusion requires an admitted common-gauge relation and otherwise fails closed.

## 4. Reconstruction research

Anchor-Constrained Local Reconstruction v0.1 is **NOT PROMOTED**. Real tele hold-out: 21,760 CFA holdouts; affine solver covered more points but was worse on directly comparable aggregate MAE/RMSE/bias and had over-optimistic uncertainty. Safety stayed closed (`target_value_used_by_solver=false`, `measured_anchors_modified=false`, `solver_applied_to_scientific_master=false`, `candidate_applied=false`, `scientific_writeback_allowed=false`). Future direction: deterministic local model selection from structural support/direction/CFA phase/uncertainty; affine is only one optional model; `no suitable model` is valid.

N2 sparse/local-spatial remains reconstruction/performance support only, never `MEASURED`. Historical v0.2.2 checkpoint: `808485b6676f23ac1a846e1eba1c625067f657bc`, 27/27 green.

## 5. Performance law

Permanent rule: **same scientific meaning, less work**.

Known validated examples: v0.2.9 direct SHA transport (~914.96 MB/RAW direct path); Pixel-Triplet Authority Encoder (`12,533,760` triplets, `37,601,280` canonical records, fallback 0, ~5.52% faster authority route); PR #125 fixed-topology exact-parity specialization; PR #126/#127 measured Scientific-Master binder at 6,144 Stage-2/RAW tile reads vs 3,072 reconstruction calls because canonical v0.2 has two tile passes.

Never trade scientific semantics for speed or invent unmeasured performance claims.

## 6. Exact Gauge Retained Artifact v0.3

Current candidate removes the second Stage-2 reread **without changing semantics** by retaining exact Float32 bit patterns of eligible gauge candidates during pass 1 and resolving the exact low-16 median from that bounded artifact.

Parity must be bit-exact for Scientific Master/hash, `gaugeMedian` bits, samples/states/counts, authority, reconstruction behavior, source/measured-anchor immutability, firewall state and deterministic repeats. Only truthful resource/timing diagnostics may differ.

Host candidate is green for configure, warnings-as-errors build, ASan/UBSan, exact parity oracle and forced budget fallback. Candidate remains **NOT PROMOTED**. Old pre-atomic retained-gauge core was removed and must not be resurrected.

Firewall remains:
`candidate_applied=false`, `creates_new_evidence=false`, `scientific_writeback_allowed=false`.

## 7. Atomic + PassArtifact architecture

Atomic is a Scientific-Master transaction/safety boundary, **not** a closed cable taxonomy.

- Admission/budget/topology failure before semantic start may use complete canonical v0.2 fallback.
- Once candidate semantic processing/tile observation starts: **no replay**.
- Later candidate failures propagate hard; never silently redo canonical after partial semantic execution.

Binding architectural law:
**The atomic core does not decide which cables D.RAW may have; it defines how a cable behaves safely once it touches Scientific Master.**

Exact Gauge must be only one registered/versioned `PassArtifact`, never a hard-coded architectural exception.

Generic registry/descriptor may define identity/version, applicability/admission, dispatch, provenance/diagnostics, atomic/no-replay lifecycle and authority classification. Artifact-specific payload/state remains open. Future optics/geometry/temporal/calibration/restoration/unknown artifacts need not use Exact Gauge's data model. **Registration never grants authority.**

## 8. Android/runtime state

Real runtime app: `suite_android` (not historical `android/app`). Target SDK 37. The current Android lineage predates completed Exact Gauge v0.3 wiring; do not call an existing APK a v0.3 device build unless native integration/tests/build actually occurred.

Known integration target: `suite_android/app/src/main/cpp/CMakeLists.txt` plus Foundation/observation JNI + Java/Kotlin path under `suite_android/app/src/main/...`.

Do not silently substitute the general preview flow. First device integration should be selectable/diagnostic Foundation/observation execution with canonical v0.2 fallback.

## 9. Exact next work

1. Fetch live branch head; never force-reset to a remembered SHA.
2. Search existing native + Java/Kotlin tree for registry/descriptor/artifact/pass_artifact/candidate/admission dispatch.
3. Reuse an existing generic abstraction if present; do not create a parallel registry.
4. If none exists, add the smallest generic PassArtifact interface and register Exact Gauge v0.3 as one concrete type.
5. Add v0.3 admission + atomic-core sources/dependencies to `suite_android` CMake.
6. Wire Foundation/observation callsite through generic artifact dispatch, not preview.
7. Emit diagnostics: artifact id/version, candidate/canonical reads, retained bytes, budget fallback, peak bytes, admission/fallback state, semantic-start/no-replay state, authority/candidate status.
8. Preserve all firewalls and canonical v0.2 fallback.
9. Run exact parity + forced-budget fallback + all regressions.
10. Only when green, build a new APK and then real-device measure.

## 10. Recovery pointers

For full detail/provenance read:
- `docs/handoff/DRAW_44489_RECOVERY_2026-10-03.md`
- `state/DRAW_PROJECT_STATE_2026-10-03.json`
- `docs/research/scientific-master-exact-gauge-retained-artifact-v0.3/README.md`
- `START_HERE_NEW_CHAT.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-03.md`

When the user says **44489**, restore this capsule + live repository state first, then continue directly. Do not ask the user to reconstruct prior chats when repository state is sufficient.
