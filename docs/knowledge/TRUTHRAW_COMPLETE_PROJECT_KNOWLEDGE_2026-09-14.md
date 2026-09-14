# TruthRaw complete project knowledge — 2026-09-14

Status: **ACTIVE CROSS-SESSION KNOWLEDGE PRESERVATION / NOT A SILENT CANON REWRITE**

Purpose: preserve the technical and conceptual knowledge established across the long-running TruthRaw project discussions so future sessions do not have to rediscover the project from fragments. This document deliberately contains both current rules and historically important context. Where a newer live implementation or explicit canonical document conflicts with a historical statement, the newer higher-authority source wins, but the historical statement remains provenance.

## 1. One-sentence project definition

**TruthRaw preserves one sealed RAW observation as immutable evidence, interprets that evidence through explicit measurement/calibration authority, reconstructs only evidence-supported missing scene information into a separate uncertainty-aware Scientific Master, keeps hypothetical/counterfactual worlds separate, and exports PURE/ADVANCED/JPG projections without allowing presentation choices to strengthen the underlying evidence claim.**

## 2. Two project laws

These are the most important laws in the project:

**Measured where measured. Reconstructed where necessary. Never invented.**

Dutch working form:

**Echt gemeten. Echt gereconstrueerd. Geen verzinsels.**

Second law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Consequences:

- a richer float32 representation may contain negative or `>1` values without claiming extra photons;
- a reconstructed pixel may be numerically precise while epistemically uncertain;
- a simulated relight may look physically plausible without becoming evidence for what the camera actually measured;
- an export container may preserve more range than the input integer RAW without upgrading sensor evidence;
- hashes/certificates prove identity/integrity under a serialization, not physical truth by themselves.

## 3. Authority pipeline

The project-wide authority direction is:

`Foundation -> Measurement -> Reconstruction -> Scene -> Counterfactual -> Appearance -> Projection`

A downstream floor may not silently strengthen or rewrite an upstream claim.

Examples:

- reconstruction is not measurement;
- demosaic is not original sensor evidence;
- a preview is not source evidence;
- a virtual EV/ISO view is not another frame;
- a color rendering preference is not color calibration;
- an HDR presentation is not capture dynamic range calibration;
- a DNG writer cannot create `FULL_PHYSICAL` authority;
- a certificate cannot promote an uncalibrated scene to calibrated physical truth.

## 4. Core state separation

TruthRaw keeps these conceptually separate:

1. **Sealed source evidence** — original RAW/CFA bytes plus capture metadata identity.
2. **Controlled measurement interpretation** — black level, white/saturation, noise semantics, GainMap/opcodes, source-bound color metadata, and later independently validated calibration where available.
3. **Scientific/Latent Scene Master** — reconstructed full-color scientific scene state, currently camera-native RGB before the normal `camera_to_xyz()` route and before appearance.
4. **Uncertainty/support/censor state** — what is strongly measured, noise-limited, reconstructed, clipped/censored, or unknown.
5. **Hypotheses/counterfactuals** — virtual lighting/camera/world alternatives that do not retroactively become source evidence.
6. **Appearance** — human-facing rendering decisions such as Soft, Detailed, Colourful, HDR.
7. **Projection/export** — Linear DNG, reconstructed CFA DNG, JPG, future JPEG XL, display preview, etc.

Every important output should remain answerable in terms of:

- what came directly from the sensor;
- what was deterministic source correction;
- what was reconstructed;
- what remained unknown/censored;
- what calibration authority was used;
- what was appearance only.

## 5. Three orthogonal axes

Do not mix these axes:

### 5.1 Evidence / authority

Measured, calibrated, reconstructed, uncertain, counterfactual, appearance, projection.

### 5.2 Representation richness

Examples: RAW10-in-uint16 DNG, signed float32 camera RGB, TruthRange coordinates, float32 XYZ-D50 LinearRaw DNG.

A richer representation does not imply stronger evidence.

### 5.3 Resource / execution

Tile size, worker count, cache, bounded buffers, CPU/GPU backend, concurrency, read count, memory pressure.

Resource changes may change speed/memory use only, not scientific authority.

The Gatehouse is an ingress/trust boundary, not a fourth truth axis.

## 6. Sealed house / new house

The project historically used a house metaphor.

### Sealed house / gezegelde woning

The original RAW/CFA bytes and source metadata are the old/sealed house. They are never repainted or rewritten merely because a better reconstruction exists.

