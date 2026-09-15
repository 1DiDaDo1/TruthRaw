# TruthRaw

TruthRaw is a single-frame RAW reconstruction research project and software-ISP built around one rule:

> **Measured where measured. Reconstructed where necessary. Never invented.**
>
> **Echt gemeten. Echt gereconstrueerd. Geen verzinsels.**

## Start here

For project continuity and the current reading order, use:

1. `docs/TRUTHRAW_PRECISION_200MP_NEXT_CHAT_HANDOFF_2026-09-15.md` — first continuity source for the current precision/uncertainty/200MP state
2. `START_HERE_NEW_CHAT.md`
3. `docs/TRUTHRAW_EXPERTISE_FOUNDATION_FREE_SPACE_200MP_2026-09-15.md`
4. `docs/TRUTHRAW_EXPERTISE_200MP_ROUTE_v0_8_ADDENDUM_2026-09-15.md`
5. `docs/CORE_VISION_FREE_SCIENTIFIC_SPACE_ARCHITECTURE.md`
6. `docs/PROJECT_HISTORY_AND_FOTOGRAAF_CONTEXT_2026-09-15.md`
7. `docs/research/precision-independent-scientific-master-v0.1/STATUS_v0_1.json`
8. `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md`
9. `state/CURRENT_CANONICAL_STATE_2026-09-10.json`
10. `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md`

The 2026-09-15 continuity documents are **project/navigation authorities**, not replacements for module-local scientific, CI or promotion authority. They preserve the corrected development line so future chats do not bootstrap from a narrow Camera2 interruption point or stale research status.

Dated state files and research README files are preserved as provenance/history. They are **not** global current-state authorities unless the relevant current status/promotion evidence says so.

## Scientific foundation — Free Scientific Space

The source RAW/CFA and capture metadata are preserved as the **Source Evidence Record**.

They are not the numerical prison or architectural boundary of TruthRaw.

TruthRaw constructs a separate scientific Scene Master in a **Free Scientific Space**. Its representation is not forced to inherit RAW10 range, source WhiteLevel as an output ceiling, source ISO as working scale, SDR range, integer storage, the original CFA lattice as the only possible master lattice, DNG container limits, or float32 precision.

That representational freedom never enlarges the evidence:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Measured, reconstructed, censored/unknown, counterfactual and appearance quantities remain distinguishishable. A derived LinearRaw/DNG is an export or compatibility projection, not a relabelling of reconstructed values as original sensor measurements.

The older `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md` is retained as historical provenance only. The active vision is `docs/CORE_VISION_FREE_SCIENTIFIC_SPACE_ARCHITECTURE.md`.

## Numeric precision — current research result

The scientific master is **precision-independent**. `float32`, `float64`, 128-bit floating point and arbitrary-precision arithmetic are implementation/reference profiles rather than different kinds of scientific truth.

Original integer/packed source bytes remain exact evidence.

Current measured policy is stage-specific:

- simple Stage-2 black/white normalization is an F32 hot-path candidate in the tested 4080x3072 scope, with F64 reference;
- the tested HONOR DNG GainMap family is also an F32 hot-path candidate with F64 reference;
- **branch-sensitive v4.7i-class reconstruction requires F64 compute as the current scientific reference** because tiny F32 differences can cross the direction threshold and amplify strongly in reconstructed channels;
- calibration fits, reductions, optimization and covariance use F64 as default reference;
- after F64 reconstruction compute, F32 Scientific-Master storage has a retained numerical PASS in the eight-file v0.4 4080x3072 scope;
- arbitrary/high precision remains an offline/reference validator.

The authoritative v0.4 eight-file result is `3545` direction differences, `0` green-clamp differences, `0` colour-clamp differences and `0` measured-channel violations across `50,135,040` direction-checked sites. The maximum local F32-vs-F64 reconstruction difference was `0.03754056890225277`; the maximum tested F64-compute -> F32-storage absolute error was only `5.960464477539063e-08`.

Older `3549 / 44 / 2580` locator values are explicitly superseded provisional output and must not be treated as retained gate authority.

At the 16320x12288 Camera-5 lattice, one scalar plane is approximately 0.747 GiB in F32 and 1.494 GiB in F64, so the tiled/streamed execution model remains essential.

## Current uncertainty blocker

Canonical local uncertainty is **not yet bound** into the mixed-precision Scientific-Master conversion.

The frozen v5.0g/p1 model requires 18 historically defined features. The canonical manifest records `uncertainty_core_v5_0g.py`, but the exact historical extractor is absent from the current tree and has not been recovered from the expected historical paths checked so far. The current reconstruction trace is not sufficient to recreate all 18 feature semantics with proven parity and no hidden-CFA leakage.

Current blocker:

`OPEN_NEEDS_EXACT_V5G_FEATURE_EXTRACTOR_RECOVERY_OR_HASH_VERIFIED_EQUIVALENT_FEATURE_DEFINITION`

Do not invent an approximate feature adapter. v0.7 remains a synthetic uncertainty-relative storage gate; v0.8 remains correctly fail-closed with canonical uncertainty unbound; v0.9 provides a model-spec/runtime research bridge but not a canonical feature binding.

This blocker does not prevent the separate physical 200MP capture programme from continuing.

## TruthRange and zero-line

For positive physical scene light TruthRaw may use:

`T = log2(L/L0)`

`L0` is the chosen zero-line reference. `T=0` is not sensor black, DNG `BlackLevel`, display black, clipping or “zero photons”.

The TruthRange coordinate may be unbounded as a representation, while every real capture supplies only finite evidence. Signed scene-linear estimates remain a separate companion representation; a negative numerical estimate is not negative physical light.

The detailed domain authority remains `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`.

