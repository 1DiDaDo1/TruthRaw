# START HERE — TruthRaw current bootstrap

This is the living bootstrap entry point for the consolidated TruthRaw research state as reconstructed and updated on 2026-09-16.

It supersedes older bootstrap ordering as a **current-navigation document**. It does not rewrite or invalidate frozen historical evidence, dated handoffs, module-local validation or canonical bytes.

## Mandatory current reading order

Read in this order:

1. `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
2. `docs/CURRENT_SCENE_PHYSICS_RESTORATION_200MP_2026-09-16.md`
3. `state/CURRENT_PROJECT_STATE_2026-09-16.json`
4. `docs/DOCUMENT_STATUS_INDEX_2026-09-16.md`
5. `docs/PROJECT_HISTORY_AND_CHANGES_2026-09-16.md`
6. `docs/handoff/TRUTHRAW_CONSOLIDATED_HANDOFF_2026-09-16.md`
7. `docs/handoff/TRUTHRAW_DETAILED_HANDOFF_2026-09-16.md`
8. only then the exact canonical/research module documents relevant to the task

Historical architecture documents remain required background when their domain is involved:

- `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md`
- `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
- `docs/CORE_VISION_UNCERTAINTY_AWARE_APPEARANCE.md`
- `docs/CORE_VISION_VIRTUAL_OBSERVATION_MANIFOLD.md`
- `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md` — historical 2026-09-10 current-house snapshot, not the 2026-09-16 global state
- `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md` — historical governance snapshot
- `state/CURRENT_CANONICAL_STATE_2026-09-10.json` — historical state snapshot

Do not bootstrap global current state from older `CURRENT_CANONICAL_STATE_*` snapshots or old project audits merely because their filenames contain `CURRENT`. Their date and the 2026-09-16 document-status index control their interpretation.

## One-sentence current definition

**TruthRaw seals one app-visible RAW/CFA observation as immutable source evidence, reconstructs a separate uncertainty- and authority-aware Scientific Master in Free Scientific Space, and permits open-world/counterfactual/restoration/appearance projections only without upgrading what the capture actually measured.**

## Permanent scientific laws

1. **Source evidence is immutable.** Original admitted bytes, decoded CFA identity and capture provenance are history, not a workspace to rewrite.
2. **Representation can exceed the source. Knowledge claims cannot exceed the evidence.** Free Scientific Space may exceed RAW10, `WhiteLevel`, `[0,1]`, source ISO scale, original CFA lattice, SDR, integer storage and DNG.
3. **Measured is not reconstructed.** `MEASURED`, `RECONSTRUCTED`, `CENSORED`, `UNKNOWN`, `COUNTERFACTUAL` and appearance/transport/restoration state must remain distinguishable.
4. **Single-frame evidence count stays one.** `physicalFrameCount=1`, `independentEvidenceCount=1`; virtual observations do not create additional measurements.
5. **Censoring is a bound, not a guessed exact value.** Saturation may justify an inequality; it does not reveal exact latent radiance.
6. **Uncertainty/support must be locally bound.** Wrong identity, wrong coordinate, missing runtime field or missing feature semantics fails closed.
7. **Numerical precision is stage-specific.** F64 is required where branch-sensitive reconstruction has demonstrated F32 hazards; controlled F32 storage is a separate gate.
8. **Counterfactual state never becomes capture evidence.** CICM/relighting/virtual cameras may describe hypothetical worlds only in their proper authority class.
9. **Appearance and transport never write back into science.** Lightroom, Gain Maps, tone, display HDR and exports are downstream projections.
10. **Restoration never overpaints valid measured support in science.** Loss compensation stays reconstructed; aesthetic reintegration remains non-writing.
11. **Compute resources never increase truth authority.** Device class changes execution strategy, not evidence.

## Terminology correction: house -> Free Scientific Space

The sealed-house/new-house language is retained because it records how the architecture was discovered.

Current formal reading:

- **sealed house** = historical metaphor for immutable Source Evidence;
- **new house** = historical metaphor for the reconstructed scientific world;
- **Free Scientific Space** = current formal reconstructed representation domain;
- **open-world correction** = Free Scientific Space is not bounded to an interior, finite house or Room Capsule. Exterior, street, landscape, sky, distant structure and other scene graphs are allowed as reconstruction when authority/support permits them;
- **Room Capsule** = bounded local compute, not a boundary on the possible represented world.

## Current project flow

