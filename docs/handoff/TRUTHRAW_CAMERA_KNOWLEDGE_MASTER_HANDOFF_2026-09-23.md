# TruthRaw Camera Knowledge Master Handoff

Date: 2026-09-23  
Project: TruthRaw  
Device focus: HONOR Magic 8 Pro / Android 17 beta / BKQ-N49  
Camera focus: logical Camera 0, physical tele Camera 5  
Purpose: final technical handoff of camera, RAW, Camera2, HONOR vendor-route and TruthRaw scientific knowledge accumulated through the v0.73 research line.

> This document is documentation-only. No production or research source code was changed to create this handoff.

---

## 1. Project state and frozen production

Frozen production reference:

- branch: `integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master`
- frozen commit: `172a100786eb18d4b08564bbfb025a44f42cfa1e`
- frozen APK: `TruthRaw_v0.84.3_Final_Freeze_TruthNegative_200MP_FullColour_debug_arm64.apk`
- APK bytes: `6,095,333`
- APK SHA-256: `5805d291b163d66e68d5ab98aa4d2971e38b062b724325469d6002fbd8b1917e`

The frozen production branch must remain untouched unless a future operator receives explicit authorization to change production.

Research after the freeze was isolated on test/research branches.

Permanent scientific law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

---

## 2. TruthRaw scientific boundary

The camera work established several permanent interpretation rules.

### 2.1 App-visible RAW is not native-ADC proof

An Android Camera2 RAW buffer can prove that an app received a specific buffer through a specific Camera2 route.

It does not by itself prove:

- untouched photodiode ADC values;
- native physical photosite count;
- native sensor geometry;
- native ADC bit depth;
- absence of sensor-side remosaic or reconstruction;
- absence of HAL/vendor preprocessing;
- optical resolving power equal to the output raster.

Therefore phrases such as “200 MP RAW” must always be qualified by the exact evidence class.

### 2.2 Single-frame evidence

For a qualifying TruthRaw scientific capture:

- `physicalFrameCount=1`
- `independentEvidenceCount=1`

A larger output representation, Float32 master or reconstructed space does not create extra independent evidence.

### 2.3 Source / master / appearance separation

Keep separate:

1. immutable source CFA/sample bytes + capture metadata;
2. de-ISP / measurement interpretation;
3. Scientific Master;
4. uncertainty / Dynamic Authority;
5. Open Scene / Free Scientific Space;
6. TruthNegative / projection;
7. appearance and export.

Appearance must never write back as new measurement authority.

### 2.4 Black and white interpretation

- DNG/Camera2 BlackLevel is a readout/code-domain offset quantity.
- It is not TruthRange zero.
- TruthRange zero uses a scene-reference gauge `L0`.
- WhiteLevel is a clipping/censoring threshold, not maximum possible world brightness.
- A clipped measurement is lower-bound evidence, not “known maximum light”.
- Negative numerical estimates are not negative physical light.

### 2.5 Precision

TruthRaw may use Float32, Float64, signed values, values above 1 and wider scientific range.

This increases representational headroom and numerical quality.

It does not add independent source evidence.

---

## 3. Device camera map

Known Camera2 camera IDs used throughout the project:

- Camera 0: logical rear camera host
- Camera 1: front camera
- Camera 2: main rear physical camera
- Camera 4: wide rear physical camera
- Camera 5: tele rear physical camera

Historical lens mapping used in the project:

- Main 1.0x -> system ID 2
- Wide 0.6x -> system ID 4
- Tele 3.7x -> system ID 5
- Front 1.0x -> system ID 1

Physical tele Camera 5 focal length observed repeatedly:

- approximately `22.48 mm`

The proven high-resolution topology for the tele route uses logical Camera 0 as the opened parent and physical Camera 5 as the bound physical output/result camera.

Do not assume that opening Camera 5 directly is equivalent to the stock/proven route.

---

## 4. Public Camera-5 RAW capability map

Public characteristics for physical Camera 5 established:

### Standard sensor / RAW10

- STANDARD `RAW_SENSOR`: 4080 x 3072
- STANDARD `RAW10`: 4080 x 3072

### Maximum-resolution map

