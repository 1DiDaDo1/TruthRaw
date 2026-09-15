# TruthRaw Expertise Foundation — Free Scientific Space, colour, light, calibration, sharpness, restoration and 200MP

**Date:** 2026-09-15  
**Status:** PROJECT CONTINUITY + EXPERTISE AUTHORITY  
**Scope:** scientific foundation and next-step guidance; does not by itself promote any calibration domain to FULL_PHYSICAL and does not replace module-local CI/evidence gates.

## Mandatory rule for future chats

A future TruthRaw chat that continues reconstruction, calibration, FotoGraaf, Camera2, 200MP, colour, light, sharpness, restoration, noise or numeric precision should read this document before proposing architecture or code changes.

This document records two things together:

1. the user's corrected project vision: TruthRaw is a **free scientific scene space**, not a RAW-bound or "sealed" reconstruction domain;
2. the scientific/engineering expertise required to continue the physical Camera-5 200MP path correctly.

---

# 1. Corrected core vision: Free Scientific Space

## 1.1 The source RAW is evidence, not a prison

TruthRaw must preserve the exact admitted source RAW/CFA bytes and capture metadata as provenance/evidence. That preservation requirement does **not** mean the reconstructed scientific world is sealed inside the RAW.

The active vision is therefore:

> **The source RAW is an immutable measurement record. TruthRaw reconstructs in a separate Free Scientific Space whose representation is not constrained by the RAW container.**

The older "sealed house" metaphor is historically useful for explaining why original evidence must not be silently overwritten, but it is now **superseded as the primary architectural metaphor** because it can falsely imply that TruthRaw itself is enclosed by the source RAW.

TruthRaw is not enclosed by:

- RAW10 / RAW12 / RAW14 integer ranges;
- the source `WhiteLevel` as a master-output ceiling;
- the original ISO as a scene scale;
- `[0,1]` normalization;
- source Bayer geometry as the only possible master geometry;
- source pixel count as the only possible reconstruction lattice;
- the source camera colour basis as the final colour basis;
- DNG container conventions;
- SDR/HDR display ranges;
- a fixed float type such as float32;
- a requirement that every reconstructed coordinate correspond one-to-one to an original stored sample.

The correct conceptual chain is:

`exact source evidence -> measurement model -> inference/reconstruction -> Free Scientific Scene Space -> optional scientific/compatibility/appearance projections`

## 1.2 What remains constrained

Free Scientific Space removes **representation constraints**, not the laws of information.

A reconstructed value may exceed the source encoding range, but its evidential status must still be explicit. TruthRaw retains the distinction between:

- `MEASURED` — directly supported by admitted measurement evidence;
- `RECONSTRUCTED` — inferred from evidence plus a documented model;
- `CENSORED/BOUNDED` — only a limit is measured, e.g. clipping;
- `WEAK/UNKNOWN` — insufficient support;
- `COUNTERFACTUAL` — hypothetical world or illumination, not the captured world;
- `APPEARANCE` — reversible presentation choice.

Permanent principle:

> **Representation may be freer than the source. Evidence claims may never be stronger than the evidence.**

and:

> **Measured where measured. Reconstructed where necessary. Never invented.**

## 1.3 No fixed numeric cage

The scientific definition of the Scene Master should not be "a float32 image" or "a float64 image".

The scientific contract should be **precision-independent**: conceptually real-valued scene quantities, uncertainty/covariance/support/censor information and provenance. A concrete implementation may choose a numerical precision profile as an execution decision.

This is the numeric equivalent of the cheap-phone / expensive-phone rule: resource capability may change implementation strategy, but not scientific meaning.

---

# 2. Float64 and higher: what exists and how TruthRaw should use it

## 2.1 Available precision families

Yes: float64 exists and is ordinary double precision. Java/Kotlin `Double` corresponds to IEEE-754 binary64. Java/JVM also has binary32 `float` and binary64 `double` as its standard primitive floating-point types.

Beyond float64, IEEE-style 128-bit floating point exists as binary128 / quadruple precision. Compiler support is target-dependent rather than universally portable. GCC documents `_Float128` / `__float128` on supported targets, not as a type guaranteed everywhere.