`Source Evidence`
`-> measurement/de-ISP`
`-> Scientific Master in Free Scientific Space`
`-> Dynamic Authority / uncertainty / support`
`-> Open Scene State`
`-> optional counterfactual/restoration state`
`-> appearance / HDR / transport`
`-> finite export`

DNG, LinearRaw, reconstructed CFA, restored renders and display files are projections/compatibility products. They do not replace the Scientific Master or retroactively become original measurement evidence.

## Scene physics continuity

The latest research makes explicit that light, colour, detail and HDR are coupled through the image-formation chain:

`illumination -> geometry/visibility -> material response -> scene radiance -> optics -> sensor/CFA/noise -> RAW evidence`

Read `docs/CURRENT_SCENE_PHYSICS_RESTORATION_200MP_2026-09-16.md` before changing relighting, colour, detail/sharpening, HDR or Camera-5 maximum-resolution logic.

Current rules include:

- inverse-square falloff is conditional on source geometry, not camera-object distance by itself;
- relighting without sufficient geometry/material/visibility/illumination support remains `COUNTERFACTUAL`;
- D65/D50/A are standard reference illuminants, not automatic proof of a captured illuminant;
- source/DNG matrices can be source-bound transforms without being FULL_PHYSICAL calibration;
- sample count and optical resolution are distinct;
- SFR/MTF/detail support and visual acutance are separate authorities;
- physical scene range, sensor evidence range, Scientific-Master representation range and display/HDR transport range must remain separate.

## Conservation/restoration continuity

The historical Restorer room now has a formal conservation-inspired authority model.

Professional conservation contributed the operational pattern:

`examine -> condition report -> scientific investigation -> stabilize -> compensate/restorate -> document intervention`

TruthRaw maps this to:

`sealed source -> condition/evidence assessment -> stable Scientific Master -> bounded reconstruction -> optional aesthetic reintegration -> provenance-preserving export`

Critical rules:

- valid measured support is preserved like surviving original material;
- inferred repair cannot overwrite valid measured support in the Scientific Master;
- supported loss compensation is `RECONSTRUCTED`, never `MEASURED`;
- censored support remains a bound;
- unsupported loss remains unresolved;
- aesthetic reintegration may be visually seamless but has no scientific writeback;
- restoration must remain provenance-bound and computationally retreatable.

See:

- `docs/research/conservation-restoration-authority-v0.1/README.md`
- `tools/restoration_authority_v01.py`
- `tests/test_restoration_authority_v01.py`

## Precision and uncertainty continuity

The current precision direction was not chosen by preference. Real MotionCam and Honor data showed that tiny F32/F64 differences before a branch threshold can select different reconstruction paths.

The retained direction is:

`exact RAW integer/packed evidence`
`-> F32 only where proven safe`
`-> F64 branch-sensitive reconstruction`
`-> F64 calibration / optimization / covariance`
`-> optional controlled F32 storage after F64 compute`
`-> arbitrary/high precision as reference validation`

The retained eight-file promotion evidence supersedes provisional locator counts. Historical `3549/44/2580` values are provenance only; the retained v0.4 authority result is `3545/0/0` for branch differences / green-clamp differences / colour-clamp differences, with no measured-channel violations.

The historical v5.0g/p1 uncertainty model specification is known, but the exact historical feature extractor source is not. The missing 10,023-byte `uncertainty_core_v5_0g.py` remains a formal blocker. Do not infer the 18 feature semantics from names and do not “clean up” the historical role-order quirk.

## Current Android/device evidence

The v0.3 Android report supplied from the target Honor device is part of the current research evidence set:

- `schema = TruthRawAndroidVerificationReport/0.3`
- `classification = DEVICE_VALIDATION_ONLY_NO_SCIENTIFIC_WRITEBACK`
- `app_version = 0.3-debug`
- device: `HONOR BKQ-N49`
- Android: `16 (API 36)`
- selected source: `IMG_BNC_TRUTHRAW20260907_094449_565.dng`
- result: `PASS_EXACT_SOURCE_AND_DECODED_CFA`
- source SHA-256: `7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67`
- decoded CFA SHA-256: `883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c`

It proves exact source/CFA identity for the scoped verifier. It explicitly does **not** prove that Scientific Master, Dynamic Authority or HDR have been recomputed on-device.

The v0.3 build identity retained in `android/README.md` includes exact signed APK SHA-256:

`01f94593ebd9dcb8d7e5f8c5681eed1f79f469de3862b98723912fb9a36d8b61`

## Scientific Master / Dynamic Authority frozen references

Current source-bound research identities carried by the v0.3 report:

- Scientific Master SHA-256: `a86034da7b6f9663640ee3b4478ccf4294d23b720f2b087d083dca38f5fb4640`
- Dynamic Authority v1.9 SHA-256: `7678a0b145f8721347cdcc5177fedb720ba19b91bf34984a3f18bd8216408098`
- source-bound P3 transform SHA-256: `2105712a1be9950089976ba358afd4b062347680d5e6d8c500462c2e8d541533`
- reference `L0`: `0.12564234435558320`

Treat these as frozen downstream references for the validated source, not as values recomputed by Android v0.3.

## 200 MP / FotoGraaf continuation gate

Do not call advertised/static maximum resolution “200 MP RAW truth”. Current Camera-5 static evidence establishes the advertised path, not a delivered physical 200 MP frame.

Current static facts:

- physical camera id `5`;
- `ULTRA_HIGH_RESOLUTION_SENSOR=true`;
- `REMOSAIC_REPROCESSING=false`;
- maximum array `16320x12288` = `200,540,160` samples;
- advertised high-resolution `RAW_SENSOR 16320x12288` and `RAW10 16320x12288`;
- `SENSOR_INFO_BINNING_FACTOR=2x2` is also reported and is retained as vendor-metadata tension rather than proof of app-visible same-colour binning.

Android's UHR contract supports the bounded interpretation `APP_VISIBLE_RAW_SENSOR_REGULAR_BAYER_BY_ANDROID_CONTRACT` when remosaic reprocessing is absent.

Machine guard:

- `tools/camera5_200mp_android_contract_v04.py`
- `tests/test_camera5_200mp_android_contract_v04.py`

The next physical promotion must establish an actual Camera-5 maximum-resolution RAW_SENSOR observation and bound capture result:

`logical camera 0 -> physical camera 5 -> SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION -> RAW_SENSOR 16320x12288 -> physical TotalCaptureResult 5`

Current physical gate:

`OPEN_NEEDS_REAL_16320x12288_RAW_PAYLOAD_AND_TOTALCAPTURERESULT_BINDING`

A successful result is bounded to `APP_VISIBLE_MAXIMUM_RESOLUTION_RAW_SENSOR_CFA_PROVEN`. It is **not** automatically `UNTOUCHED_NATIVE_200MP_ADC`.

After a genuine qualifying sample exists, the correct order is:

`physical RAW proof -> readout-domain-specific precision/uncertainty -> noise/PTC -> shading -> color/illuminant -> SFR/MTF -> held-out calibration -> Scientific Master admission`

Do not borrow 4080x3072 uncertainty/calibration into 16320x12288 merely because the same physical camera is involved. A 200 MP sample raster also does not by itself prove 200 MP optical detail.

## HDR / Adobe boundary

Scientific HDR headroom belongs to the Scientific Master + authority/support. `UNKNOWN` contributes no scientific headroom. `CENSORED` may carry a lower/upper bound as appropriate but not an invented exact radiance.

Adobe Gain Map is downstream presentation/display adaptation. A viewer's HDR capability changes presentation, not source evidence or Scientific Master authority.

## Repository/governance rules

- Preserve historical/read-only snapshots as provenance rather than rewriting their past claims.
- Use `docs/DOCUMENT_STATUS_INDEX_2026-09-16.md` to determine global-current versus historical/version-local authority.
- Do not promote research success into canonical/main status without the explicit relevant gate.
- APK/GCam/computational-RAW material does not determine scientific evidence, calibration, noise, topology or color truth.
- No silent LICENSE changes.
- Keep rejected/failed experiments and superseded counts visible as historical evidence.
- `canonical/ptc/v1.1` means **Pure Truth Certificate**.

## Immediate continuation

The safe current implementation work is:

1. keep the v5.0g feature-extractor blocker open until exact or hash-verified equivalent semantics exist;
2. obtain and ingest a real qualifying Camera-5 16320x12288 RAW_SENSOR evidence set;
3. bind the future 200 MP readout domain to its own precision/uncertainty, noise/PTC, shading, colour and SFR/MTF gates;
4. continue integrating scene physics and restoration masks into Open Scene State / Dynamic Authority without widening measured authority;
5. keep display/restoration/counterfactual operations non-writing with respect to source evidence.
