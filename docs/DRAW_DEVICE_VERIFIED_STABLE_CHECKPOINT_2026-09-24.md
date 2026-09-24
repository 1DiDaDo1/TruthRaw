# D.RAW device-verified stable checkpoint — 2026-09-24

Status: DEVICE-VERIFIED WORKING BASELINE

## Exact source state

Checkpoint branch:

`checkpoint/draw-stable-signed-device-verified-2026-09-24`

Source commit:

`3daa200af8af634631b23c206b7456c4e9623870`

This is the exact source state that produced the APK confirmed by the user to launch successfully on-device.

## Verified GitHub Actions build

Workflow run:

`36063051154`

Run attempt:

`2`

Result:

`success`

Artifact ID:

`10835716922`

Artifact name:

`truthraw-v0-84-2-compute-router-debug-arm64`

## Verified APK

APK bytes:

`6249283`

APK SHA-256:

`ab73c5ef0d7fdfdeafb42ede3fe06dbf9de2df117a725532c776aa51d6d2380b`

Android signing certificate SHA-256:

`a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`

Architecture:

`arm64-v8a`

Package:

`com.truthraw.adaptiveui`

## What this checkpoint proves

- The D.RAW launcher starts successfully on the target device.
- The D.RAW launcher icon is the accepted current icon.
- The startup hardening introduced before this checkpoint is compatible with the target device.
- The stable D.RAW development signing identity is correctly materialized in CI.
- The final APK is verified against the pinned certificate before artifact publication.
- Future development builds using the same signing identity can be installed as updates over this baseline.

## Scientific boundary

This checkpoint changes Android packaging, startup robustness, UI/branding and development-signing continuity only.

It grants no new scientific authority and changes no Direct-CFA evidence, Scientific Master evidence, TruthNegative evidence class, calibration authority, reconstruction provenance or measured/reconstructed/appearance separation.

## Regression rule

If a future APK fails to launch, install as an update, or unexpectedly changes device-level app identity behavior, compare it first against this exact checkpoint before changing scientific or reconstruction code.
