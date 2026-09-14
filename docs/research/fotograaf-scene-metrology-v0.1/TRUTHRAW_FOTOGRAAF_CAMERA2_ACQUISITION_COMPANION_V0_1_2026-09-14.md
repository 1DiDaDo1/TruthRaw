# TruthRaw FotoGraaf Camera2 acquisition companion v0.1 — 2026-09-14

**Status: SOURCE-SIDE CAMERA2 OBSERVATION IMPLEMENTED / C0 PROMOTION STILL FAIL-CLOSED**

This step implements the first controlled Android source-side companion for FotoGraaf physical calibration acquisition.

It is deliberately a separate application from the normal TruthRaw reconstruction UI.

Permanent laws remain:

**Measured where measured. Reconstructed where necessary. Never invented.**

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## 1. Why this is a separate APK

The normal TruthRaw Android UI imports existing RAW/DNG files through the document picker. That route can verify source bytes after import, but it cannot retroactively recreate capture-time Camera2 identity.

The FotoGraaf companion therefore has one narrow job:

`Camera2 capture-time state -> one RAW_SENSOR frame -> finalized DNG -> post-finalization SHA-256 -> acquisition observation JSON`

It does not reconstruct the photo, does not create a Scientific Master, does not apply appearance, and does not alter the normal TruthRaw app's camera-permission boundary.

Application ID:

`com.truthraw.fotograafcapture`

Project:

`app/android/truthraw-fotograaf-capture-v01`

## 2. Camera2 evidence that is actually recorded

The companion enumerates Camera2 RAW-capable endpoints. For a logical multi-camera it also enumerates advertised physical IDs and can create a physical `OutputConfiguration` for one of those IDs.

For a capture it records:

- the opened Camera2 camera ID;
- whether it is a logical multi-camera;
- the set of advertised physical camera IDs;
- a requested physical output ID when one was explicitly targeted;
- the physical `TotalCaptureResult` when a physical output was requested and the platform returned it;
- otherwise `LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID` when the logical result actually exposes it;
- for a non-logical directly opened Camera2 endpoint, the Camera2 result camera ID as the direct endpoint identity;
- exact RAW width/height and CFA arrangement reported by Camera2;
- MANUAL_SENSOR capability;
- requested and returned ISO/exposure;
- sensor-result timestamp and RAW `Image.timestamp` with exact equality required;
- focus-distance result, OIS result and noise-reduction result when available;
- dynamic black/white observations when available;
- Android build fingerprint.

The physical-ID rules follow the Camera2 API rather than filename/focal-length inference. Android documents that logical multi-cameras expose their physical IDs through `CameraCharacteristics.getPhysicalCameraIds()`, that a physical output can be selected with `OutputConfiguration.setPhysicalCameraId()`, and that `TotalCaptureResult.getPhysicalCameraTotalResults()` carries the physical result when a physical stream was requested. `LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID` remains optional, so its absence is recorded as absence rather than guessed.

## 3. Exact DNG byte binding

The captured `RAW_SENSOR` image is written with Android `DngCreator`.

The DNG is first closed/finalized in MediaStore. Only after that does the companion reopen the exact file and compute:

- SHA-256;
- byte length.

The observation record must say:

`hashTiming = AFTER_DNG_FINALIZATION`

The host verifier can be run with the observation JSON plus the copied DNG and recomputes both byte length and SHA-256.

A changed DNG therefore breaks the acquisition observation.

## 4. Important correction: Camera2 observation is not yet the C0 envelope

The previous Capture Evidence Seal v0.1 requires complete `captureSampleDomainId` and `gainReadoutStateId` observations.

Those quantities are not safely derivable from ISO magnitude. The exact-ISO8192 experiment is the concrete reason not to create such a shortcut.

Therefore v0.1 does **not** place either field in its acquisition observation.

The observation explicitly remains:

`CAMERA2_ACQUISITION_OBSERVATION_ONLY`

with:

`c0EnvelopeReady = false`

and a `missingBeforeC0Envelope` list containing at least:

- `cameraSystemIdMapping`;
- `captureSampleDomainId`;
- `gainReadoutStateId`;
- `focusStateClass`;
- `stabilizationState`.

The host verifier rejects an observation if `captureSampleDomainId` or `gainReadoutStateId` is smuggled into this pre-classification record.

## 5. Camera-system ID is not silently equated to the historical lens label

