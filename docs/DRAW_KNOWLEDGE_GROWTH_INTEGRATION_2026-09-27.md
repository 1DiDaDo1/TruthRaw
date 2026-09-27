# D.RAW knowledge-growth integration — 2026-09-27

Status: **CURRENT INTEGRATION DIRECTION**

## Purpose

This document captures the project-wide synthesis after rereading the active
code, the historical branch families and relevant external theory from
photography, camera/lens calibration, stop-motion/animation, architecture and
light transport, human visual perception, restoration/conservation and sensor
metrology.

It exists so a later chat does not have to rediscover which historical
research remained useful.

## Core interpretation

Lens-independent input means the Free World is not defined by any one lens.
Lens identity is preserved as observation provenance rather than erased.

Every admitted piece of information must know its place before it can
participate in the world model. Knowledge grows by adding verified context and
relations, not by changing the historical authority of the information.

Canonical growth rule:

> **Evidence stays what it was. Knowledge can grow through admitted relations.**

## Current hierarchy

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

## Relation families

The graph must keep at least these relation families separate:

1. geometry;
2. radiometric / Zero-Line gauge;
3. colorimetric;
4. spectral;
5. optical support;
6. uncertainty / correlation;
7. temporal compatibility;
8. provenance / ancestry.

One admitted relation never silently grants another.

## Findings now made explicit

### Photography and lenses

Pixel count is not optical resolution. Future optical support should be
field-, spatial-frequency-, focus- and where justified channel/wavelength-aware.
PSF/OTF/MTF evidence belongs upstream of appearance and downstream of immutable
source evidence.

### Calibration and metrology

A calibration belongs to a validity domain. Device, physical route, readout
mode, geometry, gain/exposure region, temperature, focus, source identity,
method, uncertainty and held-out validation may all constrain applicability.
Metadata similarity never transfers calibration by itself.

### Stop-motion / animation / time

A physical exposure may have a temporal footprint rather than one ideal
instant. Exposure interval, shutter weighting and rolling/global readout can
eventually become Observation properties.

A virtual EV/ISO/view is not a second physical observation and adds zero
independent evidence. A separately captured frame is a new sealed observation.

### Architecture and light transport

Geometry, material, illumination, visibility and transport remain separate
state families. A computed reflection, hidden light path or relight cannot be
written back as measured sensor evidence.

### Human vision

Viewing condition, adaptation, surround, display and future
spatial-frequency/eccentricity-aware appearance belong downstream. Human
perception can guide the final projection without upgrading scene authority.

### Restoration

Intervention remains provenance-visible even when visually seamless.
Reconstructed, restoration-hypothesis and appearance contributions never
become measured by presentation quality.

## Historical branches preserved/integrated

The current integration line now preserves:

- the full current D.RAW code-bearing ancestry promoted from the old stale
  default main;
- Virtual Observation Manifold v0.9 history;
- FotoGraaf Acquisition Domain v0.8 history;
- unique Professional RAW Gatehouse / decoder / LibRaw ingress modules,
  copied from their validated historical branch without restoring obsolete
  house documentation.

Existing ancestors already include Open World foundations, covariance/XYZ
uncertainty, Bound Uncertainty Admission, Free World output pixel work,
TruthNegative/D.RAWnegative and the current Android/JNI pipeline.

## Versioning boundary

D.RAWnegative v0.1 and the sealed lens-independent v0.1 architecture are not
edited.

Free World Observation Graph v0.1 is a new downstream integration layer.
Future semantic changes to sealed objects require explicit versioned
successors.

## Executable knowledge-growth foundations now integrated

The 2026-09-27 architecture is no longer documentation-only.

### Free World Observation Graph v0.1

A native C++ graph core now provides deterministic identities and fail-closed
guards for Observation nodes, information placement, independent relation axes
and composite fusion admission.

Merge commit: `ee4c426e403ac87f8db86dc760aa1b2c975ff3f5`

