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

`#114 — Stream authority records directly into canonical digest v0.2.7`

Source-code merge checkpoint:

`33c69b635be5bcc57ec4a947eb8fda3649e492ad`

PR #114 source head before merge:

`31d2c1fc6555eb82138547628bff3776b6bea36d`

PR #114 was host-parity green and real-device validated before merge.

## Current Android build line

Application ID:

`com.truthraw.adaptiveui`

Version code:

`26100108`

Version name:

`0.53-v0.84.2-authority-direct-stream`

Stable signing certificate SHA-256:

`a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`

Current APK SHA-256:

`eecd83309b5136acc79cff11b9775a3c5635cf539d2e93d73bc3a6335a53a15c`

APK size:

`8,483,287 bytes`

Post-merge Suite Universal Intake run:

`36998622812`

Post-merge artifact:

`11222019082 — DRAW_Full_Suite_Universal_Intake_v0.1_debug_arm64`

Artifact ZIP digest:

`sha256:eb7dc0366874d0821ac24fcdfbd9fd8d54acf3907eeffcf795fc524f128c54a9`

The post-merge build reproduced the same APK SHA-256 and the same stable signing certificate.

Post-merge source checkpoint CI on `33c69b635be5bcc57ec4a947eb8fda3649e492ad`:

- total workflows: `36`
- successful: `36`
- failed: `0`
- queued/in-progress: `0`

This includes the direct-stream, fused-authority, N2 sparse/row-band/phase/subphase, Shared Scientific Context, Canonical, version-lineage, Free World and the three heavy Android/APK build families.

The APK is an in-place update from 26100107. Do not instruct the user to uninstall or clear app data for normal continuation tests unless a specific failure requires that and the reason is proven.

## Current real-device performance result — v0.2.7

The real-device Foundation export used:

`D.RAW/UniversalSourceProfileCache/0.2.7-authority-direct-stream-v1`

Profile count:

`2`

Aggregate profile elapsed:

`40,683.016651 ms`

### Observation 1

- profile elapsed: `19,376.694576 ms`
- N2 stage: `N2_LOCAL_SPATIAL_V01_R7_AUTHORITY_DIRECT_STREAM`
- N2 elapsed: `18,998.244055 ms`
- shared acquire: `13,401.1 ms`
- v0.1 CFA audit: `865.024 ms`
- center-excluded: `3,983.37 ms`
- center-excluded predictor: `2,988.03 ms`
- Scientific Master bind: `12,820.6 ms`
- fused authority finalize: `0.001198 ms`
- direct authority record stream: `9,571.86 ms`
- authority tiles: `3,072`
- authority records: `37,601,280`
- temporary authority record vector used: `false`
- accumulator resident record-vector upper bound: `0 bytes`

### Observation 2

- profile elapsed: `21,306.322075 ms`
- N2 stage: `N2_LOCAL_SPATIAL_V01_R7_AUTHORITY_DIRECT_STREAM`
- N2 elapsed: `20,395.912075 ms`
- shared acquire: `12,582.2 ms`
- v0.1 CFA audit: `888.623 ms`
- center-excluded: `6,089.38 ms`
- center-excluded predictor: `4,837.6 ms`
- Scientific Master bind: `11,795.7 ms`
- fused authority finalize: `0.001302 ms`
- direct authority record stream: `8,756.52 ms`
- authority tiles: `3,072`
- authority records: `37,601,280`
- temporary authority record vector used: `false`
- accumulator resident record-vector upper bound: `0 bytes`

### Device conclusion

The direct-stream route is genuinely active:

- `authority_direct_record_streaming_active=true`
- `authority_temporary_record_vector_used=false`
- `authority_accumulator_resident_bytes_upper_bound=0`
- `authority_field_fused_into_scientific_master_pass=true`
- `authority_field_replay_pass_performed=false`

The structural optimization is therefore proven on-device, but the aggregate speed gain versus v0.2.6 is modest, about 1.4% on this two-RAW run.

The dominant authority cost is no longer allocation/materialization of a tile-wide `std::vector<ChannelRecord>`. It is the repeated per-record construction, validation, classification accounting and canonical SHA feed for 37.6 million channel records per RAW.

## Current next performance frontier

Do **not** reintroduce the removed authority replay or temporary record vector.

The next candidate direction is a canonical **direct-byte authority encoder**:

- preserve exactly the same source-record semantics;
- preserve the same CFA phase classification;
- preserve censor classification and bound semantics;
- preserve Float32 value bits;
- preserve contribution masks and count semantics;
- preserve canonical tile order;
- preserve exactly the same authority SHA-256;
- avoid constructing a full `ChannelRecord` object for every channel when the canonical bytes/count updates can be emitted directly from the same semantic primitive;
- avoid repeated generic `validate_record()` work when equivalent fail-closed validity can be proven in the specialized canonical source-record path;
- retain the materialized route as parity oracle;
- require host bit/SHA/count parity before any APK;
- require real-device telemetry before merge/promotion.

This is a performance/transport optimization only. It may not change scientific authority.

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
