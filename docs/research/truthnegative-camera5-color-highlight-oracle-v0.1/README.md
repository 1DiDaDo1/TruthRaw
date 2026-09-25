# TruthNegative Camera-5 Color / Highlight Oracle v0.1

Status: **EXECUTABLE DIAGNOSTIC ORACLE**

This oracle localizes the first defensible failure stage for the known Camera-5 bright-neutral / green-reveal problem without changing Scientific Master pixels.

It consumes source/master/TN/color-binding identities plus sampled camera-native, XYZ and EV-swept display values. Apparent white highlight candidates are selected from bright, low-chroma EV0 output. Their camera-native R/G and B/G ratios can then be compared with DNG AsShotNeutral, while lower exposure reveals whether a neutral-looking clipped highlight exposes a green-biased color binding.

Possible first stages are SOURCE_CENSORING, METADATA_NEUTRAL_MISMATCH, COLOR_BINDING, APPEARANCE_DISPLAY, NONE or UNRESOLVED.

The known Camera-5 diagnostic neutral around [0.59,1,0.55] versus the source DNG AsShotNeutral [0.8701,1,0.3457] is represented only as a **diagnostic mismatch pattern**. It is never promoted to calibration.

Remosaic remains unresolved unless independently sealed runtime evidence is supplied. The DNG alone may not manufacture that state.

Permanent outputs: createsNewEvidence=false and scientificWritebackAllowed=false.
