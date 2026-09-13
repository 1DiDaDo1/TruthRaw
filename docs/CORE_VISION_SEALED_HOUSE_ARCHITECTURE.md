# TruthRaw Core Vision — Sealed House / New House Architecture

**Status: CANONICAL PROJECT VISION**

This document is a mandatory conceptual foundation for TruthRaw. It is not a rendering preset, a temporary research idea, or a calibration-specific note. Future work and future chats must preserve this architecture unless the user explicitly changes it.

## 1. The sealed house

The original RAW is the **sealed original house**.

TruthRaw never demolishes, repaints, or secretly edits that house. The source CFA/sample bytes and capture metadata remain immutable evidence. They answer the historical question:

> **What did this camera actually record in this exposure?**

That evidence includes, where available:

- original CFA/sample codes;
- black/white levels;
- clipping/censoring state;
- exposure time;
- ISO/gain provenance;
- lens/camera/source identity;
- DNG opcodes/GainMap;
- noise metadata;
- color metadata;
- focus/OIS/thermal/capture state;
- hashes and provenance.

The sealed house is evidence. It is **not** the final numerical container, rendering space, dynamic-range ceiling, color space, or structural limit of TruthRaw.

## 2. The new house

TruthRaw builds a **new house, stone by stone**, next to the sealed original.

The new house is the **Latent Scene Truth / Scene Master**: TruthRaw's best-supported estimate of the physical scene given the immutable measurements, a documented forward model, calibration evidence, reconstruction constraints, and uncertainty.

TruthRaw is therefore not fundamentally "editing the RAW". It is:

> **constructing a new scene estimate from RAW evidence.**

The source RAW remains the measurement record. The new master is a separate reconstructed state.

## 3. Representation limits of the source are not master limits

The Latent Scene Master is not required to inherit arbitrary representation limits of the source container.

Examples of limits that do **not** have to define the new master:

- RAW10 / 10-bit code range;
- `WhiteLevel = 1023` as an output maximum;
- the original ISO label as a working image scale;
- source camera white balance;
- source DNG gamut or camera-native gamut as the final output space;
- SDR display range;
- integer-only storage;
- values constrained to `[0,1]`;
- source DNG compatibility conventions;
- the original Bayer export layout;
- a final DNG's representational limitations.

A scientific master may use float/high precision, retain values below 0 or above 1 when meaningful, carry censor bounds, carry uncertainty/covariance/support maps, and use a wider numerical dynamic range than the original code values.

**The source's representational ceiling is not automatically the scene master's ceiling.**

## 4. ISO is capture provenance, not scene identity

The earlier TruthRaw idea of effectively **removing the ISO limit** is canonical.

This does **not** mean pretending the exposure captured more photons than it did. It means:

- the original requested/reported ISO remains immutable capture provenance;
- analog/digital gain and readout state remain part of the measurement model;
- the reconstructed scene master is allowed to normalize away that capture gain;
- the master does not have to remain an "ISO 3200 image", "ISO 6400 image", etc.;
- noise/uncertainty follows the measurement evidence rather than being baked into the identity or display scale of the master.

Conceptually:

`captured RAW + exposure + gain/readout state + noise + clipping`

is transformed into a scene-domain estimate such as:

`latent scene signal + support + censor bounds + uncertainty`

The reconstructed master can therefore be **ISO-neutral** even though its provenance permanently records the source ISO.

**ISO belongs to the history of the measurement, not to the identity of the reconstructed scene.**

## 5. We may go around source-container limits — but not around evidence

The phrase "we can fly around the house" means TruthRaw is free to design a superior reconstruction space instead of treating the original file format as the only possible world.

We may design:

- a larger numerical dynamic range;
- different sample structures;
- better colorimetric coordinates;
- a virtual-camera representation;
- uncertainty-aware reconstructed values;
- highlight estimates above source WhiteLevel;
- scene-linear masters that are not constrained by display or DNG conventions;
- future super-resolution/spatial reconstruction where evidence and the forward model support it.

But there is a permanent distinction between:

1. **measured** — directly supported by admitted source measurements;
2. **reconstructed** — inferred from the measurements/model;
3. **censored/bounded** — only a lower/upper bound is measured;
4. **unknown/weakly supported** — evidence is insufficient;
5. **appearance/rendering** — a reversible presentation choice.

TruthRaw can reconstruct beyond a source representation limit. It must never relabel that reconstruction as newly measured photons.

## 6. Knowledge limits remain real

TruthRaw removes **representation limits**, not the laws of information.

Examples:

