# TruthRaw current scene-physics, restoration and 200 MP supplement — 2026-09-16

Status: **CURRENT RESEARCH SUPPLEMENT / NOT A MAIN PROMOTION**

This document extends `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md` with the latest explicit research on light/light transport, colour/calibration, detail/structure, scientific HDR, conservation/restoration methodology and the exact Camera-5 200 MP Android contract.

It does not widen scientific claims beyond existing evidence.

## 1. Common scene model

TruthRaw should no longer treat relighting, colour correction, sharpening and HDR as four independent image effects. They are coupled views of one authority-bound scene state:

`illumination + geometry/visibility + material response + optics + sensor/CFA + noise/calibration -> measured RAW evidence`

The inverse problem reconstructs only what the evidence/support allows.

Permanent law:

> **Representation may exceed the source; knowledge claims may not exceed the evidence.**

## 2. Light and light falloff

The relevant physical chain distinguishes incident illumination from outgoing scene radiance, material interaction, visibility/shadowing, optical throughput and sensor response.

The inverse-square law is conditional: it applies to irradiance from an ideal point-like source as source-to-surface distance changes. It must not be used merely because an object is farther from the camera. Sunlight is approximately directional over ordinary photographic scene scales.

Shadows are not necessarily scalar exposure reductions. Direct illumination, skylight, bounce light and mixed local spectra can change independently.

A physically strong relighting claim therefore needs support for geometry/normals, visibility, material/BRDF and illumination. Otherwise relighting remains `COUNTERFACTUAL`.

## 3. Colour and calibration

Camera RGB is produced by the interaction of illuminant spectrum, scene spectral reflectance/transmittance and camera spectral sensitivities. Three camera channels cannot uniquely determine an arbitrary spectrum, so metameric ambiguity remains possible.

CIE standard illuminants such as D65 are reference spectral power distributions, not proof that a real scene was illuminated by exactly D65. Source/DNG matrices can define a reproducible source-bound transform, but they do not by themselves become independent physical calibration.

TruthRaw therefore preserves the distinction between:

- source-metadata-bound colour transforms;
- independently measured calibration;
- appearance grading.

Stronger physical colour claims require calibration capture, model fit, held-out validation and uncertainty, with lens/readout/illumination context recorded.

## 4. Detail and structure

Sample count is not optical resolution. ISO 12233 explicitly treats spatial-frequency response/resolution separately from addressable photoelements.

TruthRaw therefore separates:

- CFA/sample support;
- optical/detail support;
- reconstruction support;
- appearance/acutance.

Sharpening/local contrast may increase perceived acutance without creating measured high-frequency scene information.

For the future 16320x12288 route, a successful raster proof closes only the capture/raster gate. Optical useful resolution still requires an independent SFR/MTF/PSF-style gate across field position, focus and relevant capture state.

## 5. Scientific HDR

TruthRaw separates four ranges:

1. physical scene radiance range;
2. capture/sensor evidence range;
3. Scientific-Master representation range;
4. presentation/transport/display range.

A clipped/censored source sample supports a bound, not an invented exact radiance. `UNKNOWN` creates no scientific HDR headroom.

BT.2100/PQ/HLG and Adobe Gain Maps are presentation/transport mechanisms. They can encode/display large luminance ranges but do not prove sensor dynamic range or add capture evidence.

## 6. Conservation/restoration as a provenance model

Professional conservation strongly reinforces TruthRaw's architecture:

`examination -> condition report -> scientific investigation -> stabilization -> restoration/loss compensation -> documented presentation`

The digital mapping is:

`sealed source -> condition/evidence assessment -> stable Scientific Master -> bounded reconstruction -> optional aesthetic reintegration -> provenance-preserving export`

Key conservation principles that map directly:

- preserve original material;
- document condition before intervention;
- distinguish stabilization from retouching;
- compensation for loss must remain identifiable/provenanced;
- avoid overpainting surviving original material;
- favour reversibility/retreatability;
- document the exact intervention.

In TruthRaw, valid measured CFA support is the digital analogue of surviving original material. It may not be overwritten by a prettier inferred value in the scientific master.

A repaired/loss-compensated quantity remains `RECONSTRUCTED`; an appearance-only repair has no scientific writeback; unsupported loss remains `UNKNOWN`/unresolved.

The machine-readable guard is implemented in `tools/restoration_authority_v01.py` with tests in `tests/test_restoration_authority_v01.py`.

## 7. Multi-modal evidence

Conservation may use raking light, UV, IR, XRF and other physical examinations. Those can genuinely add evidence because they are additional observations.

TruthRaw distinction:

- a genuinely independent physical modality can add evidence under an explicit admission contract;
- a virtual exposure, tone curve, relight, generative fill or display transform cannot.

