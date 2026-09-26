# TruthNegative Center-Excluded Neighborhood v0.2 — Validation Report

Date: 2026-09-26
Base: `c0e0872ffff2e24f685e7fccbe27cff3a2aa6e74`

## Scope

Host-only research predictor. No Android/PRO route integration, no Scientific
Master mutation, no TruthNegative mutation, no N2 production promotion.

## Verified properties

- predictor input contains no center observation value;
- symmetric same-channel support is evaluated without center radiometry;
- horizontal, vertical and both diagonal directions are representable;
- at least two agreeing directions are required per scale;
- conflicting symmetric pairs fail closed;
- a coarse scale that conflicts with admitted fine support is rejected;
- UNKNOWN/CENSORED/censor-boundary support is excluded;
- known cross-object support is excluded;
- `createsNewEvidence=false`;
- `scientificWritebackAllowed=false`.

## Host validation

- GCC C++20: PASS
- Clang C++20: PASS
- ASan/UBSan: PASS

The tests include flat multiscale support, diagonal-only support, a directional
edge conflict, coarse-scale conflict, authority/censor rejection and
insufficient-support fail-closed behavior.

## Non-claims

This does not prove spatially independent camera noise, does not estimate
DSNU/PRNU or row/column fixed-pattern structure, and is not yet allowed to
change an appearance candidate. The current v0.1 N2 route and the proven
reconstruction-support closure remain unchanged.
