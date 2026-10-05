# D.RAW / TruthRaw — CURRENT BOOTSTRAP — 2026-10-05 — CODE 44489

Continuation code: **44489**

A new chat receiving only **44489** must restore the project from the repository before changing code. The project is in a **test-first handoff state**: recover the current authority, resolve the live PR head, then continue real-device evidence collection rather than redesigning architecture.

## Read in this order

1. Fetch live PR **#130** and record its current `head_sha`.
2. Read `docs/handoff/DRAW_44489_T5_RUNTIME_AUDIT_READY_2026-10-05.md`.
3. Read `docs/handoff/DRAW_44489_KNOWLEDGE_CAPSULE.md`.
4. Read `state/DRAW_PROJECT_STATE_2026-10-04.json`.
5. Read `docs/handoff/DRAW_44489_TEST_START_2026-10-04.md`.
6. Read `docs/DOCUMENT_STATUS_INDEX_2026-10-04.md` when older/normative provenance is needed.
7. Inspect current source and CI for the exact live head before mutation.

Historical recovery provenance remains in `docs/handoff/DRAW_44489_RECOVERY_2026-10-03.md`, but it is no longer the first operational continuation document.

## Runtime candidate versus documentation head

The exact T5 runtime/source candidate used to build the current physical-test APK is:

`6a557cda2b8f10db7f4c81dde50b5dc8c77ea210`

Later commits beginning with `9f3a38c...` are handoff/documentation updates unless source inspection proves otherwise. Do not rebuild scientific history merely because the live branch head moved after the validated APK build.

Validated APK for the next T5 device round:

- workflow run: `37232652035`
- artifact ID: `11314697580`
- artifact: `DRAW-free-world-research-debug-arm64-stable-signed`
- APK size: `8,588,087 bytes`
- APK SHA-256: `31bba97ddb220284059eeb0c0402ba87b1b840935ccf2f39e848a865cd1b03aa`

The exact-head APK workflow completed successfully, including stable signing verification and artifact upload. This proves buildability, **not** physical runtime T5 success or merge/promotion readiness.

## Live branch rule

Candidate branch: `fix/android-exact-gauge-pass-artifact-v03`  
PR: #130  
Base: `fix/research-fresh-rerun-v01`

PR #130 remains open/draft and is currently reported `mergeable=false`. Do not merge or resolve base conflicts as part of the physical T5 evidence round.

Earlier runtime/scientific baseline before the 2026-10-04 documentation refresh:

`2766aca5f8acf0f441c7b4f7fb6d892de9ae5778`

Documentation-only commits intentionally move HEAD beyond runtime baselines. **Never force-reset the branch to a remembered SHA.** Resolve the live PR head first and distinguish runtime/source commits from documentation-only commits.

## Permanent scientific laws

- **Seal the evidence, not the thinking.**
- **MEASURED != RECONSTRUCTED != APPEARANCE.**
- **Representation may become richer than the source; knowledge claims may not exceed evidence.**
- **One Free World. Many sealed observations. One evidence law.**
- Direct-CFA / RAW_SENSOR evidence is immutable and sealed.
- Source capability is not proof of the captured sample domain.
- Scientific Master is scene-linear and pre-appearance.
- `UNKNOWN` is valid and may not be converted to zero/certainty for convenience.
- Unknown covariance is not zero.
- Registration/reconstruction/precision/resolution/performance do not create authority.
- Cross-observation radiometric fusion requires an admitted common-gauge relation.
- TruthNegative remains Observation-bound; observations meet only above that boundary.
- Inferred depth/normal/geometry/material/illumination may not silently become `MEASURED`.
- World/Appearance layers may not write back into sealed evidence or Scientific Master without explicit admitted authority.
- No AI/ML/neural/generative runtime is scientific evidence in D.RAW.

## Current Android identity

Actual runtime app: `suite_android`.

Current source configuration for the T5 candidate lineage:

- application ID / namespace: **`com.truthraw.adaptiveui`**
- versionCode: **`26100127`**
- versionName: **`0.54-v0.84.2-open-world-authority-corridor-v01`**
- minSdk 31
- targetSdk 37
- ARM64

Do not use stale package/version information from older snapshots. Before installation, bind the APK to the intended source state using SHA-256, package/version and signer identity.

