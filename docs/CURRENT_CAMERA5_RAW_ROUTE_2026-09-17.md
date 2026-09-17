# TruthRaw current Camera-5 RAW route — 2026-09-17

Status: **CURRENT CAMERA-5 ACQUISITION / PAYLOAD-INTERPRETATION AUTHORITY**  
Device: HONOR Magic 8 Pro / BKQ-N49  
Logical camera: `0`  
Physical tele camera: `5`

This document is the current bounded interpretation of the Camera-5 RAW work through v0.20. It supplements, rather than rewrites, the proven v0.14 acquisition document and the v0.17 two-door-airlock document.

Permanent boundary:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`

## 1. Current bounded result

The proven route is:

`logical camera 0`
`-> physical output Camera 5`
`-> MAXIMUM_RESOLUTION output declaration`
`-> physical-scoped still request`
`-> Android Image/HardwareBuffer declared 16320x12288 RAW_SENSOR`
`-> original Image.Plane[0] sealed first`
`-> post-HAL envelope recorded read-only`
`-> camera/image closed`
`-> sealed file audited read-only`
`-> populated prefix matched against device-advertised standard RAW geometries`

The current evidence supports the following combined statement:

> Camera 5 can deliver an app-visible Android RAW_SENSOR/HardwareBuffer envelope declared as `16320x12288`, but the tested v0.19/v0.20 captures populate only the first `25,067,520` bytes. In v0.20 that populated prefix is the unique exact byte-count match to Camera 5's advertised standard `4080x3072` RAW_SENSOR geometry, is an exact byte-for-byte source-prefix copy, produces a coherent full-frame scene when indexed as `4080x3072`, and exhibits Bayer-like 2x2 phase structure.

This is stronger than a byte-count coincidence, but remains weaker than proof of native ADC geometry or untouched photodiode output.

## 2. v0.20 qualifying capture

Capture evidence file:

`TRUTHRAW_1789654132526_CAM5_200MP_EVIDENCE_v020.json`

Primary sealed source named by that evidence:

`TRUTHRAW_1789654132526_CAM5_200MP_16320x12288_v020.rawsensor`

Observed capture facts:

- physical result Camera ID: `5`
- delivered Image geometry: `16320 x 12288`
- declared sample positions: `200,540,160`
- `Image.timestamp == physical SENSOR_TIMESTAMP`: pass
- ISO: `125`
- exposure: `9,999,993 ns`
- focal length: `22.48 mm`
- focus distance: `0.07848677 D`
- returned `SENSOR_PIXEL_MODE = 0`
- request/output MAX path: true
- returned `rawBinningFactorUsed = true`
- source bytes: `401,080,320`
- row stride: `32,640`
- pixel stride: `2`
- source SHA-256: `39da26192d1278af797b0c304d6ad29862735b886bdcf768b6239bc74f6a3636`

The source remains the 401,080,320-byte sealed evidence object even though most of that envelope is zero in these captures.

## 3. Stage 3.6 full-raster write finding

Stage 3.6 re-reads the already sealed file after Camera2 objects are closed and never opens the source for writing.

The v0.19/v0.20 finding is:

- first non-zero byte offset: `0`
- last non-zero byte offset: `25,067,518`
- populated prefix bytes: `25,067,520`
- populated U16 samples: `12,533,760`
- non-zero rows in the declared 16320-wide interpretation: exactly rows `0..767`
- rows `768..12287`: all zero
- bands `1..15`: all zero in the 16-band audit

Exact identity:

`16320 * 768 * 2 = 25,067,520 bytes`

and

`4080 * 3072 * 2 = 25,067,520 bytes`.

This is why the earlier gallery/DNG view showed one narrow populated strip followed by black. The effect already exists in the original sealed Plane[0]; it is not created by Google Photos or DngCreator.

## 4. Stage 3.7 payload-geometry finding

v0.20 compares the populated-prefix byte count against the physical Camera-5 standard RAW_SENSOR output sizes reported at runtime.

Exactly one advertised standard RAW size matches:

`4080 x 3072 x 2 = 25,067,520 bytes`

Selected Stage-3.7 candidate:

- width: `4080`
- height: `3072`
- row bytes: `8160`
- sample count: `12,533,760`
- min code: `64`
- max code: `1023`
- exact-prefix SHA-256: `d41d48d644c4aa8804825a529a0ea651352979a7023e4948e39913b71ce16e3e`
- copied source range: `[0, 25,067,520)`
- copy transform: none
- prefix SHA equals Stage-3.6 first-band SHA: true

The derived file is:

`TRUTHRAW_1789654132526_CAM5_PAYLOAD_STANDARD_RAW_BYTE_MATCH_v020.rawpayload`

It is an auxiliary exact-prefix view. It does not replace the sealed 401 MB source.

## 5. Bayer-like spatial evidence

When the exact same bytes are indexed as `4080x3072`, the 2x2 phase means are approximately:

- phase `(0,0)`: `131.978941`
- phase `(0,1)`: `174.729906`
- phase `(1,0)`: `174.945847`
- phase `(1,1)`: `127.204966`

The two cross phases are almost equal while the two diagonal phases differ, which is strongly consistent with a normal Bayer-like 2x2 CFA organization. It does not by itself identify which diagonal phase is R or B.

Sampled spatial correlations also support the `4080x3072` interpretation:

- horizontal dx=1: correlation `0.845771`, mean absolute difference `45.7344`
- horizontal dx=2: correlation `0.984627`, mean absolute difference `7.6461`
- vertical dy=1: correlation `0.879186`, mean absolute difference `43.6312`
- vertical dy=2: correlation `0.981845`, mean absolute difference `7.3667`

Distance 2 compares the same 2x2 CFA phase and is dramatically more similar than immediate cross-colour neighbors.

The diagnostic PNG:

`TRUTHRAW_1789654132526_CAM5_PAYLOAD_GEOMETRY_DIAGNOSTIC_v020.png`

renders a coherent full-frame scene from the same bytes at `4080x3072`. It is appearance-only and not scientific evidence.

## 6. Effective code-domain clue

The v0.20 payload values span exactly `64..1023` in this capture.

This is compatible with a 10-bit code domain stored in 16-bit words and a black offset near 64, but it is **not yet promoted** to a formal bit-depth/black-level conclusion. That requires direct binding to CameraCharacteristics/CaptureResult black/white metadata and multiple controlled captures.

## 7. Two-door airlock / HAL envelope

v0.16/v0.17 established the request-side and delivery-side provenance architecture.

Pre-HAL request/session gates record what TruthRaw asks for before vendor execution. They are control provenance, not pixel interception.

Post-HAL observation shows the same live output as:

- Java Image: `16320x12288`, RAW_SENSOR format 32
- one plane
- pixel stride `2`
- row stride `32640`
- buffer capacity `401080320`
- HardwareBuffer: `16320x12288`, one layer, format 32
- native HardwareBuffer stride: `16320` pixels
- Java/native buffer ID identity: pass
- probe lock/map/write: false

The primary bytes are sealed before this metadata is interpreted.

## 8. HONOR/QTI metadata clues

These remain observations with unknown or only partially bounded semantics:

- HONOR `binningFactor = 4`
- HONOR `AECRealCropWindow` begins approximately `[11, 8, 4058, 3055]`, close to the 4080x3072 domain
- HONOR `allISPCropWindow` reports the full `16320x12288` domain
- returned Android `SENSOR_PIXEL_MODE = 0`
- returned Android `rawBinningFactorUsed = true`
- HONOR `sensorCustomMetaData` is a 40-byte sidecar payload
- QTI exposes `ActiveCameraInfo` and other multicamera metadata
- QTI/HONOR AF metadata exposes a physical/vendor lens position (`org.quic.camera.afData.lenspos`; value `2856` in the v0.20 capture)

These facts suggest that multiple internal coordinate/readout domains coexist, but they do not yet prove how HONOR physically bins, remosaics or routes photodiode data.

## 9. Request-side vendor controls discovered

The physical/logical session/request surface exposes names including:

- `org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW`
- `org.codeaurora.qcamera3.sessionParameters.RawCbSourceType`
- `org.codeaurora.qcamera3.sessionParameters.EnableXCFAOptimization`
- `org.codeaurora.qcamera3.sessionParameters.HALOutputBufferCombined`
- `org.codeaurora.qcamera3.sessionParameters.EnableInsensorZoom`
- `org.codeaurora.qcamera3.sessionParameters.EnableSnapshotOnlyInsensorZoom`
- `org.codeaurora.qcamera3.sessionParameters.EnableMCXMasterCb`

v0.17/v0.20 deliberately set none of these unknown vendor controls. `EnableXCFAOptimization` was visible on the builder with value `0`; `EnableIdealRAW` and `RawCbSourceType` were visible as available keys but their builder current/default values were null in the recorded route fingerprint.

External Qualcomm metadata tables from other devices suggest `EnableIdealRAW` is often byte-sized and `RawCbSourceType` often int32, but HONOR semantics must be verified on-device before any write. Cross-device metadata type evidence is a probe-design clue, not device authority.

## 10. Multi-camera / focus knowledge retained

The route exposes enough AF/multicamera state to justify a future controlled probe:

- Android `LENS_FOCUS_DISTANCE` is available on Camera 5;
- physical capture results report focus distance;
- QTI reports AF lens position and phase-detect state;
- QTI multicamera sidecars are present during the tele capture.

A dedicated v0.18 physical-focus branch exists, but it is intentionally **not** part of the trusted v0.19/v0.20 acquisition chain. The full-raster/payload-geometry question was prioritized first.

Future multi-camera/multi-focus captures must keep each physical RAW as a separately sealed observation. One shutter event does not automatically mean one independent evidence source, and multiple physical frames do not automatically imply statistical independence.

## 11. Current scientific claim

Recommended current classification:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

A shorter descriptive form is:

`APP_VISIBLE_4080x3072_BAYER_LIKE_RAW_PAYLOAD_EMBEDDED_IN_16320x12288_HAL_ENVELOPE`

The second string is a descriptive research summary, not a claim of native physical sensor geometry.

## 12. Claims that remain forbidden

Do not promote the current result to:

- untouched/native photodiode ADC output;
- proof that the physical sensor itself is only 4080x3072;
- proof that all 12,533,760 payload samples are independent photodiode ADC measurements;
- proof of exact sensor binning/remosaic mechanism;
- proof of 200 MP optical resolution;
- proof of calibrated black level merely because min code is 64;
- proof of 10-bit ADC merely because max code is 1023;
- proof that a vendor key name states its semantics.

## 13. Next safe route experiment

The next route-changing experiment must preserve v0.20 as the control and change **one upstream vendor variable at a time**.

Highest-value first candidate:

`EnableIdealRAW`

Protocol:

1. reconstruct the exact v0.20 acquisition/audit chain;
2. discover the runtime key object and verify its value type on this device;
3. create a separate experimental branch/build;
4. set only `EnableIdealRAW` if the type/value operation can be represented safely;
5. record exactly where it is applied: session versus still request, logical versus physical scope;
6. preserve all Gate A/B state;
7. run the same 16320x12288 capture;
8. seal source first;
9. run the same Stage 3.6 and Stage 3.7 audits;
10. compare populated byte count, prefix geometry, result pixel mode, binning metadata, HardwareBuffer envelope and all vendor fingerprints against the untouched v0.20 control.

Do not combine `EnableIdealRAW`, `RawCbSourceType`, XCFA, in-sensor zoom or other unknown controls in one first experiment. A single-variable design is required for interpretable evidence.

## 14. Position in TruthRaw architecture

This entire layer is acquisition/provenance and sample-topology research:

`request/session intent`
`-> HONOR/QTI vendor pipeline`
`-> app-visible HardwareBuffer/Image`
`-> immutable source seal`
`-> integer-exact raster/payload audit`
`-> only then calibration/de-ISP`
`-> Scientific Master`
`-> Dynamic Authority / uncertainty`
`-> TruthRange / zero-line`
`-> appearance/export`

F64, FP32, Scientific Master and zero-line do not reinterpret or improve the original Camera2 evidence. They operate only after the source geometry/topology and authority are correctly established.
