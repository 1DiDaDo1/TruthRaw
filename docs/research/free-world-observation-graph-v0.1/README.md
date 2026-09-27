# D.RAW Free World Observation Graph v0.1

Status: **RESEARCH INTEGRATION CANDIDATE**

Date: 2026-09-27

This module turns the lens-independent D.RAW architecture into an explicit
knowledge graph above sealed per-observation D.RAWnegative states.

It does not modify D.RAWnegative v0.1, TruthNegative Continuous v0.5,
Scientific Master bytes, Direct CFA evidence, or any sealed v0.1 contract.

Permanent laws:

> **One Free World. Many sealed observations. One evidence law.**

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## 1. Why this layer exists

A lens-independent input gives more control only when source identity is not
discarded. Main, ultra-wide, telephoto, front and external cameras are not
different worlds and are not interchangeable instruments. They are separate
observations of one world.

Every new piece of information must therefore know:

- which observation or relation it belongs to;
- what information domain it describes;
- on which epistemic floor it lives;
- what authority it currently has;
- which immutable identity/provenance binds it;
- whether it is measured, calibrated, reconstructed, inferred, hypothetical
  or appearance-only.

Information is not strengthened by relabelling it. It grows by acquiring
additional admitted context and relations while its original provenance and
authority remain unchanged.

## 2. Knowledge placement

v0.1 uses these placement domains:

- SOURCE_EVIDENCE
- SAMPLING_GEOMETRY
- GEOMETRY_POSE
- RADIOMETRY
- COLORIMETRY
- SPECTRAL
- OPTICAL_SUPPORT
- NOISE_UNCERTAINTY
- TEMPORAL
- MATERIAL
- ILLUMINATION
- PROVENANCE
- RESTORATION
- APPEARANCE

and these floors:

- EVIDENCE
- SCIENTIFIC_DERIVED
- RELATION
- SCENE_MODEL
- APPEARANCE

The floor is part of the record. A visually useful value cannot silently move
upstream.

## 3. Independent relation axes

Cross-observation knowledge is factored. v0.1 recognizes:

- GEOMETRY
- RADIOMETRIC_GAUGE
- COLORIMETRIC
- SPECTRAL
- OPTICAL_SUPPORT
- UNCERTAINTY_CORRELATION
- TEMPORAL
- PROVENANCE

A relation on one axis grants nothing on another axis.

Examples:

- a proven pose relation does not prove equal radiometry;
- a shared radiometric gauge does not prove equal spectral response;
- matching raster dimensions do not prove matching optical support;
- shared device identity does not prove transferable calibration;
- temporal overlap does not prove static-scene equality.

## 4. Knowledge growth

Relations may be:

- UNKNOWN
- HYPOTHESIS
- SOURCE_BOUND
- CALIBRATED
- ADMITTED
- REJECTED

Promotion to ADMITTED requires an explicit certificate identity. REJECTED is
not a failure of the world model: it is retained knowledge that a proposed
relation did not pass its gate.

This is the project's technical interpretation of a "positive knowledge
boost": the immutable observation does not gain invented authority; the graph
gains more usable, explicitly supported connections.

No scalar confidence score is introduced by v0.1.

## 5. Fusion is a composite admission

A single admitted relation is not sufficient permission to fuse observations.

The graph therefore separates:

1. relation knowledge;
2. coordinate/equality capability;
3. composite fusion admission.

A relation record itself may never set fusion permission in v0.1.
A separate FusionAdmission must reference the required admitted relation
certificates.

For example, radiometric fusion requires at least admitted:

- RADIOMETRIC_GAUGE;
- UNCERTAINTY_CORRELATION;
- TEMPORAL compatibility.

Colour fusion additionally requires COLORIMETRIC authority.
Spatial-detail fusion additionally requires GEOMETRY and OPTICAL_SUPPORT.

This downstream guard is intentionally stricter than any single upstream
capability flag. It does not rewrite D.RAWnegative v0.1 semantics.

## 6. Temporal footprint

A physical photograph is not assumed to be an infinitesimal time sample.
An observation may eventually bind an exposure interval, shutter weighting and
rolling/global readout model.

When those quantities are unknown, the graph records UNKNOWN rather than
inventing them.

Virtual EV/ISO/view nodes remain deterministic projections:

- they create no physical frame;
- they add zero independent evidence;
- they cannot be used as a second observation in a fusion certificate.

A genuinely separately captured frame is a separate sealed observation.

## 7. Optical support

Pixel count is not optical resolution. Optical support may later bind
field position, spatial frequency, focus distance, wavelength/channel, PSF,
OTF/MTF and motion/focus support.

v0.1 stores only authority and identity; it does not invent an optical model.

## 8. Human vision and appearance

Viewing conditions, adaptation, display target, spatial-frequency-aware
appearance and other perceptual transforms remain downstream. They may improve
the final visible product but cannot change source, Scientific Master,
D.RAWnegative, relation certificates or fusion authority.

## 9. Restoration and scene models

Restoration, material, illumination and deep/light-transport states keep
their own floors. A visually seamless restoration may remain provenance-visible.

Computed geometry, material, illumination, reflection, hidden radiance or
counterfactual light never become measured sensor evidence by writeback.

## 10. Relationship to preserved research

This integration explicitly preserves and reuses earlier project knowledge:

- Virtual Observation Manifold v0.9: many virtual views, one evidence item;
- FotoGraaf Acquisition Domain v0.8: geometry/sample-domain facts stay
  separate from physical sensor claims;
- Professional RAW Gatehouse v0.1: external decode is quarantined and cannot
  manufacture zero-line, Scientific Master or extra evidence;
- camera-RGB covariance / XYZ uncertainty: unknown covariance is not silently
  independence;
- Bound Uncertainty Admission: superficially similar source domains do not
  inherit uncertainty calibration;
- Open World / Structure Evidence / conservation research: information keeps
  authority and provenance through reconstruction and appearance.

## 11. Current production boundary

The Camera-5 telephoto observation remains the first proven anchor.

This module does **not**:

- grant a main/ultra-wide relation;
- grant a shared Free World radiometric gauge;
- grant cross-lens colour equivalence;
- grant optical equivalence;
- grant cross-observation fusion;
- alter the validated Android pixel route.

The next real graph growth should come from separately admitted main and
ultra-wide Observation Records and measured relation campaigns.

## 12. External theoretical anchors

The design is consistent with established ideas from sensor metrology,
sampling/MTF, camera calibration, covariance propagation, physically based
light transport, colour appearance, stop-motion/temporal exposure and
conservation provenance. Useful reference families include EMVA 1288,
ISO 12233/SFR, CIECAM16, PBRT/radiometry, Radiance daylight simulation and
professional conservation documentation/retreatability principles.