The current master remains single-frame: `physicalFrameCount=1`, `independentEvidenceCount=1`.

## 8. Camera-5 200 MP: current Android contract

Current static Camera-5 evidence reports:

- physical camera id `5`;
- `ULTRA_HIGH_RESOLUTION_SENSOR = true`;
- `REMOSAIC_REPROCESSING = false`;
- maximum pixel array `16320 x 12288` = `200,540,160` samples;
- advertised high-resolution `RAW_SENSOR 16320 x 12288`;
- advertised `RAW10 16320 x 12288`;
- `SENSOR_INFO_BINNING_FACTOR = 2 x 2` is nevertheless present.

Android's UHR contract states that when `REMOSAIC_REPROCESSING` is not advertised, RAW_SENSOR is already regular Bayer. Therefore the simultaneous binning-factor metadata is retained as **vendor metadata tension**, not interpreted as proof of a 2x2 same-colour app-visible CFA.

The bounded topology interpretation remains:

`APP_VISIBLE_RAW_SENSOR_REGULAR_BAYER_BY_ANDROID_CONTRACT`

The new machine-readable guard is:

`tools/camera5_200mp_android_contract_v04.py`

with regression coverage in:

`tests/test_camera5_200mp_android_contract_v04.py`.

## 9. Exact 200 MP promotion boundary

The current repository has static capability proof, host-side probe preparation, synthetic/CI proof-chain tests and fail-closed CFA identity/topology tooling.

It still lacks a qualifying real BKQ-N49 `16320x12288 RAW_SENSOR` payload set bound to the physical Camera-5 `TotalCaptureResult`.

Therefore the physical gate stays:

`OPEN_NEEDS_REAL_16320x12288_RAW_PAYLOAD_AND_TOTALCAPTURERESULT_BINDING`

A future qualifying run may support:

`APP_VISIBLE_MAXIMUM_RESOLUTION_RAW_SENSOR_CFA_PROVEN`

It may not, without separate evidence, support:

`UNTOUCHED_NATIVE_200MP_ADC`.

## 10. Required physical evidence for Step 3B

The real run must retain at least:

- physical camera/result id `5`;
- requested and returned maximum-resolution pixel mode where exposed;
- real `Image` dimensions `16320 x 12288`;
- timestamp binding between Image and capture result;
- row stride, pixel stride, buffer size and padding facts;
- original RAW_SENSOR bytes and SHA-256;
- canonical sample-domain report with exactly `200,540,160` samples;
- CFA topology;
- black/white levels;
- exposure, ISO, focus and stabilization state;
- NoiseProfile and shading/calibration metadata where available;
- DNG only as an auxiliary container, separately identity-checked.

After that, readout-domain-specific gates still remain for precision/uncertainty, noise/PTC, shading, colour/illuminant and optics/SFR.

## 11. Primary references used for this supplement

- Android Camera2 `CaptureRequest.SENSOR_PIXEL_MODE`: https://developer.android.com/reference/android/hardware/camera2/CaptureRequest
- Android `CameraCharacteristics` UHR/remosaic/binning contract: https://developer.android.com/reference/android/hardware/camera2/CameraCharacteristics
- Android `TotalCaptureResult` and `CaptureResult.SENSOR_TIMESTAMP`: https://developer.android.com/reference/android/hardware/camera2/TotalCaptureResult and https://developer.android.com/reference/android/hardware/camera2/CaptureResult
- ISO 12233:2024: https://www.iso.org/standard/88626.html
- CIE ISO/CIE 11664-2:2022: https://cie.co.at/publications/colorimetry-part-2-cie-standard-illuminants-0
- CIE 015:2018: https://www.cie.co.at/publications/colorimetry-4th-edition
- ITU-R BT.2100-3: https://www.itu.int/rec/R-REC-BT.2100-3-202502-I/en
- American Institute for Conservation Code of Ethics/Guidelines: https://www.culturalheritage.org/conservation-at-work/uphold-professional-standards/code
- Smithsonian American Art Museum Painting Conservation Studio: https://americanart.si.edu/art/conservation/center/paintings-studio
- Canadian Conservation Institute paintings/condition reporting guidance: https://www.canada.ca/en/conservation-institute/services/preventive-conservation/guidelines-collections/paintings.html
- Library of Congress photograph preservation/digitization guidance: https://www.loc.gov/preservation/about/faqs/photographs.html

## 12. Next implementation direction

The next open-world/Dynamic-Authority runtime should expose a shared scene-state contract in which light, colour, spatial/detail support, HDR/censoring and restoration masks constrain one another while remaining separately authoritative.

The immediate acquisition priority remains the real Camera-5 Step 3B evidence set; no software-side interpretation should pre-close that physical gate.
