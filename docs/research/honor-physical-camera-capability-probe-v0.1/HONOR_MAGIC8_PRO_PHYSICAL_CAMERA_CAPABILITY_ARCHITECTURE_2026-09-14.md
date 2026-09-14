# HONOR Magic8 Pro physical-camera capability architecture — 2026-09-14

Status: **ACTIVE RESEARCH / CAPABILITY OBSERVATION ONLY / FAIL CLOSED**

## Purpose

Before adding more hard-coded capture buttons, TruthRaw must determine what the HONOR BKQ-N49 Camera2 stack actually advertises for each physical rear camera and each sensor mode.

The target is one generic physical-camera engine that can later support:

- main-camera RAW;
- ultrawide RAW;
- tele RAW;
- maximum-resolution RAW where advertised;
- close-focus / macro where physically supported;
- RAW14 when an Android 17+ runtime and the physical camera both advertise it.

A capability is not capture proof and is never calibration authority by itself.

## Existing physical evidence that motivated this probe

Real FotoGraaf observations on BKQ-N49 showed:

- logical Camera2 ID `0` is a logical multi-camera;
- it advertised physical IDs `2`, `4`, `5`;
- when no physical ID was requested, Camera2 selected active physical ID `2`;
- that observation produced one `RAW_SENSOR` frame at `4096x3072`, BGGR, with dynamic WhiteLevel `1023`;
- separate direct-camera observations through logical/direct ID `1` were `4096x3072`, GRBG and did not represent the desired tele route.

Therefore automatic logical-camera lens selection is rejected for calibration acquisition when a specific physical sensor is required.

The historical project mapping remains a hint only:

- ID `2` -> main candidate;
- ID `4` -> ultrawide candidate;
- ID `5` -> tele candidate.

The mapping becomes authoritative for a capture only after Camera2 returns a matching physical `TotalCaptureResult` for the requested physical ID and the RAW timestamp matches that result.

## Capability matrix to observe

For every advertised physical ID, record independently:

1. standard `RAW_SENSOR` output sizes;
2. `SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION` `RAW_SENSOR` sizes;
3. standard `RAW14` sizes when the runtime exposes `ImageFormat.RAW14`;
4. maximum-resolution `RAW14` sizes when available;
5. pixel-array and active-array dimensions;
6. maximum-resolution pixel-array and active-array dimensions;
7. CFA arrangement;
8. MANUAL_SENSOR / RAW / ultra-high-resolution capability flags;
9. sensitivity and exposure ranges;
10. focal lengths;
11. minimum-focus-distance metadata and focus-distance calibration;
12. advertised AF modes, including `CONTROL_AF_MODE_MACRO`;
13. OIS modes.

These observations determine which capture experiments are scientifically justified. They do not themselves certify that a stream can be opened or that a requested physical sensor produced the file.

## Native 200 MP tele rule

A 200 MP-class sensor specification is not enough.

TruthRaw may call the tele path a **native maximum-resolution RAW candidate** only if Camera2 advertises a maximum-resolution RAW stream near the full sensor lattice for the requested physical route.

The current engineering threshold for a `200MP-class candidate` is deliberately only a candidate marker: largest advertised maximum-resolution RAW >= `180,000,000` pixels.

Promotion from candidate to physical capture proof additionally requires:

`logical physical-output session -> requested physical ID -> matching physical TotalCaptureResult -> RAW timestamp identity -> finalized source SHA-256 -> C0 identity seal`

If the stock HONOR camera can make a 200 MP processed image while Camera2 does not expose a comparable RAW stream, TruthRaw must not invent a Direct-CFA 200 MP route.

## Main and ultrawide rule

Main and ultrawide use the same physical-camera architecture as tele. They are not special-case pipelines.

Per physical camera, TruthRaw should eventually expose only modes that pass the same admission sequence:

`capability observation -> session support -> physical capture proof -> sealed source -> C0`

This allows the main and ultrawide to have different standard/max-resolution sizes without changing scientific rules.

## Macro / close-focus rule

`macro` is not inferred from a marketing label or digital crop.

The capability probe records:

- `LENS_INFO_MINIMUM_FOCUS_DISTANCE`;
- focus-distance calibration class;
- available AF modes;
- whether `CONTROL_AF_MODE_MACRO` is advertised.

A future physical macro mode requires an actual physical capture through the selected camera and validation of focus behavior.

For the ultrawide, a vendor macro mode may be implemented by close focus on the physical wide sensor, by automatic logical-camera switching, or by a vendor-private path. TruthRaw must identify which one occurs before calling it a physical ultrawide macro RAW mode.

For tele, the project should use the term **tele close focus** until measured reproduction/focus evidence justifies a macro label.

## RAW14 / Android 17 rule

RAW14 support is separated into three questions:

1. does the runtime platform expose `ImageFormat.RAW14`?
2. does the selected camera advertise one or more RAW14 stream sizes?
3. can a physical capture be completed and its byte/sample semantics validated?

The current branch compiles against Android API 35. Therefore the probe uses runtime reflection for `ImageFormat.RAW14` so Android-16 builds remain valid while an Android-17+ device can reveal RAW14 availability without prematurely raising the entire project compile SDK.

No RAW14 route is considered available merely because Android 17 exists.

RAW14 and 200 MP are independent capabilities. Possible valid devices may expose, for example:

- max-resolution RAW10/RAW_SENSOR but no RAW14;
- binned RAW14 but no max-resolution RAW14;
- RAW14 on one physical camera and not the others.

TruthRaw must preserve that matrix rather than combine capabilities that were never advertised together.

## Probe implementation

Android activity:

`app/android/truthraw-fotograaf-capture-v01/app/src/main/java/com/truthraw/fotograafcapture/HonorCapabilityProbeActivity.kt`

It creates a read-only JSON observation:

`truthraw.fotograaf-honor-physical-camera-capability-probe.v0.1`

The report is saved below:

`Download/TruthRawFotoGraaf/capability-probes/`

Each physical route contains:

- logical camera ID;
- physical camera ID;
- historical role hint with explicitly non-authoritative status;
- logical RAW maps;
- physical characteristics when readable;
- standard/max-resolution RAW status;
- RAW14 status;
- close-focus/macro status;
- 200 MP-class candidate status.

The report explicitly sets:

- `captureAuthorityGranted=false`;
- `calibrationAuthorityGranted=false`;
- `physicalLensIdentityProvenForFutureCapture=false`;
- `native200MpRawProven=false`;
- `raw14CaptureProven=false`;
- `macroCaptureProven=false`.

## Required next phases

### Phase A — capability probe

Current step. Read characteristics only.

### Phase B — runtime session probe

For each candidate physical route/mode, test whether a physical output session can be configured. Maximum-resolution candidates must use the matching maximum-resolution sensor pixel mode.

### Phase C — physical capture proof

Capture one RAW and require:

- exact requested physical ID;
- matching physical `TotalCaptureResult`;
- sensor/image timestamp identity;
- exact result ISO/exposure provenance;
- finalized source SHA-256.

### Phase D — C0 identity

Only after Phase C may the camera-system mapping and route identity be bound into a C0 envelope. `captureSampleDomainId` and `gainReadoutStateId` remain separately classified and may not be derived from ISO alone.

### Phase E — calibration acquisition

Only routes that pass C0 are candidates for C1/C2/C3/... controlled calibration captures.

## Current status

- Generic logical-camera automatic lens selection for tele calibration: **FAIL / REJECTED**.
- Forced physical ID 5 tele route: **IMPLEMENTED, device validation OPEN**.
- Generic physical-camera capability probe for IDs 2/4/5 and any other advertised physical camera: **IMPLEMENTED, device observation OPEN**.
- Main native max-resolution RAW: **OPEN**.
- Ultrawide native max-resolution RAW: **OPEN**.
- Tele native 200 MP-class RAW: **OPEN**.
- Physical ultrawide macro RAW: **OPEN**.
- Tele close-focus RAW: **OPEN**.
- RAW14 on any lens: **OPEN until Android 17+ runtime plus camera advertisement plus capture proof**.

Scientific laws remain unchanged:

**Measured where measured. Reconstructed where necessary. Never invented.**

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
