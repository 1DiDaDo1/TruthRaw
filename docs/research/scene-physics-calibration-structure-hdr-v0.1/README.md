# TruthRaw scene physics: light, colour, structure, HDR and 200 MP — v0.1

Status: **RESEARCH FOUNDATION / DOES NOT UPGRADE SCIENTIFIC AUTHORITY**

Date: 2026-09-16

This note turns the historical TruthRaw studies of light falloff, colour/calibration, detail/structure and real HDR into explicit current engineering constraints. It also records the Android Camera2 contract that constrains the Camera-5 200 MP route.

Permanent rule:

> **Representation may exceed the source; knowledge claims may not exceed the evidence.**

No item below turns source metadata, a standard reference illuminant, a display encoding, a sample count, or an Android capability advertisement into physical scene truth by itself.

## 1. Light and light falloff

A pixel is downstream of a physical chain, not a direct label for object brightness:

`source illumination -> transport/visibility -> material interaction -> outgoing radiance -> optics -> sensor/CFA -> sample`

TruthRaw must keep at least these quantities conceptually distinct:

- incident illumination / irradiance;
- outgoing scene radiance toward the camera;
- surface reflectance/material response;
- surface normal/orientation;
- visibility/occlusion/shadowing;
- optical throughput/vignetting/shading;
- sensor response and noise.

### 1.1 Inverse-square is conditional

The inverse-square law applies to irradiance from an ideal point-like source as source-to-surface distance changes. It must not be applied merely because an object is farther from the camera. Distant sunlight is approximately directional over photographic scene scales, so camera-object distance is not the relevant source distance.

### 1.2 Radiance and BRDF/material state

Scene relighting is not equivalent to multiplying RGB. A material can be diffuse, glossy, specular, transmissive, scattering or mixed. Directional response matters. A physically strong relighting claim therefore needs geometry, normals, visibility and material/BRDF support, plus an illumination model.

Counterfactual relighting remains useful, but is `COUNTERFACTUAL` unless the needed scene quantities are independently constrained.

### 1.3 Shadow and night

A shadow is not necessarily a scalar exposure reduction: direct illumination can disappear while sky light, bounce light and local spectral mixtures remain. Night scenes can contain strongly mixed spectra and channels with very different SNR. White balance can rescale channels but cannot create missing photons or missing certainty.

## 2. Colour and calibration

Camera colour is an interaction of illuminant spectrum, material spectral reflectance/transmittance and camera spectral sensitivities. Three camera channels do not uniquely identify an arbitrary spectrum; metameric ambiguity remains possible.

### 2.1 Standard illuminants are references

CIE D65 is a standard representative daylight spectrum, not proof that a captured outdoor scene had exactly D65 illumination. CIE explicitly notes real daylight varies with season, time and location. Likewise D50/A are defined reference illuminants for specified uses.

Therefore a tag or profile labelled D65/D50/A is a colour-processing/calibration reference unless the actual illuminant has been independently measured.

### 2.2 DNG metadata authority

DNG matrices and tags can define a reproducible source-bound colour transform, but source metadata is not automatically independent physical calibration.

TruthRaw keeps:

- `SOURCE_METADATA_BOUND_*` for transforms bound to source/profile metadata;
- stronger physical calibration claims only after independent measurement and validation.

A strong calibration programme should separate:

`calibration capture -> model fit -> held-out validation -> promotion`

and should record lens, focus/readout domain, illumination, target/reference data and uncertainty.

### 2.3 Readout-domain separation

A colour/uncertainty model validated at 4080 x 3072 must not automatically be promoted to 8160 x 6144 or 16320 x 12288. Different sensor modes may have different readout, noise, shading, spectral/spatial behaviour or upstream processing.

## 3. Detail, structure and sharpness

Sample count is not optical resolution.

ISO 12233:2024 explicitly distinguishes addressable photoelements from resolution and defines spatial frequency response (SFR) as contrast loss versus spatial frequency. Therefore:

> **200,540,160 app-visible samples do not prove 200,540,160 independently resolved scene details.**

TruthRaw should separate:

- CFA/sample support;
- optical/detail support;
- reconstruction support;
- appearance/acutance.

Sharpening or local contrast can increase perceived acutance without increasing measured optical information.

### 3.1 Structure Evidence

Structure authority should be tied to measured/reconstructed spatial support, not semantic object labels. Preferred inputs include repeatable SFR/MTF/edge/PSF evidence, focus/motion state, local uncertainty and CFA support.

No high-frequency texture may be promoted as measured merely because it is visually plausible.

### 3.2 200 MP optics gate

A physically captured 16320 x 12288 RAW_SENSOR raster would close the raster/capture gate only. A separate optics gate is still required to determine useful resolved spatial frequency across field position, focus distance, aperture/stabilisation state and wavelength/colour channel.

## 4. HDR and dynamic range

TruthRaw distinguishes:

1. physical scene radiance range;
2. sensor/capture evidence range;
3. Scientific-Master representation range;
4. presentation/transport/display range.

These ranges must never be collapsed.

### 4.1 Censoring

A source-white/clipped sample supports a bound such as `true signal >= capture ceiling` under the relevant calibration; it does not provide an invented exact value above the ceiling. Such a quantity remains `CENSORED` unless independent evidence constrains it further.

### 4.2 PQ/BT.2100 is presentation/transport

BT.2100 defines HDR signal systems including PQ and HLG. PQ can represent very high display luminance, but a PQ code value or display ceiling is not a sensor-dynamic-range measurement. A display/tone/gain-map operation cannot write scientific authority back into the Scientific Master.

