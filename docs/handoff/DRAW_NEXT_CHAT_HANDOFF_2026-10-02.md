# D.RAW next-chat handoff — 2026-10-02

## Continuation code: 44488

When a new chat receives only **44488**, treat it as a request to restore the complete D.RAW/TruthRaw context through this checkpoint before making further scientific or code changes.

44488 supersedes the earlier continuation anchors 44487, 44485 and 44484 as the newest handoff code. Older handoffs remain provenance.

The code is a recognition anchor, not a magical transport mechanism. The durable recovery source is this repository state, especially:

1. `state/CURRENT_PROJECT_STATE_2026-10-02.json`
2. this handoff
3. `START_HERE_NEW_CHAT.md`
4. `docs/DOCUMENT_STATUS_INDEX_2026-10-02.md`
5. the referenced current research READMEs and older dated handoffs when historical provenance is needed.

A future chat receiving 44488 should not ask the user to reconstruct this project manually. It should restore the repository state first, verify the current branch/head/build before modifying code, and preserve all authority boundaries below.

## Repository checkpoint

Repository:

`1DiDaDo1/TruthRaw`

Current development branch:

`fix/android17-research-resilience-v02`

Latest merged performance PR:

`#116 — SHA-256 Direct Block v0.2.9`

Source-code merge checkpoint:

`edcae3eabb077d53b6a25802e6b9ca62c188b973`

PR #116 source head before merge:

`0e7c8d70974a45ac5171dc06d894a8dc4f422c93`

PR #116 was 34/34 green and real-device validated before merge.

## Current validated Android build line

Application ID:

`com.truthraw.adaptiveui`

Version code:

`26100110`

Version name:

`0.53-v0.84.2-sha-direct-block`

Stable signing certificate SHA-256:

`a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`

Validated candidate APK:

- size: `8,487,331 bytes`
- SHA-256: `30756dbff117b4fba432e7f541eea37e0b4bbe35ef9ea24beb301a20d6e1e2bd`

The app remains an in-place update. Do not uninstall or clear app data for normal performance continuation.

## Current real-device performance result — v0.2.9

The validated Foundation export used:

`D.RAW/UniversalSourceProfileCache/0.2.9-sha-direct-block-v1`

Aggregate two-RAW profile elapsed:

`32,515.211967 ms`

This is about **1.37% faster** than v0.2.8.

Observation 1:
- profile elapsed `14,466.439474 ms`
- N2 `14,102.344890 ms`
- shared acquire `8,481.58 ms`
- Scientific Master bind `7,922.83 ms`
- authority hot path `5,058.70 ms`
- center-excluded `4,008.33 ms`
- predictor `3,010.19 ms`

Observation 2:
- profile elapsed `18,048.772493 ms`
- N2 `17,135.811608 ms`
- shared acquire `9,282.79 ms`
- Scientific Master bind `8,511.31 ms`
- authority hot path `5,481.69 ms`
- center-excluded `6,118.97 ms`
- predictor `4,863.84 ms`

For each RAW:
- direct authority-byte records: `37,601,280`
- generic authority fallback records: `0`
- SHA direct input block transforms: `14,296,320`
- SHA buffered input block transforms: `392,832`
- SHA direct input bytes: `914,964,480`
- authority record-vector scratch: `0 bytes`

Safety remains unchanged:
- sparse reuse verified;
- no v0.1 rerun;
- row-band active with zero fallback requests;
- no source-value modification;
- candidate application false;
- creates-new-evidence false;
- Scientific Master writeback false.

## Current next performance frontier

The next isolated candidate is **Pixel-Triplet Authority Encoder v0.2.10**.

The current fast path still determines CFA phase/censor semantics and emits one 25-byte canonical authority record three times per pixel.

v0.2.10 may:
- determine the pixel's measured CFA channel and censor state once;
- emit the exact three channel records in order 0,1,2 as one 75-byte canonical triplet;
- update counts from the same specialized pixel semantics;
- preserve the exact v0.2.9 byte stream and therefore the exact authority SHA-256;
- retain a versioned fail-closed fallback to the general per-channel path for future semantic extensions.

Do **not** combine template/suffix reuse, larger batch-size tuning or center-excluded changes into v0.2.10. Those remain later isolated steps and are conditional on real-device evidence.

## N2 performance lineage that must remain understood

The recent optimization line is cumulative:

- PR #109 — N2 sparse-reference reuse v0.2.2
- PR #110 — N2 row-band reuse v0.2.3
- PR #111 — diagnostic native phase timing v0.2.4
- PR #112 — preparation/center-excluded subphase timing v0.2.5
- PR #113 — fuse authority-field summary into Scientific Master pass v0.2.6
- PR #114 — direct-stream authority records into canonical digest v0.2.7

Important learned facts:

- v0.1 reruns were eliminated by sparse-reference reuse;
- row-band reuse removed repeated RAW/gain tile reads across adjacent requests;
- phase timing proved that shared preparation and center-excluded were the dominant remaining blocks;
- subphase timing proved the old authority summary replay dominated shared preparation;
- authority fusion removed the second full replay, but most authority work moved into Scientific Master pass 1;
- v0.2.7 removed the temporary `ChannelRecord` vector, proving that allocation was not the dominant remaining cost;
- the current authority hot path is per-record semantics + validation + canonical hashing;
- center-excluded remains the second major compute block and its cost scales strongly with candidate-center count.

## Existing scientific result that must not be forgotten

Anchor-Constrained Local Reconstruction v0.1 real-device hold-out remains a scientific non-promotion result.

The affine candidate had better coverage but was worse overall on directly comparable true CFA hold-outs:

- true hold-outs: 21,760
- comparable baseline-valid samples: 19,560
- affine MAE about 0.00154976 vs baseline about 0.00147486
- affine RMSE about 0.00205484 vs baseline about 0.00190590
- affine bias about -0.00049664 vs baseline about -0.00009471
- affine wins 9,363 vs baseline 10,197
- affine added 2,200 valid points
- channel 2 was slightly better; channel 1 clearly worse
- uncertainty was too optimistic
- candidate application/writeback remained false

Do not promote that affine solver merely because later performance work is green. The scientific next direction there remains deterministic local model selection using structural support, direction, CFA phase and uncertainty.

## Permanent scientific laws

These are not performance options:

- **MEASURED != RECONSTRUCTED != APPEARANCE**
- **Seal the evidence, not the thinking.**
- **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
- Direct-CFA source evidence stays immutable/sealed.
- Single-frame provenance remains explicit.
- Scientific Master is separate from export/presentation.
- Camera/lens/vendor/RAW identity may route parsing/decoding but may not select scientific truth.
- APK/GCam/computational-RAW content may not determine TruthRaw evidence, calibration or source truth.
- Held-out values may evaluate a frozen candidate but may not fit/select it.
- Unknown residual remains UNKNOWN until supported.
- No AI/ML/neural/generative runtime in the scientific path.
- No candidate availability implies correction or promotion.
- No Scientific Master writeback from diagnostic/performance research.
- SOURCE/SENSOR SPACE, WORLD/SCENE SPACE and VIEW/OUTPUT SPACE remain distinct.
- Appearance-derived frontside geometry is not sensor evidence.
- A topographic surface is an observable visualization, not automatically scene depth, lens sag, field curvature, vignetting or aberration.
- A stitched panorama is not a new physical observation.
- Independent observations may only be related through admitted evidence; shared lens/camera identity alone is insufficient.

## Broad scientific knowledge expected in continuation

A future 44488 chat should continue to reason across the full project domain, not only Git/Android code:

- RAW/CFA imaging, radiometry, photon/read noise, dynamic range and demosaicing;
- camera optics, PSF/MTF/SFR, aberrations, diffraction, vignetting, distortion, flare, field curvature and focus;
- calibration, projective geometry, intrinsic/extrinsic relations, black/white level, linearity and uncertainty;
- colour science, chromatic adaptation, scene/display separation and perceptual colour;
- 3D geometry, light transport, material/BRDF reasoning, parallax, visibility and occlusion;
- temporal imaging, shutter integration, stop-motion, motion blur and temporal aliasing;
- human visual perception, spatial frequency, local contrast, luminance/chroma and adaptation;
- conservation/restoration authority and provenance;
- numerical precision, covariance and deterministic validation.

Those fields may inform hypotheses and candidate design, but they may not bypass the evidence/authority gates.

## GCam / APK boundary

Historical GCam tuning work is a separate side project. GCam/APK/computational RAW may be studied as software behavior, but it may not be used as TruthRaw scientific evidence, source calibration or Scientific Master authority.

## Current device-test workflow

For the present two-RAW performance line:

1. install the new version over the existing app;
2. do not clear app data;
3. keep the same two RAWs selected unless a test explicitly changes the dataset;
4. run **Analyseer alle geselecteerde bronnen universeel**;
5. keep D.RAW foregrounded for clean wall-clock comparison where possible;
6. export **Free World Observation Geometry Foundation v0.1 · JSON**;
7. compare exact native stage/subphase telemetry, not only the UI timer;
8. keep a candidate PR draft until host parity, full CI and real-device validation all pass.

The separate Field Response Repeatability minimum=3 gate is unrelated to a two-RAW N2 performance test and should not be treated as a failure of the N2 run.

## 44488 recovery instruction

A future chat receiving **44488** should:

- recognize this as the newest D.RAW/TruthRaw continuation anchor;
- recover this file and `state/CURRENT_PROJECT_STATE_2026-10-02.json`;
- inspect the repository for any newer branch/head/PR before modifying code;
- preserve every permanent authority boundary;
- treat v0.2.7 direct authority streaming as merged and real-device validated;
- continue from the canonical direct-byte authority encoder frontier unless newer evidence supersedes it;
- never silently turn a diagnostic/performance result into scientific promotion;
- never ask the user to manually reconstruct all prior project history when the repository can provide it.
