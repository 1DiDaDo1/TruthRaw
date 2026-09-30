# D.RAW Observation-World Field Separation v0.1

Date: 2026-09-30

Status: **research-only, identity-independent coordinate/authority foundation, no automatic world registration, no calibration/correction/writeback**

## Why this exists

The first real multi-scene Field Response Repeatability result showed that ordinary scenes do not yet isolate a stable radial field response strongly enough to justify a light-falloff or lens correction.

At the same time, D.RAW should not require every user to perform a camera-specific flat-field calibration, nor should it hardcode the Honor Magic 8 Pro or any other device.

The next layer therefore separates three spaces explicitly:

1. SOURCE / SENSOR SPACE
2. WORLD / SCENE SPACE
3. VIEW / OUTPUT SPACE

This is the foundation for a future universal natural self-calibration path.

## Core law

> **A field behaviour may be called sensor-fixed only after it remains tied to source/sensor coordinates while independently related world structure moves across those coordinates.**

And conversely:

> **A feature may be called world-fixed only after a valid inter-observation geometry relates it across different source/sensor positions.**

v0.1 does not yet perform that registration. It freezes the authority boundaries first.

## SOURCE / SENSOR SPACE

This space contains only observation-relative measurement geometry.

Where available it includes:

- exact source raster/sample geometry;
- PR96 rho/azimuth/radial/tangential basis;
- CFA phase;
- measured composite field signal;
- source SHA-256 provenance.

It does not require:

- camera model name;
- lens model name;
- vendor identity;
- device profile.

The measured PR96 signal remains:

`MEASURED_SOURCE_SAMPLES_SCENE_LENS_SENSOR_COMPOSITE`

It is not re-labelled as lens vignetting.

## WORLD / SCENE SPACE

This space is for relations between structure seen in multiple observations.

v0.1 records whether each frontside observation has enough deterministic visible structure for later classical pair matching.

However:

- the existing structural signature hash is **not** registration proof;
- dominant edge orientation is **not** registration proof;
- visual similarity is **not** relation proof;
- user grouping is **not** world geometry;
- absolute metric scale is not assumed.

The current registration status is explicitly:

`UNREGISTERED_V0_1`

## VIEW / OUTPUT SPACE

This is a derived display/projection space.

Examples:

- rendered D.RAW view;
- JPEG;
- panorama projection;
- 360° equirectangular view;
- final finite output raster.

View/output coordinates never become sensor calibration coordinates.

A stitched panorama is therefore not source evidence.

## 360° sequences

A sequence of original sealed observations may become useful for natural self-calibration because the same world structure can move through different parts of source/sensor space.

The protocol explicitly records:

- `360_observation_sequence_allowed=true`;
- `single_stitched_360_image_sufficient=false`;
- `original_individual_observations_required_for_measured_authority=true`.

The person holding the camera is not the scientific origin.

The centre of a panorama is not the calibration origin.

The future world graph is relative geometry between observations.

## Natural self-calibration target

The future target is:

`SEPARATE_WORLD_FIXED_FROM_SENSOR_FIXED_FIELD_BEHAVIOUR`

Conceptually:

```text
sealed observation A
sealed observation B
sealed observation C
...
        |
        +--> sensor/source coordinates
        |
        +--> deterministic world correspondences
                 |
                 +--> world-fixed behaviour candidate
                 +--> sensor-fixed behaviour candidate
```

Only after both relations are independently constrained may D.RAW test whether a repeatable field term belongs to the capture system rather than the photographed scene.

## Controlled rotation experiment

The classic 0°/90°/180°/270° flat-field experiment remains useful, but only as an optional **method-validation experiment**.

It is not a normal-user requirement.

Its purpose is to prove whether the separation algorithm behaves correctly under a controlled transform.

Results may not be hardcoded under a camera/lens name.

## Why no automatic matching yet

The existing FrontsideSceneInspector already reports:

- deterministic visible structure;
- edge density;
- orientation histogram;
- structural centroid;
- structural-content bounding box;
- structural feature signature;
- whether natural feature geometry is a candidate.

But these global descriptors are not enough to prove that a local physical world feature in observation A corresponds to a local feature in observation B.

The next algorithmic stage therefore needs a deterministic classical local-feature geometry path with:

- inspectable local keypoints/descriptors;
- pairwise candidate matching;
- deterministic outlier rejection;
- explicit 2D projective/rotational relation candidate;
- residuals and uncertainty;
- no AI/ML/neural/generative model.

No hidden semantic matcher is allowed.

## Android workflow

The existing Multi-observation panel now also offers:

**Export Observation-World Field Separation v0.1 · JSON**

The export is available after at least two selected observations have completed Universal Intake.

It records:

- each sealed observation root;
- frontside readiness;
- measured sensor-field availability;
- the three coordinate spaces;
- 360/natural self-calibration policy;
- the future deterministic registration contract;
- all promotion boundaries.

It deliberately reports world registration as unregistered in v0.1.

## Permanent safety

- no camera/lens/vendor identity key;
- no mandatory user calibration;
- no flat-field requirement for normal use;
- no panorama centre as physical/calibration origin;
- no user position as physical/calibration origin;
- no stitched panorama as source evidence;
- no structural-hash-as-registration proof;
- no AI/ML/neural/generative runtime;
- no source sample mutation;
- no new measured samples;
- no camera-system-response claim;
- no lens-only-vignetting claim;
- no calibration promotion;
- no correction;
- no Scientific Master writeback.

## Next gate

1. Green integrity + ARM64 Android build.
2. Export v0.1 from a selected multi-observation set.
3. Then implement a **deterministic classical local-feature pair geometry v0.1** as a new separate gate.
4. Only after pair geometry is independently validated may a world-fixed vs sensor-fixed separation estimator be attempted.

No correction path is authorized by this branch.


## Android build validation

The full ARM64 Android build is green.

- all pull-request checks: `26/26 SUCCESS`;
- Universal Intake run: `36685364035`;
- artifact id: `11083343949`;
- artifact: `DRAW_Full_Suite_Universal_Intake_v0.1_debug_arm64`;
- APK bytes: `7648015`;
- APK SHA-256: `c3e915a8ffbf428e367a48bbf946ba052f4afd01d7117928c80db3d20f811111`.

The built APK was inspected and contains the v0.1 schema, UNREGISTERED world-space boundary, natural self-calibration target, deterministic classical local-feature contract, and Android export label.

This closes the software/build gate only. The next gate is a real-device JSON export from a multi-observation set.
