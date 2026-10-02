# Center-Excluded Fixed Topology v0.1

Status: performance candidate only. No scientific promotion.

## Motivation

The real-device Center-Excluded v2 profiling run on the two established RAWs showed that `predictor_estimate` consumes about 86–87% of the candidate-loop time. Within the deterministic 1-in-64 sample, the largest repeated costs were generic admissibility/slot construction, symmetric-pair build/gating, and scale-consistency handling.

The runtime caller already constructs a fixed geometry: radii 2, 4 and 8; four directions; two symmetric sides. The generic predictor nevertheless rebuilds maps, radius lists and vectors for every candidate center.

## Candidate

`estimateFixedTopology()` replaces only those generic data structures with bounded stack-resident arrays:

- 3 radius slots: 2, 4, 8;
- 4 direction slots;
- 2 side slots;
- at most 12 symmetric pairs;
- at most 3 scale estimates.

Thresholds, admissibility, pair math, scale math, accumulation order, Result fields and authority remain the established v0.2 contract.

The normal `estimate()` entry point uses the fixed path only for unsampled runtime calls. Unsupported admissible geometry or a fixed-path failure falls back to the canonical generic implementation. The existing 1-in-64 diagnostic calls continue through the generic path so the previous profiling reference remains observable during the validation build.

## Parity oracle

The host test runs the canonical generic route by supplying a diagnostic sink, then runs `estimateFixedTopology()` on the same inputs and compares every Result field. All floating-point fields are compared by exact IEEE-754 bit pattern, not tolerance. It also covers rejected pairs, cross-scale rejection, inadmissible/censored samples, duplicate geometry and an unsupported radius that must fall back generically.

The CI workflow configures, builds and executes this parity test in addition to the static authority checker.

## Derived-cache validation epoch

The public R12 profiler stage identity remains unchanged because the scientific output contract is unchanged. The private `ResearchProfileStageCacheV01` wrapper requires implementation epoch `CENTER_EXCLUDED_FIXED_TOPOLOGY_V01` for that stage. Existing R12 cache files therefore miss once after installing the validation APK; newly computed results are then reusable. This implementation epoch is private runtime cache provenance only and is not scientific evidence.

## Invariants

- source values are not modified;
- held-out center value is never used to build its predictor;
- no reconstruction/calibration/correction is promoted;
- `candidate_applied=false`;
- `creates_new_evidence=false`;
- `scientific_writeback_allowed=false`;
- diagnostic timing never changes scientific authority;
- no SIMD/NEON is introduced at this stage;
- the v0.2.1 Center-Excluded audit hash definition is unchanged.

Promotion requires full CI plus a fresh real-device two-RAW measurement. A speedup alone is insufficient: exact scientific/result parity and all safety invariants must remain intact.
