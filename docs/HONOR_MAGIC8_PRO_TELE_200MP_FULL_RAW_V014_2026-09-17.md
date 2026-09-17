# HONOR Magic 8 Pro tele 200 MP full-resolution RAW — proven Camera2 route v0.14

Date: 2026-09-17  
Status: **CURRENT DEVICE ACQUISITION AUTHORITY / PHYSICAL RUN PROVEN**  
Device family: HONOR Magic 8 Pro / BKQ-N49  
Logical camera: `0`  
Physical tele camera: `5`

This document supersedes the earlier repository state that described the 16320x12288 Camera-5 RAW_SENSOR route as capability-visible but still awaiting a qualifying physical payload.

The physical gate is now closed for one specific claim:

> **Physical camera 5 can deliver one app-visible Android Camera2 `RAW_SENSOR` frame at 16320x12288 through logical camera 0, with physical-Camera-5 capture-result binding, exact Image/result timestamp equality, and the original Image.Plane[0] payload preserved and SHA-256 sealed.**

This does **not** prove untouched photodiode/ADC output, absence of sensor/HAL remosaic or other upstream processing, 200 million independent photodiode ADC readings, electron calibration, or 200 MP optical resolving power.

Permanent evidence boundary:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`

## 1. Why earlier builds missed the route

The 200 MP RAW route is not obtained by inspecting only the ordinary stream map or only `MAXIMUM_RESOLUTION.getOutputSizes(RAW_SENSOR)`.

The successful inventory must keep all four RAW_SENSOR lists separate:

1. `standardMap.getOutputSizes(ImageFormat.RAW_SENSOR)`
2. `standardMap.getHighResolutionOutputSizes(ImageFormat.RAW_SENSOR)`
3. `maximumMap.getOutputSizes(ImageFormat.RAW_SENSOR)`
4. `maximumMap.getHighResolutionOutputSizes(ImageFormat.RAW_SENSOR)`

On the tested device the decisive route is number 4:

`MAXIMUM_MAP_HIGH_RESOLUTION -> RAW_SENSOR 16320x12288`

Observed v0.10 inventory:

- standard output: `4080x3072`
- standard high resolution: empty
- maximum-resolution output: `8160x6144`
- maximum-resolution high resolution: `16320x12288`

Therefore an implementation that omits `maximumMap.getHighResolutionOutputSizes(ImageFormat.RAW_SENSOR)` can falsely conclude that 200 MP RAW_SENSOR is unavailable.

## 2. Correct camera topology

Do not assume that physical camera `5` should always be opened directly.

For the proven HONOR route:

- open logical camera `0`;
- verify that physical camera `5` is a child of the logical camera;
- use the logical session as the parent session;
- bind the 200 MP RAW output to physical camera `5` with `OutputConfiguration.setPhysicalCameraId("5")`;
- create the still request as physical-camera scoped to `5` where the platform accepts that form.

The preview-stage observation at 3.7x reported `activePhysical=5`, which is useful route telemetry, but preview selection is not the primary proof of the final RAW frame. The final proof comes from the physical Camera-5 result bound to the received RAW Image.

## 3. Maximum-resolution stream configuration

For the 16320x12288 RAW target:

- create the ImageReader for exact `16320x12288`, format `RAW_SENSOR`;
- create an `OutputConfiguration` for its surface;
- set physical camera id `5`;
- call `addSensorPixelModeUsed(CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)`;
- create a `SessionConfiguration` containing that output;
- use `CameraDevice.isSessionConfigurationSupported()` where available as a preflight check, but do not convert an unavailable/throwing support-query into fabricated proof either way.

The target dimensions must remain exact. Do not silently substitute 8160x6144, 4080x3072, JPEG, YUV, RAW10, or a software-upscaled raster and call it the same evidence class.

## 4. Request construction and HONOR-specific observations

The successful development sequence exposed an important framework/vendor asymmetry.

Observed during the v0.12-v0.14 route:

- global request `SENSOR_PIXEL_MODE=MAXIMUM_RESOLUTION`: not writable on logical camera 0 in this path (`globalMAX=false`);
- physical Camera-5 builder setting: writable/readable as MAX (`physicalMAX=true`);
- `SENSOR_PIXEL_MODE` was not advertised as an available physical override (`physicalOverrideAdvertised=false`);
- the eventual physical Camera-5 `TotalCaptureResult` reported `SENSOR_PIXEL_MODE=0` even though the maximum-resolution stream/request path produced the exact 16320x12288 RAW Image.

These facts must be recorded as observations, not normalized away.

Do not require `globalMAX=true` as a pre-submit condition on this device. That blocked v0.12 before Android/HONOR could answer.

Do not use returned physical `SENSOR_PIXEL_MODE==MAXIMUM_RESOLUTION` as a post-capture prerequisite for preserving the payload. On the qualifying v0.14 run the returned value was `0`, while the exact 16320x12288 Image and exact physical Camera-5 timestamp binding were already present.

## 5. Correct evidence ordering

This ordering is mandatory because v0.13 demonstrated that a valid physical frame can be destroyed by a later metadata interpretation gate.

After an Image arrives:

1. require exact dimensions `16320x12288`;
2. require a physical capture result for camera `5`;
3. require exact equality between `Image.timestamp` and physical Camera-5 `SENSOR_TIMESTAMP`;
4. inspect plane count/strides/buffer bounds;
5. **persist and SHA-256 seal the original `Image.Plane[0]` buffer before interpreting advisory/contradictory result metadata**;
6. only after sealing, record returned pixel mode, binning metadata, NR/edge metadata and other result fields;
7. create DNG only as an auxiliary container after the original payload is safe.

A later metadata mismatch may lower or qualify the interpretation. It must not erase already acquired primary evidence.

## 6. Qualifying v0.14 physical frame

The qualifying run produced:

- physical result camera id: `5`
- received Image: `16320x12288`
- sample count: `200,540,160`
- Image timestamp: `691266629782500 ns`
- physical Camera-5 sensor timestamp: `691266629782500 ns`
- timestamp relation: exact equality
- reported tele focal length: approximately `22.48 mm`
- plane count: one RAW plane for the primary payload path
- pixel stride: `2` bytes
- row stride: `32640` bytes
- expected active row bytes: `16320 * 2 = 32640`
- original payload bytes: `401,080,320`
- canonical contiguous RAW_SENSOR condition: true
- original RAW payload SHA-256: `af3ad73e5919b816881a661f00ffd84a7b537f23198a5877c242717c5d7526de`
- auxiliary DNG creation: successful
- auxiliary DNG size: `401,184,072` bytes

The primary payload is the original app-visible `Image.Plane[0]`, not the DNG.

## 7. `.rawsensor` versus `.rawbuffer`

A capture may be labelled `.rawsensor` only when the preserved primary payload is canonical contiguous for this route:

- width `16320`
- height `12288`
- pixelStride `2`
- rowStride `32640`
- payload size exactly `401,080,320` bytes

If row padding or another layout difference exists, preserve the untouched buffer as `.rawbuffer` instead. Do not rewrite it in-place to make it look canonical.

Any normalization must happen off-device as a derived artifact with:

- source-file identity;
- source SHA-256;
- exact stride/layout description;
- deterministic normalization procedure;
- separate normalized SHA-256.

The original buffer remains the primary evidence object.

## 8. DNG role

`DngCreator` output is useful for ecosystem compatibility and metadata inspection, but it is secondary evidence here.

Required ordering:

`original Image.Plane[0] -> seal/hash -> evidence JSON -> optional auxiliary DNG`

DNG creation failure must not invalidate an otherwise valid primary RAW capture. Conversely, a successful DNG alone must never substitute for proof that the original Camera2 RAW payload was received and preserved.

## 9. Failure ladder and what each version taught us

### v0.9

Stage 1 falsely blocked because the staged app omitted `maximumMap.getHighResolutionOutputSizes(RAW_SENSOR)`.

Lesson: inventory all four stream-map routes.

### v0.10

Stage 1 found `maximum.high=[16320x12288]`; Stage 2 confirmed logical 0 -> active physical 5. Stage 3 still failed.

Lesson: capability and preview routing are necessary observations, not capture proof.

### v0.11

Android rejected the request with a sensor-pixel-mode/stream consistency error.

Lesson: maximum-resolution stream configuration and request scoping must agree.

### v0.12

Observed `globalMAX=false`, `physicalMAX=true`, `physicalOverrideAdvertised=false`, but an overly strict app-side gate required global MAX and blocked before submit.

Lesson: do not turn absence of the logical global key into proof that the physical maximum-resolution route is impossible.

### v0.13

The app received the exact 16320x12288 Image, physical Camera-5 result and exact timestamp binding, but then rejected `physical SENSOR_PIXEL_MODE=0` before persisting the buffer.

Lesson: primary evidence must be sealed before secondary metadata interpretation.

### v0.14

The app seals the original RAW payload first and records the pixel-mode mismatch afterward. This produced the qualifying 401,080,320-byte physical-run payload and evidence record.

## 10. What is proven

The v0.14 physical run supports all of the following together:

- the device advertises 16320x12288 RAW_SENSOR in the maximum-resolution high-resolution map;
- logical camera 0 can host the capture route;
- the RAW output is bound to physical tele camera 5;
- one actual Image of 16320x12288 was delivered;
- one physical Camera-5 TotalCaptureResult was available for that capture;
- Image and physical sensor timestamps were exactly equal;
- the original Image.Plane[0] payload was persisted and SHA-256 sealed;
- the qualifying payload is canonical contiguous under the observed stride facts.

Recommended short classification:

`APP_VISIBLE_PHYSICAL5_200MP_RAW_SENSOR_CAPTURE_PROVEN`

This classification is narrower than untouched/native sensor truth.

## 11. What is explicitly not proven

Do not promote the v0.14 result to any of the following without new independent evidence:

- `UNTOUCHED_NATIVE_200MP_ADC`;
- one independent ADC measurement per output sample;
- absence of sensor-internal or HAL-side remosaic/reconstruction;
- absence of any upstream processing before the app-visible RAW_SENSOR buffer;
- electron-domain calibration;
- independently measured black/white/noise/PTC truth across the full operating range;
- 200 MP optical resolving power;
- full physical colour/spectral truth.

The returned physical `SENSOR_PIXEL_MODE=0`, `rawBinningFactorUsed=true`, and returned NR/edge metadata are retained as evidence fields requiring interpretation. They must not be silently rewritten into a cleaner story, but neither should they alone be treated as proof that the RAW pixel values were sharpened, denoised or classically binned.

## 12. Next scientific work on the payload

The next phase is not another capability probe. It is sample-domain analysis of the real primary RAW payload.

Required investigations include:

- 16-bit container usage and effective bit depth;
- possible left/right shifts or packing conventions;
- value histogram and saturation/black behavior;
- Bayer phase/topology verification;
- row/column periodicity and spatial correlations;
- repeated-value and interpolation signatures;
- evidence for/against remosaic, resampling or synthetic expansion;
- black/white-level consistency;
- NoiseProfile and noise statistics where metadata supports the comparison;
- binning-factor metadata versus observed sample-domain topology;
- spatial-frequency/SFR/MTF testing using suitable physical targets before claiming 200 MP optical detail.

These are separate gates. Passing the raster/capture gate does not pre-answer them.

## 13. Host-side validation update required

Historical host gate `tools/camera5_200mp_runtime_gate_v07.py` encodes the older assumption that camera 5 must be opened directly. That assumption is incompatible with the proven v0.14 logical-0 -> physical-5 route.

Do **not** weaken or rewrite v0.7 in-place. Preserve it as historical evidence logic and add a successor gate that accepts the proven topology only when all required bindings are present.

The successor gate must require at least:

- opened logical camera `0`;
- output physical camera `5`;
- physical result camera `5`;
- exact 16320x12288 dimensions;
- exact Image/result timestamp equality;
- primary RAW payload path and SHA-256;
- pixelStride `2`;
- rowStride at least `32640`;
- exact payload identity/length facts;
- maximum-resolution capability/output-configuration evidence;
- preservation of returned result pixel-mode mismatch as an observation rather than a fabricated MAX result;
- physical frame count `1`.

The gate may promote only to app-visible physical-5 200 MP RAW_SENSOR capture proof. It must retain the untouched-ADC boundary.

## 14. On-device operating procedure

For a repeatable physical capture:

1. install the staged FotoGraaf build containing the v0.14-or-later evidence ordering;
2. keep sufficient free storage; approximately 1 GB free space is a practical minimum when retaining the original ~401 MB RAW plus an auxiliary ~401 MB DNG and metadata;
3. run Stage 1 and require the exact target under `maximum.high`;
4. run Stage 2 and record logical/physical route telemetry;
5. run Stage 3;
6. save/export the **original 200 MP RAW buffer first**;
7. save/export the evidence JSON second;
8. save/export DNG only as an auxiliary artifact;
9. verify the exported RAW SHA-256 against the evidence record before off-device analysis.

A dark preview is not by itself evidence of capture failure. The final still-capture evidence is determined by the received RAW Image and its bound physical capture result.

## 15. Project integration rule

All later TruthRaw reconstruction, uncertainty, colour, HDR, detail and export work must consume this capture through normal provenance authority. The 200 MP raster increases app-visible sample support; it does not automatically increase epistemic authority for optics, colour, noise, missing-channel reconstruction or untouched-sensor claims.

Permanent rule:

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
