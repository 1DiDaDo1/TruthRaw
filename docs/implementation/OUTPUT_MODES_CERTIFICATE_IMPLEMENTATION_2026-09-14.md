# TruthRaw Output Modes + Certificate Implementation — 2026-09-14

Status: ACTIVE IMPLEMENTATION CONTRACT

For session continuity, also read:

`docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-14.md`

## User-facing modes

TruthRaw exposes four primary output choices:

1. `JPG` — universal presentation output.
2. `JPG XL` — high-quality presentation output. The mode is reserved in UI; it must not be called production-ready until an encoder passes host and Android validation.
3. `TRUTHRAW PURE` — scientific output. No creative appearance toggles are permitted in this mode.
4. `TRUTHRAW ADVANCED` — the same scientific foundation with downstream appearance/export controls.

`Colourful`, `Detailed`, `Soft`, and `HDR` are selectable only for JPG, JPG XL, and TRUTHRAW ADVANCED. They are appearance controls and may never alter sealed source evidence, measurement authority, the Scientific Master identity, TruthRange zero-line, evidence counts, or the Technical Backplane.

The launcher-to-processing handoff is now policy-gated. Missing or unknown mode data fails safe to `TRUTHRAW PURE`; unsupported appearance toggles sanitize to neutral. JPG export and RAW/DNG projectors are re-checked against the same policy at action time, and RAW/DNG export is checked again after the Android document dialog before native processing begins.

The current renderer does **not** yet apply the selected `Colourful`, `Detailed`, `Soft`, or `HDR` intent to scientific processing. The processing UI reports such selections as downstream appearance intent only until separately validated rendering semantics exist.

## Language contract

English is the canonical fallback language. Initial Android resource localizations are English, Dutch, German, and French. Product-mode names and option names remain stable identifiers across locales.

## TRUTHRAW PURE RAW/DNG path

The high-fidelity scientific RAW export is the float32 Scientific Master Linear DNG projection:

`sealed source -> source-bound color -> Scientific Master v0.2 -> Technical Backplane phase 2 -> canonical 64x64 master replay -> exact master digest gate -> cameraToXyzD50 -> float32 XYZ-D50 LinearRaw DNG`

The writer retains negative and greater-than-one float components. The older uint16 projection writers remain compatibility outputs and are not allowed to redefine TRUTHRAW PURE.

The current output policy exposes only `TRUTHRAW_PURE_FLOAT32_DNG` in TRUTHRAW PURE. Compatibility projectors remain available only through TRUTHRAW ADVANCED.

## TruthRaw Certificate

The certificate belongs inside the file/technical backside, never as a visible watermark.

Certificate v0.1 binds at least:

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

Certificate v0.1 and DNG embedding are implemented on this branch. Until a trusted issuer key exists, the embedded record and Android UI explicitly report `UNSIGNED DEVELOPMENT`; the verified badge gate must remain false.

### Signature rule

No private brand-signing key may be embedded in the APK or repository. Until a trusted TruthRaw issuer key is provisioned, metadata must explicitly report an unsigned/development state and the UI must not show `VERIFIED`.

The long-term signature covers a canonical certificate payload so changing any covered lineage or authority field invalidates verification. Brand/legal protection and cryptographic provenance are related but distinct: the certificate provides technical authenticity/integrity; trademark rights are not created by EXIF/XMP/DNG metadata alone.

## Physical regression inputs

The active branch now records the five uploaded Honor/MotionCam DNGs as a formal physical regression set in:

`docs/research/physical-dng-test-set-v0.1/TRUTHRAW_PHYSICAL_TESTSET_2026-09-14.json`

They are test evidence, not automatically calibration evidence. The manifest records source SHA-256, ISO/exposure, black levels, AsShotNeutral, NoiseProfile, clipping/below-black statistics and the shared GainMap OpcodeList2 identity.

The five filenames are:

- `IMG_260816_134122_304_005.dng`
- `IMG_260816_134204_911_008.dng`
- `IMG_260816_143716_085_041.dng`
- `IMG_260830_143012_297_014.dng`
- `IMG_260908_193845_662_027.dng`

## Validated implementation snapshot

Active implementation branch:

`research/ui-output-modes-certificate-v0.1-2026-09-14`

Draft PR:

`#23 — Integrate four-mode output policy + TRUTHRAW PURE float32 DNG`

The implementation head `ef5175707d2b69973601408d6053fd234b2c6906` was re-checked before the session handoff and all observed PR-triggered workflows for that head were successful, including certificate, DNG certificate embed, product UI contract, output-mode policy, float32 Scientific Master DNG, Android adaptive UI/preview/finalized-color builds, Honor empirical harness, canonical/documentation governance and TruthRange/uncertainty integrity workflows.

Subsequent handoff/documentation commits move the branch head without changing the validated implementation code. A future chat must therefore re-fetch the live head/workflow state before repeating an all-green claim.

## Current implementation boundary

Implemented on this branch:

- restored validated float32 Scientific Master DNG writer and streaming adapter;
- Android export kind for TRUTHRAW PURE float32 DNG;
- exact Scientific Master digest gate and source re-verification around export;
- four-mode launcher UI;
- shared downstream `OutputModePolicy` with fail-safe TRUTHRAW PURE default;
- processing-UI enforcement for JPG versus PURE/ADVANCED RAW/DNG export access;
- PURE appearance sanitization to neutral and JPG XL fail-closed state;
- explicit appearance-intent-only reporting until renderer semantics are validated;
- EN/NL/DE/FR resources;
- adaptive launcher icon resource structure;
- TruthRaw Certificate v0.1 canonical record;
- DNG certificate embedding with explicit unsigned/development state;
- host validation workflows for float32 DNG, certificate, certificate embedding, output-mode policy and product UI contract;
- five-file physical regression manifest for the supplied Honor/MotionCam test photos.

Still blocked from production claim until separately validated:

- JPEG XL encoder;
- final profile rendering semantics for Colourful/Detailed/Soft/HDR;
- cryptographic issuer signing key and verifier trust distribution;
- real-device Honor/MotionCam end-to-end float32 DNG output over the five-file physical set;
- extraction/integrity verification of the embedded certificate from the actual Android-produced DNG;
- Adobe/Lightroom ingestion and behavior.

## Next continuation gate

Do not redesign the scientific authority model in the next chat. Continue by validating the existing green implementation on real device/data first: five-source PURE float32 export, certificate extraction, exact source/master/backplane/evidence invariants, then Adobe/Lightroom interoperability. Only after that should the downstream profile renderers, JPEG XL encoder and trusted signing path be promoted.
