# D.RAW Interdisciplinary Scientific Decision Foundation v0.1

**Status: RESEARCH / GOVERNANCE FOUNDATION — NOT A RUNTIME PROMOTION**  
**Applies to:** D.RAW / TruthRaw architecture, Scientific Master, TruthNegative, Free Raster, Observation Graph, Appearance and future restoration/world-space work.  
**Core law:** **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**  
**Companion law:** **Measured where measured. Reconstructed where necessary. Never invented.**  
**Project motto:** **One Free World. Many sealed observations. One evidence law.**

This document synchronizes the project with university-level and standards-based knowledge from photography/computational imaging, radiometry and physically based rendering, human vision and color appearance, 3D geometry/depth, temporal sampling/stop-motion, artificial lighting, architecture/photogrammetry and conservation/restoration.

It does **not** make any candidate scientific runtime authoritative, does not modify sealed evidence, does not permit Scientific Master writeback from Appearance, and does not relax existing promotion firewalls.

---

## 1. The project must keep four spaces distinct

Every implementation decision must state which space it operates in.

### A. Observation / measurement space

What the physical capture actually contains: sensor samples, CFA/readout topology, capture interval, exposure/gain, metadata, clipping/censoring, optics and calibration records where available.

Authority originates here. Missing facts remain UNKNOWN.

### B. Scientific inferred space

Calibrated estimates and reconstructions derived from admitted evidence. This includes Scientific Master and per-domain uncertainty/authority. Numerical sophistication never increases evidence authority by itself.

### C. World / geometry / light-transport space

Relations among observations, geometry, visibility, surfaces, illumination and light transport. A physically plausible world is not automatically a proven world. Geometry, material and illumination can be mutually ambiguous and must retain separate authority and uncertainty.

### D. View / Appearance space

Viewing transforms, tone mapping, display adaptation, user intent, free raster placement, zoom/pan, perceptual rendering, artistic restoration preview and export appearance. This space may be highly flexible but may never write new MEASURED authority upstream.

A transform may preserve authority, reduce authority or create a derived estimate. It may never silently increase authority.

---

## 2. Photography and sensor image formation

A camera sample is not simply a scene pixel. It is the result of scene radiance and light transport filtered/integrated by optics, aperture, focus, exposure interval, sensor spectral response, CFA/sample topology, readout/electronics, noise and quantization.

D.RAW therefore treats lens, sensor, CFA, shutter/readout and exposure as the **Observation Procedure**, not as cosmetic metadata.

Consequences:

- RAW values are evidence from a measurement system, not direct object reflectance or direct human brightness.
- PSF/MTF, diffraction, aberrations, defocus and motion integration limit what spatial detail one observation can support.
- Noise and censoring are measurement-state properties and must remain visible to later reconstruction.
- Demosaicing, denoising, deconvolution, super-resolution and restoration create derived values unless a specific target sample is directly supported by sealed measurement.
- A larger output raster may carry a finer representation but cannot create finer measured evidence.

Primary academic anchors include Stanford EE367/CS448I Computational Imaging and Stanford computational photography material.

References:
- https://web.stanford.edu/class/ee367/
- https://graphics.stanford.edu/courses/cs478/
- https://gfxcourses.stanford.edu/cs348k/spring26content/media/camera1/02_camerapipeline.pdf

---

## 3. Radiometry, light transport and 3D rendering

Physically based rendering models image formation through radiance transport, emitted light, reflection/scattering, geometry, visibility and direction. This is extremely useful to D.RAW because it makes one fact explicit: the same observed image can often be explained by multiple combinations of geometry, material and illumination.

Therefore inverse reconstruction is generally underdetermined.

D.RAW rules:

- Forward light transport may be used to test physical consistency.
- Inverse light transport must expose ambiguity and uncertainty.
- `physically_plausible` is not equivalent to `MEASURED` or `proven`.
- BRDF/BSDF, surface normal, depth, illumination and visibility authority are separate domains.
- A photorealistic render is an Appearance result unless its scientific quantities are separately evidence-bound.
- Monte Carlo accuracy or numerical convergence proves a renderer calculation, not that the assumed world model is true.

Academic anchors:
- Cornell CS5630 Physically Based Realistic Rendering: radiometry, colorimetry, radiative transfer and light transport.
- Cornell Program of Computer Graphics global-illumination/light-transport work.

