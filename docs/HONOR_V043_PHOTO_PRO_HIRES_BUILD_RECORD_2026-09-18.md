# HONOR v0.43 PHOTO / PRO / HI-RES passive timeline — build record

Date: 2026-09-18

## Triggering UI observation

During preparation of v0.43 the user showed the current fresh Honor Camera "More" mode grid.
A separate mode literally labelled "200 MP" was not visible. The visible relevant labels were PHOTO,
PRO and HI-RES.

The experiment was therefore changed before release:

- removed `PROFILE_200MP`;
- removed `MODE_200MP_SELECTED`;
- added PHOTO control profile;
- added PRO control profile;
- added HI-RES candidate profile;
- HI-RES is not semantically promoted to 200 MP.

This preserves the project's rule that UI/vendor names are observations, not scientific semantics.

## Released profiles

- `FRESH_PHOTO_CONTROL_REPLICATE`
- `FRESH_PRO_CONTROL_REPLICATE`
- `FRESH_HIRES_CANDIDATE_REPLICATE`

## Marker design

Foreground Honor Camera can remain open while the TruthRaw foreground-service notification supplies
three actions:

- MAIN STABLE
- MODE SELECTED
- SHUTTER PRESSED

MODE SELECTED is resolved using the chosen run profile into one of:

- `MODE_PHOTO_SELECTED`
- `MODE_PRO_SELECTED`
- `MODE_HIRES_SELECTED`

All markers are user timing observations on `elapsedRealtimeNs`; none is a hardware shutter timestamp.

## MediaStore temporal sampling

Every MediaStore change receives a unique `changeGroupId` and metadata-only snapshots scheduled at:

`0, 25, 100, 250, 1000 ms`

Each snapshot records its actual monotonic delay in addition to the requested delay.

This is intended to detect transient row state, size, dimensions or `IS_PENDING` changes without
opening the image file.

## Scientific invariants

Build checks confirm that the v0.43 service contains no:

- CameraManager `openCamera`
- CaptureRequest creation
- ImageReader
- media `openInputStream`
- Bitmap/ImageDecoder
- ExifInterface
- Honor `bindService`
- physical camera-key write

Authority remains:

`PASSIVE_SYSTEM_VISIBLE_OUTPUT_OBSERVATION_ONLY`

## Repository

Branch:

`integration/truthraw-suite-v0-43-passive-mode-shutter-timeline`

Build commit:

`8bc6805387e22d3ffe5fe584945d25bf1d422106`

GitHub Actions run:

`35389948999`

Job:

`105745719274`

Result:

`SUCCESS`

Invariant log:

`v0.43 PHOTO/PRO/HI-RES passive timeline invariants PASS`

Gradle:

`BUILD SUCCESSFUL in 1m 42s`

## APK

CI APK bytes:

`5,151,689`

CI APK SHA-256:

`58e14d06b475205e1e67a76ad30c8f3065d6618a2531d6da77c4bb6ccc1a9987`

Artifact:

- ID: `10565886038`
- name: `truthraw-suite-v0-43-passive-mode-shutter-timeline-debug-arm64`
- ZIP bytes: `1,682,440`
- artifact digest:
  `sha256:4b63acd667e2ee2b73eae62843d1c8d92a057cfc9cf88925672d084ccfca4001`

The downloaded artifact was independently extracted and the APK rehashed locally. Local byte count and
SHA-256 exactly match CI.

## Run interpretation rule

If HI-RES can be selected, it is tested as an unknown high-resolution UI mode.

Only replicated device output may associate it with the previously observed 16320x12288 system-visible
output class.

If HI-RES is unavailable/unusable on a particular fresh state, do not force or reinterpret it. PHOTO
and PRO runs remain valid independent controls.