Beyond fixed 128-bit floating point, arbitrary-precision floating-point arithmetic exists. GNU MPFR, for example, allows the significand precision to be selected explicitly per variable.

Therefore TruthRaw should distinguish:

- **binary32 / float32** — 4 bytes/sample; high throughput and lower memory;
- **binary64 / float64 / double** — 8 bytes/sample; substantially more precision;
- **binary128 / float128 class** — 16 bytes/sample where supported; reference/high-precision use, not a universal Android primitive;
- **arbitrary precision** — software precision chosen as needed; useful for reference verification and sensitive calibration mathematics, unsuitable as a default 200MP per-pixel hot path.

## 2.2 Recommended TruthRaw precision policy

TruthRaw should adopt a **Precision-Independent Scientific Master Contract** with concrete execution profiles rather than making one float type canonical truth.

Recommended policy:

### Evidence storage

Preserve original integer/packed RAW sample bytes exactly. Do not convert the only authoritative copy to floating point.

### Per-pixel reconstruction hot path

Use float32 where validation proves its numerical error is negligible relative to the physical/model uncertainty. Permit float64 tile paths for numerically difficult operations or devices/workstations where resources permit.

### Scientific calibration and parameter estimation

Default to float64 for:

- colour-matrix / spectral fitting;
- least-squares and robust regression;
- PTC/noise-model fitting;
- covariance propagation where conditioning matters;
- PSF/MTF fitting;
- geometric optimisation;
- reductions over many samples;
- calibration-profile generation;
- reference implementations against which faster kernels are validated.

### Reference / audit mathematics

Use binary128 or arbitrary precision selectively when testing whether float64 itself is contributing meaningful numerical error. Do not force all pixels through binary128 merely because it exists.

### Rule

**Higher precision is an implementation tool, not extra photographic evidence.**

A float128 result is not more physically true than float64 if measurement uncertainty dominates. The test is whether numerical error is safely below the relevant scientific uncertainty/error budget.

## 2.3 Why full-frame float64/128 is expensive at 200MP

The Camera-5 maximum lattice is 16320 x 12288 = 200,540,160 samples.

For one scalar plane, excluding all metadata/allocator overhead:

| representation | bytes/sample | one 200MP scalar plane | one 200MP RGB triplet |
|---|---:|---:|---:|
| float32 | 4 | 802,160,640 B = ~0.747 GiB | ~2.241 GiB |
| float64 | 8 | 1,604,321,280 B = ~1.494 GiB | ~4.482 GiB |
| 128-bit float class | 16 | 3,208,642,560 B = ~2.988 GiB | ~8.965 GiB |

This reinforces the existing TruthRaw tiled/streaming architecture. It does **not** justify reducing scientific precision blindly; it means high precision should be applied where it contributes, with tile-local workspaces and high-precision accumulators/parameters when possible.

GPU note: Vulkan exposes 64-bit shader floating point through the optional `shaderFloat64` device feature. TruthRaw must therefore never assume GPU float64 is universally available on Android.

---

# 3. Light and light falloff: required physical separation

TruthRaw must not represent all spatial brightness variation as one generic "light falloff" field.

At least four physical layers must remain separable:

1. **Scene illumination** — incident light field on the subject;
2. **Material / BRDF / transmission response** — how a surface or volume reflects, scatters, transmits or specularly redirects that light;
3. **Optics** — lens transmission, vignetting, flare, PSF, aberrations and angle-dependent effects;
4. **Sensor / upstream camera response** — microlens/pixel response, PRNU, lens-shading correction already applied by sensor/HAL, and other app-visible processing.

The inverse-square law is not a universal scene-lighting model. It applies under suitable source geometry; extended light sources, windows, sky, bounced light and near-field large panels require their actual geometry. Surface orientation and material properties are also independent factors.

For TruthRaw the consequence is:

> **Do not "repair" physical scene shading using a lens-shading model, and do not interpret residual camera shading as physical scene illumination.**