### New house / alle vrijheid

The Scientific Scene Master is the new house: a richer representation that may remove integer-container limitations and hold signed, high-range, full-color state. It may exceed RAW10/WhiteLevel/SDR numeric limits, but it cannot exceed the evidence in its truth claims.

Container freedom removes representational limits; it does not remove photon statistics, saturation/censoring, optics, sampling, spectral ambiguity, or scene ambiguity.

## 7. Float32 and the Scientific Master

The early pipeline explored:

`RAW integers -> black removal -> normalize WhiteLevel -> noise-aware correction -> demosaic -> AsShotNeutral -> ForwardMatrix -> XYZ -> linear RGB/display`

An early important artifact was a large float32 linear-scene TIFF before display gamma. That became a precursor of the Scientific Master idea.

The modern Scientific Master concept is stronger and more explicit:

- full-resolution;
- full-color;
- camera-native RGB at the current project-level definition;
- signed float32 or richer;
- uncertainty/support/censor/model-influence attached as scientific state;
- source/calibration identity bound separately;
- not limited to `[0,1]`;
- not a display image;
- not original CFA evidence.

A future colorimetric scene master may use documented XYZ/wide-gamut coordinates, including negative and `>1` components, but historical colorimetric experiments must not silently redefine the current camera-native Scientific Master.

Posterior uncertainty is not the same object as a DNG `NoiseProfile`.

## 8. Nul-lijn / zero-line / TruthRange

For positive light:

`T = log2(L / L0)`

`L0` is a gauge/reference. `T=0` is therefore not:

- physical darkness;
- zero photons;
- DNG BlackLevel;
- clipping;
- display black;
- middle grey.

The intended representational address space is two-sided:

`darkness <- ... <- -EV <- nul-lijn -> +EV -> ... -> brighter light`

Mathematically:

- `L -> infinity` gives `T -> +infinity`;
- `L -> 0+` gives `T -> -infinity`.

This does **not** imply infinite sensor dynamic range. The sensor still provides a finite noisy/quantized/censored observation.

TruthRaw therefore keeps a signed scene-linear estimator for unbiased residual/noise/reconstruction work and may also keep a positive-light TruthRange coordinate with explicit support/bounds/uncertainty.

Negative post-black numerical values are not negative physical light. They remain scientifically useful and must not be clipped merely to make logarithms convenient.

Virtual EV shifts the gauge/view; it creates no new evidence.

## 9. Technical Backplane / achterkant van de foto

The Technical Backplane formalizes the historical idea of the digital backside of a photograph.

It binds lineage/provenance such as:

- source evidence identity;
- Scientific Master identity;
- zero-line identity;
- scene-scale identity;
- physical/evidence counts;
- room/authority status;
- claim/projection status.

It is not hidden image evidence and not a second image/master.

The promoted compact Backplane design uses a frozen binary layout and must not be silently changed just to add a new research field. New persistent fields require a versioned schema.

One lineage has one Backplane. Zero-line is referenced by the Backplane; it is not the Backplane itself.

## 10. FotoGraaf / local room / scene metrology

The historical `Foto graaf` idea evolved into a local investigator/metrology subsystem rather than a world generator.

Pipeline concept:

`Sealed RAW -> Scientific Scene Master -> uncertainty/topology -> Room Selection -> Room Capsule -> Light Simulation -> Counterfactual/Appearance Output`

Key law:

**Never simulate the whole world when only the illuminated local domain is required.**

A Room Capsule may contain temporary/local geometry/material/light hypotheses for a selected region. It does not edit the sealed RAW or Scientific Master.

`VOM`/virtual observation concepts are reparameterizations of the same evidence. `CICM`/counterfactual illumination/capture models are hypothetical.

Important law:

**A simulated world may generate new hypothetical measurements; it never retroactively creates new evidence for the captured world.**

Full physical relighting from one image is ill-posed because geometry, normals, visibility, BRDF/material, illuminant spectra, and indirect transport are underdetermined. Counterfactual output must therefore remain explicitly hypothetical unless independently constrained.

## 11. Water / droplets / difficult material tests

The exact original phrase `waterdruppels` was not reliably recovered as a foundational quote and must not be overclaimed as such.

Later water/material tests were useful because water stresses multiple parts of the model at once:

- surface reflection;
- body/water-leaving signal;
- transmitted/submerged scene;
- specular glints;
- foam/spray;
- refraction/transparency;
- clipping/censoring;
- tiny high-frequency detail.

From one RGB RAW, those components are generally not uniquely identifiable. Therefore TruthRaw should preserve evidence-supported glints/ripples and avoid generic blue/cyan invention or fabricated highlight peaks.

## 12. Cheap phone versus expensive phone

Canonical resource principle:

**TruthRaw uses the same scientific decisions on cheap and high-end phones. Lower-resource devices may use smaller tiles, fewer workers and less cache. They do not receive lower truth.**

Also:

**Stronger hardware can give a larger/faster room. It cannot give more truth.**

Hardware may alter execution only, not scientific permission.

## 13. Building runtime and twelve rooms

Current logical rooms:

- Archivist
- MeasurementLab
- Architect
- Restorer
- SceneRegistry
- Surveyor
- ManifoldConditioning
- LightingStudioCicm
- RoomCapsule
- Colorist
- Finisher
- Exporter

The Building Runtime owns orchestration. Scientific algorithms keep their own contracts.

Important runtime rules:

- truth floor controls epistemic permission;
- resource profile controls RAM/CPU/GPU/tile/cache only;
- corridors carry handles/provenance/authority instead of unnecessary full-frame copies;
- immutable identity is compact shared state;
- rebuildable caches may be evicted;
- resource policy may be re-derived between indivisible scientific operations, not halfway through one if that could change the result;
- execution fusion is allowed only when adjacent tile data stay resident without merging scientific authority roles.

High-value runtime direction:

`RoomLease + CorridorToken + deterministic tile scheduler + per-room profiler + bounded buffer pools`

CPU/reference remains the validation floor. Vulkan/GPU can later be an optional execution backend only.

## 14. Gatehouse / tussenwoning

External/proprietary RAW decode is isolated before Main-House processing:

`sealed source -> Gatehouse decode/audit/topology/provenance/resource check -> sealed handoff -> detach/free Gatehouse -> Main House`

Lifecycle shorthand:

`ATTACHED -> SEALED_HANDOFF -> DETACHED -> MAIN_HOUSE_ACTIVE`

The Gatehouse may unload or isolate complexity but may never create a second truth.

Typical Gatehouse responsibilities include:

- source vestibule;
- format probe;
- codec resolver;
- decode chamber;
- sample audit;
- topology audit;
- metadata semantics;
- frame/evidence audit;
- resource quarantine;
- provenance binding;
- admission inspection;
- handoff airlock.

No external decoder context/buffer should cross the persistent sealed handoff. A certified direct native RAW path may bypass the Gatehouse.

Decoder support/version is tooling provenance, not scientific authority.

## 15. RGB RAW returned / LinearRaw direction

A major correction in project history was returning from constructed/re-Bayer output toward direct full-color Scientific Master export.

Correct downstream direction:

`Scientific Master camera-native RGB -> cameraToXyzD50 -> float32 3-channel LinearRaw DNG`

This avoids the inferior compatibility detour:

`reconstructed RGB -> artificial Bayer remosaic -> external demosaic again`

Therefore:

- reconstructed CFA DNG is a compatibility projection;
- LinearRaw/Linear DNG is the natural full-color downstream scientific projection;
- direct CFA source evidence remains a separate measured-evidence class;
- re-Bayer output must never be called original measured sensor RAW.

The richer float32 route preserves negative and `>1` components. Older 16-bit routes remain finite compatibility projections and cannot redefine PURE.

## 16. Reconstruction versions

### v4.7i

Current canonical scientific reconstruction baseline:

- measured-preserving;
- physically measured CFA component re-injected exactly where required;
- missing RGB components reconstructed;
- research backend is directional/edge-aware/support-limited;
- scientific role only.

### v4.7j / v4.7k

Appearance/detail/output-acutance only. They must not be treated as stronger scientific reconstruction.

### older v3i/v3j

Restricted/withdrawn after exact-kernel mismatch and later architectural corrections. Preserve history, do not promote as current science.

Any confidence model must be bound to the exact backend/schema/protocol it was validated against.

## 17. Sharpness and detail authority

Keep three things separate:

1. optical/sample detail authority;
2. reconstructed spatial detail;
3. output acutance/sharpening.

Physical detail is limited by lens MTF, diffraction, sampling/Nyquist, CFA layout, noise, motion, focus, and sensor response.