- MAXIMUM_RESOLUTION `RAW_SENSOR`: 8160 x 6144
- MAXIMUM_RESOLUTION `RAW10`: 8160 x 6144

### Maximum high-resolution route

- high-resolution `RAW_SENSOR`: 16320 x 12288
- high-resolution `RAW10`: 16320 x 12288

### Sensor arrays

- standard sensor array: 4080 x 3072
- max pixel array: 16320 x 12288
- standard `android.sensor.info.binningFactor`: 2 x 2

Important:

The 4080 -> 16320 dimension ratio alone is not evidence of physical 4x4 binning.

---

## 5. Proven Camera2 high-resolution route

The high-resolution Camera2 route that repeatedly works is:

1. open logical Camera 0;
2. verify Camera 5 is a physical child;
3. create the target RAW output;
4. bind the output to physical Camera 5 using `OutputConfiguration.setPhysicalCameraId("5")`;
5. for maximum-resolution output call `addSensorPixelModeUsed(SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)`;
6. create a physical-scoped still request where accepted;
7. write physical `CaptureRequest.SENSOR_PIXEL_MODE=MAXIMUM_RESOLUTION`;
8. bind the delivered Image to the physical Camera-5 result by exact timestamp equality.

Observed HONOR asymmetry:

- physical MAX write/readback can succeed on the request;
- the returned physical capture result may still report `SENSOR_PIXEL_MODE=0`;
- this returned 0 must be preserved as evidence rather than rewritten;
- it does not invalidate an already received and timestamp-bound RAW Image.

---

## 6. Historical 16320x12288 RAW_SENSOR lesson

An early 16320x12288 RAW_SENSOR capture produced a declared 401,080,320-byte U16-like envelope.

Later byte-topology analysis showed that only the first 25,067,520 bytes carried the expected 4080x3072-sized populated prefix, with the remaining declared high-resolution envelope not carrying independent populated sample support.

Therefore the earlier broad wording “physical Camera-5 200MP RAW_SENSOR capture proven” was narrowed by later topology evidence.

The lasting lesson:

**Envelope dimensions are not equivalent to populated sample geometry.**

Primary evidence must be analyzed at the byte/sample population level.

---

## 7. RAW10 control and population topology

### 7.1 v0.58 STANDARD control

Physical Camera 5, 4080 x 3072 RAW10:

- payload bytes: 15,728,640
- rowStride: 5120
- packed image bytes per row: 5100
- zero padding per row: 20 bytes
- all 3072 rows populated
- black level: 64
- white level: 1023
- noise reduction: 0
- edge: 0
- `rawBinningFactorUsed=true`

Classification:

`RAW10_SEALED_ANALYSIS_ONLY_NO_U16_ADMISSION`

### 7.2 8160 x 6144 RAW10

Repeated 8160 x 6144 RAW10 captures used a 62,914,560-byte declared envelope, but only the first 15,728,640 bytes were meaningful.

The populated prefix reinterprets exactly as:

- 3072 rows
- 5120 bytes per effective row
- 5100 RAW10 packed bytes
- 20 zero padding bytes

Everything after the effective prefix was exact zero.

Classification:

`APP_VISIBLE_4080x3072_PACKED_RAW10_PAYLOAD_EMBEDDED_IN_8160x6144_CAMERA2_ENVELOPE`

### 7.3 16320 x 12288 RAW10

The v0.59 line is the strongest repeated topology observation.

Declared envelope:

- width: 16320
- height: 12288
- rowStride: 20400
- total accessible bytes: 250,675,200

Repeated population facts:

- first nonzero byte: 0
- last nonzero byte: 15,728,619
- effective populated boundary: 15,728,640
- tail bytes after boundary: 234,946,560
- tail is exact zero
- tail fraction: approximately 93.72549%
- effective prefix: 3072 x 5120 bytes
- effective packed bytes per row: 5100
- effective zero pad per row: 20
- effective pad bytes across 3072 rows: 61,440
- all effective pad bytes zero
- all 3072 effective rows contain image data
- highest declared 20,400-byte row containing nonzero data: 771

Classification:

`APP_VISIBLE_4080x3072_PACKED_RAW10_PAYLOAD_EMBEDDED_IN_16320x12288_CAMERA2_ENVELOPE`

This population boundary was repeated across multiple independent single-frame captures.

