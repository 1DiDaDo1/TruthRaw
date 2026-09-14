# TruthRaw × FotoGraaf — HONOR Native RAW Probe v0.2

Status: **RESEARCH DESIGN STARTED — stock-guided, runtime-proof required**

## Purpose

Improve the TruthRaw/FotoGraaf Android camera acquisition path by learning from the stock HONOR Camera APK without allowing APK content to become TruthRaw scientific evidence, calibration, color authority, noise authority or topology authority.

The stock APK is a **technical route map only**. Every usable route must be rediscovered through Camera2 on the actual device and then proven by the capture result and sealed RAW.

## Existing physical proof retained

The previous route remains valid as a known physical-capture result for its exact lineage:

`logical camera 0 -> forced physical output 5 -> RAW_SENSOR 4080x3072 BGGR`

The capture was accepted only when the physical result confirmed ID 5 and the RAW image timestamp matched the capture-result `SENSOR_TIMESTAMP` exactly. This v0.2 work does not weaken that gate.

## New stock-guided discovery

Static inspection of the supplied stock `Camera.apk` (SHA-256 `3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52`) shows that HONOR's own code knows a vendor characteristic named:

`com.hihonor.device.capabilities.professionalTeleRawLogicalCameraID`

The stock app's `CameraUtil.getTeleRawCameraId()` reads that characteristic, and the stock zoom controller references it in ProPhoto RAW logic. This makes a **HAL-advertised professional-tele RAW logical route** a high-priority runtime candidate, but not a fact until the device reports it.

Other stock-guided characteristics and request/result tags are recorded in `HONOR_VENDOR_TAG_REGISTRY_v0_2.json`. Their static expected Java types are clues only and must be checked against the device/runtime.

## v0.2 acquisition sequence

### Phase A — read-only inventory

For every Camera2 camera ID:

1. record standard characteristics, logical/physical topology, CFA, pixel/active arrays, focal lengths, focus/OIS capability, RAW/max-resolution stream maps and available capabilities;
2. enumerate all runtime `CameraCharacteristics` keys whose names begin with `com.hihonor.` and serialize their values plus the runtime value class where available;
3. enumerate available capture-request, capture-result and physical-camera request keys by name;
4. specifically read the stock-guided RAW/tele/remosaic characteristics when they are actually exposed;
5. do not set a vendor request in this phase.

The inventory is `CAPABILITY_OBSERVATION_ONLY` and cannot grant C0 or calibration authority.

### Phase B — route candidates

Create separate candidates rather than silently merging them:

- **Route A:** HAL-advertised `professionalTeleRawLogicalCameraID`, if present and openable;
- **Route B:** existing proven `logical 0 -> physical output 5` path;
- **Route C:** maximum-resolution/remosaic RAW path only when standard or vendor capabilities advertise it;
- **Route D:** ordinary logical-camera RAW path for main/wide comparison.

No candidate inherits the authority of another candidate.

### Phase C — capture proof

A candidate capture is admitted only when all applicable checks pass:

- one physical frame and one independent evidence root;
- `Image.timestamp == TotalCaptureResult[SENSOR_TIMESTAMP]` exactly;
- requested logical and physical route recorded;
- actual `TotalCaptureResult.physicalCameraResults` recorded when available;
- runtime HONOR result `previewCameraPhysicalId` recorded when exposed;
- runtime HONOR `opticalSwitchStatus` recorded when exposed;
- actual RAW format, dimensions, CFA, dynamic black/white and sensor metadata recorded;
- no processed-RGB input is relabelled as Direct CFA;
- no multi-frame merge is relabelled as a single Direct-CFA observation.

A stock APK hint can never substitute for any of these checks.

## Request-policy change

v0.2 deliberately separates **discovery** from **control**.

Unknown vendor requests are never written just because their names were found in the stock app. A request may enter an A/B experiment only if:

1. the device advertises the exact request-key name;
2. the runtime or a separately verified static mapping establishes the expected value type;
3. the baseline route succeeds without it;
4. the experiment changes one controlled setting at a time;
5. capture/result metadata and RAW hashes are retained for both baseline and treatment;
6. failure causes rollback, never a weaker proof rule.

`cameraSaveRawMode`, optical-zoom controls and remosaic controls therefore begin as **experimental execution controls**, never as evidence/calibration facts.

## Focus, OIS and exposure work

The next static-analysis target is the stock ProPhoto/RAW path for:

- AF mode and trigger sequence;
- manual-focus/focus-distance behavior;
- OIS enable/disable policy;
- AE/manual exposure transition;
- RAW-tele switching thresholds;
- request ordering around a physical/logical camera switch.

These settings can improve usability and repeatability, but the observed result state must remain the provenance authority. A requested OIS/AF state is not the same as a proven capture state.

## App/UI target

The next Android UI should expose three clearly different pages:

1. **HONOR Inventory** — read-only runtime characteristics/vendor-key scan;
2. **RAW Route Proof** — capture one selected route and emit sealed DNG + observation JSON;
3. **Experimental Controls** — explicit A/B request experiments, visually marked `NON_CALIBRATION_EXPERIMENT`.

This prevents a convenient camera control from silently becoming a scientific claim.

## Output contract

Each capture-sidecar should bind:

- source DNG SHA-256;
- device/build fingerprint;
- stock-hint APK hash used for research, if any;
- logical camera ID;
- requested physical ID;
- confirmed physical result IDs;
- runtime vendor result values used only as corroborating route metadata;
- actual standard Camera2 capture metadata;
- C0 unresolved fields;
- explicit authority fields.

Default authority remains `CAMERA2_ACQUISITION_OBSERVATION_ONLY` until the separate C0/calibration gates are satisfied.

## Permanent boundary

**The stock HONOR APK may tell FotoGraaf where to look. Only the actual device capture may tell TruthRaw what happened.**
