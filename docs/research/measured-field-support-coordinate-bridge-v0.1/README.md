# Measured Field Support Coordinate Bridge v0.1

## Purpose

This module is the current successor to the earlier field-bin-centre geometry dry-runs.

It exists because the controlled-rotation field experiment showed that a full appearance-derived residual similarity could lower training RMSE while worsening an independent 270-degree held-out observation. The project therefore stopped treating a theoretical 12x12 polar bin centre as if it were the exact geometric support location of an aggregate field sample.

Implementation:

`suite_android/app/src/main/java/com/truthraw/adaptiveui/MeasuredFieldSupportCoordinateBridgeAuditV01.kt`

## Input authority

The audit consumes:

- one admitted controlled-rotation `FIELD_RESPONSE` Calibration Observation Record;
- the already-computed constrained rotation audit;
- the Universal Source Profiles for the same active source roots;
- `BacksideSignalSupportAudit.sparse_measured_sample_grid`.

The sparse grid exposes actual measured source-payload positions `source_x/source_y` plus normalized signal values.

For support geometry this module uses only source position. Photometric values are not used to construct support centroids, select geometry, fit a bridge variant or choose a winner.

## Support-cell reconstruction

Every sparse measured point is assigned using the same radial/azimuth field partition used by the measured optical-field signal:

- active-area geometric centre is the current field origin;
- rho is normalized to the active-area corner radius;
- 12 radial annuli;
- 12 azimuth sectors.

For each populated `(radial_bin, sensor_sector)` cell the audit derives:

- measured support-point count;
- centroid X/Y in source pixels;
- centroid X/Y in centred isotropic source coordinates;
- centroid rho and azimuth;
- deviation from the ideal theoretical polar-bin centre;
- footprint width and height in source pixels;
- RMS footprint radius in source pixels.

The individual sparse source positions are measured source evidence.

The centroid and footprint are derived statistics. They are not new physical measurements and they do not create new evidence.

## Diagnostic bridge variants

The audit compares the admitted nominal relation with measured-support variants based on:

- nominal rotation;
- residual rotation;
- uniform scale;
- translation;
- residual rotation + scale;
- residual rotation + translation;
- full residual similarity.

All comparison variants remain diagnostic.

No variant is selected automatically.

Held-out photometric values do not fit the constrained geometry and do not select a bridge variant.

## Authority boundary

The following are explicitly false:

- source-grid isotropic coordinates are proven identical to frontside-analysis isotropic coordinates;
- frontside decoder crop binding is proven;
- frontside decoder orientation binding is proven;
- appearance residual rotation is proven sensor-field rotation;
- appearance uniform scale is proven sensor-field scale;
- appearance translation is proven sensor-field translation;
- lower RMSE is physical truth;
- a derived support centroid is an exact measured sample position.

## Non-promotion guarantees

The audit must not:

- mutate the Calibration Observation Record;
- mutate source sample values;
- mutate source sample positions;
- create a replacement calibration record;
- create new sensor evidence;
- promote world registration;
- promote field calibration;
- authorize correction;
- apply an image transform;
- write to the Scientific Master.

## Current status

Source implementation: **GREEN BUILD**

Source-code checkpoint:

`faac21e2e2fe97477c846609b8dc4e71af29b3c1`

Device result for the measured-support successor itself: **PENDING**

The next device run reuses the same four controlled-rotation RAWs and the same immutable relation record.
