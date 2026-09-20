# START HERE — TruthRaw current bootstrap

## CURRENT ACTIVE INTEGRATION — 2026-09-20

**For a new chat, read this section first, then open:**

1. `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-19.md`
2. `state/CURRENT_PROJECT_STATE_2026-09-19.json`
3. `docs/TRUTHRAW_FULL_GENEALOGY_RECOVERY_AUDIT_2026-09-19.md`
4. `docs/research/multivendor-raw-source-adapter-v0.1/README.md`
5. `docs/research/nikon-nef-radiometric-admission-v0.59/README.md`

Current active branch:

`integration/truthraw-suite-v0-67-fullres-restoration`

Current app:

`0.32-v0.67-fullres-restoration`

Current product direction: **one app, RAW-file import first, camera capture second; both converge at `SEALED_SOURCE_ADMISSION`.**

Current proprietary-RAW progress: DNG uses the generic RAW adapter ABI; Nikon NEF has the first strict real proprietary sample decoder and v0.59 exact-scope black/saturation gate. **v0.66 restores TruthNegative TN-2, Open-World/Scene Physics and Dynamic Authority into the current app. v0.67 adds full-resolution retreatable Restoration: a source-resolution camera-native Float32 derivative plus a per-pixel role mask, with exact Scientific-Master replay and no scientific writeback. The validated PURE writer remains `TRUTHRAW_PURE_SELF_BINDING_V0_63`.** The old 16-bit Linear DNG remains compatibility-only. Scientific Master admission for NEF remains blocked until noise/uncertainty, source-bound color and held-out validation are separately closed.

Current v0.59 host CI: run `35456672651` = **SUCCESS** on GCC + Clang.

Current v0.59 Android CI: run `35456737363` = **SUCCESS**.

Historical v0.60 PURE CI: run `35458367764` = **SUCCESS** on host GCC, host Clang and Android.\n\nCurrent writer contract is `TRUTHRAW_PURE_SELF_BINDING_V0_63`. New PURE outputs use `*_truthraw_pure_float32_v0_63.dng`. Read `docs/TRUTHRAW_V063_CRC_UI_BRANDING_2026-09-19.md` before changing CRC, self-binding or launcher semantics.

v0.64 CI run `35471926706` = **SUCCESS** on host GCC, host Clang and Android. Artifact ID `10593425008`; extracted APK SHA-256 `8d5b0e54517fe29c7e26eade989652aba5292fd498308d41d11dd6691d662441`. Read `docs/TRUTHRAW_V064_ADVANCED_DERIVATIVE_2026-09-19.md`.

v0.65 CI run `35473170260` = **SUCCESS** on host GCC, host Clang and Android. Artifact ID `10593941511`; extracted APK SHA-256 `9557aaba0222ebed23be961b3865fe5004eaa0af5589de654439eacb766bf21d`. v0.65 changes only compact-phone UI/insets/icon presentation; read `docs/TRUTHRAW_V065_UI_ICON_POLISH_2026-09-19.md`.

v0.66 CI run `35497663010` = **SUCCESS** on GCC, Clang, scientific contracts and Android. TruthNegative/Open-World/Dynamic Authority are active in the current app.

v0.67 CI run `35498407249` = **SUCCESS** on GCC, Clang, scientific contracts and Android. Artifact ID `10601092611`; extracted APK SHA-256 `1ce0e75d409f3c2c8e5f6886e43cb39796dae58fc406d9aa3b78bc4406783c94`. Read `docs/TRUTHRAW_V067_FULLRES_RESTORATION_2026-09-20.md` before changing full-resolution restoration semantics.\n\nv0.63 CI run `35469414682` = **SUCCESS** on host GCC, host Clang and Android. Artifact ID `10592980623`; extracted APK SHA-256 `82dc3103b4e945e129fa5ea38fe717a342d459d1f6b71e4895bab9e00282d173`.

v0.62 CI run `35466767939` = **SUCCESS** on host GCC, host Clang and Android. Artifact ID `10591985003`; extracted APK SHA-256 `36e468a8a7e5449d6c649006a109f3a9163dbd5f3f746ed0c7cf521a4f88c447`. Read `docs/TRUTHRAW_V061_PURE_SELF_BINDING_DNG_2026-09-19.md` before changing the PURE export metadata contract.\n\nHistorical/current v0.59 Android artifact: `truthraw-suite-v0-59-nef-radiometric-admission-debug-arm64`, artifact ID `10588558376`, archive SHA-256 `4de4346d18fc9c3477ed5740d08f400c3e96500b89d9626a9a553e1cad0793b8`.

Do not use the older TruthNegative or 2026-09-16/17 bootstrap below as the first current-state interpretation. Those sections remain retained provenance/background.

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