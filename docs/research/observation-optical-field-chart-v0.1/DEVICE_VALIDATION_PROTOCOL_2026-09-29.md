# D.RAW Observation Optical Field Chart v0.1
## Frozen real-device validation protocol

Date frozen: 2026-09-29

## Objective

Validate the flat source-field coordinate chart and measured radial/azimuthal CFA audit without promoting a single-scene brightness pattern to lens truth.

## Frozen v0.1 implementation

- active-area geometric center is the chart origin when valid;
- 20-bit source lattice is unchanged;
- rho is source-field radius normalized by the farthest active-area corner;
- radial/tangential basis is geometric only;
- measured payload sampling inherits BacksideSignalSupportAudit's 64-pixel grid;
- 12 radial annuli;
- 12 azimuth sectors;
- four CFA phases;
- compact OpcodeList2/GainMap metadata summary is provenance-only;
- no gain-map application;
- no correction/writeback.

These rules must not be changed after inspecting a validation result.

## Development observation

`IMG_20260403_182145.dng` is development evidence because it was already inspected while v0.1 was designed.

It may be used only to verify runtime output and parser correctness.

Observed development metadata:

- four OpcodeList2 GainMap opcodes;
- each GainMap is 13x17x1;
- row/column pitch 2x2, consistent with phase-selective Bayer application;
- center gain range about 1.01-1.03;
- corner gain range about 1.80-2.44.

These are source-metadata intent hints, not scientific calibration.

## First runtime verification

Run Universal Intake on the development DNG and preserve the exported/source profile JSON.

Required checks:

- source SHA binding is unchanged;
- chart origin and active area are deterministic;
- measured field signal is either available or fails closed with an explicit topology reason;
- no sample is modified;
- gain-map count and compact summaries are parsed if present;
- GainMap applied=false;
- correction_gain_allowed=false;
- scientific_writeback_allowed=false.

## Independent optical-field observations

After runtime verification, use new sealed RAW/DNG observations not inspected during v0.1 design.

Lens identity may be recorded as provenance but may not select a calibration or algorithm.

At least three substantially different scenes from the same physical capture route are required before treating a radial pattern as a repeatability candidate.

## Why multiple scenes are required

A single scene can contain strong center-to-edge brightness and colour structure. Therefore annular p50 trends and outer/inner ratios are scene+lens+sensor composites.

Cross-scene evidence should examine whether a field-position pattern remains after scene content changes.

Useful diagnostics include:

- annular CFA-phase medians;
- azimuth-sector median dispersion;
- radial trend consistency across scenes;
- phase-specific field response;
- repeatability of inferred field center/symmetry;
- agreement or disagreement with DNG GainMap metadata as a non-authoritative secondary hint.

## Promotion boundary

No single observation may:

- prove lens-only vignetting;
- prove the optical axis;
- generate a correction gain;
- modify Scientific Master;
- modify measured anchors.

A future version may propose a stronger optical inference only after a separately frozen multi-observation criterion is defined.
