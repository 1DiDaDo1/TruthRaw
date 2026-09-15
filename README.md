# TruthRaw

TruthRaw is a single-frame RAW reconstruction research project and software-ISP built around one rule:

> **Measured where measured. Reconstructed where necessary. Never invented.**
>
> **Echt gemeten. Echt gereconstrueerd. Geen verzinsels.**

## Start here

For project continuity and the current reading order, use:

1. `START_HERE_NEW_CHAT.md`
2. `docs/PROJECT_HISTORY_AND_FOTOGRAAF_CONTEXT_2026-09-15.md`
3. `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md`
4. `state/CURRENT_CANONICAL_STATE_2026-09-10.json`
5. `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md`

The 2026-09-15 history document is a **continuity authority**, not a replacement for module-local scientific, CI or promotion authority. It records the full development line from the GCam/MotionCam precursor through the Scientific Master/House architecture and calibration programme to the later FotoGraaf/Camera2 acquisition layer. Recent camera-heavy conversation interruption points must not be mistaken for architectural milestones or for the whole direction of TruthRaw.

Dated state files and research README files are preserved as provenance/history. They are **not** global current-state authorities unless the relevant current status/promotion evidence says so.

## Scientific foundation

The original RAW/CFA and capture metadata are the **sealed original house**: immutable measurement evidence.

TruthRaw constructs a separate scientific Scene Master — the **new house**. Its numerical representation is not forced to inherit RAW10 range, source WhiteLevel as an output ceiling, source ISO as working scale, SDR range, integer storage, or DNG container limits.

That representational freedom never enlarges the evidence:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Measured, reconstructed, censored/unknown, counterfactual and appearance quantities remain distinguishable. A derived LinearRaw/DNG is an export or compatibility projection, not a relabelling of reconstructed values as original sensor measurements.

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

## Memory ownership model

The renewed house uses five storage classes:

1. **Immutable shared state** — one instance, referenced by handles. This includes source/master identity, scene-scale and zero-line binding.
2. **Per-pixel scientific data** — tile/stream whenever possible; never duplicate a full master merely to cross a room boundary.
3. **Reproducible derived data** — compact, downsampled and/or rebuildable caches.
4. **Appearance intermediates** — tile-local and disposable.
5. **Export data** — streamed to the destination where possible.

Room Capsule v0.1 already follows this model strongly: local geometry is compact/downsampled and tile-workspace is bounded independently of full image megapixels.

The historical canonical v4.7i public API contains full-frame vectors for decoded RAW and output RGB. Later branches must prove equivalence/integrity when changing that ownership model; historical canonical bytes are not silently rewritten.

## Technical Backplane

TruthRaw uses the Technical Backplane concept as a compact format-neutral “digital backside” that can bind source evidence, scientific master, zero-line, scene-scale, provenance and room status without repeating those values per pixel or per tile.

The backplane is not extra measurement evidence and is not hidden image content. Module-local later CI/promotion evidence determines its exact current implementation status; older bootstrap wording must not be treated as the sole current authority.

## Permanent boundaries

- original CFA/sample bytes and capture metadata remain immutable;
- physical frame count and independent evidence count remain one for the single-frame master;
- WhiteLevel clipping is censored evidence, not exact latent radiance;
- GainMap is applied exactly once where the canonical chain requires it;
- appearance never modifies the scientific master;
- counterfactual simulations never become evidence for the captured world;
- virtual EV/ISO views never become independent measurements;
- no generative/semantic texture enters scientific evidence;
- APK/GCam/computational-RAW content does not determine TruthRaw scientific authority or Direct-CFA calibration;
- historical GCam/MotionCam work remains part of project provenance as the precursor to confidence/evidence-aware mobile photography;
- `canonical/ptc/v1.1` means **Pure Truth Certificate**, not photon-transfer calibration;
- FULL_PHYSICAL remains blocked where required independent sensor/color/illuminant/optics calibration is missing;
- maximum-resolution Camera2 capability/session support is not equivalent to proof of an actually delivered maximum-resolution Direct-CFA buffer;
- no open-source LICENSE is added unless explicitly chosen.

## Repository navigation

Read `START_HERE_NEW_CHAT.md` first. Use `docs/PROJECT_HISTORY_AND_FOTOGRAAF_CONTEXT_2026-09-15.md` to understand why the recent Camera2 work exists and where it sits in the larger history. Then use the relevant status/claim/promotion documents before treating any dated README, report, audit or state file as current.

Canonical module documentation remains authoritative for the exact module/version it accompanies. Historical material is intentionally retained to preserve failures, rejected candidates, validation context and scientific provenance.