ISO 17957 provides a formal framework for measuring luminance and colour shading of digital cameras, including camera phones. TruthRaw should use this as guidance for the calibration protocol, while keeping its own provenance/evidence requirements.

## 3.1 Honor-specific boundary

Camera 5 reports:

`SENSOR_INFO_LENS_SHADING_APPLIED = true`

Therefore a Camera2 RAW_SENSOR buffer may be preserved byte-exactly as the app-visible measurement, but it cannot automatically be claimed to contain untouched physical optical falloff from the photodiode/ADC domain.

TruthRaw must distinguish:

- **Physical Optical Shading** — only claimable when independently measured/identified;
- **App-Visible Residual Shading** — directly characterisable from the Camera2 output domain;
- **Scene Illumination** — inferred physical scene quantity;
- **Appearance Vignette** — presentation only.

---

# 4. Colour truth and colour calibration

## 4.1 Camera metadata is useful, not universal colour truth

A Camera2 colour transform, AWB state or CCT estimate is not sufficient to define FULL_PHYSICAL colour truth across arbitrary illuminants.

The camera records a small number of broad sensor-channel responses to the product of:

`illumination spectrum x material spectral response x optics/sensor spectral sensitivity`

Different spectra can map to the same or similar RGB triplets. Therefore colour reconstruction has unavoidable metameric ambiguity unless stronger spectral/calibration information is supplied.

## 4.2 Calibration authority

ISO 17321-1 specifies camera colour-characterisation procedures using either narrow spectral band stimuli or a spectrally and colorimetrically calibrated target. That is much closer to TruthRaw's physical-calibration goal than treating a vendor matrix as absolute truth.

TruthRaw should calibrate **per physical camera and per sample/readout mode** rather than assuming one lens profile covers all modes.

At minimum, the calibration programme should support:

- a spectrally characterised colour target;
- multiple known illuminants, not one white balance condition;
- daylight-like illumination;
- warm Planckian/tungsten-like illumination;
- representative LED spectra with discontinuous/narrow spectral components;
- stable exposure and temperature logging;
- mode identity: 4080x3072 / 8160x6144 / 16320x12288 must not be silently merged;
- uncertainty/residual reporting rather than a single matrix with no error model.

Preferred conceptual path:

`CFA evidence -> black/gain/noise interpretation -> camera-native scene estimate -> illuminant/spectral model -> colourimetric scientific state -> appearance transform`

Appearance colour must never write back into scientific colour state.

---

# 5. Sensor/noise calibration programme

ISO 15739 and EMVA 1288 provide useful objective methods for noise, signal, dynamic range, linearity and sensor non-uniformity. TruthRaw should adapt these methods to its app-visible Camera2 domain while preserving stricter evidence provenance.

The calibration layers should remain separate:

1. **Black level / dark offset**
   - optical black where truly present and validated;
   - otherwise dynamic black where available;
   - static black only as a lower-authority fallback.

2. **White/saturation level**
   - dynamic white where valid;
   - static white as fallback;
   - clipping recorded as censored information.

3. **Gain / linearity / photon-transfer behaviour**
   - exposure series under stable uniform illumination;
   - temporal mean/variance analysis;
   - model read noise, shot-noise contribution and gain where identifiable in the app-visible domain.

4. **Spatial non-uniformity**
   - PRNU-like residual response;
   - DSNU/dark pattern;
   - row/column structure;
   - hot/defect pixels;
   - screen for upstream processing before interpreting these as pure sensor quantities.

5. **Mode-specific noise profile**
   - 12.5MP, 50MP and 200MP modes need independent validation;
   - do not transfer a noise profile merely because physical camera ID and focal length match.

6. **Temperature/readout dependence**
   - capture temperature/thermal state where available;
   - characterise if changes are scientifically meaningful.

A calibration can improve certainty and physical fidelity. It does not create information absent from the exposure.

---

# 6. Sharpness, detail and optics

## 6.1 Megapixels are sampling, not guaranteed scene detail

