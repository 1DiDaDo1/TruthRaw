# D.RAW next-chat handoff — 2026-09-27

## Start here

Official product name: **D.RAW**

Repository: `1DiDaDo1/TruthRaw`

Primary current repository line: `main`.

The stale-main gap is closed. The latest substantive 2026-09-27 integration
checkpoint before this documentation refresh is:

`b0e88971a6fe3ed84ec6913865e50073d16512cf`

The current integration-provenance branch for this documentation refresh is:

`integration/draw-knowledge-growth-state-refresh-2026-09-27`

A new chat should normally start on `main`; it does not need to reconstruct
the old branch ladder first.

## Mandatory current reading order

1. `START_HERE_NEW_CHAT.md`
2. `state/CURRENT_PROJECT_STATE_2026-09-27.json`
3. `docs/DRAW_KNOWLEDGE_GROWTH_INTEGRATION_2026-09-27.md`
4. `docs/DOCUMENT_STATUS_INDEX_2026-09-27.md`
5. `docs/research/free-world-observation-graph-v0.1/README.md`
6. `docs/research/draw-observation-record-v0.3/README.md`
7. `docs/research/draw-observation-record-v0.3/examples/CAMERA5_LAMP_SCENE_OBSERVATION_v0_3.json`
8. `docs/research/draw-calibration-binding-v0.1/README.md`
9. `docs/research/draw-calibration-binding-v0.1/DRAW_CALIBRATION_BINDING_CONTRACT_v0_1.json`
10. `docs/research/drawnegative-v0.1/SEAL_MANIFEST_v0_1.json`
11. `docs/CORE_VISION_LENS_INDEPENDENT_FREE_WORLD_OBSERVATION_ARCHITECTURE_v0_2_DRAWNEGATIVE.md`

## Permanent laws

- **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
- **Measured where measured. Reconstructed where necessary. Never invented.**
- **One Free World. Many sealed observations. One evidence law.**
- **Evidence stays what it was. Knowledge can grow through admitted relations.**

## Current architecture

```text
sealed physical observation / Source Evidence
 -> D.RAW Observation Contract
 -> Source Capability Envelope
 -> Float64 measurement/calibration/reconstruction
 -> Scientific Master
 -> validated scientific storage
 -> legacy TruthNegative Continuous parent
 -> D.RAWnegative per observation
 -> Free World Observation Graph
 -> Deep Scene / Light Transport
 -> View / Appearance
 -> finite projection
```

D.RAWnegative v0.1 remains byte/semantics sealed and unchanged.

## Current scientific-negative / Android boundary

D.RAWnegative v0.1 remains the current scientific-negative identity.

Latest fully host + Android validated D.RAWnegative checkpoint remains:

`3e150afeb36cbb68fd318b0143a315e64d5a637f`

Android/NDK/JNI run `36260305613`: **SUCCESS**.

The three 2026-09-27 knowledge-growth modules below are host/control-plane
contracts. They do not change the validated image/pixel route and are not being
misrepresented as new Android/device validation.

## Free World Observation Graph v0.1

Status: **main-integrated, host/native validated**.

Merge commit:

`ee4c426e403ac87f8db86dc760aa1b2c975ff3f5`

Native GCC/Clang/ASan/UBSan workflow run:

`36282087425` — **SUCCESS**

The graph gives every information item an explicit domain/floor and factors
cross-observation relations into independent axes:

- geometry;
- radiometric / Zero-Line gauge;
- colorimetric;
- spectral;
- optical support;
- uncertainty / correlation;
- temporal;
- provenance.

A relation on one axis grants nothing on another axis.

A single relation may not directly grant fusion. Composite fusion admission is
separate and certificate-bound.

Current minimum relation sets are:

- radiometric fusion = radiometric gauge + uncertainty/correlation + temporal;
- colour fusion = radiometric gauge + colorimetric + uncertainty/correlation + temporal;
- spatial-detail fusion = geometry + optical support + uncertainty/correlation + temporal.

This stricter downstream rule does not rewrite sealed D.RAWnegative v0.1.

## Observation Record v0.3

Status: **current Observation Record successor, host validated**.

Merge commit:

`472194decc1418906c9f99e70b37098fb6276912`

Validation run:

`36282295104` — **SUCCESS**

v0.3 is a strict successor to v0.2. Its validator reconstructs the parent v0.2
record and runs the complete v0.2 gate before accepting any v0.3 additions.

It adds explicit knowledge placement and independent authority dimensions for:

- sampling geometry;
- geometry/pose;
- radiometry;
- colorimetry;
- spectral;
- optical support;
- noise/uncertainty;
- temporal;
- provenance.

It also adds a temporal footprint, optical-support state and calibration
bindings.

The Camera-5 migration keeps the same source, Scientific Master, authority
field, legacy TruthNegative parent and D.RAWnegative v0.1 identities.

