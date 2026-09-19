# TruthRaw v0.53 — Android 17 replay of the proven Android-16 v0.14 Camera-5 route

Date: 2026-09-19

## Why v0.52 failed

The first Android-17 payload-delta build reused the older v0.11 capture implementation.

That was historically wrong for the successful Android-16 route.

The screenshot from the Android-17 run showed:

`IllegalArgumentException: submitRequestList:658: Request settings CONTROL_SENSOR_PIXEL_MODE are not consistent with streams configured`

The repository's Android-16 acquisition authority document records the exact development sequence:

- v0.11: request rejected by sensor-pixel-mode/stream consistency;
- v0.12: `globalMAX=false`, `physicalMAX=true`, `physicalOverrideAdvertised=false`, but an app-side global-MAX gate blocked submission;
- v0.13: exact 16320x12288 Image + physical Camera-5 result + exact timestamp arrived, but the app discarded it because returned physical `SENSOR_PIXEL_MODE=0`;
- v0.14: source-first sealing fixed the evidence ordering and preserved the qualifying frame.

So the Android-17 v0.52 failure is not evidence that the proven Android-16 route disappeared. It reproduced an earlier **known-bad v0.11 request policy**.

## Exact Android-16 v0.14 reference

Qualifying Android-16 frame:

- logical camera opened: `0`
- physical camera: `5`
- output: RAW_SENSOR 16320x12288
- output physical binding: camera 5
- output maximum-resolution sensor-pixel mode declared
- logical/global SENSOR_PIXEL_MODE write: **false**
- physical Camera-5 SENSOR_PIXEL_MODE write/readback: **MAXIMUM_RESOLUTION**
- physical override advertised: **false**
- returned physical `SENSOR_PIXEL_MODE`: **0**
- Image timestamp == physical Camera-5 SENSOR_TIMESTAMP
- pixelStride: 2
- rowStride: 32640
- preserved buffer: 401,080,320 bytes
- preserved buffer SHA-256:
  `af3ad73e5919b816881a661f00ffd84a7b537f23198a5877c242717c5d7526de`

The returned mode 0 is retained as an observation, not rewritten.

## v0.53 controlled replay

v0.53 changes the capture policy back to the proven v0.14 semantics.

### Request

- open logical camera 0;
- bind RAW output to physical 5;
- call `addSensorPixelModeUsed(SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)` on the OutputConfiguration;
- use a physical-5-scoped still request;
- **never write logical/global SENSOR_PIXEL_MODE**;
- attempt physical Camera-5 `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION` even when the physical override is not advertised;
- record write/readback/error separately.

### Evidence ordering

After Image + result pairing:

1. exact 16320x12288 dimensions;
2. require physical Camera-5 result;
3. require exact Image/physical SENSOR_TIMESTAMP identity;
4. validate RAW plane layout;
5. **persist + SHA-256 seal Image.Plane[0]**;
6. only then inspect returned SENSOR_PIXEL_MODE and other advisory result metadata;
7. create optional DNG afterward.

Returned `SENSOR_PIXEL_MODE != MAXIMUM_RESOLUTION` does not destroy already acquired primary evidence.

## Post-capture delta

The v0.53 companion then runs the existing raster audit and payload geometry decoder against the sealed source.

Android-16 comparison target:

- envelope: 401,080,320 bytes;
- populated prefix: 25,067,520 bytes;
- unique standard RAW candidate: 4080x3072.

The purpose is to measure whether Android 17 changed the app-visible Camera2 payload topology.

## Authority boundary

Capture authority:

`CAMERA2_ACQUISITION_OBSERVATION_ONLY`

Post-capture analysis:

`CAMERA2_SEALED_RAW_PAYLOAD_DELTA_AUDIT`

No result may be promoted to untouched ADC, native 200MP CFA, photodiode-count truth, or calibration authority.
