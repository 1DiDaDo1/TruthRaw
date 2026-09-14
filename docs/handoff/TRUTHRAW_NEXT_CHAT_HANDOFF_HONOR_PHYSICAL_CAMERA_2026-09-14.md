# TruthRaw next-chat handoff — HONOR physical camera / FotoGraaf — 2026-09-14

Status: **MANDATORY CONTINUATION HANDOFF FOR HONOR MAGIC8 PRO PHYSICAL CAMERA WORK**

This document supplements the broader TruthRaw knowledge-preservation handoffs. It records the exact current state of the HONOR BKQ-N49 physical-camera / FotoGraaf work, including the first successful forced physical tele RAW capture and the first on-device physical-camera capability observation.

Future chats must read this file before changing physical-camera routing, maximum-resolution RAW, macro/close-focus, RAW14, Camera2 C0 identity, or FotoGraaf calibration acquisition logic.

## 1. Project laws remain unchanged

- **Measured where measured. Reconstructed where necessary. Never invented.**
- **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
- Normal photographic evidence remains one physical frame / one independent evidence root.
- Capture capability advertisement is not capture proof.
- A historical lens-role mapping is a routing hypothesis until a physical Camera2 result and sealed RAW confirm the route.
- ISO may not define `captureSampleDomainId` or `gainReadoutStateId`.
- Camera resource/capability discovery grants no CalibrationPack authority by itself.

## 2. Relevant device identity

Observed device:

- manufacturer: `HONOR`
- model: `BKQ-N49`
- Android: 16
- current SDK observed in capability probe: `36`
- logical multi-camera: camera ID `0`
- physical IDs advertised under logical 0: `2`, `4`, `5`

Historical project lens-role hypotheses remain:

- physical `2` -> main candidate
- physical `4` -> ultrawide candidate
- physical `5` -> 3.7x tele candidate

Only the tele route has now received direct forced-physical capture confirmation in the uploaded evidence described below.

## 3. Important correction to the original generic FotoGraaf Camera2 route

The first generic FotoGraaf Camera2 implementation enumerated logical and physical candidates but could still leave `requestedPhysicalCameraId=null` for a logical RAW endpoint. On the HONOR logical multi-camera this allowed the vendor stack to choose the active physical camera itself.

Observed consequence from earlier user capture:

- logical camera `0` advertised physical IDs `2`, `4`, `5`;
- no physical ID was explicitly requested;
- Camera2 selected physical ID `2`;
- therefore that capture was not proof of tele ID `5` even though the logical camera exposed it.

Decision:

**Generic logical RAW selection is insufficient for a scientific physical-lens claim on this HONOR stack.**

The route must force a physical output and then verify the returned physical result.

## 4. Forced physical tele route v0.2

A dedicated `HonorTeleActivity` was added to the FotoGraaf capture companion and to the combined TruthRaw Suite launcher.

Intended mechanism:

1. find logical camera `0` that advertises physical ID `5`;
2. create RAW output through the logical multi-camera;
3. bind `OutputConfiguration` to physical ID `5`;
4. use a capture request associated with physical ID `5` where supported;
5. require `physicalCameraTotalResults["5"]`;
6. require returned physical result camera ID `5`;
7. require RAW image timestamp == physical `TotalCaptureResult` sensor timestamp;
8. only then save/hash the DNG and label the route as confirmed physical tele candidate.

The route still grants no CalibrationPack authority and still leaves sample-domain/gain-readout classification OPEN.

## 5. First successful forced physical tele capture — PASS AS ROUTE OBSERVATION

User supplied:

- `C2TELE_20260914_184028_242_honor_tele_observation_v0_2.json`
- `C2TELE_20260914_184028_242_4080x3072.dng`

The observation JSON records:

- schema: `truthraw.fotograaf-honor-tele-acquisition-observation.v0.2`
- capture ID: `C2TELE_20260914_184028_242`
- route class: `LOGICAL_MULTI_CAMERA_FORCED_PHYSICAL_OUTPUT`
- logical camera ID: `0`
- requested physical camera ID: `5`
- confirmed physical result camera ID: `5`
- active logical physical ID: `5`
- advertised physical IDs: `2,4,5`
- physical RAW capability advertised: true
- raw-size authority: `PHYSICAL_RAW_STREAM_MAP`
- topology: `RAW_SENSOR`
- RAW dimensions: `4080x3072`
- CFA: `BGGR`
- direct CFA measurement: true
- processed RGB input: false
- multi-frame merge: false
- `SENSOR_INFO_LENS_SHADING_APPLIED=true`
- requested ISO: `100`
- result ISO: `100`
- requested exposure: `16,367,398 ns`
- result exposure: `16,367,382 ns`
- image/result timestamps exactly match
- focal length result: `22.48 mm`
- OIS mode result: `0`
- noise reduction mode result: `0`
- dynamic black: approximately `[64, 63.75, 64, 64]`
- dynamic white: `1023`
- record class: `CAMERA2_ACQUISITION_OBSERVATION_ONLY`
- `physicalRouteConfirmed=true`
- lens-role candidate: `TELE_3_7X_PHYSICAL_ID_5`
- calibration authority: false
- C0 envelope ready: false
- physical frame count: `1`
- independent evidence count: `1`

Still missing before C0 admission:

- independent validation of camera-system/lens-role mapping;
- `captureSampleDomainId`;
- `gainReadoutStateId`;
- `focusStateClass`;
- `stabilizationState`.

### Exact source identity

Observation JSON reports:

- DNG SHA-256: `865a1decf87a0f43e281d0fcab12e0ebf39f2bfcde554ca9700466d7973f37db`
- DNG byte length: `25,097,676`

The uploaded DNG was independently re-hashed in the chat environment and exactly matched both values.

### Additional local DNG audit

The uploaded DNG was parsed directly and showed:

- one TIFF/DNG image IFD;
- width `4080`;
- height `3072`;
- uint16 container;
- `BitsPerSample=16` container storage;
- `PhotometricInterpretation=CFA`;
- `Compression=1`;
- Make `HONOR`;
- Model `BKQ-N49`;
- 2x2 CFA repeat;
- CFA bytes `02 01 01 00` = BGGR;
- WhiteLevel `1023` -> effective 10-bit code ceiling despite 16-bit container storage;
- BlackLevel encoded around 64 DN;
- ISO 100;
- exposure exactly matching the physical result (`16367382 / 1e9 s`);
- focal length `22.48 mm`;
- f/2.6;
- DNG 1.4.

The file's pixel values in this particular capture were approximately 61..70 DN. Do not infer a general tele response model from that single capture.

### Metadata quirk to preserve

The DNG Orientation tag was observed as value `9`. Standard TIFF Orientation enumerations are normally 1..8, and common parsers warn on 9. Treat this as an **OPEN source-metadata quirk**. Do not silently normalize or reinterpret it until the HONOR/DngCreator source semantics are understood and regression-tested.

## 6. Physical-camera capability probe — first on-device observation

A new `HONOR Physical Camera Capability Probe` was added to the combined suite. It is an observation-only scanner. It does not capture a scene and does not grant calibration authority.

User screenshot from 2026-09-14 18:52 shows:

- `PASS CAPABILITY OBSERVATION`
- device: `HONOR BKQ-N49`
- SDK: `36`
- runtime RAW14 field present: `false`
- physical routes found: `3`

Observed route summary from the screenshot:

### logical 0 -> physical 2

- maximum-resolution RAW: `PHYSICAL_MAX_RAW_ADVERTISED_REQUIRES_CAPTURE_PROOF`
- reported maximum-resolution RAW size corresponds to about `50.33 MP`
- 200MP classification: `MAX_RAW_PRESENT_BELOW_180MP_50.33MP`
- macro: `AF_MACRO_ADVERTISED_REQUIRES_CAPTURE_VALIDATION`
- RAW14: `PLATFORM_FIELD_NOT_PRESENT`

### logical 0 -> physical 4

- maximum-resolution RAW: `NO_MAXIMUM_RESOLUTION_RAW_SENSOR_STREAM_ADVERTISED`
- 200MP classification: `NO_MAX_RAW_ADVERTISED`
- macro: `AF_MACRO_ADVERTISED_REQUIRES_CAPTURE_VALIDATION`
- RAW14: `PLATFORM_FIELD_NOT_PRESENT`

### logical 0 -> physical 5

