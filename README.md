# TruthRaw

TruthRaw is a single-frame RAW reconstruction research project and software-ISP built around one rule:

> **Measured where measured. Reconstructed where necessary. Never invented.**
>
> **Echt gemeten. Echt gereconstrueerd. Geen verzinsels.**

## Start here

For project continuity and the current reading order, use:

1. `START_HERE_NEW_CHAT.md`
2. `docs/TRUTHRAW_EXPERTISE_FOUNDATION_FREE_SPACE_200MP_2026-09-15.md`
3. `docs/TRUTHRAW_EXPERTISE_200MP_ROUTE_v0_8_ADDENDUM_2026-09-15.md`
4. `docs/CORE_VISION_FREE_SCIENTIFIC_SPACE_ARCHITECTURE.md`
5. `docs/PROJECT_HISTORY_AND_FOTOGRAAF_CONTEXT_2026-09-15.md`
6. `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md`
7. `state/CURRENT_CANONICAL_STATE_2026-09-10.json`
8. `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md`

The 2026-09-15 expertise/history documents are **continuity authorities**, not replacements for module-local scientific, CI or promotion authority. They record the full development line and the current corrected scientific interpretation so future chats do not bootstrap from a narrow interruption point.

Dated state files and research README files are preserved as provenance/history. They are **not** global current-state authorities unless the relevant current status/promotion evidence says so.

## Scientific foundation — Free Scientific Space

The source RAW/CFA and capture metadata are preserved as the **Source Evidence Record**.

They are not the numerical prison or architectural boundary of TruthRaw.

TruthRaw constructs a separate scientific Scene Master in a **Free Scientific Space**. Its representation is not forced to inherit RAW10 range, source WhiteLevel as an output ceiling, source ISO as working scale, SDR range, integer storage, the original CFA lattice as the only possible master lattice, DNG container limits, or float32 precision.

That representational freedom never enlarges the evidence:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Measured, reconstructed, censored/unknown, counterfactual and appearance quantities remain distinguishable. A derived LinearRaw/DNG is an export or compatibility projection, not a relabelling of reconstructed values as original sensor measurements.

The older `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md` is retained as historical provenance only. The active vision is `docs/CORE_VISION_FREE_SCIENTIFIC_SPACE_ARCHITECTURE.md`.

## Numeric precision

The scientific master is **precision-independent**. `float32`, `float64`, 128-bit floating point and arbitrary-precision arithmetic are implementation/reference profiles rather than different kinds of scientific truth.

Original integer/packed source bytes remain exact evidence. float64 is particularly appropriate for calibration fits, reductions, covariance-sensitive calculations and scientific reference paths; float32 may remain a validated high-throughput per-pixel path. Higher precision never creates new photons or stronger evidence by itself.

At the 16320x12288 Camera-5 lattice, one scalar plane is approximately 0.747 GiB in float32 and 1.494 GiB in float64, so the renewed tiled/streamed execution model remains essential.

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

Camera 5 exposes a static maximum-resolution high-resolution `RAW_SENSOR 16320x12288` route. The capability gate is proven, while the actual physical 200MP payload gate remains open until a real device capture returns the exact raw payload plus matching result/timestamp/hash evidence.

The standalone v0.7 probe contains much of the correct maximum-resolution evidence machinery, but **v0.8 is the current route authority**. v0.7 can discard `parent_logical_id` and then try to open camera 5 directly. When Android exposes camera 5 as a physical child of logical camera 0, Step 3B must preserve the real route:

`logical 0 -> OutputConfiguration physical 5 -> RAW_SENSOR 16320x12288 -> physical CaptureResult 5`

Important interpretation:

- `SENSOR_INFO_BINNING_FACTOR` is a characteristics `Size`;
- `SENSOR_RAW_BINNING_FACTOR_USED` is an optional Boolean result key;
- they must not be conflated;
- a null binning-result flag is not by itself failure for this Honor route;
- `SENSOR_INFO_LENS_SHADING_APPLIED=true` prevents an automatic claim of untouched physical optical falloff / photodiode-ADC purity.

After the physical 200MP capture succeeds, the 4080x3072, 8160x6144 and 16320x12288 modes must be characterised separately for noise, shading, colour and optical SFR/MTF before physical calibration is transferred between them.

## Memory ownership model

The renewed house uses five storage classes:

1. **Immutable shared state** — one instance, referenced by handles. This includes source/master identity, scene-scale and zero-line binding.
2. **Per-pixel scientific data** — tile/stream whenever possible; never duplicate a full master merely to cross a room boundary.
3. **Reproducible derived data** — compact, downsampled and/or rebuildable caches.
4. **Appearance intermediates** — tile-local and disposable.
5. **Export data** — streamed to the destination where possible.

Room Capsule v0.1 already follows this model strongly: local geometry is compact/downsampled and tile-workspace is bounded independently of full image megapixels.

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
- APK/GCam/computational-RAW content does not determine TruthRaw scientific authority or Direct-CFA calibration;
- historical GCam/MotionCam work remains part of project provenance as the precursor to confidence/evidence-aware mobile photography;
- `canonical/ptc/v1.1` means **Pure Truth Certificate**, not photon-transfer calibration;
- FULL_PHYSICAL remains blocked where required independent sensor/color/illuminant/optics calibration is missing;
- maximum-resolution Camera2 capability/session support is not equivalent to proof of an actually delivered maximum-resolution RAW buffer;
- no open-source LICENSE is added unless explicitly chosen.

## Repository navigation

Read `START_HERE_NEW_CHAT.md` first. The expertise foundation and its v0.8 routing addendum then give the current scientific interpretation and exact 200MP continuation point. Use `docs/PROJECT_HISTORY_AND_FOTOGRAAF_CONTEXT_2026-09-15.md` to understand why the recent Camera2 work exists and where it sits in the larger history. Then use the relevant status/claim/promotion documents before treating any dated README, report, audit or state file as current.

Canonical module documentation remains authoritative for the exact module/version it accompanies. Historical material is intentionally retained to preserve failures, rejected candidates, validation context and scientific provenance.
