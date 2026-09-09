# TruthRaw v0.8 — Co-sited Missing-Channel Topology Validation Capture Protocol

## Purpose

Create independent validation evidence for RGB components that are physically absent at a Bayer pixel in a single exposure.

This protocol is **validation-only**. Multiple frames may establish external ground truth. They must never be merged into the single-frame TruthRaw scientific master.

## Scientific claim to be tested

For a frozen reconstruction backend and source domain, a reconstructed missing RGB component preserves local scene topology closely enough to receive a topology-risk classification. `topologyCertified=true` is forbidden until this protocol and prospective held-out gates pass.

## Target

Use a rigid, planar, matte target. Do not use a phone/monitor display because display subpixels can alias with the camera CFA.

The target should contain:

- neutral flat patches from dark to bright midtones;
- slanted edges at 0°, 15°, 30°, 45°, 60°, 75° and 90°;
- fine black/white line groups below, near and above the camera Nyquist region;
- smooth luminance gradients;
- large red, green, blue, cyan, magenta and yellow matte regions;
- coloured slanted boundaries between those regions;
- small fiducial crosses/grid marks distributed across centre, mid-field and edge.

Avoid glossy/specular materials.

## Lighting

- Continuous non-flickering light.
- Keep illumination fixed for the entire sequence.
- No auto exposure, auto ISO, auto white balance or refocus between frames.
- Prefer a low-noise exposure with the useful target range comfortably below source WhiteLevel.
- Reject evaluated pixels that are source-censored/clipped.

## Camera state

Use the exact lens/source domain that is to be certified.

Freeze:

- lens;
- focus position;
- ISO/gain state;
- shutter time;
- white-balance metadata policy;
- stabilization state;
- RAW capture path;
- reconstruction backend/hash.

Store all capture metadata and source-file SHA-256 values.

## Phase-diversity capture

Preferred setup: rigid phone mount on a two-axis micro-translation stage.

Capture **at least 16 RAW frames** of the same target. Between frames translate the camera by small amounts corresponding to approximately 0.5–3 raw sensor pixels in the image plane. Do not intentionally change focus or exposure.

The final registered set must contain multiple observations spanning all four Bayer phase classes modulo the 2×2 CFA lattice. Prefer at least three independent usable measurements for every required phase class in every evaluated field region.

Small rotation can be tolerated only if registration remains within the qualification gate below. Translation is preferred.

## Registration must use measured evidence

Registration may not use TruthRaw reconstructed missing colours as its ground truth.

Primary registration signal:

1. build a half-resolution green image from the two **physically measured** green CFA samples in every 2×2 Bayer cell;
2. estimate coarse translation;
3. fit a global affine/homography on target fiducials;
4. estimate local residual displacement;
5. map each raw photosite to a common planar target coordinate.

### Proposed registration qualification gate — freeze before capture

- direct measured-green alignment RMS <= **0.10 raw pixel** in each evaluated region;
- 95th-percentile local residual <= **0.20 raw pixel**;
- photometric scale drift after fixed-exposure normalization <= **0.5%** on neutral midtone patches;
- no evaluated target/source support may be clipped;
- regions with motion, defocus change, flare changes or registration ambiguity are rejected, not repaired.

These are protocol gates, not claims about the sensor's intrinsic resolution.

## Building independent co-sited references

For target frame A and scene coordinate q:

1. identify the RGB component absent at A's Bayer photosite;
2. use the registered phase-diverse frames to find direct photosites of that colour that observe q within the registration tolerance;
3. convert each direct measurement through the exact source Stage-2 transform (BlackLevel/WhiteLevel + GainMap exactly once);
4. normalize only for measured residual frame exposure scale if the qualification step proves such a correction is necessary;
5. combine at least three independent direct observations with their source-bound marginal uncertainties;
6. retain the reference uncertainty and registration uncertainty separately.

The resulting reference is validation evidence, not a replacement pixel for the TruthRaw master.

## Repeatability ceiling

Before judging reconstruction, measure how well **direct same-colour measurements agree with each other** after registration. This defines the attainable validation ceiling and prevents the reconstruction from being penalized for reference noise/registration error.

Record per channel and field region:

- direct/direct absolute residual p50/p95;
- ordering agreement for significant neighbour pairs;
- curvature-sign agreement;
- registration-induced uncertainty;
- source NoiseProfile marginal uncertainty.

If direct/direct repeatability itself fails, the dataset is invalid for topology certification.

## Reconstruction evaluation

Freeze the reconstruction backend before opening the prospective holdout results.

Evaluate reconstructed missing channels against the co-sited direct references using:

- p50/p95 absolute error;
- error in source-bound sigma units;
- local pairwise ordering agreement;
- local curvature-sign agreement;
- local-rank error;
- false-edge / edge-reversal rate;
- support overshoot rate;
- centre / mid-field / edge breakdown;
- luminance and chroma-edge strata;
- low / medium / high local-SNR strata.

No semantic labels such as fur, skin or material may enter the scientific gate.

## Prospective pass policy

The calibration sequence may be used to establish implementation bugs and freeze the evaluator. It may **not** be used as the final certification holdout.

After freezing code and gates, acquire at least two new static-target sequences not used for tuning.

A candidate topology certificate requires:

- registration qualification PASS;
- direct/direct repeatability PASS;
- no hidden target leakage;
- no source-censored values treated as exact references;
- all RGB channels evaluated;
- centre, mid-field and edge evaluated;
- significant ordering agreement >= **95%** in every channel/field stratum with sufficient sample count;
- reconstruction ordering no more than **2 percentage points below** the matched direct/direct repeatability ceiling;
- significant curvature agreement >= **90%** where the direct/direct ceiling itself is >=92%;
- no unexplained field/channel failure hidden by global averaging.

If a stratum fails, keep `topologyCertified=false` there and report the failure.

## Relation to the three 2026-09-07 dog captures

The 094414 / 094416 / 094423 sequence is useful real-life reconstruction evidence but is **not** a valid phase-diversity topology-certification set. Subject pose and camera/scene geometry change too much between frames for trustworthy co-sited raw-pixel references.

## Output of a successful campaign

The validation package should contain:

- immutable source RAW hashes;
- capture metadata;
- measured-green registration transforms and residual maps;
- phase-coverage maps;
- direct/direct repeatability report;
- co-sited direct-reference records and uncertainty;
- frozen reconstruction backend hashes;
- prospective reconstruction-vs-reference metrics;
- explicit per-region `topologyCertified` status;
- failed strata preserved, never silently removed.
