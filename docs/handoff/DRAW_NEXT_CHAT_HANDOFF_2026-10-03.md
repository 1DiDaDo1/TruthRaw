# D.RAW next-chat handoff — 2026-10-03

## Continuation code: 44488

When a new chat receives only **44488**, restore the complete D.RAW/TruthRaw project through this checkpoint before changing scientific or Android code.

The continuation code is a recognition anchor. The durable authority for recovery is the repository. Read, in order:

1. `state/CURRENT_PROJECT_STATE_2026-10-03.json`
2. this handoff
3. `START_HERE_NEW_CHAT.md`
4. `docs/DOCUMENT_STATUS_INDEX_2026-10-03.md`
5. `docs/research/scientific-master-exact-gauge-retained-artifact-v0.3/README.md`
6. older dated handoffs only when historical provenance is needed.

Do not ask the user to reconstruct the project manually when repository state can recover it.

## Current repository checkpoint

Repository: `1DiDaDo1/TruthRaw`

Active integration branch: `fix/research-fresh-rerun-v01`

Current scientific/code checkpoint before this documentation-only refresh:

`b82ada319ede5a87b0da0cdfe8732a19a3e74ff6`

That commit is the merge of PR #127, **Scientific Master Tile-Read Attribution v0.1**. It contains the accumulated validated performance line through PRs #125, #126 and #127.

Current Android lineage at that source checkpoint:

- application ID: `com.truthraw.adaptiveui`
- versionCode: `26100124`
- versionName: `0.53-v0.84.2-scientific-master-tile-read-attribution-v01`
- normal continuation remains install-over-current; do not uninstall or clear app data unless a specific test explicitly requires it.

The documentation refresh itself must not be mistaken for a scientific/source-code change. After its merge, the integration-branch Git head may advance while the scientific/code checkpoint above remains the same until the next code PR.

## Recent validated performance lineage

### Authority line

PRs #109 through #117 progressively optimized N2 support reuse and the authority stream. The accepted authority-performance reference remains **v0.2.10 Pixel-Triplet Authority Encoder**.

Accepted v0.2.10 two-RAW aggregate profile: `31,973.220820 ms`.

Accepted combined authority direct-record-stream time: `9,958.35 ms`.

v0.2.11 Canonical Template/Suffix Reuse was scientifically/byte-contract safe but did not produce a meaningful device speedup. It was not promoted as a performance baseline. The authority micro-optimization diminishing-returns gate is therefore active.

Do not restart authority byte-stream micro-optimization merely because it remains a large block. Re-open that frontier only with a new structural hypothesis.

### Center-Excluded profiling and fixed topology

PR #124 profiled the Center-Excluded v2 internals and showed on the two established device RAWs that `predictor_estimate` consumed about 86–87% of the candidate loop. Pair construction/gating, scale consistency and admissibility/slot construction were the dominant sampled predictor internals.

PR #125 added the fixed-topology fast path for the established `{2,4,8}` radii × four directions × two symmetric sides while keeping the generic predictor as the fail-closed fallback and requiring exact parity.

Real-device result:

- RAW1 Center-Excluded: `4088.13 ms -> 1985.62 ms` (`-51.43%`)
- RAW1 predictor: `3021.54 ms -> 1020.15 ms` (`-66.24%`)
- RAW2 Center-Excluded: `5675.69 ms -> 2870.54 ms` (`-49.42%`)
- RAW2 predictor: `4467.30 ms -> 1628.82 ms` (`-63.54%`)
- combined Center-Excluded: `9763.82 ms -> 4856.16 ms` (`-50.26%`)
- combined predictor: `7488.84 ms -> 2648.97 ms` (`-64.63%`)
- aggregate two-RAW profiler: about `31963.15 ms -> 26169.37 ms` (`-18.13%`).

Parsed Foundation content outside `performance_diagnostics_v0_1` was exactly unchanged.