---

## 8. Repeated v0.59 evidence

Distinct v0.59 RAW10 source SHA-256 values included:

- `e058d33821bb3ec66242c6618c0e0083bf597645e31089e4877ecd3e31bb1ee6`
- `b7e8227150cff52b55d8434c0c1d3c86caac9d90985917eff99fd0515ce7c813`
- `16ebfb34a49e75b43f3fa2da3960f6e2ae0aae9c0a4328894233cda1c9360b56`
- `b489812c9dd26a2d574fc74e620a2df812e2fb24f17ab879188886139f1e6dc1`
- `2980ca9eb68aa535a6929588205645ec32d9191cac8efdb1c29e1d9655f0377d`
- `d4131dcbb00005714345c1226ea9c2ad18d769af7162efd3b5cf512900217bcc`
- `6e8e47e2c228f719abe6f5da8fbc0931152a6b26e8059d778f431bce9b4dedb9`

All repeated the same effective population boundary.

The high-ISO capture with source SHA:

`6e8e47e2c228f719abe6f5da8fbc0931152a6b26e8059d778f431bce9b4dedb9`

had:

- ISO 102400
- exposure about 70 ms
- dynamic black approximately [64.296875, 64.8125, 64.453125, 65.28125]
- decoded min 0
- decoded max 1023
- many numerical samples below dynamic BlackLevel

Important:

Samples numerically below BlackLevel are not proof of negative physical light or untouched native ADC behavior.

---

## 9. Android 17 / RAW14

Android 17 / API 37 defines `ImageFormat.RAW14` as format 44.

This does not mean every camera exposes RAW14.

On Camera 5, direct public capability probing showed format 44 unsupported for:

- 4080 x 3072
- 8160 x 6144
- 16320 x 12288

Therefore blind iteration on a public Camera2 RAW14 route was deprioritized.

Android 17 vendor-defined Camera Extensions do not, by themselves, provide RAW evidence; those APIs are oriented around extension/processed output support.

---

## 10. HONOR Camera APK identity

Exact static APK analyzed:

- package: `com.hihonor.camera`
- version family: Camera 171.0.10.452
- bytes: 84,167,938
- SHA-256: `3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52`

All OEM semantic claims in this handoff that refer to HONOR .452 are static bytecode findings from this exact APK and remain separate from runtime Camera2 evidence.

---

## 11. HONOR scene meanings

Static bytecode analysis established:

### High-pixel scene mapping

- scene 53 -> UltraHighPixel / 200M
- scene 87 -> UltraHighPixel with Live Photo
- scene 110 -> UltraResolution / 50M

### Pro Photo

`CameraSceneModeUtil.getProPhotoSceneMode()`:

- scene 65 -> Pro Photo JPEG-L branch
- scene 66 -> Pro Photo RAW branch
- fallback -> scene 2

Critical correction:

**scene 66 is not the 200M scene.**

The 200M UltraHighPixel scene remains 53.

---

## 12. v0.50 direct characteristics rerun

For Camera 5, direct typed characteristics reads returned:

- `physicalCameraScene=[66]`
- `sceneCameraIdCapability=[1]`
- `teleSupport=1`
- `rawZoomSupported=1`

while:

- `rawSensorResolution=null`
- `needOpenPhysicalCamera=null`
- `ultraResolutionSwitchSupportedSize=null`

The same `physicalCameraScene=[66]` and `sceneCameraIdCapability=[1]` were also observed for Cameras 0, 2 and 4.

This is consistent with Pro Photo RAW physical-camera capability, not proof of a 200M physical route.

---

## 13. cameraIdCustomInfo

Camera 5 `cameraIdCustomInfo` length:

- 310 integers

For Cameras 0, 2, 4 and 5 the first 90 values formed nine configured groups of ten and the remaining values were zero.

Observed groups:

```
[1, -1, -1, -1, -1, -1, 10, -1, -1, 8]
[1,  0, -1, -1, -1, -1, 10, -1, -1, 8]
[1, 47, -1, -1, -1, -1, 10, -1, -1, 4]
[1, 33, -1, -1, -1, -1,  9, -1, -1, 2]
[1, 59, -1, -1, -1, -1, 10, -1, -1, 8]
[1, 71, -1, -1, -1, -1, 10, -1, -1, 2]
[1, 53, -1, -1, -1, -1, 10, -1, -1, 0]
[1, 65, -1, -1, -1, -1, 10, -1, -1, 6]
[1, 67, -1, -1, -1, -1, 10, -1, -1, 2]
```

