# TruthRaw Full Sensor Research v0.7 — exact Camera 5 200MP capture route

## Decision
The user-supplied Camera2 characteristics establish a real Camera-5 `RAW_SENSOR 16320x12288` high-resolution route. v0.7 therefore stops treating 200MP as a reconstruction-first problem and targets direct acquisition of the app-visible full sensor lattice.

## Critical correction from v0.4
v0.4 enumerated `SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION.getOutputSizes(format)` but did not separately enumerate `getHighResolutionOutputSizes(format)`. On the BKQ-N49 Camera 5 this matters:

- maximum-resolution normal RAW_SENSOR: 8160x6144 (~50.14 MP)
- maximum-resolution high-resolution RAW_SENSOR: 16320x12288 (200.54016 MP)

v0.7 explicitly queries both and accepts only the 16320x12288 RAW_SENSOR route for the 200MP proof.

## Memory hardening
A contiguous 16320x12288 RAW_SENSOR plane at 16-bit storage is 401,080,320 bytes. v0.7:

- uses ImageReader `maxImages=1`;
- never copies the whole plane into a Java/Kotlin `ByteArray`;
- hashes a duplicate direct ByteBuffer;
- writes a duplicate direct ByteBuffer through FileChannel;
- preserves the exact accessible app-visible plane buffer;
- names it `.rawsensor` only when `pixelStride=2`, `rowStride=32640`, and accessible bytes are exactly 401,080,320; otherwise it remains `.rawbuffer` for off-device row-padding normalization.

## Runtime evidence gate
`camera5_200mp_runtime_gate_v07.py` closes only when:

- evidence class is real DEVICE_RUNTIME_CAPTURE;
- device is HONOR BKQ-N49;
- opened/physical camera is 5;
- focal length is 22.48 mm within tolerance;
- format is RAW_SENSOR;
- output is exactly 16320x12288;
- that size was reported in maximum-resolution high-resolution RAW sizes;
- maximum pixel-array geometry is 16320x12288;
- MAXIMUM_RESOLUTION was both requested and applied;
- Image timestamp equals SENSOR_TIMESTAMP;
- row/pixel stride is plausible;
- the returned payload file is actually supplied and its byte count + SHA-256 match the manifest.

A manifest without the raw payload cannot close the gate. Simulations cannot close it.

## Provenance boundary
Camera 5 reports `SENSOR_INFO_LENS_SHADING_APPLIED=true`. Therefore a successful capture is classified as full-sensor maximum-resolution **app-visible RAW**, not untouched photodiode/ADC truth.

## Host validation
Full Full-Sensor regression through v0.7: **19/19 PASS** (v0.2-v0.5 + v0.7 test sets).

Android compilation/device execution is still pending because this sandbox has no Android SDK/Gradle runtime. No APK-built claim is made.