Sharpening changes edge contrast. It does not create missing information.

Do not call uncalibrated deconvolution physical recovery.

## 18. Light and darkness

TruthRaw should respect radiometry rather than visual folklore.

Important boundaries:

- point-source surface irradiance may follow inverse-square under valid assumptions;
- do **not** use generic object-to-camera `1/r^2` brightness as a prior for resolved surfaces;
- radiance is conserved along a free-space ray absent participating-medium loss;
- shadows are not zero light because indirect/bounce illumination may remain;
- specular highlights are weak evidence of base material color;
- participating media require volumetric transport;
- artificial-light/night scenes may have weak blue-channel evidence and strong WB amplification;
- lighting hypotheses may guide ambiguity analysis or appearance but may not rewrite the original-world Scientific Master without evidence.

Counterfactual sun/night/relight belongs to hypothesis/counterfactual state unless independently observed.

## 19. Color truth and calibration

There is no context-free object RGB that is automatically the one `true colour`.

Observed sensor color depends on:

- illuminant spectral power distribution;
- reflectance/transmittance;
- geometry and BRDF;
- optics;
- sensor spectral sensitivities;
- source processing/opcodes;
- exposure/noise.

Metamerism remains a fundamental limit.

Therefore target claims should distinguish:

- reproducible source-bound DNG color interpretation;
- calibrated colorimetry;
- full physical spectral truth, which is generally unavailable from one RGB RAW.

White balance is a coordinate/neutral-adaptation operation; it does not discover the unique true object color. Large channel gains amplify noise.

Keep separate confidence axes for:

- spatial/luminance support;
- color/channel support;
- illuminant/calibration confidence.

Current source-bound color metadata can be reproducible without being independently calibrated physical color.

R/B swap plus double-Orientation correction was rejected.

No demographic skin calibration is part of the physical color authority model.

## 20. Optics

The lens is part of the measurement operator.

Potential physical calibration targets include:

- PSF/MTF;
- chromatic aberration;
- flare/veiling glare;
- shading/color shading;
- focus-state-dependent response.

However optical inverse correction is **BLOCKED** without controlled calibration. Source GainMap/opcodes are deterministic source corrections when properly interpreted; they are not proof of a complete lens inverse.

Star fields may help sample PSF only under controlled assumptions.

## 21. Device / capture identity

Primary device family in current research:

`HONOR BKQ-N49 / Honor Magic 8 Pro`

Historical Android lens/system mapping supplied in the project:

- Main 1.0x — System ID `2` — RAW10
- Wide 0.6x — System ID `4` — RAW10
- Tele 3.7x — System ID `5` — RAW10
- Front 1.0x — System ID `1` — RAW10

Tele is a high-priority calibration target.

MotionCam Direct-CFA and Honor/vendor/BnCam DNG domains are separate source domains and must not be silently merged.

A representative MotionCam source:

`IMG_260830_143012_297_014.dng`

- 4080x3072
- SHA-256 `fac842110cfd5b527aae2fa46237824d444aa5701017c2b68021962ebce47da3`

Historical early tele source facts included BGGR, WhiteLevel `1023`, ISO100, approximately f/2.6 and ~1/154 s, with very low saturation fraction.

## 22. Full-sensor / high-resolution capture research

Tele System 5 was reported with:

- `RAW_SENSOR` maximum-resolution lattice around `16320x12288` (~200.54 MP);
- normal high-resolution around `8160x6144` (~50.14 MP).

The research direction shifted away from merely reconstructing a hypothetical 200 MP image toward proving the actual app-visible full sensor lattice and payload.

A processed 200 MP JPEG is not RAW proof.

Proof requires exact camera/system identity, dimensions, max-resolution mode, timestamp/frame equivalence, payload/stride/size/hash, and source provenance.

`SENSOR_INFO_LENS_SHADING_APPLIED=true` means app-visible RAW may already contain upstream shading correction; do not assume pristine uncorrected sensor values without proof.

## 23. Streaming and supported DNG subset

The strict TileNativeDngSource v0.1 subset intentionally supports only a narrow fail-closed topology, including classic TIFF/DNG, 2x2 RGB Bayer, one sample/pixel, uint16 and supported metadata/opcodes. Packed/compressed/BigTIFF/X-Trans and other unsupported topologies must fail closed rather than be guessed.

The full-frame path is bounded/tiled/two-pass rather than unconstrained full-frame materialization.

