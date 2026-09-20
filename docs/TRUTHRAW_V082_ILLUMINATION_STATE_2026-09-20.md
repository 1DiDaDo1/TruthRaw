# TruthRaw v0.82 — Observed Illumination State — 2026-09-20

## Status

**CLOSED INTEGRATION — CI + Android APK PASS**

Integration branch:

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


## Validation closure

Full integration CI run:

`35522724167`

All green:
- GCC illumination state
- Clang illumination state
- Clang ASan/UBSan illumination state
- v0.81 canonical Output Acutance
- v0.80 canonical Adaptive Detail parity
- v0.78-v0.79 authority and uncertainty gates
- existing scientific lineage contracts
- Android v0.82 Illumination State
- APK verification
- artifact upload

Artifact:
- GitHub artifact id: `10609012748`
- artifact ZIP SHA-256: `5875b9f72670fd87c153e0193b6f598d8c4601906b34c2e80e432c1dd71a2b6b`
- APK bytes: `6,491,089`
- APK SHA-256: `9dd30ca31c810e71080ae7bdb512e2c3145bb4f5eaadb48ff0e95fdacea1e6fb`
- CI head: `86f92217a700a52bb26583d1f967a072e3e4c660`

v0.82 is therefore the current closed integration baseline.

## Immediate next integration

Recommended next branch:

`integration/truthraw-suite-v0-83-authority-aware-hdr`

Goal:
replace appearance-only HDR gating with an explicit local HDR-authority contract that consumes:
- Scientific Master;
- v0.78 per-channel authority;
- CENSORED lower bounds;
- v0.79 admitted uncertainty when it eventually exists;
- v0.82 illumination state as context only.

Hard rule:
v0.82 white-point/CCT/Duv context may not create extra HDR authority by itself.

Current Camera-5 and generic imported DNG routes still have RECONSTRUCTED=0 and no admitted uncertainty, so v0.83 must remain conservative/fail-closed in those regions rather than inventing headroom.
