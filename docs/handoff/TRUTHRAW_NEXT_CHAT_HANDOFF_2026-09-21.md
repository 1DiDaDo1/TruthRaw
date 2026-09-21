# FINAL CODE FREEZE NOTICE — 2026-09-21

**Read first:** `docs/handoff/TRUTHRAW_FINAL_CODE_FREEZE_V0843_2026-09-21.md`

Frozen code-bearing branch/head:

`integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master`

`d44e0096e85d75703e8de54d5a832b727c450f66`

Frozen APK SHA-256:

`ee9bdf25f85c7790301f7419146061b1366974073172f7e5a0e251fc3348a676`

The user explicitly requested **no more code changes**. Documentation commits after the frozen head do not change the APK binary. Real-device validation is the next action. Do not modify code unless the user explicitly lifts the freeze.

---

# TruthRaw next-chat handoff — 2026-09-21

**READ THIS FIRST.**

Active integration branch:

`integration/truthraw-suite-v0-84-2-adaptive-compute-router`

Current Android integration version:

`0.51-v0.84.2-adaptive-compute-router`

Latest fully green **code-bearing** CI:

- workflow: `TruthRaw v0.84.2 Adaptive Compute Router`
- run: `35547319268`
- code head: `5a7acf28f90cac82297c2d5af3e81006c438da2f`
- status: **SUCCESS**
- artifact id: `10617071043`
- artifact: `truthraw-v0-84-2-compute-router-debug-arm64`
- artifact ZIP SHA-256: `25e1dd0568f1ed91acb93f2bcee01cf39520184d5e824d0f2ae5317332cdae96`
- APK bytes: `5,994,861`
- APK SHA-256: `9b4184afdec22d6313ba08d7abe8884e812e3b3cadc7f29d492685af4d820295`

Documentation commits may make the branch HEAD newer than the code-bearing head above.
Do not interpret that as an unverified code change.

Machine-readable state:

`state/CURRENT_PROJECT_STATE_2026-09-21.json`

Previous dated state/handoff remain historical snapshots and must not be rewritten.

## Permanent law

> Measured where measured. Reconstructed where necessary. Never invented.

> Representation can exceed the source. Knowledge claims cannot exceed the evidence.

Additional compute law:

> Faster execution never creates stronger evidence or authority.

Source Evidence remains immutable. Scientific Master, Dynamic Authority/Open Scene,
appearance, restoration, edit derivatives and exports remain separate domains.

## Current high-level product

TruthRaw now presents three production routes:

`sealed source -> Scientific Master -> PURE / ADVANCED / PRO -> result -> export`

Inputs remain:

- existing RAW/DNG;
- Camera, after acquisition/admission, feeding the same RAW route.

Every successor page has a visible back action; Android Back is intended to match
one semantic level back.

### Non-destructive orientation

Accidentally upside-down/sideways captures can use a manual quarter-turn override.

The override:
- never rewrites sealed source bytes;
- never rotates the canonical Scientific Master itself;
- is applied to preview/output projection;
- does not change scientific authority;
- composes with source orientation.

## Current Float32 / Lightroom architecture

### PURE

PURE is a **32-bit IEEE Float Scientific Linear DNG**, not an 8-bit JPEG wrapper.

Primary image:
- XYZ-D50 linear Float32 projection;
- negative values retained;
- values above 1 retained;
- no SDR clamp;
- no sRGB OETF;
- no appearance writeback.

An optional embedded JPEG preview is presentation-only/non-authority.

Terminology should prefer:

`PURE Float32 Scientific Linear DNG`

rather than implying that the reconstructed RGB DNG is the original CFA sensor mosaic.

### JPG-L RAW/Edit v0.3

The old production idea
`JPEG front + hidden Float32/TN-3 tail`
was rejected for Lightroom editing because Lightroom edits only the JPEG front.

Current JPG-L RAW/Edit:
- external file identity: `.dng`;
- MIME: `image/x-adobe-dng`;
- primary editable raster: Float32 linear image;
- embedded JPEG preview: secondary/non-authority;
- Advanced settings stored as a non-destructive edit recipe;
- Scientific Master remains unchanged;
- `scientific_writeback_allowed=0`;
- `creates_new_evidence=0`.

