# HONOR Camera-5 physical focus probe — v0.18 — 2026-09-17

Status: **EXPERIMENTAL ACQUISITION / FOCUS-PROVENANCE PROBE**

## Motivation

The v0.17 Camera-5 airlock proved that the physical Camera-5 200 MP RAW route can be observed on both sides of the HONOR/QTI vendor pipeline while preserving the original app-visible RAW_SENSOR buffer first.

The qualifying v0.17 device evidence also exposed three useful focus observations for the same physical result:

- standard Android `android.lens.focusDistance`;
- standard Android AF/lens state;
- QTI vendor observations including `org.quic.camera.afData.lenspos`, `isPDEnable`, and `PDType`.

At the same time, the logical camera advertised no physical-camera request override keys even though the already-proven physical `SENSOR_PIXEL_MODE` write succeeds on this device. Therefore absence from `availablePhysicalCameraRequestKeys` is treated as lack of API advertisement, not as proof that every physical override will fail.

## Goal

v0.18 tests one narrow question:

> Can a standard Camera2 `LENS_FOCUS_DISTANCE` request, scoped specifically to physical Camera 5, cause a reproducible change in returned Camera-5 focus state while preserving the proven 200 MP acquisition route?

This is a control/provenance experiment. It does not change the Scientific Master and does not treat a successful setter call as proof of physical lens movement.

## Preserved acquisition topology

The v0.14/v0.17 route remains authoritative:

`logical camera 0`

`-> physical output bound to camera 5`

`-> MAXIMUM_RESOLUTION output declaration`

`-> physical-scoped still request`

`-> 16320x12288 RAW_SENSOR`

`-> exact image/physical-result timestamp binding`

`-> original Image.Plane[0] persisted and SHA-256 sealed first`

`-> post-HAL HardwareBuffer/vendor envelope observed read-only`

The default focus mode is `AUTO_DEFAULT`; in that mode v0.18 writes no focus key and should reproduce v0.17 focus behavior.

## Manual probe modes

The UI cycles through four modes for the next Stage-3 capture:

- `AUTO_DEFAULT` — no manual focus write;
- `INFINITY_0D` — request 0.0 diopters;
- `MID_50PCT_MIN_FOCUS` — request 50% of Camera-5 `LENS_INFO_MINIMUM_FOCUS_DISTANCE`;
- `NEAR_85PCT_MIN_FOCUS` — request 85% of Camera-5 `LENS_INFO_MINIMUM_FOCUS_DISTANCE`.

For a manual mode, v0.18 attempts only standard Camera2 keys as physical-camera-5 overrides:

- `CONTROL_AF_MODE = OFF`;
- `LENS_FOCUS_DISTANCE = requestedDiopters`.

No HONOR/QTI vendor key is written.

There is deliberately no global-focus fallback. If the physical write is rejected, the error is evidence.

## Evidence recorded per capture

The v0.18 evidence JSON adds `focusProbe` containing:

- selected mode;
- Camera-5 minimum-focus-distance characteristic;
- requested focus distance;
- whether the physical override was advertised;
- whether physical AF-OFF and focus-distance writes were accepted;
- write errors if any;
- returned Android AF mode/state, lens state, focus distance and focus range;
- observed QTI `lenspos`, `isPDEnable`, and `PDType` values;
- observed HONOR AF state;
- an explicit proof-status field.

Vendor values remain observations with unknown vendor semantics. Their names alone are not promoted to scientific meaning.

## Proof rule

A single successful request write is **not** physical-focus proof.

Minimum useful differential evidence is at least two captures of the same static target from the same position with clearly separated requested focus states, for example:

`INFINITY_0D`

versus

`NEAR_85PCT_MIN_FOCUS`.

A physical-focus control claim requires reproducible differences in the returned standard focus state and/or QTI lens-position observation that track the requested direction. Image-domain sharpness/PSF change is a later independent confirmation.

## Multi-camera consequence

If physical Camera-5 focus control is proven, the next experiment may attempt a multi-camera/multi-focus session. That remains separate work because the current device does not advertise `availablePhysicalCameraRequestKeys` and because a dual physical RAW stream combination must be independently admitted by Camera2/session support.

## Scientific authority

Each v0.18 capture remains one physical frame and one independent evidence source:

`physicalFrameCount = 1`

`independentEvidenceCount = 1`

Focus request/result/vendor metadata are capture provenance. They do not create extra scene evidence.

## Boundaries

- app-visible Camera2 RAW_SENSOR is not promoted to untouched photodiode/ADC truth;
- a requested focus distance is not assumed to equal the physical lens position;
- vendor `lenspos` is not assigned physical units without calibration or authoritative documentation;
- F64/FP32 reconstruction and TruthRange zero-line are downstream and are not used in this acquisition-control experiment.