- maximum-resolution RAW: `PHYSICAL_MAX_RAW_ADVERTISED_REQUIRES_CAPTURE_PROOF`
- reported maximum-resolution RAW size corresponds to about `50.14 MP`
- 200MP classification: `MAX_RAW_PRESENT_BELOW_180MP_50.14MP`
- macro: `AF_MACRO_ADVERTISED_REQUIRES_CAPTURE_VALIDATION`
- RAW14: `PLATFORM_FIELD_NOT_PRESENT`

The probe itself correctly states that no capability line is capture proof and that physical `TotalCaptureResult + sealed RAW` remain mandatory.

### Immediate interpretation

Current public Camera2 observation on Android 16 / SDK36 does **not** advertise a >=180MP RAW_SENSOR maximum-resolution stream for any of the three physical routes.

Therefore:

- public Camera2 native 200MP tele RAW: **NOT ADVERTISED on this observed firmware/API state**;
- physical ID5 maximum-resolution RAW around 50.14MP: **ADVERTISED, requires forced-physical max-resolution capture proof**;
- physical ID2 maximum-resolution RAW around 50.33MP: **ADVERTISED, requires forced-physical max-resolution capture proof**;
- physical ID4 maximum-resolution RAW: **not advertised in the observed public stream map**.

This does **not** prove that HONOR has no private/vendor 200MP path. It only proves that the public Camera2 capability observation shown here does not expose a >=180MP RAW_SENSOR maximum-resolution stream.

The machine-readable capability JSON named in the screenshot was not uploaded in this chat, so only the on-screen values above are preserved as screenshot-derived observation. Future work should ingest and hash the actual capability JSON as a sealed observation artifact.

## 7. RAW14 / Android 17 direction

The user explicitly wants RAW14 considered when Android 17 becomes available.

Current device probe state:

- Android 16 / SDK36
- RAW14 platform field absent at runtime

Therefore current status is:

`RAW14 = PLATFORM_FIELD_NOT_PRESENT`

Future rule:

- detect RAW14 by runtime/API capability, not by OS-name assumptions alone;
- test RAW14 separately for each physical camera and separately for default versus maximum-resolution sensor mode;
- 200MP and RAW14 are independent capabilities and must never be combined by assumption;
- preserve original packed RAW14 bytes as sealed evidence;
- do not force RAW14 through a writer/API that only proves RAW_SENSOR support;
- any unpacked uint16 representation is a derived measurement representation, not the original byte evidence.

## 8. Multi-camera design target

Do not build three unrelated camera implementations. Continue toward one generic physical-camera engine/profile system.

Desired profile dimensions per physical camera:

- logical parent camera ID;
- physical camera ID;
- verified project lens role;
- standard RAW_SENSOR sizes;
- maximum-resolution RAW_SENSOR sizes;
- maximum-resolution sensor-mode support;
- RAW14 sizes when/if available;
- CFA pattern;
- active/pixel array information;
- focal lengths;
- minimum focus distance;
- focus calibration;
- AF modes including macro advertisement;
- OIS capabilities;
- manual sensor capability;
- physical request keys;
- `SENSOR_INFO_LENS_SHADING_APPLIED`;
- session-configuration support;
- capture-proof status per advertised mode.

Suggested user-facing modes should only appear after evidence supports them:

- Main standard RAW
- Main maximum-resolution RAW
- Wide standard RAW
- Wide maximum-resolution RAW if ever advertised
- Wide macro / close focus after physical validation
- Tele standard RAW
- Tele maximum-resolution RAW
- Tele close focus / macro only after physical validation
- RAW14 variants later, per actual route

## 9. Macro / close-focus authority

The first capability scan shows AF macro advertisement on physical IDs 2, 4, and 5.

This is only an advertised control capability. It is not proof that each physical lens delivers a useful photographic macro mode.

Validation must record at least:

- forced physical camera ID;
- AF mode request/result;
- actual focus distance result;
- minimum focus distance characteristic;
- focus-calibration characteristic;
- timestamp-matched RAW;
- scene target distance / controlled ruler/target where possible;
- whether the physical camera stayed the requested lens throughout the capture;
- no vendor auto-switch to another lens.

Until that is done, label the state `AF_MACRO_ADVERTISED_REQUIRES_CAPTURE_VALIDATION`, not `MACRO_PASS`.

