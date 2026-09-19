# TruthRaw v0.55 — current Android 17 + installed HONOR Camera baseline

Date: 2026-09-19

Status: **READ-ONLY ACQUISITION BASELINE / NO CAPTURE / NO SCIENTIFIC WRITEBACK**

Branch:

`integration/truthraw-suite-v0-55-current-android17-honor-camera-baseline`

## Why this baseline exists

The project history now contains multiple Android/HONOR states that must not be collapsed:

- Android 16 with HONOR Camera `171.0.10.452`;
- Android 17 v0.47/v0.48 observations with HONOR Camera `171.0.10.706`;
- Android 17 v0.53 active replay of the proven Camera-5 v0.14 route;
- Android 17 v0.54 passive HONOR Pro TELE DNG fingerprint;
- the user's current Android 17 installation, reported to be newer again.

The third-chat history also established that blind vendor-key sweeps are low-value after the v0.30e/v0.31–v0.38 line: tested vendor-key combinations kept reproducing the same 4080x3072 meaningful payload topology inside the 16320x12288 envelope.

Therefore v0.55 starts with identity and capability measurement before any new active route experiment.

## Historical anchors that v0.55 must preserve

### Camera-5 topology

The later v0.19/v0.20 audit superseded the earlier shorthand “200MP RAW proven”.

The observed third-party Camera2 route was:

- declared/envelope geometry: `16320x12288`;
- envelope bytes: `401,080,320`;
- populated rows: `0..767`;
- all-zero rows: `768..12287`;
- meaningful contiguous prefix: `25,067,520` bytes;
- exact U16 byte-equivalent geometry: `4080x3072`.

v0.53 reproduced that topology on Android 17.

### OEM Pro TELE control

v0.54 then measured an HONOR-produced Pro TELE DNG:

- `4080x3072`;
- uncompressed CFA;
- 16-bit storage;
- BlackLevel 64;
- WhiteLevel 1023;
- 3072 strips × 8160 bytes;
- total RAW payload `25,067,520` bytes;
- 22.48 mm / f2.6 tele identity.

This is strong structural corroboration of the ordinary 4080x3072 tele RAW domain, not proof of untouched ADC topology or of the OEM 200MP JPEG construction path.

## v0.55 purpose

Create an exact read-only baseline of the **currently installed** software/capability state before re-running any active Camera-5 route.

The baseline records:

1. Android runtime/build identity:
   - SDK/release;
   - build fingerprint;
   - build ID/display/incremental;
   - security patch;
   - build type/tags/time.

2. TruthRaw probe identity:
   - package version;
   - target/min/compile SDK.

3. Actually installed `com.hihonor.camera` identity:
   - version name/code;
   - target/min/compile SDK;
   - system/update flags;
   - install/update timestamps;
   - SHA-256 and byte length of base/split APK archives where readable.

4. Camera2 public/disclosed-physical topology:
   - camera IDs and physical IDs;
   - focal lengths/apertures;
   - UHR capability surface;
   - standard/max-resolution sensor geometry;
   - binning factor;
   - CFA/black/white metadata;
   - RAW_SENSOR / RAW10 / RAW12 / runtime RAW14 output sizes;
   - public characteristic/request/result key names;
   - public Camera Extension results.

## Strict boundary

Authority:

`ANDROID17_HONOR_CAMERA_PACKAGE_AND_CAMERA2_CHARACTERISTICS_BASELINE_ONLY`

v0.55:

- opens no CameraDevice;
- submits no CaptureRequest;
- creates no ImageReader;
- reads no camera image buffer;
- writes no vendor request key;
- invokes no HONOR Binder service;
- creates no extension session;
- creates no sensor evidence;
- changes no Scientific Master or Dynamic Authority.

Package hashes and Camera2 characteristics are software/capability observations only.

They cannot prove:

- successful capture;
- hidden OEM route semantics;
- RAW14 delivery;
- native ADC topology;
- Direct-CFA 200MP;
- photodiode count;
- calibration authority.

## Comparison rule

The baseline must be compared against v0.47/v0.48/v0.53/v0.54 **field by field**.

A changed HONOR Camera version or Android fingerprint is not itself evidence that the RAW route changed.

An active replay is justified only after the passive delta is known.

## Implementation status\n\nThe v0.55 baseline Activity, manifest entry, launcher entry, version bump and CI guard are implemented on this branch. CI must pass before the APK is treated as the runnable baseline.\n\n## Next gate

After the user exports `TRUTHRAW_ANDROID17_HONOR_CAMERA_BASELINE_v055.json`:

1. diff Android build/package identity against the prior Android-17 state;
2. diff Camera-5 standard/max RAW maps and key surfaces;
3. decide the smallest controlled active follow-up;
4. do not return to broad vendor-key sweeps unless the new baseline exposes a concrete changed surface.

