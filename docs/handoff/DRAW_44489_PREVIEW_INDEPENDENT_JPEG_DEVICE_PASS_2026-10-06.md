# D.RAW 44489 — Preview-independent full-resolution JPEG — real-device PASS — 2026-10-06

Continuation code: **44489**

This record captures user-provided real-device evidence plus direct inspection of the uploaded `DRAWTEST.zip`. It is a product/runtime acceptance record only. It does not promote scientific authority or modify the frozen meaning of PR #130.

## Repository boundary before recording this evidence

- repository: `1DiDaDo1/TruthRaw`
- active PR: **#131**
- branch: `feat/draw-workspace-free-raster-v01`
- live head resolved before this record: `74327d189128ca0b8fa53e9dac6aa881e6c9433a`
- PR state: open, draft, unmerged, mergeable
- frozen scientific/audit reference: PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`

## Device test source

Visible source in the app:

`IMG_260830_143010_214_008.dng`

UI reports:

- DNG / LinearRaw ProRAW
- imported-file route
- multi-vendor DNG adapter
- finalized scientific preview route available

## Physical timing evidence — preview independence PASS

The first screenshot was taken while the app still showed:

- `Universele bronkennis wordt opgebouwd...`
- `JPG full-resolution opbouwen` already active at about `00:02`
- the normal `D.RAW foto verwerken` worker still active at about `00:05`
- no normal rendered preview visible yet

A later screenshot showed:

- JPEG worker still active at about `00:09`
- only then `D.RAW Advanced/PRO render gereed` at `00:09`
- the normal preview became visible at that point

This ordering is direct real-device evidence that the JPEG request/worker starts and proceeds before the normal presentation preview reaches its Ready/rendered state. Therefore the full-resolution JPEG path is physically accepted as **preview-independent** for this tested source/route.

This proves the intended product dependency separation. It does not by itself prove every possible source/provider/device combination.

## Final JPEG runtime result — PASS

Final on-device status reports:

- `JPG full-resolution gereed`
- output dimensions: `4080x3072`
- reported size: about `1.9 MB`
- route: `PRO`
- detail: `false`
- Light pixels: `9838861`
- Scientific Master/Backplane: `true/true`
- rotation: `0°`
- HDR-front: `true (APPEARANCE_ONLY)`
- Restoration-front: `false (AESTHETIC_REINTEGRATION_ONLY)`
- elapsed: about `00:17`

The UI simultaneously states the hard output boundary:

- preview is a sibling branch, not pixel source or authority
- `Source mutation=false`
- `Scientific Master writeback=false`

## Uploaded artifact inspection

`DRAWTEST.zip` contains:

1. `IMG_260830_143010_214_008_draw_pro_fullres.jpg`
2. `IMG_260830_143010_214_008_draw_full_colour_scientific_master_float32_v0_1.dng`
3. `IMG_260830_143010_214_008_draw_observation_optical_field_chart_v0_1.json`
4. `IMG_260830_143010_214_008_draw_universal_observation_calibration_atlas_v0_1.json`
5. `DRAW_GLOBAL_RESEARCH_SNAPSHOT_v0_1 (6).json`

### JPEG independent inspection

The exported JPEG is independently readable and reports:

- exact bytes: `1,991,838`
- geometry: `4080x3072`
- colourspace: sRGB
- 3-channel 8-bit JPEG
- quality marker interpreted as 96 by ImageMagick
- SHA-256: `115354213c3fd828bd2708414a423dba33e39c644f8aa6ea8a251b1c042f1bbc`

This independently confirms that a real full-resolution 4080x3072 JPEG file was produced, not merely an on-screen preview.

### Float32 Scientific Master inspection

The uploaded scientific-master DNG is independently readable as TIFF/DNG data and reports:

- exact bytes: `153,008,398`
- geometry: `4080x3072`
- 3 samples per pixel
- `BitsPerSample = 32/32/32`
- `SampleFormat = IEEE floating point` for all three channels
- contiguous planar configuration
- orientation = upper-left
- SHA-256: `55830c6c780b137c25834d0c2ca64163aea7402c4066f46e404a384d7e7bbeb2`

This is separate from the JPEG acceptance and must not be confused with the presentation-output authority of the JPEG.

## Safety/authority evidence from uploaded JSON records

The observation optical-field record carries source SHA-256:

`c7356b5ad112e898d720a888410a29705c4aeaa9fcae8b8a6ebf88e2b10c52a6`

and explicitly reports:

- `source_sample_values_modified=false`
- `source_sample_positions_modified=false`
- `new_measured_samples_created=false`
- `creates_new_evidence=false`
- `scientific_writeback_allowed=false`

The universal observation calibration atlas reports the same source SHA and additionally keeps:

- frontside authority = `APPEARANCE_DERIVED_ONLY`
- frontside may create sensor evidence = false
- frontside may write scientific state = false
- measured-support overpaint allowed = false
- appearance reintegration scientific writeback allowed = false
- precision may change authority = false

The global research snapshot keeps:

- PURE/PRO routes from gaining stronger evidence authority
- ADVANCED appearance from writing Scientific Master
- promotion-gate scientific writeback = false
- `creates_new_evidence=false`
- `scientific_writeback_allowed=false`

The N2 A/B screenshot also visibly reports `primary_candidate_applied=false`; its A/B output remains labelled appearance-only.

## Acceptance classification

### PASS now proven on a real device

- preview-independent full-resolution JPEG worker start/progress
- JPEG completion on PRO route
- actual exported 4080x3072 JPEG exists and is readable
- output remains a new derived presentation product by contract/runtime telemetry
- no new-evidence claim in uploaded records
- no scientific-writeback permission in uploaded records
- N2 candidate remains unapplied in the shown A/B state

### Still not independently proven

Byte-exact **source-file immutability** is not independently proven by this package because the original input DNG bytes were not included in `DRAWTEST.zip` for a before/after hash comparison. The runtime/records all report no source mutation, and the app continues to retain the same selected source, but this must remain distinct from an independent byte comparison.

Also still pending:

- non-zero orientation export
- route-mismatch fail-closed
- source-switch/reprocess stale-state clearing
- optional cold-start/process-death restoration
- optional installed-package hash readback

## Next physical test

The next highest-value acceptance test is **non-zero orientation** using the same known-good source:

1. press `90°` once so the downstream override is non-zero;
2. invoke `JPG · full resolution professional`;
3. save a new derivative;
4. require final status to report `rotatie=90°`;
5. visually confirm the output orientation;
6. upload the resulting JPEG so its pixel geometry/orientation can be independently inspected.

Expected behavior is downstream presentation rotation only. It must not alter source samples, sealed evidence or Scientific Master authority.

After that, test route-mismatch fail-closed and source-switch/reprocess stale clearing.

## Permanent boundary

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

**Stable outside. Flexible inside. Evidence law unchanged.**