## 10. Maximum-resolution RAW validation next

The highest-priority physical camera capture tests are now:

1. force physical ID5 in standard RAW again as reproducibility check;
2. force physical ID5 in the advertised maximum-resolution RAW mode (~50.14MP) and require physical result + exact timestamp + sealed RAW hash;
3. force physical ID2 in its advertised maximum-resolution RAW mode (~50.33MP) with the same proof;
4. confirm that physical ID4 truly has no public maximum-resolution RAW_SENSOR stream on this device/firmware;
5. run controlled close-focus/macro capture validation per physical route;
6. when Android 17 / API37 is actually present, re-run capability discovery for RAW14 and only expose RAW14 modes that the physical route advertises and can capture.

Do not present a `200MP RAW` button unless the public/vendor route being used produces an actual sealed RAW payload at that resolution and physical identity is proven.

## 11. Relation to FotoGraaf CalibrationPack

Physical camera routing/capability work is acquisition identity, not calibration.

Even a successful ID5 maximum-resolution RAW does not automatically grant:

- `captureSampleDomainId`;
- `gainReadoutStateId`;
- black/noise calibration;
- gain/linearity calibration;
- physical color authority;
- irradiance/light authority;
- calibrated HDR range.

Those remain under the existing CalibrationPack -> Scene Admission -> Shadow Measurement -> Physical Promotion Gate architecture.

Each newly proven physical mode must receive its own exact calibration scope. Standard RAW and maximum-resolution RAW must not be assumed to share the same sample/noise/readout domain.

## 12. Relevant current app architecture

The combined TruthRaw Suite contains:

- normal TruthRaw processing UI;
- generic FotoGraaf Camera2 acquisition observation;
- dedicated HONOR tele physical-ID5 route;
- HONOR physical-camera capability probe.

The suite is a packaging layer. Normal TruthRaw scientific authority and FotoGraaf acquisition/calibration authority remain contract-separated even though camera permission is app-wide.

## 13. CI / branch handling

Active work remains on PR #23:

`research/ui-output-modes-certificate-v0.1-2026-09-14`

PR must remain draft/open and must not be merged unless the user explicitly directs it.

Always re-fetch the current head SHA and exact-head workflow status before claiming CI green. Do not reuse an older green head after new documentation/camera commits.

## 14. Next-chat continuation command

If the next chat receives `Begin`, `Ga verder`, or `Ga door`, the next scientifically justified path is:

1. read this handoff plus the broader preservation handoffs;
2. re-fetch PR #23 live head/CI;
3. ingest the actual machine-readable physical-camera capability JSON if the user can upload it;
4. build/validate forced-physical maximum-resolution capture for ID5 and ID2;
5. preserve ID4 no-max-RAW observation unless new runtime evidence contradicts it;
6. add capture-proof statuses to the capability profile;
7. build controlled macro/close-focus validation without lens auto-switch;
8. prepare RAW14 capability/capture path behind runtime API/capability checks for Android 17/API37;
9. keep all new modes fail-closed until physical result + timestamp + sealed RAW prove them.

## 15. Status summary

- Generic logical RAW route for tele claim: **FAIL / insufficient physical selection proof**
- Forced physical ID5 tele route implementation: **IMPLEMENTED**
- First forced physical ID5 RAW capture: **PASS AS ROUTE OBSERVATION**
- Physical ID5 calibration authority: **BLOCKED / not yet calibrated**
- Public Camera2 >=180MP RAW on current Android16/SDK36 scan: **NOT ADVERTISED**
- ID5 max-resolution RAW ~50.14MP: **ADVERTISED / CAPTURE PROOF OPEN**
- ID2 max-resolution RAW ~50.33MP: **ADVERTISED / CAPTURE PROOF OPEN**
- ID4 max-resolution RAW: **NOT ADVERTISED IN CURRENT SCAN**
- AF macro on IDs 2/4/5: **ADVERTISED / CAPTURE VALIDATION OPEN**
- RAW14 current platform field: **ABSENT ON SDK36**
- RAW14 Android17/API37 future path: **PLANNED / RUNTIME CAPABILITY REQUIRED**
- 200MP vendor/private path: **OPEN / not proven by public Camera2 capability scan**
