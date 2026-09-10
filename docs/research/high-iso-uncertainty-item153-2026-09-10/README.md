# TruthRaw — High-ISO uncertainty Item 153

This module preserves the frozen-v5.0g evaluation of the post-freeze HONOR BKQ-N49
22.48 mm tele ISO1600/3200/6400/12800 sweep.

## Project-level status

`SCORED_MIXED_FAIL_GENERALIZATION_REVIEW_OPEN_NO_RETUNE`

Literal frozen scorer outcomes:
- ISO1600: FAIL
- ISO3200: FAIL
- ISO6400: FAIL
- ISO12800: PASS

All four satisfy the global p50/p95 coverage and risk-ordering gates. ISO1600–6400
fail the all-risk-quintile requirement because the Q5 predicted/observed p95 ratio
falls below the frozen 0.75 lower bound.

## Evidence class

The captures were acquired after the v5.0g freeze, but were exposed to implementation,
parity, tile and memory work before this scoring. They are therefore retained as
implementation-exposed generalization evidence, not promoted as pristine blind
prospective certification.

## Non-negotiable interpretation

- Frozen v5.0g coefficients, feature schema, gates and reconstruction backend remain unchanged.
- The historical 094414 failure remains binding.
- The broader uncertainty-generalization blocker remains open.
- These four captures are not independent promotion holdouts for a successor model.
- CI/integrity PASS means the evidence package and frozen bindings are intact; it does not
  reinterpret the mixed scientific result as PASS.

**Measured where measured. Reconstructed where necessary. Never invented.**
