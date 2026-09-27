# D.RAW Observation Record v0.3 — Knowledge Placement

Status: **CURRENT OBSERVATION-RECORD SUCCESSOR — HOST VALIDATED**

v0.2 remains valid historical/executable provenance. v0.3 is a strict
knowledge-placement successor: it preserves the exact source, Scientific
Master, legacy TruthNegative parent and D.RAWnegative v0.1 identities while
making the observation's knowledge domains explicit.

Permanent rule:

> **Evidence stays what it was. Knowledge can grow through admitted relations.**

## What v0.3 adds

The record now carries:

- a deterministic Free World Observation Graph node identity;
- explicit per-domain knowledge placement;
- independent authority dimensions;
- a temporal footprint;
- explicit optical-support state;
- calibration bindings with validity-domain and hold-out identities;
- fail-closed knowledge-growth invariants.

The parent v0.2 record is reconstructed in the validator and must still pass
the complete v0.2 validator. v0.3 therefore cannot loosen the existing
D.RAWnegative/gauge/evidence-count rules.

## Authority dimensions

The observation records these dimensions independently:

- sampling geometry;
- geometry / pose;
- radiometry;
- colorimetry;
- spectral;
- optical support;
- noise / uncertainty;
- temporal;
- provenance.

They may have different authority. Lens or device identity never transfers one
dimension into another.

Cross-observation admission does not live in an Observation Record. Every
`cross_observation_relation_admitted` flag in v0.3 is therefore fixed false.
Relations and composite fusion belong to Free World Observation Graph v0.1.

## Temporal footprint

A photograph is not assumed to be an infinitesimal time sample. The record can
later bind a capture timestamp, exposure interval and rolling/global readout
model. Unknown timing stays explicitly UNKNOWN.

The migrated Camera-5 lamp scene has no newly admitted temporal measurement in
this successor, therefore it remains `UNKNOWN`.

## Optical support

Raster/sample count remains separate from optical detail support. A future
PSF/OTF/MTF/SFR calibration can be attached only with its own identity and
validity domain.

The current Camera-5 lamp scene remains:

`UNKNOWN_FOR_SCIENTIFIC_SUPPORT`

with `model_kind=NONE`.

## Calibration bindings

A calibration binding requires:

- calibration-record SHA-256;
- validity-domain SHA-256;
- held-out validation report SHA-256;
- applicability = true;
- admitted = true.

The Camera-5 migration intentionally has an empty calibration-binding list.
Existing source-bound radiometric/color metadata is not reclassified as a new
independent calibration merely to fill the new structure.

## Migration invariant

The migrated Camera-5 record keeps exactly:

- source SHA-256 `45d44d…c2ba`;
- Scientific Master SHA-256 `01f22f…8fd8`;
- authority field SHA-256 `5ad5f4…1f38`;
- legacy TruthNegative parent `73c908…02c5`;
- D.RAWnegative v0.1 state `7fabfd…e9a5`;
- one physical frame;
- one independent evidence item;
- source-local Zero-Line gauge;
- no cross-observation fusion.

No new sensor evidence is created by the migration.
