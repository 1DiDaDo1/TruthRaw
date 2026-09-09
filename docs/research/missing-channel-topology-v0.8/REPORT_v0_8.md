# TruthRaw topology validation v0.8 — real held-out CFA study

## Decision

**`HELDOUT_CFA_TOPOLOGY_NECESSARY_CONDITION_PASS_CO_SITED_CERTIFICATION_OPEN`**

This study strengthens the evidence for local missing-channel topology, but it does **not** set `topologyCertified=true` for reconstructed RGB channels.

## Why this study exists

TruthRaw v0.3 deliberately kept transported missing-channel entries `topologyCertified=false`. The unresolved question is whether local reconstruction topology can be independently bounded using measurements that were not shown to the predictor.

## Real sources

- `094414`: SHA-256 `578fad42dad6819b1d3f9a1f9cbfcc5c547b63ae01f3951f26dca988d655d10e`
- `094416`: SHA-256 `53352735e577e1465a0c10b9f162eeb2dede3c6cb23ff256f327027cb29bd8db`
- `094423`: SHA-256 `ade9d84542916678d1a198c6baff219806d44fa8f050ef8d7545dfae90644f65`

All are 4080×3072 BGGR source DNGs. Stage-2 is reconstructed from source raw codes using per-phase BlackLevel, WhiteLevel and OpcodeList2 GainMap exactly once via the closed v0.4 decoder semantics.

## No-leak protocol

For every source and every CFA colour:

1. split the measured CFA lattice into four deterministic spatial folds;
2. hide one fold at a time;
3. exclude source-censored targets/support;
4. predict the hidden sample only from non-hidden local measured support;
5. for R/B, use a target-independent adjacent-green colour-difference guide;
6. for G, use same-channel spatial support;
7. compare prediction only after reconstruction to the original hidden physical measurement.

A total of **2,160,000** held-out measured CFA samples are evaluated (720,000 per R/G/B channel).

## Aggregate results

| channel | ordering agreement | curvature agreement | weighted MAE | mean p95 abs | worst-fold ordering | worst-fold curvature |
|---|---:|---:|---:|---:|---:|---:|
| R | 0.978457 | 0.909313 | 0.00386283 | 0.01203809 | 0.971287 | 0.887277 |
| G | 0.947504 | 0.852285 | 0.00504050 | 0.01633521 | 0.938371 | 0.842440 |
| B | 0.976971 | 0.906931 | 0.00356478 | 0.01111963 | 0.968943 | 0.873462 |

`ordering agreement` is evaluated only where the hidden physical measurement differs from a support sample by more than 2× the source-bound combined marginal noise sigma. `curvature agreement` similarly evaluates the sign of the centre-vs-opposed-neighbour curvature only when that curvature is significant relative to source-bound marginal noise.

## Findings

- R/B local ordering is strong across the three real scenes (~97–98%).
- Green is consistently the weakest topology layer (~94–96% ordering; ~84–87% curvature depending on scene/fold).
- The weakness is not uniform spatially; the held-out data again identifies difficult green regions, consistent with the project’s earlier green/mid-field uncertainty concern.
- The R/B adjacent-green colour-difference guide is not universally better than the pure spatial baseline: aggregate MAE ratio is about 0.9869 for R and 0.9866 for B. Therefore no claim is made that the guide is globally optimal.
- Raw unconstrained R/B estimates occasionally leave the local support interval (3.44% R, 4.07% B); the support-limited policy keeps the evaluated estimate bounded.

## Hard claim boundary

This is a **necessary-condition topology proxy**, not complete missing-channel certification.

A Bayer sensor does not measure R, G and B at the same physical pixel. Hiding a measured R/G/B site gives independent ground truth for self-supervised reconstruction on that measured lattice, but it does not create a physically measured co-sited reference for the colours that were absent there.

Therefore:

- `topologyCertified` remains `false` for all reconstructed missing-channel entries;
- v0.3/v0.5 transported p50/p95 anchors remain proxies;
- this study may support future bounded topology-risk modelling;
- full certification requires independent co-sited evidence, e.g. controlled phase-shift/reference capture or another defensible external measurement design.

## Reproducibility

- deterministic RNG seed: `0x5452555448524157`
- first result SHA-256: `758599a534ed5226d6eb3088fed30fa5cb38889df827a0c804d709617c0353ba`
- exact rerun result SHA-256: `758599a534ed5226d6eb3088fed30fa5cb38889df827a0c804d709617c0353ba`
- exact rerun byte comparison: **PASS**

## Next gate

Build an independent **co-sited topology validation capture protocol**. The preferred route is a controlled static target with calibrated spatial registration / phase diversity so a colour absent at one Bayer site in one exposure is independently measured at the same scene coordinate in another validation-only exposure. Multi-frame data may be used for validation evidence; it must not enter the single-frame TruthRaw scientific master.
