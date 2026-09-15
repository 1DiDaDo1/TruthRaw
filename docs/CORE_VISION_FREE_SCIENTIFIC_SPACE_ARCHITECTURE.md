# TruthRaw Core Vision — Free Scientific Space Architecture

**Status: CANONICAL PROJECT VISION — user-directed update 2026-09-15**

This document supersedes the old **sealed-house metaphor as the active architectural description**. The historical metaphor remains useful only to explain preservation of original evidence; it must not be interpreted as a limitation on TruthRaw reconstruction.

## 1. Source evidence

The admitted source RAW/CFA bytes and capture metadata are preserved as an immutable **measurement record**.

They answer:

> **What did this camera actually return for this exposure and capture route?**

Preserving that record does not make the RAW a sealed computational world. The source is evidence, not the numerical or architectural boundary of TruthRaw.

## 2. Free Scientific Space

TruthRaw constructs a separate scientific scene state from the evidence.

The reconstructed state is a **Free Scientific Space**: a representation selected for scientific usefulness rather than inherited from arbitrary limits of the source file.

It is not required to inherit:

- RAW10/RAW12/RAW14 integer range;
- source WhiteLevel as an output ceiling;
- source ISO as a scene scale;
- `[0,1]` limits;
- SDR/display limits;
- original Bayer layout as the final master layout;
- original pixel count as the only possible reconstruction lattice;
- source camera colour coordinates as the final scientific colour coordinates;
- DNG conventions;
- a single fixed precision such as float32.

The master may contain real-valued reconstructed quantities, uncertainty/covariance/support, censor bounds, scene-scale coordinates, values greater than one, signed estimator values where mathematically meaningful, and wider representational dynamic range than the source encoding.

## 3. Precision-independent scientific contract

The scientific master is defined by its meaning, not by one storage type.

`float32`, `float64`, 128-bit floating point and arbitrary-precision arithmetic are implementation choices with different performance and numerical-error properties.

The project therefore uses this rule:

> **Numeric precision is an execution profile. Scientific semantics are precision-independent.**

Original integer/packed evidence remains byte-preserved. Calibration, optimisation and reference calculations may use higher precision than per-pixel mobile hot paths.

## 4. Freedom does not mean invention

Free Scientific Space removes source-container restrictions. It does not remove information limits.

Every scientific quantity retains an evidence class such as:

- measured;
- reconstructed;
- censored/bounded;
- weak/unknown;
- counterfactual;
- appearance.

A clipped source value may support a reconstructed value above source WhiteLevel, but the original measurement remains censored. A reconstructed high-resolution coordinate may exist in the Scientific Master, but it is not relabelled as a newly measured photodiode value unless corresponding evidence exists.

Permanent law:

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## 5. ISO, exposure and capture state

ISO, shutter/exposure time, camera/lens identity, sensor pixel mode, gain/readout state and capture controls remain provenance of the measurement.

They do not have to remain the scale or identity of the reconstructed scene.

The Scene Master may be normalised to a scene-domain representation while retaining exact capture provenance and uncertainty.

## 6. Colour and light

The Scientific Master is not required to retain the vendor camera's final colour transform or white-balance presentation.

Physical colour and illumination are estimated from admitted evidence plus independent calibration. Appearance colour remains downstream and never writes back into the scientific state.

Scene illumination, material response, optics/shading and sensor/upstream response remain separable concepts.

## 7. Spatial freedom

The source CFA lattice is the measurement lattice, not automatically the final reconstruction lattice.

TruthRaw may use a different latent sampling structure if the forward model and evidence support it. A larger or different lattice does not create evidence by itself; reconstructed samples retain reconstructed status and uncertainty.

This rule is particularly important for the Camera-5 12.5MP / 50MP / 200MP programme: the real app-visible 200MP route must be measured first, then its relation to lower-resolution modes must be characterised rather than assumed.

## 8. Exports are projections

A DNG, Linear DNG, reconstructed CFA file, EXR, TIFF, HDR preview or other output is a projection/representation of scientific state.

No export container is the definition of TruthRaw and no export is allowed to silently increase evidence authority.

## 9. Resource freedom

A weaker phone may use smaller tiles, fewer parallel rooms and more recomputation. A stronger phone/workstation may use larger tiles, float64 paths, larger caches or optional accelerators.

Resource capability does not change what is measured or what may be claimed.

## 10. Historical sealed-house terminology

Earlier project documents used:

- sealed original house = immutable RAW evidence;
- new house = reconstructed master.

The useful part that survives is **immutability of the admitted source evidence**.

The word **sealed** must no longer be used to imply that TruthRaw reconstruction itself is bounded by that RAW. For current reasoning, prefer:

- **Source Evidence Record**;
- **Free Scientific Space**;
- **Scientific Scene Master**.

The historical document `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md` is retained only as provenance of how this idea developed.

## 11. Core formula

Current canonical conceptual formula:

**Exact source evidence -> physical/statistical measurement model -> uncertainty-aware reconstruction -> Free Scientific Scene Space -> colourimetric/scientific interpretation -> optional compatibility/counterfactual/appearance projections.**

And the permanent epistemic boundary remains:

**Measured where measured. Reconstructed where necessary. Never invented.**
