# TruthRaw v0.52 — Android 17 Camera-5 sealed payload delta

Date: 2026-09-19

## Trigger

v0.51 established that public CameraDeviceSetup:

- accepts physical-5-bound RAW_SENSOR and RAW10 configurations at 4080x3072, 8160x6144 and 16320x12288;
- rejects all predeclared RAW14 configurations on physical cameras 5, 2 and 4.

The public RAW14 path is therefore no longer the highest-value experiment.

## Question

The Android-16 Camera-5 capture route produced an app-visible RAW_SENSOR envelope declared as:

- 16320x12288
- 200,540,160 declared samples
- 401,080,320 bytes at 2 bytes/sample

but the raster audit showed only a contiguous:

- 25,067,520-byte populated prefix

which exactly matched the advertised standard:

- 4080x3072 RAW_SENSOR
- 25,067,520 bytes at U16 storage

The Android-17 OS/HAL update gives a clean opportunity to repeat the **same proven acquisition topology** and ask whether this payload topology changed.

## v0.52 design

v0.52 deliberately does **not rewrite the established capture activity**.

Button 1 launches the existing `FotoGraaf200MpStagedActivity`, preserving its already-tested capture gate:

- open logical camera 0;
- physical Camera-5 output binding;
- 16320x12288 RAW_SENSOR;
- maximum-resolution output mode;
- physical Camera-5 TotalCaptureResult required;
- Image timestamp == physical Camera-5 SENSOR_TIMESTAMP;
- physical result SENSOR_PIXEL_MODE must report MAXIMUM_RESOLUTION;
- original Image.Plane buffer sealed before DNG containerization.

After that capture returns to the suite, button 2 performs a read-only analysis of the newest sealed v0.11 capture.

## Read-only post-capture audit

v0.52:

1. finds the latest v0.11 evidence JSON;
2. locates the exact raw-buffer file named by that evidence;
3. verifies the raw file byte count;
4. recomputes SHA-256 and requires identity with the sealed evidence JSON;
5. runs existing `RawSensorRasterAudit` read-only;
6. runs existing `RawPayloadGeometryDecoder` read-only against physical Camera 5's currently advertised standard RAW_SENSOR sizes;
7. compares envelope size, populated-prefix byte count and unique standard-RAW candidate against the Android-16 reference.

The sealed source is never opened for writing.

## Decision classes

### Exact structural replication

If all are true:

- source envelope = 401,080,320 bytes;
- populated prefix = 25,067,520 bytes;
- unique candidate = 4080x3072;

classify:

`ANDROID17_REPLICATES_ANDROID16_CAM5_ENVELOPE_AND_4080x3072_POPULATED_PREFIX_TOPOLOGY`

This would show that the Android-17 update and new Honor Camera package did not alter the public Camera2 app-visible payload topology of the established route.

### Envelope stable, payload changed

If the full envelope remains 401,080,320 bytes but populated topology differs, preserve the new result as a new Android-17 observation and do not force it into the old model.

### Envelope changed

Treat as a new HAL transport behavior requiring fresh characterization.

## Authority boundary

The capture retains `CAMERA2_ACQUISITION_OBSERVATION_ONLY`.

The post-capture audit is `CAMERA2_SEALED_RAW_PAYLOAD_DELTA_AUDIT`.

Even exact Android-16/17 replication does not prove:

- native physical sensor geometry;
- untouched ADC output;
- one photodiode per app-visible sample;
- native 200MP Direct-CFA;
- optical 200MP resolution.
