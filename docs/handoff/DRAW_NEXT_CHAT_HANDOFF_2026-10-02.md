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

`#115 — Encode canonical authority bytes directly v0.2.8`

Source-code merge checkpoint:

`85e67ca3090aa113edda7133d656f28087e3ad22`

PR #115 source head before merge:

`052b95ef8e648517281aeac141030f2d41fe1e9e`

PR #115 was exact-parity green and real-device validated before merge. The merge checkpoint then completed **37/37 workflows green, 0 failures**.

## Current Android build line

Application ID:

`com.truthraw.adaptiveui`

Version code:

`26100109`

Version name:

`0.53-v0.84.2-authority-direct-byte`

Stable signing certificate SHA-256:

`a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`

Post-merge APK:

- size: `8,486,427 bytes`
- SHA-256: `71390d5f58c431c565d987e76daa86f908d3f7ce1aef646674ef18a27d2b466a`
- Suite run: `37003837807`
- artifact: `11224452595 — DRAW_Full_Suite_Universal_Intake_v0.1_debug_arm64`
- artifact ZIP digest: `sha256:bbec758e92837c2ef813415d2e177aef02f3dbe8bda24ee0d28a204b401b9674`

This remains an in-place update on the same package/signing lineage. Normal continuation tests must not uninstall the app or clear app data unless a specific proven failure requires it.

## Current real-device performance result — v0.2.8

The validated Foundation export used:

`D.RAW/UniversalSourceProfileCache/0.2.8-authority-direct-byte-v1`

Profile count:

`2`

Aggregate profile elapsed:

`32,967.853217 ms`

This is about **18.96% faster** than the same two-RAW v0.2.7 aggregate (`40,683.016651 ms`).

### Observation 1

- profile elapsed: `14,797.067651 ms`
- N2 stage: `N2_LOCAL_SPATIAL_V01_R8_AUTHORITY_DIRECT_BYTE`
- N2 elapsed: `14,459.694161 ms`
- shared acquire: `8,891.15 ms`
- v0.1 CFA audit: `858.39 ms`
- center-excluded: `3,967.78 ms`
- center-excluded predictor: `2,975.81 ms`
- Scientific Master bind: `8,344.42 ms`
- authority direct-byte/stream hot path: `5,346.54 ms`
- direct-byte records: `37,601,280`
- generic fallback records: `0`
- canonical bytes per authority record: `25`
- hash batch: `96 records / 2,400 bytes`
- authority accumulator record-vector scratch: `0 bytes`

### Observation 2

- profile elapsed: `18,170.785566 ms`
- N2 stage: `N2_LOCAL_SPATIAL_V01_R8_AUTHORITY_DIRECT_BYTE`
- N2 elapsed: `17,269.418014 ms`
- shared acquire: `9,482.18 ms`
- v0.1 CFA audit: `884.319 ms`
- center-excluded: `6,077.23 ms`
- center-excluded predictor: `4,821.46 ms`
- Scientific Master bind: `8,731.73 ms`
- authority direct-byte/stream hot path: `5,670.28 ms`
- direct-byte records: `37,601,280`
- generic fallback records: `0`
- canonical bytes per authority record: `25`
- hash batch: `96 records / 2,400 bytes`
- authority accumulator record-vector scratch: `0 bytes`

### v0.2.8 device conclusion

The specialized route is genuinely active and complete for the current source semantics:

- `authority_direct_record_streaming_active=true`
- `authority_temporary_record_vector_used=false`
- `authority_direct_byte_encoding_active=true`
- `authority_generic_record_validation_bypassed=true`
- `authority_generic_fallback_record_count=0`
- `authority_accumulator_resident_bytes_upper_bound=0`
- `authority_field_fused_into_scientific_master_pass=true`
- `authority_field_replay_pass_performed=false`

Scientific safety remains intact:

- sparse-reference reuse verified;
- no v0.1 rerun;
- row-band reuse active with zero fallbacks;
- row-band scientific values not modified;
- source values not modified;
- candidate not applied;
- no new evidence;
- no scientific writeback.

The direct-byte step therefore removed a large part of the former per-record object/validation overhead. The remaining authority hot path still processes exactly `37,601,280 * 25 = 940,032,000` canonical authority bytes per RAW through SHA-256.

## Current next performance frontier

The next isolated candidate is **SHA-256 direct-block transport v0.2.9**.

The current generic `Hasher::update()` copies every complete 64-byte SHA block into its internal 64-byte buffer before calling the unchanged compression transform. For the authority stream this creates a redundant memory pass over almost 940 MB per RAW.

The candidate may:

- fill an existing partial block exactly as before;
- transform already-contiguous complete 64-byte input blocks directly from the caller buffer;
- keep the internal buffer only for the remaining partial tail;
- leave `transform()`, padding, bit length and digest semantics unchanged;
- prove known SHA-256 vectors, long multi-block vectors and arbitrary chunk-boundary parity;
- expose only diagnostic/performance telemetry if useful;
- make no scientific/authority/evidence changes.

Do **not** combine this with the later per-pixel/three-channel authority encoder optimization. Keep the changes separable so real-device gains remain attributable.

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
