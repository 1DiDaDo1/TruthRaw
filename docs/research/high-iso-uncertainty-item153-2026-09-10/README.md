# TruthRaw — High-ISO uncertainty Item 153

This research-evidence module preserves the 2026-09-10 evaluation of the exact frozen v5.0g uncertainty scorer on the post-freeze HONOR BKQ-N49 BnCam 22.48 mm tele ISO1600/3200/6400/12800 sweep.

## Project-level decision

`SCORED_MIXED_FAIL_GENERALIZATION_REVIEW_OPEN_NO_RETUNE`

Literal frozen-scorer outcomes:

- ISO1600: **FAIL** — Q5 predicted/observed p95 ratio `0.565435`
- ISO3200: **FAIL** — Q5 ratio `0.647388`
- ISO6400: **FAIL** — Q5 ratio `0.715192`
- ISO12800: **PASS** — Q5 ratio `0.794887`

All four satisfy the global p50/p95 coverage and risk-ordering gates. ISO1600–6400 fail only the frozen all-risk-quintile requirement because Q5 is below the `0.75` lower bound.

## Evidence class

The captures were acquired after the v5.0g freeze, but had already been exposed to implementation/parity/tile/memory work before this scoring. They are therefore **implementation-exposed generalization evidence**, not pristine blind prospective certification.

## Binding interpretation

- Frozen v5.0g coefficients, feature schema, scorer gates and reconstruction backend remain unchanged.
- The historical 094414 failure remains binding.
- The broader uncertainty-generalization blocker remains open.
- These four captures must not be reused as independent promotion holdouts for a successor model.
- Integrity/CI PASS means only that this retained evidence package and the frozen bindings are byte-consistent; it does **not** reinterpret the mixed scientific result as PASS.

**Measured where measured. Reconstructed where necessary. Never invented.**
