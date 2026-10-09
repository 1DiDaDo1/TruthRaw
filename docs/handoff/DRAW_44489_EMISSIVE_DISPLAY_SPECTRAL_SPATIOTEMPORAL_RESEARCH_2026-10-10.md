# D.RAW 44489 — Emissive Display Spectral / Spatial / Temporal Research — 2026-10-10

Status: **BINDING RESEARCH EXPERIENCE / NOT A SCIENTIFIC PROMOTION / NO RUNTIME CHANGE**

Purpose: preserve the interdisciplinary reasoning triggered by the user-verified yellow TV subtitles that currently render near white/yellow-white in PURE. This record is a research constraint for future work; it is not proof that one mechanism below is already the causal answer.

## Permanent evidence boundary

The following remain unchanged:

- `MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`;
- sealed CFA/source evidence is immutable;
- Scientific Master is not rewritten by Appearance;
- a plausible colour is not evidence authority;
- Lightroom / vendor output / human visual matching is not a scientific calibration chart;
- accepted Censored Chroma Fallback v0.1 remains standard ADVANCED/PRO Appearance behavior and must not be weakened to solve this separate issue;
- no hue-specific yellow detector is admitted as a scientific fix.

## 1. 3D / physically based rendering lesson: emission is not reflection

The rendering equation separates emitted radiance from reflected radiance conceptually as:

`L_o = L_e + reflected/scattered contribution`.

A television pixel, LED wall, OLED pixel or other self-emissive source therefore belongs to a different physical class than a yellow diffuse surface illuminated by room light.

For D.RAW this means that the following two paths must not be assumed equivalent:

`illuminant -> surface reflectance/transmission -> lens -> sensor`

and

`display primary emission -> lens -> sensor`.

This is a modelling distinction, not an object detector requirement. Future work may use evidence-based source behaviour, but must not simply label semantic objects such as “TV”.

## 2. Spectral rendering lesson: RGB does not uniquely identify the spectrum

Modern physically based rendering increasingly performs spectral light transport because three RGB values do not uniquely encode a spectral power distribution. Metameric spectra can look similar to a human observer yet interact differently with another observer or camera spectral sensitivity.

The user-verified yellow subtitle may be produced by narrow or structured red/green display-primary spectra rather than by a broadband yellow reflectance spectrum.

A camera's CFA spectral sensitivity generally does not exactly satisfy the ideal Luther-Ives colorimetric condition. Therefore one fixed 3x3 camera-RGB -> XYZ transform cannot be assumed to map every possible incident spectrum exactly to human colorimetry.

Consequence for D.RAW:

**a fixed camera->XYZ matrix may be accurate over its admitted calibration distribution while remaining uncertain for narrowband or unusual emissive spectra.**

Do not interpret a finite XYZ/RGB triplet from such a case as automatically exact human chromaticity merely because the arithmetic is deterministic.

## 3. Virtual production / LED-wall lesson

Professional virtual-production systems encounter camera-vs-human colour differences when RGB LED walls with narrow spectral primaries are photographed directly or used as scene illumination. This is a known spectral-observer mismatch class.

The user's TV is therefore physically analogous, at smaller scale, to a directly photographed emissive LED/OLED display in virtual production.

Important distinction:

- display viewed directly by camera;
- display used as illuminant on another surface;
- reflected/transmitted room light.

These paths can produce different camera responses even when the human visual impression is related.

## 4. Spatial sampling lesson: display subpixels x lens PSF x CFA

A digital display has a spatial RGB subpixel lattice. The camera has a lens PSF/MTF, sensor pixel aperture and CFA lattice. Small high-contrast coloured glyphs can therefore produce channel-dependent sampling effects that a large uniform colour patch would not.

Potential chain:

`display RGB subpixels -> optical PSF/MTF -> sensor aperture -> Bayer/CFA phase -> demosaic/reconstruction -> colour transform`.

For small yellow text, red and green display primaries need not be sampled in the same effective proportions at every camera pixel. Edge pixels and glyph strokes can be especially sensitive. This can create colour shifts or moire-like chromatic errors without implying that the whole camera colour matrix is wrong.

Therefore future tests must distinguish:

- large uniform yellow field;
- large white/red/green/blue fields;
- small yellow glyphs;
- repeated captures with slightly changed distance/scale/registration where practical.

If large yellow is stable/correct but small yellow glyphs fail, spatial display-subpixel/CFA/PSF interaction becomes a stronger hypothesis.

## 5. Stop-motion / cinematography lesson: one RAW integrates time

A nominally static stop-motion frame still integrates radiance over a finite exposure interval. Emissive sources can vary during that interval because of:

- PWM dimming;
- display scanout;
- frame-rate control/dithering;
- refresh cadence;
- temporal colour sequencing or modulation;
- rolling-shutter row timing.

Human vision and a rolling-shutter CMOS exposure need not integrate these signals identically.

Therefore a TV colour that appears stably yellow to the eye can yield a different camera-channel mixture in one exposure.

