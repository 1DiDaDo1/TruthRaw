# Optical Field Topography v0.1

## Purpose

Optical Field Topography v0.1 converts the current measured-support field map into a diagnostic X/Y/Z representation.

The user observation behind the module is simple:

- a top-down map is good at showing **where** something occurs;
- a side or oblique view can make **height differences, ridges, basins and abrupt transitions** much easier to recognize.

D.RAW therefore allows the same field positions to be viewed with different observables used as Z-height.

Implementation:

`suite_android/app/src/main/java/com/truthraw/adaptiveui/OpticalFieldTopographyAuditV01.kt`

## Coordinate plane

Current coordinate plane:

`MEASURED_SUPPORT_CENTROID_UNIT_DISK`

X/Y come from the measured-support centroid of the corresponding source-field cell.

The underlying sparse source positions are measured source evidence.

The centroid is derived and remains non-authoritative as a new measurement.

## Height layers

Current selectable height semantics:

1. `RELATIVE_SIGNAL_EV`
2. `NOMINAL_MODEL_SIGNED_RESIDUAL_EV`
3. `NOMINAL_MODEL_ABS_RESIDUAL_EV`
4. `SUPPORT_POINT_COUNT`
5. `SUPPORT_CENTROID_OFFSET_NORMALIZED`
6. `SUPPORT_FOOTPRINT_RMS_SOURCE_PX`

These are independent diagnostic observables. They must not be collapsed into one physical meaning.

## View semantics

The export defines four view presets:

- `TOP_DOWN` — spatial field location;
- `SIDE_X` — height profile along field X;
- `SIDE_Y` — height profile along field Y;
- `OBLIQUE` — ridge/basin inspection.

These are render/view semantics only. They do not change the source evidence or scientific state.

An interactive/on-screen 3D renderer is **not implemented yet**. The current APK exports the topographic points, Z-layer fields and view presets in the Foundation JSON so the scientific data model can be validated before a visual renderer is added.

## Association diagnostics

For both TRAIN and HELD_OUT samples separately, the module reports descriptive Pearson association between nominal absolute model residual and:

- support-centroid displacement from the theoretical bin centre;
- support footprint RMS;
- support-point count.

These are descriptive diagnostics.

The following are permanently false:

- association is causation proof;
- correlation is a promotion threshold;
- a correlation automatically selects a problem cause.

## Critical physical interpretation rule

Topographic height is **not** automatically:

- physical scene depth;
- literal lens-surface sag;
- field curvature;
- geometric distortion;
- vignetting;
- optical aberration;
- sensor angular response;
- scene illumination.

A topographic peak means only that the selected observable is numerically high at that field location.

A basin means only that the selected observable is numerically low.

A ridge shared by multiple independent layers is useful diagnostic evidence for where to investigate next, but does not by itself prove a common physical cause.

## Lens-unfolding relation

This module preserves the user's earlier lens-unfolding idea in a scientifically conservative form.

The eventual target is not a cosmetic lens-correction filter. The stronger research direction is:

`source CFA sample -> field/ray coordinate -> local optical support -> free world coordinate`

before final reconstruction.

Current topography is an inspection layer on the path toward that richer coordinate model. It does not yet claim ray geometry or literal optical-surface curvature.

## Current status

Source implementation: **GREEN BUILD**

Source-code checkpoint:

`faac21e2e2fe97477c846609b8dc4e71af29b3c1`

Green Android artifact:

- run `36831352713`
- artifact `11146499757`
- APK SHA-256 `a892a9220c554f5ad655cfaffe23f580dbba37f8eba1ec9fe34f2fe9c00571d3`

Device topography export result: **PENDING**

The same four 0/90/180/270 controlled-rotation RAWs should be reused. No new capture is required for the first topography validation.