The earlier Honor/MotionCam research identified the tele lens as System ID 5.

The Camera2 companion does **not** assume that Android's opened Camera2 ID is numerically identical to that historical source-app label. It records Camera2 identity exactly as Camera2 exposes it.

The future C0 promotion step must either demonstrate the mapping or revise the C0 scope semantics with explicit evidence. A coincidental string `"5"` is not proof by itself.

This prevents a source-app identifier from being silently reinterpreted as a different API namespace.

## 6. Honor tele target

The research target remains:

- HONOR BKQ-N49;
- tele lens historical role;
- 4080 × 3072;
- BGGR.

The app sorts exact `4080x3072 + BGGR` candidates to the top when available, but it still displays the observed Camera2 endpoint identity.

A topology match is only a candidate match. It does not independently prove the physical lens.

## 7. Capture controls

v0.1 requires `REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR` before it will issue a calibration observation capture.

The requested capture uses:

- AE off;
- explicit sensor sensitivity;
- explicit sensor exposure time;
- AF off with infinity-focus request (`0.0` diopters);
- OIS off when the endpoint reports that OFF is supported;
- noise reduction OFF when reported as supported.

The observation records the returned values. Requested values are never substituted for missing capture-result values.

For a requested physical output the application also applies the sensor sensitivity and exposure time through `setPhysicalCameraKey` for that physical ID.

## 8. Timestamp identity gate

One RAW image is paired with one `TotalCaptureResult`.

The companion requires:

`CaptureResult.SENSOR_TIMESTAMP == Image.timestamp`

before writing the observation.

A mismatch fails closed. This avoids binding the metadata of one capture to the RAW payload of another.

This is still one physical frame and one independent scene-evidence root.

## 9. Output location

The APK writes into scoped MediaStore Downloads:

```text
Download/TruthRawFotoGraaf/<session>/
  captures/
    <capture>.dng
  observations/
    <capture>_camera2_observation_v0_1.json
```

The JSON is an acquisition observation, not a calibration certificate.

## 10. Host verification

Contract:

`docs/research/fotograaf-scene-metrology-v0.1/TRUTHRAW_FOTOGRAAF_CAMERA2_ACQUISITION_OBSERVATION_CONTRACT_V0_1.json`

Verifier:

`tools/verify_fotograaf_camera2_acquisition_observation_v0_1.py`

Example:

```text
python3 tools/verify_fotograaf_camera2_acquisition_observation_v0_1.py \
  --contract docs/research/fotograaf-scene-metrology-v0.1/TRUTHRAW_FOTOGRAAF_CAMERA2_ACQUISITION_OBSERVATION_CONTRACT_V0_1.json \
  --observation <camera2-observation.json> \
  --source <exact-finalized-source.dng>
```

The verifier checks the exact source hash/length, Camera2 identity structure, RAW topology, exact image/result timestamp identity, 1/1 evidence counts, and the fact that sample/gain domain classifications are still absent.

## 11. Current authority status

- standalone source-side Camera2 companion: **IMPLEMENTED IN SOURCE / APK BUILD GATE ADDED**;
- physical Camera2 ID through requested physical result: **SUPPORTED**;
- active physical result when exposed by logical camera: **SUPPORTED**;
- directly opened non-logical Camera2 endpoint result: **SUPPORTED AS DIRECT ENDPOINT EVIDENCE**;
- ordinary DNG metadata as physical-camera authority: **REJECTED**;
- ISO-derived sample-domain identity: **REJECTED**;
- ISO-derived gain/readout-state identity: **REJECTED**;
- Camera2 observation as a complete C0 envelope: **NO**;
- real Honor device execution: **OPEN**;
- real sample-domain classifier: **OPEN**;
- real gain/readout-state classifier: **OPEN**;
- Scientific Master/reconstruction change: **NONE**;
- calibration authority granted by this step: **NONE**.

## 12. Next gate

The next scientific implementation is not to weaken the envelope contract.

It is to build the missing classifier bridge:

`verified Camera2 observation + exact DNG payload/metadata`

`-> validated captureSampleDomainId decision`

`-> validated gainReadoutStateId decision`

`-> focus/stabilization state classification`

`-> explicit Camera2-to-C0 camera-system mapping evidence`

`-> full Capture Evidence Envelope`

`-> existing post-finalization Capture Evidence Seal`

Only then can a real capture become eligible for the existing C0 -> C1/C2 calibration intake.
