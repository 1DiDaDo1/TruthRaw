# TruthRaw Canonical Ancestry Spine v0.77

Status: **integration candidate — provenance-only, no pixel/science change**

This module makes the "every building block knows its origin" rule machine-readable for downstream derivatives.

## Bound parent graph

The canonical record binds, in fixed order:

`sealed source evidence -> Scientific Master -> Zero-Line -> scene-scale -> 180-byte Technical Backplane -> Canonical Open Scene -> derivative raster -> restoration role-mask`.

It also binds:
- width/height;
- physical frame count / independent evidence count;
- source evidence id;
- colour binding id;
- precision policy;
- runtime reconstruction backend;
- scientific coordinate space;
- derivative role.

The complete 180-byte Technical Backplane bytes are embedded in the canonical text as hex, and a separate SHA-256 of those bytes is recorded.

## Permanent boundaries

- source evidence immutable;
- Scientific Master not modified by the derivative;
- restoration/appearance cannot write back scientifically;
- no new evidence is created;
- counterfactual state cannot upgrade authority;
- representation may exceed the source representation;
- knowledge claims may not exceed evidence.

## v0.77 scope

v0.77 first fixes the projection provenance asymmetry.

The existing v0.71 Restoration DNG/TIFF/EXR pixels, role-mask semantics and Open Scene v0.70 identity stay unchanged. All three projections receive the same canonical ancestry manifest and manifest SHA.

PURE v0.63 is intentionally not modified. Its frozen self-binding remains the reference scientific projection contract.

Later Open Scene authority/uncertainty versions may add new child identities, but they must preserve this parent graph rather than replacing it.
