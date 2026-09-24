# CURRENT MAIN NOTICE — 2026-09-24

**Read first:**

1. `state/CURRENT_PROJECT_STATE_2026-09-24.json`
2. `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-24.md`
3. `docs/CURRENT_RAW_DNG_INGRESS_AND_OUTPUT_PREVIEW_2026-09-24.md`
4. `docs/research/scientific-master-f64-reconstruction-v0.1/README.md`
5. `docs/research/formal-color-calibration-audit-v0.1/README.md`
6. `docs/research/open-scene-field-v0.85/README.md`
7. `docs/research/unified-output-preview-v0.1/README.md`

Active main integration:

`integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master`

Current green code-bearing checkpoint:

`675265b084577aa0c005905363ab9f6ba39dfc7f`

Final general Android validation: run `35963545440` — **SUCCESS**.

Final Unified Output Preview validation: run `35963625751` — GCC/Clang/route-contract/Android **SUCCESS**.

Current 2026-09-24 architecture checkpoint includes:

- active F64 branch-sensitive Scientific-Master reconstruction with Float32 canonical storage;
- Open Scene Field v0.85;
- TruthNegative TN-4 + local authority projection v0.4;
- local authority policy v0.86;
- Unified Output Preview v0.1 with per-output stored-orientation binding;
- DNG full scientific ingress, NEF measurement-only subset, proprietary RAW decoder-pending fail-closed handling;
- visible UI synchronized away from stale TN-3 / v0.84.2 wording;
- JPG-L scientific payload migrated from TN-3 to TN-4.

F64 + formal-color production promotion code commit:

`ad3a1f465fc77f76972c64b3a806838fbeddc310`

The user's 2026-09-21 **no-more-code-changes freeze was explicitly lifted on 2026-09-24**. Validated recommendations may now be integrated directly into the main project. Sealed source evidence and byte-frozen scientific reference modules remain immutable; new science must use explicit versioned successors rather than silently rewriting historical reference code.

The 2026-09-21 freeze notice below is preserved as historical provenance only.

---

# HISTORICAL FINAL CODE FREEZE NOTICE — 2026-09-21

**Historical reference:** `docs/handoff/TRUTHRAW_FINAL_CODE_FREEZE_V0843_2026-09-21.md`

The historical frozen code-bearing head was:

`172a100786eb18d4b08564bbfb025a44f42cfa1e`

Historical frozen APK SHA-256:

`5805d291b163d66e68d5ab98aa4d2971e38b062b724325469d6002fbd8b1917e`

---

# START HERE — TruthRaw current bootstrap

## CURRENT ACTIVE INTEGRATION — 2026-09-21

**Read first:**

1. `state/CURRENT_PROJECT_STATE_2026-09-21.json`
2. `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-21.md`
3. `docs/research/android-acceleration-v0.1/README.md`
4. `docs/research/android-acceleration-v0.1/APV_PROFESSIONAL_VIDEO_ROUTE.md`
5. `docs/research/android-acceleration-v0.1/PARALLEL_SCIENTIFIC_MASTER_PLAN.md`
6. `docs/TRUTHRAW_V0842_ADVANCED_RENDER_EDIT_FLOAT32_2026-09-21.md`

Active branch:

`integration/truthraw-suite-v0-84-2-adaptive-compute-router`

Current Android integration:

`0.51-v0.84.2-adaptive-compute-router`

Latest fully green code-bearing CI:

`35547319268` at `5a7acf28f90cac82297c2d5af3e81006c438da2f`

Current APK:
- bytes: `5,994,861`
- SHA-256: `9b4184afdec22d6313ba08d7abe8884e812e3b3cadc7f29d492685af4d820295`
- artifact id: `10617071043`

Current scientific boundary:

- v0.84 per-output RGB-channel authority map exists;
- Scientific HDR remains `BLOCKED`;
- blocked reason is now `UNKNOWN_CHANNEL_AUTHORITY_PRESENT`;
- Natural HDR remains `APPEARANCE_ONLY`;
- compute acceleration never increases scientific authority.

