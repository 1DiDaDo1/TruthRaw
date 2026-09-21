# TruthRaw final code freeze handoff — 2026-09-21 — v0.84.3

**READ THIS FIRST IN THE NEXT CHAT.**

## Code freeze

The user explicitly requested:

> Geen code meer veranderen.

Therefore the current code-bearing state is frozen at:

- branch: `integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master`
- code-bearing commit: `d44e0096e85d75703e8de54d5a832b727c450f66`
- workflow: `TruthRaw v0.84.2 Adaptive Compute Router`
- workflow run: `35625892299`
- CI result: **SUCCESS**
- artifact id: `10652446563`
- artifact name: `truthraw-v0-84-2-compute-router-debug-arm64`
- artifact ZIP digest: `sha256:334a3f550b9f42922812ba807e2d75cf4ab11fee96f72cf7f834bdd6853a7548`
- APK bytes: `6,095,333`
- APK SHA-256: `ee9bdf25f85c7790301f7419146061b1366974073172f7e5a0e251fc3348a676`

Documentation commits after the code-bearing commit do **not** change the APK binary.

Do not modify code in a successor chat unless the user explicitly reverses this freeze.

## Green build gates at the frozen code head

CI recorded:

- ordered parallel executor: **PASS**
- sealed tile-native / full-frame streaming integration: **PASS**
- canonical v4.7i O0 signature: `5dba058e5742346a`
- canonical v4.7i O2 signature: `5dba058e5742346a`
- TruthNegative dense projection v0.3: **PASS**
- ADVANCED Render/Edit partition invariance: **PASS**
- negative extended-linear headroom preservation: **PASS**
- Android ARM64 debug APK: **BUILD SUCCESSFUL**

## Scientific laws remain unchanged

> Measured where measured. Reconstructed where necessary. Never invented.

> Representation can exceed the source. Knowledge claims cannot exceed the evidence.

Permanent constraints:

- sealed Source Evidence is immutable;
- single physical frame remains `physicalFrameCount=1`;
- single admitted observation remains `independentEvidenceCount=1`;
- Scientific Master is separate from appearance/export;
- negative and >1 Float32 values are valid representation values;
- appearance never upgrades measurement authority;
- compute acceleration never upgrades scientific authority;
- APK/GCam/computational-RAW material never determines TruthRaw source truth/calibration.

## Float32 Full Colour Scientific Master

The former JPG-L product line was repurposed.

The current intended line is:

`sealed CFA -> camera-native full-colour Scientific Master RGB -> IEEE Float32 LinearRaw DNG`

For the Full Colour Scientific Master flavor:

- the DNG primary raster stores camera-native Scientific Master RGB directly;
- `BitsPerSample = 32,32,32`;
- `SampleFormat = IEEE floating point`;
- `PhotometricInterpretation = LinearRaw`;
- negative components remain legal;
- values above 1 remain legal;
- JPEG is secondary/non-authority preview only;
- no ADVANCED appearance is baked into this master flavor;
- source Scientific Master remains unchanged;
- DNG camera-profile metadata is derived only from the already-authorized camera-to-XYZ-D50 binding;
- the export creates no new evidence.

This replaces the old conceptual role of JPG-L. The historical `JpgLExport.kt` / JPEG-front-plus-tail concept is legacy provenance, not the preferred Lightroom editing model.

## ADVANCED appearance controls present in the frozen APK

ADVANCED now includes downstream presentation/edit controls for:

- Exposure;
- Shadows;
- Detail / sharpness;
- Colour fullness;
- Natural Light;
- Natural HDR / authority-aware presentation;
- Restoration.

These remain downstream of the scientific source.

Dark-image handling was improved with bounded presentation-only exposure/shadow logic. This does not rewrite sensor black level or Zero-Line.

## Camera calibration telemetry

FotoGraaf camera paths can record observation-only Camera2 metadata such as:

- dynamic black level;
- dynamic white level;
- noise profile;
- lens shading map;
- lens intrinsics / distortion when available;
- rolling-shutter timing;
- colour-correction observations.

These observations do **not** grant calibration authority and do not write back into the Scientific Master.

## Camera-5 / historical 200MP evidence boundary

Historical facts remain:

- physical Camera-5 delivered an app-visible `16320x12288` RAW_SENSOR envelope;
- envelope size: `401,080,320` bytes;
- later v0.19/v0.20 audit proved only the first `25,067,520` bytes were populated;
- that populated prefix exactly matches `4080x3072x2` bytes;
- therefore the evidence does **not** prove 200MP measured CFA detail.

Correct interpretation:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

The 16320x12288 geometry may be used as a reconstructed target world, but it may never be relabelled as 200MP measured sensor evidence.

## TruthNegative dense 200MP full-colour route

The frozen code contains the first executable dense TruthNegative pixel route.

Intended route:

`4080x3072 admitted CFA`
`-> camera-native Scientific Master`
`-> TruthNegative 4x dense reconstruction`
`-> 16320x12288 camera-native Float32 LinearRaw DNG`

Key contract:

- scale = exactly 4x per axis;
- target geometry = `16320x12288`;
- target support role = `RECONSTRUCTED_DENSE_SUPPORT`;
- measured target claim count = 0;
- no new evidence is created;
- the original Scientific Master remains the parent;
- target output-channel authority fails closed to UNKNOWN/resampled authority;
- target projected raster has its own SHA-256 identity;
- negative and >1 values remain representable;
- JPEG remains preview-only.

The UI label in the frozen line is:

`TruthNegative 200MP · Float32 Full Colour · DNG`

This must never be described as 200MP measured CFA.

## CPU and Vulkan

The CPU dense projection is the canonical reference.

TruthNegative dense projection v0.3 uses a deterministic 4x pixel-centre bilinear reconstruction with explicit Float32 operation order and no sharpening/texture synthesis/AI reconstruction.

A Vulkan compute backend is present for this kernel with the following rule:

- Vulkan is acceleration only;
- software/CPU Vulkan devices are rejected for performance selection;
- Vulkan may become exact-scientific eligible only after its kernel-specific runtime self-test matches the CPU reference bit-for-bit;
- if unavailable, ineligible or failing, the route falls back to CPU;
- backend choice does not change authority;
- execution backend is provenance only;
- accelerator memory is included in bounded-memory accounting.

Do not treat Vulkan capability itself as proof of a valid scientific backend.

## Lightroom interpretation

The Float32 DNG primary is a real LinearRaw primary raster, not a JPEG front with hidden science data.

The embedded JPEG is preview-only.

Still open for real-device validation:

- Lightroom Android import of Full Colour Scientific Master;
- proof that Lightroom edits the Float32 primary;
- exposure/highlight/shadow behavior over negative and >1 values;
- HDR-mode interpretation of extended-linear headroom;
- camera-native profile/white-balance behavior;
- DNG 1.7 / ProfileDynamicRange research for explicit HDR signalling.

Natural HDR must not invent headroom. It is a presentation of headroom that already exists and is authorized.

## Immediate next action

**Do not code.**

First action in the next chat should be real-device testing of the frozen APK and inspection of produced DNG files.

If a bug is found, document it first. Only modify code after the user explicitly lifts the freeze.

## Read order

1. this file;
2. `state/CURRENT_PROJECT_STATE_2026-09-21.json`;
3. `START_HERE_NEW_CHAT.md`;
4. `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-21.md`;
5. Camera-5 v0.14/v0.19/v0.20 evidence documents;
6. TruthNegative v0.2 historical contract;
7. TruthNegative dense projection v0.3 implementation/docs;
8. current Float32 Scientific Master DNG writer;
9. current output-channel authority v0.84 code.

