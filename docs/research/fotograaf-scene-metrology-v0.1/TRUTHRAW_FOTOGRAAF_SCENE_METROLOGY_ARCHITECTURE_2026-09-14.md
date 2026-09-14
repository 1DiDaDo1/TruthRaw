# TruthRaw FotoGraaf Scene Metrology — architecture v0.1 — 2026-09-14

**Status: RESEARCH ARCHITECTURE / ACTIVE DESIGN RATIONALE / NO SCIENTIFIC-MASTER ROUTE CHANGE**

This document formalizes the recovered **Foto graaf / fotograaf-kamer** intuition as a scientific metrology capability without weakening the current 12-room TruthRaw authority model.

It integrates three project lines that previously existed separately:

1. source/capture interpretation and ISO/gain factorization;
2. scene-light / local-geometry / HDR diagnostics;
3. RoomCapsule + LightingStudioCicm local illumination work.

The result is **not a 13th canonical room**. `FotoGraaf Scene Metrology` is an authority-tagged protocol that uses existing rooms at different stages.

Permanent laws remain:

**Measured where measured. Reconstructed where necessary. Never invented.**

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

---

## 1. Why this capability exists

The user’s core question is whether the “photographer’s room” can determine the things that conventional photography often spreads across ISO, exposure, HDR, lighting, shadow and relighting controls.

The answer is partly yes, but only if TruthRaw separates three questions:

1. **What did the camera actually measure?**
2. **What physical scene/light state is supported by calibration or inference?**
3. **How should that state later be displayed or changed counterfactually?**

The FotoGraaf protocol exists to answer the first two without turning the third into evidence.

The preferred high-level route is:

`sealed Direct-CFA`

`-> capture-domain metrology`

`-> measurement likelihood / ISO-gain-exposure interpretation`

`-> Scientific Master reconstruction`

`-> scene metrology / light + range diagnostics`

`-> nul-lijn / TruthRange scene-range envelope`

`-> RoomCapsule / LightingStudioCicm only for local counterfactual work`

`-> Finisher/Exporter for HDR or other appearance projection`

This order matters. An HDR display choice or a hypothetical lamp may never travel backward and become observed source evidence.

---

## 2. It is not a new room

The current canonical house still has twelve rooms. FotoGraaf Scene Metrology is a **cross-room protocol**, because its inputs live at different scientific floors.

### Existing-room ownership

**MeasurementLab** owns source-facing FotoGraaf work:

- CFA code interpretation;
- phase BlackLevel / WhiteLevel;
- clipping/censoring;
- exposure time and aperture provenance;
- ISO/gain/readout provenance;
- source NoiseProfile and source calibration metadata;
- capture/sample-domain classification;
- measurement likelihood and uncertainty.

**Architect / Restorer** consume the resulting measurement model when constructing the Scientific Master. They may factor capture gain/exposure effects out of scene identity, but they may not erase capture provenance.

**SceneRegistry** binds the finalized master to nul-lijn / scene-scale identity and retains metrology provenance/authority handles.

**Surveyor** owns post-reconstruction observational scene metrology:

- evidence-supported scene-range diagnostics;
- local radiance/brightness relationships in the declared gauge;
- geometry-support descriptors;
- shadow/specular/illumination-support diagnostics;
- uncertainty and identifiability diagnostics.

**RoomCapsule** remains a compact selected local domain for light/geometry/material operations. It is not promoted into a source-measurement room.

**LightingStudioCicm** remains the counterfactual illumination room. It may consume FotoGraaf estimates as input, but its changed-light output remains counterfactual.

**Finisher / Exporter** own HDR/tone/display projection. They do not define the scientific scene dynamic range.

This preserves the canonical authority direction:

`Foundation -> Measurement -> Reconstruction -> Scene -> Counterfactual -> Appearance -> Projection`

---

## 3. Radiance is not irradiance

