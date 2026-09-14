# TruthRaw Project Map — 2026-09-13

Status: current project-level navigation map for the 2026-09-13 audit branch, with the active 2026-09-14 zero-line empirical overlay recorded in section 19.

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

Historical note: early Latent/Scene Master work explored richer scene representations and at times colorimetric/device-independent state. That history is design provenance; it does not silently redefine the current Scientific Master, which is camera-native reconstructed RGB before the normal `camera_to_xyz()` route and before appearance.

## 5. TruthRange and the zero-line / nul-lijn

For positive scene light:

`T = log2(L / L0)`

where `L0` is a reference/gauge. `T=0` is not sensor black, DNG `BlackLevel`, clipping, display black, zero photons, or middle grey unless an explicitly separate policy chooses such a display mapping.

Three concepts must never be conflated:

1. **TruthRange address space** — mathematically capable of extending toward `-infinity` and `+infinity`.
2. **Evidence support** — finite, uncertain and possibly censored because it comes from a real sensor observation.
3. **Gauge** — selects the coordinate origin and removes a multiplicative scene-scale ambiguity; it does not create information.

The historical house intent is explicitly two-sided: the reconstructed/new house may continue without a finite representational ceiling above the nul-lijn and without a finite representational floor below it.

Therefore the project must not say that a physical sensor has infinite dynamic range. The unbounded property belongs to the representational coordinate space, not the measurement.

The signed scene-linear estimator remains separate from positive-light TruthRange. Real 2026-09-14 Honor dark/shadow measurements now empirically reinforce the need to preserve below-black signed numerical values rather than clipping them to zero.

Historical evolution is documented in `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_EVOLUTION_2026-09-13.md`.

The newest source-bound evidence synthesis is in:

`docs/research/zero-line-empirical-validation-v0.1/TRUTHRAW_ZERO_LINE_HISTORY_AND_EMPIRICAL_STATE_2026-09-14.md`.

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

## 8. Recovered house genealogy and design rationale

The house is not only a performance metaphor. Recovered project history shows four ideas converging into the current architecture:

- **gezegelde woning / sealed house** — immutable original RAW evidence;
- **alle vrijheid / new house** — a richer reconstructed scene representation unconstrained by arbitrary source-container limits, while evidence claims remain bounded;
- **achterkant van de foto / Technical Backplane** — compact scientific identity/provenance behind the visible image;
- **tussenwoning / Gatehouse** — isolated external RAW decode, sealed handoff, then detach before heavy Main-House work.

The zero-line/nul-lijn belongs to the **new-house freedom**: it supplies a scene gauge around which the representational house is not forced to stop at the source container's bright or dark limits.

Repository verification confirms the sealed-house vision was formally canonized by commit:

`13075856895ee6815c8a72fcf733d52bad596583` — `Canonize sealed-house TruthRaw core vision`.

The full recovered genealogy and current interpretation live in:

`docs/CORE_VISION_HOUSE_GENEALOGY_BACKPLANE_GATEHOUSE_2026-09-13.md`

That document is active design rationale, not a substitute for module-level validation.

## 9. Gatehouse / external RAW ingress

Native/direct supported RAW should take the shortest validated route. Formats requiring an external decoder enter through a temporary Gatehouse / **tussenwoning**:

`sealed source -> Gatehouse decode/audit/topology/provenance/resource check -> persisted sealed handoff -> Gatehouse detach/free -> Main House`

Lifecycle shorthand:

`ATTACHED -> SEALED_HANDOFF -> DETACHED -> MAIN_HOUSE_ACTIVE`

The Gatehouse is a controlled ingress stage, not a second source of truth. Decoder support must fail closed when topology, metadata, or sample interpretation cannot be established safely.

## 10. Technical Backplane / backside of the photo

The visible image is only the front-facing projection. The **Technical Backplane** is the compact digital backside that binds source/master identity, zero-line/scene-scale, frame/evidence counts, authority and projection status.

Historical Backplane work included a compact fixed serialization of roughly 180 bytes for one validated phase; the exact layout is module/version-specific.

The Backplane is metadata/control-plane state, not image evidence and not a secret second master.

The 2026-09-14 dark-frame work suggests that future empirical calibration identity may need a versioned capture/sample-domain identifier. This must not be inserted silently into any frozen Backplane/certificate layout; it requires an explicit schema/version change if adopted.

## 11. Streaming and memory

The project already established the key production direction:

`TileSource -> reconstruction/statistics -> release -> exposure/scene plan -> reread tile -> reconstruction -> appearance/export -> StreamedSink -> release`

A full frame must not be materialized merely because the final image is large. Rebuildable intermediate data should be disposable. Scientific identity/backplane state should be compact and immutable.

Historical 200-MP streaming experiments demonstrated that algorithmic workspace can remain sub-megabyte even when output resolution is huge; those numbers are historical engineering measurements/estimates for specific test modules, not universal RAM guarantees.

## 12. Physically exercised read optimization

The finalized read optimization reduced exact tile reads from 13,824 to 7,680 by replacing four exact median passes with two exact radix scans while preserving the canonical sample set/median definition and Scientific Master identity.

On the physical HONOR run this corresponded to:

- tile reads: 13,824 -> 7,680 (`-44.44%`)
- RAW payload read: roughly `-30.4%`
- wall time: roughly `-11.5%`
- worker time: roughly `-10%`

Peak PSS rose slightly in that run, so **no memory-improvement claim is allowed from this measurement**.

## 13. Output taxonomy

Keep these output classes separate:

- **Original Direct CFA** — measured evidence.
- **Direct-CFA evidence repack** — same measured payload in another carriage, if byte/sample identity is preserved and proven.
- **Scientific Master** — reconstructed scene state.
- **Reconstructed CFA DNG** — reconstructed/generated mosaic projection; label `RECONSTRUCTED_CFA_PROJECTION`, never measured sensor RAW.
- **Linear Scientific Master DNG / LinearRaw DNG** — RGB compatibility projection from the Scientific Master; label as compatibility/projection, not evidence.
- **JPEG/ARGB/HDR preview** — appearance/presentation projection.
- **rawsensor** — internal/nonstandard low-level payload; it requires an explicit ABI/provenance definition before it can be called canonical.

No export may create photons, independent evidence, `FULL_PHYSICAL`, a new zero-line, or a different Scientific Master digest.

The current float32 PURE projection intentionally preserves negative and greater-than-one numerical components. This is compatible with the richer Scientific Master representation; it does not turn those components into new measurement evidence.

## 14. DNG interoperability boundary

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

## 15. Counterfactual / virtual observations

Virtual EV, virtual ISO/gain, relighting, and virtual camera forward models can be useful numerical observations of one latent scene. They do not create independent captures.

A virtual observation may improve numerical conditioning or provide an appearance/counterfactual projection only under an explicit contract that returns uncertainty to the one original measurement likelihood.

Counterfactual illumination must never be written back as observed evidence.

## 16. Light and material claims

Illumination reasoning may use geometry, normals, visibility, BRDF/material response, source direction, falloff, spectrum and boundary illumination when those inputs are actually supported.

`Room Lite`/relative relighting is appearance-level/counterfactual. Strong physical relighting requires physical calibration and sufficient geometry/material/spectral evidence.

Semantic labels such as “wood”, “grass” or “skin” must not be used as scientific evidence for missing texture/detail. Measurable texture support may be used without claiming the semantic material identity.

Recovered history also contains a separate material/detail chain such as `RAW evidence -> v4.7j detail -> Unified Material Truth Limiter -> PTC v1.1 -> export`. It is related claim-governance history, not a replacement for the house/reconstruction pipeline.

## 17. Current validation posture

The strongest project posture is deliberately asymmetric:

- source identity/sealing: strong where hashes and byte paths are recorded;
- single-frame evidence accounting: strong;
- bounded streaming/read optimization: physically exercised on the reference Honor source;
- source-bound DNG color: reproducible but not independently physical;
- signed post-black preservation: empirically reinforced by real-scene and covered dark data;
- ISO100/400 temporal dark-noise measurement: physically exercised for the tested duplicate pairs;
- source-bound DNG NoiseProfile high-ISO consistency: supported, while exact MotionCam/device semantics remain open;
- simple monotonic ISO8192 noise-domain threshold: rejected;
- reproducible discrete exact-ISO8192-associated capture/sample domain: observed across independent sessions; physical cause open;
- DNG projection/export: active validation work;
- universal external RAW support: dependent on decoder/topology validation;
- `FULL_PHYSICAL` color/light/material truth: blocked until independent calibration exists;
- physical PTC/conversion-gain/DCG certification: open until controlled repeated dark + flat protocols exist;
- Vulkan/GPU execution: future optional execution backend only, never a truth source.

## 18. Current authoritative project-level documents

Read in this order:

1. `START_HERE_NEW_CHAT.md`
2. `docs/TRUTHRAW_PROJECT_MAP_2026-09-13.md`
3. `docs/audit/PROJECT_FACT_CHECK_2026-09-13.md`
4. `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-13.md`
5. `docs/CORE_VISION_HOUSE_GENEALOGY_BACKPLANE_GATEHOUSE_2026-09-13.md`
6. `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md`
7. `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
8. `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_EVOLUTION_2026-09-13.md`
9. `docs/research/zero-line-empirical-validation-v0.1/TRUTHRAW_ZERO_LINE_HISTORY_AND_EMPIRICAL_STATE_2026-09-14.md`
10. `docs/research/zero-line-empirical-validation-v0.1/TRUTHRAW_ZERO_LINE_EMPIRICAL_STATE_2026-09-14.json`
11. `state/CURRENT_CANONICAL_STATE_2026-09-13.json`
12. `docs/DOCUMENT_STATUS_INDEX_2026-09-13.md`
13. then the module-local canonical/research documents relevant to the task.

Older dated state and architecture documents remain provenance/history and must not silently be treated as the latest global state.

## 19. 2026-09-14 zero-line / noise empirical overlay

The 2026-09-14 Honor tele campaigns must now be carried forward with the project history rather than treated as isolated chat experiments.

The combined result is:

- the nul-lijn remains the two-sided gauge of an unbounded **representational** address space;
- the sealed sensor evidence remains finite, noisy, quantized and censored;
- real below-black source values reinforce signed scientific storage on the dark side;
- dark/noise-limited physical light is represented by uncertainty/bounds, not by declaring negative photons or exact darkness;
- source ISO remains provenance, but ISO magnitude alone is insufficient as a future empirical-noise calibration key;
- calibration must fail closed across an observed capture/sample-domain discontinuity;
- exact ISO8192 repeatedly selected a high-scale/censored source domain while nearby ISO8184 and higher ISO10244 stayed ordinary, so the simple threshold model is rejected;
- no Scientific Master/reconstruction algorithm change is justified from these measurements alone.

Permanent project wording:

**The nul-lijn allows the new house to extend without a finite representational ceiling upward and without a finite representational floor downward. Calibration locates finite evidence inside that house; it never grants the house its extent and never upgrades uncertainty into measured photons.**
