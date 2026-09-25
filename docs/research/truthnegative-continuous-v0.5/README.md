# TruthNegative Continuous Scientific Negative v0.5

Status: **MAIN-INTEGRATED SCIENTIFIC STATE + PRO-ONLY PRODUCTION PREVIEW BRIDGE**

v0.5 changes the role of the dense scientific negative from a fixed target raster into a **raster-independent scientific-negative state**.

It does not replace TN-4. TN-4 remains the current materialized source-resolution scientific-negative container. v0.5 adds the continuous/queryable layer that can sit between TN-4 semantics and the Free-World resolver.

## Architecture

```text
sealed source
  -> Scientific Master
  -> Open Scene local authority field
  -> TruthNegative Continuous State v0.5
  -> area-integrated target query
       -> 12 MP
       -> 50 MP
       -> 200 MP
       -> preview raster
       -> another finite lattice
```

The target raster is not part of the scientific-negative state identity.

The same state SHA-256 is retained across all finite output lattices.

## State identity

A v0.5 state binds:

- exact source-evidence SHA-256;
- exact Scientific-Master SHA-256;
- canonical local-authority-field SHA-256;
- source width and height;
- reconstruction backend ID;
- colour-binding ID;
- one physical frame;
- one independent evidence item.

This makes TruthNegative the authority-preserving interface between the Scientific Master and the richer Free-World representation.

## Canonical local-authority digest

v0.5 defines a canonical SHA-256 over the complete source-grid Open Scene Field records in 64x64 tile order.

The hash covers every channel record's:

- numeric Float32 value bits;
- creation role;
- authority;
- uncertainty class;
- bound domain;
- p95/support/bound presence and values;
- contribution mask.

Changing one authority or bound changes the digest even if the visible RGB stays numerically identical.

## Query

A target query uses the Free-World v0.2 area-integrated continuous resolver.

Every query returns:

- Float64 scene-linear RGB;
- exact positive normalized source footprint;
- source role fractions;
- local authority fractions;
- uncertainty where admitted;
- censor-bound semantics;
- state SHA-256;
- query SHA-256.

The query always reports:

- `measuredTargetClaimCount = 0`;
- `createsNewEvidence = false`;
- `scientificWritebackAllowed = false`;
- `physicalFrameCount = 1`;
- `independentEvidenceCount = 1`.

## Why this supersedes fixed 4x as the conceptual TruthNegative

TN dense projection v0.3 remains a valid historical finite projection. Its 4x bilinear lattice is no longer required to define TruthNegative itself.

TruthNegative v0.5 is the state/function.

A 16320x12288 grid is one possible projection of that state, not its identity and not proof of 200 MP measured CFA.

## Round-trip / explainability gate

v0.5 does not claim arbitrary resampling is mathematically invertible.

Instead, every query is explainable by:

- one unchanged state identity;
- one declared target footprint;
- a normalized list of contributing source coordinates;
- a deterministic query digest;
- authority that can only stay equal or fail closed.

This is the first round-trip oracle: a target result can always be traced back to the exact scientific-negative state and source footprint that produced it.

## Main-integrated production bridge

The first Android bridge is now implemented, PRO-only and bounded. It renders a diagnostic Free-World preview from:

```text
DNG source
 -> Scientific Master
 -> Open Scene Field v0.85
 -> TruthNegative Continuous v0.5
 -> v0.7 Appearance/Display resolve
 -> preview
```

PURE remains untouched. Existing validated exports remain untouched until separately admitted.

The bridge is DNG-only, capped at 192 pixels on the longest preview edge, requires Scientific Master/Open Scene bit-identity for loaded tiles, re-verifies sealed source identity, and exposes authority/footprint/state diagnostics in the PRO UI.

Validation:

- v0.5 GCC/Clang/ASan/UBSan: run `36107899950` — SUCCESS;
- Android stable-signed ARM64 bridge build: run `36108523949` — SUCCESS;
- Unified Output Preview Android integration: run `36108523910` — SUCCESS.
