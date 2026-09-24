# Human / Display Colour Appearance

Status: **RESEARCH MODEL**

## 1. Physical radiance and perceived colour are different domains

A spectral distribution is a physical quantity.

Colour appearance is a perceptual result that depends on the human visual system and viewing conditions.

A final D.RAW pixel therefore cannot be specified correctly by scene-linear RGB alone if the goal is consistent appearance across:

- SDR and HDR;
- dim and bright surrounds;
- different display peak luminance;
- different display primaries;
- different white points;
- different adaptation states.

## 2. Scene state must not be display state

Keep this one-way separation:

```text
scene / scientific state
    -> appearance rendering
    -> target display colorimetry
    -> display encoding
```

Never invert this relationship and use a pleasant display rendering as evidence about the scene.

## 3. CIECAM16 lesson

CIECAM16 maps tristimulus values and explicit viewing-condition parameters to perceptual appearance correlates.

The architectural lesson for D.RAW is that appearance depends on more than XYZ/RGB values.

A future view state should explicitly carry at least:

- adapting white;
- adapting luminance;
- background luminance;
- surround;
- target display white;
- display black / flare assumptions where modelled;
- target peak luminance;
- target gamut.

## 4. ACES 2 lesson

ACES 2 separates its Output Transform into:

1. a **Rendering Transform** that determines appearance;
2. a **Display Encoding Transform** that encodes the result for the actual display.

Its rendering transform operates in a JMh appearance-correlate space derived from the Hellwig 2022 CAM, applying tone mapping primarily to lightness J and colourfulness control to M while preserving hue h as a design goal.

D.RAW does not need to copy ACES 2 verbatim.

The important architectural lesson is:

> appearance rendering and display encoding are separate operations.

This is directly compatible with the D.RAW evidence/appearance boundary.

## 5. BT.2100 lesson

BT.2100 defines HDR systems including PQ and HLG.

PQ/HLG describe display/transport behaviour. They do not increase scene evidence or recover clipped sensor values.

Therefore D.RAW should only apply them at a target-output boundary.

## 6. Proposed D.RAW Appearance State

```text
ViewAppearanceState
  reference_white_xyz
  adapting_luminance_cd_m2
  background_luminance
  surround
  target_peak_luminance_cd_m2
  target_black_luminance_cd_m2
  target_primaries
  target_white
  transfer_function
  viewing_distance_or_visual_angle_optional
  creative_intent_optional
```

A deterministic identifier/hash should bind the final output to this state.

## 7. Appearance operations should be perceptual, not per-channel accidents

The output pipeline should avoid independent RGB operations when the desired control is perceptual.

Candidates:

- lightness/tone control in an appearance-correlate domain;
- colourfulness/chroma control separately from lightness;
- hue-preserving gamut compression;
- local adaptation only when its viewing rationale is explicit;
- highlight appearance that responds to target peak luminance;
- shadow appearance that accounts for target black and surround.

This is more coherent than applying arbitrary channel curves in display RGB.

## 8. HDR does not mean “brighter everywhere”

A higher peak-luminance target gives more available display headroom, but the rendering must decide what scene relationships should remain stable and what should expand.

D.RAW should preserve a stable scene master and derive different output views:

```text
same scene state
 -> SDR 100 nit view
 -> HDR 500 nit view
 -> HDR 1000 nit view
 -> HDR 4000 nit research view
```

The outputs may differ perceptually while sharing the same scene evidence identity.

## 9. Human spatial perception matters too

Perceived sharpness, contrast and colourfulness depend on spatial scale, viewing distance and luminance.

Therefore a future “perfect output pixel” cannot be defined independently of the size and context in which it will be viewed.

The rendering target should eventually support visual-angle-aware decisions rather than assuming that one raster pixel has a fixed perceptual meaning.

This belongs to appearance policy, not Scientific-Master authority.

## 10. Candidate internal colour domains

D.RAW should evaluate at least:

- scene-linear XYZ D50/D65 as a colorimetric interchange;
- a large-gamut scene-linear RGB basis for efficient computation;
- CAM16/CAM16-UCS for appearance experiments;
- ACES 2-style JMh/Hellwig-based appearance logic as an engineering reference;
- ICtCp/JzAzBz only for specific HDR/perceptual operations after validation.

No one space should be promoted as “physical truth.”

## 11. Required validation

A future appearance renderer should be tested for:

- hue stability under tone compression;
- monotonic lightness response;
- neutral-axis stability;
- gamut boundary behaviour;
- cross-target consistency;
- SDR/HDR roundtrip expectations where mathematically applicable;
- no scene-master mutation;
- deterministic output from the same scene state + view state.

## References

- CIE 248:2022 CIECAM16: https://www.cie.co.at/publications/cie-2016-colour-appearance-model-colour-management-systems-ciecam16
- CIE Division 8 publications: https://cie.co.at/technical-work/divisions/division8/division-publication
- ITU-R BT.2100-3: https://www.itu.int/rec/R-REC-BT.2100
- ACES 2 Output Transforms: https://docs.acescentral.com/system-components/output-transforms/
- ACES 2 rendering overview: https://docs.acescentral.com/system-components/output-transforms/technical-details/rendering-overview/
- OpenColorIO: https://opencolorio.org/
