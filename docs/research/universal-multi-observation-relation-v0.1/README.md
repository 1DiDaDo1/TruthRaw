# D.RAW Universal Multi-Observation Relation Protocol v0.1

Date: 2026-09-30

Status: **research-only, frozen relation protocol, no calibration promotion, no correction/writeback**

## Why this exists

The first Universal Observation & Calibration Atlas physical gate is complete:

1. one admitted DNG with measured backside + frontside + sample lattice + field chart;
2. one opaque/unsupported CR3 with frontside inspection but fail-closed UNKNOWN scientific backside.

The next problem is not intake. It is how D.RAW may relate multiple sealed observations without falling back to camera names, lens names, vendor profiles or unsupported identity assumptions.

A relation is therefore a separate scientific object.

Two observations remain two immutable evidence roots.

## Core law

> **Relate observations with explicit evidence, never by camera or lens name.**

A relation may organize observations for a calibration experiment. It may not merge source evidence, rewrite measured anchors, invent a new frame, or silently upgrade Scientific Master authority.

## What may support a relation

v0.1 defines five relation-evidence classes:

- `SEALED_CAPTURE_SESSION_PROVENANCE`
- `EXPLICIT_CALIBRATION_CAPTURE_RECORD`
- `OBSERVATION_REPEATABILITY_CANDIDATE`
- `USER_GROUPING_HINT_ONLY`
- `NONE`

### SEALED_CAPTURE_SESSION_PROVENANCE

Strongest current generic relation basis.

The observations are linked through D.RAW acquisition provenance from the same declared capture session/route.

This is not the same thing as saying that a camera model string or lens name matches.

### EXPLICIT_CALIBRATION_CAPTURE_RECORD

A future versioned calibration record may list exact source SHA-256 roots and the experiment role of each observation.

Examples:

- flat-field orientation 0/90/180/270;
- known colour target under characterized illuminant A and illuminant B;
- optical slanted-edge position/direction;
- dark/noise observation;
- physical temporal/motion experiment.

The record is evidence about the experiment relation, not permission to rewrite source measurements.

### OBSERVATION_REPEATABILITY_CANDIDATE

Repeated field/detail/colour/noise behaviour may become an observation-derived candidate relation.

It does **not** prove that two imported files came from the same physical camera or lens.

### USER_GROUPING_HINT_ONLY

The user may place files into one experiment.

That grouping can guide analysis but does not itself become scientific proof.

### NONE

No relation has been admitted.

## What may NEVER prove the relation by itself

- camera model name;
- lens model name;
- vendor name;
- file extension;
- focal-length metadata;
- visually similar frontside content;
- a vendor/lens profile key.

This is the practical consequence of the universal-input law.

## Relation graph

Each observation stays a node identified by its sealed source SHA-256.

A relation is an edge with:

- all source SHA-256 roots;
- a versioned relation evidence class;
- one or more authority-axis scopes;
- remaining uncertainty/UNKNOWN state;
- provenance of the relation itself.

The edge never modifies either node.

## Axis scopes

Relations remain axis-specific.

### FIELD_RESPONSE_REPEATABILITY

May support a repeated camera-system field-response candidate.

It does not automatically become lens-only vignetting.

### COLOUR_RELATION

May support empirical camera-channel to colorimetric calibration after independent reference/illuminant validation.

Visible preview RGB and white balance are insufficient.

### OPTICAL_SUPPORT_RELATION

May support SFR/MTF/PSF, chromatic displacement, field curvature or directional detail claims.

A higher output raster does not create optical support.

### DARK_NOISE_OFFSET_RELATION

May refine offset/noise uncertainty after independent dark/noise observations.

Measured anchors remain immutable.

### TEMPORAL_CAPTURE_RELATION

May relate physical exposure timing/motion observations.

Synthetic motion blur, copied frames and virtual exposures remain appearance/derived and add zero independent evidence.

## Frozen set-level minimums

These are conservative admission minimums for future experiments. They do not themselves promote calibration.

### Field repeatability candidate

- at least 3 independent observations;
- scene variation preferred;
- camera/lens name is not a key;
- lens-only claim remains forbidden.

### Flat-field rotation separation candidate

- 4 recorded orientations preferred/required for this specific stronger separation experiment;
- 0°, 90°, 180°, 270°;
- physical rotation relation must be recorded;
- no automatic vignetting correction.

### Multi-illuminant colour candidate

- at least 2 independently characterized illuminants;
- known reference target;
- held-out validation;
- three camera channels do not prove an arbitrary full spectrum;
- no automatic colour correction.

### Optical-support candidate

- multiple field positions;
- radial/tangential sampling for directional claims;
- focus state recorded when known;
- deconvolution remains unauthorized.

### Dark/noise candidate

- independent dark/neutral observations required;
- uncertainty refinement only;
- no anchor rewriting.

### Temporal candidate

- physical capture timing relation required;
- virtual/derived frames do not count as independent evidence;
- synthetic motion blur remains `APPEARANCE_ONLY`.

## Pair evaluator

`UniversalMultiObservationRelationProtocolV01.evaluatePair()` accepts two Atlas JSON objects plus optional explicit relation evidence.

It verifies:

- valid atlas inputs;
- distinct source SHA-256 roots;
- admitted relation evidence class.

It returns one of:

- provenance-bound relation;
- calibration-provenance-bound relation;
- observation-derived candidate;
- grouping hint;
- relation unproven.

Even the strongest pair result still reports:

- same physical camera proven = false;
- same physical lens proven = false;
- calibration promoted = false;
- correction authorized = false;
- Scientific Master writeback = false.

Those stronger claims require their own later axis-specific validation.

## Permanent safety

- no camera-model relation key;
- no lens-model relation key;
- no vendor mapping;
- no AI/ML/neural/generative runtime;
- no source evidence merge;
- no independent-evidence-count inflation;
- no measured sample mutation;
- no new measured samples;
- no calibration promotion from relation alone;
- no correction from relation alone;
- no Scientific Master writeback.

## Next gate

Build and integrity validation of the frozen relation protocol.

After that, the first axis-specific experiment should remain read-only. The safest starting point is field-response repeatability across multiple observations, because PR96 already provides a common rho/azimuth/radial/tangential coordinate frame.

No correction should be implemented before that experiment passes independent validation.
