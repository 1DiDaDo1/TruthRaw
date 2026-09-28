# D.RAW Core Vision Realignment — 2026-09-28

Status: **CURRENT ARCHITECTURAL DIRECTION**

## Core statement

D.RAW starts from an immutable RAW/DNG/photographic source, but the sealed source is the evidence foundation — not the boundary of the new world.

The primary flow is:

```text
source at the door
  -> seal original evidence
  -> universal source understanding
       -> BACK SIDE: bytes / container / metadata / CFA / calibration hints
       -> FRONT SIDE: visible photograph / proportions / light / colour / structure
  -> scientific measurement and reconstruction
  -> Scientific Master
  -> open D.RAW scene/state
  -> Float64 compute where numerically necessary
  -> controlled Float32 scientific storage / RAW representation
  -> free reconstruction / restoration / appearance
  -> outputs such as Float32 RAW/DNG and JPEG
```

Backside and frontside are two knowledge views of the same source. They do not have equal authority.

## Universal means format-neutral, not evidence-blind

D.RAW should not require a fixed phone, lens or camera map before a source can enter.

An unknown RAW format may require a versioned decoder adapter, but the source itself is still accepted and sealed. Format recognition, decode certification and scientific admission remain separate questions.

UNKNOWN is a valid knowledge state, not a failure of the Free World.

## Permanent no-AI rule

D.RAW does not use AI, machine-learning, neural, generative or learned inference models in its runtime or scientific/appearance pipeline.

Allowed methods include transparent deterministic algorithms such as:

- RAW/container parsing;
- classical image processing;
- feature geometry and correspondence;
- photogrammetry and multi-view geometry;
- numerical optimization;
- optical/PSF/MTF/SFR analysis;
- colorimetry and appearance mathematics;
- calibrated noise/uncertainty modelling;
- deterministic restoration and reconstruction with explicit provenance.

Historical research text may mention learned systems as comparison/background. That does not authorize them in D.RAW.

## Frontside intake

Frontside inspection belongs at the entrance, not only at the final appearance stage.

The first integrated full-suite implementation is deliberately conservative and deterministic. It can inspect visible:

- dimensions and aspect ratio;
- luminance distribution and approximate percentiles;
- shadows/highlights in the rendered preview;
- visible RGB/chroma statistics;
- edge density and gradient structure;
- dominant edge orientation;
- structural bounding box and centroid;
- a deterministic structural signature;
- readiness for later classical feature geometry.

All such information is typed `APPEARANCE_DERIVED_ONLY` unless another independent scientific path upgrades a specific fact. It cannot write back into source evidence or silently become a sensor measurement.

## Precision and the Float32 RAW direction

D.RAW keeps stage-specific precision.

The intended scientific direction remains:

```text
exact integer/packed source evidence
  -> Float64 where reconstruction/calibration/optimization/covariance is branch-sensitive
  -> controlled Float32 Scientific-Master / RAW-state storage
  -> downstream views and exports
```

The existing full suite still contains the mixed F64/F32 Scientific Master reconstruction and Float32 LinearRaw DNG routes. The longer-term container-independent D.RAW scientific RAW/state remains an open goal rather than being replaced by JPEG.

## Open-world law

> **Seal the evidence, not the thinking.**

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Provenance, authority, uncertainty, admission and relation certificates are guardrails around claims. They do not define the maximum size, precision, dimensionality or richness of the reconstructed D.RAW world.

## Universal physical camera

The canonical Camera entry now uses `UniversalPhysicalCaptureActivity`.

Its role is deliberately narrow:

```text
physical camera
  -> Android Camera2 transport only
  -> one standard RAW_SENSOR capture
  -> exact app-visible RAW_SENSOR plane seal (.rawsensor + SHA-256)
  -> acquisition/layout provenance
  -> derived DNG compatibility container
  -> the same Universal Source Intake used by imported RAW/DNG
```

Ultra-wide, wide/main and normal tele are discovered dynamically from available back-facing RAW routes. Focal-length ordering is only `UI_FOCAL_ORDER_HINT_ONLY`; fixed device maps, fixed Camera IDs and vendor request keys are not required and cannot create scientific authority.

For the normal universal route, resolution is selected only from the ordinary `SCALER_STREAM_CONFIGURATION_MAP` RAW_SENSOR sizes. D.RAW chooses the highest standard RAW_SENSOR resolution exposed by that physical camera. It does not silently enter a vendor maximum-resolution mode.

The app-visible `.rawsensor` byte stream is the source-first capture foundation. Camera2 characteristics and capture results remain acquisition/interpretation provenance; they are not promoted to untouched photodiode/ADC proof. The DNG is created afterwards and is explicitly a derived compatibility container for the current main-house scientific ingress.

The existing special Camera-5 4K→200MP route remains separate. Its maximum-resolution/envelope logic is never generalized to ultra-wide, main or the normal tele route.

### Source resolution versus Open-World resolution

The measured source grid, optical resolving support, reconstruction field and chosen output raster are different quantities.

```text
highest standard measured RAW_SENSOR grid
  -> Scientific Master
  -> D.RAWnegative / Open World
  -> freely selected finite output raster
```

Open-World output resolution may be lower, equal to or higher than the source sample grid. Choosing a denser output raster does not create additional measured samples and never upgrades source or optical authority.

PR #76 merged this architecture into `main`.

Merge commit:

`1fa3850e4ec0c58f7dc294acd011721465041ea9`

## Canonical Android product

The canonical user-facing Android application is `suite_android`.

It preserves:

- the D.RAW icon and opening banner;
- D.RAW PURE;
- D.RAW ADVANCED;
- D.RAW PRO;
- file/RAW input;
- camera input;
- Scientific Master / Float32 routes;
- D.RAWnegative/Open Scene research paths;
- JPEG as downstream compatibility/output.

`capture/android/draw-geometry-capture-v01` remains a useful historical/test capture assistant. It is not the replacement user interface for D.RAW.

## First integration milestone

PR #74 merged Universal Intake directly into the full D.RAW suite.

Merge commit:

`434f42888b0b289a064b5972c9f10a82739bbb68`

The integration preserves the existing D.RAW branding and Float32/F64 stack while adding parallel read-only backside and deterministic frontside intake.