Architectural lesson: optimization may specialize a proven topology, but the specialized path is not allowed to redefine the general scientific contract. Keep the generic path available for future topology and model expansion.

### Scientific Master bind profiling

PR #126 added diagnostic-only Scientific Master bind attribution. A true cold two-RAW device run measured about `27,014.29 ms` aggregate and split the combined Scientific Master bind as follows:

- total bind: `15,739.38 ms`
- authority observer: `9,912.67 ms` (~63.0%)
- RAW source reads: `2,640.68 ms` (~16.8%)
- reconstruction: `1,305.22 ms` (~8.3%)
- unattributed binder remainder: `1,880.81 ms` (~11.9%).

Each RAW showed `6,144` RAW-tile reads and `3,072` reconstruction calls.

The scientific output remained unchanged; timings are diagnostic only and do not grant authority.

### Scientific Master tile-read attribution

PR #127 resolved the 6,144-vs-3,072 count without inventing a per-pass timing split.

The established Scientific Master streaming binder v0.2 has two canonical tile passes:

- **pass 1** reads Stage-2 support, performs reconstruction/Scientific Master work, authority observation and the high-16 self-gauge selection work;
- **pass 2** performs no reconstruction and re-reads Stage-2 support only to resolve the low 16 bits of the exact self-gauge median inside the selected high-16 bucket.

Attribution is accepted only when the observed total RAW-read count reconciles exactly with the two-pass schedule. Otherwise the diagnostic state is fail-closed/unknown.

The established two RAWs reported:

- `TWO_PASS_BINDER_SCHEDULE_RECONCILED`
- total RAW reads `6144`
- pass-1 attributed reads `3072`
- pass-2 attributed reads `3072`
- unattributed reads `0`
- `per_pass_timing_inferred=false`
- `optimization_applied=false`.

PR #127 is merged. It is diagnostic evidence, not itself a performance optimization.

## Next code frontier — Exact Gauge Retained Artifact v0.3

The next candidate is **Scientific Master Exact Gauge Retained Artifact v0.3**.

Goal: remove the second Stage-2/RAW tile-read pass without changing the exact Scientific Master or gauge semantics.

The intended route is:

1. keep v0.2 as the canonical proven two-pass binder;
2. introduce a versioned v0.3 binder/optimization route above it;
3. during pass 1 retain only the exact Float32 bit patterns required by the existing self-gauge eligibility contract;
4. preserve the existing high-16 selection semantics;
5. after high-16 selection, derive the exact low-16 histogram/median result from the retained artifact rather than re-reading every Stage-2 tile;
6. if the artifact cannot be admitted because of memory budget, topology, version, semantic mismatch or future pass requirements, fall back completely to the canonical v0.2 two-pass route.

Do not implement this as an assumption that Scientific Master will always have exactly two passes. The retained object must be an extensible **pass-artifact contract**. Gauge retention is the first artifact type, not the definition of the abstraction.

### Mandatory parity gates for v0.3

Before any device promotion, require at minimum:

- exact Scientific Master SHA-256 identity;
- exact `gaugeMedian` Float32 bits;
- exact sample values/states/counts;
- exact authority digest/count semantics;
- exact reconstruction behavior;
- unchanged source and measured-anchor values;
- unchanged promotion/firewall state;
- no new evidence;
- no scientific writeback;
- deterministic repeated-run parity;
- explicit budget-fallback parity;
- unsupported topology/semantic extension must fail closed to v0.2;
- diagnostic telemetry must state which route ran, retained bytes, peak-resident effect, avoided rereads and fallback reason.

`residentPeakBytes` is allowed to change honestly because the candidate intentionally trades bounded temporary memory for fewer Stage-2 rereads. That memory change is not scientific evidence. The optimization must not be applied when its explicit budget cannot be satisfied.