ISO 12233:2024 defines methods for camera resolution and spatial frequency response (SFR). TruthRaw should use SFR/MTF/PSF-style characterisation rather than equating advertised megapixels with independent optical detail.

The detail chain is approximately:

`scene structure -> illumination/material contrast -> lens PSF/MTF -> focus/motion -> sensor sampling/CFA -> app-visible upstream processing -> reconstruction -> appearance sharpening`

These stages must not be collapsed.

## 6.2 Honor Camera-5 scale

Project metadata records an approximately 9.1392 mm sensor width across the 16320-sample maximum-resolution lattice, which corresponds to about 0.56 micrometre per sample pitch in that app-visible geometry.

For a simple diffraction reference, the Airy-disc diameter for a circular aperture is approximately:

`d ~= 2.44 * wavelength * f-number`

At f/2.6 and green light around 520-550 nm, this is roughly 3.3-3.5 micrometres. This is several 0.56-micrometre sample pitches wide.

This does **not** prove that the 200MP route has no value. It means:

- 200.54 million samples are not automatically 200.54 million independent optical details;
- oversampling can still improve CFA reconstruction, alias control, PSF estimation and downsample quality;
- effective resolution must be measured, not inferred from dimensions.

## 6.3 Required matched-resolution test

After the real 200MP RAW capture gate closes, capture controlled matched scenes in 4080x3072, 8160x6144 and 16320x12288 modes where possible.

Measure:

- SFR/MTF centre;
- mid-field;
- corners;
- tangential/sagittal differences where the target setup permits;
- chromatic aberration;
- focus repeatability;
- flare/veiling glare;
- texture/detail survival versus noise;
- downsampled 200MP -> 50MP / 12.5MP performance.

Any inverse lens correction/deconvolution must be bounded by the measured transfer function and uncertainty. It must not amplify unsupported frequencies and relabel them as measured detail.

---

# 7. Restoration science and TruthRaw

Professional conservation/restoration provides a useful epistemic model for TruthRaw.

The American Institute for Conservation requires examination/documentation before intervention and states that compensation for loss should be documented, detectable and should not falsely modify known characteristics of the original object.

TruthRaw should preserve the same logic digitally:

- source evidence is retained;
- interventions/reconstruction are documented;
- reconstruction is distinguishable from measurement;
- absence of evidence remains visible in the scientific state even if a presentation is visually complete;
- appearance edits remain reversible and downstream.

The Library of Congress conservation-imaging workflow also separates normal illumination from raking, transmitted, specular, polarized, UV and IR imaging modes. Those physical modes can provide genuinely new measurements because new photons/modalities are captured.

TruthRaw's **Virtual Observation / Multi-Light / EV probing** is different: it re-examines one existing exposure numerically. It may reveal structure to the analyst/algorithm but does not create new independent evidence.

Permanent distinction:

> **new physical modality/exposure may add evidence; a virtual re-parameterisation of the same RAW does not.**

This is why restoration ideas strengthen, rather than weaken, the measured/reconstructed/unknown separation.

---

# 8. 200MP Camera-5 state: what is already proven

The current project evidence establishes the following static Camera2 route for the HONOR BKQ-N49 tele camera:

- physical/public camera ID: `5`;
- focal length: approximately `22.48 mm`;
- aperture: `f/2.6`;
- CFA reported: BGGR;
- default RAW_SENSOR: `4080x3072`;
- maximum-resolution map normal RAW_SENSOR: `8160x6144`;
- maximum-resolution high-resolution RAW_SENSOR: `16320x12288`;
- maximum pixel/active geometry reaches `16320x12288`;
- `ULTRA_HIGH_RESOLUTION_SENSOR` is advertised;
- `REMOSAIC_REPROCESSING` is not advertised;
- `SENSOR_INFO_BINNING_FACTOR = 2x2` is nevertheless reported, creating an Android-contract/implementation tension;
- `SENSOR_INFO_LENS_SHADING_APPLIED = true`.

Current status remains:

- `FULL_SENSOR_MAXIMUM_RESOLUTION_CAPABILITY_PROVEN`: PASS;
- actual 16320x12288 payload proof: OPEN until a real device capture returns the image + matching TotalCaptureResult + payload evidence.

