# D.RAW / TruthRaw

> **CURRENT PROJECT STATE — 2026-10-13 — continuation code `44489`**
>
> Canonical project/product name: **D.RAW**  
> Historical/repository name: `TruthRaw`  
> Repository: `1DiDaDo1/TruthRaw`  
> Active candidate branch: `audit/source-capability-raw-to-linear-contract-v01`  
> Active candidate PR: **#136 — draft / open / unmerged**  
> Integrated scientific/runtime anchor: `96be8a23c0d43f5dec304bdeb635824868cc80a7`

D.RAW is deterministic, provenance-bound and non-destructive:

> **Seal the evidence, not the thinking.**  
> **MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE.**  
> **Representation may become richer than the source; knowledge claims may not exceed the evidence.**  
> **One Free World. Many sealed observations. One evidence law.**

## Start here — 44489

For the current continuation, read:

1. `START_HERE_NEW_CHAT.md`
2. `docs/handoff/DRAW_44489_MASTER_CAPSULE_CURRENT.md`
3. `docs/handoff/DRAW_44489_NEXT_CHAT_2026-10-13.md`
4. resolve the live head of `audit/source-capability-raw-to-linear-contract-v01`
5. `state/DRAW_PROJECT_STATE_2026-10-13.json`
6. `docs/DOCUMENT_STATUS_INDEX_2026-10-13.md`
7. `docs/handoff/DRAW_44489_RAW_TO_LINEAR_CONTRACT_AUDIT_V0_1_2026-10-12.md`
8. `docs/handoff/DRAW_44489_INTERDISCIPLINARY_OBSERVATION_AUDIT_2026-10-10.md`

Do not treat older 44488 / PR #131 material as current authority. Historical files remain provenance and are discoverable below.

## Current authority split

Integrated scientific/runtime authority remains:

`96be8a23c0d43f5dec304bdeb635824868cc80a7`

The current unintegrated code candidate checkpoint for PR #136 is:

`e6bdbce11c02ccac0e3edf31f39f0eb869105d51`

At that exact code checkpoint the dedicated `Source Capability Raw-to-Linear Contract v0.1 Integrity` workflow passed. Newer documentation/governance commits do not become scientific authority merely by being newer.

PR #136 remains draft/open/unmerged unless explicitly changed later.

## Current single-cable Source Capability work

The existing runtime envelope is reused. The candidate represents the metadata/specification ordering:

`Linearization -> Black Subtraction -> Rescaling / Normalization -> Clipping`

The cable remains single:

- Effective Black/Normalization is computed once in the existing envelope;
- LinearizationTable is computed once in the existing envelope;
- the Raw-to-Linear compositor is called once in the existing envelope;
- the DNG post-normalization clipping-rule is derived once inside that compositor from the already supplied normalization report;
- no second parser, source read, hash, Observation Evidence Ledger, Raw-IFD detector or pixel route is introduced.

The clipping-rule contract is specification/metadata authority only. It is not observed sample censoring, physical full-well/sensor saturation, recovered scene radiance, TruthRange authority or Appearance highlight roll-off.

`full_raw_to_linear_mapping_validated=false` remains mandatory until actual sample-domain execution is separately traced and proven.

## Immediate technical frontier

Perform a read-only audit of the **existing** RAW sample execution path before implementing any new pixel processing. Determine whether LinearizationTable application, effective-black subtraction, normalization and upper clipping already exist. Reuse existing execution where present; do not create a second raw-to-linear implementation.

No current metadata-contract result authorizes Scientific Master writeback or sample mutation.

## Permanent scientific boundaries

- Direct-CFA/source evidence is immutable and sealed.
- Source capability is not proof of measured sample-domain execution.
- Scientific Master remains separate from presentation/export/Appearance.
- `UNKNOWN`, `CENSORED`, `RECONSTRUCTED`, `MEASURED`, `CALIBRATED_ESTIMATE` and `APPEARANCE` remain distinct.
- `VALIDATED != ADMITTED != USED`.
- WhiteLevel metadata is not automatically an independently calibrated physical full-well or scene-radiance boundary.
- Unknown covariance is not zero covariance.
- Cross-observation radiometric fusion requires an admitted common-gauge relation.
- Performance artifacts cannot create evidence or authority.
- Camera/lens/vendor/container identity may route parsing but cannot select scientific truth.
- SOURCE/SENSOR, WORLD/SCENE and VIEW/OUTPUT remain distinct.
- No diagnostic, UI, Appearance or output route may create MEASURED evidence or authorize scientific writeback.
- AI/ML/neural/generative runtime is not admitted as scientific inference authority.

## Historical governance / provenance pointers

The following dated material is retained for deterministic discoverability. These paths are **historical provenance**, not a replacement for the 2026-10-13 reading order:

- `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
- `docs/PROJECT_HISTORY_AND_CHANGES_2026-09-16.md`
- `docs/handoff/TRUTHRAW_CONSOLIDATED_HANDOFF_2026-09-16.md`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-19.md`
- `state/CURRENT_PROJECT_STATE_2026-09-19.json`
- `docs/DOCUMENT_STATUS_INDEX_2026-09-19_MULTIVENDOR.md`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-20.md`
- `state/CURRENT_PROJECT_STATE_2026-09-20.json`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-21.md`
- `state/CURRENT_PROJECT_STATE_2026-09-21.json`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-24.md`
- `state/CURRENT_PROJECT_STATE_2026-09-24.json`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-09-25.md`
- `state/CURRENT_PROJECT_STATE_2026-09-25.json`
- `docs/DRAW_MAIN_PROJECT_STATE_2026-09-25.md`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-09-27.md`
- `state/CURRENT_PROJECT_STATE_2026-09-27.json`
- `docs/DRAW_KNOWLEDGE_GROWTH_INTEGRATION_2026-09-27.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-09-27.md`
- `docs/research/free-world-observation-graph-v0.1/README.md`
- `docs/DRAW_CORE_VISION_REALIGNMENT_2026-09-28.md`
- `state/CURRENT_PROJECT_STATE_2026-09-28.json`
- `state/CURRENT_PROJECT_STATE_2026-10-01.json`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-01.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-01.md`
- `docs/research/measured-field-support-coordinate-bridge-v0.1/README.md`
- `docs/research/optical-field-topography-v0.1/README.md`
- `state/CURRENT_PROJECT_STATE_2026-10-02.json`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-02.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-02.md`
- `docs/handoff/DRAW_44489_RECOVERY_2026-10-03.md`
- `state/DRAW_PROJECT_STATE_2026-10-04.json`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-04.md`
- `docs/handoff/DRAW_44489_REAL_DEVICE_VALIDATED_2026-10-05.md`
- `state/DRAW_PROJECT_STATE_2026-10-05.json`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-05.md`
- `docs/handoff/START_HERE_NEW_CHAT_LEGACY_2026-10-06.md`

Older dated snapshots remain provenance and must not be rewritten to pretend they knew later results.
