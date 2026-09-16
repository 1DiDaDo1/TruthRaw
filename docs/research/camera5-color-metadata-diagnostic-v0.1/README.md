# Camera-5 color metadata diagnostic v0.1 — 2026-09-16

## Status

`REAL_DEVICE_COLOR_APPEARANCE_FAILURE_REPRODUCED__SOURCE_CFA_ROUTE_STILL_VALID`

This note records the first real-device color failure reported from the integrated TruthRaw Suite v0.5 field test. It does **not** invalidate the Camera-5 RAW_SENSOR evidence route and it does **not** promote a replacement white balance to calibration authority.

## Bound source

Field capture:

- device: HONOR BKQ-N49 / Android 16;
- logical camera 0 -> requested physical camera 5;
- physical result for camera 5 present;
- RAW_SENSOR 4080 x 3072;
- CFA BGGR;
- single physical frame;
- image timestamp exactly equals SENSOR_TIMESTAMP;
- DNG SHA-256: `fc21e7096c8322e5cf723d807782bd32a1b43b168ab844a064587f6893283cdc`;
- DNG bytes: `25097544`.

The capture observation itself states `calibrationAuthorityGranted=false`. Therefore source-DNG color metadata may support a source-bound appearance transform, but is not independent physical color calibration.

## Direct DNG metadata inspection

The captured DNG contains:

- `CFAPattern = [B,G,G,R]` / BGGR;
- four-phase black level approximately `[64,64,64,63.75]`;
- `WhiteLevel = 1023`;
- `AsShotNeutral = [891/1024, 1, 354/1024] = [0.8701171875, 1.0, 0.345703125]`;
- corresponding normalized inverse-neutral gains approximately `[1.14927, 1.0, 2.89266]`;
- dual `ColorMatrix1/2` and `ForwardMatrix1/2` metadata;
- calibration illuminants D65 and Standard Light A;
- four OpcodeList2 GainMaps. Their mean gains differ only by a few percent between CFA phases, so lens-shading GainMap asymmetry is not large enough to explain the strong blue appearance by itself;
- TIFF Orientation value `9`, which is invalid for the supported TIFF orientation enum. This is a separate v0.3 DngCreator path defect and must be forced to Orientation=1 on all capture paths.

## Independent diagnostic observation

A direct numerical decode of the sealed CFA was performed without changing the source. With the DNG AsShotNeutral/ForwardMatrix color path, nominally neutral-looking wall/shelf regions render with a strong blue cast, matching the user's field report.

For several bright low-chroma wall/shelf regions, the decoded camera-space ratios cluster approximately around:

- `R/G ~ 0.57..0.61`
- `B/G ~ 0.54..0.56`

whereas the DNG `AsShotNeutral` asserts approximately:

- `R/G = 0.8701`
- `B/G = 0.3457`.

Using a diagnostic scene-neutral estimate around `[0.59, 1.0, 0.55]` removes most of the blue cast while retaining the same CFA and ForwardMatrix path. This is useful fault localization only. It is **not** a scientific calibration and must never overwrite source metadata or Dynamic Authority.

## Current interpretation

The failure is presently localized to the color / white-balance metadata binding, not to:

- physical Camera-5 route identity;
- CFA topology;
- R/B CFA phase interpretation;
- timestamp pairing;
- single-frame evidence count.

The strongest current hypothesis is that the DNG `AsShotNeutral` produced from the physical Camera-5 capture-result path is not a trustworthy scene-neutral for this capture, or that its relation to the physical Camera-5 color transform is not being represented consistently by Android/HONOR on this route.

This remains a hypothesis until capture-time standard Camera2 color metadata is recorded and compared directly with the generated DNG metadata.

## Required v0.6 capture diagnostics

Every physical-camera capture must additionally seal into its observation sidecar:

- `CONTROL_AWB_STATE`;
- `COLOR_CORRECTION_MODE`;
- `COLOR_CORRECTION_GAINS`;
- `COLOR_CORRECTION_TRANSFORM` when present;
- the physical camera's static reference illuminants, color transforms, calibration transforms and forward matrices;
- generated DNG `AsShotNeutral`, ColorMatrix and ForwardMatrix values or a deterministic post-write parser digest;
- explicit agreement/disagreement classification between capture-result white-balance state and DNG white-balance metadata.

The comparison is diagnostic/source-bound only until held-out physical color calibration exists.

## Pure / Advanced authority rule

TruthRaw Pure must not silently call this source-metadata transform physically calibrated color. If capture-result/DNG color metadata disagree or produce an unresolved color state, Pure must retain the scientific color authority as unresolved/source-bound.

TruthRaw Advanced may offer a natural/neutral appearance correction, including scene-derived neutral estimation, only as `APPEARANCE_ONLY` with no scientific writeback and no authority upgrade.

## Immediate implementation changes

1. Force TIFF/DNG Orientation=1 on every app capture path.
2. Persist the standard Camera2 color/AWB result fields from the effective physical result.
3. Add a source-bound color consistency gate before calling DNG metadata a valid Pure appearance transform.
4. Add an Advanced-only adaptive-neutral fallback for visual evaluation; never use it as calibration evidence.
5. Use the next real Camera-5 capture to determine whether the failure originates in DngCreator's physical-result metadata path, HONOR's physical-result color fields, or the TruthRaw metadata-to-appearance transform.