The old reconstruction-first 8x8/Tetra-style physical-topology hypothesis is superseded as a device topology claim. Do not revive it merely from dimensional arithmetic.

---

# 9. Audit of the existing v0.7 200MP proof code

The existing independent probe under:

`capture/android/camera5-200mp-probe-v07`

already implements the most important pieces correctly.

Verified from `MainActivity.kt` on the continuity branch:

- selects Camera ID 5 and the exact 16320x12288 RAW_SENSOR candidate;
- enumerates the maximum-resolution high-resolution output list separately;
- uses `ImageReader(..., maxImages=1)` for the enormous RAW_SENSOR buffer;
- creates an `OutputConfiguration` for the RAW surface;
- calls `addSensorPixelModeUsed(SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)`;
- sets `CaptureRequest.SENSOR_PIXEL_MODE = MAXIMUM_RESOLUTION`;
- attempts to disable ZSL/NR/hot-pixel/shading/edge/aberration/distortion/stabilisation where advertised;
- hashes and writes the direct `Image.Plane` buffer without first making a ~401MB Kotlin/Java ByteArray;
- records width/height/rowStride/pixelStride/payload bytes/SHA-256;
- binds `Image.timestamp` to `CaptureResult.SENSOR_TIMESTAMP`;
- records applied sensor pixel mode, noise profile, dynamic black/white and controls;
- creates DNG only as a derivative convenience container while keeping the raw buffer as primary evidence.

The runtime gate also correctly does **not** require `SENSOR_RAW_BINNING_FACTOR_USED` to be true or non-null in order to pass the 200MP payload proof.

Therefore the 200MP work is not conceptually broken and does not need a rewrite from zero.

---

# 10. Important Android Camera2 clarification: binning semantics

Two Android fields must never be conflated:

## `SENSOR_INFO_BINNING_FACTOR`

CameraCharacteristics type: `Size`.

It describes dimensions of same-colour pixel groups for the relevant UHR sensor model.

Android documentation states that on an `ULTRA_HIGH_RESOLUTION_SENSOR` device this key normally is not present when `REMOSAIC_REPROCESSING` is absent, because app RAW targets then have a regular Bayer pattern.

The HONOR BKQ-N49 nevertheless reports `2x2` while not advertising remosaic reprocessing. Treat that as **observed metadata**, not proof of hidden physical topology.

## `SENSOR_RAW_BINNING_FACTOR_USED`

CaptureResult type: optional `Boolean`.

It indicates whether the requested RAW image uses the Bayer grouping described by `SENSOR_INFO_BINNING_FACTOR`.

Android documents this key for UHR devices that also advertise `REMOSAIC_REPROCESSING`; it may be null.

Therefore:

> **A null/absent `SENSOR_RAW_BINNING_FACTOR_USED` on this Honor is not by itself a Step-3B failure.**

The app-visible RAW geometry, applied sensor pixel mode, CFA contract, payload, timestamp and provenance are the decisive capture evidence.

---

# 11. Step 3B — authoritative next physical experiment

The next decisive experiment remains one real Camera-5 16320x12288 `RAW_SENSOR` capture.

Required procedure:

1. Query and preserve the complete Camera-5 characteristics inventory immediately before capture.
2. Select only the 16320x12288 `RAW_SENSOR` entry from the maximum-resolution **high-resolution** list.
3. Use a bounded `ImageReader`; `maxImages=1` is appropriate for the first proof.
4. Bind the output to maximum-resolution sensor pixel mode with `OutputConfiguration.addSensorPixelModeUsed(...)` where the API/device path uses it.
5. Set the still request itself to `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION`.
6. Keep the proof request minimal: RAW evidence first; do not add unnecessary preview/JPEG/reconstruction targets to the same still request.
7. Persist requested controls and applied `TotalCaptureResult`; request intent alone is not proof.
8. Require returned dimensions `16320x12288` and applied result pixel mode `MAXIMUM_RESOLUTION`.
9. Require exact `Image.timestamp == SENSOR_TIMESTAMP` for the result used as evidence.
10. Preserve exact accessible source buffer bytes first; record SHA-256, rowStride, pixelStride and byte count. If padding exists, retain that exact `.rawbuffer` before producing any normalized active `.rawsensor`.
11. Record CFA, black/white, NoiseProfile, exposure, ISO, lens/focus state, shading/NR/hot-pixel/edge/aberration/distortion controls, active arrays and relevant physical-camera identity.
12. A DNG may be generated from the same Image + matching characteristics/result, but remains a derivative container. It does not outrank the preserved raw buffer.
13. Classify a successful result as an **app-visible maximum-resolution RAW_SENSOR measurement**, not as untouched per-photodiode ADC truth.