Read optimization physical result:

- exact tile reads reduced from `13,824` to `7,680` (~44.444% reduction);
- wall time ~`98.154 s`;
- CPU time ~`97.189 s`;
- peak PSS did **not** improve, so this is not a memory-win claim.

## 24. Artificial-light / night stress test

Representative dark/artificial source:

`IMG_20260906_002826.dng`

Historical observed conditions:

- ISO ~3106;
- 1/60 s;
- f/2.6;
- blue evidence much weaker than green/red;
- large blue white-balance gain around `3.066`;
- many blue sensels below a moderate SNR threshold;
- non-trivial below-black samples.

Lesson: white balance is not equivalent to truthful artificial-light recovery. Denoise/reconstruction should operate camera-native before large WB gains where possible, and uncertainty must survive the transformation.

## 25. Noise / DNG NoiseProfile

Noise modeling must distinguish:

- source metadata `NoiseProfile` semantics;
- empirical sensor/noise calibration;
- posterior reconstruction uncertainty.

They are not interchangeable.

MotionCam/BnCam/device NoiseProfile channel ordering/coordinate semantics remain an OPEN area unless independently verified for the exact source domain.

Empirical calibration must be bound to exact capture/sample-domain identity, not ISO magnitude alone.

## 26. Calibration philosophy

Calibration captures may be multi-frame because they estimate a camera/system model, but a later photograph still remains one scene observation.

Calibration therefore must not change:

- `physicalFrameCount=1`;
- `independentEvidenceCount=1`;
- sealed scene-source identity.

Calibration can strengthen interpretation only for quantities that were independently measured, validated, and admitted inside a matching scope/domain.

Typical required calibration evidence includes:

- covered/dark frames;
- flat-field frames;
- linearity/saturation steps;
- repeated measurements;
- gain/readout-state separation;
- thermal/domain characterization;
- independent fit/validation split;
- controlled color targets/illuminants for color authority;
- controlled optics targets for lens inverse work.

Operational interpolation across an observed domain discontinuity is forbidden unless separately validated.

## 27. FotoGraaf CalibrationPack and runtime admission

The calibration architecture now formalizes:

`Calibration dataset -> validated CalibrationPack -> exact scene admission -> immutable CalibrationBindingPacket -> measurement-model artifact -> shadow comparison -> future promotion gate`

Scene admission is fail-closed and exact-scope-bound. Relevant exact scope includes device/lens/capture API/mode/dimensions/CFA/sample representation/capture-domain/firmware/focus/stabilization and runtime measurement state such as shutter, ISO metadata, f-number and gain/readout state.

ISO alone may not select a calibration domain.

Worker count is execution-only and excluded from binding authority.

Product `HDR` is not a calibration quantity.

If calibration is required but not admitted, the result is a fail-closed calibration-unavailable state rather than silent fallback to a stronger claim.

## 28. Measurement-model shadow adapter

Current research adapter keeps source and calibrated interpretations side-by-side.

Canonical source Stage-2 arithmetic shape:

`((raw - phaseBlack) / max(whiteLevel - phaseBlack, 1)) * existingGainMap`

The shadow calibrated branch may, when exact model authority permits, compare alternative calibrated:

- phase black;
- noise model;
- scalar response scale;
- saturation/censor threshold;
- dark SNR threshold.

Rules already enforced in research:

- exact source/binding/model/protocol identity required;
- source GainMap may not be applied twice;
- source censoring may never be undone by calibration;
- true below-black numerical values remain signed;
- a noise-only calibration must not move the signal coordinate;
- 1/1 scene evidence counts remain unchanged;
- worker count must not change results;
- per-channel arbitrary response scaling is not admitted as a shortcut into color calibration.

The shadow model remains research-only and is intentionally not wired into canonical/Android production routes yet.

## 29. Product output modes

Current public mode contract:

1. `JPG`
2. `JPG XL`
3. `TRUTHRAW PURE`
4. `TRUTHRAW ADVANCED`

Appearance toggles:

- `Colourful`
- `Detailed`
- `Soft`
- `HDR`

These apply only to JPG, JPG XL and ADVANCED. PURE remains appearance-neutral.

Missing/unknown mode fails safe to PURE.

Appearance selections are currently intent only unless/until their renderer semantics are separately implemented/validated.

JPEG XL remains fail-closed until a real encoder is validated.

