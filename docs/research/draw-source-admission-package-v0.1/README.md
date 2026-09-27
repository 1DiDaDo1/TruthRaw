# D.RAW Source Admission Package v0.1

Status: **MAIN-INTEGRATED HOST-VALIDATED GENERIC SOURCE ADMISSION GATE**

This module defines the lens-independent entrance for a genuinely new physical
source into the current D.RAW Free World.

It separates **placement before scientific admission** from **final
source-local scientific admission**.

## Phase A — Pre-Admission Manifest

A Pre-Admission Manifest can be created as soon as real source evidence and
basic source facts are known.

It binds sealed source SHA-256, device/camera/lens role, source route, sample
domain, raster/CFA topology and explicit readout/pixel-mode identity only where
those facts are actually established.

The generator never invents scientific capability. Before reconstruction the
following begin as `UNKNOWN`:

- geometry/pose;
- radiometry;
- colorimetry;
- spectral;
- optical support;
- noise/uncertainty;
- temporal.

Sampling geometry and source provenance may be source-bound because those are
the facts supplied to the manifest.

A Pre-Admission Manifest is not a Scientific Master, D.RAWnegative,
calibration admission, graph relation or fusion permission.

## Phase B — Final Source-Local Admission

Final admission happens only after the existing route has produced:

1. a valid `D.RAW/DRAWObservationRecord/0.3`;
2. a valid `D.RAW/SourceCapabilityEnvelope/0.1`.

The Final Admission validator executes both existing validators first. It then
binds the same Observation ID, sealed source, instrument, graph node,
source-local Zero-Line gauge and one-frame/one-evidence lineage.

The initial status is:

`ADMITTED_SOURCE_LOCAL`

No shared Free World gauge, graph relation or fusion admission is allowed
inside this package.

## Practical workflow

Create the source placement with:

`tools/create_draw_source_pre_admission_v01.py`

After the scientific route has produced Record v0.3 and Capability Envelope
v0.1, create the final package with:

`tools/create_draw_source_final_admission_v01.py`

and validate it with:

`tools/validate_draw_source_admission_package_v01.py`.

## Why two phases

Before reconstruction we can know that a source exists and where its known and
unknown capabilities belong. We cannot yet truthfully claim its Scientific
Master, D.RAWnegative or calibration authority.

After reconstruction, final admission binds the produced scientific identities
without rewriting the earlier source.

## Lens-independent law

All sources use the same gate:

```text
real source evidence
 -> Pre-Admission placement
 -> source-local scientific route
 -> Observation Record v0.3
 -> Source Capability Envelope v0.1
 -> Final Source-Local Admission
 -> Free World Observation Graph
```

Only after that may independent relation campaigns attempt geometry,
radiometric, colorimetric/spectral, optical, uncertainty/correlation, temporal
or provenance relations.

CI uses Camera-5 only as an already-admitted proof fixture. This module does
not claim that main or ultra-wide are already admitted.
