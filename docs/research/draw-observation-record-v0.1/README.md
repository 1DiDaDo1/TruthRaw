# D.RAW Observation Record v0.1

Status: **FIRST EXECUTABLE DERIVATIVE OF THE SEALED LENS-INDEPENDENT ARCHITECTURE**

This module does not change any pixel route.

It implements the first concrete machine-level object implied by:

- `docs/CORE_VISION_LENS_INDEPENDENT_FREE_WORLD_OBSERVATION_ARCHITECTURE.md`;
- `D.RAW/LensIndependentFreeWorldObservationContract/0.1`.

A `DRAWObservationRecord/0.1` describes exactly one admitted physical
observation lineage.

## Purpose

The record moves camera/lens/sensor specifics out of the Free World itself.

A source-specific adapter may populate:

- Source Evidence identity;
- acquisition/procedure identity;
- source topology;
- capture provenance;
- Source Capability Envelope;
- Zero-Line/gauge binding;
- Scientific Master identity;
- TruthNegative identity;
- authority/uncertainty boundaries;
- evidence counts;
- immutable invariants.

The Free World consumes this common record rather than special-casing tele,
main, ultra-wide or a particular manufacturer.

## Fail-closed rules

A valid record must satisfy:

- one observation ID;
- one sealed Source Evidence SHA-256;
- one physical frame for this v0.1 single-frame record;
- one independent evidence item under the existing single-frame contract;
- Scientific Master and TruthNegative identities present;
- TruthNegative is raster-independent;
- target raster does not define TruthNegative state identity;
- resampling creates no measured target claims;
- geometry and radiometric authority remain separate;
- absent common gauge means cross-observation radiometric equality/fusion is
  forbidden;
- UNKNOWN optics cannot be used scientifically;
- precision cannot upgrade authority;
- no appearance or scientific writeback.

## Camera-5 example

The example record is deliberately conservative and binds the real lamp-scene
source used by the v0.2 headroom audit.

It proves only what the existing project state supports.

In particular:

- Camera-5 / tele identity is provenance, not a world type;
- the source raster is 4080x3072;
- the gauge remains source-local;
- shared Free-World radiometric gauge is UNPROVEN;
- optics scientific use remains blocked;
- TruthNegative remains one observation lineage.

The example is not calibration for main or ultra-wide.

## Next implementation step

Create separate observation records for admitted main and ultra-wide
RAW/DNG captures.

Only after an explicit relation/calibration contract may records share a
Free-World gauge or participate in radiometric fusion.