### 4.3 Gain maps

Gain maps may adapt an SDR/base representation to available HDR display headroom. They are presentation/transport metadata unless a separate scientific contract explicitly binds them to measured scene radiance. Current TruthRaw classification remains:

`PRESENTATION_AND_DISPLAY_ADAPTATION_ONLY`.

## 5. Camera-5 / 200 MP Android contract

Current project target:

`logical camera 0 -> physical camera 5 -> SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION -> RAW_SENSOR 16320x12288 -> physical TotalCaptureResult 5`

Android Camera2 states that `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION` switches to the maximum-resolution sensor mode and that sensors *typically* operate unbinned in this mode. "Typically" is not proof of untouched native ADC samples.

For an `ULTRA_HIGH_RESOLUTION_SENSOR`, Android also states that when `REMOSAIC_REPROCESSING` is not advertised, `RAW_SENSOR` images will already have a regular Bayer pattern.

Current Camera-5 static evidence reports:

- physical camera `5`;
- `ULTRA_HIGH_RESOLUTION_SENSOR = true`;
- `REMOSAIC_REPROCESSING = false`;
- maximum pixel array `16320 x 12288`;
- advertised high-resolution `RAW_SENSOR 16320 x 12288`;
- `SENSOR_INFO_BINNING_FACTOR = 2 x 2` nevertheless present.

That last combination is a vendor-metadata tension relative to the Android contract. It is **not** evidence that the app-visible 200 MP RAW has a 2x2 same-colour CFA. For TruthRaw, the correct bounded interpretation remains:

`APP_VISIBLE_RAW_SENSOR_REGULAR_BAYER_BY_ANDROID_CONTRACT`

with the metadata inconsistency recorded separately.

## 6. Exact current 200 MP status

As of this research note, the repository has:

- static capability proof for the 16320 x 12288 route;
- host-side probe validation and CI/synthetic proof-chain tests;
- a fail-closed RAW_SENSOR -> canonical CFA -> DNG identity/topology validator;
- no repository evidence of a real BKQ-N49 16320 x 12288 RAW_SENSOR payload + manifest + bound physical TotalCaptureResult satisfying the promotion gate.

Therefore the physical gate remains:

`OPEN_NEEDS_REAL_16320x12288_RAW_PAYLOAD_AND_TOTALCAPTURERESULT_BINDING`

The strongest permitted future claim after a qualifying run remains:

`APP_VISIBLE_MAXIMUM_RESOLUTION_RAW_SENSOR_CFA_PROVEN`

and not:

`UNTOUCHED_NATIVE_200MP_ADC`.

## 7. Required Step 3B evidence

A qualifying real run must retain, at minimum:

- physical camera id/result id `5`;
- requested and returned `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION` where reportable;
- real `Image` dimensions `16320 x 12288`;
- `Image.timestamp == SENSOR_TIMESTAMP` binding;
- row stride, pixel stride, buffer length and padding facts;
- original RAW_SENSOR payload bytes and SHA-256;
- a canonical sample-domain report with exact sample count `200,540,160`;
- CFA arrangement/topology;
- black/white levels;
- exposure, ISO, focus and stabilisation state;
- NoiseProfile and lens-shading/calibration metadata where available;
- DNG only as an auxiliary/derived container, with sample identity checked separately.

After that, separate promotion gates remain for noise/PTC, shading, colour/illuminant, SFR/MTF and held-out calibration.

## 8. Sources consulted for this foundation

Primary/current standards and platform documentation:

- Android Camera2 `CaptureRequest.SENSOR_PIXEL_MODE`: https://developer.android.com/reference/android/hardware/camera2/CaptureRequest
- Android Camera2 `CameraCharacteristics.SENSOR_INFO_BINNING_FACTOR` and UHR/remosaic contract: https://developer.android.com/reference/android/hardware/camera2/CameraCharacteristics
- Android `TotalCaptureResult` physical-camera result binding: https://developer.android.com/reference/android/hardware/camera2/TotalCaptureResult
- Android `CaptureResult.SENSOR_TIMESTAMP`: https://developer.android.com/reference/android/hardware/camera2/CaptureResult
- ISO 12233:2024 — Digital cameras — Resolution and spatial frequency responses: https://www.iso.org/standard/88626.html
- CIE ISO/CIE 11664-2:2022 — Standard Illuminants for Colorimetry: https://cie.co.at/publications/colorimetry-part-2-cie-standard-illuminants-0
- CIE 015:2018 — Colorimetry, 4th Edition: https://www.cie.co.at/publications/colorimetry-4th-edition
- ITU-R BT.2100-3 (2025) — HDR television image parameters: https://www.itu.int/rec/R-REC-BT.2100-3-202502-I/en
- Adobe DNG 1.7.1 resources: https://helpx.adobe.com/camera-raw/desktop/dng-and-file-formats/digital-negative.html

Supporting physical-image-formation reference:

- MIT Computational Photography, light/radiometry/BRDF fundamentals: https://people.csail.mit.edu/fredo/comp-photo-book/02-fundamentals-01-light-and-physics.html

## 9. Engineering consequence

The next TruthRaw scene model should not be four unrelated enhancement stages (`relight`, `colour`, `sharpen`, `HDR`). It should expose a common authority-bound scene state where:

- illumination/light transport;
- colour/calibration;
- spatial/detail support;
- radiometric/censoring/HDR status

can constrain one another without upgrading one another's authority.

This is the scientific direction for the next open-world runtime iteration.
