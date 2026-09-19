# TruthRaw v0.49 — installed Honor Camera package fingerprint + export

Date: 2026-09-19

## Trigger

The v0.48 targetSdk-37 control exactly replicated the v0.47 camera capability result.

Deep comparison of the two source JSON files found only:

- a different `createdAtUtc`;
- `device.truthRawTargetSdk`: 35 -> 37.

All Camera2 RAW maps, RAW14 results, extension results and installed Honor Camera metadata were otherwise identical.

Therefore the specific hypothesis "targetSdk 37 unlocks an advertised RAW14 route" is closed for the current Android-17 firmware.

At the same time, the OS update changed the system Honor Camera package from `171.0.10.452` to `171.0.10.706`.

## Goal

Acquire the exact installed Android-17 Honor Camera software artifact for static comparison with the preserved Android-16 Honor Camera APK.

v0.49 reads PackageManager metadata for `com.hihonor.camera`, fingerprints the base APK and all reported split APKs with SHA-256, and—only when every package file is readable—allows the user to save a ZIP containing:

- `base.apk`;
- every split APK;
- `truthraw_package_manifest.json` containing the measured hashes and package metadata.

## Authority

`SOFTWARE_PACKAGE_ARTIFACT_ONLY`

This route is intentionally outside TruthRaw scientific capture authority.

It does not:

- open a camera;
- submit a capture;
- read an image buffer;
- write a vendor request key;
- invoke Honor Binder;
- create calibration authority;
- alter the Scientific Master.

Static APK differences can identify code paths and route hypotheses, but APK content can never make a Direct-CFA, sensor, capture or calibration claim true.

## Comparison target

Android-16 preserved Honor Camera reference:

- version: `171.0.10.452`
- SHA-256: `3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52`

Android-17 currently installed package, from v0.47/v0.48 PackageManager observation:

- version: `171.0.10.706`
- targetSdk: 37
- system app: true

The package export will allow a direct .452 -> .706 static diff, especially around RAW14, UltraHighPixelMode, ServiceHost pipelines, vendor extension IDs and physical-camera routing.