## Current connected corridor and T5 audit

The project connects the single-observation scientific path through:

`Scientific Master -> v0.4 Deep Scene Contribution -> v0.5 Deep Scene Binding -> v0.6 Light Transport -> existing Room Capsule -> v0.7 Appearance`.

T5 now audits that **existing** corridor read-only. It does not create a second scientific cable. The runtime candidate records source/Scientific-Master SHA binding, per-stage counts and lineage, authority, firewalls, frame/evidence counts and v0.7 exposure application count.

Expected current result without an admitted world-evidence package:

- v0.6: `READY` only when parent/authority/firewall checks pass;
- Room Capsule: `EXACT_PRESERVING_BYPASS`;
- v0.7: downstream Appearance only;
- `exposureApplicationCount == 1`;
- `candidate_applied=false`;
- no scientific writeback/new evidence.

Missing runtime evidence remains `UNKNOWN`; a known failing v0.6 construction may be `BLOCKED`.

Room Capsule is **not** a missing architecture. Without admitted geometry/material/illumination evidence, it must remain exact-preserving bypass.

## Important things that remain incomplete

- physical real-device proof of the new T5 telemetry and corridor result;
- universal end-to-end professional non-DNG RAW admission;
- one canonical source-agnostic Source Capability Envelope governing downstream authority;
- first-class standalone D.RAWnegative write/read/re-import/export round-trip;
- productized arbitrary/full-resolution Free Raster Projection;
- active evidence-bound multi-observation Free World Observation Graph;
- broadly admitted metric 3D geometry;
- admitted material/illumination packages for non-bypass Room Capsule behavior;
- consolidation of restoration/render-edit research into the modern provenance-bound flow;
- coherent user-visible authority/uncertainty UI;
- physical performance/memory proof for full-resolution and multi-observation workloads;
- dynamic HDR/display-target product behavior.

The **largest incomplete scientific block** after clean single-observation device validation remains the active multi-observation Free World Observation Graph.

## Things that are deliberately NOT missing

### Anchor-Constrained Local Reconstruction

Do not promote it merely because the research code exists. Real hold-out evidence showed worse aggregate performance than baseline on directly comparable samples and over-optimistic uncertainty. It is intentionally **NOT PROMOTED**.

### Room Capsule architecture

Do not build a second Room Capsule/cable architecture. The existing flexible host is already the one to use. What is missing for non-bypass behavior is admitted world evidence.

## CI / promotion warning

For exact runtime head `6a557cda...`, the head-bound Actions query contains the APK workflow and that workflow is green. Do **not** reinterpret this as proof that all historical PR/governance/lifecycle gates are green.

PR #130 remains draft and not mergeable at this checkpoint. Earlier recovery state also recorded red `integrity` and `lifecycle-contract` gates. Those must be re-evaluated separately before promotion. A device pass cannot override repository governance failures.

## Exact next action

**Continue testing, not architecture work.**

Open:

`docs/handoff/DRAW_44489_TEST_START_2026-10-04.md`

The repository/APK freeze portion is now stronger:

- runtime source commit known: `6a557cda...`;
- exact signed APK known;
- APK SHA-256 known;
- exact APK workflow green.

Continue with the physical round:

`T1 install/update exact APK -> T2 launch/logging -> T3 one real RAW_SENSOR/DNG observation -> T4 Scientific Master verification -> T5 v0.4/v0.5/v0.6/Room Capsule/v0.7 runtime audit -> T6 evidence bundle`.

Stop on identity/provenance/calibration mismatch, crash/OOM, nondeterministic Scientific Master, sealed-source mutation, duplicate exposure, Room Capsule non-bypass without admitted evidence, inferred geometry becoming MEASURED, missing/contradictory T5 stage lineage, or any authority increase without evidence.

## After a clean test

Do not jump directly into another reconstruction heuristic. The next major scientific frontier is:

`multiple real sealed observations -> proven inter-observation registration/gauge/uncertainty -> active Free World Observation Graph -> controlled free raster/world evaluation`.

Parallel tracks remain universal source admission, first-class D.RAWnegative, Free Raster productization, authority UI, dynamic Appearance/HDR and scale/performance proof.

A new chat should not ask the user to manually reconstruct older chats when this recovery layer and live repository state are sufficient.
