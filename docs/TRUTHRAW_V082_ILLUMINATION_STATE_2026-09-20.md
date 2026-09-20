# TruthRaw v0.82 — Observed Illumination State — 2026-09-20

## Status

Integration candidate on:

`integration/truthraw-suite-v0-82-illumination-state`

Android version:

`0.48-v0.82-illumination-state`

Schema:

`TruthRawObservedIlluminationState/0.82`

v0.82 introduces a separate illumination-authority state. It does not modify pixels, Scientific Master, HDR transport or channel authority.

## Why this state exists

Image/channel authority and illumination authority are not the same thing.

A captured scene can have known source-bound color/chromaticity coordinates while the physical illuminant spectrum, light direction, spatial extent or temporal modulation remain unknown.

v0.82 makes that uncertainty explicit rather than letting later HDR/relighting infer more than the evidence supports.

## Current admitted evidence

The first v0.82 implementation consumes only information already produced by the source-bound DNG color-binding path.

For v0.2 dual-illuminant DNG color binding, the producer already resolves:
- white chromaticity x/y;
- correlated color temperature;
- CalibrationIlluminant1/2 profile-reference codes.

v0.82 records x/y/CCT as:

`SOURCE_METADATA_BOUND_ESTIMATE`

and derives a signed CIE-1960 uv Planckian-locus polyline-distance coordinate:

`duv1960PolylineEstimate`

The Duv value is a bounded chromaticity-coordinate estimate. It is not a spectral measurement.

For delegated single-illuminant DNG color-binding paths, the current public producer audit does not expose resolved source-white x/y/CCT. v0.82 therefore leaves white-point authority UNKNOWN rather than duplicating/parsing hidden state.

## Critical non-inferences

Even if CCT/Duv are available, current v0.82 keeps all of these UNKNOWN:
- scene light kind: DAYLIGHT / ARTIFICIAL / MIXED;
- SPD;
- light direction;
- spatial/angular extent;
- temporal modulation/flicker.

Why:
- the same CCT can be produced by physically different spectra;
- DNG profile CalibrationIlluminant codes describe calibration references, not proof of the scene illuminant;
- a single image without bound geometry/timing cannot establish physical direction or flicker;
- CCT is not SPD.

## Ancestry

The immutable v0.82 state binds:
- sealed source-evidence SHA-256;
- Scientific Master SHA-256;
- canonical Open Scene v0.70 SHA-256;
- sourceEvidenceId;
- colorBindingId;
- exact white x/y/CCT/Duv values when available;
- dual-calibration flag;
- profile CalibrationIlluminant reference codes;
- physicalFrameCount=1;
- independentEvidenceCount=1.

## State invariants

Always:
- createsNewEvidence=false;
- scientificMasterModified=false;
- channelAuthorityModified=false;
- counterfactual=false;
- CCT is not SPD proof;
- profile calibration illuminants are not scene-light proof.

## Relationship to existing illumination research

Local Illumination Room Capsule remains a **counterfactual appearance** system.

Counterfactual Illumination & Capture Manifold remains a hypothetical-world/capture system.

v0.82 is different: it records what is known about the *captured illumination state* and is intended to become an upstream authority input to later HDR/illumination reasoning.

Counterfactual states never write into v0.82 observed authority.

## Standalone falsification

The v0.82 host tests require:
- exact Planckian-locus sample -> Duv approximately zero;
- off-locus white -> finite non-zero Duv;
- calibration reference changes alter identity but never light-kind authority;
- known CCT never creates SPD authority;
- ancestry changes alter state SHA;
- invalid xy/CCT fail closed;
- frame/evidence counts other than 1/1 fail closed;
- unknown producer white audit remains UNKNOWN rather than guessed;
- deterministic state identity.

## Future evidence paths

Future revisions may independently add:
- capture-timing-bound flicker observations;
- row-time/rolling-shutter temporal modulation evidence;
- image-space directional evidence with limited authority;
- calibrated SPD;
- calibrated spatial illumination fields.

Each future field must promote only its own illumination-authority dimension.

## Next after v0.82

Only after illumination-state closure should Scientific HDR be redesigned to consume:
- Scientific Master;
- local v0.78 channel authority;
- censor bounds;
- admitted uncertainty;
- v0.82 illumination state.

That HDR stage must still keep CENSORED as bounds and UNKNOWN as unknown.