English is fallback; Dutch, German and French resources exist in the active prototype.

## 30. TRUTHRAW PURE float32 DNG

Current preferred scientific projection:

`sealed source -> source-bound color -> Scientific Master -> Technical Backplane -> canonical replay/digest gate -> cameraToXyzD50 -> float32 XYZ-D50 LinearRaw DNG`

Properties:

- 32-bit IEEE float;
- 3 samples/pixel;
- DNG `LinearRaw` photometric role;
- negative and `>1` values preserved;
- no creative tone/appearance/gamut mapping;
- exact Scientific Master identity gate;
- 1 physical frame / 1 independent evidence root.

This is a projection of the Scientific Master, not a second source RAW.

## 31. Certificate v0.1

The TruthRaw Certificate belongs inside the file/technical backside, never as a visible watermark.

It binds source/master/zero-line/scene-scale/backplane/projection/evidence identity under its schema.

Current state:

`UNSIGNED DEVELOPMENT`

No private brand-signing key may be embedded in the repository/APK.

A verified badge is forbidden until a real trusted issuer signature and independent verification path exist.

Current certificate/Backplane details must not be overclaimed as collision-resistant where only CRC32 is used.

Certificate v0.1 does not independently serialize every exporter-internal boolean such as `appearanceApplied` / `counterfactualObservationCreated`; artifact verification must state that limitation.

## 32. Existing-app integration guidance

TruthRaw can be integrated into an existing Android app, especially if that app already opens/saves RAW/DNG, but it is not a normal image-filter library.

Recommended architecture:

- host app owns UI/file/storage workflow;
- TruthRaw remains a sealed processing/science module;
- narrow host interface can resemble `processRaw(input) -> result/output`;
- native/JNI layer performs decode/reconstruction/master/export under TruthRaw rules;
- existing app must not preprocess/tone/denoise/WB/re-Bayer the source before TruthRaw;
- direct Camera2/RAW_SENSOR capture integration is more complex because stride/payload/metadata/lens/mode/source sealing all become part of evidence identity.

## 33. GCam / APK boundary

Historical GCam tuning/noise-model exploration is separate from TruthRaw scientific authority.

GCam/APK/computational-RAW content must not be used as TruthRaw evidence, calibration source, topology authority, or scientific model authority. TruthRaw may coexist with/export to other ecosystems, but those pipelines may not determine TruthRaw truth claims.

## 34. Rejected / restricted / blocked paths

### REJECTED or compatibility-only

- constructed/virtual ideal RAW as if it were source truth;
- artificial Bayer remosaic called original RAW;
- virtual EV treated as extra evidence;
- display black treated as zero-line;
- R/B swap plus double Orientation workaround;
- generative scene content as scientific truth;
- GCam/APK authority over TruthRaw.

### RESTRICTED / historical research only

- old constructed-RAW / Virtual Ideal Camera experiments;
- withdrawn v3 reconstruction variants;
- reconstructed CFA DNG as anything more than compatibility projection.

### BLOCKED pending calibration/evidence

- full optical inverse / physical deconvolution;
- independent physical color authority;
- complete spectral/material/light truth from one RGB RAW;
- trusted certificate signing;
- JPEG XL production path;
- production use of the FotoGraaf calibration shadow model;
- promotion of uncontrolled test frames into calibration authority.

## 35. Preservation rule for future sessions

Historical keywords should trigger project genealogy recovery rather than isolated interpretation:

`kleur echtheid`, `licht`, `pure RAW`, `geen verzinsels`, `single-frame`, `constructed RAW`, `Multi-Light`, `achterkant foto`, `Foto graaf`, `verzegelde woning`, `waterdruppels`, `midden lijn`, `float32`, `goedkope en dure telefoon`, `RGB RAW weer terug`, `tussenwoning`.

For each, recover:

`original motivation -> hypothesis -> experiment/RAW -> finding -> limitation/error -> fact-check -> correction -> later integration -> current meaning`

Do not flatten history into present-day terminology.

## 36. Final continuity statement

TruthRaw is not trying to hide uncertainty behind a prettier image. Its architecture is specifically designed so a richer reconstructed scene can be built from a finite RAW measurement while every boundary between measurement, correction, reconstruction, uncertainty, hypothesis, appearance and export remains inspectable. The scientific ambition can grow, but the project is only allowed to strengthen a claim when the evidence and calibration authority grow with it.
