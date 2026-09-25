# D.RAW TruthNegative N2 Candidate v0.1 — Validation Report

Status: **HOST CI PASS**

Validated commit: `4cc6a5361de9976b324c569ad6c80d2e029753fd`
GitHub Actions run: `36157203448`

Results:
- GCC host-reference: PASS
- Clang host-reference: PASS
- Clang ASan/UBSan sanitizer: PASS
- CTest candidate-pipeline test: PASS

Validated composition:
`Noise/variance contract -> Structure Preservation Gate -> Authority-Aware Neighborhood -> Bounded Residual Estimator -> Audit`

This pass proves that the current research primitive compiles and its synthetic invariant tests pass under both host compilers and sanitizers. It does **not** promote the estimator to production output and does not establish real-camera detail preservation.

Required next gate remains real-RAW side-car validation: residual/difference maps, flat-field statistics, edge MTF/SFR, fine texture, low-light colour, CFA colour edges, censor boundaries, and motion/occlusion cases.

Scientific invariants remain:
- Direct CFA unchanged.
- Scientific Master unchanged.
- TruthNegative unchanged.
- createsNewEvidence=false.
- scientificWritebackAllowed=false.
