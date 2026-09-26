# D.RAW Lens-Independent Free World Observation Contract v0.1

Status: **SEALED ARCHITECTURE CONTRACT — NO PIXEL-ROUTE CHANGE**

This contract makes the lens-independent Free World explicit.

It binds existing project foundations rather than replacing them:

- immutable sealed Source Evidence;
- Zero-Line / TruthRange;
- Float64 branch-sensitive science and controlled Float32 storage;
- Scientific Master;
- Dynamic Authority / Open Scene;
- TruthNegative Continuous;
- Deep Scene / separate geometry and radiometry authority;
- temporal/multi-view observation research;
- View/Appearance separation.

## Core topology

```text
Source-specific physical observation
 -> D.RAW Observation Contract
 -> Source Capability Envelope
 -> calibrated Scientific Master
 -> TruthNegative Continuous
 -> Free World Observation Graph
 -> View Contract
 -> finite projection
```

The Observation Contract is source/lens specific.

The Free World is not.

## Source-neutral observation

A conforming observation identifies:

- source evidence;
- physical acquisition/procedure;
- source topology;
- calibration/capability scope;
- gauge;
- authority and uncertainty;
- Scientific Master;
- TruthNegative;
- provenance.

Missing support fails closed to UNKNOWN/UNPROVEN.

## Zero-Line binding

`T = log2(L/L0)` remains the canonical positive-light relative coordinate.

Two observations may inhabit this coordinate family without having a proven
common gauge.

Cross-source radiometric comparison or fusion requires an explicit admitted
gauge relation.

## TruthNegative boundary

One TruthNegative represents one admitted observation lineage.

It can be continuous/raster-independent.

It does not become multi-observation by resampling or by attaching another
camera label.

The Free World Observation Graph composes multiple separately sealed
TruthNegative states.

## Scientific RAW state

The D.RAW scientific state may carry signed scene-linear values, TruthRange
coordinates/bounds, authority, uncertainty, censor state, source footprints,
gauge identity and provenance.

It is a reconstructed scientific representation and may not impersonate the
original sensor RAW.

## Precision

- exact source integers/packed bytes remain exact evidence;
- Float64 is preferred for branch-sensitive compute;
- controlled Float32 canonical storage is allowed only where validated;
- precision never changes evidence authority.

## Permanent fail-closed rules

- no camera/lens automatically transfers calibration to another;
- no same numeric TruthRange value implies common radiometry without common
  gauge authority;
- no resampling creates measured target pixels;
- no virtual view creates physical evidence;
- no geometry authority upgrades radiometric authority;
- no appearance/restoration writes back into science;
- no multi-source fusion without explicit observation identities and declared
  relation authority.

## Seal

The exact v0.1 contract files are byte-sealed by the accompanying
`SEAL_MANIFEST_v0_1.json` and CI verifier.

Changing a sealed file requires a new versioned successor. Historical v0.1
bytes remain provenance.
