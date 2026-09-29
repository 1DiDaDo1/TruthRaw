# D.RAW Observation Optical Field Chart v0.1

Date: 2026-09-29

Status: research-only, read-only, no correction/writeback.

## Purpose

This research line implements the user's "flat lens" idea as a scientific coordinate transform, not as a physical claim that the lens itself is flattened.

The sealed observation is unfolded into a deterministic field chart:

`source x,y -> declared geometric field origin -> radius rho + azimuth -> radial/tangential basis`

This is designed to make position-dependent optical/sensor behaviour easier to inspect without using a lens database or device-specific calibration.

## What v0.1 proves

- exact source/active-area geometry;
- deterministic radial/tangential coordinates;
- exact mapping back to the existing 20-bit raster-independent sample lattice;
- measured sparse CFA signal statistics can be grouped by radial annulus and azimuth sector;
- CFA-phase behaviour can be compared over field radius;
- the original sealed samples remain unchanged.

## What v0.1 does NOT prove

- the active-area geometric center is not claimed to be the physical optical axis;
- rho is not claimed to be chief-ray angle or field angle;
- a radial brightness falloff is not automatically lens vignetting;
- scene illumination, lens relative illumination and sensor angular response are not separated;
- no cos^4 model is assumed;
- no distortion correction, shading correction or gain map is applied;
- no optical-resolution gain is claimed.

## Geometry

The chart origin is the geometric center of the active area when an admissible ActiveArea is available, otherwise the full source raster center.

For a measured source position `(x,y)`:

- `dx = x - center_x`;
- `dy = y - center_y`;
- `r = sqrt(dx^2 + dy^2)`;
- `rho = r / max_active_corner_radius`;
- `azimuth = atan2(dy,dx)`;
- radial unit direction = `(dx/r,dy/r)`;
- tangential unit direction = `(-dy/r,dx/r)`.

rho therefore describes position in the observed source field. It is deliberately not converted to degrees because focal length alone does not prove sensor dimensions, projection model, distortion state or physical chief-ray angle.

## Measured field signal

When the existing BacksideSignalSupportAudit can read the selected source payload, it now additionally records an audit-only field profile.

Current supported measured topology remains deliberately narrow:

- classic TIFF/DNG;
- uncompressed unsigned 16-bit;
- one sample per pixel;
- strips;
- measured sparse grid inherited from BacksideSignalSupportAudit.

The measured samples are grouped into:

- 12 radial annuli;
- 12 azimuth sectors per annulus;
- four CFA phases per annulus.

For each annulus the audit reports measured p10/p50/p90, CFA-phase p50 values, sector medians and azimuthal median absolute dispersion.

It also reports purely descriptive quantities such as observed outer/inner p50 ratio and the fraction of adjacent annular medians that do not increase.

Those values are COMPOSITE_SCENE_LENS_SENSOR_OBSERVATION_ONLY. They are not a lens shading profile.

## Why this is useful

A lens/sensor system can produce position-dependent relative illumination and off-axis behaviour. A flat field coordinate chart allows those patterns to be described in a common radial/tangential frame rather than only as arbitrary raster positions.

Later research can ask whether a pattern is sufficiently symmetric, repeatable or structurally supported to justify a stronger optical interpretation.

## Future gates

Possible successors may investigate:

- target-blind optical-axis estimation from repeatable field symmetry;
- separation of radial illumination from scene structure using multiple independent observations;
- radial versus tangential local detail/blur behaviour;
- CFA-phase field response;
- chromatic radial/tangential displacement;
- relation between field position and local reconstruction-model reliability;
- optional comparison to DNG shading/distortion metadata as non-authoritative provenance hints.

None of those may silently mutate the sealed source or Scientific Master.

## Permanent safety

- no lens-profile lookup;
- no camera-model routing;
- no vendor mapping;
- no AI/ML/neural/generative runtime;
- no measured sample mutation;
- no new measured evidence;
- no correction gain;
- no Scientific Master writeback.
