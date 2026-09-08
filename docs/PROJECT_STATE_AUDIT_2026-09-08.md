# TruthRaw A-to-Z repository/state audit — 2026-09-08

## Decision

**PASS WITH SCIENTIFIC BLOCKERS EXPLICITLY OPEN.**

The repository migration/integrity state is internally coherent after the v4.7i and v4.7j/v4.7k ancillary imports. No repository result justifies a FULL_PHYSICAL claim.

## Closed since the previous handoff

1. **LinearRaw v0.4 DNG validator gate — CLOSED / PASS** for the two exact frozen candidate hashes using `dng_validate` 1.7.1 (2611). Correct status: `DNG_SDK_VALIDATED_RELEASE_CANDIDATE`, still `DERIVED_RECONSTRUCTED_RAW`, not Adobe-certified.
2. **v4.7i canonical core migration — CLOSED / PASS.** Exact source bytes are now under `canonical/reconstruction/v4.7i/`; import CI passed.
3. **v4.7j/v4.7k ancillary completeness — CLOSED FOR THE SELECTED REPRODUCIBLE SUBSET.** Twenty-one compact files were added in one commit; Canonical Integrity run #22 passed.

## Important package-boundary decision

The historical v4.7j release ZIP contains a `native/CMakeLists.txt` that references four C++ test/benchmark sources absent from that release package and not recovered in the Library-wide C++ inventory. It was therefore **not** promoted as working canonical build support. The scientific core and standalone result/evidence files remain valid.

The v4.7k native CMake/CLI/benchmark layer was independently rebuilt with strict warnings before import and is suitable for an automated compile gate.

## Stale-state corrections

- The dated `state/CURRENT_CANONICAL_STATE_2026-09-06.json` is retained as a historical snapshot and is not rewritten.
- A new `state/CURRENT_CANONICAL_STATE_2026-09-08.json` becomes the current state record.
- The old `PENDING_DEEP_INSPECTION` APK task is not carried forward. Current binding policy is that APK evidence remains non-canonical and causes no TruthRaw changes unless explicitly reversed.
- README DNG wording is updated from an open validation gate to the exact v0.4 validator-PASS boundary.
- `state/REPOSITORY_MIGRATION_STATUS.json` is updated to the current migration closures and CI layers.

## Integrity-checker gap found and fixed

Before this audit the checker pinned v4.7j, PTC and uncertainty bytes but did not pin the newly imported v4.7i core or new v4.7j/v4.7k evidence. The checker is extended to bind these critical blobs. CI is also extended to compile the v4.7i core and build the standalone v4.7k native targets.

## Scientific state that remains open

TruthRaw stays `PURE_TRUTH_DERIVED` until all required physical evidence exists. For the priority tele camera the remaining physical blockers are:

- independent per-lens/unit color calibration;
- physical illuminant calibration;
- electron/PTC calibration by gain/readout state;
- optics calibration (PSF, MTF/SFR, chromatic displacement/CA, flare and residual color shading).

The high-ISO black/white-dog holdout remains a frozen uncertainty generalization failure (Q5 ratio 0.705044). It must not be used to retune the frozen model; future investigation requires new independent data.

## Claim boundaries retained

- Reconstructed values are never relabelled as measured photons.
- Original CFA/sample bytes and capture metadata remain immutable evidence.
- GainMap is applied exactly once; signal and corresponding noise/likelihood scale together.
- WhiteLevel clipping remains censored/lower-bound evidence.
- Scene-linear values are not clipped merely because they exceed 1.
- LinearRaw v0.4 is a derived reconstructed DNG, not original sensor RAW.
- DNG validator PASS is interoperability evidence, not Adobe certification or physical-truth certification.
- No APK-derived changes are canonical under the current binding policy.
- No open-source LICENSE is silently added.