Future controlled tests should compare repeated otherwise-identical RAW captures and materially different exposure durations where safe. If colour changes with capture timing/exposure duration, temporal display modulation becomes a strong causal candidate.

## 6. Current D.RAW yellow-subtitle observation

Same-capture PURE case:

`DRAW_CAPTURE_1791583374795_tele_4080x3072_draw_pure_float32_v0_63.dng`

and

`DRAW_CAPTURE_1791583374795_tele_4080x3072_draw_pure_fullres.jpg`.

User states the physical subtitles are yellow. Current output appears near white/yellow-white.

Using the exact current XYZ-D50 -> linear-sRGB conversion, sampled bright subtitle pixels were previously found at median approximately:

`(R,G,B) = (0.885, 0.915, 3.581)`

before the final display clamp.

Thus the discrepancy is not explained solely by final sRGB clipping. A strong B-dominant relation already exists earlier in the current colour path.

This does **not** yet prove:

- bad camera matrix;
- bad reconstruction;
- spectral mismatch;
- temporal aliasing;
- subpixel aliasing;
- clipping/censoring;
- or any one combination.

Exact subtitle-pixel authority/support must be resolved first.

## 7. Current ranked hypotheses

These are research priorities, not proven causes:

1. **spectral observer mismatch / narrowband display emission vs fixed camera->XYZ matrix** — serious;
2. **display-subpixel lattice x lens PSF x CFA sampling/reconstruction** — serious, especially for small glyphs;
3. **display PWM/scanout/refresh x exposure interval/rolling shutter** — serious and testable;
4. **clipping/censoring/uncertainty affecting colour formation** — serious;
5. final JPEG/sRGB clamp or accepted highlight Appearance — substantially weaker as primary cause for this PURE observation.

## 8. Correct experimental discriminator

A controlled display target should contain at least:

- large red field;
- large green field;
- large blue field;
- large yellow field;
- large white field;
- the same colours as small glyphs/lines.

Capture repeated sealed RAW observations with fixed geometry/settings, plus a second exposure-duration condition where feasible.

Interpretation guide:

- large yellow fails consistently -> spectral/matrix or clipping/authority becomes stronger;
- large yellow works but small yellow glyphs fail -> subpixel/CFA/PSF sampling becomes stronger;
- repeated captures or exposure-duration change alter chromaticity -> temporal/PWM/scanout becomes stronger;
- only high-radiance cases fail -> clipping/censoring/authority becomes stronger.

Do not promote an inference merely from one outcome; bind all conclusions to sealed observation provenance.

## 9. Architectural research direction

Future architecture may need to represent uncertainty around **emissive-like spectral cases** separately from ordinary reflected-light colour formation, but it must remain evidence-based and source-agnostic.

Conceptual research route:

`sealed CFA -> support/authority + spatial structure + exposure/temporal metadata where available -> colour formation with observer/spectral uncertainty -> Scientific Master authority preserved -> Natural Appearance`.

Possible state distinctions may eventually include evidence that a sample is compatible with direct emission, reflection/transmission, or unresolved/mixed behaviour, but **none of these may be inferred semantically from object class alone**.

No new state is promoted by this document.

## 10. Reference knowledge families

Future work should remain synchronized with primary/academic material on:

- the rendering equation / emitted vs reflected radiance;
- spectral physically based rendering;
- camera spectral sensitivities and Luther-Ives colorimetry;
- virtual-production / LED-wall camera colour reproduction;
- display-recapture, subpixel/CFA sampling and chromatic moire;
- temporal light modulation, PWM, rolling shutter and exposure integration;
- stop-motion anti-flicker practice and controlled exposure.

Representative sources previously consulted include Stanford/physically based rendering material, PBRT spectral rendering literature, peer-reviewed camera/colorimetric studies, CVF display-recapture research, CIE temporal-light-modulation work and stop-motion exposure guidance. These sources inform hypotheses only; they do not become observation evidence.

## 11. Permanent do-not-do rules from this research

Do not:

- detect “yellow subtitle” and recolour it by semantic/hue rule;
- call a 3x3 camera->XYZ result exact human colour truth for every possible spectrum;
- use a vendor camera or Lightroom image as scientific spectral calibration;
- alter the accepted purple-highlight fallback to compensate for this separate failure;
- infer an emissive source solely because an object looks like a screen;
- ignore temporal exposure integration merely because a stop-motion frame appears visually static;
- ignore CFA/subpixel/PSF interaction for small emissive glyphs;
- convert spectral/spatial/temporal uncertainty into MEASURED authority.

## 12. Current conclusion

The strongest new research insight is:

> The yellow-TV-subtitle discrepancy may be the first clear real-device case where D.RAW's current trichromatic colour formation is being asked to represent a narrowband, spatially sampled and temporally modulated emissive source with more colorimetric certainty than the sealed evidence supports.

This is a **research hypothesis and architecture warning**, not a promoted scientific fact.
