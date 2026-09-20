# TruthRaw v0.70 — Canonical Open Scene + Restoration role-mask binding

Date: 2026-09-20

Branch:

`integration/truthraw-suite-v0-70-canonical-open-scene-role-binding`

App version:

`0.35-v0.70-canonical-open-scene-role-binding`

Status: **active integration / CI pending at document creation**.

## Purpose

v0.70 starts closing the first two direct post-v0.69 loose cables:

1. canonical Open Scene identity across TN-3 and normal Restoration projections;
2. cryptographic binding of the exact full-resolution Restoration role mask into every normal projection.

It does not change the validated v0.63 PURE pixel route and does not change the v0.67 Restoration algorithm.

## Canonical Open Scene v0.70

New native module:

- `suite_android/app/src/main/cpp/open_scene_canonical_v0_70.h`;
- `suite_android/app/src/main/cpp/open_scene_canonical_v0_70.cpp`.

Schema:

`TruthRawOpenSceneCanonicalState/0.70`

Semantic parents are explicitly recorded:

- `TruthRawOpenSceneRegion/0.7`;
- `TruthRawOpenSceneStateSummary/0.8`.

The bridge transplants their central invariants into the current native Android path:

- one source evidence identity;
- one Scientific Master identity;
- one physical frame / one independent evidence item;
- Dynamic Authority stays distinct per RGB channel;
- generic missing channels remain `UNKNOWN` until source-bound uncertainty admits reconstruction;
- colour authority is `SOURCE_METADATA_BOUND`, not independent physical calibration;
- illumination authority is `SOURCE_BOUND_ESTIMATE`;
- detail status is `NEUTRAL_OR_BLOCKED` unless stronger support is separately admitted;
- counterfactual pixel count = 0;
- Scientific-Master writeback = 0;
- creates-new-evidence = 0;
- chunking changes scientific identity = 0.

### Canonical per-pixel state

The state raster no longer collapses all finite sites into one generic byte. It retains which CFA channel was directly supported:

1. `R_CALIBRATED_ESTIMATE__G_UNKNOWN__B_UNKNOWN`;
2. `R_UNKNOWN__G_CALIBRATED_ESTIMATE__B_UNKNOWN`;
3. `R_UNKNOWN__G_UNKNOWN__B_CALIBRATED_ESTIMATE`;
4. `R_CENSORED__G_UNKNOWN__B_UNKNOWN`;
5. `R_UNKNOWN__G_CENSORED__B_UNKNOWN`;
6. `R_UNKNOWN__G_UNKNOWN__B_CENSORED`.

This is a categorical authority state, not a replacement pixel image.

### Four canonical hashes

The v0.70 builder produces:

- `dynamic_authority_artifact_sha256` — exact canonical RGB authority byte stream;
- `open_scene_state_sha256` — exact canonical per-pixel Open Scene categorical raster;
- `open_scene_policy_sha256` — versioned policy + lineage + geometry binding;
- `open_scene_artifact_sha256` — domain-separated binding of authority + content + policy.

The content hash is over canonical raster order and is independent of processing chunk size.

## TN-3 integration

TN-3 remains source-resolution and camera-native.

Its header now additionally records:

- `open_scene_canonical_schema=TruthRawOpenSceneCanonicalState/0.70`;
- semantic parent v0.7/v0.8 identifiers;
- all four canonical hashes;
- explicit categorical state definitions;
- source-metadata colour authority;
- source-bound illumination estimate;
- no counterfactual;
- no scientific writeback;
- no new evidence.

Android post-write verification requires those markers and hash shapes before TN-3 success is reported.

## Restoration projections recompute the same canonical state

DNG/TIFF/EXR projection does not merely trust a TN-3 file.

The projection engine reopens the original sealed source route, reconstructs the same source-bound Scientific Master lineage, rereads the raw CFA support, and independently rebuilds the v0.70 canonical Open Scene state.

This prevents a normal projection from silently carrying an unrelated Open Scene identity.

Current projection chain:

`sealed source + admitted Scientific Master + verified .trr`
→ recompute canonical Dynamic Authority/Open Scene
→ require `.trr` lineage equality
→ project full-resolution derivative
→ write canonical Open Scene identity into provenance
→ whole-file staging/destination SHA verification.

## Restoration role-mask binding

The `.trr` role raster remains the full authoritative per-pixel mask:

- role 0 = `PRESERVE_SCIENTIFIC_MASTER`;
- role 1 = `AESTHETIC_REINTEGRATION_ONLY`;
- role 2 = `UNRESOLVED_LOSS`.

v0.70 computes the exact role-mask SHA-256 while validating the full `.trr`.

That SHA-256 is now bound into:

- Float32 DNG private TruthRaw metadata;
- Float32 TIFF provenance;
- OpenEXR `truthrawProvenance`.

DNG derivative admission now fails closed unless an explicit non-zero role-mask hash is present.

Therefore all three normal projections identify the exact role raster that governed the derivative.

## What is still open

This closes only part of the first two loose cables.

### Open Scene cable

**Improved to:** TN-3 and all normal projections share the same native canonical Open Scene algorithm and identity.

Still open:

- Advanced preview/runtime should consume/report the same canonical artifact identity rather than only equivalent authority concepts;
- the v0.67 `.trr` container itself predates v0.70 and therefore does not yet store the canonical Open Scene artifact hash internally.

### Role-mask cable

**Improved to:** every normal projection cryptographically identifies the exact role mask.

Still open:

- the complete mask bytes themselves remain in `.trr`;
- DNG/TIFF/EXR do not yet embed the full mask or automatically emit a bound companion mask sidecar.

This is intentional: a hash binding is stronger than losing provenance, but it is not yet a self-contained copy of the full mask.

## Remaining direct cables after v0.70

1. finish canonical Open Scene propagation into Advanced and the Restoration container;
2. decide/implement full role-mask embedding or a versioned companion sidecar for DNG/TIFF/EXR;
3. independent DNG/TIFF/OpenEXR conformance validation;
4. effectful real-device Restoration validation with actual censored CFA support.

Larger later lines remain current PTC/certificate, multi-vendor scientific admission, and evidence-bounded physical Scene Physics.

## Non-regression requirements

- PURE stays `TRUTHRAW_PURE_SELF_BINDING_V0_63`;
- v0.67 Restoration eligibility/support rules stay unchanged;
- v0.68 foreground/staging transaction semantics remain required;
- TN-3/Open Scene does not create a second scientific world;
- projection is not new evidence;
- role-1 is never relabelled measured;
- role-2 is never relabelled successfully restored truth.