Static consumer work linked this characteristic to `CameraServiceFactory.createCameraAbility`, with log wording referring to a scene logical camera-ID list.

The exact meaning of all ten fields was not fully decoded before handoff.

Do not invent per-column semantics.

---

## 14. ServiceHost RAW surface evidence

Static HONOR .452 analysis established a distinct internal RAW capture surface:

- `SURFACE_FOR_CAPTURE_RAW`
- `service_host_capture_raw`
- `rawCaptureHolder`

`NormalProcessor.setRawFormat(...)` selects a RAW capture surface based on capture metadata.

This is important because stock HONOR high-pixel processing may involve an internal ServiceHost path that direct third-party Camera2 does not reproduce.

Safe classification:

`STATIC_SERVICEHOST_DISTINCT_RAW_CAPTURE_SURFACE_PROVEN`

This is not proof that the app-visible Camera2 RAW buffer is the same internal surface or that TruthRaw has reached it.

---

## 15. UltraHighPixel processor pipeline

Static bytecode from:

`UltraHighPixelModeProcessor.getJsonFileName()`

decoded direct switch mappings:

- internal mode 22 -> `pipeline4arcmfnrmscap.json`
- internal mode 23 -> `pipeline4rawmfultrahighpixelcap.json`
- internal mode 24 -> `pipeline4rawmfultrahighpixelcap.json`
- internal mode 31 -> `pipeline4hdrcap.json`
- internal mode 32 -> `pipeline4rawmfultrahighpixelcap.json`
- internal mode 33 -> `pipeline4rawmfultrahighpixelcap.json`

Therefore 23, 24, 32 and 33 are directly tied by executable switch logic to the RAW MF UltraHighPixel processor pipeline.

This is stronger than string proximity.

---

## 16. hintUserValue -> processor bridge

An important static bridge was decoded before v0.73.

CaptureResult key:

`com.hihonor.capture.metadata.hintUserValue`

is a scalar Integer result key.

In `UltraHighPixelMode$2.onCaptureCompleted(...)` the stock app:

1. reads `hintUserValue`;
2. converts it to integer;
3. updates the mode scene value when changed;
4. writes `Key.SMART_SCENE_MODE` into both CaptureFlow and PreviewFlow;
5. ServiceHost forwards that internal key to `CameraService.setSceneMode(int)`;
6. the processor receives the scene mode;
7. modes 23/24/32/33 select `pipeline4rawmfultrahighpixelcap.json`.

Exact bridge:

`CaptureResult hintUserValue -> UltraHighPixelMode.sceneMode -> SMART_SCENE_MODE -> CameraService.setSceneMode -> UltraHighPixelModeProcessor -> ServiceHost JSON pipeline`

The unresolved upstream question is:

**What exact request/mode orchestration causes the HAL/result side to emit hint 23, 24, 32 or 33?**

---

## 17. qcomRemosaicEnable

Exact vendor key:

`com.hihonor.capture.metadata.qcomRemosaicEnable`

Static findings:

- stored in HONOR request-key field `Ls8/c;->A2`;
- Qualcomm route writes it into OEM CaptureFlow;
- Java representation in the stock app is scalar Integer;
- `PhotoResolutionFunction.isRemosaicEnable` is initialized to 1;
- a pre-capture handler writes the current value;
- a remosaic-status callback also updates remosaic state based on prior `hintUserValue`.

Important revised state-machine finding:

- `hintUserValue == 5` -> `isRemosaicEnable = 1`
- otherwise -> `isRemosaicEnable = 0`

Therefore a simplistic “always combine scene 53 + qcomRemosaicEnable=1” is not a faithful reconstruction of the full stock state machine.

The remosaic state is downstream of the previous hint in the decoded path.

---

## 18. MotionCam/vendor session key discovery

The v0.61 work separated logical versus physical Camera-5 key surfaces.

Physical-5-only session/request keys included:

