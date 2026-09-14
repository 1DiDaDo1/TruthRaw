# FotoGraaf HONOR Route Proof v0.4 — sample-domain implementation contract

Status: implementation branch only. This document does not upgrade TruthRaw calibration authority or C0 identity.

## Evidence boundary

Keep these layers separate:

1. Standard Camera2 inventory = discovery/capability observation only.
2. HONOR vendor inventory = route candidate / vendor observation only.
3. Runtime forced-physical capture = acquisition evidence for the exact captured frame.
4. Independent camera-identity validation = required before sealing `physical 5 = TELE`.
5. Sample-domain classification = required before C0 identity can be sealed.
6. Gain/readout/focus/stabilization classes remain independent blockers.

Inventory records never increment `independentEvidenceCount` for a capture. A stock-APK or vendor hint never grants calibration authority.

## Current observed runtime route

The current v0.3 observation records:

- logical camera `0`
- requested physical camera `5`
- confirmed physical result camera `5`
- `RAW_SENSOR`
- `4080 x 3072`
- BGGR
- one physical frame
- no processed RGB / no multi-frame merge

The current HONOR metadata also exposes a 16320 x 12288 coordinate domain and a vendor `binningFactor = 4`, but this vendor integer is not to be relabeled as Android `SENSOR_INFO_BINNING_FACTOR`.

## Existing code to reuse

`capture/android/camera5-200mp-probe-v07` already contains standard Camera2 probes that must be merged into the current FotoGraaf route-proof path instead of being re-invented:

- `SENSOR_INFO_PIXEL_ARRAY_SIZE_MAXIMUM_RESOLUTION`
- `SENSOR_INFO_ACTIVE_ARRAY_SIZE_MAXIMUM_RESOLUTION`
- `SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE_MAXIMUM_RESOLUTION`
- `SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION`
- `SENSOR_INFO_BINNING_FACTOR`
- requested/result `SENSOR_PIXEL_MODE`
- result `SENSOR_RAW_BINNING_FACTOR_USED`
- physical `TotalCaptureResult` selection
- raw buffer SHA-256 and timestamp identity

The old v0.7 probe currently hard-wires the candidate as direct camera `5`; the v0.4 route-proof must instead preserve the proven logical-to-physical routing semantics where available: open logical `0`, bind the RAW `OutputConfiguration` to physical `5`, and consume the physical result for `5`.

## Required v0.4 static Camera2 block

Add `standardCamera2.staticCharacteristics` with at least:

```json
{
  "logicalCameraId": "0",
  "physicalCameraId": "5",
  "logicalPhysicalIds": [],
  "hardwareLevel": null,
  "capabilities": [],
  "pixelArrayDefault": null,
  "activeArrayDefault": null,
  "preCorrectionActiveArrayDefault": null,
  "pixelArrayMaximumResolution": null,
  "activeArrayMaximumResolution": null,
  "preCorrectionActiveArrayMaximumResolution": null,
  "sensorInfoBinningFactor": null,
  "rawSizesDefault": [],
  "rawSizesMaximumResolution": [],
  "sensorPixelModeRequestKeyPresent": false,
  "sensorRawBinningFactorUsedResultKeyPresent": false
}
```

Do not infer absent values.

## Required runtime attempt model

Use an explicit attempt array. Never silently fall back.

### Attempt A — maximum-resolution RAW

If 16320 x 12288 RAW_SENSOR is exposed for physical 5 through a compatible logical/physical session:

- create `ImageReader(16320, 12288, RAW_SENSOR, 1)`
- bind `OutputConfiguration` to physical `5`
- register `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION` on the output/session where supported
- request `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION`
- record `isSessionConfigurationSupported`
- capture exactly one physical frame
- save exact RAW payload and DNG separately
- hash both
- record image timestamp and physical CaptureResult timestamp
- record actual result pixel mode
- record `SENSOR_RAW_BINNING_FACTOR_USED`
- never call a default-resolution capture a maximum-resolution success

### Attempt B — default RAW

If maximum-resolution is unsupported or fails, record that attempt and then optionally perform a separately identified default attempt:

- `4080 x 3072 RAW_SENSOR`
- `SENSOR_PIXEL_MODE_DEFAULT` when available
- same logical 0 -> physical 5 routing
- same provenance and result fields

Attempt B is evidence for the default sample-domain only. It does not satisfy Attempt A.

## Sample-domain classification

The app may assign a deterministic observation ID only from observed standard Camera2 facts. Example IDs:

- `PHYSICAL_5_MAX_RAW_16320x12288_BGGR`
- `PHYSICAL_5_DEFAULT_RAW_4080x3072_BGGR`

Store relations separately:

```json
{
  "sampleDomain": {
    "domainId": "PHYSICAL_5_DEFAULT_RAW_4080x3072_BGGR",
    "classificationAuthority": "OBSERVED_CAMERA2_RUNTIME",
    "parentMaximumResolutionDomain": null,
    "derivedLinearScaleFromMaximumResolution": null,
    "derivedAreaScaleFromMaximumResolution": null,
    "rawBinningFactorUsed": null,
    "rawBinningInterpretation": "UNRESOLVED"
  }
}
```

A 16320/4080 = 4 and 12288/3072 = 4 relation may be stored as a mathematical derivation only after both domains are observed from standard Camera2 evidence. It must not by itself be interpreted as one specific physical binning/remosaic mechanism.

## Claim separation

Add three arrays to every observation:

- `observedClaims`: direct runtime/static observations only.
- `derivedClaims`: deterministic calculations from observed values, with inputs named.
- `hypotheses`: unsealed interpretations such as a specific sensor model or a specific upstream binning/remosaic mechanism.

No hypothesis may set `calibrationAuthorityGranted`, `c0IdentitySealed`, or `scientificMasterModified`.

## UI state wording

Before capture:

`Geïnventariseerde routekandidaat · logical 0 -> physical 5`

After a successful physical-result-matched capture:

`Runtime-bevestigde RAW-route · logical 0 -> physical 5`

Only after independent FOV/camera-identity validation:

`Sealed camera identity · physical 5 = TELE`

## DNG rule

The scientific source identity is the exact app-visible RAW/CFA payload. DNG is a convenience/evidence container. DNG opcodes are metadata/instructions and must not mutate C0 ingest. The known invalid Orientation value must be treated as a container metadata defect and fixed without changing the RAW payload.

## v0.4 acceptance gates

A maximum-resolution route is `PASS` only if all are true:

1. requested RAW size is 16320 x 12288;
2. output is bound to physical 5 through the intended logical route where available;
3. physical result for camera 5 is present;
4. actual CaptureResult pixel mode is MAXIMUM_RESOLUTION;
5. image/result timestamps match;
6. one physical frame only;
7. exact RAW payload SHA-256 is saved;
8. no fallback is mislabeled as high-resolution success.

A default 4080 x 3072 capture can independently pass its own route-proof gate while maximum-resolution remains blocked.

## Non-goals

This implementation must not claim:

- one RAW sample equals one native physical photodiode;
- HP9 or any other exact sensor model as sealed identity without direct evidence;
- absence of all upstream sensor/HAL operations merely because Camera2 exposes RAW_SENSOR;
- calibration authority from vendor tags, stock-camera behavior, or capability inventory alone.
