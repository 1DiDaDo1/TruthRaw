# D.RAW 44489 — compact knowledge capsule

Purpose: smallest practical project-state document that preserves the binding scientific/architectural knowledge needed for a new chat to continue safely. Read this first; use the larger recovery handoff only when detail/provenance is needed.

Project: **D.RAW** (`TruthRaw` is historical/repository naming)  
Repo: `1DiDaDo1/TruthRaw`  
Branch: `fix/research-fresh-rerun-v01`  
Recovery code: **44489**  
Always fetch the live branch head before mutation. Documentation-only commits may move the head beyond older runtime/scientific checkpoints; never reset or force-move the branch merely to match a remembered SHA.

## 1. Permanent scientific law

D.RAW is deterministic, provenance-bound and AI/ML-free in its scientific path.

- **Seal the evidence, not the thinking.**
- **MEASURED != RECONSTRUCTED != APPEARANCE.**
- Richer representation may never imply richer evidence.
- Direct-CFA / RAW_SENSOR evidence is immutable/sealed.
- One physical frame remains one physical frame; virtual/derived views do not create captures.
- Scientific Master is scene-linear scientific state, separate from view/export/appearance; signed values and >1 are valid.
- UNKNOWN is valid and must not silently become zero/certainty/estimate.
- More precision, more resolution, reconstruction, speed or registration never creates authority.
- APK/GCam/computational RAW/presentation may not define source evidence, calibration or Scientific-Master truth.

Canonical chain:
`sealed Observation -> Observation Contract -> Source Capability Envelope -> measurement/calibration/reconstruction -> Scientific Master -> Dynamic Authority+uncertainty -> Observation-bound TruthNegative -> Free World Observation Graph -> view/appearance/projection`

## 2. Evidence / authority

Authority classes include:
`MEASURED`, `CALIBRATED_ESTIMATE`, `RECONSTRUCTED`, `CENSORED`, `UNKNOWN`, `COUNTERFACTUAL`, `APPEARANCE`.

Authority is local and provenance-bound. Registration, interpolation or reconstruction never upgrades to `MEASURED` automatically.

Clipping may be `CENSORED`: a bound is known while the exact latent signal remains unknown.

Unknown RGB covariance terms stay UNKNOWN/NaN, never convenience-zero. Full covariance requires all needed terms known/consistent; transport is conceptually `J Sigma J^T`.

Optics (NPS/signal PSD/MTF/SFR/PSF), geometry, colour and world-space claims require explicit proven binding. World-space information may not rewrite Scientific Master merely because it exists.

## 3. Observation / gauge law

**One Free World. Many sealed observations. One evidence law.**

Lens/sensor/CFA/readout/capture route belong to an Observation. TruthNegative is Observation-bound; multiple observations meet only above that boundary.

`source capability != proven sample domain`.

Proven Camera-5 example: TELE provenance, `4080x3072`, RAW10/BGGR, WhiteLevel `1023`, BlackLevel `64`, source-local gauge. A `16320x12288`/200MP capability is not by itself proof that a particular captured frame measured 200MP.

TruthRange family: `T = log2(L/L0)`. Shared coordinate notation is not shared radiance. Cross-observation radiometric fusion requires an admitted common-gauge relation and otherwise fails closed.

## 3A. 200 MP / Camera-5 / Free Raster knowledge

The historical 200 MP research contains **two scientifically different paths that must never be collapsed into one “200 MP mode.”**

### A. 12.5 MP observation -> TruthNegative -> x4 Free Raster

Historical `truthnegative_dense_full_colour_v0_4` defines:
- source raster `4080x3072`;
- target raster `16320x12288`;
- `kScale = 4` on each axis;
- `createsNewEvidence=false`;
- `impliesPhysicalSensorGeometry=false`;
- `sourceMasterAnchorValuesPreserved=true`;
- `appearanceApplied=false`;
- one physical frame and one independent evidence item remain one.

Scientific interpretation: Free Raster may evaluate a continuous TruthNegative at more output positions than existed in the source raster. New output positions are **RECONSTRUCTED** unless direct source support proves otherwise. A denser raster is not automatically more measurement and is not ordinary pixel stretching either: output geometry and evidence authority are separate domains.

Binding slogan: **FREE RESOLUTION != UPSCALING; representation may exceed source sampling, knowledge claims may not exceed evidence.**

### B. Physical Camera-5 MAXIMUM_RESOLUTION RAW observation

Historical Camera-5 static capability evidence reports:
- physical camera id `5`;
- `ULTRA_HIGH_RESOLUTION_SENSOR=true`;
- advertised high-resolution `RAW_SENSOR 16320x12288`;
- advertised high-resolution `RAW10 16320x12288`;
- `REMOSAIC_REPROCESSING=false`;
- a reported `2x2` binning factor also exists and is retained as **vendor metadata tension**, not silently resolved in favour of a convenient CFA story.

