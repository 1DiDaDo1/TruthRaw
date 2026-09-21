# TruthRaw v0.84.2 — ADVANCED Render/Edit Float32

Date: 2026-09-21  
Status: IMPLEMENTED / CI GREEN / REAL-LIGHTROOM ROUND-TRIP OPEN  
Branch: `integration/truthraw-suite-v0-84-2-adaptive-compute-router`

## Purpose

ADVANCED Render/Edit is the Lightroom-oriented developed Float32 master.

It is deliberately different from both:

- PURE Float32 Scientific Linear DNG; and
- JPG-L RAW/Edit, whose Float32 primary remains the scientific/linear image
  with Advanced settings carried as a non-destructive recipe.

Render/Edit instead makes a real developed Float32 derivative the primary
editable raster while preserving the Scientific Master as an immutable parent.

Correct model:

```
sealed Direct-CFA/source evidence
-> v4.7i reconstruction
-> camera-native Scientific Master parent
-> authorized camera->XYZ-D50
-> extended-linear sRGB derivative
-> optional Detail / Light / aesthetic Restoration
-> projected-raster SHA-256
-> deterministic replay
-> linear-sRGB -> XYZ-D50 Float32 storage projection
-> DNG primary raster
+ embedded non-authority JPEG preview
+ Scientific Master / Open Scene / output-authority lineage
```

The developed primary must never be renamed or promoted to Scientific Master.

## Why this route exists

The rejected JPG-L v0.1 design placed a normal 8-bit JPEG first and appended
Float32/TN-3 data after it. Lightroom could edit the JPEG front but did not use
the appended Float32 science as the editable source.

The current design makes a 32-bit Float DNG primary image visible to Lightroom
and keeps JPEG only as an embedded preview.

Render/Edit goes one step further than JPG-L RAW/Edit: it can bake selected
ADVANCED development into the Float32 primary while preserving extended-linear
headroom for further editing.

## Derivative identity domain

Canonical derivative identity is computed over:

`EXTENDED_LINEAR_SRGB_FLOAT32`

The DNG stores:

`XYZ_D50_LINEAR_FLOAT32`

The storage transform is:

`LINEAR_SRGB_TO_XYZ_D50`

The inverse matrix of the byte-frozen D50-XYZ -> linear-sRGB transform used by
the existing streaming path is:

```
0.43607472  0.38506492  0.14308038
0.22250448  0.71687860  0.06061692
0.01393217  0.09710452  0.71417328
```

The projected-raster identity is therefore over the developed linear-sRGB edit
master itself, not over its later XYZ-D50 storage projection.

## Float32 rules

The derivative route is before SDR finalization.

Hard rules:

- no negative clamp;
- no max-RGB-to-1 normalization;
- no SDR tone mapping;
- no sRGB OETF;
- no 8/10/16-bit quantization of the editable primary;
- negative components are retained;
- values above 1.0 are retained;
- output acutance is not applied.

A pixel with any negative component in the original extended-linear conversion
is restored exactly after the Detail stage and is not modified by Light or
Restoration.

## Development operations

### Detail

Uses the existing v4.7j Adaptive Detailed/Crisp implementation.

Its required halo is honored outside the canonical DNG tile. The developed
tile-source reconstructs enough source support so canonical 64x64 storage/hash
boundaries cannot become Detail boundaries.

### Light

Open-World Light uses the same current Advanced/full-resolution strength model:

- luminance dark gate;
- black protection;
- v0.1 strength ceiling 0.18;
- sealed pass-1 evidence confidence.

The exposure/evidence analysis is read-only and does not change the Scientific
Master or source evidence.

### Restoration

Restoration in Render/Edit is:

`AESTHETIC_REINTEGRATION_ONLY`

It may change the developed derivative where source support is censored, but it
does not create a scientific observation and cannot write back into Scientific
Master or output-channel authority.

### Natural HDR

