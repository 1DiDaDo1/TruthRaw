# D.RAW — Raster-independent sample world realignment — 2026-09-29

This note promotes one architectural distinction into the active D.RAW project vocabulary.

## New explicit distinction

A camera raster is a sampling of an observation.

It is not the definition of the photographic world.

The sealed source therefore has two different roles:

1. **evidence** — exact bytes, sample values, CFA phase, metadata and source provenance;
2. **sampling geometry** — the positions at which this observation measured the world.

D.RAW is no longer required to use that same finite sampling raster as its scientific coordinate domain.

## Canonical wording

**Source Evidence has a boundary. The world and its pixels do not.**

For coordinate architecture:

**The source raster determines where D.RAW measured. It does not determine the raster on which D.RAW must think.**

The Dutch canonical wording is:

**Het bronraster bepaalt waar D.RAW heeft gemeten. Het bepaalt niet op welk raster D.RAW moet denken.**

## Pipeline placement

The lattice belongs immediately after source sealing/admission, not before it:

```text
physical/source observation
 -> immutable source bytes + SHA/provenance
 -> source sampling geometry
 -> Raster-Independent Sample Lattice
 -> measurement / reconstruction
 -> Scientific Master / authority / uncertainty
 -> D.RAWnegative
 -> Free World / Open Scene
 -> Appearance / finite projection
```

This order matters. D.RAW must never rewrite the historical observation in order to gain coordinate freedom.

## Information conservation

Changing coordinate representation does not lose source information if every original sample remains an exact anchored constraint.

It also does not create information.

Therefore the project must distinguish:

```text
coordinate density
measurement density
reconstruction density
output pixel density
optical resolving power
```

These are not interchangeable.

A coordinate domain may be extremely fine while measurement density remains exactly the original sensor sampling.

## Why this helps

The architecture removes artificial questions created by the source raster.

For example, structure support need not mean:

`this entire 32x32 source tile is protected`

It can mean:

`these exact measured support samples occupy these coordinates and constrain this local region by this geometry`.

Similarly, future noise estimation can reason over a local latent function constrained by measured anchors rather than being forced to assign all meaning to source-pixel cells.

## Permanent evidence boundary

No future lattice implementation may silently turn UNKNOWN positions into MEASURED positions.

If a later solver estimates a value between measured anchors, its authority must remain reconstructed/estimated with explicit ancestry and uncertainty.

This note does not authorize denoising writeback, hidden-colour invention, super-resolution claims or optical-resolution claims.