The bounded Android-contract interpretation used by the research is:
`APP_VISIBLE_RAW_SENSOR_REGULAR_BAYER_BY_ANDROID_CONTRACT`.

A qualifying runtime proof may establish:
`APP_VISIBLE_PHYSICAL5_200MP_RAW_SENSOR_CAPTURE_PROVEN`.

Its hard authority boundary remains:
`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`.

Therefore an admitted `16320x12288` Camera2 RAW_SENSOR frame can be genuine source-observed app-visible RAW evidence without proving any of the following automatically:
- untouched photodiode/ADC values;
- absence of on-sensor, ISP or HAL preprocessing;
- one ADC conversion per delivered output sample;
- electron-count calibration;
- optical 200 MP resolving power;
- full physical colour truth.

Sample count is not optical resolution. CFA/sample support, optical/detail support, reconstruction support and appearance/acutance stay separate.

### 200 MP runtime admission / promotion gate

For MAXIMUM_RESOLUTION source admission, preserve and validate at minimum:
- exact raster `16320x12288` (`200,540,160` samples);
- physical camera id `5` and physical-scoped output/request binding;
- one physical frame / one independent evidence item;
- Camera2 acquisition-observation authority only unless stronger calibration is separately proven;
- exact `Image`/physical `SENSOR_TIMESTAMP` identity;
- bound `TotalCaptureResult` / physical-result identity;
- RAW payload size/stride/layout consistency;
- sealed payload SHA-256 identity;
- requested/read-back sensor-pixel-mode observations kept independently;
- Scientific Master not modified merely by admission diagnostics.

Historical v0.14 evidence intentionally preserved a returned `SENSOR_PIXEL_MODE=0` mismatch rather than rewriting it to MAXIMUM_RESOLUTION. Exact raster + physical-result + timestamp/payload proof and returned mode are separate observations; contradiction is evidence to retain, not metadata to normalize away.

Fail closed if physical 200 MP source admission is not proven. The fallback may use the normal admitted RAW observation and an explicitly RECONSTRUCTED Free Raster 200 MP projection; it may not relabel that projection as a native 200 MP measurement.

### Route orthogonality

`PURE`, `ADVANCED` and `PRO` are **processing/view routes above the sealed source**, not sensor readout modes. Historical Android UI already described all three as starting from the same sealed source / Scientific Master and routed camera input through the 200 MP staged camera activity.

Keep these dimensions independent in architecture and UI:

`Lens/Observation` = e.g. Tele / physical camera 5  
`Sensor/readout` = e.g. Default `4080x3072` or MAXIMUM_RESOLUTION `16320x12288`  
`D.RAW route` = `PURE` / `ADVANCED` / `PRO`  
`Output raster` = Native / 4K / 8K / 200 MP / Custom

Never encode all four dimensions into one overloaded “200 MP” flag.

### Recommended 200 MP implementation semantics

1. Discover capabilities per physical camera at runtime; never hard-code that another device or lens has Camera-5 semantics.
2. Put exact capture/readout facts in the Source Capability Envelope: physical/logical ids, format, exact dimensions, stream-map origin, requested and returned sensor pixel mode, binning metadata, remosaic capability, timestamps/result binding, row/pixel stride and payload SHA.
3. Seal source bytes before interpretive promotion.
4. Give native MAX-resolution observations and x4 Free Raster projections distinct state/telemetry labels such as `MAXIMUM_RESOLUTION_SOURCE` versus `FREE_RASTER_200MP_RECONSTRUCTION`.
5. In UI/provenance, distinguish concise badges such as **200 MP source** and **200 MP projection**; full authority/provenance belongs in Foundation/PRO diagnostics/export.
6. Preserve MEASURED source anchors exactly where their mapping is proven. Generated Free Raster sites remain RECONSTRUCTED with footprint/uncertainty; interpolation cannot promote itself.
7. Never accept vendor JPEG/HEIF/processed 200 MP output as RAW evidence merely because dimensions match.
8. Dimensions alone never grant CFA, optical, colour, radiometric or photodiode authority.
9. PURE/ADVANCED/PRO may render/project the same admitted source differently, but ADVANCED/PRO appearance must never write back into Scientific Master or upgrade source authority.

### Recommended same-lens three-way validation

Use the same physical tele lens/scene and compare:
1. normal `4080x3072` tele RAW -> D.RAW TruthNegative -> x4 Free Raster `16320x12288`;
2. physical MAXIMUM_RESOLUTION `16320x12288` RAW -> D.RAW;
3. HONOR/vendor 200 MP photographic pipeline output.

