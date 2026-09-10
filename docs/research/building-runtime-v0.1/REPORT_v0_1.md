# TruthRaw Building Runtime v0.1 — Validation Report

## Decision

`LOCAL_PASS__12_ROOM_GRAPH__GCC_CLANG_SANITIZED__RESEARCH_ORCHESTRATION_ONLY__SCIENTIFIC_PROMOTION_OPEN`

## What was implemented

A bounded C++20 orchestration core modeling TruthRaw as a flexible building. It contains 12 explicit rooms, a forward truth-floor hierarchy, typed corridor tokens, an evidence-neutral decision ledger, an Android-agnostic resource governor and a deterministic execution-wave planner.

The runtime changes **execution topology**, not scientific authority. A stronger phone may receive larger tiles, more concurrent heavy-room leases and an optional Vulkan preference. A weaker phone walks the same admissible graph with smaller tiles and fewer simultaneous heavy rooms.

## Safety contracts tested

1. Activated rooms never multiply physical or independent evidence.
2. Appearance EV changes cannot alter scientific evidence counts, claim status or Best Conditioning EV.
3. Physical Capture EV / Best Conditioning EV / Appearance EV remain distinct.
4. Unknown sigma blocks sigma-dependent rooms rather than becoming zero uncertainty.
5. Identical inputs produce deterministic meaningful runtime state and ledger decisions.
6. Cyclic graphs and backward truth-floor dependencies are rejected.
7. A pre-modified scientific master is rejected.
8. Research-only and candidate status are preserved; no automatic promotion exists.
9. Runtime structures are fixed-size and bounded for mobile use.
10. Provenance identity is preserved while decisions are written to a separate ledger.
11. Invalid evidence multiplicity fails closed.
12. Scientific rooms cannot read/write Counterfactual/Appearance state, mutate the scientific master, or increase evidence.
13. Corridor tokens carry external artifact handles and authority only; backward Appearance -> Scientific hand-off is rejected.
14. Low/mid/high resource tiers change scheduling/resources only.
15. Severe thermal pressure reduces concurrency/backend preference without changing truth state.
16. Explicit too-small working-set budgets fail closed.

## Preserved negative intermediate findings

### Raw-struct determinism fixture
The first Release fixture used `memcmp(RuntimeResult)` and failed. This was a **test-design failure**, because C++ padding bytes are not semantic state and need not be stable. The gate was corrected to compare every meaningful scalar, decision and ledger field. The corrected test passes under GCC, Clang and sanitizers.

### Superseded staging blobs
Before the 12-room/resource-governor expansion was re-audited, three Git blob objects were created for an older 6-room core (`ca5db526...`, `6d0819d0...`, `29903f2e...`). A later byte audit detected that they no longer matched the current local source. They were deliberately **not committed** and are not candidate evidence. The current candidate is re-hashed and re-tested from the final 12-room bytes.

## Current local validation

- GCC 14.2.0 Release, `-O3 -DNDEBUG -Wall -Wextra -Werror`: PASS.
- Clang 17.0.0 Release, same warning gate: PASS.
- Clang 17.0.0 ASan/UBSan: PASS.
- GCC/Clang/sanitizer executable metric output: byte-identical.
- `physicalFrameCount=1`.
- `independentEvidenceCount=1`.
- `scientificMasterModified=0`.
- 12 bounded room decisions / 12 ledger events.
- `sizeof(RuntimeResult)=240` bytes.
- low fixture: 32 MiB total lease budget, 1 heavy room, 128 px tile, 11 execution waves.
- high fixture: 256 MiB total lease budget, 4 heavy rooms, 512 px tile, 8 execution waves.

## Claim boundary

This validates the scheduler/resource contracts only. It does not prove physical relighting, calibrated electron noise, missing-channel truth, colorimetric accuracy, Android integration or improved photographic quality. Those remain properties and gates of their own rooms.

## Local upstream-binding note

The standalone `/mnt/data` candidate directory is not a full Git checkout, so the local integrity verifier cannot independently resolve repository-relative upstream paths there. Its local run therefore used the explicit `TRUTHRAW_LOCAL_NO_GIT_SKIP_UPSTREAM=1` staging flag for **only** that dependency-presence subgate; all manifest, state, source and metric hashes were still verified. The three expected upstream Git blobs were separately checked against GitHub before candidate construction and the GitHub Actions workflow does **not** set the skip flag, so upstream Manifold/CICM/Room-Capsule bindings remain mandatory in CI.
