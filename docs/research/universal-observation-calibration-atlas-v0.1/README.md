# D.RAW Universal Observation & Calibration Atlas v0.1

Date: 2026-09-30

Status: **research-only, read-only, universal-input foundation, no correction/writeback**

## Purpose

This branch applies the next D.RAW architecture step without violating the universal-input law.

D.RAW must not need to know which camera, lens, vendor or RAW family is standing at the door before it can seal and inspect the observation.

The atlas therefore starts from:

```text
sealed observation
  -> back side: bytes / container / CFA / metadata / measured signal when decodable
  -> front side: visible proportions / luminance / colour / structure
  -> separate authority axes
  -> optional later calibration-observation relations
```

It never starts from:

```text
camera name -> lens profile -> correction
```

## Universal-input law

The v0.1 contract explicitly records:

- camera identity is not required;
- lens identity is not required;
- vendor identity is not required;
- prior user calibration is not required;
- a RAW format may select a decoder adapter, but the format may not define scientific truth;
- unknown facts remain UNKNOWN;
- no device-specific map is required.

Canonical wording:

> **Observe what is present before asking who produced it.**

A normal user does not need to photograph a calibration chart or flat field before D.RAW can ingest and develop a source.

## Back side and front side remain linked

The atlas binds both sides to the same sealed `source_sha256`.

### Back side

Where available it records the state of:

- CFA geometry;
- BlackLevel and WhiteLevel;
- ActiveArea;
- NoiseProfile presence;
- raster-independent sample lattice;
- measured backside signal support;
- source metadata;
- PR96 optical-field coordinates and composite field signal.

### Front side

It records the deterministic `FrontsideSceneInspector` result as:

`APPEARANCE_DERIVED_ONLY`

including visible luminance/colour statistics and structure where available.

Frontside data may help decide whether an interpretation is unsafe or ambiguous. It may not create sensor measurements or silently become colorimetric/optical calibration.

## Colour authority

v0.1 separates:

1. `UNKNOWN_OR_UNCALIBRATED`;
2. `SOURCE_METADATA_BOUND_COLOUR_AVAILABLE` when the source supplies usable colour metadata;
3. future empirical multi-illuminant calibration;
4. future spectral calibration.

The latter two are **not** fabricated by v0.1.

White balance is not spectral calibration.

Visible frontside RGB is not colorimetric calibration.

A three-channel camera observation does not prove an arbitrary full spectrum.

No automatic colour-matrix refit is authorized by this branch.

## Illumination and light falloff

PR96 already provides a source-field coordinate chart and measured composite signal where topology permits.

v0.1 carries that forward as:

`COMPOSITE_SCENE_LENS_SENSOR_OBSERVATION_ONLY`

It does not rename it to lens vignetting.

The atlas explicitly keeps UNKNOWN:

- physical light kind;
- SPD;
- light direction;
- spatial/angular extent;
- flicker/temporal modulation;
- lens-only falloff;
- camera-system relative illumination calibration.

No automatic light-falloff correction is authorized.

## Optional calibration observations

Calibration is extra evidence, not an entrance requirement.

The atlas defines future optional observation classes:

### FLAT_FIELD_RELATIVE_ILLUMINATION

A set of independent observations may test whether a spatial response repeats after scene content changes. Rotation of the camera/system relative to an external flat field can help separate source-plane illumination gradients from sensor-coordinate repeatability.

This should be described conservatively as a **camera-system field response** until lens, sensor angular response and scene illumination are independently separated.

### COLOUR_REFERENCE_MULTI_ILLUMINANT

Known reflectance/reference targets under independently characterized illuminants may support an empirical camera-channel -> colorimetric relation.

Any fit must carry held-out validation and uncertainty.

It is optional and may never be selected by camera/lens name alone.

### OPTICAL_SFR_MTF_PSF

Independent slanted-edge/SFR, PSF or MTF observations may support local radial/tangential optical-bandwidth claims.

Output raster density remains independent from optical resolving support.

### DARK_NOISE_OFFSET

Independent dark/noise observations may refine uncertainty and offset/noise interpretation.

They cannot rewrite measured anchors.

### TEMPORAL_MOTION_FOOTPRINT

Additional physical timing/motion observations may constrain shutter-time/rolling-shutter/motion footprint.

Synthetic motion blur remains appearance-only.

## Optical support

The atlas deliberately keeps SFR/MTF/PSF support UNKNOWN unless independent evidence is attached through a later relation contract.

Focal length metadata cannot prove optical support.

A denser D.RAW output raster cannot create optical resolving power.

No deconvolution is authorized by this branch.

## Temporal footprint

When ExposureTime is present, v0.1 may record it as source-metadata-bound capture integration duration.

