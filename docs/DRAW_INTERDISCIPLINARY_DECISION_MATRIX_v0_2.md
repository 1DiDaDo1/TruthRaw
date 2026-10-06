# D.RAW Interdisciplinary Decision Matrix v0.2

**Status:** research/governance decision layer — no scientific promotion  
**Date:** 2026-10-06  
**Continuation code:** `44489`  
**Companion:** `docs/DRAW_INTERDISCIPLINARY_SCIENTIFIC_DECISION_FOUNDATION_v0_1.md`

## Purpose

This document turns the interdisciplinary foundation into a mandatory decision matrix for concrete D.RAW implementation choices. It is not observation-specific evidence and cannot promote a runtime candidate.

Permanent law:

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

`Representation may become richer than the source; knowledge claims may never become richer than evidence.`

`Stable outside. Flexible inside. Evidence law unchanged.`

`One Free World. Many sealed observations. One evidence law.`

## 1. Photography / RAW / CFA / sensor metrology

University computational-imaging material treats a camera image as the result of optics, aperture/focus, exposure integration, sensor sampling/noise, ISP operations and inverse problems rather than a direct copy of the scene.

D.RAW consequence:

- sealed source samples and their sample geometry remain primary evidence;
- demosaic, denoise, deconvolution, restoration and free-raster resampling are derived operations unless a target quantity is directly measured;
- a larger raster is a richer representation, not additional measured spatial detail;
- full-resolution JPEG must render from the admitted source/scientific cable, never from a small UI preview bitmap;
- preview is a sibling downstream adapter, not a prerequisite or evidence source for full-resolution output.

Reference anchors:

- Stanford EE367 / CS448I Computational Imaging: https://web.stanford.edu/class/ee367/
- Stanford digital camera pipeline / computational imaging teaching: https://gfxcourses.stanford.edu/

## 2. Optics / PSF / MTF / resolution

Optical support is bounded by aperture, diffraction, aberration, defocus, motion integration and sensor sampling. Sharpening or increased raster density cannot create optical authority.

D.RAW consequence:

- source-supported spatial bandwidth stays distinct from output raster resolution;
- `Preview 1:1` means one display pixel per decoded preview pixel only;
- output sharpening/detail is `APPEARANCE` unless a separately admitted scientific deconvolution contract exists;
- Free Raster may sample a continuous representation at arbitrary finite coordinates without relabelling those positions `MEASURED`.

## 3. Radiometry / physically based rendering / 3D animation

Physically based rendering models radiance transport through geometry, visibility, emission, reflection/scattering and illumination. Inverse image formation is generally underdetermined: several worlds may explain a similar image.

D.RAW consequence:

- physically plausible is not equivalent to proven;
- geometry, material, illumination, visibility and radiometry carry separate authority/uncertainty;
- forward rendering can test consistency but cannot manufacture observation evidence;
- 3D/animation render quality belongs to representation/Appearance unless its world quantities are independently evidence-bound;
- Room Capsule and light-transport stages remain exact-preserving bypasses when admitted world evidence is absent.

Reference anchor:

- Cornell CS5630 Physically Based Realistic Rendering: https://www.cs.cornell.edu/courses/cs5630/2026sp/

## 4. Human vision / lightness / colour appearance

Human vision is adaptive and contextual. Perceived lightness, contrast, colour, depth and detail depend on surround, adaptation, luminance, spatial scale and multiple depth cues.

D.RAW consequence:

- the Scientific Master is not silently changed to make an image look right;
- display adaptation, tone mapping, local contrast, warmth and colourfulness remain downstream Appearance;
- perceptual quality can guide output design but never upgrade scientific authority;
- an attractive preview is not evidence that a physical reconstruction is correct.

Reference anchors:

- MIT OpenCourseWare Perception / Visual System: https://ocw.mit.edu/courses/9-35-perception-spring-2024/
- MIT Sensory Systems depth/vision material: https://ocw.mit.edu/courses/9-04-sensory-systems-fall-2013/

## 5. Depth / projective geometry

Depth can arise from stereopsis, motion parallax, perspective, occlusion, shading, focus/defocus, known geometry or active range sensing. These cues have different assumptions and authority.

D.RAW consequence:

- a depth product must name its method, calibration identity, source footprint and uncertainty;
- active calibrated ranging may be `MEASURED` in its own domain;
- calibrated multi-view triangulation is generally `CALIBRATED_ESTIMATE`;
- shape-from-shading/focus and similar inverse methods remain reconstructed/estimated;
- monocular appearance cues never become silently measured geometry;
- observation-graph edges must preserve per-domain geometric and radiometric authority separately.

Reference anchor:

- MIT depth-perception teaching: https://ocw.mit.edu/courses/9-04-sensory-systems-fall-2013/resources/lec-7-depth-perception/

## 6. Stop-motion / temporal sampling / rolling shutter

A camera frame integrates over a finite exposure interval and a sequence samples time discretely. Motion blur and temporal aliasing are coupled consequences of temporal integration/sampling.

D.RAW consequence:

- capture time and exposure interval belong to observation provenance where available;
- rolling-shutter timing is measurement geometry, not merely a display defect;
- stop-motion frames remain separate sealed observations;
- interpolated in-betweens or synthetic motion blur are `RECONSTRUCTED`/`APPEARANCE` unless supported by stronger temporal measurements;
- temporal processing never rewrites original frame evidence.

Reference anchor:

- MIT Computational Photography — Motion blur and temporal sampling: https://people.csail.mit.edu/fredo/comp-photo-book/12-video-01-motion-blur-and-temporal-sampling.html

## 7. Black / white / clipping / display endpoints

The words black and white are ambiguous and must be qualified.

D.RAW keeps separate:

