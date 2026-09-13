# TruthRaw

TruthRaw is a single-frame RAW reconstruction research project and software-ISP built around one rule:

> **Measured where measured. Reconstructed where necessary. Never invented.**
>
> **Echt gemeten. Echt gereconstrueerd. Geen verzinsels.**

A second permanent law is equally important:

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## Start here

For the current project state and reading order, use:

1. `START_HERE_NEW_CHAT.md`
2. `docs/TRUTHRAW_PROJECT_MAP_2026-09-13.md`
3. `docs/audit/PROJECT_FACT_CHECK_2026-09-13.md`
4. `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-13.md`
5. `docs/CORE_VISION_HOUSE_GENEALOGY_BACKPLANE_GATEHOUSE_2026-09-13.md`
6. `state/CURRENT_CANONICAL_STATE_2026-09-13.json`
7. `docs/DOCUMENT_STATUS_INDEX_2026-09-13.md`

The previous 2026-09-10 house/state/index files and older dated snapshots remain preserved as provenance/history. They are **not** global current-state authorities after this audit unless a module-local document explicitly needs them as historical evidence.

## Scientific foundation

The original RAW/CFA and capture metadata are the **sealed original house**: immutable measurement evidence.

TruthRaw constructs a separate Scientific/Scene Master — the **new house**. Its numerical representation is not forced to inherit RAW10 range, source WhiteLevel as an output ceiling, source ISO as working scale, SDR range, integer storage, or DNG container limits.

That representational freedom never enlarges the evidence. Measured, reconstructed, censored/unknown, counterfactual, appearance and projection quantities remain distinguishable.

The normal scientific route remains single-frame: `physicalFrameCount=1` and `independentEvidenceCount=1`.

The house is not merely a runtime/performance metaphor. It is also an **epistemic and provenance architecture**: the sealed source, reconstruction rooms, Technical Backplane, counterfactual/appearance floors and export projections have different authority.

## Recovered house design rationale

The current architecture is the convergence of four historical ideas:

- **gezegelde woning / sealed house** — preserve the original RAW as immutable evidence;
- **alle vrijheid / new house** — allow a richer reconstructed scene representation without inventing stronger evidence;
- **achterkant van de foto / Technical Backplane** — bind source/master/zero-line/provenance behind the visible image;
- **tussenwoning / Gatehouse** — isolate external RAW decoding and detach it after a sealed handoff before heavy Main-House work.

The recovered chronology and current interpretation are documented in:

`docs/CORE_VISION_HOUSE_GENEALOGY_BACKPLANE_GATEHOUSE_2026-09-13.md`

Historical chat-derived rationale does not replace module-local tests/manifests. It explains why the current boundaries exist.

## TruthRange and zero-line

For positive physical scene light TruthRaw may use:

`T = log2(L/L0)`

`L0` is the chosen zero-line reference/gauge. `T=0` is not sensor black, DNG `BlackLevel`, display black, clipping, zero photons, or automatically middle grey.

The key distinction is:

- the **TruthRange address space** may be mathematically unbounded;
- the **sensor evidence** remains finite, noisy, quantized and possibly censored;
- the **gauge** chooses the coordinate origin and creates no new information.

TruthRaw therefore does **not** claim infinite physical sensor dynamic range. A clipped source sample remains a bound/censoring event rather than an exact recovered latent radiance.

Read:

- `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
- `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_EVOLUTION_2026-09-13.md`

## Scientific Master

The Scientific Master is a reconstructed camera-native RGB scientific scene state before normal display appearance and before the ordinary camera-to-XYZ/display route. It is not original CFA evidence and not a rendered preview.

A digest/identity proves content identity under its declared serialization. It does not, by itself, prove physical truth.

Do not flatten historical terminology: early Latent/Scene Master experiments explored richer scene representations, while the current project-level Scientific Master definition is specifically reconstructed camera-native RGB before the normal `camera_to_xyz()` route and before appearance.

## Color authority

Source DNG metadata can bind a reproducible source-specific color transform. That authority is `SOURCE_METADATA_BOUND`, not automatically `FULL_PHYSICAL`.

Independent camera/lens/illuminant calibration is required before stronger physical color claims are allowed. Generic three-channel RAW cannot uniquely recover arbitrary spectra/material reflectance in every scene because metamerism and unknown spectral response remain real limitations.

## The renewed house

TruthRaw separates scientific authority from execution resources.

The Building Runtime defines 12 logical rooms: Archivist, MeasurementLab, Architect, Restorer, SceneRegistry, Surveyor, ManifoldConditioning, LightingStudioCicm, RoomCapsule, Colorist, Finisher and Exporter.

A room's **truth floor** determines what it is allowed to read, reconstruct, claim or modify. A device's RAM, CPU/GPU and thermal state determine only how the same science is scheduled and buffered.

A faster phone may open more compatible rooms in parallel, retain more rebuildable cache, use larger tiles or choose an optional acceleration backend. It does **not** receive more evidence or permission to make stronger scientific claims.

The next production runtime optimization is explicit `RoomLease + CorridorToken + deterministic tile scheduler + per-room profiler + bounded buffer pools` around already-validated scientific algorithms.

## Gatehouse and external RAW

Native/direct supported RAW should take the shortest validated route. External/proprietary decode uses a controlled Gatehouse / **tussenwoning**:

`sealed source -> decode/audit/topology/provenance/resource check -> sealed handoff -> detach Gatehouse -> Main House`

Lifecycle shorthand:

`ATTACHED -> SEALED_HANDOFF -> DETACHED -> MAIN_HOUSE_ACTIVE`

Decoder support is environment/tooling, not scientific authority. Ambiguous or unsupported topology fails closed. The Gatehouse is not a second truth source and must not define TruthRaw color, calibration or Scientific Master semantics.

## Memory ownership and streaming

The renewed house uses these storage roles:

1. immutable shared identity/provenance state;
2. per-pixel scientific data, tiled/streamed where possible;
3. reproducible derived caches that may be discarded/rebuilt;
4. appearance intermediates that are tile-local/disposable;
5. export data streamed toward its destination.

Corridors should carry compact handles/tokens rather than duplicate full-frame images.

The physically exercised read optimization reduced exact tile reads from 13,824 to 7,680 and reduced source read payload/runtime on the reference Honor route. **It did not establish a memory improvement**: peak PSS rose slightly in that run.

## Output taxonomy

Keep these roles distinct:

- original Direct CFA = measured evidence;
- Scientific Master = reconstructed scientific scene state;
- reconstructed CFA DNG = `RECONSTRUCTED_CFA_PROJECTION`, not measured sensor RAW;
- Linear DNG/LinearRaw = compatibility projection from the RGB Scientific Master;
- JPEG/ARGB/HDR = appearance/presentation projection;
- rawsensor = nonstandard internal payload until an explicit ABI/provenance contract defines it.

A direct-CFA evidence repack is different from a reconstructed CFA DNG.

No export may create photons, new independent evidence, a stronger color authority, a new zero-line, or a different Scientific Master merely by serialization.

## DNG interoperability

DNG writer correctness and scientific correctness are separate gates. A valid TIFF/DNG structure can still carry a wrongly labelled scientific payload, and a correct Scientific Master can still be serialized incorrectly.

Linear DNG export should remain bounded/streaming and downstream of finalized source/master identity. Re-mosaicing reconstructed RGB merely to force another demosaic is not the preferred compatibility path.

## Counterfactual and virtual observations

Virtual EV/ISO/gain/camera views and relighting can be useful numerical/counterfactual projections of one latent scene. They do not create independent measurements.

Counterfactual illumination never becomes evidence for the captured world. Relative/Room-Lite relighting remains appearance-level unless the physical calibration/geometry/material/spectral requirements for a stronger claim are actually met.

## Technical Backplane — the digital backside

TruthRaw uses/develops a compact Technical Backplane: the **digital backside of the photo**. The visible preview/export is the front; the Backplane binds source identity, Scientific Master identity, zero-line/scene-scale, frame/evidence counts, color authority, provenance and projection status without repeating those values per pixel.

Historical Backplane work included a compact fixed serialization of roughly 180 bytes for one validated phase. That exact byte layout is module/version-specific and must not be generalized without checking its evidence.

The backplane is metadata/control-plane state, not hidden image evidence.

> **Preview is a window onto TruthRaw, never the source of TruthRaw.**

## Permanent boundaries

- original CFA/sample bytes and capture metadata remain immutable;
- `physicalFrameCount=1` and `independentEvidenceCount=1` remain single-frame evidence invariants;
- WhiteLevel/saturation is censored evidence, not exact latent radiance;
- source GainMap/corrections are applied exactly once where the validated chain requires them;
- appearance never modifies the Scientific Master;
- counterfactual simulations never become captured-world evidence;
- virtual EV/ISO views never become independent measurements;
- no generative/semantic texture enters scientific evidence;
- APK/GCam/computational-RAW content does not determine TruthRaw;
- `canonical/ptc/v1.1` means **Pure Truth Certificate**, not photon-transfer calibration;
- `FULL_PHYSICAL` remains blocked where independent sensor/color/illuminant/optics calibration is missing;
- failed/rejected experiments remain preserved;
- no open-source LICENSE is added unless explicitly chosen.

## Repository navigation

Use `docs/DOCUMENT_STATUS_INDEX_2026-09-13.md` before treating any dated README, report, audit or state file as current.

Canonical module documentation remains authoritative for the exact module/version it accompanies. Historical material is intentionally retained to preserve failures, rejected candidates, validation context, design rationale and scientific provenance.
