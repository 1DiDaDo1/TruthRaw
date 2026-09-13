# TruthRaw Project Fact Check — 2026-09-13

Scope: project-wide scientific/architectural audit against the current repository lineage, recorded physical evidence, and current external specifications/standards checked on 2026-09-13.

This audit does **not** convert research hypotheses into facts. It classifies claims by evidence strength and records corrections without deleting historical failures.

## Verdict

The core TruthRaw architecture is scientifically defensible **if its authority boundaries remain strict**. The strongest parts are evidence sealing, single-frame accounting, explicit measured/reconstructed separation, censoring-aware reasoning, source-bound color qualification, and bounded/tiled execution. The largest risks are not the core idea; they are terminology drift, stale global state documents, projection/evidence conflation, overclaiming physical color/relighting, and allowing runtime optimizations to change scientific results.

## Claim classification used here

- **CONFIRMED_PROJECT_EVIDENCE** — supported by repository evidence/tests and, where applicable, a physical run.
- **STANDARDS_ALIGNED** — consistent with an external specification/standard, but the external standard does not validate the full TruthRaw claim.
- **PROJECT_DEFINITION** — a deliberate TruthRaw coordinate/authority convention. It may be mathematically valid without being an external standard.
- **ENGINEERING_HYPOTHESIS** — plausible design awaiting broader implementation/measurement.
- **PHYSICAL_CALIBRATION_REQUIRED** — cannot be promoted without independent measurement.
- **HISTORICAL_ONLY** — useful provenance but not a current global authority.

## A. Source evidence and single-frame accounting — PASS

**CONFIRMED_PROJECT_EVIDENCE**

The immutable Direct-CFA/source rule is internally coherent and is the correct epistemic anchor for a project that wants to preserve what the sensor actually measured. The single-frame rule `physicalFrameCount=1`, `independentEvidenceCount=1` prevents virtual views, reconstructions, or multiple numerical passes from masquerading as extra measurements.

Correction retained: a reconstructed RGB master, reconstructed CFA, virtual EV view, synthetic forward-camera observation, preview, or export is not a second physical capture.

Optimization retained: pass compact source/master identities and hashes rather than repeatedly copying full image payloads.

## B. TruthRange / zero-line — PASS WITH TERMINOLOGY GUARD

**PROJECT_DEFINITION + mathematically sound coordinate choice**

For positive light, `T = log2(L/L0)` is a valid log-ratio coordinate. A multiplicative change in `L` becomes an additive shift in `T`, and choosing `L0` fixes an origin/gauge. The coordinate can be mathematically unbounded even when the evidence is finite.

The project must use the following exact distinction:

- TruthRange address space can be `(-infinity, +infinity)`.
- A real sensor has finite measurement capability and finite/censored evidence support.
- `L0` selects a coordinate reference; it does not create information.

**Required correction:** do not describe TruthRaw as proving “infinite sensor dynamic range”. If shorthand “infinite dynamic range” appears, it must explicitly mean the representational TruthRange address space, not sensor information.

**Required correction:** a clipped sample is censored. If the calibrated mapping says saturation starts near some TruthRange coordinate, the correct statement is a bound such as `T >= threshold` unless other independent evidence narrows it. Saturation is not an exact latent value.

## C. ISO/exposure and self-gauge — PASS AS PROJECT ARCHITECTURE

**PROJECT_DEFINITION**

Removing capture ISO/exposure from the definition of *relative within-scene* TruthRange is valid when the gauge is derived consistently from the scene representation and tested for multiplicative scale invariance. Capture ISO/shutter still remain indispensable provenance and may remain necessary for sensor/camera forward models, noise interpretation, and cross-scene physical radiometry.

**Required guard:** “ISO is unnecessary for relative TruthRange” must never be shortened to “ISO is scientifically irrelevant”. It remains relevant to the physical capture and uncertainty model.

The historical `SELF_GAUGE` result and the approximately `1.39e-7 EV` change under a x37 multiplicative scaling are project validation of numerical invariance, not proof of absolute radiometric calibration.

## D. Sensor dynamic range and EMVA language — PASS AFTER SCOPING

**STANDARDS_ALIGNED**

EMVA 1288 Release 4.0 is a current standard for objective characterization of image sensors/cameras. It provides a measurement framework for real sensor/camera performance. It does not define TruthRange and does not validate an unbounded representational address space.

