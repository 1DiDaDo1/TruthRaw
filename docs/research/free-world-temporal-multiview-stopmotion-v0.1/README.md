# D.RAW Free-World Temporal / Multi-View / Stop-Motion v0.1

Status: **ARCHITECTURE RESEARCH CONTRACT — NOT A MULTI-FRAME SCIENTIFIC-MASTER PROMOTION**

## Purpose

D.RAW keeps the sealed single-frame Direct-CFA evidence contract intact while allowing the Open Free World above TruthNegative to ingest additional observations as separately identified evidence or hypotheses.

The central rule is:

> Representation and scene understanding may grow without bound; an evidence claim may grow only when a new observation is explicitly admitted with its own provenance.

The existing single-frame Scientific Master is never silently replaced by a burst, stop-motion sequence, photogrammetric model, optical-flow estimate, or rendered frame.

## Why stop-motion belongs in the Free World

Stop-motion and motion-control photography expose useful ideas that a single raster hides:

- repeated observations of a scene under controlled camera poses;
- frame-to-frame object motion as explicit state rather than blur;
- clean plates and repeatable lighting/effects passes;
- sub-pixel camera displacement;
- parallax between separated scene planes;
- temporal ordering and persistent object identity;
- controlled focus and viewpoint sweeps.

These are not loopholes for inventing evidence. They are additional observation dimensions.

## Observation classes

### 1. Sealed single-frame observation

The canonical D.RAW source remains one physical frame / one independent evidence item.

It produces the Scientific Master, Open Scene authority field and TruthNegative Continuous state.

### 2. Admitted temporal observation set

A future sequence may contain multiple independently sealed frames.

Each frame needs its own:

- source SHA-256;
- timestamp / ordering;
- camera/lens/readout identity;
- exposure and calibration metadata;
- physical-frame identity;
- authority field;
- censor/unknown state.

A temporal solver may establish correspondences between frames but may not rewrite the individual source records.

### 3. Multi-view / photogrammetric observation set

Known or solved camera poses can constrain geometry, depth, occlusion and visibility.

Geometry authority remains separate from radiometric authority. A high-confidence 3D surface does not make an UNKNOWN or CENSORED colour channel measured.

### 4. Motion-control / stop-motion observation set

Repeatable camera paths and controlled object poses can be treated as a calibrated acquisition manifold. Clean plates, lighting passes, focus stacks and stereo/multi-view passes may share a coordinate system while retaining separate evidence identities.

### 5. Counterfactual animation state

A user-created pose, camera path, material, light, deformation or inserted object belongs to the Counterfactual Scene. It can be rendered freely but never increases measured-evidence counts.

## Free-World state axes

The Free World should be able to address a scene as a function of more than x/y:

    S(x, y, z, t, view, direction, wavelength?, focus, exposure, ...)

No dimension is automatically claimed measured. Each field carries authority, uncertainty and provenance.

Useful future dimensions include:

- continuous image-plane position;
- depth / geometry;
- time / frame phase;
- camera pose and ray direction;
- object identity and deformation state;
- focus / aperture observation;
- illumination pass identity;
- spectral hypothesis identity.

## What stop-motion can improve

### Occlusion and hidden-surface reasoning

As an object or camera moves, previously hidden boundaries may become visible. Those newly observed surfaces can become evidence only from the frames in which they are actually visible.

### Geometry and depth

Parallax across controlled poses can constrain 3D structure more strongly than a single image. This is particularly useful for droplets, glass edges, fine foreground/background separation and restoration topology.

### Sub-pixel spatial support

Small controlled or natural shifts can provide independent sampling phases. A future multi-frame reconstruction can use them for denoising and super-resolution while recording exactly which frames contributed to each estimate.

This must remain separate from the current one-frame Camera-5 Scientific Master.

### Motion as structure

Stop-motion makes object pose explicit at discrete instants. D.RAW can preserve those states and interpolate a continuous temporal hypothesis without claiming the interpolated instants were photographed.

### Clean plates and controlled passes

Repeated camera motion can acquire background-only, alternate-lighting, polarization, focus, or exposure observations. These can constrain scene decomposition when separately sealed and calibrated.

### Multiplane reasoning

Animation's multiplane technique is a useful conceptual model for separating foreground, subject, background and atmosphere. D.RAW can generalize this from a few planes to continuous/deep geometry with authority per contribution.

## Connection to the current pipeline

Current production-oriented PRO path:

    Direct CFA
      -> Scientific Master
      -> Open Scene authority
      -> TruthNegative Continuous
      -> target footprint query
      -> TruthNegative Deep Scene Bridge v0.8
      -> evidence-constrained camera-plane Deep Scene contribution
      -> Appearance / Display

Future optional Free-World branches:

    additional sealed frames
      -> temporal / multi-view observation graph
      -> geometry / motion / visibility constraints
      -> Deep Scene contributions

    Deep Scene + admitted/inferred material + illumination
      -> Light Transport State
      -> path/integrand contributions
      -> Deep resolve
      -> Appearance / Display

The current Light Transport v0.6 remains a bounded one-direction Lambertian integrand and must not be presented as a solved rendering equation.

## Authority rules

1. Never merge physical-frame identities merely because frames align.
2. Never promote interpolation to photographed evidence.
3. Never promote photogrammetric geometry to radiometric measurement.
4. Never use a counterfactual animation frame to repair Scientific Master.
5. Multi-frame super-resolution may increase reconstruction support, not retroactively increase the measured photosite count of any source frame.
6. A hidden surface becomes measured only through an admitted observation that actually saw it.
7. Appearance and display remain downstream and write back nowhere.

## Research directions

- temporal observation graph with per-frame SHA-256 ancestry;
- sub-pixel CFA phase/support map;
- calibrated camera-pose manifold;
- deep visibility/occlusion graph;
- object-persistent IDs across time;
- focus-stack / aperture-stack observation support;
- clean-plate and controlled-illumination passes;
- non-rigid scene state for moving/deforming subjects;
- light-field-style ray/view representation where acquisition supports it;
- temporal appearance coherence without scientific writeback.

## External knowledge anchors

The architecture is informed by established ideas in RAW burst super-resolution, light-field photography, photogrammetry, motion studies, motion-control photography and multiplane animation. These are references for acquisition and representation ideas, not limits on D.RAW's internal state space.