This distinction is mandatory for TruthRaw lighting claims.

The CIE defines **irradiance** as incident radiant flux density at a surface point. Its photometric analogue is illuminance.

The CIE defines **radiance** as directional radiant intensity per projected area at a point/direction. A camera looking at a resolved surface is primarily constrained by radiance arriving at its entrance pupil from scene directions, not by a direct measurement of the irradiance incident on every scene surface.

Therefore:

**the camera does not generically measure a per-surface incident-light field directly from one RGB RAW.**

To infer incident light at a surface, TruthRaw may need some combination of:

- scene geometry / surface normal;
- visibility / occlusion;
- material reflectance or BRDF support;
- source direction/distribution;
- spectral information or calibrated channel response;
- participating-medium model where relevant.

This is why a single observed dark patch can be ambiguous: it may be a dark material, weak illumination, orientation away from a light, shadow/occlusion, a spectral effect, or a combination.

The general inverse-rendering problem is therefore ill-posed. Modern inverse-rendering literature likewise treats geometry, reflectance/material and illumination as entangled and requires priors, known geometry, broader observations or other constraints to reduce ambiguity.

### Distance rule

TruthRaw must not use generic object-to-camera `1/r^2` falloff as a brightness prior for resolved surfaces.

Inverse-square behavior is appropriate to irradiance from point-like emitters under its assumptions. It is not a generic rule saying a resolved surface becomes four times darker in the RAW merely because camera distance doubles.

This preserves the existing TruthRaw light-transport correction.

---

## 4. FotoGraaf authority classes

Every FotoGraaf output must carry one of these authority roles. A value may never silently upgrade itself by moving downstream.

### `MEASURED_SOURCE`

Directly supported by the sealed source or exact source metadata under a validated parser/semantics contract.

Examples:

- CFA samples;
- source BlackLevel / WhiteLevel;
- clipping state;
- exposure time;
- aperture/f-number metadata;
- ISO field as capture provenance;
- source NoiseProfile bytes/values;
- exact source identity/hash;
- source opcode/GainMap identity;
- source sample topology.

### `CALIBRATED_PHYSICAL`

Derived from the source through an independent calibration whose scope matches this camera/lens/mode/capture-domain and whose uncertainty is known.

Possible examples after validation:

- absolute/relative radiometric responsivity;
- gain/conversion mapping;
- dark/read-noise model;
- linearity correction;
- flat-field response;
- lens shading / color shading;
- spectral/color response under a declared calibration scope;
- absolute radiance or irradiance only where the calibration chain actually supports it.

Calibration authority must never be inferred merely from the existence of DNG metadata.

### `INFERRED_SCENE`

A best-supported physical scene estimate obtained from measured/calibrated evidence plus a declared model.

Examples:

- local surface normal estimate;
- depth/geometry support;
- dominant-light direction estimate;
- relative illumination field;
- material/BRDF parameter estimate;
- local scene radiance estimate where not independently radiometrically calibrated.

This class always carries uncertainty/model identity.

### `BOUNDED_CENSORED`

The source constrains a quantity but does not identify a finite exact value.

Examples:

- clipped highlight: `T >= T_clip_lower`;
- noise-limited darkness: `T <= T_dark_upper`;
- saturated channel with other channels providing only partial reconstruction support.

### `UNKNOWN`

The source/calibration/model cannot support a useful claim.

Examples may include hidden emitters, occluded geometry, exact full-spectrum illumination, or exact BRDF/material identity from one ordinary RGB RAW.

### `COUNTERFACTUAL`

A hypothetical changed world, such as moving the sun, adding a lamp, changing illumination spectrum or creating a virtual exposure/camera observation beyond a mere coordinate re-expression.

Counterfactual state never upgrades the captured scene evidence.

### `APPEARANCE`

A display decision such as tone mapping, display HDR mapping, creative Colourful/Soft/Detailed/HDR intent or output acutance.

