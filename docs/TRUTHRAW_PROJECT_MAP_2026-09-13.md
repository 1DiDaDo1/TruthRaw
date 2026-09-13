# TruthRaw Project Map — 2026-09-13

Status: current project-level navigation map for the 2026-09-13 audit branch.

This file summarizes the useful project-wide information without replacing module-local evidence, manifests, hashes, tests, or historical documents.

## 1. Non-negotiable scientific laws

1. **Measured where measured. Reconstructed where necessary. Never invented.**
2. **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
3. The original Direct-CFA/source RAW and capture metadata are immutable, sealed evidence.
4. The normal scientific path is single-frame: `physicalFrameCount=1`, `independentEvidenceCount=1`.
5. Measured, reconstructed, censored, counterfactual, appearance and projection quantities are different authority classes and must remain distinguishable.
6. Missing physical truth must not be filled by semantics, generative detail, display rendering, or a stronger label.
7. A failed gate remains failed. Thresholds or labels must not be moved merely to turn a failure into a pass.
8. Historical validated bytes/modules are not silently rewritten.
9. APK/GCam/computational-RAW content is not scientific TruthRaw evidence and must not determine TruthRaw topology, color, noise, calibration, or reconstruction authority.

## 2. Authority direction

The scientific direction is forward-only:

`Foundation -> Measurement -> Reconstruction -> Scene -> Counterfactual -> Appearance -> Projection`

No lower-authority representation is allowed to feed a stronger claim backward into an earlier floor.

A preview is a window onto TruthRaw, never the source of TruthRaw.

## 3. Source and measurement domain

The current physically exercised source family includes HONOR BKQ-N49 MotionCam DNG input. The reference physical source used by the finalized-preview/read-optimization evidence is:

- file: `IMG_260830_143012_297_014.dng`
- dimensions: 4080 x 3072
- bytes: 25,369,034
- SHA-256: `fac842110cfd5b527aae2fa46237824d444aa5701017c2b68021962ebce47da3`

The source remains evidence even after reconstruction. A derived DNG, preview, Scene Master, rawsensor payload, or virtual observation never replaces it.

Radiometric measurement ordering remains conceptually:

`CFA sample -> phase BlackLevel handling -> source normalization -> required source GainMap exactly once -> signed Stage-2/measurement-domain representation`

Negative numerical estimates and values above one may remain valid internal numerical values. Sensor clipping is treated as censored evidence, not as proof that scene radiance stops at the container limit.

## 4. Scientific Master / Scene Master

The Scientific Master is a reconstructed, camera-native RGB scientific scene representation before display appearance and before the normal camera-to-XYZ/display route. It is not original sensor evidence and is not itself a display image.

The source CFA and the Scientific Master therefore have different roles:

- Direct CFA: measured evidence.
- Scientific Master: reconstructed scientific scene state.
- Appearance image: display decision derived later.
- Export DNG/JPEG/HDR: downstream projection/compatibility representation.

The Scientific Master must retain its identity/digest through downstream projections. Export is not allowed to promote the master to a stronger evidence class.

## 5. TruthRange and the zero-line

For positive scene light:

`T = log2(L / L0)`

where `L0` is a reference/gauge. `T=0` is not sensor black, DNG `BlackLevel`, clipping, display black, zero photons, or middle grey unless an explicitly separate policy chooses such a display mapping.

Three concepts must never be conflated:

1. **TruthRange address space** — mathematically capable of extending toward `-infinity` and `+infinity`.
2. **Evidence support** — finite, uncertain and possibly censored because it comes from a real sensor observation.
3. **Gauge** — selects the coordinate origin and removes a multiplicative scene-scale ambiguity; it does not create information.

Therefore the project must not say that a physical sensor has infinite dynamic range. The unbounded property belongs to the representational coordinate space, not the measurement.

Historical evolution is documented in `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_EVOLUTION_2026-09-13.md`.

## 6. Color authority

Source DNG metadata can provide a reproducible, source-bound color transform. That is useful and valid as `SOURCE_METADATA_BOUND`, but is not equivalent to an independently measured physical camera/lens/illuminant calibration.

Current rule:

- source metadata may support source-bound preview/export color;
- `FULL_PHYSICAL` remains blocked without independent physical calibration;
- metamerism and unknown spectral sensitivity prevent a generic three-channel RAW from proving perfect spectral/material color truth in all scenes;
- camera/lens/light physical claims require appropriate independent measurements.

The late source-bound path used by the project is conceptually:

`AsShotNeutral -> CameraCalibration -> ForwardMatrix -> XYZ(D50) -> chromatic adaptation/display transform`

The exact DNG-tag semantics remain controlled by the module that parses and validates the source metadata.

## 7. House/runtime architecture

The current logical house contains 12 rooms:

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

Scientific permission and execution resources are independent dimensions. A faster device can receive larger tiles, more parallel rooms, more disposable cache, or an optional acceleration backend. It cannot receive stronger evidence or a different scientific answer merely because it is faster.

The preferred production runtime is local/tiled and lease-based:

`TileSource -> bounded room workspace -> corridor token/handle -> next compatible room -> StreamedSink`

Large pixel payloads should move as little as possible. Identity, provenance and authority should move exactly.

The next runtime optimization target is a real **Building Runtime + Room Lease + Corridor Token + per-room profiler** applied to production paths without changing scientific outputs.