Current implementation additions since the old 2026-09-20 bootstrap:

- non-destructive orientation override;
- true Float32 Lightroom JPG-L RAW/Edit DNG + embedded JPEG preview;
- PURE Float32 DNG preview/self-binding improvements;
- Android background `mediaProcessing` execution + wake-lock + truthful timers/status dots;
- strict-FP native `-O2` with exact v4.7i O0/O2 signature gate;
- bounded ordered multicore executor;
- dynamic multicore Full-res Restoration;
- Android thermal + CPU/GPU resource headroom scheduling;
- runtime ADPF worker hints on API33+;
- Vulkan hardware capability discovery;
- APV hardware encoder/decoder discovery in PRO;
- ADVANCED/PRO Render/Edit Float32 DNG: developed extended-linear derivative, deterministic projected-raster SHA replay, embedded non-authority JPEG preview, tile-partition invariance and negative-headroom preservation.

Immediate continuation is documented in the 2026-09-21 handoff. Do not revert to the
old v0.83 assumption that output-channel authority is absent.

## HISTORICAL ACTIVE INTEGRATION SNAPSHOT — 2026-09-20



**Read first:**

1. `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-20.md`
2. `state/CURRENT_PROJECT_STATE_2026-09-20.json`
3. `docs/TRUTHRAW_V083_HDR_AUTHORITY_2026-09-20.md`
4. `docs/research/hdr-authority-v0.83/README.md`
5. `docs/TRUTHRAW_V082_ILLUMINATION_STATE_2026-09-20.md`
6. `docs/TRUTHRAW_V081_OUTPUT_ACUTANCE_V47K_2026-09-20.md`
7. `docs/TRUTHRAW_V080_ADAPTIVE_DETAIL_V47J_2026-09-20.md`

Current active branch:

`integration/truthraw-suite-v0-83-authority-aware-hdr`

Current app:

`0.49-v0.83-authority-aware-hdr`

Current green CI:

`35523343893`

Current APK SHA-256:

`88d8ac29b64ddef67668c026c05711974c645e3fc1ef6da4b8fb942bda4066a7`

Frozen v0.72 baseline references retained for governance/provenance:

- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-19.md`
- `state/CURRENT_PROJECT_STATE_2026-09-19.json`

Current HDR boundary:
presentation HDR remains usable but is explicitly `APPEARANCE_ONLY`. Scientific HDR is `BLOCKED / NO_PER_OUTPUT_CHANNEL_AUTHORITY` until a canonical authority/support map exists at final output RGB coordinates. CENSORED stays a bound, UNKNOWN gets no headroom, v0.82 illumination cannot create headroom.

Immediate next branch:

`integration/truthraw-suite-v0-84-output-channel-authority-map`

Next objective: propagate channel authority/support through reconstruction and resampling without altering HDR gain. Compute tiles must never become authority regions.

## TruthNegative research-branch overlay — 2026-09-19

If the checked-out branch is `research/truthnegative-v0-2-existing-house-binding`, read this overlay before the older bootstrap below:

1. `docs/DOCUMENT_STATUS_INDEX_2026-09-19_TRUTHNEGATIVE_BRANCH.md`
2. `docs/TRUTHNEGATIVE_EXISTING_HOUSE_ALIGNMENT_AUDIT_2026-09-19.md`
3. `docs/CORE_VISION_TRUTHNEGATIVE_SCIENTIFIC_NEGATIVE.md`
4. `docs/CURRENT_TRUTHNEGATIVE_ARCHITECTURE_2026-09-19.md`
5. `docs/research/truthnegative-v0.2/README.md`
6. `state/TRUTHNEGATIVE_V0_2_EXISTING_HOUSE_BINDING_STATE_2026-09-19.json`
7. `docs/handoff/TRUTHNEGATIVE_BRANCH_HANDOFF_2026-09-19.md`

Corrected branch model:

`any admitted RAW -> native ingress OR Gatehouse handoff -> Source Evidence -> Measurement/de-ISP -> Latent Camera Scene -> Scientific Master -> Dynamic Authority/Open Scene`

TruthNegative binds to that existing reconstructed state:

`Scientific Master + lineage + authority -> TruthNegative Core -> optional reconstructed sensor-negative projection`

This is research only, not canonical/main promotion. The HONOR OEM/HAL/RAW14 route remains an independent parallel research line.

This is the living bootstrap entry point for the consolidated TruthRaw research state, updated through the HONOR Camera-5 v0.20 payload-geometry result on 2026-09-17.

It is a navigation/current-state document. It does not rewrite frozen historical evidence, dated handoffs, rejected experiments or canonical bytes.

## Mandatory current reading order

Read in this order:

1. `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
2. `docs/CURRENT_SCENE_PHYSICS_RESTORATION_200MP_2026-09-16.md` — scientific background; its old open Camera-5 physical gate is superseded by the 2026-09-17 Camera-5 documents
3. `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md`
4. `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json`
5. `docs/DOCUMENT_STATUS_INDEX_2026-09-17.md`
6. `docs/DOCUMENT_STATUS_INDEX_2026-09-16.md` — preserved historical integration index; Camera-5 interpretation is superseded by the 2026-09-17 index
7. `state/CURRENT_PROJECT_STATE_2026-09-16.json`
8. `docs/PROJECT_HISTORY_AND_CHANGES_2026-09-16.md`
9. `docs/handoff/TRUTHRAW_CONSOLIDATED_HANDOFF_2026-09-16.md`
10. `docs/handoff/TRUTHRAW_DETAILED_HANDOFF_2026-09-16.md`
11. `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md`
12. only then the exact canonical/research/module documents relevant to the task.

Camera-5 detailed reading order:

- `docs/HONOR_MAGIC8_PRO_TELE_200MP_FULL_RAW_V014_2026-09-17.md`
- `docs/HONOR_CAMERA5_TWO_DOOR_AIRLOCK_V017_2026-09-17.md`
- `docs/HONOR_MAGIC8_PRO_TELE_PAYLOAD_GEOMETRY_V020_2026-09-17.md`.

Historical architecture documents remain background authorities in their own domain:

- `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md`
- `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
- `docs/CORE_VISION_UNCERTAINTY_AWARE_APPEARANCE.md`
- `docs/CORE_VISION_VIRTUAL_OBSERVATION_MANIFOLD.md`.

## One-sentence current definition

**TruthRaw seals app-visible RAW/CFA observations as immutable source evidence, reconstructs a separate uncertainty- and authority-aware Scientific Master in Free Scientific Space, and permits open-world/counterfactual/restoration/appearance projections only without upgrading what the physical evidence actually measured.**

## Permanent scientific laws

1. **Source evidence is immutable.** Original admitted bytes, decoded CFA identity and capture provenance are history, not a workspace to rewrite.
2. **Representation can exceed the source. Knowledge claims cannot exceed the evidence.** Free Scientific Space may exceed RAW code range, source CFA lattice, SDR or DNG, while authority remains bounded.
3. **Measured is not reconstructed.** `MEASURED`, `RECONSTRUCTED`, `CENSORED`, `UNKNOWN`, `COUNTERFACTUAL` and appearance/transport/restoration state remain distinguishable.
4. **Evidence count is physical.** A single physical frame remains one frame/evidence source unless another real modality is explicitly admitted; virtual observations do not create measurements.
5. **Censoring is a bound, not a guessed exact value.**
6. **Uncertainty/support is local and fail-closed.** Wrong identity, coordinates, missing runtime fields or missing semantics cannot silently widen authority.
7. **Precision is stage-specific.** Exact integer/packed evidence first; F64 where branch-sensitive science requires it; controlled F32 storage only after validation.
8. **Counterfactual state never becomes capture evidence.**
9. **Appearance/transport never writes back into science.**
10. **Restoration never overpaints valid measured support in the Scientific Master.**
11. **Compute resources never increase truth authority.**
12. **Vendor metadata and vendor-key names are observations, not calibration/semantic authority by themselves.**
13. **Rejected/failed experiments remain provenance.**

## Current project flow

`Source Evidence`
`-> acquisition/provenance + sample-topology proof`
`-> measurement/de-ISP`
`-> Scientific Master in Free Scientific Space`
`-> Dynamic Authority / uncertainty / support`
`-> Open Scene State`
`-> optional counterfactual/restoration state`
`-> appearance / HDR / transport`
`-> finite export`.

DNG, LinearRaw, reconstructed CFA, restored renders, diagnostic previews and display files are projections/compatibility products. They do not replace the Scientific Master or become original measurement evidence.

## Current Camera-5 result through v0.20

The old 2026-09-16 state said a real physical `16320x12288` Camera-5 RAW_SENSOR delivery still had to be obtained. That gate is now historical.

v0.14 proved one physical-Camera-5 app-visible `16320x12288` RAW_SENSOR delivery through logical camera 0 with:

- physical output binding to camera 5;
- exact Image/physical-result timestamp equality;
- original `Image.Plane[0]` persisted and SHA-256 sealed before interpreting contradictory pixel-mode metadata;
- app-visible source envelope size `401,080,320` bytes, rowStride `32640`, pixelStride `2`.

v0.16/v0.17 then added a two-door airlock:

`request/session fingerprint before HONOR/QTI execution`
`-> vendor pipeline`
`-> delivered HardwareBuffer/Image/physical result envelope`.

The post-HAL probe is descriptor/metadata-only and does not lock, map or write the HardwareBuffer.

v0.19 established that the tested sealed source envelope is not fully populated:

- only declared rows `0..767` contain non-zero source codes;
- rows `768..12287` are all zero;
- populated source prefix = exactly `25,067,520` bytes;
- therefore `16320*768*2 == 4080*3072*2`.

v0.20 then proved that `4080x3072` is the **only runtime-advertised standard Camera-5 RAW_SENSOR geometry** whose U16 byte count matches that populated prefix. The derived `.rawpayload` is an exact no-transform prefix copy and its SHA equals the Stage-3.6 first-band SHA.

The same bytes, indexed as `4080x3072`, produce:

- a coherent full-frame diagnostic image;
- Bayer-like 2x2 phase statistics;
- much stronger same-phase distance-2 correlations than immediate cross-colour distance-1 correlations;
- observed code range `64..1023` in the v0.20 capture.

Current bounded classification:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Descriptive shorthand:

`APP_VISIBLE_4080x3072_BAYER_LIKE_RAW_PAYLOAD_EMBEDDED_IN_16320x12288_HAL_ENVELOPE`.

Neither string proves native physical sensor geometry, untouched ADC, exact binning/remosaic mechanism or 200 MP optical resolving power.

## Current Camera-5 route clues

The untouched v0.20 request/session surface exposes candidate vendor controls including:

- `EnableIdealRAW`
- `RawCbSourceType`
- `EnableXCFAOptimization`
- `HALOutputBufferCombined`
- `EnableInsensorZoom`
- `EnableSnapshotOnlyInsensorZoom`
- `EnableMCXMasterCb`.

v0.20 changed none of them. `EnableXCFAOptimization` was visible as zero; `EnableIdealRAW` and `RawCbSourceType` were present but had null builder current/default values in the captured fingerprint.

Post-HAL route clues include HONOR `binningFactor=4`, an AEC crop near the `4080x3072` domain, ISP crop metadata in the `16320x12288` domain, a 40-byte `sensorCustomMetaData` sidecar and QTI multicamera/AF metadata. These are hypotheses/route observations, not vendor-semantic proof.

## Current next route experiment

Use v0.20 unchanged as the control.

First single-variable candidate:

`org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW`.

Rules:

1. separate branch/build;
2. runtime key lookup and type verification on this device;
3. change exactly one unknown vendor variable;
4. preserve logical0 -> physical5 -> exact 16320x12288 MAX topology;
5. preserve source-first sealing and both airlock sides;
6. repeat Stage 3.6 and Stage 3.7 unchanged;
7. compare populated byte count, geometry, hashes, result pixel mode, binning/crop metadata and full route fingerprints against v0.20.

Only after that should `RawCbSourceType`, XCFA-related controls, in-sensor zoom or other route keys be tested separately.

## Multi-camera / focus status

Camera 5 exposes Android focus-distance results and QTI AF lens-position/phase-detect telemetry. QTI multicamera sidecars are present during the tele capture.

An isolated v0.18 physical-focus branch exists, but it is deliberately not in the trusted v0.19/v0.20 parent chain because source-population/topology had higher priority.

Future multi-camera/multi-focus work must seal each physical RAW separately and keep physical-frame/evidence counts explicit. Simultaneous capture does not automatically mean independent evidence.

## Precision and zero-line continuity

The current precision direction remains:

`exact RAW integer/packed evidence`
`-> integer-exact topology/content audit`
`-> F32 only where proven safe`
`-> F64 branch-sensitive reconstruction/calibration/optimization/covariance`
`-> optional controlled F32 storage after F64 validation`.

The zero-line / TruthRange gauge remains downstream of acquisition and calibration. RAW code 0, BlackLevel, display black and the zero-line are not interchangeable concepts.

## Frozen downstream references

For the previously validated source-bound research state:

- Scientific Master SHA-256: `a86034da7b6f9663640ee3b4478ccf4294d23b720f2b087d083dca38f5fb4640`
- Dynamic Authority v1.9 SHA-256: `7678a0b145f8721347cdcc5177fedb720ba19b91bf34984a3f18bd8216408098`
- source-bound P3 transform SHA-256: `2105712a1be9950089976ba358afd4b062347680d5e6d8c500462c2e8d541533`
- reference `L0 = 0.12564234435558320`.

These are frozen downstream references for their validated source and are not automatically transferred to the newly investigated Camera-5 payload domain.

## Repository/governance rules

- Use `docs/DOCUMENT_STATUS_INDEX_2026-09-17.md` for current-vs-historical interpretation.
- Preserve the 2026-09-16 index/current supplement as historical snapshots where later Camera-5 evidence supersedes their open physical gate.
- Do not rewrite v0.14 history to pretend it already knew the v0.19/v0.20 payload topology.
- v0.15 direct-open Camera-5 remains rejected-route provenance.
- v0.18 focus remains isolated parallel research.
- APK/GCam/computational-RAW material does not determine source authority, calibration, noise, topology or colour truth.
- No silent license changes.
- `canonical/ptc/v1.1` means **Pure Truth Certificate**.

## Immediate continuation

1. keep v0.20 as immutable experimental control;
2. run the first single-variable upstream vendor-route differential only after runtime type verification;
3. continue direct `.rawpayload` bit/CFA/row/column/noise topology analysis without promoting it to native ADC geometry;
4. obtain black/white/noise/shading/colour/optics calibration separately for any readout domain that may eventually be admitted;
5. resume physical focus/multi-camera research only with per-observation sealing and explicit authority counts;
6. keep Scientific Master, Dynamic Authority, TruthRange, HDR, restoration and appearance downstream from correctly established source topology.

v0.71 CI run `35506676249` = **SUCCESS** on GCC, Clang, scientific contracts and Android. Documentation governance run `35506676288` = **SUCCESS**. Artifact ID `10604525855`; APK SHA-256 `5653f33c5bbf3263473ee89cfc70e1d0807ce0a529b258b61e3325b791700e46`.