Evaluate separately:
- CFA topology/phase and sample support;
- SFR/MTF/PSF or equivalent resolved-detail evidence across field/focus;
- aliasing and false-detail behaviour;
- noise/NPS and signal-dependent noise;
- radiometry/linearity, black/white/clipping behaviour and dynamic-range bounds;
- registration/support footprints and uncertainty;
- vendor-added processing differences;
- repeatability under identical capture conditions.

Promotion of an optical “200 MP detail” claim requires an independent optical gate; exact 200 MP raster capture alone is insufficient.

### Required 200 MP regressions

Maintain tests that prove:
- x4 Free Raster creates no new evidence;
- directly supported source anchors remain exact;
- one input frame remains one physical frame/evidence item;
- MAX-resolution gate requires exact `16320x12288` plus correct physical binding;
- timestamp/result/payload SHA identity is enforced;
- wrong camera/result/payload binding fails closed;
- `MAXIMUM_RESOLUTION_SOURCE` cannot be confused with `FREE_RASTER_200MP_RECONSTRUCTION`;
- PURE/ADVANCED/PRO preserve identical source lineage and cannot cause scientific writeback from appearance;
- a processed vendor 200 MP image cannot be promoted to RAW evidence;
- dimensions/sample count alone cannot grant optical-resolution authority.

Historical provenance pointers to retain:
- branch `integration/truthraw-suite-v0-84-4-truthnegative-200mp-full-colour`, `suite_android/app/src/main/cpp/truthnegative_dense_full_colour_v0_4.h`;
- branch `integration/truthraw-suite-v0-10-200mp-max-highres`, `docs/CURRENT_SCENE_PHYSICS_RESTORATION_200MP_2026-09-16.md`;
- branch `integration/truthraw-suite-v0-16-v014-stage4-rawsensor-audit`, `tools/camera5_200mp_runtime_gate_v09.py`;
- historical launcher `TruthRawSuiteLauncherActivity.kt`, where PURE/ADVANCED/PRO share sealed-source semantics and camera input enters the staged 200 MP route.

## 4. Reconstruction research

Anchor-Constrained Local Reconstruction v0.1 is **NOT PROMOTED**. Real tele hold-out: 21,760 CFA holdouts; affine solver covered more points but was worse on directly comparable aggregate MAE/RMSE/bias and had over-optimistic uncertainty. Safety stayed closed (`target_value_used_by_solver=false`, `measured_anchors_modified=false`, `solver_applied_to_scientific_master=false`, `candidate_applied=false`, `scientific_writeback_allowed=false`). Future direction: deterministic local model selection from structural support/direction/CFA phase/uncertainty; affine is only one optional model; `no suitable model` is valid.

N2 sparse/local-spatial remains reconstruction/performance support only, never `MEASURED`. Historical v0.2.2 checkpoint: `808485b6676f23ac1a846e1eba1c625067f657bc`, 27/27 green.

## 5. Performance law

Permanent rule: **same scientific meaning, less work**.

Known validated examples: v0.2.9 direct SHA transport (~914.96 MB/RAW direct path); Pixel-Triplet Authority Encoder (`12,533,760` triplets, `37,601,280` canonical records, fallback 0, ~5.52% faster authority route); PR #125 fixed-topology exact-parity specialization; PR #126/#127 measured Scientific-Master binder at 6,144 Stage-2/RAW tile reads vs 3,072 reconstruction calls because canonical v0.2 has two tile passes.

Never trade scientific semantics for speed or invent unmeasured performance claims.

## 6. Exact Gauge Retained Artifact v0.3

Current candidate removes the second Stage-2 reread **without changing semantics** by retaining exact Float32 bit patterns of eligible gauge candidates during pass 1 and resolving the exact low-16 median from that bounded artifact.

Parity must be bit-exact for Scientific Master/hash, `gaugeMedian` bits, samples/states/counts, authority, reconstruction behavior, source/measured-anchor immutability, firewall state and deterministic repeats. Only truthful resource/timing diagnostics may differ.

Host candidate is green for configure, warnings-as-errors build, ASan/UBSan, exact parity oracle and forced budget fallback. Candidate remains **NOT PROMOTED**. Old pre-atomic retained-gauge core was removed and must not be resurrected.

Firewall remains:
`candidate_applied=false`, `creates_new_evidence=false`, `scientific_writeback_allowed=false`.

## 7. Atomic + PassArtifact architecture

Atomic is a Scientific-Master transaction/safety boundary, **not** a closed cable taxonomy.

- Admission/budget/topology failure before semantic start may use complete canonical v0.2 fallback.
- Once candidate semantic processing/tile observation starts: **no replay**.
- Later candidate failures propagate hard; never silently redo canonical after partial semantic execution.

Binding architectural law:
**The atomic core does not decide which cables D.RAW may have; it defines how a cable behaves safely once it touches Scientific Master.**

