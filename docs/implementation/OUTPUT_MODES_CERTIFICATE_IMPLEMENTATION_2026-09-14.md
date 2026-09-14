# TruthRaw Output Modes + Certificate Implementation — 2026-09-14

Status: ACTIVE IMPLEMENTATION CONTRACT

## User-facing modes

TruthRaw exposes four primary output choices:

1. `JPG` — universal presentation output.
2. `JPG XL` — high-quality presentation output. The mode is reserved in UI; it must not be called production-ready until an encoder passes host and Android validation.
3. `TRUTHRAW PURE` — scientific output. No creative appearance toggles are permitted in this mode.
4. `TRUTHRAW ADVANCED` — the same scientific foundation with downstream appearance/export controls.

`Colourful`, `Detailed`, `Soft`, and `HDR` are selectable only for JPG, JPG XL, and TRUTHRAW ADVANCED. They are appearance controls and may never alter sealed source evidence, measurement authority, the Scientific Master identity, TruthRange zero-line, evidence counts, or the Technical Backplane.

## Language contract

English is the canonical fallback language. Initial Android resource localizations are English, Dutch, German, and French. Product-mode names and option names remain stable identifiers across locales.

## TRUTHRAW PURE RAW/DNG path

The high-fidelity scientific RAW export is the float32 Scientific Master Linear DNG projection:

`sealed source -> source-bound color -> Scientific Master v0.2 -> Technical Backplane phase 2 -> canonical 64x64 master replay -> exact master digest gate -> cameraToXyzD50 -> float32 XYZ-D50 LinearRaw DNG`

The writer retains negative and greater-than-one float components. The older uint16 projection writers remain compatibility outputs and are not allowed to redefine TRUTHRAW PURE.

## TruthRaw Certificate

The certificate belongs inside the file/technical backside, never as a visible watermark.

Certificate v0.1 must bind at least:

- certificate schema/version;
- producer `TruthRaw`;
- output/projection class;
- sealed source SHA-256;
- Scientific Master SHA-256;
- zero-line identity;
- scene-scale identity;
- Technical Backplane identity/integrity value;
- color-binding identity and authority scope;
- `physicalFrameCount=1`;
- `independentEvidenceCount=1`;
- build/pipeline identity when available;
- signature state and signature algorithm.

A certificate may describe reconstructed/projected output, but it may never relabel it as measured Direct-CFA evidence.

### Signature rule

No private brand-signing key may be embedded in the APK or repository. Until a trusted TruthRaw issuer key is provisioned, metadata must explicitly report an unsigned/development state and the UI must not show `VERIFIED`.

The long-term signature covers a canonical certificate payload so changing any covered lineage or authority field invalidates verification. Brand/legal protection and cryptographic provenance are related but distinct: the certificate provides technical authenticity/integrity; trademark rights are not created by EXIF/XMP/DNG metadata alone.

## Current implementation boundary

Implemented on this branch:

- restored validated float32 Scientific Master DNG writer and streaming adapter;
- Android export kind for TRUTHRAW PURE float32 DNG;
- exact Scientific Master digest gate and source re-verification around export;
- four-mode launcher UI;
- appearance-option visibility contract;
- EN/NL/DE/FR resources;
- adaptive launcher icon resource structure;
- host validation workflow for the float32 writer.

Still blocked from production claim until separately validated:

- JPEG XL encoder;
- final profile rendering semantics for Colourful/Detailed/Soft/HDR;
- cryptographic issuer signing key and verifier trust distribution;
- real-device Honor/MotionCam end-to-end float32 DNG output;
- Adobe/Lightroom ingestion.