- `android.control.extendedSceneMode`
- HONOR `MasterFilmSensorType`
- HONOR `aoRunningMode`
- HONOR `mmiLaserEyeSafeMode`
- HONOR `videoDynamicFrameRate`
- QTI `EnableVSR`
- QTI `ExtendedMaxZoom`
- QTI `enableQLL`
- QTI `inSensorZoomEnable`

Physical request additionally included:

- `android.sensor.pixelMode`

The first v0.61 default-read approach was limited because it attempted physical-key reads from a builder that was not created with a physical-ID set.

That was an experiment-context failure, not proof that the keys were invalid.

---

## 19. v0.64 physical-targeted template defaults

v0.64 corrected the builder construction using:

`device.createCaptureRequest(TEMPLATE_STILL_CAPTURE, setOf("5"))`

This proved that the physical-targeted builder is legal on the tested device.

Important defaults:

- `android.control.extendedSceneMode` -> scalar Integer 0
- `android.sensor.pixelMode` -> null default
- `enableQLL` -> int[] [0]
- `EnableHDRDCGMode` -> int[] [0]
- `SnapshotHDRMode` -> int[] [0]

Many vendor keys had null template defaults.

Null template default means only that TEMPLATE_STILL_CAPTURE carries no default value for that key. It does not mean unsupported.

---

## 20. v0.65 Camera2 local type oracle

v0.65 resolved direct Camera2 Java/marshaling representation for 19 keys without creating a capture session.

### Scalar Integer

- `android.control.extendedSceneMode`
- `android.sensor.pixelMode`

### int[]

- `MasterFilmSensorType`
- `aoRunningMode`
- `videoDynamicFrameRate`
- `EnableVSR`
- `ExtendedMaxZoom`
- `enableQLL`
- `EnableHDRDCGMode`
- `EnableOfflineHALZSL`
- `EnableAICameraHSR`
- `AICameraMode`
- `SnapshotHDRMode`
- `cameraSceneMode`
- `extStreamSize`

### byte[]

- `inSensorZoomEnable`
- `HDRVividEnable`
- `teleconverterEnable`
- `thirdPartyCamera`

This was Camera2 local marshaling evidence only.

It did not prove HAL acceptance or semantics.

---

## 21. v0.66 MasterFilm HAL/session acceptance

v0.66 used a matched control session and candidate session for:

`MasterFilmSensorType = intArrayOf(1)`

Both configured successfully.

This was the first controlled transition from local marshaling proof to real Camera2/HAL session-configuration acceptance.

No capture was submitted.

---

## 22. v0.69 complete session acceptance matrix

The crash-safe v0.69 matrix tested all remaining suitable physical-5 session candidates.

Results:

- candidates declared: 19
- candidates attempted: 18
- configured successfully: 18
- HAL/session rejects: 0
- control failures: 0
- one key skipped: `android.sensor.pixelMode`, because it was a request key but not a physical-5 session key

This established session acceptance for all 18 attempted candidates with their v0.65-resolved representations.

Session acceptance still does not prove active effect.

---

## 23. v0.70 capture-effect matrix

v0.70 moved from session acceptance to real single-frame capture-effect testing.

Eight candidates were tested with one control frame and one candidate frame:

- `MasterFilmSensorType=1`
- `aoRunningMode=1`
- `EnableVSR=1`
- `ExtendedMaxZoom=1`
- `inSensorZoomEnable=1`
- `cameraSceneMode=1`
- `extStreamSize=1`
- `teleconverterEnable=1`

Results:

- 8/8 pairs completed
- 16 physical Camera-5 RAW10 frames
- 0 structural topology differentials
- 0 selected metadata differentials
- all captures retained the known v0.59 population topology

Important confound:

Control and candidate acquisition states were not exactly matched. Exposure/ISO/focus changes meant sample-value differences could not be attributed to candidate keys.

v0.70 therefore provided strong topology-negative evidence but not clean sample-statistics attribution.

---

## 24. v0.71 semantic-value locked capture-effect matrix

v0.71 fixed the major v0.70 confound.

It tested OEM-backed values:

- `MasterFilmSensorType=intArrayOf(3)` - OEM tele role
- `cameraSceneMode=intArrayOf(53)` - UltraHighPixel/200M
- `cameraSceneMode=intArrayOf(110)` - UltraResolution/50M
- `cameraSceneMode=intArrayOf(66)` - Pro Photo RAW
- `teleconverterEnable=byteArrayOf(1)` - OEM on/true

