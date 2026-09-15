# START HERE — TruthRaw current bootstrap

This file originated as the authoritative renewed-house bootstrap on 2026-09-10. A **2026-09-15 continuity + expertise + precision layer** now prevents recent FotoGraaf/Camera2 interruption points, older metaphors, stale precision locator output, or source-container limits from being mistaken for the whole direction of TruthRaw.

## Mandatory reading order

1. `docs/TRUTHRAW_PRECISION_200MP_NEXT_CHAT_HANDOFF_2026-09-15.md` — **first continuity source for the next chat**; exact mixed-precision state, resolved discrepancy, uncertainty blocker, Free Scientific Space and the physical 200MP continuation order
2. `docs/TRUTHRAW_EXPERTISE_FOUNDATION_FREE_SPACE_200MP_2026-09-15.md` — scientific expertise foundation; Free Scientific Space; colour/light/calibration/sharpness/restoration; float precision; 200MP Step 3B
3. `docs/TRUTHRAW_EXPERTISE_200MP_ROUTE_v0_8_ADDENDUM_2026-09-15.md` — mandatory routing correction: preserve logical-0 -> physical-5 parentage; do not resume from v0.7 direct-open assumptions
4. `docs/CORE_VISION_FREE_SCIENTIFIC_SPACE_ARCHITECTURE.md` — current core vision
5. `docs/PROJECT_HISTORY_AND_FOTOGRAAF_CONTEXT_2026-09-15.md` — historical/project-wide continuity; this does not replace module-local scientific authority
6. `docs/research/precision-independent-scientific-master-v0.1/STATUS_v0_1.json` — current machine-readable precision/uncertainty research state
7. `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md`
8. `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
9. `state/CURRENT_CANONICAL_STATE_2026-09-10.json`
10. `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md`
11. only then: the current claim/status documents and canonical/research module documents relevant to the task

`docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md` is retained as **historical provenance only**. Its old sealed-house wording is no longer the active architectural metaphor.

Do **not** bootstrap from `CURRENT_CANONICAL_STATE_2026-09-06.json`, `...2026-09-08.json`, `...2026-09-09.json`, or `docs/PROJECT_STATE_AUDIT_2026-09-08.md`. Those are preserved historical snapshots.

Do **not** infer global project direction merely from the last recoverable Camera2/FotoGraaf experiment. The recent camera-heavy conversations are execution history inside the newer acquisition/metrology layer. Their interruption points are not architectural milestones.

## Non-negotiable scientific rules

The source RAW/CFA + capture metadata are an immutable **Source Evidence Record**. TruthRaw reconstructs a separate **Free Scientific Scene Space**.

The source evidence must be preserved, but the reconstruction is **not sealed inside it**.

TruthRaw's scientific representation is not required to inherit RAW bit depth, WhiteLevel as a master ceiling, source ISO as a scene scale, source pixel lattice, source camera colour basis, `[0,1]`, DNG limits, SDR/HDR display limits, or float32 precision.

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Measured, reconstructed, censored/unknown, counterfactual and appearance data remain distinct. `physicalFrameCount=1` and `independentEvidenceCount=1` remain the single-frame evidence invariants unless an explicitly separate physical-capture programme adds independent exposures.

For positive physical light, TruthRange may use `T = log2(L/L0)`. The zero-line `L0` is a gauge/reference; it is not sensor black, absolute darkness, DNG BlackLevel or clipping. Signed scene-linear estimates remain distinct from the positive-light log coordinate.

Source ISO/shutter remain immutable capture provenance. Virtual EV/ISO reparameterization does not create information. Counterfactual illumination/capture may create hypothetical measurements only inside the explicitly counterfactual world; it never retroactively creates evidence.

Appearance must never modify the scientific master. DNG/LinearRaw/export is a compatibility/presentation projection, not the scientific master.

APK/GCam/computational-RAW content must not determine TruthRaw Direct-CFA evidence, calibration, topology, colour or noise authority. GCam/MotionCam material may be retained as **historical precursor/provenance** and separately classified input evidence, but it cannot silently redefine physical calibration authority.

`canonical/ptc/v1.1` is **Pure Truth Certificate**. Do not infer photon-transfer calibration from that path name.

No silent LICENSE. Preserve failed/rejected experiments and their provenance.

## Precision-independent master rule — current measured policy

Do not define TruthRaw scientific truth as `float32`, `float64`, or any other one datatype.

The master contract is precision-independent. float32, float64/double, 128-bit floating point and arbitrary-precision arithmetic are implementation/reference profiles.

The current measured policy is **stage-specific**:

- preserve original integer/packed RAW bytes exactly;
- F32 is a tested hot-path candidate for simple black/white Stage-2 normalization in the tested 4080x3072 scope;
- F32 is a tested hot-path candidate for the tested HONOR DNG GainMap family, with F64 reference;
- **F64 compute is required as the scientific reference for the tested v4.7i-class branch-sensitive reconstruction** because tiny F32 errors can cross the `0.72` direction threshold and amplify locally;
- F64 is the default reference for calibration fits, reductions, optimization and covariance-sensitive calculations;
- after F64 reconstruction compute, F32 Scientific-Master storage has a tested numerical PASS in the retained v0.4 eight-file 4080x3072 scope;
- 128-bit/arbitrary precision remains a selective offline/reference validator;
- higher numeric precision never creates new photographic evidence.

The authoritative eight-file v0.4 aggregate is `3545` direction differences, `0` green-clamp differences, `0` colour-clamp differences, and `0` measured-channel violations. Older `3549 / 44 / 2580` values are superseded provisional locator output, not retained gate authority. See `DISCREPANCY_AND_UNCERTAINTY_BINDING_RESOLUTION_v0_9.md`.

At 16320x12288, one scalar plane is approximately 0.747 GiB in float32, 1.494 GiB in float64 and 2.988 GiB with 16-byte values. Therefore high-precision 200MP work must remain tiled/streamed rather than relying on repeated full-frame allocations.

## Current uncertainty boundary

The v0.9 native uncertainty/model-spec research runtime exists, but **canonical per-pixel uncertainty is not yet bound to the mixed-precision Scientific Master conversion**.

The frozen v5.0g/p1 model requires 18 historically defined features. The canonical manifest records the historical extractor `uncertainty_core_v5_0g.py`, but the exact extractor is absent from the current tree and direct recovery attempts at known historical expected paths have not recovered it.

The current reconstruction trace is insufficient to recreate all 18 features with proven identical hidden-CFA/support-spread semantics. Therefore the current blocker is:

`OPEN_NEEDS_EXACT_V5G_FEATURE_EXTRACTOR_RECOVERY_OR_HASH_VERIFIED_EQUIVALENT_FEATURE_DEFINITION`

Do **not** invent an approximate feature adapter. Until exact feature semantics are recovered and verified, v0.8 remains correctly fail-closed with canonical uncertainty unbound. This blocker does not prevent the independent physical 200MP acquisition experiment from continuing.

## Renewed house execution rules

The Building Runtime owns orchestration; algorithms retain their own scientific contracts.

- **Truth floor** controls epistemic permission.
- **Resource profile** controls only RAM/CPU/GPU/tile/cache execution.
- Cheap phones may run one heavy room at a time with small tiles and disposable caches.
- Strong phones may run more compatible rooms in parallel, use larger tiles/caches and optional acceleration.
- Hardware capability never upgrades scientific claims.
- Corridors carry handles/provenance rather than duplicate full-frame payloads.
- Source/master/zero-line/scene-scale identity belongs to one shared provenance binding, not per-pixel duplication.
- Rebuildable caches may be evicted; evidence/master identity may not be silently rewritten to satisfy memory pressure.
- Resource policy may be re-derived between room operations, never halfway through an indivisible scientific operation.

## FotoGraaf naming rule

Do not conflate these three layers:

1. **Fotograafkamer** — historical/counterfactual photographic-light concept; mature descendants include CICM/Room Capsule/Lighting Studio/appearance floors.
2. **Broad FotoGraaf** — photography + metrology + physical calibration trajectory connecting controlled capture to TruthRaw.
3. **Android FotoGraaf** — concrete Camera2 acquisition/metrology application before reconstruction; proves physical camera/lens/mode/sample-domain evidence and collects data for calibration.

Android FotoGraaf is a subsystem of the larger trajectory, not a replacement for the Scientific Master, House architecture or calibration programme.

## 200MP current boundary

Camera 5 exposes a real static Camera2 maximum-resolution high-resolution `RAW_SENSOR 16320x12288` route. The static capability/session gate is proven; the physical capture gate remains open until a real payload plus matching physical `TotalCaptureResult` and hash/timestamp evidence are returned.

The standalone v0.7 probe contains much of the correct maximum-resolution evidence machinery, but **v0.8 is the route authority** because it preserves the logical-parent/physical-child relationship. If camera 5 is exposed as physical child of logical camera 0, Step 3B must use `logical 0 -> OutputConfiguration physical 5 -> matching physical CaptureResult`, not assume `openCamera("5")` is valid.

`SENSOR_RAW_BINNING_FACTOR_USED` is an optional Boolean result key and must not be confused with `SENSOR_INFO_BINNING_FACTOR` (`Size`). A null binning-result flag is not by itself failure on this Honor route.

A successful 16320x12288 capture proves an **app-visible maximum-resolution RAW_SENSOR measurement**. Because `SENSOR_INFO_LENS_SHADING_APPLIED=true`, it does not automatically prove untouched photodiode/ADC output or untouched physical optical falloff.

After capture proof, calibrate 4080x3072 / 8160x6144 / 16320x12288 separately for noise, shading, colour and optics before transferring physical models between modes. The complete mixed-precision gate must also be rerun on the physically proven 200MP source before any 200MP precision promotion.

## Current implementation boundary

Building Runtime v0.1 and Room Capsule v0.1 are integrated on the renewed-house lineage. CICM v1 and Manifold Conditioning v1 remain bounded research components in that lineage.

The canonical v4.7i/v4.7j historical APIs and later streaming branches must be interpreted through their own module-local evidence. Do not silently rewrite historical canonical bytes merely to conform to newer ownership architecture.

Recent Android acquisition work has stronger physical-route and raw-buffer evidence than the 2026-09-10 bootstrap originally contained. That later acquisition evidence does not by itself promote a calibration domain or redefine the reconstruction core.

## One-sentence definition

**TruthRaw preserves exact source measurement evidence, reconstructs an uncertainty-aware Free Scientific Scene Space whose numeric/spatial/colour representation is not restricted by the RAW container, and uses FotoGraaf as the acquisition/metrology layer that proves and calibrates the physical origin of that evidence without converting inference into invented measurement.**