The generic 8-bit JPG remains a delivery/compatibility export.

### ADVANCED Render/Edit Float32 v0.1

Implemented and CI-green.

UI/export:
`Render/Edit · Float32 DNG · Lightroom`

Correct model:
`Scientific Master parent -> developed extended-linear Float32 derivative -> own projected-raster digest -> deterministic replay -> XYZ-D50 Float32 DNG`

Primary derivative identity:
`EXTENDED_LINEAR_SRGB_FLOAT32`

Stored DNG primary:
`XYZ_D50_LINEAR_FLOAT32`

The route is implemented **above/outside** byte-frozen
`full-frame-streaming-v0.1`. The sealed streaming module remains unchanged.

Current baked/recipe rules:
- v4.7j Detail may be baked into the Float32 derivative;
- Open-World Light may be baked;
- Restoration may be baked only as `AESTHETIC_REINTEGRATION_ONLY`;
- negative components are preserved;
- values above 1 are preserved;
- Natural HDR is **not** baked into the primary while v0.84 output authority contains UNKNOWN;
- Natural HDR remains recipe/preview-only;
- v4.7k Output Acutance is not baked because it belongs after final output sizing;
- Scientific Master remains the unchanged verified parent;
- `scientific_writeback_allowed=0`;
- `creates_new_evidence=0`.

Identity gate:
1. derive the extended-linear raster;
2. hash it on the canonical 64×64 grid;
3. replay the same derivative source;
4. require the DNG writer to reproduce the declared projected-raster SHA exactly;
5. only then commit the artifact.

A corrected halo implementation reconstructs Detail/Restoration support directly from
the same admitted Stage-2 source + canonical v4.7i, so storage/compute tile boundaries
do not become image boundaries.

Automated gates currently pass:
- wide-tile versus split-tile Float32 bit identity;
- exact negative-headroom preservation;
- repeated projected-raster SHA determinism;
- sealed tile-native streaming reference;
- exact v4.7i O0/O2 signature equality.

Architecture/details:
`docs/TRUTHRAW_V0842_ADVANCED_RENDER_EDIT_FLOAT32_2026-09-21.md`

Still open:
- real-device Lightroom Android 17 import/edit/export round-trip proving that Lightroom
  edits the Float32 primary as intended rather than only the embedded preview.

## v0.84 output-channel authority

Schema:

`TruthRawOutputChannelAuthority/0.84`

A conservative per-output RGB-channel authority map now exists and is bound into:
- Advanced preview;
- full-resolution photo export;
- PURE Float32 DNG;
- JPG-L RAW/Edit Float32 DNG.

It records:
- `CALIBRATED_ESTIMATE`;
- `RECONSTRUCTED`;
- `CENSORED`;
- `UNKNOWN`.

Hard rules:
- orientation changes coordinates, not authority;
- compute tile boundaries never become authority boundaries;
- resampling without explicit bound propagation fails closed to UNKNOWN;
- appearance cannot upgrade authority;
- current v0.79 does **not** admit reconstructed uncertainty.

### Current HDR state changed from v0.83

Old v0.83 reason:
`NO_PER_OUTPUT_CHANNEL_AUTHORITY`

Current v0.84-integrated reason:
`UNKNOWN_CHANNEL_AUTHORITY_PRESENT`

Therefore:
- per-output-channel authority **is available**;
- Scientific HDR authority remains **BLOCKED**;
- `scientificGainAllowed=false`;
- Natural HDR presentation remains `APPEARANCE_ONLY` when enabled;
- censored exact recovery remains forbidden;
- UNKNOWN gets no scientific headroom;
- illumination cannot create RGB authority/headroom.

Do not say v0.84 automatically admitted Scientific HDR.

## Background execution / operation status

The earlier symptom was clarified as **app backgrounding**, not autofocus.

Long operations previously could show an elapsed timer while actual work made little
or no progress until the app returned to foreground.

v0.84.1 adds a real Android `mediaProcessing` foreground execution guard plus
partial wake-lock and persistent operation state.

