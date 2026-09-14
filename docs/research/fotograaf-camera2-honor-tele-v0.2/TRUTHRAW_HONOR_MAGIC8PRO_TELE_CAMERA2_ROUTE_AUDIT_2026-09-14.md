# TruthRaw FotoGraaf — HONOR Magic 8 Pro tele Camera2 route audit

Date: 2026-09-14
Status: **GENERIC V0.1 TELE ROUTE FAIL / HONOR-SPECIFIC V0.2 CANDIDATE IMPLEMENTED**
Device: HONOR BKQ-N49 / Android 16

## Empirical trigger

Three on-device FotoGraaf v0.1 captures were supplied after the combined TruthRaw + FotoGraaf APK was installed.

### Capture A

`C2OBS_20260914_174825_895_4096x3072.dng`

- source SHA-256: `a85cac58601d6cc8138bf8f9372e5161b85ab1e89afa70372f97c46461dcea79`
- Camera2 logical ID: `0`
- logical multi-camera: `true`
- advertised physical IDs: `2`, `4`, `5`
- requested physical ID: `null`
- measured active physical result: `2`
- RAW: `4096x3072`, BGGR, RAW_SENSOR
- ISO 100, result exposure 16,367,389 ns
- exact image/result timestamp match
- DNG focal-length metadata observed during audit: about 6.55 mm

Conclusion: **not a tele capture**. The logical camera auto-selected physical ID 2.

### Capture B

`C2OBS_20260914_174805_731_4096x3072.dng`

- source SHA-256: `f410885a8c040820e64d2b9785856a45e6af53b9a5edba4805f5a409e948de93`
- direct Camera2 ID: `1`
- logical multi-camera: `false`
- no advertised physical IDs
- RAW: `4096x3072`, GRBG, RAW_SENSOR
- DNG focal-length metadata observed during audit: about 2.98 mm

Conclusion: not the desired rear tele route.

### Capture C

`C2OBS_20260914_174527_215_4096x3072.dng`

- source SHA-256: `57f58ee9d3396242a23d86f7c42f78e4267bbeb512256c88d8e59527d2888a21`
- same direct Camera2 ID `1` route as Capture B
- RAW: `4096x3072`, GRBG, RAW_SENSOR

Conclusion: not the desired rear tele route.

## Root cause in generic v0.1

The generic endpoint enumerator only offered a physical endpoint when the physical camera's own queried characteristics advertised both `REQUEST_AVAILABLE_CAPABILITIES_RAW` and a non-empty `RAW_SENSOR` stream map.

That assumption is too strong for logical multi-camera vendor stacks. Android permits physical IDs that are not standalone camera IDs and expects physical streams to be requested through the logical camera. Therefore an advertised physical ID can still be scientifically interesting even if its standalone-style metadata does not mirror the logical RAW capability surface.

A second weakness was that generic v0.1 created the request with `createCaptureRequest(template)` and only later attempted `setPhysicalCameraKey`. The explicit physical-camera request builder overload, `createCaptureRequest(template, physicalCameraIdSet)`, is the correct contract for requests that customize a physical camera.

## v0.2 HONOR tele candidate route

New activity:

`com.truthraw.fotograafcapture.HonorTeleActivity`

The route is deliberately fail-closed:

1. Find a logical multi-camera that advertises physical ID `5`.
2. Query physical ID `5` characteristics directly.
3. Prefer a physical RAW_SENSOR size; if the vendor hides that map, use the logical RAW_SENSOR stream size only as a **routing candidate**, not as proof that physical ID 5 has RAW authority.
4. Create `OutputConfiguration(rawSurface)` and call `setPhysicalCameraId("5")` before session creation.
5. Build the capture request with `createCaptureRequest(TEMPLATE_STILL_CAPTURE, setOf("5"))`.
6. Apply physical request keys only when listed by `availablePhysicalCameraRequestKeys`.
7. Accept the frame only when `TotalCaptureResult.physicalCameraTotalResults["5"]` exists and its `cameraId` equals `5`.
8. Require physical result timestamp == RAW Image timestamp.
9. Build the DNG with physical ID 5 characteristics + the physical ID 5 result.
10. Record route identity and output hash after DNG finalization.

## Scientific authority

A successful v0.2 route proves only:

`requested physical ID 5 -> Camera2 returned a matching physical result for ID 5 -> the RAW image is timestamp-bound to that result`

It does **not** by itself prove the historical project label `tele 3.7x` as a calibration authority. The lens-role mapping remains a project candidate until independently cross-validated using source identity, focal/lens metadata and controlled captures.

It also does not create:

- `captureSampleDomainId`;
- `gainReadoutStateId`;
- independent physical color calibration;
- lens/PSF calibration;
- CalibrationPack promotion.

Those remain fail-closed.

## Acceptance test for the next device run

A new capture is a routing PASS only if the v0.2 observation says:

- `routeClass = LOGICAL_MULTI_CAMERA_FORCED_PHYSICAL_OUTPUT`
- `requestedPhysicalCameraId = 5`
- `confirmedPhysicalResultCameraId = 5`
- `timestampMatch = true`
- DNG hash is non-empty and stable after finalization.

If the session itself is rejected, record `PHYSICAL_RAW_SESSION_REJECTED`. That is useful evidence that the vendor HAL does not permit this physical RAW stream combination and a different, explicitly validated routing strategy is required. Do not silently fall back to a main-sensor crop and call it tele.
