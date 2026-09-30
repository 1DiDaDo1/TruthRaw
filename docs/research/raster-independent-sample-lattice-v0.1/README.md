# D.RAW Raster-Independent Sample Lattice v0.1

Date: 2026-09-29

Status: active research successor stacked above Dark Chroma v0.6.

## Core law

> The source raster determines where D.RAW measured. It does not determine the raster on which D.RAW must think.

Equivalent Dutch project law:

> Het bronraster bepaalt waar D.RAW heeft gemeten. Het bepaalt niet op welk raster D.RAW moet denken.

This module implements that distinction without modifying or replacing the sealed source.

## What this is

The lattice is a **sparse implicit fixed-point coordinate world**.

Every admitted source sample keeps its exact source coordinate and is mapped to:

```text
u = x * 2^20
v = y * 2^20
```

with:

```text
coordinate_units_per_source_pixel = 1,048,576
coordinate_fraction_bits = 20
```

The very fine coordinate scale is chosen for deterministic geometry and future sub-source-pixel relations. It is **not** a claim of 1,048,576× sensor resolution.

No dense micro-image is allocated.

## Evidence law

For a DNG CFA source:

- every source CFA location remains a measured anchor;
- its sealed source value is unchanged;
- its source position is unchanged;
- no source sample is deleted;
- no source sample is averaged merely to enter the lattice;
- positions between source anchors exist as coordinates only;
- those unanchored positions begin with authority `UNKNOWN`;
- they do not contain an interpolated pixel value;
- they may receive a value only from a later explicit versioned reconstruction with provenance and uncertainty.

Therefore:

```text
SOURCE SAMPLE != LATTICE POSITION WITHOUT ANCHOR
MEASURED != RECONSTRUCTED != APPEARANCE
```

## It is not upscaling

Classical upscaling commonly means:

```text
finite raster -> estimate additional output pixels -> larger finite raster
```

The D.RAW lattice is:

```text
sealed measurement samples
 -> exact anchor coordinates
 -> sparse raster-independent coordinate domain
 -> optional later scientific reconstruction
 -> independently chosen finite projection
```

The lattice itself has no required output resolution.

Hard statements:

- `upscaling_performed=false`
- `source_values_interpolated=false`
- `dense_lattice_materialized=false`
- `new_sensor_measurements_created=false`
- `optical_resolution_increased=false`

## Why fixed-point rather than an enormous dense raster

The project could conceptually choose a 4×, 16×, 1024× or larger internal grid. Materializing all cells would waste memory and would tempt later code to treat empty cells as image pixels.

Instead v0.1 uses 20-bit fixed-point source coordinates:

- source anchors are exact;
- half-pixel and far finer future geometry is exactly representable;
- memory use does not scale with an imagined dense micro-raster;
- the coordinate domain can be much finer than the source raster while knowledge authority remains unchanged.

The lattice therefore gives the freedom of an extremely fine raster without pretending that all of its positions are observed pixels.

## Current project integration

Universal Intake now creates:

`D.RAW/RasterIndependentSampleLattice/0.1`

for a parsed raster source.

The profile records:

- source SHA-256;
- source raster dimensions;
- source class;
- CFA pattern where available;
- measured anchor count for admitted CFA raster layout;
- fixed-point coordinate scale;
- explicit UNKNOWN status of unanchored positions;
- no-upscale/no-interpolation/no-new-evidence invariants.

The Universal Intake UI exposes this as:

`D.RAW Sample Lattice v0.1 · RASTER-INDEPENDENT`

## N2 / Dark Chroma integration

The exact sampled N2 support geometry from v0.6 is projected into:

`D.RAW/N2RasterIndependentSampleGeometry/0.1`

For every v0.6 candidate query it preserves:

- exact source rectangle;
- exact source support sample coordinates;
- equivalent lattice rectangle;
- equivalent lattice center;
- nearest Structure/Censored/CensorBoundary support source coordinate;
- equivalent exact lattice coordinate;
- source-pixel distance;
- lattice-unit distance;
- radius support fractions.

Nothing is reclassified by the coordinate transform.

Dark Chroma v0.7 records the binding as:

`D.RAW/Frontside/DarkChromaStability/0.7`

v0.7 is still audit-only.

## Noise interpretation

A finer coordinate lattice does **not** itself remove photon shot noise or create missing measurements.

Its scientific value is that future estimation no longer has to ask:

> which source pixel owns this effect?

It can instead ask:

> which measured anchors constrain this location, over what support, with which uncertainty and authority?

That is potentially useful for:

- CFA reconstruction;
- noise estimation;
- local structure preservation;
- optics footprint reasoning;
- subpixel geometric relations;
- future calibrated observation fusion;
- finite output projections at arbitrary resolution.

But v0.1 does not yet infer values into unanchored coordinates.

## Current safety boundary

Still hard false:

- noise correction enabled;
- chroma correction supported;
- private chroma A/B/Delta;
- distance threshold admitted;
- source writeback;
- Scientific-Master writeback;
- creation of new sensor evidence;
- inference that coordinate precision equals optical resolution.

## Single-observation law

The current Dark Chroma/N2 path remains:

```text
1 sealed selected observation
 -> many deterministic analyses
 -> one sample-coordinate world
 -> constraints / authority
 -> one D.RAWnegative context
```

No other lens, burst, temporal frame, hidden fusion or AI/ML is introduced.

## Next research gate

The next scientific gate is **not** "fill every lattice position".

It is:

`ANCHOR_CONSTRAINED_LOCAL_RECONSTRUCTION_AUDIT`

A future successor may study a private local reconstruction candidate in the lattice only if it:

1. keeps all source anchors unchanged;
2. leaves unmeasured positions explicitly reconstructed rather than measured;
3. carries local uncertainty;
4. respects exact structure/censor support;
5. is compared against the unchanged Scientific Master path;
6. remains audit-only until device evidence justifies promotion.
