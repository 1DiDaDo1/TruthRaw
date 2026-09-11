# TruthRaw

TruthRaw is a single-frame RAW reconstruction research project and software-ISP built around one rule:

> **Measured where measured. Reconstructed where necessary. Never invented.**
>
> **Echt gemeten. Echt gereconstrueerd. Geen verzinsels.**

## Start here

For the current project state and reading order, use:

1. `START_HERE_NEW_CHAT.md`
2. `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md`
3. `state/CURRENT_CANONICAL_STATE_2026-09-10.json`
4. `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md`

Dated state files and research README files are preserved as provenance/history. They are **not** global current-state authorities unless the current document-status index explicitly says so.

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

TruthRaw now separates scientific authority from execution resources.

The Building Runtime defines 12 logical rooms: Archivist, Measurement Lab, Architect, Restorer, Scene Registry, Surveyor, Manifold Conditioning, Lighting Studio/CICM, Room Capsule, Colorist, Finisher and Exporter.

A room's **truth floor** determines what it is allowed to read, claim or modify. A phone's RAM, CPU, GPU and thermal state determine only how much execution space the room receives.

A faster phone may open more compatible rooms in parallel, retain more rebuildable caches, use larger tiles or choose an optional acceleration backend. It does **not** receive more evidence or permission to make stronger scientific claims.

Corridors carry compact handles, provenance and authority, not duplicate full-frame images.

## Professional RAW ingress

TruthRaw does **not** treat “the file can be opened” as equivalent to “the sensor measurement is scientifically understood”, and it does not currently claim universal RAW support.

The existing Tile-Native DNG Source v0.1 remains a deliberately strict native low-memory path. Broader professional-camera support is added through a format-neutral decoder-adapter boundary that keeps three decisions separate:

1. container/codec recognition;
2. byte-to-sample decode certification;
3. measurement-topology/scientific admission.

The original CR3/NEF/ARW/RAF/RW2/ORF/PEF/3FR/IIQ/DNG/etc. container remains sealed evidence. Decoder output is derived from that object and may not replace the original evidence identity. Filename extension alone never certifies evidence.

Bayer, X-Trans, monochrome, layered/Foveon, linear-RGB, multi-shot composite and computational RAW are distinct measurement topologies. A generic decoder may expose samples, but topology-specific downstream science must be independently certified before those samples can enter the single-frame scientific master.

The active research contract is `docs/research/professional-raw-ingress-v0.1/README.md`. Its format-family registry is a target map, not a supported-camera marketing list.

## Memory ownership model

The renewed house uses five storage classes:

1. **Immutable shared state** — one instance, referenced by handles. This includes source/master identity, scene-scale and zero-line binding.
2. **Per-pixel scientific data** — tile/stream whenever possible; never duplicate a full master merely to cross a room boundary.
3. **Reproducible derived data** — compact, downsampled and/or rebuildable caches.
4. **Appearance intermediates** — tile-local and disposable.
5. **Export data** — streamed to the destination where possible.

Room Capsule v0.1 already follows this model strongly: local geometry is compact/downsampled and tile-workspace is bounded independently of full image megapixels.

The current canonical v4.7i public API still contains full-frame vectors for decoded RAW and output RGB. Replacing those ownership points with a `TileSource -> persistent master handle -> StreamedSink` path is the next explicit production-memory migration. Until validated, the existing canonical v4.7i bytes remain unchanged.

Professional RAW decoder adapters must also report their resident/scratch memory shape. A weak phone may reject a full-frame decoder lease that a stronger phone can admit, but device class never changes the evidence class. The preferred endpoint is tile/streaming decode so the same certified science can run under different resource envelopes.

## Technical Backplane

TruthRaw is developing a compact format-neutral **Technical Backplane**: a small shared “digital backside” that can bind source evidence, scientific master, zero-line, scene-scale, provenance and room status without repeating those values per pixel or per tile.

The backplane is not extra measurement evidence and is not hidden image content. DNG carriage is deferred until interoperability is explicitly validated.

Verbose decoder identity/version/codec metadata belongs behind immutable provenance handles/registries; it must not cause the fixed backplane to be duplicated per room or tile.

## Permanent boundaries

- original CFA/sample bytes and capture metadata remain immutable;
- the original source container remains immutable even when a decoder produces a derived sample view;
- filename extension or successful parsing alone never certifies a RAW evidence class;
- physical frame count and independent evidence count remain one for the single-frame master;
- WhiteLevel clipping is censored evidence, not exact latent radiance;
- GainMap is applied exactly once where the canonical chain requires it;
- appearance never modifies the scientific master;
- counterfactual simulations never become evidence for the captured world;
- virtual EV/ISO views never become independent measurements;
- no generative/semantic texture enters scientific evidence;
- computational/multi-shot RAW cannot be silently relabelled as Direct-CFA single-frame evidence;
- APK/GCam/computational-RAW content does not determine TruthRaw;
- `canonical/ptc/v1.1` means **Pure Truth Certificate**, not photon-transfer calibration;
- FULL_PHYSICAL remains blocked where independent sensor/color/illuminant/optics calibration is missing;
- no open-source LICENSE is added unless explicitly chosen.

## Repository navigation

Use `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md` before treating any dated README, report, audit or state file as current.

Canonical module documentation remains authoritative for the exact module/version it accompanies. Historical material is intentionally retained to preserve failures, rejected candidates, validation context and scientific provenance.