Appearance never modifies the Scientific Master.

---

## 5. Phase M0 — Capture-Domain Metrology

FotoGraaf begins before scene-light inference.

`M0` belongs to MeasurementLab and creates a fail-closed `CaptureMetrologyPacket`.

Required identity/measurement concepts:

- `sourceEvidenceId`;
- camera/device identity where source-bound;
- lens/mode identity where source-bound;
- CFA topology;
- exposure time;
- f-number where present;
- ISO/gain/readout provenance;
- BlackLevel / WhiteLevel;
- clipping/censoring map or statistics;
- source NoiseProfile identity/semantics status;
- source GainMap/opcode identity;
- `captureSampleDomainId`;
- parser/backend identity;
- uncertainty/calibration bindings.

### ISO is not discarded

FotoGraaf does **not** delete ISO from provenance.

Instead, M0 uses ISO/gain/readout/exposure information as part of the measurement history needed to interpret the source. After the validated source forward/inverse relationship is accounted for, Architect/Restorer may build a scene estimate whose numerical identity is no longer “an ISO 800 scene”.

Project wording:

**ISO is factored out of scene identity after measurement interpretation; it is never erased from evidence history.**

The 2026-09-14 Honor tests prove why this must be capture-domain aware rather than an ISO-only lookup. Exact ISO8192 repeatedly selected a high-scale/censored sample domain while same-exposure ISO8184 and higher ISO10244 remained in the ordinary domain.

Therefore:

`captureSampleDomainId != function(ISO alone)`

and empirical calibration must fail closed when the exact domain cannot be matched.

---

## 6. Phase M1 — Scientific Master construction

M0 provides source interpretation to the existing Architect/Restorer route.

The Scientific Master remains the current project-defined reconstructed **camera-native RGB** scene state before normal `camera_to_xyz()` and before appearance.

FotoGraaf does not create a second master.

The reconstruction may be ISO-neutral in scene identity after source measurement effects are accounted for, but uncertainty must still reflect the actual capture history.

Example:

Two captures of the same stable scene made at different validated gain/exposure settings may map toward the same latent scene coordinate while retaining different posterior uncertainty because their sensor-noise/clipping histories differ.

This is the correct sense in which ISO can disappear from the reconstructed scene without disappearing from science.

---

## 7. Phase M2 — Scene Metrology after reconstruction

After a provisional/final Scientific Master exists, Surveyor may derive a `SceneMetrologyPacket`.

This is observational/inferential scene analysis. Because the global authority direction is forward-only, Surveyor diagnostics do not silently travel backward and overwrite measured CFA or relabel reconstructed values as measured.

The packet may contain:

- local scene-light coordinate relative to the bound nul-lijn;
- local uncertainty/support class;
- scene-range envelope;
- clipped/high-side lower bounds;
- dark/noise-side upper bounds;
- geometry/depth/normal support descriptors;
- shadow/occlusion likelihood;
- specular-confidence mask;
- diffuse-support confidence;
- dominant-light-direction hypothesis;
- relative illumination-field hypothesis;
- calibration level and physical-unit availability;
- identifiability/ambiguity flags.

Each field must carry authority and uncertainty. A beautiful plausible light estimate with weak identifiability remains `INFERRED_SCENE`, not `MEASURED_SOURCE`.

---

## 8. Scene HDR is not the HDR appearance button

TruthRaw now distinguishes two different meanings of HDR.

### Scientific scene range

FotoGraaf/Surveyor may describe the scene’s evidence/reconstruction envelope around the nul-lijn:

`-infinity <- dark bound <- measured/reconstructed support <- 0 <- support -> clipped lower bound -> +infinity`

Useful components include:

- `evidenceLowerEv` / `evidenceUpperEv` for the finite well-supported interval;
- `darkUpperBoundEv` for noise-limited regions;
- `clipLowerBoundEv` for saturated regions;
- `reconstructionLowerEv` / `reconstructionUpperEv` when finite posterior support extends beyond direct evidence;
- uncertainty/confidence summaries;
- support masks/classes.