It does not infer the camera/subject motion path or rolling-shutter geometry.

A virtual exposure or copied/derived frame creates zero new physical evidence.

## Precision and Scientific Master

This branch does not change the existing precision architecture:

```text
exact integer/packed source evidence
  -> Float64 branch-sensitive scientific compute where required
  -> controlled Float32 canonical scientific storage
```

Precision changes do not change authority.

## Zero-Line / TruthRange

The atlas records that the Zero-Line / TruthRange architecture remains active.

- BlackLevel is not the Zero-Line.
- Zero-Line is a reference/gauge.
- WhiteLevel is not the ceiling of TruthRange.

## Restoration

The conservation boundary remains explicit:

- measured support may not be overpainted;
- loss compensation remains RECONSTRUCTED;
- appearance reintegration may not write back into Scientific Master.

## What v0.1 actually implements

`UniversalObservationCalibrationAtlasV01` is attached by `UniversalSourceProfiler`.

It exists for:

- fully parsed DNG/TIFF intake;
- opaque/other RAW or image containers;
- TIFF parse failures.

For unknown/unsupported formats the atlas still exists, but unsupported scientific axes remain UNKNOWN rather than guessed.

This is important: **universal intake does not mean universal hallucination.**

## Permanent invariants

- no lens-profile lookup;
- no camera-model scientific routing;
- no vendor scientific mapping;
- no AI/ML/neural/generative runtime;
- no user calibration requirement;
- no source sample mutation;
- no sample-position mutation;
- no new measured samples;
- no correction gain;
- no Scientific Master writeback.

## Next gates

The next implementation gates are intentionally separate:

1. explicit JSON export from the Android UI;
2. real-device export on an arbitrary DNG;
3. export on an opaque/proprietary RAW source to verify the UNKNOWN/fail-closed universal path;
4. define a versioned multi-observation relation contract;
5. only then test optional flat-field, colour-reference, optical-support, dark/noise and temporal calibration observations.

No correction path should be added before an independent relation/promotion protocol is frozen.


## Android build validation

The full D.RAW ARM64 Android build is green for this branch.

- workflow: `D.RAW Suite Universal Intake v0.1`;
- run: `36641226959`;
- artifact: `DRAW_Full_Suite_Universal_Intake_v0.1_debug_arm64`;
- artifact id: `11066632486`;
- artifact ZIP SHA-256: `8a75ae20b3755ecf0638d6a0b2f8170cd8895f2be117f80e7d2b6cbeefa3b722`;
- APK bytes: `7582479`;
- APK SHA-256: `12c95e8c4d6501cf5ff8c7d8d3ca9076b5e1d02c6ebf96469b2245dc7f2dc32d`.

The built APK was inspected and contains the v0.1 schema and safety markers, including the universal identity law, no-user-calibration requirement, source-metadata colour state, composite scene/lens/sensor field authority, Zero-Line separation and explicit Android export label.

This closes the software/build gate only. It does not replace the next physical/device validation gate.


## First real-device admitted-DNG validation

A real-device export bundle was returned and inspected:

- bundle: `DRAWrawdogatlas.zip`;
- bundle SHA-256: `5b87ef06f783cdd12fc8b8ac270005a39f48dca8350970f442e2287832a81d70`;
- atlas source SHA-256: `578fad42dad6819b1d3f9a1f9cbfcc5c547b63ae01f3951f26dca988d655d10e`;
- source class: `DNG_CFA_RAW`;
- source route: `IMPORTED_FILE`;
- front side: `FRONTSIDE_STRUCTURAL_INSPECTION_AVAILABLE`;
- back side: `MEASURED_SOURCE_PAYLOAD_SAMPLE_AVAILABLE`;
- sample lattice: `AVAILABLE`;
- field chart: `FIELD_CHART_AVAILABLE`;
- colour base authority: `SOURCE_METADATA_BOUND_COLOUR_AVAILABLE`;
- lens-only vignetting: not proven;
- user calibration required: false;
- automatic colour correction: false;
- automatic light-falloff correction: false;
- Scientific Master writeback: false.

The returned Full Colour Scientific Master was also independently inspected:

- 4080x3072;
- three-channel IEEE Float32;
- LinearRaw;
- uncompressed primary;
- sealed source SHA exactly matches the atlas;
- F64 reconstruction policy identifier preserved;
- Zero-Line binding preserved;
- negative scene-linear components present and retained;
- two green-channel components above 1.0 were retained;
- no NaN or infinity values were found.

This passes the admitted-DNG half of the first physical validation gate.

The opaque/unsupported-RAW half remains pending. No calibration or correction promotion is authorized yet.