Therefore:

- physical sensor dynamic range remains a finite measured property;
- TruthRange is a separate representation/coordinate system;
- EMVA terminology must not be cited as proof that TruthRange itself is a physical sensor characteristic.

Official reference checked: https://www.emva.org/standards-technology/emva-1288/

## E. DNG source interpretation — PASS WITH SPECIFICATION BOUNDARY

**STANDARDS_ALIGNED**

Adobe currently publishes DNG Specification 1.7.1.0. DNG is an extension of TIFF 6.0 and provides public metadata/pixel carriage rules. Adobe also publishes a current DNG SDK; on 2026-09-13 Adobe lists DNG SDK 1.7.1 Build 2724 dated 2026-09-08.

Official reference checked: https://helpx.adobe.com/camera-raw/desktop/dng-and-file-formats/digital-negative.html

TruthRaw may use DNG metadata to establish a reproducible source-bound transform, but DNG metadata alone does not prove independent physical calibration of a particular lens/sensor/illuminant instance.

**Required guard:** `SOURCE_METADATA_BOUND` and `FULL_PHYSICAL` remain different authority levels.

**Required guard:** DNG writer validity and scientific master validity are separate gates. A structurally valid DNG can carry a scientifically mislabelled payload; a scientifically valid master can be serialized incorrectly. Both must be tested.

## F. DNG color semantics — PASS WITH PHYSICAL CLAIM BLOCK

**STANDARDS_ALIGNED + PHYSICAL_CALIBRATION_REQUIRED**

The project’s use of source white balance/calibration/forward transforms for source-bound color is reasonable when tag semantics and interpolation are implemented according to the DNG contract and validated independently.

However, three-channel camera data does not uniquely determine the full incident/material spectrum in general. Metamerism remains a real limitation. Therefore “perfect physical color in every scene” cannot be inferred merely from a correct DNG matrix path.

`FULL_PHYSICAL` color remains blocked until independent camera/lens/illuminant characterization supports it.

The historical synthetic `NeutralSolveDidNotConverge` case must remain negative evidence and must not be hidden by changing convergence criteria solely to make it pass.

## G. GainMap / source corrections — PASS IF APPLIED EXACTLY ONCE

**CONFIRMED_PROJECT_CONTRACT + STANDARDS_ALIGNED IN PRINCIPLE**

The project’s strongest rule here is not “always apply a GainMap”; it is: parse the source correctly and apply the required source correction **exactly once in the correct domain**. Double application corrupts radiometry. Omitting a required source correction also corrupts it.

Per-lens/per-camera spatial color or shading corrections require source metadata or independent calibration; they must not be guessed from unrelated cameras.

## H. Scientific Master identity — PASS

**PROJECT_DEFINITION + CONFIRMED_PROJECT_EVIDENCE**

Defining the Scientific Master before appearance and before normal display conversion is a coherent architecture. It correctly prevents a preview/tone curve from becoming the scientific source.

Required terminology:

- Direct CFA = measured evidence.
- Scientific Master = reconstructed scientific scene state.
- Display/preview = appearance projection.
- DNG/JPEG/HDR export = downstream carriage/projection.

**Required guard:** a digest proves byte/content identity under the defined serialization; it does not by itself prove physical truth.

## I. Linear DNG export — PASS AS COMPATIBILITY PROJECTION, ACTIVE VALIDATION

**ENGINEERING_HYPOTHESIS / ACTIVE IMPLEMENTATION**

Exporting reconstructed RGB directly as a LinearRaw/Linear DNG is architecturally preferable to fabricating a Bayer mosaic and forcing a second demosaic, because the Scientific Master is already reconstructed RGB. A fake remosaic would add an unnecessary lossy transform and could be misread as measured CFA.

Required label: compatibility/projection role, not original evidence.

The writer must remain streaming/tiled so a full 4080x3072 (or much larger) floating RGB master is not materialized merely to serialize the DNG.

## J. Reconstructed CFA DNG — PASS ONLY WITH EXPLICIT ROLE

**PROJECT_DEFINITION**

A reconstructed/generated mosaic can be useful as a downstream projection or compatibility experiment, but inferred/generated sites are not measured sensor samples.

