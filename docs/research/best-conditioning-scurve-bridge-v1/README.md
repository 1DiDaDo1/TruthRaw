# TruthRaw Best Conditioning → S-Curve Bridge v1

Research bridge between evidence-neutral **Best Conditioning Gauge** and appearance-only uncertainty-aware tone/color processing.

## Core rule

Best Conditioning EV is a numerical gauge, not a photographic exposure recommendation and not an evidence multiplier. For power-of-two conditioning,

`mean' = 2^EV mean`, `sigma' = 2^EV sigma`, therefore `|mean'|/sigma' = |mean|/sigma`.

The local EV itself is deliberately absent from the appearance API. Upstream Manifold Conditioning must decondition first. Only gauge-invariant/deconditioned evidence such as qualified confidence, topology/censor state and the one-frame evidence ledger may cross the boundary.

## Appearance optimization

The existing v1 pixel S-curve varies shadow curve strength with local luma confidence. That is mathematically useful for compressing visible noise, but an abrupt confidence field can create a luminance seam even when the underlying RGB field is perfectly flat.

This bridge tests a safer decomposition:

1. one global monotonic S-curve;
2. edge-aware confidence regularization;
3. edge-aware local tone base;
4. uncertainty-dependent compression of the **local residual around that base**, not of the base luminance itself;
5. confidence-aware chroma restraint/enhancement with censored support capped at gain <= 1;
6. scientific master never modified.

On a perfectly flat RGB field, the local residual is zero, so even a discontinuous confidence map cannot create a luminance edge through the residual-compression stage.

## Status

`RESEARCH_PASS_NOT_YET_CANONICAL_APPEARANCE_REPLACEMENT`

The bridge is suitable for repository research promotion after CI, but it does not yet replace the existing uncertainty-aware S-curve module. Broader real-scene, HDR/SDR, colored-low-light and highlight-censor generalization remain required.