Exact Gauge must be only one registered/versioned `PassArtifact`, never a hard-coded architectural exception.

Generic registry/descriptor may define identity/version, applicability/admission, dispatch, provenance/diagnostics, atomic/no-replay lifecycle and authority classification. Artifact-specific payload/state remains open. Future optics/geometry/temporal/calibration/restoration/unknown artifacts need not use Exact Gauge's data model. **Registration never grants authority.**

## 8. Android/runtime state

Real runtime app: `suite_android` (not historical `android/app`). Target SDK 37. The current Android lineage predates completed Exact Gauge v0.3 wiring; do not call an existing APK a v0.3 device build unless native integration/tests/build actually occurred.

Known integration target: `suite_android/app/src/main/cpp/CMakeLists.txt` plus Foundation/observation JNI + Java/Kotlin path under `suite_android/app/src/main/...`.

Do not silently substitute the general preview flow. First device integration should be selectable/diagnostic Foundation/observation execution with canonical v0.2 fallback.

Android update continuity is a build invariant: preserve package/application identity, signing identity and monotonically increasing `versionCode` so validated newer APKs can update the existing D.RAW installation rather than requiring uninstall/data loss. The APK file hash may change per build; update compatibility depends on package/signing/version continuity.

## 9. Exact next work

1. Fetch live branch head; never force-reset to a remembered SHA.
2. Search existing native + Java/Kotlin tree for registry/descriptor/artifact/pass_artifact/candidate/admission dispatch.
3. Reuse an existing generic abstraction if present; do not create a parallel registry.
4. If none exists, add the smallest generic PassArtifact interface and register Exact Gauge v0.3 as one concrete type.
5. Add v0.3 admission + atomic-core sources/dependencies to `suite_android` CMake.
6. Wire Foundation/observation callsite through generic artifact dispatch, not preview.
7. Emit diagnostics: artifact id/version, candidate/canonical reads, retained bytes, budget fallback, peak bytes, admission/fallback state, semantic-start/no-replay state, authority/candidate status.
8. Preserve all firewalls and canonical v0.2 fallback.
9. Run exact parity + forced-budget fallback + all regressions.
10. Only when green, build a new APK and then real-device measure.

200 MP work is orthogonal to the active Exact-Gauge/PassArtifact work. When returning to 200 MP integration, first reuse the existing Camera-5 contracts/gates and the Free Raster distinction above rather than inventing a new 200 MP path.

## 10. Recovery pointers

For full detail/provenance read:
- `docs/handoff/DRAW_44489_RECOVERY_2026-10-03.md`
- `state/DRAW_PROJECT_STATE_2026-10-03.json`
- `docs/research/scientific-master-exact-gauge-retained-artifact-v0.3/README.md`
- `START_HERE_NEW_CHAT.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-03.md`

When the user says **44489**, restore this capsule + live repository state first, then continue directly. Do not ask the user to reconstruct prior chats when repository state is sufficient.

## 11. Continuous maintenance rule

This capsule is a **living recovery artifact**, not a one-time summary. During normal D.RAW work it must be kept current after every materially meaningful change.

Update 44489 when any of the following occurs:
- a scientific law, interpretation or calibration assumption is added, refined, falsified or restricted;
- a new measurement, hold-out, real-device result or uncertainty result changes what is known;
- a candidate is promoted, rejected, superseded or deliberately kept non-promoted;
- architecture changes, including Observation, Authority, Scientific Master, TruthNegative, Free World, PassArtifact, optics, geometry, temporal, colour, restoration or calibration contracts;
- performance work changes implementation or measured runtime/resource behavior;
- Android integration, APK identity/signing/version continuity, build/test status or real-device state changes;
- the exact next implementation step changes.

Maintenance discipline:
1. record the **scientific conclusion**, not only the code change;
2. preserve failed/falsified experiments when they constrain future work;
3. distinguish measured fact, derived fact, estimate, candidate and appearance;
4. cite/retain provenance for important numbers and test results in the larger handoff/state when the compact capsule would become too large;
5. keep the capsule concise enough for fast cold-start recovery;
6. move detail to dedicated research/state documents, but keep the binding conclusion and pointer here;
7. never rewrite history to make a newer approach look inevitable;
8. before ending a long work session or when chat context is becoming unreliable, refresh 44489 so the next chat can recover without depending on conversation memory.

Scientific knowledge should be updated alongside implementation. When a new domain materially affects semantics (photography, colour science, low-light vision, calibration, black/highlight behavior, optics/MTF/PSF, restoration, geometry/3D camera models, temporal exposure/readout, stop-motion sampling, human perception, etc.), study authoritative/primary material where practical and translate the result into contracts, provenance, uncertainty, authority rules and tests. Perceptual/appearance models may inform output but may not silently overwrite physical evidence.