Guarded main operations include:
- preview/render;
- NEF measurement;
- full-resolution JPG;
- JPG-L RAW/Edit;
- ADVANCED Render/Edit Float32 DNG;
- PURE Float32 DNG;
- TruthNegative TN-3;
- Linear DNG.

Existing specialized foreground workers remain for:
- full-resolution Restoration;
- Restoration DNG/TIFF/EXR projections.

UI convention:
- green dot + live timer = worker really active;
- green terminal state = success;
- red terminal state = unexpected stop/failure;
- interrupted/stale workers are recovered as explicit failure, not eternal green.

No Camera-5 autofocus change was made from the earlier misunderstanding.

## Android acceleration v0.84.2

Research/implementation root:

`docs/research/android-acceleration-v0.1/README.md`

The acceleration layer is performance-only. It does not redefine science.

### Strict-FP O2

The Android native debug library now uses:
- `-O2`;
- `-fno-fast-math`;
- `-ffp-contract=off`.

CI builds the same canonical v4.7i fixture at O0 and O2 before the APK.

Current signatures:

- O0: `5dba058e5742346a`
- O2: `5dba058e5742346a`

Mismatch must block promotion.

### Ordered multicore executor

A new bounded executor permits:
- parallel tile compute;
- limited in-flight results;
- strictly ordered canonical commit/write/hash;
- fail-fast cancellation.

The host test proves:
- more than one worker can be active;
- commit order is always 0..N;
- compute failure fails closed;
- commit failure fails closed.

### Full-resolution Restoration

Restoration is the first real multicore production candidate.

It now uses:
- dynamic worker count, initially 1..8;
- one independent read-only DNG source per worker;
- one independent canonical v4.7i reconstruction workspace per worker;
- parallel restoration compute;
- single canonical ordered digest/role-mask/`.trr` commit.

Worker count is runtime telemetry only and is **not embedded into artifact identity**.

Still-open proof:
- run the exact same fixture/source at workers=1 and workers=N;
- require whole-file `.trr` SHA-256 equality.

Do not call this equivalence proven until that gate is run.

### Dynamic CPU worker policy

Inputs:
- online CPU count;
- memory class and declared bytes/worker;
- power-save state;
- Android thermal status;
- thermal headroom;
- Android 16+/17 CPU resource headroom.

No manual big/little core affinity is used.

The policy leaves Android free to place work across heterogeneous cores.

### Android 16+/17 CPU/GPU resource headroom

Schema:

`TruthRawSystemHeadroom/0.1`

To preserve compileSdk/minSdk compatibility, the NDK API is discovered at runtime
through `libandroid.so`:

- `ASystemHealth_getCpuHeadroom`;
- `ASystemHealth_getGpuHeadroom`;
- CPU/GPU minimum polling interval calls.

Calls are cached according to the device's reported minimum interval.

TruthRaw scheduling heuristics:
- low CPU resource headroom reduces worker count;
- known GPU resource headroom below 20% temporarily suppresses future Vulkan candidacy.

These are performance heuristics only.

### ADPF Performance Hint

Restoration workers can create an optional Performance Hint session.

Because native Performance Hint symbols are API33+, while TruthRaw minSdk remains 31,
they are loaded dynamically from `libandroid.so`.

Per worker:
- a hint session is attempted on that worker thread;
- each successful tile reports actual work duration;
- unsupported/missing symbols fall back to normal Android scheduling;
- number of workers with a real ADPF session is runtime telemetry only.

UI/capability reporting treats ADPF worker hints as API33+.

## Vulkan

Implemented now:
- runtime Vulkan loader probe;
- reject CPU/software physical devices for performance selection;
- require hardware device + compute queue;
- record API/vendor/device/driver;
- probe Android HardwareBuffer external-memory extension;
- probe relevant Float16/Int8/Float64 capabilities.

Not implemented/promoted:
- no Vulkan pixel kernel yet;
- CPU_REFERENCE remains the selected backend.

First Vulkan promotion should target APPEARANCE or PRESENTATION work, not Scientific Master.

A candidate backend must pass:
1. kernel-specific correctness self-test;
2. benchmark against optimized CPU;
3. sustained/thermal test;
4. failover test.

## ARM SIMD / SHA2