Native validation run: `36282087425` — **SUCCESS** on GCC, Clang and
ASan/UBSan.

The graph performs no image processing and has no scientific writeback path.

### D.RAW Observation Record v0.3

v0.3 is now the current Observation Record successor.

Merge commit: `472194decc1418906c9f99e70b37098fb6276912`

Validation run: `36282295104` — **SUCCESS**.

It retains the complete v0.2 parent gate while adding explicit knowledge
placement, independent authority dimensions, temporal footprint, optical
support state and calibration-binding slots.

The migrated Camera-5 observation intentionally leaves spectral, optical and
temporal claims unknown where no new evidence was admitted.

### Calibration Observation Binding v0.1

The existing Open-World Calibration Registry now has a deterministic
observation-binding layer.

Merge commit: `b0e88971a6fe3ed84ec6913865e50073d16512cf`

Validation run: `36282461281` — **SUCCESS**.

The binding identity covers the validity domain, calibration payload, held-out
validation report and exact target capture context. D.RAW additionally binds
source-route, sample-domain and readout-domain identities so superficially
similar rasters cannot silently share calibration.

This layer creates no calibration values and no real Camera-5 calibration has
been promoted by it.

### Validation-class separation

These three additions are host/control-plane contracts and do not change the
scientific pixel route. Their successful host validation does not replace or
upgrade the existing Android/device proof for D.RAWnegative.

Latest fully host + Android validated D.RAWnegative checkpoint remains
`3e150afeb36cbb68fd318b0143a315e64d5a637f`, Android run
`36260305613` — **SUCCESS**.

## Real main-camera physical admission completed

The project has now exercised the lens-independent architecture against a real
physical Camera-2/main source rather than only a generic contract fixture.

The immutable source is anchored by SHA-256
`a85cac58601d6cc8138bf8f9372e5161b85ab1e89afa70372f97c46461dcea79`.

A malformed TIFF Orientation value 9 blocked strict DNG ingress. The source was
not changed. Orientation Quarantine v0.1 created a one-byte compatibility
container (`09 -> 01`) with byte-identical serialized CFA payload and no
presentation/world orientation claim.

The unchanged common scientific C++ pipeline then produced:

- Scientific Master `c26939ef…43202`;
- authority field `e18f0038…d6b2a`;
- TruthNegative Continuous `cd3acbd9…2a60c`;
- D.RAWnegative v0.1 `87955cae…0ccf7`.

The compatibility ingress exposed an important identity distinction. The
physical observation is now anchored to the immutable parent source, while the
D.RAWnegative v0.1 Observation ID remains a computational pipeline-lineage ID.

Observation Record v0.4 formalizes that distinction and Free World relations
must use the stable physical Observation ID.

Source Capability Envelope v0.2 then binds knowledge to the physical source
while keeping compatibility ingress, capture-sample domain, readout domain and
sensor pixel mode separate.

Source Admission Package v0.2 finally admits main as:

`ADMITTED_SOURCE_LOCAL`

with zero calibration bindings, zero graph relations and zero fusion
admissions.

This is a direct implementation of the project rule:

> **Evidence stays what it was. Knowledge can grow through admitted relations.**

The source is more useful now because its lineage and scientific state are
better connected. Its historical authority was not rewritten.

## Current empirical growth

Main admission is complete; do not repeat it.

Next:

1. locate and seal a real ultra-wide / physical-Camera-4 source;
2. pre-admit only proven source facts;
3. run the unchanged common scientific route;
4. preserve any compatibility ingress as lineage, not new evidence;
5. create the ultra-wide physical Observation Record and Capability Envelope;
6. admit ultra-wide source-local only after its own gates pass;
7. then measure cross-observation relation axes independently.

No main↔tele↔ultra-wide calibration, gauge, color, optical support or
uncertainty relationship is inferred merely from sharing one handset.
