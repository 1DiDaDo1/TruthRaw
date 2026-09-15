# TruthRaw Precision Sweep Method Resolution v0.9

Date: 2026-09-15  
Status: **RESOLVED — FORMAL C++ SWEEP IS THE NUMERICAL REFERENCE FOR v0.4 COUNTS**  
Authority: numerical research only; no photographic evidence upgrade and no canonical promotion

## Why this document exists

Two aggregate count sets appeared during development:

- preliminary local sweep: `3545` direction divergences and `0/0` green/color clamp divergences;
- formal repository sweep: `3549` direction divergences, `44` green-clamp divergences and `2580` color-clamp divergences.

These values must not be silently merged or treated as interchangeable.

## Resolution

The repository-backed v0.4 result is the reference count set for this research line:

- `greenDirectionDivergence = 3549`
- `greenClampDivergence = 44`
- `colorClampDivergence = 2580`
- `measuredChannelViolationsF32 = 0`
- `measuredChannelViolationsF64 = 0`

The reference artifact is:

`TRUTHRAW_REAL_RECONSTRUCTION_PRECISION_CPP_SWEEP_v0_4.json`

Its declared method is a full-frame tiled sweep in compiled C++ using the frozen-v4.7i precision-instrumented topology with `core=256` and `halo=4`.

## Cause of the discrepancy

The earlier `3545 / 0 / 0` result was a preliminary development sweep, not the final v0.4 reference. During that phase the working method used a different local sweep/aggregation path and a 3-pixel halo while the branch-sensitive reconstruction tooling was still being consolidated. It did not provide the same complete clamp-decision trace accounting that the final compiled C++ reference provides.

The final C++ comparator explicitly records and compares three separate decision classes:

1. green direction decision (`Horizontal`, `Vertical`, `WeightedBlend`);
2. green support-limit clamp decision (`Low`, `None`, `High`);
3. color support-limit clamp decision for reconstructed RGB components.

Therefore `0/0` clamp divergences from the preliminary path are not evidence that clamp divergences do not exist. They are superseded by the instrumented C++ sweep for aggregate v0.4 counts.

## Why halo=4 is retained

The current research runtime requires `config.halo >= 4` for the F64 audit stream. The reconstruction itself has a multi-pass dependency: directional green estimation samples out to two CFA positions, and later color-difference reconstruction consumes reconstructed green/support values around the core. The four-pixel research halo is therefore retained as the validated v0.4/v0.8 reference condition rather than shrinking the support merely because a smaller halo can appear sufficient in selected interiors.

## Scientific consequence

This resolution changes no photographic claim. It only fixes which numerical experiment owns the aggregate counts.

The conclusions remain:

- F32 branch-sensitive reconstruction is rejected as the scientific numerical reference for the tested v4.7i topology;
- F64 branch-sensitive reconstruction remains the reference;
- measured CFA-channel reinjection has zero violations in both tested precision paths;
- F64 -> F32 post-compute storage remains a research candidate, pending uncertainty-relative and 200MP gates.

## Governance rule added by v0.9

Future aggregate precision reports must record, at minimum:

- implementation identity;
- tile core size;
- halo size;
- Stage-2/GainMap implementation identity;
- direction/clamp trace coverage;
- source hashes;
- whether the result is locator evidence, deterministic C++ fixture evidence, or promotion-gate evidence.

Counts produced under different support/trace configurations must never be numerically merged.
