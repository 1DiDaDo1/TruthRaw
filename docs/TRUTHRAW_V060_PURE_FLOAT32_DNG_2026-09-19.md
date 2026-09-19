# TruthRaw v0.60 — TRUTHRAW PURE Float32 Scientific DNG restored

Date: 2026-09-19

Status: **IMPLEMENTED / HOST + ANDROID CI GREEN / REAL-DEVICE v0.60 ARTIFACT INSPECTION NEXT**

Active branch:

`integration/truthraw-suite-v0-60-pure-float32-dng`

App version:

`0.25-v0.60-pure-float32-dng`

## Why v0.60 exists

The current multi-vendor Android app had inherited the newer `linear-dng-projection-v0.1` compatibility writer. That writer is deliberately:

- 3-channel camera-native RGB;
- unsigned 16-bit;
- bounded to `[0,65535]`;
- values below 0 clipped;
- values above 1 clipped;
- compatibility output only.

That is not the historical TRUTHRAW PURE contract.

The user supplied a known-good historical PURE file report showing the intended class:

- DNG / Digital Negative;
- `4080×3072`;
- `32 bits`;
- software `TruthRaw scientific-master-linear-dng-projection-v0.1`;
- DNG 1.4;
- lossless representation.

The recovery audit had already restored the historical host writer into the current project tree. v0.60 reconnects that exact scientific writer to the current multi-vendor Android app.

## Current PURE route

`sealed source DNG`
→ source-bound DNG color binding
→ generic multi-vendor DNG adapter
→ exact Scientific Master streaming binding
→ TruthRange / zero-line / scene binding
→ Technical Backplane Phase 2
→ canonical Scientific Master replay
→ exact Scientific Master digest gate
→ `cameraToXyzD50`
→ **IEEE Float32 XYZ-D50 LinearRaw DNG**.

## Serialized DNG contract

The restored writer itself serializes:

- `PhotometricInterpretation = 34892 (LinearRaw)`;
- `SamplesPerPixel = 3`;
- `BitsPerSample = 32,32,32`;
- `SampleFormat = 3,3,3` = IEEE floating point;
- `Compression = 1` = uncompressed TIFF storage;
- `DNGVersion = 1.4.0.0`;
- `DNGBackwardVersion = 1.4.0.0`;
- `Software = TruthRaw scientific-master-linear-dng-projection-v0.1`;
- canonical `64×64` tiling;
- identity `ColorMatrix1` in the synthetic XYZ-D50 LinearRaw storage space;
- D50 `AsShotNeutral`;
- D50 calibration illuminant.

Media tools may describe uncompressed TIFF data as “Lossless”. The precise storage fact is `Compression=1`: no lossy compression is used.

## Range behavior

TRUTHRAW PURE does not force the scientific representation into `[0,1]`.

The writer preserves:

- finite negative XYZ-D50 components;
- finite components greater than 1.

These are counted and returned to the Android UI.

This is a major distinction from the 16-bit compatibility writer.

## Exact authority gate

The v0.60 Android bridge requires:

- source bytes SHA-256 sealed before processing;
- source seal reverified before projection;
- current generic DNG source adapter;
- Scientific Master hash from the current streaming binding;
- Technical Backplane Phase-2 bound to the exact same source/master;
- canonical Scientific Master replay during write;
- replayed master digest equals admitted master digest;
- source seal reverified after projection;
- `physicalFrameCount=1`;
- `independentEvidenceCount=1`;
- no appearance application;
- no counterfactual observation;
- no Scientific Master mutation.

If the exact Master digest does not match, the transactional output is not committed.

## Product separation

The current app now exposes both:

**TRUTHRAW PURE · 32-bit Float DNG**

and

**16-bit Linear DNG (compatibility)**.

They are not synonyms.

TRUTHRAW PURE is the high-fidelity scientific projection.

The uint16 route remains useful for compatibility but may not redefine PURE.

## Multi-vendor boundary

The current v0.60 PURE Android route is enabled only for the already fully admitted DNG path.

Nikon NEF remains at the separate v0.59 measurement/radiometric-admission stage and is **not** silently allowed into PURE until its noise/uncertainty, source-bound color and held-out scientific admission gates are closed.

## Validation

CI run:

`35458367764`

Results:

- host PURE writer GCC: **SUCCESS**;
- host PURE writer Clang: **SUCCESS**;
- Android arm64 app: **SUCCESS**;
- APK verification: **SUCCESS**;
- artifact upload: **SUCCESS**.

Android artifact:

- name: `truthraw-suite-v0-60-pure-float32-dng-debug-arm64`;
- artifact ID: `10589820246`;
- archive SHA-256: `fbf4973bc49714dadf821e4421141bd11a8e60022cd495e5ec3821b39547e443`.

## Next physical gate

The next phone test should use a known-good DNG and save:

`*_truthraw_pure_float32_v0_1.dng`.

Then independently inspect the saved artifact and require at least:

- image dimensions equal source dimensions;
- 32-bit samples;
- IEEE floating-point SampleFormat;
- 3-channel LinearRaw;
- DNG 1.4;
- Software string `TruthRaw scientific-master-linear-dng-projection-v0.1`;
- no lossy compression;
- negative and >1 component counts retained;
- exact source/master lineage still bound.

The expected media-info class should therefore resemble the historical 32-bit report supplied by the user, not the previous 16-bit compatibility output.

## Nonclaims

v0.60 does not yet claim that a newly produced phone artifact has passed this real-device inspection.

The implementation and CI are green; the next evidence must come from the actual saved Android-produced DNG.
