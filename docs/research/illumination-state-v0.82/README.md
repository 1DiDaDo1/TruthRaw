# TruthRaw v0.82 — Observed Illumination State

Status: **integration candidate**

Schema:

`TruthRawObservedIlluminationState/0.82`

v0.82 creates a new immutable derived state. It does not alter pixels.

## Core separation

Image/channel authority and illumination authority are different axes.

A scene can have well-defined RGB/channel authority while illumination spectrum,
direction, geometry or temporal modulation remain unknown.

## Current v0.82 evidence source

The first implementation consumes only already source-bound information produced
by the DNG color-binding path.

When the v0.2 dual-illuminant producer has successfully resolved:
- source white chromaticity x/y;
- correlated color temperature;

v0.82 records those values with authority:

`SOURCE_METADATA_BOUND_ESTIMATE`

and derives a signed CIE-1960-uv Planckian-locus polyline distance:

`duv1960PolylineEstimate`

This is a coordinate estimate, not a calibrated spectral measurement.

For delegated single-illuminant DNG paths the current producer does not expose
resolved source-white x/y/CCT in its public audit. v0.82 therefore leaves the
white-point state UNKNOWN rather than reparsing/guessing it.

## Explicit non-claims

Even when CCT/Duv are known, v0.82 does **not** infer:
- DAYLIGHT vs ARTIFICIAL vs MIXED;
- spectral power distribution (SPD);
- light direction;
- angular/spatial extent;
- scene geometry;
- temporal modulation/flicker.

DNG CalibrationIlluminant1/2 values are preserved only as **profile calibration
references**. They are not treated as evidence of the actual scene illuminant.

Therefore:
- CCT is not SPD;
- profile calibration illuminants are not scene-light labels;
- warm/cool CCT does not classify artificial/daylight;
- missing timing evidence leaves flicker UNKNOWN.

## Ancestry

The state is bound to:
- sealed source-evidence SHA-256;
- Scientific Master SHA-256;
- canonical Open Scene v0.70 SHA-256;
- sourceEvidenceId;
- colorBindingId;
- exact white-point values when available;
- calibration-reference codes;
- frame/evidence count 1/1.

## Immutable laws

The state always asserts:
- createsNewEvidence=false;
- scientificMasterModified=false;
- channelAuthorityModified=false;
- counterfactual=false;
- physicalFrameCount=1;
- independentEvidenceCount=1.

## Future extensions

Later versions may add:
- capture-timing-bound flicker observations;
- image-space directional evidence with clearly limited authority;
- calibrated SPD;
- calibrated spatial illumination fields.

Those additions must promote only their own illumination-authority fields and
must never silently upgrade RGB/channel authority.