Runtime capability detection exists for:
- ARM64/NEON;
- ARMv8 SHA2.

The current TruthRaw SHA-256 transform is still the portable reference.
An ARM SHA2 path remains open and must produce **exactly the same digest** before selection.

## Qualcomm / MediaTek acceleration

Investigated but not production-selected:

- Qualcomm FastCV;
- Qualcomm QNN / Hexagon NPU;
- MediaTek NeuroPilot / Neuron.

Do not select a backend merely from a brand/model string.
Selection must be capability + correctness + benchmark based.

## APV — Advanced Professional Video

Research doc:

`docs/research/android-acceleration-v0.1/APV_PROFESSIONAL_VIDEO_ROUTE.md`

Android runtime probe:
- MIME `video/apv`;
- encoder and decoder enumerated separately;
- alias duplicates ignored;
- hardware/vendor/software flags;
- color formats;
- profile/levels;
- max resolution and bitrate;
- performance points.

On Android 17 the PRO workbench reports this explicitly.

APV is **not** a TruthRaw compute backend and **not** Scientific Master.

Allowed research roles:
- professional all-intra video intermediate;
- high-quality edit proxy;
- future auxiliary preview stream;
- possible camera-video target after device-specific route proof.

Forbidden:
- Source Evidence replacement;
- Scientific Master replacement;
- Float32 still-photo replacement;
- authority upgrade.

A hardware APV route is not production-selected until a real encode/decode
round-trip, quality/tolerance and sustained thermal test pass.

OpenAPV is research-only and not bundled.

## PRO diagnostics now expose

Read-only:
- CPU core count;
- NEON/SHA2 capability;
- Vulkan hardware identity/capabilities;
- thermal headroom;
- Android CPU resource headroom;
- Android GPU resource headroom;
- dynamic CPU worker recommendation;
- APV encoder/decoder hardware diagnostics;
- current compute candidates;
- active backend remains `CPU_REFERENCE`.

## Scientific modules that remain frozen

Do not silently alter:
- immutable Source Evidence;
- PURE scientific identity contract;
- Zero-Line/L0;
- Technical Backplane;
- canonical v4.7i reconstruction semantics;
- byte-frozen `full-frame-streaming-v0.1`;
- v0.67 Restoration scientific role semantics;
- canonical Open Scene v0.70;
- v0.78/v0.79 authority/uncertainty law;
- physicalFrameCount=1;
- independentEvidenceCount=1.

Acceleration provenance never upgrades these.

## Immediate continuation

In this order:

1. real-device Android 17 smoke test:
   - leave app during render/export and verify actual work continues;
   - inspect green/red timer state;
   - inspect PRO CPU/GPU headroom, Vulkan and APV probe;
   - record actual dynamic worker count and ADPF worker coverage;
2. prove `.trr` whole-file SHA equality for workers=1 versus workers=N;
3. run the real-device Lightroom Android 17 import/edit/export round-trip for ADVANCED Render/Edit and verify Lightroom uses the Float32 primary;
4. create first Vulkan APPEARANCE/PRESENTATION kernel and require correctness + CPU benchmark;
5. add exact ARMv8 SHA2 transform only behind digest-equivalence testing;
6. prototype parallel Scientific Master wrapper per
   `PARALLEL_SCIENTIFIC_MASTER_PLAN.md` and keep v0.3 reference authoritative;
7. APV hardware encode/decode round-trip before production selection.

## Read order for the next chat

1. `state/CURRENT_PROJECT_STATE_2026-09-21.json`
2. this handoff
3. `docs/research/android-acceleration-v0.1/README.md`
4. `docs/research/android-acceleration-v0.1/APV_PROFESSIONAL_VIDEO_ROUTE.md`
5. `docs/research/android-acceleration-v0.1/PARALLEL_SCIENTIFIC_MASTER_PLAN.md`
6. `docs/TRUTHRAW_V0842_ADVANCED_RENDER_EDIT_FLOAT32_2026-09-21.md`
7. `docs/TRUTHRAW_V083_HDR_AUTHORITY_2026-09-20.md` as historical v0.83 contract background
8. current v0.84 output-authority code before changing HDR semantics.
