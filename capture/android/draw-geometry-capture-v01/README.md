# D.RAW Geometry Capture APK v0.1

Status: **CAPTURE-ASSISTANT CANDIDATE**

This standalone Android APK guides the real MAIN + ULTRA_WIDE geometry capture campaign.

It deliberately does not make Camera2 a scientific ingress requirement.

## Flow

For each of 16 poses:

1. keep phone and target fixed;
2. open the normal RAW-capable camera;
3. capture MAIN / 1× RAW or DNG;
4. without moving phone or target, capture ULTRA_WIDE / 0.6× RAW or DNG;
5. return to the APK and import both original files;
6. the APK copies each file byte-for-byte into its session folder and verifies SHA-256;
7. confirm the two operator attestations;
8. mark the pair complete.

Poses 1–12 are TRAINING. Poses 13–16 are HOLDOUT.

## Authority boundary

The APK is workflow/provenance assistance only. It does not create sensor evidence, alter source bytes, admit D.RAWnegative, create a geometry relation, grant a coordinate transform, fusion, or calibration transfer.

The original RAW/DNG remains the evidence source.

## Camera2

Camera2 is not required. The APK opens the user's normal camera application and then uses Android's document picker to import the resulting original RAW/DNG.

## Session manifest

The APK exports `D.RAW/GeometryCaptureSession/0.1` containing the 16 pose slots, source byte lengths, source SHA-256, verified copy SHA-256, operator attestations, optional target geometry identity, and fail-closed relation authority.

The manifest is capture provenance, not a geometry certificate.

## Open-world law

**Seal the evidence, not the thinking.**