References:
- https://www.cs.cornell.edu/courses/cs5630/2026sp/
- https://en-cg-web.coecis.cornell.edu/research/globillum/transport.html

---

## 4. Human vision and Appearance

Human visual perception is adaptive and contextual. Perceived brightness, blackness, whiteness, contrast and color appearance depend on surround, adaptation, field size, luminance and viewing conditions. Depth perception also combines multiple cues.

This is a reason to keep Appearance powerful **and** downstream.

D.RAW rules:

- Scientific Master is not optimized to "look right" by silently changing physical state.
- Display/view adaptation belongs to Appearance unless a separate calibrated measurement transformation is explicitly defined.
- Color appearance and colorimetry are not interchangeable concepts.
- A display preview may adapt for viewing conditions while preserving the upstream scene-referred/scientific state.
- Perceptual enhancement may guide a user, but cannot become evidence.

CIECAM16 explicitly models color appearance as viewing-condition dependent. NIST photometry explicitly incorporates human visual sensitivity, while radiometry is the broader physical measurement of optical radiation.

References:
- https://www.cie.co.at/publications/cie-2016-colour-appearance-model-colour-management-systems-ciecam16
- https://www.cie.co.at/publications/colorimetry-4th-edition
- https://www.nist.gov/programs-projects/photometry
- https://ocw.mit.edu/courses/9-011-the-brain-and-cognitive-sciences-i-fall-2002/pages/study-materials/

---

## 5. Black and white are not single scientific values

D.RAW must never use an unqualified word `black` or `white` where multiple meanings are possible.

At minimum these concepts stay separate:

| Term | Meaning | Scientific role |
|---|---|---|
| sensor black level | electronic/readout offset estimate | calibration / measurement model |
| dark/noise floor | uncertainty-dominated lower measurement region | measurement bound |
| zero physical radiance | theoretical absence of incoming radiance | physical concept; not identical to code value 0 |
| TruthRange Zero-Line | project gauge `T = log2(L/L0)` | radiometric coordinate reference; not sensor/display black |
| clipping / sensor white level | upper censor/saturation boundary | CENSORED measurement state |
| reference diffuse white | defined calibration/reference condition | calibration/colorimetry |
| adopted/adapted white | white used/judged under capture or viewing adaptation | color appearance/viewing condition |
| display black / display white | output-device endpoints | Appearance |
| artistic black/white point | user rendering choice | Appearance |

CIE explicitly notes that adopted/adapted white depends on viewing/capture context and should not be equated blindly with a physical reflecting sample.

References:
- https://cie.co.at/eilv/1428
- https://www.cie.co.at/eilvterm/17-23-081

---

## 6. Depth and geometry need source-specific authority

Depth is not one signal. Human and computational systems can use stereopsis/disparity, motion parallax, perspective, occlusion, shading, texture gradients, focus/defocus, known geometry and active ranging. These cues have different failure modes and different evidence strength.

D.RAW depth authority must therefore describe **how** depth was obtained.

Examples:

- sealed active depth/range data: potentially MEASURED in its own calibrated domain;
- calibrated stereo/multi-view triangulation: CALIBRATED_ESTIMATE with geometry and correspondence uncertainty;
- depth from focus/defocus: derived estimate dependent on optical/PSF model and scene assumptions;
- shape from shading / photometric stereo: reconstructed estimate dependent on illumination/material assumptions;
- monocular semantic/perspective cues: inference, never silently MEASURED.

A depth map without method, calibration identity, uncertainty and source footprint is scientifically incomplete.

References:
- https://ocw.mit.edu/courses/9-011-the-brain-and-cognitive-sciences-i-fall-2002/pages/study-materials/
- https://web.media.mit.edu/~raskar/photo/Sig06Course15ComputationalPhotography/May06Course15NotesComputationalPhotoFull.pdf

---

## 7. Stop-motion, video and temporal truth

A camera frame is an **integration over a finite exposure interval**, not an infinitely thin instant. A sequence then samples time discretely. Motion blur and temporal aliasing are therefore two parts of temporal sampling.

D.RAW consequences:

- `capture_time` and where possible `exposure_interval` belong to the Observation Contract.
- rolling-shutter timing is part of measurement geometry, not a display defect only.
- a single frame cannot prove arbitrary continuous object motion between exposures.
- stop-motion frames remain distinct sealed observations.
- generated in-between motion, synthetic motion blur or temporal interpolation is RECONSTRUCTED/APPEARANCE unless a stronger admitted temporal measurement supports it.
- temporal reconstruction must not rewrite original frame evidence.

