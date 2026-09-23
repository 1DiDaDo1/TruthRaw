# v0.71 device result — semantic-value locked physical-5 capture-effect matrix

Date: 2026-09-23

Source:
`TRUTHRAW_PHYSICAL5_SEMANTIC_LOCKED_CAPTURE_EFFECT_v071.json`

Authority:
`CAMERA2_SINGLE_FRAME_SEMANTIC_VALUE_LOCKED_CONTROL_CANDIDATE_EFFECT`

## Summary

- 5 semantic-value candidates
- 5/5 control/candidate pairs complete
- 10 physical Camera-5 RAW10 frames
- 5/5 pairs matched in returned ISO, exposure, frame duration and focus state
- 0 structural topology differentials
- 0 selected result-metadata differentials
- 0 failed/skipped pairs
- no multi-frame fusion
- Scientific Master untouched
- no calibration authority

Final classification:

`PHYSICAL5_SEMANTIC_LOCKED_CAPTURE_EFFECT_MATRIX_COMPLETE_OR_PARTIAL`

## Tested OEM-backed values

- `MasterFilmSensorType = intArrayOf(3)` — HONOR .452 OEM tele role
- `cameraSceneMode = intArrayOf(53)` — HONOR .452 UltraHighPixel/200M scene
- `cameraSceneMode = intArrayOf(110)` — HONOR .452 UltraResolution/50M scene
- `cameraSceneMode = intArrayOf(66)` — HONOR .452 Pro Photo RAW scene
- `teleconverterEnable = byteArrayOf(1)` — HONOR .452 on/true representation

Every candidate was attached as a session parameter and also written to the physical Camera-5 capture request with successful local readback.

## Acquisition lock

Requested:
- ISO 800
- exposure 10,000,000 ns
- frame duration 33,322,225 ns
- focus distance 1.0 D
- AE OFF
- AF OFF

Returned per-frame acquisition state was pairwise identical for every control/candidate comparison:
- ISO 800
- exposure 9,999,993 ns
- frame duration 33,322,225 ns
- focus distance 1.8621973991394043 D

Important nuance: the returned focus distance did not equal the requested 1.0 D, but it was identical on both sides of every pair. Therefore the v0.70 control/candidate focus confound was removed for differential comparison, without claiming that the requested focus value was honored.

## Capture/result route

For all frames:
- logical camera 0
- physical camera 5
- RAW10
- declared 16320x12288 envelope
- physical output binding
- physical SENSOR_PIXEL_MODE request write/readback = 1
- returned physical-result SENSOR_PIXEL_MODE = 0
- `rawBinningFactorUsed=true`
- noise reduction = 0
- edge = 0
- physical result Camera ID = 5
- image timestamp exactly matched physical Camera-5 SENSOR_TIMESTAMP

## Topology

All 10 frames preserved the established v0.59 app-visible topology:
- accessible bytes = 250,675,200
- rowStride = 20,400
- pixelStride = 0
- first nonzero byte = 0
- last nonzero byte = 15,728,619
- tail after 15,728,640 = exact zero
- effective pad nonzero count = 0
- effective rows with image data = 3072
- max declared 20,400-byte row with nonzero data = 771
- `knownV059TopologyMatch=true`

Thus none of the five OEM-backed values changed the app-visible RAW10 population geometry in this direct Camera2 route.

## Sample statistics

Because returned acquisition state was exactly pair-matched, the sample means are now meaningfully comparable as observations, while sensor noise, scene variation and unmeasured processing remain possible confounds.

Candidate minus control decoded-mean deltas:
- MasterFilmSensorType tele=3: -0.0028724820
- cameraSceneMode 53: -0.0093659046
- cameraSceneMode 110: -0.0004822974
- cameraSceneMode 66: -0.0084048203
- teleconverterEnable on: -0.0015493356

No pair showed a structural topology change or selected result-metadata change.

These tiny per-frame mean differences are not promoted to a vendor-key effect without repeatability/noise-baseline evidence.

## What this establishes

Within the exact direct Camera2 physical-5 RAW10/MAX route, isolated application of the five tested OEM-backed values does not alter:
- app-visible RAW10 population geometry;
- selected Camera2 result metadata;
- the known v0.59 topology classification.

This is stronger negative evidence than v0.70 because the major AE/focus mismatch between control and candidate was removed.

## What this does not establish

It does not prove:
- the vendor keys are semantically irrelevant;
- the stock HONOR high-pixel route can be reproduced by one isolated key;
- the stock app's mode orchestration consists only of these keys;
- an OEM processor/service-host route is not required;
- no hidden remosaic/high-pixel route exists;
- native sensor geometry, Direct-CFA 200MP, ADC bit depth or calibration truth.

## Next gate

Do not continue brute-force single-key value sweeps.

The next gate is to reconstruct the smallest OEM high-pixel orchestration vector from the exact HONOR .452 code:
- mode/scene setup for UltraHighPixel
- qcomRemosaicEnable write path and value/domain
- processor-internal high-pixel mode values 23/24/32/33
- any logical-camera orchestration/session keys that co-occur
- ServiceHost/raw-surface selection conditions

Then test the smallest defensible OEM-derived combination as one isolated single-frame experiment, still preserving a matched control and exact provenance boundaries.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