Camera-5 currently remains explicitly:

- spectral: `UNKNOWN`;
- optical support: `UNKNOWN_FOR_SCIENTIFIC_SUPPORT`;
- temporal: `UNKNOWN`;
- calibration bindings: none.

Its graph-node SHA-256 is:

`901e5b8e853792fccee50ec6501fd22981593b7f9a3e5d8fb82a19be3f9bed62`

No cross-observation relation or fusion permission lives in an Observation
Record.

## Calibration Observation Binding v0.1

Status: **main-integrated host-validated research foundation**.

Merge commit:

`b0e88971a6fe3ed84ec6913865e50073d16512cf`

Validation run:

`36282461281` — **SUCCESS**

Documentation governance for the same PR:

`36282461248` — **SUCCESS**

This module does not create calibration values. It reuses the existing
Open-World Calibration Registry and gives calibration applicability
deterministic identities for:

- validity domain;
- calibration record + calibration-payload hash;
- held-out validation report;
- target Observation capture context;
- final binding/admission decision.

D.RAW adds explicit:

- `source_route_id`;
- `sample_domain_id`;
- `readout_domain_id`.

This prevents a calibration from moving between superficially similar source
domains, lens routes or readout modes merely because dimensions/CFA/metadata
look alike.

An admitted calibration binding still:

- creates no target-observation sensor evidence;
- adds no physical frame;
- adds no independent target-observation evidence count;
- grants no cross-observation relation;
- implies no calibration transfer;
- grants no fusion.

No real Camera-5 calibration was promoted by this module; its validation uses
synthetic calibration fixtures.

## Source Capability Envelope v0.1

Status: **main-integrated, host validated**.

- merge: `56464d3b683dcc347dd340f14e1451ed696c56ce`
- run: `36282899524` — **SUCCESS**

This is now the machine-strict knowledge map for one source. Every admitted
source must place sampling geometry, geometry/pose, radiometry, colorimetry,
spectral, optical support, noise/uncertainty, temporal and provenance.

`UNKNOWN` is a valid explicit state. Missing placement is not.

The current Camera-5 envelope deliberately keeps sensor pixel mode,
readout-domain identity, spectral support, scientific optical support and
temporal support unknown where no evidence establishes them.

## Source Admission Package v0.1

Status: **main-integrated generic source gate, host validated**.

- merge: `2ae5f42428137e5b870622fbd90417f0eb87a97c`
- run: `36283458445` — **SUCCESS**

The admission gate has two phases:

1. `CANDIDATE_NOT_SCIENTIFICALLY_ADMITTED` — a pre-admission manifest built
   only from sealed source evidence and real source facts; all unproven
   scientific capabilities remain UNKNOWN.
2. `ADMITTED_SOURCE_LOCAL` — only after Observation Record v0.3 and Source
   Capability Envelope v0.1 both validate and bind to the same source.

Initial admission forbids shared Free World gauge, graph relations and fusion.

The generic gate is ready for main and ultra-wide, but neither source is
scientifically admitted yet. Their real sealed source evidence still has to be
supplied and passed through this gate.

## Meaning of knowledge growth

New information never becomes "more measured" merely because it fits the
world model better.

The positive-growth rule is:

```text
immutable evidence
 + admitted calibration/context
 + admitted independent relations
 = richer usable knowledge
```

while original provenance and authority remain intact.

Relation knowledge can progress through:

`UNKNOWN -> HYPOTHESIS -> SOURCE_BOUND -> CALIBRATED -> ADMITTED`

and `REJECTED` remains preserved negative knowledge.

## Historical research recovered into main

The current main ancestry now preserves/reuses:

- Virtual Observation Manifold v0.9;
- FotoGraaf Acquisition Domain v0.8;
- Professional RAW Gatehouse / LibRaw / decoder / ingress unique modules;
- Open World foundations;
- camera-RGB covariance and XYZ uncertainty;
- Bound Uncertainty Admission;
- Free World output-pixel work;
- TruthNegative / D.RAWnegative;
- current Android/JNI scientific route.

Historical branch names remain useful provenance, but are no longer required
as the first navigation step.

## Current empirical frontier

Camera-5/tele remains the first real D.RAW Observation anchor.

Do **not** borrow its calibration to main or ultra-wide.

The next scientifically useful growth is physical, not another generic
architecture rewrite:

1. create a real main-camera Observation Record v0.3 + Source Capability Envelope;
2. create a real ultra-wide Observation Record v0.3 + Source Capability Envelope;
3. keep both source-local;
4. run source/readout-specific calibration campaigns with held-out validation;
5. measure geometry, radiometric, colorimetric/spectral, optical,
   uncertainty/correlation and temporal relations independently;
6. admit only passed relations;
7. allow composite fusion only when every required relation certificate exists.

No Honor/GCam/computational render may determine D.RAW scientific truth or
calibration.
