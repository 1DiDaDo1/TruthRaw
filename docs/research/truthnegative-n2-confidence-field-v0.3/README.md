# TruthNegative N2 Confidence Field v0.3

Status: **VECTOR-VALUED RESEARCH FIELD — AUDIT ONLY**

## Why this exists

The v0.2.1 whole-frame device audit showed that the existing v0.1 candidate
density and the center-excluded predictor coherence carry related but distinct
information. Quiet/noise areas tended to have high predictor coverage and high
directional/multiscale agreement, while structure and censor/highlight regions
showed more disagreement.

That does not justify a denoise-strength map.

v0.3 therefore stores a **vector of support measurements** rather than
collapsing them into a weighted probability.

## Inputs

v0.3 derives only from already admitted audit products:

- N2 CFA Audit v0.1;
- N2 Center-Excluded Spatial Audit v0.2.1;
- their exact SHA bindings to Source Evidence, Scientific Master, authority
  field and TruthNegative state.

It does not read or modify the Scientific Master.

## Per-tile vector

Each 64x64 source tile records:

- v0.1 candidate fraction;
- center-excluded predictor coverage;
- symmetric-pair acceptance;
- multiscale acceptance;
- center-only residual fractions <=1 sigma, 1-2 sigma and >2 sigma;
- v0.1 preserved fraction;
- Structure-protected fraction;
- CENSORED-protected fraction;
- censor-boundary-protected fraction;
- mean/max predictor-variance to center-variance ratio;
- maximum directional disagreement;
- maximum cross-scale disagreement;
- candidate CFA phase counts;
- predictor-valid CFA phase counts.

CFA phase is diagnostic only. It is not normalized into a claim that the four
phases should have equal candidate rates.

## No invented probability

v0.3 intentionally has no weighted scalar confidence probability.

There are no learned weights, no hand-tuned blend coefficients and no
threshold-derived denoise strength.

An ordinal support class is allowed only from exact logical conditions:

- NO_CANDIDATE: no v0.1 candidate center exists in the tile;
- UNRESOLVED: candidates exist but no center-excluded predictor is valid;
- MIXED: at least one support check rejects or an accepted center exceeds 2
  center-only sigma;
- FULLY_COHERENT: every candidate has a valid predictor, zero pair rejection,
  zero scale rejection and zero center residual above 2 sigma.

FULLY_COHERENT is **not** permission to denoise.

Every tile has:

    promotion_eligible=false

## Whole-frame support distance is not claimed

The proven reconstruction-support closure is currently a full-lattice 1:1
diagnostic using the reconstruction backend's required halo. The v0.2.1
whole-frame sidecar samples at period 8.

v0.3 therefore refuses to pretend that tile-level protection density is an
exact per-site reconstruction-support distance:

    support_distance_admitted=false

A later route may add exact support distance only if it can be computed on a
compatible lattice and cryptographically tied to the same protection mask.

## Permanent invariants

- Direct CFA immutable;
- Scientific Master immutable;
- TruthNegative immutable;
- existing N2 v0.1 candidate unchanged;
- reconstruction-support closure unchanged;
- no new evidence;
- no scientific writeback;
- no candidate application;
- no promotion eligibility.
