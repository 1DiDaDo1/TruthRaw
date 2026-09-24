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