Required label: `RECONSTRUCTED_CFA_PROJECTION` (or an equally explicit canonical role). Never call it measured sensor RAW.

A direct-CFA evidence repack is a different class and requires sample/byte identity proof.

## K. Android DngCreator — IMPORTANT BOUNDARY

**STANDARDS/PLATFORM FACT**

Android `DngCreator` writes raw pixel data to DNG and is designed for `RAW_SENSOR` buffers or Bayer-type raw pixel data generated by an application. It derives tags from `CameraCharacteristics`/`CaptureResult` or explicitly set metadata.

Official reference checked: https://developer.android.com/reference/android/hardware/camera2/DngCreator

This means Android `DngCreator` is useful for appropriate Bayer/RAW_SENSOR carriage, but it must not be treated as a generic proof that a reconstructed RGB Scientific Master has been serialized with the intended LinearRaw semantics. TruthRaw’s native deterministic RGB writer therefore remains a legitimate separate requirement.

## L. External RAW ingress / LibRaw — PASS WITH VERSION AND TRUST BOUNDARY

**ENGINEERING_HYPOTHESIS + CURRENT DEPENDENCY FACT**

The Gatehouse design is correct: external decoder code should be isolated from scientific authority, produce an auditable handoff, and be detached before the Main House performs heavy work.

The repository’s observed runner previously used LibRaw 0.21.2. Current upstream has moved beyond that: LibRaw lists 0.21.5 as the final 0.21 release and also publishes 0.22 development/release material. Therefore the project must not describe 0.21.2 as “the current LibRaw version”. It is only the tested distro/runtime version for that CI environment.

Official references checked:

- https://www.libraw.org/node/27
- https://www.libraw.org/

**Optimization:** report both `tested_decoder_version` and `upstream_version_checked_at` in Gatehouse evidence. Do not upgrade merely for version-number freshness; upgrade only with regression/format tests.

## M. Read optimization — PASS, MEMORY CLAIM CORRECTED

**CONFIRMED_PROJECT_EVIDENCE**

The physical Honor run supports the reduction from 13,824 exact tile reads to 7,680 and the associated reductions in read payload and elapsed/worker time.

The same run does **not** support a memory improvement claim: peak PSS increased slightly. Documentation must state exactly that.

This is an important example of the project rule that a desired optimization result cannot be inferred from a different metric.

## N. Building Runtime / rooms — ARCHITECTURALLY STRONG, IMPLEMENTATION INCOMPLETE

**ENGINEERING_HYPOTHESIS with validated subcomponents**

The house model is useful because it separates epistemic authority from execution policy. It should now become a production runtime contract rather than remain partly metaphorical.

Recommended implementation order:

1. `RoomLease` with bounded resident memory and explicit lifetime.
2. `CorridorToken` containing identities/authority/provenance instead of pixel copies.
3. deterministic tile IDs and deterministic reduction order.
4. per-room profiler: wall time, CPU time, queue wait, tile reads, bytes read/written, allocations, peak resident lease, cache reuse.
5. buffer pools/arenas with immediate lease return.
6. execution fusion only where adjacent room operations can share one resident tile **without merging their authority roles**.

This is the highest-value general optimization now because the physical read-optimization data show that reducing source rereads alone no longer explains all runtime cost.

## O. Cheap vs expensive phones — PASS AS INVARIANT, NEEDS CONTINUOUS EQUIVALENCE TESTING

**PROJECT_DEFINITION + ENGINEERING REQUIREMENT**

Hardware may alter tile size, thread count, queue depth, cache residency and optional backend. It must not alter evidence or the canonical scientific result.

Every adaptive execution mode should be tested against a deterministic CPU/reference path for the quantities declared canonical. If a GPU/Vulkan path is added, it is an execution backend only.

## P. Room Capsule / relighting — PASS WITH CLAIM LEVELS

**ENGINEERING_HYPOTHESIS + PHYSICAL_CALIBRATION_REQUIRED for strong claims**

Compact ROI geometry, downsampled normals/depth/visibility, and boundary illumination are reasonable performance representations. Historical MB figures are engineering estimates/measurements for specific configurations, not universal guarantees.

`Room Lite`/relative relighting must remain counterfactual appearance. Strong physical relighting requires sufficient geometry, visibility, BRDF/material response, spectrum and camera/light calibration.

