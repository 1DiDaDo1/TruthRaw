# v0.70 device result — physical-5 single-frame capture-effect matrix

Date: 2026-09-23

Tested APK lineage:
`9f0fed02f8a8c5a2bdb163271a57cd128d9f0091`

Source:
`TRUTHRAW_PHYSICAL5_CAPTURE_EFFECT_MATRIX_v070.json`

Authority:
`CAMERA2_SINGLE_FRAME_CONTROL_CANDIDATE_CAPTURE_EFFECT`

## Summary

- 8 candidate controls tested
- 8/8 control/candidate pairs completed
- 16 physical Camera-5 RAW10 frames captured
- 0 failed/skipped pairs
- 0 structural topology differentials
- 0 selected result-metadata differentials
- no multi-frame fusion
- Scientific Master not modified
- calibration authority not granted

Final classification:
`PHYSICAL5_SINGLE_FRAME_CAPTURE_EFFECT_MATRIX_COMPLETE_OR_PARTIAL`

## Capture route

Every frame used:
- logical camera 0
- physical camera 5
- RAW10
- 16320x12288 declared envelope
- physical output binding
- MAXIMUM_RESOLUTION output declaration
- physical SENSOR_PIXEL_MODE write/readback = 1 on the request side

Returned physical capture-result SENSOR_PIXEL_MODE remained 0.

## Candidates

The following value-1 candidates were tested:

- `MasterFilmSensorType = intArrayOf(1)`
- `aoRunningMode = intArrayOf(1)`
- `EnableVSR = intArrayOf(1)`
- `ExtendedMaxZoom = intArrayOf(1)`
- `inSensorZoomEnable = byteArrayOf(1)`
- `cameraSceneMode = intArrayOf(1)`
- `extStreamSize = intArrayOf(1)`
- `teleconverterEnable = byteArrayOf(1)`

For every candidate:
- session configured
- one candidate frame captured
- candidate was attached as a session parameter
- candidate was also written to the physical capture request
- local readback returned the requested value
- timestamp identity between physical Camera-5 result and Image passed

## Topology result

Across all 16 frames, control and candidate sides preserved the already-known v0.59 population topology:

- accessible RAW10 plane bytes: 250,675,200
- reported rowStride: 20,400
- pixelStride: 0
- first non-zero byte: 0
- last non-zero byte: 15,728,619
- all bytes at/after 15,728,640 remained zero
- host reinterpretation of the effective prefix: 3072 rows x 5120 bytes
- each effective row: 5100 packed RAW10 bytes + 20 zero bytes
- effective pad non-zero bytes: 0
- effective rows carrying image data: 3072
- highest declared 20,400-byte row containing non-zero data: 771
- `knownV059TopologyMatch=true`

Therefore none of the eight tested value-1 vendor controls changed the app-visible RAW10 population geometry.

Different full-plane/prefix/packed-only SHA-256 values are expected between independent frames and are not a vendor-effect claim.

## Result metadata

For all eight pairs:
- `structuralTopologyDifferentialObserved=false`
- `selectedResultMetadataDifferentialObserved=false`

The selected result fields remained stable:
- physical result camera ID = 5
- returned SENSOR_PIXEL_MODE = 0
- rawBinningFactorUsed = true
- noiseReductionMode = 0
- edgeMode = 0
- dynamic black level remained [64,64,64,64] in this run

## Important experimental confound

The matrix did not hold exposure/focus state exactly fixed between the control and candidate frame.

Several early pairs show a recurring sequence:
- control ISO around 1597, exposure about 20 ms, focus about 1.862 D
- candidate ISO around 1919, exposure about 16.667 ms, focus about 0.906 D

Later pairs reduce the AE difference, but focus state still changes.

The report correctly marks `exposureExactlyMatched=false` and `sampleStatisticsAttributionAllowed=false` for every pair.

Therefore differences in decoded mean/min/max/parity values cannot be attributed to the vendor candidate.

This recurring control→candidate state shift is consistent with capture-order / AE-AF settling as a confound and must be removed before using sample-value differences as evidence.

## What is established

For the tested value 1, none of the eight controls produced an app-visible structural RAW10 topology change or a differential in the selected Camera2 result metadata.

This is stronger than session acceptance alone because actual single-frame captures were performed.

## What is not established

This result does not prove that:
- these keys are semantically irrelevant;
- value 1 is the activating value for their intended HONOR/QTI meaning;
- another valid value would not alter the route;
- the vendor key is not consumed internally;
- native sensor geometry is 4080x3072;
- a hidden OEM 200MP/remosaic route does not exist;
- Direct-CFA 200MP is impossible;
- ADC bit depth or calibration truth is known.

## Next gate

Before more brute-force captures, resolve candidate semantics/value domains from static HONOR/QTI consumer code where possible.

For any candidate that has a defensible value from OEM code, repeat capture-effect under a locked state:

- CONTROL_AE_MODE_OFF
- same SENSOR_SENSITIVITY
- same SENSOR_EXPOSURE_TIME
- same SENSOR_FRAME_DURATION
- CONTROL_AF_MODE_OFF
- same LENS_FOCUS_DISTANCE
- same physical Camera-5 RAW10 MAX envelope
- randomized or mirrored A/B order where practical

This removes the strongest confound in v0.70.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
