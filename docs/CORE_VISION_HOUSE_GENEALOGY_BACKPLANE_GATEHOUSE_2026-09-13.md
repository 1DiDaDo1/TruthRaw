# TruthRaw Core Vision — House Genealogy, Backplane and Gatehouse

**Status: ACTIVE DESIGN RATIONALE / RECOVERED HISTORY**

This document records the recovered design history behind the current TruthRaw house architecture. It exists so future work understands *why* the sealed house, new house, Gatehouse, Technical Backplane, rooms and output projections are separated.

It does **not** replace module-local manifests, tests, hashes or experiment records. Historical chat-derived statements are design-history evidence; exact implementation claims still require repository/module evidence.

## 1. The four ideas that became one architecture

The current architecture grew from four linked ideas:

1. **Gezegelde woning / sealed house** — the original RAW/CFA remains immutable evidence.
2. **Alle vrijheid / new house** — reconstruction may use a richer representation than the source container, while never inventing stronger evidence.
3. **Achterkant van de foto / Technical Backplane** — the visible image is only the front; compact provenance, identity and authority remain bound behind it.
4. **Tussenwoning / Gatehouse** — external RAW decode and its buffers/tooling are isolated from the scientific Main House and detached after a sealed handoff.

These are not independent metaphors. Together they define TruthRaw's evidence boundary, reconstruction freedom, runtime isolation and provenance model.

## 2. Recovered chronology

### 3 September 2026 — source evidence and freedom beyond the RAW container

The project already treated Direct/master CFA as the primary Bayer measurement evidence and kept computational/derived CFA paths separate.

The key new idea was then phrased as having **no need to let the original file format define the limits of the newly constructed RAW/scene**: the world/representation could be made larger, with ultra-wide numerical range and reconstructed values, provided the original source evidence was not rewritten.

This is the early root of the later rule:

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

### 3–4 September — Latent Scene / new-house separation

The project moved from “processing a RAW” toward **constructing a separate scene estimate from RAW evidence**. Early scene-master experiments used richer floating-point scene state and explicit uncertainty/support concepts.

Historical scene-master representations evolved over time. Do **not** assume every early “Scene Master” had the same channel/color-domain definition as the current Scientific Master.

### 5–8 September — sealed-house vision becomes canonical

The source RAW became the **sealed original house**: an immutable answer to “what did this camera actually record?”. The reconstructed scene became the **new house**, built separately.

Repository verification confirms commit:

`13075856895ee6815c8a72fcf733d52bad596583` — `Canonize sealed-house TruthRaw core vision`

That commit formalized:

- original RAW as sealed evidence;
- separate new-house Latent Scene Truth / Scene Master;
- source-container limits such as RAW10, WhiteLevel, ISO scale, integer range and DNG compatibility as *representation* limits rather than universal scene limits;
- ISO as immutable capture provenance but not mandatory scene identity/scale;
- `FULL_PHYSICAL` as an evidence/certification level, not permission to use a richer architecture.

### 9 September — virtual observations remain one-evidence projections

The same architecture was extended into virtual EV/ISO/gain observations: many numerical views may be constructed from one latent scene, but they do not become additional captures or independent evidence.

### 10 September — “achterkant van de foto” becomes Technical Backplane

The project explicitly separated the visible image/front from a compact technical backside. The resulting **Technical Backplane** binds scientific identity and provenance without duplicating the image.

Historical Backplane work used a compact fixed serialization of roughly 180 bytes for one validated phase. The exact byte layout belongs to that module/version, not to this project-level document.

The Backplane conceptually binds items such as:

- source identity/seal;
- Scientific Master identity/digest;
- zero-line / scene-scale binding;
- frame/evidence counts;
- color authority;
- provenance/room/projection role;
- validation/release state.

It is control-plane/provenance state, **not hidden image evidence**.

### 10–11 September — rooms and local scene support

Local illumination/geometry work evolved toward bounded **Room Capsule** data rather than keeping a complete full-world model resident. That became part of the broader house model: specialized rooms with explicit authority and disposable/rebuildable working state.

### 11 September — “tussenwoning” becomes Professional RAW Gatehouse

External/vendor/proprietary RAW decode was separated from the Main House. The intermediate building became the **Gatehouse / tussenwoning**.

Canonical intent:

`sealed source -> Gatehouse decode/audit/topology/provenance/resource check -> sealed handoff -> Gatehouse detach/free -> Main House`

Lifecycle shorthand:

`ATTACHED -> SEALED_HANDOFF -> DETACHED -> MAIN_HOUSE_ACTIVE`

The Gatehouse is not an alternative scientific station and not a second source of truth. It isolates decoder complexity, buffers, caches and tooling before the scientific pipeline starts heavy work.

### 11–13 September — full house/runtime architecture

The project formalized the logical rooms and the runtime principle that **scientific authority and execution resources are independent dimensions**. Cheap and expensive phones may use different tile sizes, cache budgets, thread counts and concurrency, while preserving the same scientific contracts and answers.

The current 12-room model is documented in `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-13.md`.

## 3. The sealed house

The sealed house is the original source evidence:

- CFA/sample bytes;
- original capture metadata;
- black/white levels and clipping/censoring state;
- exposure and ISO/gain/readout provenance;
- source DNG opcodes/GainMap where applicable;
- source color/noise metadata;
- source identity/hashes/provenance.

It is immutable evidence and remains available even after reconstruction.

The sealed house does **not** dictate:

- the Scientific Master's numeric bit depth;
- its allowed floating-point range;
- its display range;
- a `[0,1]` clamp;
- a DNG container layout;
- the final appearance;
- a requirement that the reconstructed scene continue to be “an ISO 6400 image”.

## 4. The new house and “alle vrijheid”

“Alle vrijheid” means freedom of **representation, reconstruction architecture, scene coordinates, virtual observations and appearance** above the evidence boundary.

TruthRaw may therefore use:

- floating-point/high-precision scene state;
- signed numerical values when scientifically meaningful;
- values above one;
- censor bounds and uncertainty/support;
- a larger numerical scene range than RAW10/WhiteLevel;
- reconstructed missing channels;
- future spatial/super-resolution reconstruction where evidence/model support it;
- multiple downstream compatibility and appearance projections.

It does **not** mean freedom to invent evidence.

Permanent classes remain distinct:

- `MEASURED`;
- `RECONSTRUCTED`;
- `CENSORED/BOUNDED`;
- `UNKNOWN/WEAKLY_SUPPORTED`;
- `COUNTERFACTUAL`;
- `APPEARANCE`;
- `PROJECTION`.

A larger reconstructed raster, wider dynamic coordinate space or cleaner posterior image never changes the number of independently observed photons/frames.

## 5. Representation can grow; knowledge cannot silently grow

Examples:

- A clipped highlight can receive a reconstructed estimate above source WhiteLevel, but the clipped source sample remains a censored lower-bound observation.
- A missing CFA channel can be reconstructed, but it remains reconstructed.
- A 12–13 MP measured source can support a larger reconstructed raster, but newly created sites are not relabelled as measured photosites.
- A noise-reduced posterior can look noise-free while its uncertainty remains non-zero.
- A virtual EV/ISO view can expose the same latent scene numerically but creates no new evidence.

This is why the house architecture is intentionally more permissive than a traditional RAW container and simultaneously stricter about scientific labels.

## 6. The backside of the photo

TruthRaw treats the visible image as the **front** of the product, not the whole product.

The **backside** is the compact technical state that answers questions such as:

- Which sealed source does this image descend from?
- Which Scientific Master does it represent?
- Which zero-line/scene-scale binding was used?
- Which evidence/authority level is claimed?
- Is this measured, reconstructed, counterfactual, appearance or projection output?
- Which validation/release state applies?

The Technical Backplane is the primary architectural mechanism for this binding.

Rule:

> **Preview is a window onto TruthRaw, never the source of TruthRaw.**

The same Backplane/master/source identity may support multiple front-facing projections without allowing any projection to rewrite the scientific source/master.

## 7. The Gatehouse / tussenwoning

The Gatehouse exists for external RAW ingress and resource isolation.

Responsibilities:

- decode supported external format;
- establish/verify topology;
- audit metadata/sample interpretation;
- bind provenance and decoder/environment version;
- enforce resource limits;
- produce a sealed handoff suitable for the Main House;
- fail closed on unsupported or ambiguous topology;
- release decoder-owned threads/buffers/caches after handoff where practical.

Non-responsibilities:

- defining TruthRaw color truth;
- inventing calibration;
- defining Scientific Master semantics;
- increasing evidence count;
- promoting a vendor decoder's camera-name support into scientific authority.

Native/direct supported RAW may bypass unnecessary Gatehouse work and take the shortest validated route.

## 8. Main House, rooms and corridors

The Main House owns the scientific work after the sealed evidence/handoff boundary.

Current logical room set:

1. Archivist
2. MeasurementLab
3. Architect
4. Restorer
5. SceneRegistry
6. Surveyor
7. ManifoldConditioning
8. LightingStudioCicm
9. RoomCapsule
10. Colorist
11. Finisher
12. Exporter

The house is not merely a performance metaphor. It encodes **authority separation**.

Allowed direction remains:

`Foundation -> Measurement -> Reconstruction -> Scene -> Counterfactual -> Appearance -> Projection`

Corridors should move the smallest state that preserves identity and authority. Prefer handles/tokens to repeated full-frame copies.

## 9. Runtime consequence

Execution resources never create truth authority.

A strong device may use:

- larger tiles;
- more compatible concurrent rooms;
- more cache;
- more threads;
- optional GPU/Vulkan acceleration once validated.

A constrained device may use:

- smaller tiles;
- fewer heavy operations concurrently;
- lower cache budgets;
- more aggressive eviction/reconstruction of disposable state.

Both must preserve the same scientific semantics.

The production direction remains:

`RoomLease + CorridorToken + deterministic tile scheduler + per-room profiler + bounded buffer pools`

## 10. Front/output taxonomy

The house/front metaphor must never collapse output classes:

- original Direct CFA — measured evidence;
- Direct-CFA evidence repack — measured-preserving carriage only when identity is proven;
- Scientific Master — reconstructed scientific scene state;
- reconstructed CFA DNG — `RECONSTRUCTED_CFA_PROJECTION`;
- Linear DNG/LinearRaw — compatibility projection from the reconstructed RGB Scientific Master;
- JPEG/ARGB/HDR — appearance/presentation projection.

A DNG is a container/projection. It is not automatically the house and not automatically sensor evidence.

## 11. Historical Scene Master versus current Scientific Master

Do not flatten the project's semantic evolution.

Early Latent/Scene Master experiments used richer scene representations and at times discussed device-independent/colorimetric scene state as part of the master architecture.

The current project-level Scientific Master is specifically the **reconstructed camera-native RGB scientific state before normal `camera_to_xyz()` and before appearance**.

The historical ideas remain important design provenance, but they do not override the current definition.

## 12. Zero-line / TruthRange relation

The zero-line and TruthRange are part of the new-house coordinate architecture, not a way to alter sealed evidence.

For positive scene light:

`T = log2(L / L0)`

`L0` is a gauge/reference. It is not sensor black, DNG BlackLevel, clipping, zero photons or display middle grey.

An unbounded TruthRange address space does not imply an infinite physical sensor dynamic range. Censored evidence remains censored.

See:

- `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
- `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_EVOLUTION_2026-09-13.md`

## 13. Related but separate historical strand: material/detail support

Recovered history also contains a separate line such as:

`RAW evidence -> v4.7j detail -> Unified Material Truth Limiter -> PTC v1.1 -> export`

Its important rule is compatible with the house but should not be merged into the house genealogy: measurable texture/detail support may be used without asserting a semantic identity such as “wood”, “grass” or “skin”.

This remains a claim-boundary rule, not a replacement for the reconstruction/house pipeline.

## 14. Canonical design rationale

The complete architectural reading is:

`IMMUTABLE SEALED EVIDENCE`

`-> optional controlled Gatehouse / tussenwoning`

`-> sealed handoff`

`-> Main House Measurement/Reconstruction/Scene`

`-> Scientific Master + zero-line/TruthRange/uncertainty`

`-> Technical Backplane (digital backside)`

`-> Counterfactual/Appearance`

`-> Export/front-facing projection`

The governing rule over the entire structure is:

> **Build the new scene as richly as scientifically and computationally useful; never erase which parts were measured, reconstructed, censored, unknown, counterfactual, appearance or projection.**

The house is therefore simultaneously:

- an evidence-boundary architecture;
- an authority-separation architecture;
- a provenance architecture;
- and a resource/runtime architecture.