A clipped source does not reveal the exact latent brightness above the clipping threshold. A black/noisy source does not prove exact physical darkness.

### HDR appearance/export

The product `HDR` option remains downstream appearance intent in Finisher/Exporter.

It chooses how a finite display/output represents the Scientific Master. It is not allowed to:

- enlarge measured sensor evidence;
- move the nul-lijn;
- change the Scientific Master identity;
- turn clipped bounds into measured highlights;
- turn dark uncertainty into measured shadow detail.

In short:

**FotoGraaf measures/describes scene range; Finisher decides how much of that range to show.**

---

## 9. Incident-light / light-fall analysis

FotoGraaf may estimate light incidence only at the authority supported by its inputs.

### What may be strongly observed from one RAW

Depending on scene/support, the image can provide evidence for:

- spatial radiance relationships in camera channels;
- shadow boundaries and penumbra structure;
- clipped emitters/highlights as bounds;
- relative gradients;
- specular highlight locations;
- local contrast and channel ratios;
- geometry cues, where supported.

### What is generally inferred, not directly measured

Without independent physical calibration and sufficient geometry/material constraints:

- per-surface irradiance;
- absolute incident-light power;
- exact light-source distance/power;
- exact BRDF;
- exact material reflectance;
- full environment illumination;
- full spectral power distribution.

TruthRaw may still estimate these when useful, but the packet must retain `INFERRED_SCENE`, uncertainty and model identity.

### Calibrated promotion

A quantity may be promoted to `CALIBRATED_PHYSICAL` only when independent measurements support the full chain.

NIST radiometric guidance explicitly treats offset, linearity, flat-field/nonuniformity and radiometric responsivity as calibration terms. EMVA 1288 likewise characterizes camera response/noise through controlled measurements, including temporal variance over controlled signal levels.

For TruthRaw this implies a future calibration pack can legitimately strengthen FotoGraaf metrology when it contains matched dark/flat/linearity/responsivity evidence for the exact camera/lens/mode/sample domain.

---

## 10. Phase M3 — Local RoomCapsule handoff

When local illumination work is requested, Surveyor/SceneRegistry may create a bounded handoff into RoomCapsule.

A `MetrologyRoomCapsuleBinding` should carry only the selected local support needed by the operation, for example:

- ROI/mask identity;
- master/zero-line/scene-scale identities;
- geometry/normal/depth support + confidence;
- observed/inferred illumination descriptors + authority;
- material/BRDF support + authority;
- visibility/occlusion support;
- boundary illumination descriptor;
- uncertainty handles;
- source/calibration provenance handles.

RoomCapsule must not duplicate the whole world merely to operate on one local domain.

Historical rule remains:

**Never simulate the whole world when only the illuminated local domain is required.**

---

## 11. LightingStudioCicm remains counterfactual

This distinction is critical.

FotoGraaf Scene Metrology tries to describe the **captured world** at declared confidence.

LightingStudioCicm may ask:

- what if the lamp were brighter?
- what if sunlight arrived from another direction?
- what if the scene were rendered under a different illuminant?
- what would a virtual camera/exposure observe?

Those are useful but are not retroactive measurements of the original capture.

Permanent rule:

**A simulated world may generate new hypothetical measurements; it never retroactively creates new evidence for the captured world.**

Counterfactual outputs may be used by appearance/product modes but never to strengthen the original Scientific Master’s evidence authority.

---

## 12. Calibration-pack design

A future `FotoGraafCalibrationPack` should be scoped by exact identities and acquisition protocol.

Potential independently validated components:

- dark-frame statistics per capture/sample domain;
- repeated temporal-noise measurements;
- flat-field response at controlled illumination levels;
- linearity curve;
- gain/conversion mapping;
- absolute or relative radiometric responsivity;
- temperature dependence where material;
- lens shading / color shading;
- PSF/MTF/CA/flare only when controlled measurements support them;
- spectral/color response / chart measurements under characterized illuminants;
- illuminance/irradiance reference measurements when an external calibrated meter/source is used.

The current 2026-09-14 Honor ISO/dark campaigns are valuable **source-bound research evidence**, but they are not yet a full physical calibration pack. In particular, complete multi-level controlled flats and repeated measurements across all target domains remain absent.

Calibration captures may be multi-capture evidence without changing the normal TruthRaw photograph from one sealed source frame.

---

## 13. Calibration-domain identity

The newest dark-frame results require a calibration key richer than ISO.

A future key may conceptually include:

`CalibrationDomainKey { cameraId, lensModeId, sourceFormatDomain, cfaTopology, captureMode, sampleDomainId, gainReadoutState, exposureClass, temperatureClass?, calibrationVersion }`

Fields are optional only when the calibration evidence proves they are irrelevant within its declared scope.

Important:

- this is a research contract, not a frozen binary ABI;
- do not silently add fields to the current Technical Backplane or Certificate v0.1;
- if promoted, bind it through an explicit versioned schema and digest.

---

## 14. Data products v0.1

FotoGraaf v0.1 proposes the following logical products.

### `CaptureMetrologyPacket`

Source-facing measurement/capture identity and uncertainty information.

### `SceneRangeEnvelope`

Nul-lijn-relative evidence/reconstruction range plus dark/high censor bounds.

### `SceneRadianceEvidenceField`

Camera-relative or physically calibrated scene-radiance support with explicit calibration level.

The word `radiance` may be used as a physical quantity only if calibration supports physical units; otherwise it must be labelled relative/camera-bound.

### `IncidentLightHypothesisField`

Per-region/pixel/tile illumination/irradiance hypotheses with geometry/material model identity and uncertainty.

Default authority: `INFERRED_SCENE`, not measured.

### `MetrologyAuthorityMask`

Compact per-region/per-tile authority/support classes distinguishing measured/calibrated/inferred/bounded/unknown.

### `MetrologyRoomCapsuleBinding`

Local handoff from observational metrology into RoomCapsule/LightingStudio, retaining all authority/provenance tags.

None of these products are a second Scientific Master.

---

## 15. Fail-closed admission rules

FotoGraaf must fail closed or downgrade authority when:

- source topology/sample semantics are ambiguous;
- exact capture/sample domain cannot be identified for a required calibration;
- calibration identity does not match camera/lens/mode/domain;
- saturation removes an exact value and no valid supporting evidence narrows it;
- dark signal is below reliable identifiability;
- geometry/material/light decomposition is underdetermined;
- spectral claims exceed available spectral calibration;
- counterfactual data is the only support for an original-world claim.

Fail-closed does not mean “return black”. It means preserve the useful estimate/bound if available while lowering its authority and increasing/retaining uncertainty.

---

## 16. Invariants and forbidden shortcuts

The following are v0.1 invariants:

- Direct-CFA source remains immutable.
- `physicalFrameCount=1` and `independentEvidenceCount=1` remain normal photographic invariants.
- ISO/gain is never erased from provenance.
- ISO/gain may be factored out of reconstructed scene identity only through a documented measurement model.
- `captureSampleDomainId` may not be inferred from ISO magnitude alone when evidence shows domain discontinuities.
- signed post-black values must not be hard-clipped to zero in the scientific estimator.
- nul-lijn is not BlackLevel.
- scene HDR is not display HDR.
- incident irradiance is not silently equated with camera-observed radiance.
- object-to-camera distance is not a generic `1/r^2` surface-brightness prior.
- uncalibrated deconvolution is not physical optical recovery.
- one RGB image does not uniquely identify full geometry + BRDF + illumination + spectrum.
- counterfactual light never becomes captured evidence.
- semantic object/material labels do not fill missing physical detail.
- FotoGraaf does not create a 13th room or a second master.