Natural HDR is intentionally **not baked into the Render/Edit primary** in the
current v0.84 state.

The manifest records:

- `natural_hdr_baked_into_primary=0`;
- optional `natural_hdr_recipe_only=1`;
- `hdr_authority=APPEARANCE_ONLY_OUTPUT_CHANNEL_MAP_HAS_UNKNOWN`.

Reason: v0.84 provides a real per-output-channel authority map, but current
dense RGB output still contains UNKNOWN authority records. Scientific HDR
remains blocked.

### Output Acutance

v4.7k Output Acutance is excluded from Render/Edit.

It belongs after a real final resize/output choice. Applying it to an editable
master would incorrectly bind final-output sharpening to a still-resizable
working image.

## Projected-raster identity

The derivative is read twice.

Pass A:

```
source -> deterministic Render/Edit tile-source
       -> canonical 64x64 ScientificMasterDigestAccumulator grid
       -> projectedRasterSha256
```

Pass B:

```
same source -> same deterministic Render/Edit tile-source
            -> Float32 DNG writer
            -> digest same primary source again
            -> require actual digest == projectedRasterSha256
            -> only then commit artifact
```

For this derivative it is correct that:

- `projectedRasterIdentityVerified = true`;
- primary-raster `scientificMasterIdentityVerified = false`;
- the independently verified Scientific Master remains the immutable parent.

The existing v0.70/v0.84 lineage, zero-line, scene-scale and Technical Backplane
remain bound into the DNG private data.

## Tile-boundary correction

An early implementation tried to request expanded Detail/Restoration support
from `StreamingScientificMasterTileSource`. That adapter intentionally accepts
only exact canonical 64x64 tile requests and therefore was the wrong layer for
halo reconstruction.

The corrected implementation does not modify the sealed streaming module.

For each requested derivative tile it now:

1. computes Detail + Restoration support extent;
2. adds the canonical v4.7i reconstruction halo;
3. reads Stage-2 directly from the same admitted read-only source;
4. runs v4.7i over that expanded support region;
5. develops only the requested derivative region.

This keeps compute/storage tile boundaries out of image semantics.

## Automated equivalence gates

The acceleration CI now includes a synthetic Render/Edit partition test.

It requires:

1. one 128x64 developed request == two adjacent 64x64 requests bit-for-bit;
2. negative extended-linear pixels remain bit-identical when
   Detail/Light/Restoration are enabled;
3. repeated whole-frame projected-raster SHA-256 is identical;
4. HDR remains absent from the primary;
5. the derivative reports appearance only for operations actually baked.

The sealed tile-native streaming reference and exact v4.7i O0/O2 signature gate
continue to run before the Android APK build.

## Android UI/export

ADVANCED and PRO expose:

`Render/Edit · Float32 DNG · Lightroom`

Output identity:

- extension: `.dng`;
- MIME: `image/x-adobe-dng`;
- primary: Float32 derivative;
- embedded JPEG: non-authority preview;
- background execution: Android mediaProcessing foreground guard;
- live timer / green success / red unexpected-stop status retained.

## Current validation boundary

Implemented and build-tested:

- native derivative source;
- exact projected-raster replay gate;
- Android flavor decoding/post-write markers;
- Advanced/PRO UI action;
- background export worker;
- embedded preview;
- tile partition invariance;
- negative-headroom preservation test;
- strict-FP v4.7i reference gates.

Still requires real-device evidence:

- import generated Render/Edit DNG into Lightroom on Android 17;
- confirm Lightroom uses the Float32 primary rather than only the preview;
- exercise exposure/highlight/shadow edits;
- save/export and inspect resulting bit depth/color metadata;
- compare practical edit latitude against normal 8-bit JPG and JPG-L RAW/Edit;
- retain the original TruthRaw DNG unchanged for lineage verification.

No real-device Lightroom result may be promoted into the scientific contract
until that test is performed.