The first device expectation, only after host parity and full CI are green, is roughly a reduction from `6144` to `3072` Stage-2/RAW tile reads per established RAW when v0.3 is actually applied. This is a performance expectation, not a scientific acceptance criterion.

## Cable-room / expansion law

The user's "ruimte in de kabels" requirement remains a hard architecture constraint.

For every performance specialization:

**general semantic route -> versioned specialized route -> parity oracle -> fail-closed fallback**

Consequences:

- current topology must not become a permanent universal assumption;
- current two-pass scheduling must not become the universal binder contract;
- caches/artifacts may accelerate already-authorized data flow but may not create authority;
- future pass types, extra observers, other CFA layouts, other RAW sources and new scientific models must be able to coexist;
- unsupported future semantics return to the general route rather than being coerced into today's fast path;
- schema additions must not silently redefine older fields;
- UNKNOWN remains valid when evidence or an extension contract is absent.

The cable may become faster; it must not become narrower.

## Permanent scientific laws

These are not performance options:

- `MEASURED != RECONSTRUCTED != APPEARANCE`.
- Seal the evidence, not the thinking.
- Representation can exceed the source; knowledge claims cannot exceed the evidence.
- Direct-CFA/source evidence remains immutable/sealed.
- Single-frame provenance remains explicit.
- Scientific Master remains separate from export/presentation.
- Camera/lens/vendor/RAW identity may route decoding but may not select scientific truth.
- APK/GCam/computational-RAW content may not determine D.RAW/TruthRaw evidence, calibration or Scientific Master authority.
- Held-out values may evaluate a frozen candidate but may not fit/select it.
- UNKNOWN residual remains UNKNOWN until supported.
- No AI/ML/neural/generative runtime in the scientific path.
- No candidate availability implies correction or promotion.
- No performance/diagnostic route may write Scientific Master as a promotion action.
- SOURCE/SENSOR SPACE, WORLD/SCENE SPACE and VIEW/OUTPUT SPACE remain distinct.
- Appearance/frontside geometry is not automatically sensor evidence.
- Raster size is not independent optical information.

## Scientific non-promotion result that must remain remembered

Anchor-Constrained Local Reconstruction v0.1 remains **not promoted** after the real-CFA hold-out test:

- true hold-outs `21,760`
- comparable samples `19,560`
- affine MAE ~`0.00154976` vs baseline ~`0.00147486`
- affine RMSE ~`0.00205484` vs baseline ~`0.00190590`
- affine bias ~`-0.00049664` vs baseline ~`-0.00009471`
- affine wins `9363`, baseline wins `10197`
- affine added `2200` valid points
- uncertainty was too optimistic.

Coverage improvement is not sufficient for promotion. The later scientific direction remains deterministic local model selection based on structure support, direction, CFA phase and uncertainty, with "no suitable model" allowed.

## Device-test workflow for the next performance candidate

Once v0.3 is actually implemented and host/CI parity is green:

1. install the higher-versionCode APK over the current app;
2. do not clear app data/cache unless the specific validation design says otherwise;
3. use the same two established RAWs for the first A/B;
4. force/verify a true current-run compute rather than accepting a warm cached result as timing evidence;
5. run the universal analysis;
6. export Free World Observation Geometry Foundation v0.1 JSON;
7. compare route selection, current-run provenance, tile-read counts, exact scientific output identity, memory telemetry and performance;
8. keep the candidate unpromoted if any parity/firewall/budget gate is not exact.

## 44488 recovery instruction

A future chat receiving **44488** should restore this file and `state/CURRENT_PROJECT_STATE_2026-10-03.json`, verify whether the repository has advanced beyond the recorded source checkpoint, and then continue from the newest proven state.

At this checkpoint the next code task is **not** to micro-optimize authority or blindly cache the full frame. It is to source-inspect the exact self-gauge eligibility/cardinality and then implement the bounded, versioned Exact Gauge Retained Artifact v0.3 with exact parity and complete fallback to v0.2.