A light hypothesis may guide reconstruction or reduce confidence; it must not create confidence or evidence merely because it produces a plausible image.

## Q. Texture/material detail — PASS WITH SEMANTIC FIREWALL

**PROJECT_DEFINITION**

A measurable texture-support gate may say that fine detail is supported. It must not turn a semantic label such as “wood” or “grass” into evidence for missing structure.

The correct pattern is property-first rather than object-name-first: gradients, repetition, local frequency support, edge consistency, noise likelihood and uncertainty can be evidence; semantic identity is not a substitute for them.

## R. Preview — PASS

**CONFIRMED_PROJECT_EVIDENCE for finalized source-bound route**

The physically exercised finalized scientific preview can be released as a source-bound scientific preview while `scientificClaimAllowed=false`. That distinction is coherent: the preview can accurately display the bounded project state without promoting it to independently calibrated physical truth.

The preview must remain derivative and must never be fed back as source evidence.

## S. Repository governance — FAIL FOUND, FIXED IN THIS AUDIT BRANCH

The global project bootstrap was stale. `README.md`, `START_HERE_NEW_CHAT.md`, `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md`, `state/CURRENT_CANONICAL_STATE_2026-09-10.json`, and `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md` no longer represented all work present on the 2026-09-13 lineage. The old state/index also disagreed about some module status.

This audit branch fixes that by adding a 2026-09-13 project map, current house architecture, fact-check, TruthRange evolution, state file and document-status index, and by updating the bootstrap pointers.

The documentation-governance checker is also updated so the latest dated project-level files are discovered rather than hard-coded to 2026-09-10. This reduces the chance that the same stale-pointer failure returns on the next dated state update.

## T. Branch divergence — OPEN INTEGRATION RISK

The active research heads `research/android-dng-export-v0.1-2026-09-13` and `research/restore-rgb-linearraw-output-v0.2-2026-09-13` are divergent in Git ancestry. The restore lineage nevertheless contains relevant raw-projection/export content.

No scientific conclusion should be based merely on branch names. Before promotion/merge, compare actual trees/modules and run the relevant CI/integrity suites. Do not silently drop unique commits from either lineage.

## U. Claims that remain deliberately blocked

The following are **not** promoted by this audit:

- infinite physical sensor dynamic range;
- perfect/noiseless recovery of information that the source did not measure;
- perfect spectral/material color from generic three-channel RAW;
- `FULL_PHYSICAL` color without independent calibration;
- physical relighting without required geometry/material/spectral calibration;
- memory improvement from the Honor read-optimization run;
- reconstructed CFA being original measured CFA;
- virtual EV/ISO views being independent evidence;
- a structurally valid DNG automatically proving scientific correctness;
- faster hardware creating stronger truth.

## V. Optimization decisions adopted

1. Make the 2026-09-13 project map/state the new global navigation layer while preserving older dated files as history.
2. Keep all scientific algorithms unchanged during this fact-check unless a defect is independently demonstrated; documentation and authority corrections must not perturb validated master bytes.
3. Prioritize Building Runtime leases/tokens/profiling before speculative algorithmic rewrites.
4. Keep CPU/reference determinism as the comparison floor before Vulkan/GPU acceleration.
5. Keep DNG export as a streaming sink downstream of the finalized master.
6. Track external decoder version as an environment fact, not scientific authority.
7. Preserve negative tests and failed physical/scientific gates verbatim.
8. Add automated governance checks for stale current-state/bootstrap pointers and forbidden claim conflations.

## External references checked on 2026-09-13

- Adobe Digital Negative resources / DNG Specification 1.7.1.0 / DNG SDK: https://helpx.adobe.com/camera-raw/desktop/dng-and-file-formats/digital-negative.html
- Android `DngCreator`: https://developer.android.com/reference/android/hardware/camera2/DngCreator
- Android Camera2 RAW capability: https://developer.android.com/reference/android/hardware/camera2/CameraMetadata#REQUEST_AVAILABLE_CAPABILITIES_RAW
- EMVA 1288 Release 4.0: https://www.emva.org/standards-technology/emva-1288/
- LibRaw releases/downloads: https://www.libraw.org/node/27

External standards validate only the claims within their scope. They do not confer TruthRaw scientific authority by association.
