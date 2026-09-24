# Open Scene Field v0.85 + TruthNegative Local Authority Projection v0.4

This production line turns the Open Scene / TruthNegative scientific philosophy into a local data model.

## Local channel state

Every RGB channel may now carry, independently:

- the Float32 scientific value (bound by Scientific-Master/projected-raster identity);
- creation role;
- scientific authority;
- uncertainty knowledge class;
- optional p95 uncertainty;
- optional scientific support;
- optional censor/lower bound plus its domain;
- contribution provenance mask.

Creation role and authority are intentionally separate. A TruthNegative 4x pixel can therefore be:

`role=DENSE_PROJECTION, authority=UNKNOWN, uncertainty=UNRESOLVED`

without pretending the target pixel was measured.

## Compact storage

The field is encoded per canonical 64x64 tile.

Classification uses a palette:

- uniform tile;
- 2-bit palette (up to four local states);
- 4-bit palette (up to sixteen states);
- dense 32-bit fallback.

Float scalars are stored only when locally known. Unit support for directly measured CFA samples is implicit and consumes no Float32 side payload.

This makes the field practical for very large Open Scene / TruthNegative rasters while preserving the ability to attach richer uncertainty later.

## Dense projection

TruthNegative local projection v0.4 uses the exact same 4x pixel-centre footprint as dense RGB projection.

## Procedural 200 MP field binding

The 16320x12288 Full Colour TruthNegative DNG does not duplicate hundreds of millions of RGB values or materialize a second giant metadata raster merely for provenance.

Instead v0.4 creates a procedural local-field artifact bound to:

- sealed source-evidence SHA-256;
- Scientific-Master SHA-256;
- canonical Open Scene parent SHA-256;
- exact projected-raster SHA-256;
- source and target geometry;
- exact F64 reconstruction backend identity;
- the v0.4 projection policy.

Therefore any target `(x,y,channel)` can be deterministically re-evaluated through the same local projection operator. The value is bound by the projected-raster identity; role/authority/uncertainty/bounds are derived by the local field operator. A one-bit projected-raster identity mutation changes the procedural field artifact.

This avoids a third full RGB projection pass and avoids a mandatory hundreds-of-megabytes field sidecar while retaining local queryability.


Rules are fail-closed:

- target creation role is always `DENSE_PROJECTION`;
- no target measured claim is created;
- interpolation never promotes uncertainty by itself;
- a RAW-code censor bound is never relabelled as scene-linear;
- a mixed censored footprint remains `UNKNOWN` unless a safe scene-linear lower-bound rule exists;
- only an all-censored scene-linear footprint may carry a convex scene-linear lower bound.

## Local consumer policy

Open Scene Local Policy v0.86 provides one authority interpretation for Restoration, HDR and future detail:

- calibrated measured channel -> preserve/direct-evidence support;
- admitted reconstructed channel -> reconstruction-bound support;
- censored -> bound-only, exact recovery forbidden;
- unknown -> appearance-only/no scientific writeback, HDR headroom forbidden.

This policy does not make current appearance detail a scientific reconstruction and does not grant optical authority.

## Current Camera-5 behavior

Current Camera-5 processing DNG reconstruction uncertainty is not independently admitted. Therefore:

- measured uncensored CFA channel -> `CALIBRATED_ESTIMATE`;
- measured clipped CFA channel -> `CENSORED` with `SOURCE_RAW_CODE` bound;
- two reconstructed RGB channels -> numeric Scientific-Master values but authority `UNKNOWN`;
- dense TruthNegative target channels -> `DENSE_PROJECTION + UNKNOWN + UNRESOLVED` unless a later bound uncertainty model is admitted.

This is intentional.

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## TN-4 native Open Scene container

The native TruthNegative container is now version 4:

`TRUTHNEGATIVE_V0_4_TN4`

Each canonical 64x64 cell retains the legacy TN-3 RGB/authority/Open-Scene bytes for compatibility and adds a length-prefixed compact Open Scene Field v0.85 block.

The native result packet reports:

- encoded Open Scene Field storage bytes;
- exact RGB-channel record count;
- local bound count;
- local direct-support count.

Kotlin verifies those counts against the source dimensions and censor count before accepting the export.

## Restoration / HDR / detail consumption

Open Scene Local Policy v0.86 is the shared interpretation layer.

Full-resolution Restoration now builds the local v0.85 field for each working tile and:

- preserves uncensored measured source support;
- triggers only from `CENSORED / CENSOR_BOUND_ONLY`;
- excludes non-preserve local neighbours from scientific restoration support;
- never allows exact censored recovery or Scientific-Master writeback.

The same policy exposes tile-level HDR and detail disposition summaries:

- direct measured channels may provide direct scientific support;
- admitted reconstructed channels require bounded uncertainty;
- censored channels block exact scientific HDR gain;
- unknown channels block scientific HDR headroom and scientific detail support.

Current Natural HDR and v4.7j detail remain appearance-only; the new local policy does not promote them to scientific reconstruction.
