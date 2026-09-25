# Free-World Scientific Master + Open Scene Binding v0.3

Status: **EXECUTABLE RESEARCH BINDING — NOT PRODUCTION-PROMOTED**

This module directly binds the v0.2 continuous/area resolver to the existing D.RAW Scientific Master numeric plane and Open Scene Field v0.85 local authority plane.

## Core contract

For every loaded canonical 64x64 source tile:

1. read the camera-native Scientific Master RGB tile;
2. read the matching Open Scene Field source tile;
3. validate every Open Scene v0.85 ChannelRecord;
4. require valuePresent for every channel;
5. require the Open Scene field value to be **Float32 bit-identical** to the matching Scientific Master value;
6. map role / authority / uncertainty / censor-bound domain / contribution mask into the free-world resolver source state;
7. fail closed on any mismatch.

The Open Scene field therefore cannot silently describe a different numeric raster from the Scientific Master it claims to annotate.

## Preserved semantics

The binding transports:

- SourceMeasuredCfa / ScientificReconstruction / DenseProjection / RestorationDerivative role;
- CalibratedEstimate / Reconstructed / Censored / Unknown authority;
- admitted p95 uncertainty where present;
- SourceRawCode versus SceneLinear censor-bound domain;
- contribution provenance mask;
- single-frame / single-independent-evidence invariant.

## Bound rule

The v0.2 resolver was extended so censor bounds are not discarded.

A scene-linear lower bound is projected only when the **entire positive-weight footprint** for that channel is censored and every bound is already in the SceneLinear domain.

A SourceRawCode bound is never relabelled scene-linear.

Mixed censored/unknown/reconstructed footprints retain their conservative authority classification but carry no invented scene-linear lower bound.

## Current Camera-5 consequence

Under current Open Scene Field v0.85 source semantics:

- the physically sampled CFA channel can be CalibratedEstimate or Censored;
- the two reconstructed RGB channels remain numeric but authority UNKNOWN unless bounded reconstruction uncertainty is independently admitted.

The free-world resolver preserves that fact. A denser output raster does not upgrade those unknown channels.

This is intentionally stricter than a conventional demosaiced RGB image.

## Next gate

After this binding is stable, the next research layer can add a first Deep Scene Contribution packet above the 2D camera plane.

That packet must remain decomposable into evidence-constrained, inferred, restoration, counterfactual and appearance contributions. It may not rewrite this bound Scientific Master/Open Scene plane.