Academic anchors include MIT Computational Photography material on motion blur/temporal sampling and university graphics teaching on temporal aliasing.

References:
- https://people.csail.mit.edu/fredo/comp-photo-book/12-video-01-motion-blur-and-temporal-sampling.html
- https://visionbook.mit.edu/sampling_and_aliasing.html
- https://www.irfanessa.gatech.edu/paper-acmeg-sca-2001-image-based-motion-blur-for-stop-motion-animation/

---

## 8. Artificial illumination is a physical state, not just white balance

Artificial and mixed illumination can vary in spectral power distribution, intensity, direction, solid angle/source size, distance, polarization, temporal modulation/flicker and indirect transport from the environment.

Consequences:

- white balance cannot by itself recover an unknown illumination spectrum;
- identical RGB appearance does not prove identical spectra, reflectance or illumination;
- direct light, indirect light, emitted light and reflected light must not be collapsed into one authority label;
- mixed illumination may require local/field-dependent modeling;
- temporal light modulation can interact with exposure and rolling shutter;
- estimated illumination belongs to scientific inferred/world space with uncertainty; creative relighting belongs to Appearance.

CIE color-appearance work explicitly treats chromatic adaptation and viewing illumination as condition-dependent.

References:
- https://www.cie.co.at/publications/chromatic-adaptation-under-mixed-illumination-condition-when-comparing-softcopy-and
- https://www.cie.co.at/publications/colorimetry-4th-edition
- https://www.nist.gov/programs-projects/photometry

---

## 9. Architecture, photogrammetry and world geometry

Architecture is especially valuable for D.RAW because scenes can contain strong geometric relations: planes, parallel/perpendicular lines, repeated dimensions, known vertical direction, scale references and stable surfaces. These constraints can improve calibration and reconstruction, but they remain assumptions/priors unless observed or independently measured.

D.RAW rules:

- projective camera geometry stays separate from Appearance perspective correction;
- lens distortion calibration and world geometry must retain their calibration identity;
- architectural regularity may constrain a solution but may not overwrite contradictory evidence;
- photogrammetric/3D products retain source-image provenance, camera/calibration state, uncertainty and coordinate-frame authority;
- visually rich 3D representations (including neural/point/splat renderers) are not automatically metrologically authoritative.

Recent TU Delft architectural-heritage work is a useful example: photorealistic 3D Gaussian Splatting and LiDAR/point-cloud workflows have complementary strengths, with visualization quality not identical to structural/geometric measurement authority.

References:
- https://research.tudelft.nl/en/publications/from-comparison-to-integration-a-workflow-evaluation-of-3d-gaussi/
- https://www.ucl.ac.uk/centre-applied-archaeology/shanxi/manual

---

## 10. Restoration and conservation provide a model for scientific image restoration

Conservation practice distinguishes documentation, preservation/conservation and restoration. Restoration is an intervention intended to improve appreciation/understanding and should remain respectful of original material; heritage practice emphasizes documentation, minimal intervention, distinguishability and, where possible, reversibility/re-treatability.

This maps unusually well to D.RAW.

D.RAW restoration law:

- the sealed observation is the original material and is immutable;
- every intervention produces a new derived artifact/state rather than rewriting the source;
- the intervention method, parameters, mask/footprint, inputs and outputs are recorded;
- restored/reconstructed regions remain distinguishable by authority even when visually seamless;
- minimum intervention is preferred: do not reconstruct where evidence is already sufficient;
- reversibility means the user can return to the upstream state and reproduce/remove the restoration pass;
- future improved methods may supersede a pass without erasing the historical pass or source;
- visual plausibility is never permission to relabel restoration as MEASURED.

References:
- https://www.icom-cc.org/en/terminology-for-conservation
- https://www.getty.edu/conservation-institute/buildings-and-sites/recording-and-documentation/
- https://www.getty.edu/projects/seismic-retrofitting/case-study-ica-cathedral/

---

## 11. Free Raster must distinguish representation resolution from evidence resolution

The Free Raster can evaluate or present a continuous scientific representation at arbitrary finite x/y positions and output resolutions. That freedom is representational.

It must not imply that a 4K -> 8K raster transition doubled measured detail.

Required concepts for future runtime binding:

- source observation footprint;
- source sample/evidence density where meaningful;
- output raster dimensions;
- projection/sampling kernel identity;
- mapping from output samples to scientific support;
- authority and uncertainty propagation;
- distinction between direct/evidence-supported sample contribution and reconstruction;
- display/view scale separately from scientific/output sampling scale.

**Important Workspace v0.1 implication:** a UI button labelled `1:1` is only scientifically unambiguous if it states *which raster* is one-to-one with display pixels. If the canvas uses a downsampled preview bitmap, the control must be understood/labeled as preview-raster 1:1, not source-measurement 1:1.

---

## 12. Observation Graph and multi-observation fusion

Multiple observations can constrain one physical world while remaining separate evidence objects.

Before any fusion, D.RAW must preserve:

- Observation ID and sealed source identity;
- physical frame/capture identity;
- capture time/interval;
- camera/lens/sensor/readout procedure;
- coordinate-frame relation and its uncertainty;
- radiometric gauge relation and its uncertainty;
- overlap/source footprint;
- domain-specific authority;
- censoring/unknown state.

A relation can be strong in geometry and weak in radiometry, or vice versa. Authority is therefore per domain, not a single global confidence score.

**Many observations do not automatically make one measurement. Proven relations make a constrained world model.**

---

## 13. Mandatory decision gate for future project changes

Before implementation or promotion, every non-trivial scientific or rendering change must answer:

1. Which of the four spaces does this change operate in?
2. What sealed observation(s) support it?
3. What quantity is directly measured, calibrated, reconstructed, censored, unknown or appearance-only?
4. Does the transform preserve, reduce or derive authority?
5. What uncertainty/bounds are propagated?
6. Which optics, temporal, geometric, radiometric, color and illumination assumptions are required?
7. Could human perception or viewing conditions be confused with physical measurement?
8. Is any black/white/depth/light term ambiguous and therefore in need of a qualified definition?
9. Is the intervention reversible and fully provenance-bound?
10. Can the same output be produced without mutating sealed evidence or Scientific Master authority?
11. What physical/real-device test could falsify the claim?
12. What exact promotion evidence is required before the candidate may affect canonical scientific state?

If these questions cannot be answered, the change remains fail-closed / UNKNOWN / candidate-only as appropriate.

---

## 14. Immediate implications for PR #131 Workspace / Free Raster v0.1

PR #131 is correctly positioned as a downstream product/UI layer because it does not decode RAW locally, create a second Scientific Master, or infer authority from presentation state.

The next safe product steps are therefore:

- bind Workspace to the **existing** D.RAW Unified Output / projection runtime rather than creating a second renderer;
- expose source/output raster identity so `Fit`, `preview 1:1`, scientific output resolution and source sampling cannot be confused;
- keep pan/zoom explicitly `PRESENTATION_ONLY`;
- later bind Evidence/Authority Inspector only to source/runtime records, never to UI inference;
- represent observation relations as graph edges with per-domain authority and uncertainty;
- keep Appearance controls viewing-condition/perceptual operations downstream;
- keep restoration as reproducible derived passes with immutable upstream state.

No item above authorizes candidate promotion.

---

## 15. Source hierarchy for future decisions

When a design decision depends on external scientific knowledge, prefer in this order:

1. standards/metrology bodies (CIE, NIST, ISO where accessible and applicable);
2. primary peer-reviewed literature and university research groups;
3. university courses/texts from established imaging/vision/graphics/conservation programs;
4. vendor documentation only for device-specific behavior;
5. community material only as exploratory context, never as sole scientific authority.

External knowledge can improve models and tests. It cannot retroactively become evidence about a particular sealed observation.

---

## 16. Non-negotiable firewall

Nothing in this document changes the established project safety state:

- sealed Source Evidence remains immutable;
- MEASURED authority originates only from admitted evidence;
- CALIBRATED_ESTIMATE and RECONSTRUCTED remain distinguishable;
- CENSORED and UNKNOWN remain valid scientific states;
- Appearance never writes scientific authority upstream;
- Exact Gauge / canonical contracts remain untouched unless separately validated and promoted;
- AI/ML is not a scientific inference authority layer;
- candidate implementations remain candidates until the required tests and promotion process succeed.

**Final rule:** D.RAW may become richer in representation, geometry, rendering and user control without becoming richer in claimed knowledge than its evidence permits.
