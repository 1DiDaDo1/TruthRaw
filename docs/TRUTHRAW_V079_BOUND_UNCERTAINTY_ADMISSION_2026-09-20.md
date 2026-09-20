# TruthRaw v0.79 — Bound Uncertainty Admission — 2026-09-20

## Status

Integration candidate on:

`integration/truthraw-suite-v0-79-bound-uncertainty-admission`

Android version:

`0.45-v0.79-bound-uncertainty-admission`

v0.79 changes no reconstruction pixels. It introduces the admission gate that decides whether any uncertainty model may be used to upgrade missing-channel scientific authority.

## Core rule

Similarity is not authority.

A model may not transfer because two files happen to share:
- 4080x3072 raster size;
- BGGR CFA;
- WhiteLevel 1023;
- the same phone model;
- the same nominal lens;
- the same reconstruction family name.

Admission requires the exact source domain, sample class, backend hashes, model assets, prospective evidence and reconstructed-quantity trace binding.

## Historical v5.0g scope

The registered historical candidate is limited to:

- make: HONOR
- model: BKQ-N49
- source class: HONOR vendor DNG
- 4080x3072
- BGGR
- WhiteLevel 1023
- 22.48 mm tele
- valid NoiseProfile
- exact v4.7i backend hashes
- exact frozen v5.0g feature extractor/schema/model/binding
- exact prospective holdout result
- exact PTC uncertainty bridge

Matching all of these is still insufficient for promotion today.

The current decision remains:

`ELIGIBLE_TRACE_GATE_OPEN`

because no accepted certificate yet binds the historical v5.0g uncertainty coordinates to the exact F64 Scientific-Master reconstructed quantity.

An arbitrary non-zero trace hash cannot unlock the gate. The registry's accepted F64 trace certificate is intentionally all-zero, so `ADMITTED` is currently unreachable.

## Current Camera-5 processing DNG

The product camera path is:

`Camera2 RAW_SENSOR envelope -> topology-admitted prefix -> derived processing DNG -> Main House`.

This is not the historical HONOR vendor-DNG source domain.

The exact v0.79 decision is therefore:

`BLOCKED_SOURCE_DOMAIN_MISMATCH`

The old v5.0g model is not inherited even when geometry/CFA/WhiteLevel resemble the old tele class.

## Generic imported DNG

Until a source-class attestation is explicitly produced, imported DNG receives:

`BLOCKED_NO_SOURCE_ATTESTATION`

No filename or loose metadata heuristic is used to guess an uncertainty domain.

## Advanced binding

Advanced receives the actual ingress route from Kotlin:
- IMPORTED_FILE
- CAMERA_CAPTURE

Native code evaluates v0.79 and embeds:
- admission decision code;
- `reconstructedAuthorityAllowed`;
- immutable v0.79 decision SHA-256.

Current Advanced packet layout adds the v0.79 decision after:
- canonical Open Scene v0.70 SHA-256;
- channel-authority v0.78 SHA-256.

Current expected authority remains:

`RECONSTRUCTED = 0`

for both current camera and unattested generic import paths.

Any unexpected admitted result fails closed because a runtime p95 field generator is not yet wired.

## Exact falsification

Dedicated v0.79 tests verify:
- exact historical candidate stops at trace gate;
- fake trace certificate does not admit;
- current Camera-5 derived DNG blocks by source-domain mismatch;
- unattested import blocks;
- one-bit backend hash mutation blocks;
- one-bit model hash mutation blocks;
- prospective evidence mutation blocks;
- sample-class geometry/focal mutation blocks;
- decision SHA is deterministic.

GCC: PASS.
Clang: PASS.

## Scientific invariants unchanged

- immutable source evidence
- one physical frame / one independent evidence item
- Scientific Master pixels unchanged
- PURE v0.63 unchanged
- Zero-Line/L0 unchanged
- scene-scale unchanged
- Technical Backplane unchanged
- Open Scene v0.70 parent unchanged
- v0.78 authority sidecar semantics unchanged
- Restoration v0.67 unchanged
- no appearance writeback
- no counterfactual evidence

## Next gates

1. Finish Android integration CI.
2. Real-device v0.79 smoke test and verify Camera-5 reports the expected blocked source-domain decision.
3. Build exact F64 reconstructed-quantity trace certification for the historical v5.0g domain.
4. Independently calibrate/validate uncertainty for the current Camera-5 derived DNG domain.
5. Only then permit a source/backend-bound p95 runtime field to feed v0.78 RECONSTRUCTED authority.
6. Continue with v4.7j support-aware detail once local authority/support/uncertainty are truly available.