Recommended successful classification wording:

`APP_VISIBLE_MAXIMUM_RESOLUTION_200MP_RAW_SENSOR_CAPTURE_PROVEN`

Do **not** automatically claim:

- untouched photodiode ADC codes;
- no on-sensor processing;
- no HAL processing;
- one independently converted value per physical advertised photodiode;
- a hidden physical Quad/Tetra CFA topology;
- FULL_PHYSICAL colour;
- FULL_PHYSICAL optics;
- electron-domain calibration.

---

# 12. What comes immediately after a successful 200MP capture

A successful payload closes only the acquisition gate. Then build a matched physical calibration campaign.

## Phase A — domain equivalence / mode comparison

Capture controlled 4080x3072, 8160x6144 and 16320x12288 series and compare:

- field of view/crop;
- CFA phase/geometry;
- black/white behaviour;
- exposure/gain consistency;
- noise statistics;
- spatial correlations;
- row/column patterns;
- aliasing/remosaic signatures;
- actual information gain after matched resampling.

Do not assume a simple 4x4 or 2x2 physical forward operator until measurements support it.

## Phase B — noise/electron-domain characterisation

Use dark and stable flat-field exposure series. Build mode-specific estimates of:

- temporal read/dark noise;
- signal-dependent noise;
- gain/linearity where identifiable;
- saturation/clipping;
- PRNU/DSNU-like app-visible residuals;
- defect pixels;
- temperature dependence.

## Phase C — shading and optics

Use controlled uniform flats and SFR/edge targets to measure:

- residual luminance shading;
- residual colour shading;
- PSF/SFR/MTF centre-to-corner;
- chromatic aberration;
- distortion geometry;
- flare/veiling glare;
- focus repeatability.

Because `lensShadingApplied=true`, call the measured Camera2 result **residual app-visible shading** unless independent evidence supports a stronger physical interpretation.

## Phase D — physical colour

Use a spectrally characterised colour target under multiple characterised illuminants. Fit and validate colour transforms with held-out patches/illuminants and retain error/uncertainty, instead of promoting one matrix as universal colour truth.

## Phase E — material truth stress scenes

Keep the project's water/material tests. Include difficult materials such as:

- water droplets / splashes;
- glass / transparent material;
- glossy paint;
- metal;
- skin;
- fine foliage/texture;
- saturated colours;
- strong point highlights;
- very dark surfaces next to bright sources.

These scenes stress specular/transmissive behaviour, highlight censoring, chroma noise, microdetail and over-sharpening simultaneously.

---

# 13. Consequences for the reconstruction architecture

The 200MP path and the Free Scientific Space doctrine reinforce each other.

TruthRaw should not force the 200MP measurement into a 10-bit-looking final world. Nor should it assume that the 200MP sampling lattice is itself the final scene lattice.

The Scientific Master may use:

- a different spatial lattice if supported by the forward model;
- real-valued scene coordinates outside source code bounds;
- values above a clipped source WhiteLevel as reconstructed lower-bounded estimates with uncertainty;
- signed estimator coordinates where mathematically useful;
- TruthRange/log coordinates for positive-light representation;
- full covariance/support/censor metadata;
- float32, float64 or higher precision implementations without changing the scientific semantics;
- a scene colour basis independent of the source camera's final export basis.

The correct limiting question is never:

> "Can RAW10 / DNG / float32 store this?"

It is:

> **"What does the evidence support, and what representation/precision best preserves that knowledge without introducing material numerical error?"**

---

# 14. Next-chat bootstrap for this subject

If the next chat continues 200MP or physical calibration, read in this order:

1. `docs/TRUTHRAW_EXPERTISE_FOUNDATION_FREE_SPACE_200MP_2026-09-15.md`
2. `docs/PROJECT_HISTORY_AND_FOTOGRAAF_CONTEXT_2026-09-15.md`
3. `docs/full-sensor/FULL_SENSOR_RESEARCH_REPORT_v0_7.md`
4. `capture/android/camera5-200mp-probe-v07/app/src/main/java/com/truthraw/fullsensorprobe/MainActivity.kt`
5. `tools/camera5_200mp_runtime_gate_v07.py`
6. the most recent FotoGraaf acquisition-domain evidence from Camera 5
7. only then implement/modify Step 3B.

Do not restart with the old virtual-200MP topology hypothesis. Do not treat session support as delivered-payload proof. Do not let calibration blockers shrink the architecture back to source-container limits.

---

# 15. External technical references used for this expertise foundation

These references are supporting technical literature/standards. They do not replace TruthRaw's own evidence gates.

- Android Camera2 `CaptureResult` — `SENSOR_RAW_BINNING_FACTOR_USED`, `SENSOR_PIXEL_MODE`: https://developer.android.com/reference/android/hardware/camera2/CaptureResult
- Android Camera2 `CameraCharacteristics` — `SENSOR_INFO_BINNING_FACTOR`, maximum-resolution sensor geometry: https://developer.android.com/reference/android/hardware/camera2/CameraCharacteristics
- Android Camera2 `OutputConfiguration` — `addSensorPixelModeUsed`, physical-camera output binding: https://developer.android.com/reference/android/hardware/camera2/params/OutputConfiguration
- Android `TotalCaptureResult`: https://developer.android.com/reference/android/hardware/camera2/TotalCaptureResult
- ISO 17321-1:2012 — digital still camera colour characterisation: https://www.iso.org/standard/56537.html
- ISO 17957:2015 — digital-camera shading measurements: https://www.iso.org/standard/31974.html
- ISO 15739:2023 — electronic still-picture noise measurements: https://www.iso.org/standard/82233.html
- ISO 12233:2024 — digital-camera resolution and spatial frequency responses: https://www.iso.org/standard/88626.html
- EMVA 1288 Release 4.0 downloads/standard: https://www.emva.org/standards-technology/emva-1288/emva-standard-1288-downloads-2/
- American Institute for Conservation — Code of Ethics and Guidelines for Practice: https://www.culturalheritage.org/conservation-at-work/uphold-professional-standards/code
- Library of Congress — Digital Imaging Workflow for Treatment Documentation: https://www.loc.gov/preservation/resources/ImageDoc/
- Java Virtual Machine specification — float/double and IEEE-754 binary32/binary64: https://docs.oracle.com/javase/specs/jvms/se16/html/jvms-2.html
- GCC additional floating types (`_Float128`, `__float128` target-dependent): https://gcc.gnu.org/onlinedocs/gcc/Floating-Types.html
- GNU MPFR arbitrary-precision floating point: https://www.mpfr.org/
- Vulkan `VkPhysicalDeviceFeatures::shaderFloat64`: https://registry.khronos.org/vulkan/specs/latest/man/html/VkPhysicalDeviceFeatures.html

---

# Final project statement

TruthRaw is not a RAW-bound image editor.

It preserves the source measurement exactly enough to know what was actually observed, then reconstructs a separate **Free Scientific Space** whose numerical range, precision, colour basis, spatial representation and export format are not forced to inherit the arbitrary limits of the source container.

The freedom is representational and computational; the honesty remains evidential.

The immediate physical priority is still the same:

> **Obtain and prove one real Camera-5 16320x12288 app-visible RAW_SENSOR frame in MAXIMUM_RESOLUTION mode, then calibrate the 200MP domain rather than guessing it.**