---

## 17. Status after the 2026-09-14 studies

### PASS / supported

- architecture can factor ISO/gain out of scene identity while preserving provenance;
- nul-lijn supports an unbounded representational address space while evidence remains finite;
- signed lower-side estimator is empirically justified by below-black/dark-frame source behavior;
- source-bound high-ISO noise regression is useful;
- exact capture/sample-domain identity must participate in empirical calibration selection;
- scene HDR can be represented as bounds/support around the nul-lijn independently from display HDR;
- local observational metrology can feed RoomCapsule without changing RoomCapsule into a measurement authority.

### OPEN

- complete independent radiometric calibration of the Honor tele path;
- exact MotionCam NoiseProfile channel/coordinate semantics;
- physical cause of the discrete exact-ISO8192-associated sample domain;
- per-surface absolute irradiance from ordinary single-frame capture;
- sufficiently constrained geometry/material/light decomposition for general scenes;
- full PTC/conversion-gain/read-noise calibration across all domains;
- calibrated optical inverse/PSF/MTF path.

### BLOCKED without new evidence

- generic `FULL_PHYSICAL` light/irradiance field for arbitrary scenes;
- exact full-spectrum illumination/material recovery from one ordinary three-channel RAW;
- treating a counterfactual relight as observed lighting.

---

## 18. Scientific references used for this architecture

Authoritative terminology / calibration sources:

- CIE S 017 e-ILV, **irradiance**: incident radiant flux density at a surface point: `https://cie.co.at/eilvterm/17-21-053`
- CIE S 017 e-ILV, **radiance**: directional radiometric quantity at a point/projected area: `https://www.cie.co.at/eilvterm/17-21-049`
- CIE S 017 e-ILV, **illuminance**: incident luminous flux density: `https://cie.co.at/eilvterm/17-21-060`
- NIST Handbook 152, radiometric sensor calibration terminology including offset, nonlinearity, flat-field and radiance/irradiance responsivity: `https://nvlpubs.nist.gov/nistpubs/Legacy/hb/nisthandbook152.pdf`
- NIST Handbook 157, radiometric calibration guidance for electro-optical instruments: `https://doi.org/10.6028/NIST.HB.157`
- EMVA 1288 Release 4.0 Linear, controlled temporal variance / photon-transfer characterization: `https://www.emva.org/wp-content/uploads/EMVA1288Linear_4.0Release.pdf`

Inverse-rendering identifiability context:

- Zhang et al., CVPR 2022, *Modeling Indirect Illumination for Inverse Rendering*: full inverse rendering is severely ill-posed and requires priors/constraints: `https://openaccess.thecvf.com/content/CVPR2022/papers/Zhang_Modeling_Indirect_Illumination_for_Inverse_Rendering_CVPR_2022_paper.pdf`
- Enyo & Nishino, CVPR 2024, *Diffusion Reflectance Map*: single-image illumination/reflectance recovery remains a blind inverse problem even with known geometry: `https://openaccess.thecvf.com/content/CVPR2024/html/Enyo_Diffusion_Reflectance_Map_Single-Image_Stochastic_Inverse_Rendering_of_Illumination_and_CVPR_2024_paper.html`

These external sources support terminology and scientific limitations. They do not by themselves validate TruthRaw’s implementation.

---

## 19. Permanent design sentence

**The nul-lijn defines an unbounded scene address space; MeasurementLab determines how the finite sensor observation entered that space; FotoGraaf Scene Metrology describes where captured scene/light evidence and uncertainty are supported; the Scientific Master remains the best-supported reconstructed scene; RoomCapsule and LightingStudio may explore local hypothetical lighting without rewriting the captured world; and HDR appearance only decides how a finite output displays that state.**
