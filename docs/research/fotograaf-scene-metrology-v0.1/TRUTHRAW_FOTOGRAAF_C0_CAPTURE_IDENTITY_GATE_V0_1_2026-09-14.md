# TruthRaw FotoGraaf C0 Capture Identity Gate v0.1 — 2026-09-14

**Status: SOFTWARE GATE IMPLEMENTED / PHYSICAL HONOR TELE C0 STILL OPEN**

C0 is the boundary before C1–C7 calibration acquisition. Its job is narrow: prove that a calibration capture belongs to one exact camera/lens/mode/sample-domain scope before any dark, linearity, flat, color or radiometry frame is admitted.

It does **not** grant `CALIBRATED_PHYSICAL` authority, does not alter a later scene's Scientific Master, and does not change `physicalFrameCount=1` or `independentEvidenceCount=1`.

Permanent laws remain:

**Measured where measured. Reconstructed where necessary. Never invented.**

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## What v0.1 seals

The gate binds an exact source file and an exact metadata sidecar by SHA-256 and byte length, then binds those bytes to a canonical `CalibrationScopeKey` containing the same sixteen fields already required by the dataset-manifest contract:

- device make/model;
- observed Android camera/system ID;
- observed physical camera ID;
- lens role;
- capture API/source domain;
- capture mode;
- RAW width/height;
- CFA pattern;
- sample representation;
- capture-sample-domain ID;
- firmware/build identity;
- focus-state class;
- stabilization state;
- protocol version.

The gate additionally requires acquisition-time observations for camera/system ID, physical camera ID, capture API domain, capture-sample-domain ID, firmware/build identity, focus-state class and stabilization state. The observed values must equal the values sealed into the scope.

For the current Honor tele program, the profile is intentionally strict: `HONOR`, `BKQ-N49`, project camera/system ID `5`, lens role `TELE`, `4080x3072`, `BGGR`. Camera/system ID `5` is still required to be observed again during acquisition; the project declaration alone is not enough.

## Fail-closed rules

C0 rejects unresolved placeholders, source or metadata byte mismatches, scope/observation mismatches, an inferred physical camera ID, an ISO-derived sample-domain ID, processed-RGB input, merged multi-frame evidence, or a source that is not declared direct CFA.

A normal DNG's focal length or metadata is not sufficient to prove Android physical-camera identity. That identity must come from the capture path or a capture-bound sidecar.

Likewise, ISO magnitude may not select `captureSampleDomainId`. If the camera changes gain/readout/sample behavior, that state must be classified from acquisition evidence.

## Machine-verifiable path

Contract:

`TRUTHRAW_FOTOGRAAF_C0_CAPTURE_IDENTITY_CONTRACT_V0_1.json`

Verifier:

`tools/verify_fotograaf_c0_capture_identity_v0_1.py`

A physical seal requires all three inputs:

```text
python3 tools/verify_fotograaf_c0_capture_identity_v0_1.py \
  --contract docs/research/fotograaf-scene-metrology-v0.1/TRUTHRAW_FOTOGRAAF_C0_CAPTURE_IDENTITY_CONTRACT_V0_1.json \
  --record <capture-c0-record.json> \
  --source <exact-source.dng-or-raw> \
  --metadata-snapshot <exact-capture-sidecar.json> \
  --json-out <c0-result.json>
```

Success is exactly:

`C0_IDENTITY_SEALED`

That result means only that the capture identity/scope is sealed. It does not mean the camera is calibrated.

## Current physical status

The software gate is implemented and regression-tested. The actual Honor tele physical C0 remains **OPEN** until a new controlled acquisition provides capture-bound values for the unresolved fields, especially `physicalCameraId`, `captureApiDomain`, `captureSampleDomainId`, focus/stabilization state and exact firmware identity alongside the source bytes.

The previously supplied ordinary DNG/ISO series remains useful test evidence, but is not silently upgraded into C0 calibration evidence because it does not by itself prove all acquisition-identity fields.

## Next physical step

Acquire one controlled tele identity run before the first C1 dark batch. Save the exact RAW/DNG bytes and an acquisition sidecar from the same capture session. Rehash both through this gate. Only after `C0_IDENTITY_SEALED` may C1/C2 frames from that same exact scope enter a calibration dataset manifest.