## The renewed house

TruthRaw separates scientific authority from execution resources.

The Building Runtime defines logical rooms including Archivist, Measurement Lab, Architect, Restorer, Scene Registry, Surveyor, Manifold Conditioning, Lighting Studio/CICM, Room Capsule, Colorist, Finisher and Exporter.

A room's **truth floor** determines what it is allowed to read, claim or modify. A phone's RAM, CPU, GPU and thermal state determine only how much execution space the room receives.

A faster phone may open more compatible rooms in parallel, retain more rebuildable caches, use larger tiles or choose an optional acceleration backend. It does **not** receive more evidence or permission to make stronger scientific claims.

Corridors carry compact handles, provenance and authority, not duplicate full-frame images.

## FotoGraaf in context

Three meanings must remain separate:

- **Fotograafkamer**: the earlier local photographic/counterfactual light concept that matured into CICM/Room Capsule/appearance floors;
- **broad FotoGraaf**: the larger photography + metrology + physical-calibration trajectory;
- **Android FotoGraaf**: the later Camera2 acquisition/metrology app that proves physical camera/lens/mode/sample-domain evidence before TruthRaw reconstruction.

Android FotoGraaf is a consequence of the older TruthRaw requirement that the origin of scientific evidence must be provable. It is not the origin or total scope of TruthRaw.

## 200MP Camera-5 programme

Camera 5 exposes a static maximum-resolution high-resolution `RAW_SENSOR 16320x12288` route. The capability/session gate is proven, while the actual physical 200MP payload gate remains open until a real device capture returns the exact raw payload plus matching result/timestamp/hash evidence.

The standalone v0.7 probe contains much of the correct maximum-resolution evidence machinery, but **v0.8 is the current route authority**. v0.7 can discard `parent_logical_id` and then try to open camera 5 directly. When Android exposes camera 5 as a physical child of logical camera 0, Step 3B must preserve the real route:

`logical 0 -> OutputConfiguration physical 5 -> RAW_SENSOR 16320x12288 -> physical CaptureResult 5`

Important interpretation:

- `SENSOR_INFO_BINNING_FACTOR` is a characteristics `Size`;
- `SENSOR_RAW_BINNING_FACTOR_USED` is an optional Boolean result key;
- they must not be conflated;
- a null binning-result flag is not by itself failure for this HONOR route;
- `SENSOR_INFO_LENS_SHADING_APPLIED=true` prevents an automatic claim of untouched physical optical falloff / photodiode-ADC purity.

After physical 200MP capture succeeds, 4080x3072, 8160x6144 and 16320x12288 must be characterised separately for noise, shading, colour and optical SFR/MTF before physical calibration is transferred between them. The mixed-precision gate must also be repeated on the physically proven 200MP source.

## Memory ownership model

The renewed house uses five storage classes:

1. **Immutable shared state** — one instance, referenced by handles. This includes source/master identity, scene-scale and zero-line binding.
2. **Per-pixel scientific data** — tile/stream whenever possible; never duplicate a full master merely to cross a room boundary.
3. **Reproducible derived data** — compact, downsampled and/or rebuildable caches.
4. **Appearance intermediates** — tile-local and disposable.
5. **Export data** — streamed to the destination where possible.

Historical canonical public APIs may contain full-frame vectors. Later branches must prove equivalence/integrity when changing that ownership model; historical canonical bytes are not silently rewritten.

## Technical Backplane

TruthRaw uses the Technical Backplane concept as a compact format-neutral “digital backside” that can bind source evidence, scientific master, zero-line, scene-scale, provenance and room status without repeating those values per pixel or per tile.

The backplane is not extra measurement evidence and is not hidden image content. Module-local later CI/promotion evidence determines its exact current implementation status; older bootstrap wording must not be treated as the sole current authority.

## Permanent boundaries

- original CFA/sample bytes and capture metadata remain immutable evidence;
- preserving that evidence does **not** constrain the Free Scientific Space to the RAW container;
- physical frame count and independent evidence count remain one for the single-frame master unless an explicitly separate acquisition programme adds exposures;
- WhiteLevel clipping is censored evidence, not exact latent radiance;
- GainMap is applied exactly once where the canonical chain requires it;
- appearance never modifies the scientific master;
- counterfactual simulations never become evidence for the captured world;
- virtual EV/ISO views never become independent measurements;
- no generative/semantic texture enters scientific evidence;
- numeric precision is not evidence authority;
- APK/GCam/computational-RAW content does not determine TruthRaw Direct-CFA calibration authority;
- historical GCam/MotionCam work remains part of project provenance as the precursor to confidence/evidence-aware mobile photography;
- `canonical/ptc/v1.1` means **Pure Truth Certificate**, not photon-transfer calibration;
- FULL_PHYSICAL remains blocked where required independent sensor/color/illuminant/optics calibration is missing;
- maximum-resolution Camera2 capability/session support is not equivalent to proof of an actually delivered maximum-resolution RAW buffer;
- no open-source LICENSE is added unless explicitly chosen.

## Repository navigation

Read `docs/TRUTHRAW_PRECISION_200MP_NEXT_CHAT_HANDOFF_2026-09-15.md` and `START_HERE_NEW_CHAT.md` first. Use the expertise foundation and v0.8 routing addendum for the 200MP continuation point, and the precision module `STATUS_v0_1.json` for the machine-readable research boundary. Use `docs/PROJECT_HISTORY_AND_FOTOGRAAF_CONTEXT_2026-09-15.md` to understand where FotoGraaf sits in the larger history.

Canonical module documentation remains authoritative for the exact module/version it accompanies. Historical material is intentionally retained to preserve failures, rejected candidates, validation context and scientific provenance.
