# TruthRaw Building Runtime v0.1 — Validation Report

## Decision

`LOCAL_PASS__GCC_CLANG_SANITIZED__RESEARCH_ORCHESTRATION_ONLY__SCIENTIFIC_PROMOTION_OPEN`

## What was implemented
A bounded C++20 orchestration core modeling the TruthRaw project as a building: immutable evidence is the foundation; rooms are processing/research modules; the runtime owns only corridor/dependency decisions and a separate decision ledger.

## Safety contracts tested
1. Activated rooms never multiply physical or independent evidence.
2. Appearance EV changes cannot alter scientific evidence counts, claim status, or conditioning EV.
3. PhysicalCaptureEV / BestConditioningEV / AppearanceEV remain distinct.
4. Unknown sigma blocks sigma-dependent rooms rather than pretending certainty.
5. Identical inputs produce deterministically identical semantic runtime state and ledger decisions; object padding bytes are outside the contract.
6. Cyclic dependency graphs are rejected.
7. A pre-modified scientific master is rejected.
8. Research-only and candidate status are preserved; no automatic promotion exists.
9. Runtime structures are fixed-size and bounded for mobile use.
10. Provenance identity is preserved while decisions are written to a separate ledger.
11. Invalid evidence multiplicity fails closed.
12. Scientific rooms cannot be configured to read/write appearance, modify the scientific master, or increase evidence.

## Claim boundary
This validation proves the orchestration invariants implemented here. It does not prove physical relighting, missing-channel truth, calibrated electron-domain noise, Android integration, or improved photographic quality. Those remain properties/gates of their respective rooms.

## Preserved negative intermediate result
The first Release fixture used raw `memcmp` on `RuntimeResult` to assert determinism and failed. This was a **test-design failure**, not evidence of nondeterministic decisions: C++ object padding is not semantic state and need not have stable byte contents. The gate was corrected to compare every meaningful scalar, decision, and ledger field explicitly. The corrected deterministic-state test must pass on GCC, Clang, and sanitizer builds.

## Local validation result
- Integrity verifier: PASS
- GCC Release + CTest: PASS
- Clang Release + CTest: PASS
- Clang ASan/UBSan + CTest: PASS
- Runtime result footprint: 176 bytes
- Runtime decisions: 6 bounded room decisions / 6 decision-ledger events in the full-request fixture

The module therefore passes its local software-contract gates. Scientific/photographic superiority is **not** claimed.
