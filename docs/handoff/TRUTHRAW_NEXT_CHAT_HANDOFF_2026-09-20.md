# TruthRaw next-chat handoff — 2026-09-20

**READ THIS FIRST.**

Active branch:

`integration/truthraw-suite-v0-78-open-scene-channel-authority`

Current app version:

`0.44-v0.78-open-scene-channel-authority`

Latest fully green CI:

`35519235748`

Latest APK:
- bytes: `6,407,245`
- SHA-256: `7a02fc07472158003423a1232a2c320956c3f80b7ff02d94c303fa46a1e181ff`
- GitHub artifact id: `10608121004`
- artifact ZIP SHA-256: `f7125be3edbbaf4017fc1fee6aa57e7d384c16e1d9ab4a288ab1400eebc17ae1`

This branch is the current integration/research line. It is not a main/canonical promotion.

## Permanent law

> Measured where measured. Reconstructed where necessary. Never invented.

> Representation can exceed the source. Knowledge claims cannot exceed the evidence.

Original source evidence remains immutable. Every later building block must know its ancestry, authority, uncertainty/support and intervention history.

## Current product line

The active product now includes:
- multi-vendor RAW/DNG import
- real Camera-5 capture entry
- source-first Camera-5 sealing and topology admission
- Main-House DNG admission
- finalized Scientific Preview
- PURE 32-bit Float DNG v0.63
- TN-3 downstream Scientific Negative path
- Advanced appearance controls
- full-resolution Restoration + DNG/TIFF/EXR projection
- responsive portrait/landscape camera UI
- bit-exact performance optimisations v0.74/v0.75
- canonical derivative ancestry v0.77
- per-channel Open Scene authority sidecar v0.78

## v0.73 real-device closure

The real Camera-5 route has been demonstrated on device:

`launcher -> Gebruik camera -> Camera-5 capture -> admitted 4080x3072 DNG -> Main House -> finalized Scientific Preview -> PURE Float32 DNG`.

The known camera-derived DNG is a processing container admitted from the source-first Camera2 RAW envelope.

Do not promote the 16320x12288 envelope itself to 200 MP Scientific Master authority.

## v0.74 / v0.75 performance

v0.74 replaced thousands of small Scientific-Master source calls with bit-exact bounded stripes.

v0.75 proved 128x128 -> 512x512 finalized-preview runtime tiles bit-exact.

Host equivalence gates preserve:
- Scientific Master SHA
- exact L0 bits
- Backplane
- preview pixels
- exposure/HDR state
- frame/evidence 1/1

Theoretical logical tile requests on 4080x3072 dropped from 7680 toward about 336.

A same-device physical latency benchmark against the historical ~87.96 s baseline is still desired.

## v0.76 camera UI

Camera UI is responsive to portrait/landscape orientation and has a clear shutter.

The source/capture science was not changed.

## v0.77 canonical ancestry

New schema:

`TruthRawCanonicalAncestry/0.77`

It binds:

`source -> Scientific Master -> Zero-Line -> scene-scale -> 180-byte Backplane -> Open Scene -> derivative raster -> role-mask`.

Restoration DNG, TIFF and EXR now receive the same format-neutral ancestry manifest.

PURE v0.63 remains frozen and unchanged.

## v0.78 per-channel authority

New child schema:

`TruthRawOpenSceneChannelAuthority/0.78`

It is bound to canonical Open Scene v0.70.

Authority and uncertainty are separate axes.

For current generic admitted DNG:
- direct CFA uncensored = CALIBRATED_ESTIMATE
- direct CFA clipped = CENSORED + explicit source-code bound
- missing RGB channels = UNKNOWN
- RECONSTRUCTED = 0

That last point is intentional. No reconstruction gets scientific authority without an admitted source/backend-bound uncertainty model.

Advanced returns both:
- v0.70 Open Scene artifact SHA
- v0.78 channel-authority artifact SHA

## Frozen / do not silently change

- source evidence immutable
- physicalFrameCount=1
- independentEvidenceCount=1
- PURE self-binding v0.63
- Zero-Line/L0
- scene-scale
- Technical Backplane
- Restoration algorithm v0.67
- canonical Open Scene v0.70 parent semantics
- role-mask projection v0.71
- projection lifecycle v0.72
- Camera-5 topology admission law
- appearance cannot write back into Scientific Master
- counterfactual state never becomes evidence

## v5.0g boundary

Exact historical v5.0g feature replay is green.

However the historical tele uncertainty domain must not automatically be transferred to the current camera-derived DNG.

Before v0.78 can emit RECONSTRUCTED authority, uncertainty must be:
- source-bound
- backend-bound
- sample-domain compatible
- F64 trace/replay compatible where required
- independently validated / held-out where applicable

## Immediate next step

Start:

`integration/truthraw-suite-v0-79-bound-uncertainty-admission`

First objective: build an exact uncertainty-admission interface feeding v0.78 without changing reconstruction pixels.

Fail closed on current camera-derived DNG until a matching camera-domain uncertainty/PTC binding exists.

Then continue:
1. v4.7j support-aware detail integration;
2. v4.7k downstream acutance;
3. local HDR/light/dark/artificial-illumination state;
4. authority-aware single-frame HDR;
5. Restoration v2;
6. independent DNG/TIFF/EXR conformance + source replay;
7. proprietary RAW promotion gates such as Nikon NEF.

## Read next

1. `state/CURRENT_PROJECT_STATE_2026-09-20.json`
2. `docs/TRUTHRAW_V078_OPEN_SCENE_CHANNEL_AUTHORITY_2026-09-20.md`
3. `docs/research/open-scene-channel-authority-v0.78/README.md`
4. `docs/research/canonical-ancestry-spine-v0.77/README.md`
5. `docs/TRUTHRAW_V073_CAMERA_SOURCE_ADMISSION_TN3_2026-09-20.md`
6. `docs/TRUTHRAW_FULL_GENEALOGY_RECOVERY_AUDIT_2026-09-19.md`