Requested lock:

- ISO 800
- exposure 10,000,000 ns
- frame duration 33,322,225 ns
- focus 1.0 D
- AE OFF
- AF OFF

Returned pairwise state was exactly matched across every control/candidate pair:

- ISO 800
- exposure 9,999,993 ns
- frame duration 33,322,225 ns
- focus approximately 1.8621974 D

The returned focus did not equal the requested 1.0 D, but it was identical on both sides of each pair.

Results:

- 5/5 pairs complete
- 10 physical Camera-5 RAW10 frames
- 5/5 acquisition-state matched
- 0 structural topology differentials
- 0 selected result-metadata differentials
- 0 failures/skips

The same v0.59 population topology persisted.

Therefore isolated OEM-backed request values do not reproduce the stock high-pixel processor route in the tested direct Camera2 path.

This does not prove the stock route is absent.

It indicates additional orchestration is required.

---

## 25. v0.71 tested APK

Last fully built and runtime-tested APK in this research line:

`TruthRaw_v0.71_Semantic_Locked_Capture_Effect_debug_arm64.apk`

- bytes: 6,472,481
- SHA-256: `ba717793ed779804f0bfdcc9dd668dd46c2474d589ffdb1a6f386caa0a6ffdb9`

Device result document commit:

`7c4a2e7758aa800304d81490c6d668d3e64d70d1`

---

## 26. v0.72 static orchestration reconstruction

Research branch:

`research/honor452-highpixel-orchestration-v072`

Key commit:

`f74a8b75f344b01a4da2254dc526306d3308e876`

Later bridge commit:

`e6a73562912019a461eb7de13be5a98d3ce06a55`

v0.72 connected:

- scene 53
- qcomRemosaicEnable
- hintUserValue
- SMART_SCENE_MODE
- CameraService
- processor modes 23/24/32/33
- RAW MF UltraHighPixel JSON pipeline
- previously established ServiceHost RAW surface

This was the most important static route reconstruction reached before v0.73.

---

## 27. v0.73 intended runtime oracle

Research branch:

`test/physical5-hint-remosaic-oracle-v073`

Source/build commit:

`b0aea238b7a11899c96723c7b5ccc1278cf8e173`

The intended experiment was:

1. open logical Camera 0;
2. use physical Camera 5;
3. apply `cameraSceneMode=53`;
4. make one priming frame;
5. read scalar `hintUserValue` from logical/physical results;
6. check specifically for 23, 24, 32 or 33;
7. derive the HONOR remosaic state from the observed hint;
8. write `qcomRemosaicEnable` using the decoded OEM scalar-Integer representation;
9. make one physical-5 RAW10/MAX capture;
10. read hint again;
11. measure the familiar RAW10 payload topology;
12. retain strict single-frame and no-fusion boundaries.

The key scientific question was:

**Can direct Camera2 cause the HAL/result side to emit one of the stock processor-scene hints 23/24/32/33, thereby reaching the stock hint-to-ServiceHost bridge?**

---

## 28. v0.73 final build status

GitHub Actions run:

`35923309174`

Result:

`failure`

No APK artifact was uploaded.

The build failed during `:app:compileDebugKotlin`.

Observed compile errors included:

- unresolved references `width` and `height`;
- unresolved `Size`;
- constructor/argument type mismatches in `RawOutcome` paths;
- missing arguments for `errorClass` / `errorMessage`;
- type inference errors around preview-size selection.

Because the user explicitly requested no further code changes, these source errors remain intentionally unfixed in this final handoff.

Therefore:

**There is no valid v0.73 APK to download from commit b0aea238...**

Do not relabel an older APK as v0.73.

The complete v0.73 source design remains preserved on its branch for any future maintainer.

---

## 29. v0.73 source lineage

Important v0.73 commits:

- `e6a73562912019a461eb7de13be5a98d3ce06a55` - connect hintUserValue to ServiceHost high-pixel scene routing
- `a5edea10cd6669347e4e7a378b9595a840260910` - add scene53 hint-remosaic runtime oracle
- `72f52d4583eb3460601d65c22b44c0ffefa2484f` - register hint-remosaic oracle
- `9738ac0082a55154d9360d0f0cecf3dd98177376` - expose hint-remosaic oracle
- `b0aea238b7a11899c96723c7b5ccc1278cf8e173` - build workflow commit

This branch is the final unfinished research state.

---

## 30. mcpro24fps RAW-video DNG observation

A separate mcpro24fps RAW-video sample set showed another example of declared-raster versus populated-raster divergence.

Twelve distinct DNG files:

- declared 3840 x 2160 U16 CFA
- uncompressed
- WhiteLevel 1023
- each file 16,605,437 bytes

Population:

- rows 0..1839 populated
- rows 1840..2159 exact zero
- populated raster: 3840 x 1840
- zero tail: 320 rows
- zero-tail fraction: approximately 14.8148% of declared height

Classification:

`DECLARED_3840x2160_U16_CFA__POPULATED_3840x1840__EXACT_ZERO_ROWS_1840_TO_2159`

Origin of truncation remains unknown.

This independently reinforces the project rule that declared image dimensions must not be equated with populated source support.

---

## 31. Camera2 evidence hierarchy used by TruthRaw

From weakest to stronger:

1. key-name/string discovery;
2. characteristics presence;
3. typed characteristics read;
4. template default read;
5. local builder marshaling acceptance;
6. session-configuration acceptance;
7. request write + local readback;
8. real single-frame capture;
9. physical-result binding;
10. exact Image/result timestamp equality;
11. exact payload sealing and SHA-256;
12. host-side byte/sample topology analysis;
13. repeated independent captures;
14. calibrated physical interpretation only when independent calibration supports it.

Do not skip levels by inference.

---

## 32. What has been ruled out versus what remains open

### Strongly unsupported by current direct Camera2 evidence

No tested direct Camera2 single-key path has changed the Camera-5 RAW10 app-visible population geometry away from the repeated 4080x3072-equivalent packed prefix inside larger envelopes.

### Still open

The stock HONOR Camera may use:

- mode-level orchestration;
- previous-frame result feedback;
- ServiceHost;
- proprietary CameraService state;
- stock-only internal keys;
- multiple coordinated request/session values;
- raw surface switching;
- remosaic state callbacks;
- processor-scene values generated by HAL/result logic rather than directly requested.

Therefore current negative direct-Camera2 experiments do not prove the stock high-pixel/remosaic path does not exist.

They show that isolated third-party request keys have not reproduced it.

---

## 33. Highest-value unresolved technical questions

1. What exact upstream request/state makes `hintUserValue` become 23/24/32/33?
2. Which of 23/24/32/33 correspond specifically to 200M versus adjacent high-pixel states?
3. Does reaching one of those hint values require stock ServiceHost/CameraService orchestration unavailable to normal Camera2 apps?
4. What exact condition causes `NormalProcessor.setRawFormat(...)` to select `service_host_capture_raw` in the UltraHighPixel route?
5. What is the full semantic layout of `cameraIdCustomInfo` ten-field records?
6. Are any logical session parameters required together with scene 53?
7. Does the producer ever expose a genuinely different populated RAW sample topology under stock-app orchestration?
8. If so, is it remosaiced/reconstructed output or independent native CFA evidence?

---

## 34. Recommended future research discipline

If a future maintainer resumes:

- do not modify frozen production first;
- branch from the final research lineage;
- fix build-only errors separately from scientific logic;
- preserve v0.73 intended route exactly before adding new hypotheses;
- use checkpoint JSON before HAL/camera transitions;
- seal payload before interpretation;
- use fresh camera/session contexts where needed;
- keep matched controls;
- avoid multi-frame fusion;
- preserve all failed/null results;
- keep static APK facts separate from runtime facts;
- never infer native sensor truth from an output envelope.

For any future combined OEM-route experiment, require a documented static basis for every parameter/value included.

---

## 35. TruthRaw reconstruction philosophy

TruthRaw is not a “make the picture look better and call it truth” project.

Its core scientific separation is:

**Measured where measured. Reconstructed where necessary. Never invented as measurement.**

A reconstructed pixel can be useful, visually excellent and scientifically traceable, while still being labelled reconstructed rather than measured.