- a clipped highlight may be reconstructed above WhiteLevel, but the clipped source code is a censored lower-bound measurement;
- a noise-free posterior image can still have non-zero uncertainty;
- an optical frequency that was never transmitted/measured cannot become new measured detail;
- a missing color channel at one CFA site can be reconstructed, but it remains reconstructed;
- weak evidence must remain weak evidence even if the final rendering looks convincing.

The correct goal is therefore not "invent anything we want". The correct goal is:

> **build the best new house that the evidence can support, while allowing the new house to use a much richer architecture than the sealed source house.**

This preserves the project slogan:

> **Measured where measured. Reconstructed where necessary. Never invented.**

## 7. Calibration expands certainty; it does not grant architectural permission

Color, illuminant, electron/PTC, PSF/MTF/CA/flare/shading and uncertainty calibration are important because they improve the forward model and reduce uncertainty.

They are **not** prerequisites that grant TruthRaw permission to exceed RAW10, ISO, source gamut, WhiteLevel, or DNG representation limits.

TruthRaw's architecture already permits the richer Latent Scene Master.

Calibration answers a different question:

> **How strongly can each part of that reconstructed master be physically justified and certified?**

Therefore:

**FULL_PHYSICAL is an evidence/certification level, not the boundary of the reconstruction architecture.**

A scene master may be structurally richer than the source even while the project status remains `PURE_TRUTH_DERIVED`.

## 8. The DNG is not the house

A derived DNG is a compatibility/export projection.

The scientific target is the Latent Scene Truth / Scene Master, not the DNG container.

Recommended conceptual layers:

0. **Immutable Evidence** — original RAW/CFA + capture metadata.
1. **Measurement Domain** — black/linearization/gain/readout/noise/censor interpretation.
2. **Latent Camera Scene** — reconstructed camera-domain scene + support/uncertainty/censor bounds.
3. **Colorimetric Scene Master** — documented scene-linear/device-independent coordinates.
4. **Compatibility / presentation projections** — DNG, EXR/HDF5, SDR/HDR preview, virtual camera output.

No export format is allowed to silently redefine the scientific master.

## 9. Consequence for future TruthRaw work

When a future task asks whether TruthRaw is "limited by the original RAW", the default answer is:

**No, not by the source file's representation limits.**

TruthRaw is constrained by the **evidence and the laws of inference**, while being free to build a new representation that exceeds the source container's bit depth, gain scale, display range, gamut, and other arbitrary encoding limits.

When a future task discusses ISO:

**preserve ISO as provenance; do not force ISO to remain the scale or identity of the reconstructed scene master.**

When a future task discusses calibration:

**use calibration to improve physical correctness, support and uncertainty — not to shrink the architecture back to the source RAW's container limits.**

## 10. Mandatory new-chat rule

A new TruthRaw chat should read this document **before** interpreting calibration blockers, DNG export status, PTC, optics, color, uncertainty, or RAW reconstruction scope.

Do not replace this vision with the narrower idea that TruthRaw is merely a denoiser, RAW editor, or DNG converter.

The canonical conceptual formula is:

**Sealed original RAW evidence -> documented physical/statistical forward model -> unrestricted reconstructed Latent Scene Truth Master -> optional virtual-camera / compatibility / appearance projections.**

And the permanent boundary is:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## 11. The house is also provenance and authority architecture

Recovered project history clarifies that the house metaphor is broader than reconstruction freedom alone.

Four concepts now belong together:

- **sealed house** — immutable source evidence;
- **new house / alle vrijheid** — richer reconstruction space above the evidence boundary;
- **Technical Backplane / achterkant van de foto** — compact binding of source/master/zero-line/authority behind the visible projection;
- **Gatehouse / tussenwoning** — isolated external RAW ingress that produces a sealed handoff and then detaches before heavy Main-House work.

These concepts are documented together in:

`docs/CORE_VISION_HOUSE_GENEALOGY_BACKPLANE_GATEHOUSE_2026-09-13.md`

Repository history verifies that the sealed-house vision was canonized by commit:

`13075856895ee6815c8a72fcf733d52bad596583` — `Canonize sealed-house TruthRaw core vision`.

## 12. Current Scientific Master terminology

Historical Scene/Latent Master experiments evolved. Do not silently flatten them into the current definition.

The **current project-level Scientific Master** is the reconstructed **camera-native RGB scientific state before the normal `camera_to_xyz()` route and before appearance**.

Historical richer/colorimetric Scene Master ideas remain valuable design provenance, but they do not override the current master definition unless a future explicit canonical change does so.
