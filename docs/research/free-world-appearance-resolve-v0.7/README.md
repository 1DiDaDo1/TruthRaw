# D.RAW Free-World Appearance Resolve v0.7

Status: **EXECUTABLE RESEARCH REFERENCE — READY FOR MAIN-CODE INTEGRATION AFTER GREEN CI**

v0.7 closes the first end-to-end Free-World research arc by adding an explicit human/view/display boundary after the scene and light-transport state.

## Core rule

The scene remains upstream and immutable:

```text
scene/light transport
    -> appearance resolve
    -> display colorimetry
    -> display encoding
```

Changing a viewing condition or display target must not mutate the source scene identity, channel authority or uncertainty.

## Explicit state

The resolver binds four independent identities:

- scene colorimetry;
- viewing conditions;
- display target;
- appearance policy.

The output is then bound by its own SHA-256.

## Viewing conditions

The research reference carries:

- adapting white XYZ;
- adapting luminance;
- background luminance;
- surround class (dark/dim/average);
- viewing distance.

v0.7 does **not** claim to be a complete CIECAM16 implementation.

Instead, it establishes the architecture and uses a bounded luminance/chromaticity reference transform so that viewing state is explicit rather than hidden inside arbitrary RGB curves.

## Scene-to-display colorimetry

The input supplies a declared scene-RGB to XYZ matrix and the target supplies an XYZ-to-display-RGB matrix.

This avoids silently treating camera-native Scientific Master RGB as sRGB.

A caller must provide the admitted colorimetric binding.

## Appearance reference

The current reference performs:

1. scene-linear exposure scaling;
2. declared RGB -> XYZ conversion;
3. reference-white / surround / adaptation-dependent luminance mapping;
4. monotonic highlight compression toward target peak luminance;
5. chromaticity-preserving XYZ luminance scaling;
6. target XYZ -> display-linear RGB conversion;
7. appearance-only colorfulness scaling around the neutral axis;
8. explicit display/gamut clamp;
9. target transfer encoding.

Negative or >1 upstream scene values remain legal. They are only bounded at the final display boundary.

## Display targets

v0.7 implements three reference encodings:

- LINEAR_NORMALIZED;
- sRGB;
- PQ / ST 2084.

PQ is evaluated from absolute luminance in cd/m2 against its 10,000-nit reference.

This does not increase scene dynamic-range evidence. It only changes how an admitted scene/view result is encoded for a display.

## Authority preservation

The resolver copies channel authority and uncertainty from the deep-scene input unchanged.

It reports:

- appearanceApplied = true;
- displayEncoded = true;
- sourceSceneMutated = false;
- createsNewEvidence = false;
- scientificWritebackAllowed = false.

## Tests

The executable suite verifies:

- exact sRGB endpoint behavior and monotonic PQ encoding;
- neutral-axis stability;
- authority and uncertainty are unchanged;
- SDR/PQ display changes alter output identity but not scene identity;
- viewing-condition changes alter appearance only;
- highlights compress to target peak without scene mutation;
- negative/>1 scene values remain upstream while final display clamp is explicit;
- missing display identity fails closed.

## Main-code promotion rule

Once CI is green, v0.2-v0.7 may be compiled into the Android main native library as a downstream scene/appearance framework.

Promotion does not mean every existing preview/export is automatically rerouted through v0.7. Existing scientifically validated paths stay intact until a dedicated bridge opts into the new resolver.

The UI may, however, be reorganized immediately so its buttons and route descriptions correspond to the now-defined layers:

- PURE -> Scientific View;
- ADVANCED -> Appearance / Restoration View;
- PRO -> Open Scene / Deep / Light Transport workbench.

This improves user-facing logic without silently changing scientific pixel production.