The project intentionally allows:

- high-dynamic-range working space;
- Float32/Float64 scientific representation;
- values below nominal display black;
- values above 1;
- uncertainty;
- censored/clipped states;
- reconstruction;
- appearance derivatives;

without falsely upgrading any of those into additional camera evidence.

---

## 36. TruthNegative and Open Scene

TruthNegative is a reconstructive/binding/projection layer derived from existing scientific authority.

It is not:

- a new sensor measurement;
- a hidden-photodiode proof;
- a replacement for the sealed source;
- an extra independent frame.

Open Scene / Free Scientific Space permits representation beyond source container limits, but source authority remains bounded by evidence.

---

## 37. Restoration / conservation rule

For damaged, weak or missing photographic information:

- preserve valid measured material;
- do not overwrite surviving source truth;
- reconstructed repair remains reconstructed;
- appearance-only repair has no Scientific-Master writeback;
- unresolved unsupported information remains unknown.

This is analogous to conservation practice: recover legibility without pretending the repair is original material.

---

## 38. Light, detail and color

### Light

Physical light interpretation requires distinction between:

- source code value;
- black offset;
- clipping;
- scene-relative radiance;
- illumination;
- geometry;
- material response.

True relighting requires evidence for geometry/normals/visibility/material/BRDF/illumination.

Without that, relighting is counterfactual appearance, not measurement.

### Detail

Acutance/sharpening may improve apparent detail.

It does not create new optical evidence.

### Color

Camera matrices and metadata can be source-bound and reproducible.

They are not independent physical color truth without calibration.

CCT is not a full spectral power distribution.

Illuminant labels are not proof of actual scene illumination.

---

## 39. Precision and export

Recommended internal precision model:

`exact packed/integer evidence -> Float32 safe operations -> Float64 branch-sensitive reconstruction -> Float64 calibration/optimization/covariance -> controlled Float32 Scientific-Master storage -> higher precision only as reference when needed`

The Lightroom-oriented Render/Edit derivative may use Float32 and preserve extended range, but it remains distinct from the Scientific Master and pure scientific linear export.

---

## 40. Final branch map

### Frozen production

`integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master`

Frozen commit:

`172a100786eb18d4b08564bbfb025a44f42cfa1e`

### Key late research branches

- `test/physical5-template-default-oracle-v064`
- `test/physical5-local-type-oracle-v065`
- `test/physical5-masterfilm-session-acceptance-v066`
- `test/physical5-aorunning-session-acceptance-v067`
- `test/physical5-session-acceptance-matrix-v068`
- `test/physical5-session-acceptance-matrix-v069`
- `test/physical5-capture-effect-matrix-v070`
- `test/physical5-semantic-locked-capture-effect-v071`
- `research/honor452-highpixel-orchestration-v072`
- `test/physical5-hint-remosaic-oracle-v073`

### Final documentation-only handoff branch

`docs/camera-knowledge-handoff-2026-09-23`

---

## 41. Final status

The camera research reached a point where the direct Camera2 path and the stock HONOR high-pixel processor path are clearly separated.

What is highly repeatable:

- logical 0 -> physical 5 routing;
- MAX output configuration;
- physical request MAX write/readback;
- returned result pixelMode=0 asymmetry;
- RAW10 larger envelopes;
- repeated 4080x3072-equivalent packed population topology;
- HAL acceptance of many vendor session keys;
- lack of topology change under tested isolated vendor controls;
- lack of topology change under OEM-backed semantic values with matched acquisition state.

What static HONOR analysis added:

- correct scene meanings;
- qcomRemosaicEnable write path;
- processor mode mapping 23/24/32/33;
- hintUserValue -> SMART_SCENE_MODE -> ServiceHost processor bridge;
- distinct internal RAW capture surface.

The unfinished v0.73 oracle was designed to test the missing runtime bridge directly.

Its source is preserved, but its build failed and no v0.73 APK exists.

This handoff therefore preserves both the achieved evidence and the exact boundary of what remained unresolved.

---

# End state

**Do not erase uncertainty to make the story cleaner.**

**Do not call an envelope a sensor.**

**Do not call reconstruction measurement.**

**Do not call a stock-app static route a runtime fact until measured.**

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
