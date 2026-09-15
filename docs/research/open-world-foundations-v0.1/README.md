# TruthRaw Open-World Foundations v0.1

Status: **RESEARCH — NOT MAIN-PROMOTED**

This module starts five cross-cutting foundations for TruthRaw without changing the immutable Direct-CFA evidence rules.

## 0. Scope correction: the source is sealed; the world is not

The existing “sealed house” language remains valid only for **source evidence immutability**: the captured RAW/CFA and capture metadata are sealed against rewriting.

It must **not** be interpreted as a physical or computational boundary around the scene. TruthRaw's reconstructed scene domain may describe an open, connected world: interiors, exteriors, streets, landscapes, sky, distant objects, transitions between spaces, and future scene-graph regions without a fixed room boundary.

The open-world rule is therefore:

> **Evidence is locally finite and immutable. The reconstructed world representation is not required to be enclosed or globally bounded.**

This does not turn unsupported world content into evidence. The permanent project boundary remains: representation may exceed the source; knowledge claims may not exceed the evidence.

The implementation in this module is deliberately provenance-first. It adds machine-checkable contracts before adding new reconstruction algorithms.

## 1. Authority-typed illumination

Implemented in `tools/open_world_foundations_v01.py` as `IlluminationRecord`.

Illumination claims are explicitly one of:

- `MEASURED`
- `CALIBRATED_ESTIMATE`
- `INFERRED`
- `COUNTERFACTUAL`

The spatial scope is intentionally free-form and may describe a tile, object, room, street, landscape region, sky dome, camera frustum, or other open-world scene domain.

The validator fails closed when authority would be silently upgraded. For example, measured illumination requires admitted evidence hashes, calibrated estimates require a calibration binding, and counterfactual illumination requires an explicit parent state.

Future work should attach actual radiometric/spectral models, geometry, visibility, interreflection and uncertainty propagation to these authority types. The type itself must survive every later representation.

## 2. Calibration Registry foundation

Implemented as `CalibrationDomain`, `CalibrationRecord`, `CaptureConditions` and `CalibrationRecord.applicability()`.

A calibration record is bound to a validity domain that can include:

- device and physical camera route;
- sensor pixel mode;
- RAW geometry and CFA;
- ISO/gain region;
- exposure region;
- sensor/device temperature region;
- focus-distance region;
- immutable source-evidence hashes;
- calibration method and uncertainty description;
- independent hold-out status and hashed hold-out report.

The first rule is fail-closed applicability: a calibration outside its declared domain returns `OUT_OF_DOMAIN`; an in-domain calibration without the required passed hold-out returns `UNCERTIFIED`.

The v0.1 record kinds cover black/offset, linearity, gain/noise, PRNU, lens shading, optics/PSF/MTF, colour, illuminant and spectral-response calibration.

This is the start of a registry contract, not yet a persistent registry database.

## 3. Structure Evidence Map foundation

Implemented as `StructureEvidenceRecord`.

Each region can carry separate support values for:

- directly measured structure;
- reconstructed structure;
- optical MTF support;
- censoring risk;
- uncertainty;
- optional orientation;
- optional spatial frequency;
- evidence hashes.

v0.1 intentionally does **not** invent a sharpening formula. It returns only an epistemic support class:

- `MEASURED_SUPPORTED`
- `RECONSTRUCTED_SUPPORTED`
- `CENSORED_OR_WEAK`
- `UNKNOWN`

This prevents a visually convincing reconstructed edge from being silently relabelled as measured detail. A later detail/acutance policy may consume these fields, but must remain downstream of the support classification.

## 4. Conservation / restoration domain foundation

Implemented as `RestorationRecord` with reversible layer classes:

- `OBSERVED_LOSS`
- `EVIDENCE_SUPPORTED_RECONSTRUCTION`
- `HYPOTHETICAL_VISUAL_RESTORATION`
- `APPEARANCE_ONLY`

The contract rejects any restoration record that claims to replace the immutable source. Evidence-supported reconstruction requires provenance and confidence; hypothetical visual restoration requires an explicit hypothesis description.

This gives TruthRaw a conservation-style separation between what exists in the source, what is missing/damaged, what can be reconstructed from evidence, and what is only a visual hypothesis.

## 5. Camera 5 native-200MP CFA identity proof

Implemented in `tools/camera5_200mp_cfa_identity_v01.py`.

The existing Camera 5 maximum-resolution path can produce both an app-visible `RAW_SENSOR` buffer and an Android `DngCreator` DNG. v0.1 adds a stronger identity question:

> Do the normalized RAW_SENSOR CFA raster and the DNG RAW IFD contain exactly the same sample codes in exactly the same order?

The verifier is streaming and fail-closed. It binds:

1. the runtime capture manifest;
2. the canonical RAW normalization report;
3. the canonical RAW file hash;
4. the DNG file hash recorded in the capture manifest;
5. exactly one matching CFA RAW IFD;
6. every 16-bit CFA sample in raster order.

A production PASS requires the Camera 5 route and `16320 x 12288` dimensions. The output classification is:

`CAMERA5_200MP_RAW_DNG_CFA_IDENTITY_PROVEN`

### v0.1 storage boundary

The identity verifier intentionally supports only classic TIFF/DNG RAW IFDs that are:

- `PhotometricInterpretation = CFA`;
- uncompressed (`Compression = 1`);
- single-sample CFA (`SamplesPerPixel = 1`);
- `BitsPerSample = 16`;
- strip-organized with no hidden per-strip raster padding.

Tiled, compressed or differently packed RAW IFDs fail closed in v0.1 rather than passing through an unreviewed decoder.

A PASS proves **container/raster identity only**. It does not prove absence of on-sensor or HAL processing, one ADC code per physical photodiode, electron calibration, optical truth, spectral truth or colour truth. It does not create a second independent exposure.

## Tests

`tests/test_open_world_foundations_v01.py` checks authority separation, calibration-domain rejection, reconstruction-vs-measurement separation and source-preserving restoration.

`tests/test_camera5_200mp_cfa_identity_v01.py` constructs synthetic uncompressed CFA TIFF/DNG fixtures and verifies exact-pass, one-sample-mismatch failure and manifest-DNG hash binding.

All tests are dependency-free and run with Python's standard `unittest` module.

## Promotion boundary

This research module must not silently alter canonical reconstruction v4.7i, the immutable source evidence, or the scientific master/export separation.

Before promotion, the next evidence gates are:

- run the CFA identity verifier on a real Camera 5 `16320 x 12288` runtime capture pair;
- add real calibration-registry records only from measured campaigns and independent hold-outs;
- connect illumination authority to CICM/Room Capsule without allowing counterfactual output onto a scientific floor;
- connect structure evidence to the detail/acutance path only after a validated policy is defined;
- keep restoration outputs as separate reversible artefacts/layers with explicit provenance.
