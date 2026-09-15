# OPEN — exact v5.0g feature-extractor recovery — v0.10

Date: 2026-09-15

Status: **FAIL-CLOSED RECOVERY BLOCKER**

This document defines what counts as a valid recovery of the historical canonical v5.0g uncertainty feature semantics. It exists to prevent a later implementation from silently recreating the missing extractor approximately.

## Why this is open

The frozen v5.0g/p1 uncertainty model expects an exact 18-feature vector. The present precision/reconstruction trace does not contain enough information to regenerate all historical hidden-CFA/error/spread features with proven semantic parity.

The canonical v5.0g manifest proves that a historical extractor existed, but the extractor itself is absent from the current branch tree at the expected path.

Therefore canonical local uncertainty must remain unbound to the v0.8 mixed-precision pre/post-storage audit until exact feature semantics are recovered.

## Manifested artifacts to recover / verify

Authority: `canonical/uncertainty/v5.0g/MANIFEST_v5_0g.json`

### Primary feature extractor

- file: `uncertainty_core_v5_0g.py`
- expected SHA-256: `b1cfbf061a32aa4abccb9d8bb86a9b5b9257f88498081eb9aa659f9402d0919d`
- expected byte length: `10023`

### Historical runtime artifacts

- file: `uncertainty_runtime_v5_0g.cpp`
- expected SHA-256: `e025c399c985d31f0ae15bba81b82bd9d5b5f7aa4631c8f23e3dd2d4fa91c840`
- expected byte length: `1474`

- file: `uncertainty_runtime_v5_0g.h`
- expected SHA-256: `0783c66b52b6852ddc3507525eb607554a91ad49b93a5c6ed77e575b73d5df20`
- expected byte length: `192`

### Historical runtime parity vectors

- file: `RUNTIME_PARITY_VECTORS_v5_0g.json`
- expected SHA-256: `c98295c6c8f12c844ba089ba394f539a780a14ecb9d9e5ea00dc2e6f386b569d`
- expected byte length: `50219`

Recovery of the Python extractor is the minimum requirement for reconstructing the exact feature semantics. Recovery of runtime/parity artifacts is strongly preferred because it permits independent parity validation.

## Frozen 18-feature contract

The downstream frozen model expects:

1. `mean_sigma`
2. `mean_abs_demosaic_extra`
3. `std_abs_demosaic_extra`
4. `p90_abs_demosaic_extra`
5. `max_abs_demosaic_extra`
6. `mean_unseen_color_error`
7. `p90_unseen_color_error`
8. `max_unseen_color_error`
9. `mean_abs_r`
10. `mean_abs_g`
11. `mean_abs_b`
12. `std_r`
13. `std_g`
14. `std_b`
15. `support_std_over_sigma`
16. `spread_p90_over_sigma`
17. `spread_max_over_sigma`
18. `propagated_spread_over_sigma`

The names alone are not sufficient to reconstruct the feature definitions. Windowing, CFA visibility, target/error construction, support definitions, normalization, edge handling, held-out masking, coordinate alignment and leakage protections are part of the semantics.

## Recovery attempts already performed

Expected-path direct retrieval has been attempted against:

1. current `research/history-code-audit-2026-09-15` tree — not present;
2. archived `4f842d8fe86eca2b5808afb10fbb9f11a2631fed` state — expected path not present;
3. `docs/project-handoff-2026-09-11` commit `a8ad01df0dcf83a6e69fd49f9469522024a1240b` — expected path not present.

This does not prove the bytes do not exist in another retained archive, old handoff bundle, Library upload or unreachable Git object. It proves only that the checked expected paths do not recover them.

## Valid recovery criteria

A candidate extractor may be admitted as the historical extractor only if:

1. byte length is exactly `10023` bytes; and
2. SHA-256 is exactly `b1cfbf061a32aa4abccb9d8bb86a9b5b9257f88498081eb9aa659f9402d0919d`.

If the exact bytes cannot be recovered but a purported equivalent implementation is proposed, it must **not** be called historical parity merely because feature names match. It requires a separately documented equivalence proof against retained canonical training/validation/parity evidence. Until such equivalence is demonstrated, status remains OPEN.

## Forbidden shortcuts

Do not:

- infer formulas from feature names alone;
- use the v0.9 synthetic proxy feature as a canonical substitute;
- fill missing features with zero;
- derive hidden-CFA errors using held-out truth in a production path;
- use scene ID or source labels as predictors;
- silently change the 18-feature order;
- reinterpret model quantiles as covariance or sigma when they are not defined as such;
- call v0.7 synthetic analytic sigma a replacement for canonical local uncertainty;
- promote F64->F32 storage universally because its absolute error is small.

## What may continue while this remains open

This blocker is independent of physical acquisition. The following may continue:

- FotoGraaf physical Camera-5 16320x12288 Step 3B capture proof;
- exact `.rawsensor` provenance sealing;
- per-mode noise/PTC, shading, colour and SFR/MTF calibration collection;
- precision sweeps that do not claim canonical v5.0g/p1 uncertainty binding;
- tiled F64 reconstruction reference work;
- F64->F32 storage measurement reported as numerical error, not as canonically uncertainty-relative admission.

## Next action after exact recovery

If and only if the extractor hash is verified:

1. preserve the recovered bytes unchanged as historical/canonical provenance;
2. document exact feature and coordinate semantics;
3. build an adapter from the F64 reconstruction path to the exact 18 features;
4. prove no hidden-CFA/self-label leakage in the deployed feature path;
5. validate against recovered parity vectors if available;
6. evaluate local uncertainty only where every required feature is valid;
7. leave all unsupported pixels/features unresolved;
8. bind admitted local uncertainty into the v0.8 pre/post-storage audit;
9. re-run the F64-compute -> F32-storage gate relative to admitted local uncertainty;
10. repeat the complete gate on a physically proven 16320x12288 FotoGraaf source before 200MP precision promotion.

## Authority boundary

This document defines a blocker and recovery protocol. It does not promote the v0.9 research runtime, change the frozen v5.0g/p1 model, or create new scientific evidence.
