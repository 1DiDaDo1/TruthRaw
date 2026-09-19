# TruthRaw v0.49 — Honor Camera Android-17 package acquisition result

Date: 2026-09-19

## Exact artifact recovered

The installed Android-17 Honor Camera system package was successfully read and exported.

Runtime package:

- package: `com.hihonor.camera`
- version: `171.0.10.706`
- targetSdk: 37
- minSdk: 36
- compileSdk: 36
- system app: true
- updated-system-app: false
- split APK count: 0
- device source path: `/product_h/region_comm/oversea/app/HnCamera2/HnCamera2.apk`

Base APK:

- bytes: `86,018,325`
- SHA-256: `bbc6312e0289d01a51dbe45efc519e56715e6250521227df81cf64a633b8f027`

The exported ZIP contains that exact base APK plus the TruthRaw package manifest. The base APK SHA-256 calculated again after ZIP extraction exactly matches the device-side fingerprint.

Bundle:

- `TRUTHRAW_HONOR_CAMERA_PACKAGE_171.0.10.706_v049.zip`
- bytes: `59,236,173`
- SHA-256: `556a10407eb3f0b4d20b9fd72be2dac2c6417ecacfc2e69fccc5a33509305c97`

Fingerprint JSON:

- bytes: `1,903`
- SHA-256: `f2b8092b2bb959de48d748bfa6d259f7a14b6d2f29e01afc94edd96a4acee981`

## Exact Android-16 comparison artifact already preserved

Honor Camera `171.0.10.452`:

- bytes: `84,167,938`
- SHA-256: `3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52`

This creates a direct byte-grounded .452 -> .706 software comparison.

## Authority boundary

This is `SOFTWARE_PACKAGE_ARTIFACT_ONLY`.

APK content can identify software routes, constants, vendor metadata names and hypotheses. It cannot establish sensor sampling, Direct-CFA, native ADC topology, capture provenance or calibration truth.
