# TruthRaw v5.0e — Unified Material Truth Policy

## Runtime rule
The runtime engine does not know whether a pixel belongs to water, wood, grass, a feather, fur, skin, architecture or any other semantic object.

It may use only measurable evidence fields:
- texture-detail support
- thin/coherent-structure risk
- hard-edge risk
- glint/censoring risk
- low-SNR risk
- smooth/low-frequency diagnostic structure

## Meaning of the visualization
- Red = limiting/risk field
- Green = texture-detail support (NOT nature, foliage or object classification)
- Blue = smooth/low-frequency diagnostic cue

Therefore a wooden window frame may correctly appear green when its texture is high-confidence detail. This is not a claim that wood is being classified as nature.

## Hard constraints
1. Unified Material Truth may never create semantic detail.
2. It may not exceed the canonical v4.7j detail delta.
3. Broad scene luminance is anchored to Neutral Reference.
4. Detail changes use a uniform RGB scalar and may not intentionally rotate hue/chroma.
5. Source-white/censored evidence remains a provenance fact.
6. Material decomposition claims (reflection/transmission/species/material identity) require independent evidence and are not inferred here.
7. Scientific RAW/Stage-2/XYZ masters remain unchanged.
8. Every export is evaluated by PTC v1.1.
