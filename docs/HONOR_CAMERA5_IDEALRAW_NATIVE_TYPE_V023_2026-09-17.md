# HONOR Camera-5 IdealRAW native type v0.23 — 2026-09-17

Status: **DEVICE RESULT — NATIVE TYPE RESOLVED, NO SESSION/CAPTURE SUBMISSION**

This document records the v0.23 device result for the vendor session key:

`org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW`

It is route-control evidence only. It does not change the current v0.20 source/payload authority and does not establish the semantic meaning of `EnableIdealRAW`.

## Why v0.23 existed

v0.21 failed closed because the Java reflection path could not reveal the runtime value class. No vendor value was written.

v0.22 then showed that Java app-side custom-key round-trips accepted multiple representations:

- `Byte`
- `byte[]`
- `Int`
- `int[]`.

That did not uniquely identify the underlying native `camera_metadata` element type.

v0.23 therefore moved the type question to the NDK metadata layer without creating a capture session or submitting a vendor-modified capture.

## Device evidence

Evidence file:

`TRUTHRAW_CAM5_IDEALRAW_NATIVE_TYPE_ORACLE_v023.json`

Observed:

- camera ID: `0`
- key name: `org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW`
- vendor tag lookup available: yes
- vendor tag lookup status: `0`
- tag ID: `0x801F0027` (`2149515303` unsigned)
- camera open status: `0`

### BYTE test

`ACaptureRequest_setEntry_u8`:

- request create status: `0`
- set status: `0`
- get status: `0`
- accepted: `true`
- returned metadata entry type: `0` / BYTE
- entry count: `1`
- first value: `1`.

### INT32 test

`ACaptureRequest_setEntry_i32`:

- request create status: `0`
- set status: `-10001`
- get status: `-10000`
- accepted: `false`.

## Bounded conclusion

The tested device resolves this vendor key to native `camera_metadata` **BYTE** for the logical-camera-0 path.

Classification:

`NATIVE_METADATA_TYPE_BYTE__NO_SESSION_OR_CAPTURE_SUBMISSION`

This resolves the representation/type question needed for the first controlled upstream intervention. It does **not** establish that numeric value `1` semantically means "ideal RAW enabled", even though that is the vendor-key name. The effect must be measured experimentally.

## Safety / authority facts

v0.23:

- created request templates only;
- created no capture session;
- attached no session parameters;
- submitted no capture;
- submitted no vendor-modified request to HAL;
- accessed no RAW pixels;
- modified no source evidence;
- granted no semantic promotion.

The v0.20 control therefore remains unchanged and authoritative for the current Camera-5 source topology.

## Next experiment: v0.24

The next experiment may now use exactly one `BYTE` value with numeric value `1` for this key, provided all of the following remain true:

1. use a separate branch/build;
2. preserve Gate A before the intervention;
3. change no second unknown vendor key;
4. preserve logical camera 0 -> physical Camera 5 -> exact `16320x12288` MAX topology;
5. preserve source-first Plane[0] sealing;
6. preserve post-HAL HardwareBuffer/result observation;
7. rerun Stage 3.6 full-raster audit unchanged;
8. rerun Stage 3.7 payload-geometry decoder unchanged;
9. compare the result directly against v0.20.

A successful session/capture only proves that the intervention was accepted. A physical route effect requires a measurable differential in delivered source bytes, population topology, geometry or route metadata.
