# START HERE — TruthRaw new-chat handoff

Current handoff snapshot: **2026-09-11**

This file is the primary bootstrap for a new ChatGPT/engineering session. Do not infer project status from an older dated state file or from one research README in isolation.

## Mandatory reading order

1. `state/CURRENT_CANONICAL_STATE_2026-09-11.json`
2. `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-11.md`
3. `docs/CURRENT_MODULE_STATUS_2026-09-11.md`
4. `docs/CURRENT_CLAIM_MAP_2026-09-11.md`
5. `docs/CI_EVIDENCE_INDEX_2026-09-11.md`
6. `docs/DOCUMENT_STATUS_INDEX_2026-09-11.md`
7. `docs/DOCUMENTATION_SYNC_POLICY_2026-09-11.md`
8. `docs/CHAT_HANDOFF_2026-09-11.md`
9. `docs/PROJECT_STATE_AUDIT_2026-09-11.md`
10. `README.md`

Historical 2026-09-10 and earlier state/audit files are evidence of prior state, not current bootstrap documents.

## Repository coordinates

- Repository: `1DiDaDo1/TruthRaw`
- Promoted `main`: `514f2f4bde6aba5a6709e176c03b22c3b9aea912`
- Current integrated research basis: `2fdf05ca1bbbc59cd8867df0cae117d1eec92d51`
- Integrated research branch: `research/android-source-bound-color-preview-v0.1-2026-09-11`
- Documentation/handoff branch: `docs/project-handoff-2026-09-11`

The research basis is **not** equivalent to promotion into `main`. All PRs #9–#20 audited on 2026-09-11 remain open/draft unless a fresh GitHub check proves otherwise.

## Non-negotiable canon

- **Measured where measured. Reconstructed where necessary. Never invented.**
- sealed/original Direct-CFA evidence is immutable;
- one ordinary single-frame lineage means one physical frame and one independent evidence source;
- measured / reconstructed / counterfactual / appearance / projection are separate authorities;
- no virtual observation can add evidence, photons or SNR;
- no appearance operation may mutate scientific evidence;
- APK/GCam/computational RAW cannot determine TruthRaw evidence/calibration/topology/color/noise authority;
- resource differences may affect execution only, never scientific authority;
- zero-line is a scene-reference gauge (`T=log2(L/L0)`), not sensor black or display black;
- canonical v4.7i scientific reconstruction remains the measured-preserving single-frame baseline; v4.7j/v4.7k are appearance/detail layers only.

## Mandatory documentation maintenance

Every substantive project change must be accompanied by a documentation review in the same work cycle. Update the affected module README/state overlay, and update the global current-state/handoff files whenever branch heads, proof boundaries, CI evidence, scientific authority, architecture, or recommended next work changes.

Do not rewrite sealed/historical/versioned README material just to make it look current. If exact historical bytes must remain stable, add/update a current-state overlay or the global indexes instead. The full rule is `docs/DOCUMENTATION_SYNC_POLICY_2026-09-11.md`.

A code implementation may temporarily precede its documentation overlay during validation, but it must not be declared the new global handoff until both implementation proof and documentation synchronization are complete.

## Current source routing

```text
source
  |
  +-- native-certified strict DNG subset --> Main House
  |
  +-- external professional RAW --> Gatehouse
                                  --> decode/audit/resource isolation
                                  --> integrity-verified persisted handoff
                                  --> decoder/context detached
                                  --> Main House
  |
  +-- unknown/unsupported/unverified --> fail closed or ResearchOnly
```

The Gatehouse can be started and destroyed independently of the Main House. It is a resource/failure/provenance boundary, not a second truth pipeline.

## Current Android preview route

The newest integrated Android research route is:

`ParcelFileDescriptor -> exact source SHA-256 -> source-bound DNG color metadata -> phase-1 source binding -> TileNativeDngSource -> v4.7i streaming reconstruction -> bounded sRGB appearance preview -> Bitmap/JPEG`.

Its visible output is **SOURCE_BOUND_APPEARANCE_PREVIEW**, not a finalized Scientific Preview or Scientific Master.

Expected phase-1 authority:

- `mainHouseComputeAllowed=true`
- `sourceBoundAppearanceReleaseAllowed=true`
- `scientificPreviewReleaseAllowed=false`
- `scientificClaimAllowed=false`
- `physicalFrameCount=1`
- `independentEvidenceCount=1`

The gray CFA `SOURCE_PROXY` remains historical/diagnostic; it must never silently become the scientific color path.

## Professional RAW reality check

Do not say “TruthRaw supports all RAWs.” Current truth is narrower:

- routing/classification architecture exists;
- decoder adapter contract exists;
- LibRaw metadata compatibility probe exists and is validated with LibRaw 0.21.2;
- borrowed-fd LibRaw datastream exists;
- Gatehouse + persisted decoded-measurement handoff + tile-source bridge exist as research branches;
- there is **no certified production LibRaw pixel `unpack()` adapter/corpus proving real vendor formats yet**.

Container extension, camera brand and successful library open are not evidence.

## What is proven now

See `docs/CI_EVIDENCE_INDEX_2026-09-11.md` for exact SHAs/runs. High-level validated research includes Room ABI v0.2, Android host validation harness, Android UI/Ingress, bounded gray TileNative preview, Professional RAW Ingress/Decoder Adapter/LibRaw metadata probe, Gatehouse Runtime, decoded handoff, decoded tile-source bridge, preview representation/reconstructed color, source binding, DNG color binding, and Android source-bound color-preview build integration.

## What is not proven now

- physical target-device execution of the latest color preview;
- physical-device memory/thermal/frame-time behavior;
- independent camera/lens calibration;
- final Scientific Master digest / Backplane phase 2;
- production professional vendor RAW pixel decode certification;
- `decoded-measurement-main-house-e2e-v0.1` CI on its current head;
- multi-capture fusion/HDR evidence model;
- blanket RAW-format support.

## Recommended next work

1. Run the source-bound color-preview APK on the physical Honor target with a real MotionCam DNG and capture objective device evidence.
2. Define deterministic Scientific Master serialization/digest semantics, then implement Technical Backplane phase-2 finalization without dummy hashes.
3. Finish a real LibRaw Decode Adapter in the Gatehouse only; permit `unpack()` there, report full-frame memory honestly, and keep default admission ResearchOnly until named codec/camera sample-equivalence is proven.
4. Build a cryptographically indexed real-file professional RAW corpus and certification matrix.
5. Put CI around `decoded-measurement-main-house-e2e-v0.1`; do not promote it from existence alone.
6. Only after stacked review, decide which research chain is safe to promote toward `main`; never force-push.

## Workflow rules for the next chat

Before saying a branch/module is current or green, fetch its fresh branch head and relevant workflow result. Preserve failed runs as failed history. Do not auto-merge draft PRs. Do not alter sealed/canonical bytes merely to satisfy a newer toolchain warning. Distinguish implementation head, documentation overlay head and promoted `main` at all times.

Before ending every substantive work cycle, apply `docs/DOCUMENTATION_SYNC_POLICY_2026-09-11.md`: update affected module README/state, current module/claim/CI indexes as needed, and the global chat handoff when the continuation point changes.