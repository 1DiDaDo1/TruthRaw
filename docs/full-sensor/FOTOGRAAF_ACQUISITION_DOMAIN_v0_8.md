# TruthRaw FotoGraaf Acquisition Domain v0.8

## Scope
This is a **research/acquisition overlay**, not a change to sealed TruthRaw scientific evidence or reconstruction authority.

The v0.8 goal is to keep four concepts separate on-device:

1. **Standard Camera2 inventory** — Android-public capability discovery only.
2. **HONOR vendor inventory** — vendor-key observation only; meanings are not calibration facts without independent validation.
3. **Runtime route-proof capture** — one physical frame, with `Image.timestamp == SENSOR_TIMESTAMP`, physical-result binding, exact payload hash and DNG convenience container.
4. **Sample-domain classification** — derived only from standard Camera2 UHR/pixel-mode/binning metadata plus the returned RAW topology. It never claims untouched photodiode/ADC truth.

No inventory is counted as an independent captured frame. A single route-proof capture remains `physicalFrameCount=1` and `independentEvidenceCount=1`.

## Device evidence already observed
On the HONOR BKQ-N49, device-runtime observations have repeatedly shown a forced physical route:

`logical 0 -> physical 5 -> RAW_SENSOR 4080x3072 -> 22.48 mm`

The 4080x3072 output has been observed as BGGR Camera2 RAW_SENSOR with identical sensor/image timestamps. This is strong route evidence, but it does not by itself seal physical ID 5 as the project tele calibration authority.

## Coordinate/sample-domain problem
The HONOR observation also exposes a 16320x12288 sensor-coordinate domain while the regular RAW output is 4080x3072. The linear ratio is exactly 4 and the area ratio is 16. HONOR vendor metadata has also reported `com.hihonor.capture.metadata.binningFactor = 4`.

v0.8 does **not** reinterpret that vendor integer as Android's standard `SENSOR_INFO_BINNING_FACTOR`.

Instead, v0.8 records the Android-standard fields directly:

- `SENSOR_INFO_PIXEL_ARRAY_SIZE`
- `SENSOR_INFO_PIXEL_ARRAY_SIZE_MAXIMUM_RESOLUTION`
- `SENSOR_INFO_ACTIVE_ARRAY_SIZE`
- `SENSOR_INFO_ACTIVE_ARRAY_SIZE_MAXIMUM_RESOLUTION`
- `SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE`
- `SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE_MAXIMUM_RESOLUTION`
- `SENSOR_INFO_BINNING_FACTOR`
- `SENSOR_PIXEL_MODE` request/result
- `SENSOR_RAW_BINNING_FACTOR_USED`
- `SENSOR_INFO_LENS_SHADING_APPLIED`
- UHR / REMOSAIC / LOGICAL_MULTI_CAMERA capabilities

The 16320/4080 geometry is stored only as a **geometry relation** until the standard fields and real runtime capture resolve the sample-domain semantics.

## Two runtime RAW domains
v0.8 deliberately treats the two app-visible routes as different acquisition domains:

### A. Regular tele RAW
- Physical camera candidate: `5`
- Preferred route: logical `0` forced to physical `5` when Android advertises that relationship
- Format: `RAW_SENSOR`
- Required size: `4080x3072`
- Pixel mode: `DEFAULT`
- Authority after successful capture: `CAMERA2_ACQUISITION_OBSERVATION_ONLY`

### B. Maximum-resolution tele RAW
- Physical camera candidate: `5`
- Preferred route: the same logical->physical relationship when supported
- Format: `RAW_SENSOR`
- Required size: `16320x12288`
- The size must be present in `SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION.getHighResolutionOutputSizes(RAW_SENSOR)`
- Pixel mode: `MAXIMUM_RESOLUTION`
- A ~401 MB app-visible RAW_SENSOR plane is expected before possible row padding
- Authority after successful capture: `CAMERA2_ACQUISITION_OBSERVATION_ONLY`

The maximum-resolution capture does not prove one ADC code per original physical photodiode. If `SENSOR_INFO_LENS_SHADING_APPLIED=true`, that is explicitly retained as an upstream-processing boundary.

## Critical correction to v0.7
The v0.7 standalone 200MP probe reconstructs physical-camera records from a logical camera, but its candidate construction discards `parent_logical_id`. That can cause it to attempt `openCamera("5")` even when camera 5 is only exposed as a physical child of logical camera 0.

v0.8 must preserve the parent relationship. If camera 5 is a physical child, the evidence route is:

- open the logical parent;
- bind the `OutputConfiguration` to physical ID 5;
- when needed, construct a request with the physical ID set and use physical overrides only when Android advertises the relevant override key;
- bind the image to the matching physical `CaptureResult` and identical timestamp.

Direct opening of camera 5 is a separately named route class and is used only if Android actually exposes it as independently openable.

## Fail-closed rules
A capture does not pass its route-proof gate unless all applicable requirements are true:

- exact requested RAW size is advertised in the required standard Camera2 list;
- the session is accepted or does not explicitly report unsupported;
- requested physical output is camera 5 when using the logical route;
- a matching physical result is returned for a forced-physical route;
- image and sensor timestamps are identical;
- output format is RAW_SENSOR;
- payload bytes are persisted without a whole-frame Java/Kotlin byte-array copy;
- payload SHA-256 and byte count are stored;
- `physicalFrameCount=1` and `independentEvidenceCount=1`;
- `calibrationAuthorityGranted=false` and `c0IdentitySealed=false` remain unchanged.

## Scientific boundary
The strongest claim v0.8 may make is:

> A single app-visible Camera2 RAW sample lattice was returned by the requested runtime route, with its standard Camera2 sample-domain metadata and byte identity recorded.

It may not claim:

- untouched photodiode/ADC truth;
- absence of upstream sensor/HAL processing;
- independent tele calibration authority;
- a solved noise model from one frame;
- that an HONOR vendor tag has the same semantics as a similarly named Android-standard key;
- that a 4x geometry ratio alone proves hardware 4x4 binning.

## Next device gate
Run the regular 4080x3072 proof first. If it reproduces the existing logical-0 -> physical-5 route and records the new standard sample-domain fields, run the 16320x12288 maximum-resolution proof. The returned inventory + observation + raw payload + DNG (when DngCreator succeeds) are then evaluated off-device before any promotion of camera identity or sample-domain authority.