## 8. Gatehouse / external RAW ingress

Native/direct supported RAW should take the shortest validated route. Formats requiring an external decoder enter through a temporary Gatehouse:

`sealed source -> Gatehouse decode/audit/topology/provenance/resource check -> persisted sealed handoff -> Gatehouse detach/free -> Main House`

The Gatehouse is a controlled ingress stage, not a second source of truth. Decoder support must fail closed when topology, metadata, or sample interpretation cannot be established safely.

## 9. Streaming and memory

The project already established the key production direction:

`TileSource -> reconstruction/statistics -> release -> exposure/scene plan -> reread tile -> reconstruction -> appearance/export -> StreamedSink -> release`

A full frame must not be materialized merely because the final image is large. Rebuildable intermediate data should be disposable. Scientific identity/backplane state should be compact and immutable.

Historical 200-MP streaming experiments demonstrated that algorithmic workspace can remain sub-megabyte even when output resolution is huge; those numbers are historical engineering measurements/estimates for specific test modules, not universal RAM guarantees.

## 10. Physically exercised read optimization

The finalized read optimization reduced exact tile reads from 13,824 to 7,680 by replacing four exact median passes with two exact radix scans while preserving the canonical sample set/median definition and Scientific Master identity.

On the physical HONOR run this corresponded to:

- tile reads: 13,824 -> 7,680 (`-44.44%`)
- RAW payload read: roughly `-30.4%`
- wall time: roughly `-11.5%`
- worker time: roughly `-10%`

Peak PSS rose slightly in that run, so **no memory-improvement claim is allowed from this measurement**.

## 11. Output taxonomy

Keep these output classes separate:

- **Original Direct CFA** — measured evidence.
- **Direct-CFA evidence repack** — same measured payload in another carriage, if byte/sample identity is preserved and proven.
- **Scientific Master** — reconstructed scene state.
- **Reconstructed CFA DNG** — reconstructed/generated mosaic projection; label `RECONSTRUCTED_CFA_PROJECTION`, never measured sensor RAW.
- **Linear Scientific Master DNG / LinearRaw DNG** — RGB compatibility projection from the Scientific Master; label as compatibility/projection, not evidence.
- **JPEG/ARGB/HDR preview** — appearance/presentation projection.
- **rawsensor** — internal/nonstandard low-level payload; it requires an explicit ABI/provenance definition before it can be called canonical.

No export may create photons, independent evidence, `FULL_PHYSICAL`, a new zero-line, or a different Scientific Master digest.

## 12. DNG interoperability boundary

The current DNG projection/export work is a downstream compatibility problem. Writer correctness must be established independently from scientific correctness.

Minimum writer gates include:

- valid TIFF/DNG structure and offsets/counts;
- deterministic payload and metadata where the contract says deterministic;
- round-trip/parser validation;
- master/source identity checks;
- correct distinction between RGB LinearRaw and CFA output roles;
- no double application of GainMap/opcodes;
- no misleading source NoiseProfile on a reconstructed RGB projection;
- no authority promotion through metadata strings.

The 2026-09-13 restore branch contains active RGB LinearRaw restoration and Android DNG export work; these remain implementation/validation work rather than a reason to redefine source evidence.

## 13. Counterfactual / virtual observations

Virtual EV, virtual ISO/gain, relighting, and virtual camera forward models can be useful numerical observations of one latent scene. They do not create independent captures.

A virtual observation may improve numerical conditioning or provide an appearance/counterfactual projection only under an explicit contract that returns uncertainty to the one original measurement likelihood.

Counterfactual illumination must never be written back as observed evidence.

## 14. Light and material claims

Illumination reasoning may use geometry, normals, visibility, BRDF/material response, source direction, falloff, spectrum and boundary illumination when those inputs are actually supported.

`Room Lite`/relative relighting is appearance-level/counterfactual. Strong physical relighting requires physical calibration and sufficient geometry/material/spectral evidence.

Semantic labels such as “wood”, “grass” or “skin” must not be used as scientific evidence for missing texture/detail. Measurable texture support may be used without claiming the semantic material identity.

## 15. Current validation posture

The strongest project posture is deliberately asymmetric:

- source identity/sealing: strong where hashes and byte paths are recorded;
- single-frame evidence accounting: strong;
- bounded streaming/read optimization: physically exercised on the reference Honor source;
- source-bound DNG color: reproducible but not independently physical;
- DNG projection/export: active validation work;
- universal external RAW support: dependent on decoder/topology validation;
- `FULL_PHYSICAL` color/light/material truth: blocked until independent calibration exists;
- Vulkan/GPU execution: future optional execution backend only, never a truth source.

## 16. Current authoritative project-level documents

Read in this order:

1. `START_HERE_NEW_CHAT.md`
2. `docs/TRUTHRAW_PROJECT_MAP_2026-09-13.md`
3. `docs/audit/PROJECT_FACT_CHECK_2026-09-13.md`
4. `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-13.md`
5. `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md`
6. `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
7. `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_EVOLUTION_2026-09-13.md`
8. `state/CURRENT_CANONICAL_STATE_2026-09-13.json`
9. `docs/DOCUMENT_STATUS_INDEX_2026-09-13.md`
10. then the module-local canonical/research documents relevant to the task.

Older dated state and architecture documents remain provenance/history and must not silently be treated as the latest global state.