- sensor black level / electronic offset;
- noise floor / uncertainty-dominated dark region;
- physical zero radiance;
- TruthRange Zero-Line / project radiometric gauge;
- sensor WhiteLevel / clipping/censoring boundary;
- reference diffuse white;
- adopted/adapted white;
- display black and display white;
- artistic black/white points.

D.RAW consequence:

- display black/white-point controls are `APPEARANCE_ONLY`;
- they may not rewrite sensor BlackLevel, WhiteLevel, clipping or Zero-Line;
- highlight roll-off may map censored/bright regions for viewing but may not invent exact values behind censoring.

Reference anchors:

- CIE International Lighting Vocabulary adopted/adapted white: https://cie.co.at/eilvterm/17-32-058 and https://cie.co.at/eilvterm/17-23-081
- NIST Photometry: https://www.nist.gov/programs-projects/photometry

## 8. Artificial illumination / spectra / flicker

Illumination has spectral, directional, spatial and temporal structure. White balance alone does not identify an unknown spectrum, and equal RGB values do not prove equal spectra or reflectance.

D.RAW consequence:

- illumination spectrum, intensity, direction, source extent, distance, polarization and temporal modulation remain separate when relevant;
- mixed illumination can require local/field-dependent authority;
- flicker can interact with exposure and rolling-shutter readout;
- estimated illumination remains scientific-inferred/world state with uncertainty;
- creative relighting remains Appearance.

## 9. Architecture / photogrammetry / built heritage

Architecture provides useful geometric constraints such as planes, parallelism, orthogonality, repeated dimensions and stable surfaces, but architectural regularity is a prior unless measured or independently established.

D.RAW consequence:

- projective geometry stays separate from Appearance perspective correction;
- lens calibration and world geometry retain distinct calibration identity;
- visual 3D representations are not automatically metric models;
- photogrammetry products retain observation provenance, camera/calibration state, coordinate-frame authority and uncertainty;
- 3D Gaussian Splatting can be a strong visualization layer while LiDAR/metric capture may carry stronger structural measurement authority; neither is silently substituted for the other.

Reference anchors:

- TU Delft architectural heritage, 3DGS + LiDAR workflow: https://research.tudelft.nl/en/publications/from-comparison-to-integration-a-workflow-evaluation-of-3d-gaussi/
- UCL historic-building digital documentation manual: https://www.ucl.ac.uk/centre-applied-archaeology/shanxi/manual

## 10. Restoration / conservation / reversibility

Conservation practice emphasizes documentation, distinguishability of intervention, evidence-informed treatment and the ability to understand or retreat previous interventions.

D.RAW consequence:

- sealed observations are immutable originals;
- every restoration/edit is a derived, provenance-bound intervention;
- method, parameters, footprint/mask, input identity and output identity are recorded;
- reconstruction remains distinguishable by authority even when visually seamless;
- minimum intervention: do not reconstruct where valid measured support already suffices;
- reset/re-render begins from the immutable source, never from a previously re-encoded edited JPEG;
- export creates a new derivative and never overwrites the source by default.

Reference anchors:

- Getty Conservation Institute Recording and Documentation: https://www.getty.edu/conservation-institute/buildings-and-sites/recording-and-documentation/
- ICOM-CC conservation terminology: https://www.icom-cc.org/en/terminology-for-conservation

## 11. Mandatory decision gate

Every non-trivial project change must answer before implementation/promotion:

1. Which space is affected: observation, scientific inferred, world/light transport, or view/appearance?
2. Which sealed observation(s) support the operation?
3. Which quantities are MEASURED, CALIBRATED_ESTIMATE, RECONSTRUCTED, CENSORED, UNKNOWN or APPEARANCE?
4. What provenance and uncertainty survive the cable boundary?
5. Does the operation preserve/reduce/derive authority, and can it accidentally increase authority?
6. Which optics, radiometry, temporal, geometry, colour/vision, illumination or material assumptions are present?
7. Is any black/white/depth/light term ambiguous and therefore insufficiently qualified?
8. Is the change non-destructive and reversible where it is an intervention/edit?
9. Can an internal implementation later be replaced without changing outer evidence semantics?
10. What real-device or physical observation could falsify the claim?
11. What exact evidence would be required before scientific promotion?

If these questions cannot be answered, the change remains candidate-only, `UNKNOWN` or fail-closed as appropriate.

## 12. Current application: full-resolution JPEG output cable

The current UI contains PURE, ADVANCED and PRO JPEG actions, but legacy MainActivity gating still requires `TilePreviewUiState.Ready` before launching/committing a full-resolution JPEG. That requirement is an implementation coupling, not a scientific requirement.

Correct architecture:

`sealed DNG -> Universal Intake -> Scientific Master / admitted full-res renderer -> route/output recipe -> JPEG_FULL_RES adapter -> new derivative`

Parallel sibling:

`same admitted source/output state -> PREVIEW adapter -> display bitmap`

Therefore:

- preview may visualize the route but may not be the pixel source or authority gate for full-resolution JPEG;
- full-resolution JPEG must revalidate exact source/job/route binding independently;
- PURE uses a neutral/no-appearance output recipe;
- ADVANCED uses reversible downstream appearance settings;
- PRO may only differ where an explicitly admitted PRO output recipe exists; a label alone does not create a distinct renderer;
- the saved JPEG may be decoded afterward into a small result preview, but that post-save preview is presentation feedback only;
- source mutation, Scientific Master writeback and new-evidence creation remain false.

Runtime contract introduced for this boundary:

`suite_android/app/src/main/java/com/truthraw/adaptiveui/DrawPhotoOutputCableV01.kt`

Its `PREVIEW_REQUIRED=false` state is deliberate: a full-resolution output is bound to the source/scientific/output cable, not to a UI preview bitmap.
