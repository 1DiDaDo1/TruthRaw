# Scientific Master Exact Gauge Retained Artifact v0.3

Status: **SCALAR HOST CANDIDATE IMPLEMENTED — EXACT HOST PARITY GREEN — ANDROID RUNTIME NOT YET WIRED — NOT PROMOTED**

This document defines and records the isolated D.RAW Scientific Master performance candidate that follows real-device Center-Excluded optimization, Scientific Master bind profiling and two-pass tile-read attribution.

It does **not** promote a new scientific model and does not authorize any change to Scientific Master values, authority, reconstruction, calibration, source evidence, restoration, appearance or writeback.

The canonical v0.2 two-pass binder remains the semantic fallback authority.

## 1. Established problem

The canonical Scientific Master streaming binder v0.2 traverses the canonical tile schedule twice.

On each of the two established device RAWs the measured/attributed count is:

- `3072` pass-1 Stage-2/RAW reads;
- `3072` pass-2 Stage-2/RAW rereads;
- `6144` total reads;
- `3072` reconstruction calls.

Pass 2 performs no reconstruction, no Scientific Master digest addition and no canonical observer callback. Its remaining scientific purpose in the inspected v0.2 source is to resolve the exact low 16 bits of the self-gauge median after pass 1 has selected the high-16 bucket(s).

The optimization target is therefore deliberately narrow: retain only the exact already-observed information that this low-16 selection needs, rather than cache a whole Scientific Master or redefine the processing model.

## 2. Implemented candidate

The scalar v0.3 host candidate now retains the exact IEEE-754 Float32 bit pattern (`uint32_t`) of every sample already admitted by the unchanged v0.2 self-gauge eligibility predicate during pass 1.

After high-16 rank selection, the candidate consumes that temporary exact-bit ledger to build the same low-16 histogram/order statistic that v0.2 obtains by rereading Stage-2 tiles.

The candidate therefore uses one Stage-2 gauge scan instead of two for an admitted input while preserving the canonical Scientific Master and self-gauge semantics.

The retained representation is an implementation artifact. It is not source evidence, calibration, restoration state, appearance state or a new Scientific Master.

## 3. Open-world D.RAW architecture

D.RAW seals source evidence and provenance; it does not seal the scientific world built above them.

A current RAW encoding, JPG encoding, Stage-2 representation, canonical raster, Float32 path, Float64 path, Truth Zero representation, reconstruction model or pass schedule is never a universal boundary on future interpretation.

The expansion law remains:

**general semantic route -> versioned specialized route -> exact parity oracle -> fail-closed fallback**

The Exact Gauge ledger is only the first concrete pass artifact. Future pass artifacts may carry different intermediate products without changing Scientific Master meaning or closing D.RAW's free-resolution/free-raster world.

## 4. Canonical v0.2 remains the fallback authority

v0.3 does not replace or delete the established v0.2 two-pass binder.

Complete fallback to v0.2 is required when an admitted specialized route cannot be proven safe, including when:

- retained-artifact budget is insufficient;
- total scientific resident-memory budget is insufficient;
- artifact allocation fails;
- representable cardinality/bounds fail;
- eligibility semantics/version do not match exactly;
- topology or pass-consumer contract is unsupported;
- a future pass-2 consumer requires information absent from this artifact;
- any exact-parity or integrity precondition is not satisfied.

The first candidate does not permit a partial mixed scientific path. An artifact-admission failure selects the complete canonical v0.2 route.

## 5. Exact inspected eligibility contract

The source-inspection gate is complete and is recorded in `SOURCE_INSPECTION_CHECKPOINT.md`.

For a canonical-core sample, v0.2 admits Stage-2 into the self-gauge only when:

- it lies inside the canonical 10% border exclusion;
- RAW and Stage-2 tile-local indices are valid;
- `float(raw) < WhiteLevel` on the original RAW code;
- `Stage2 > 0.0f`;
- `Stage2` is finite.

Consequences include exclusion of `+0.0f`, `-0.0f`, NaN and infinities from the gauge. In the full binder, a non-finite Stage-2 value can fail even earlier if reconstruction causes the Scientific Master digest to encounter NaN/Inf; v0.3 preserves that earlier failure instead of broadening tolerance.

Because the admitted domain is strictly positive finite Float32, unsigned binary32 bit-pattern ordering is monotonic with numerical ordering. That proof is specific to this eligibility contract and may not be reused if eligibility is broadened later.

## 6. Exact median/rank semantics

For eligible count `N`:

- lower rank = `(N - 1) / 2`;
- upper rank = `N / 2`;
- ranks are zero-based.

For odd `N`, both ranks resolve to the same exact Float32 value.

For even `N`, each selected Float32 value is converted to `double`, then v0.2 computes:

`L0 = lowerDouble + (upperDouble - lowerDouble) * 0.5`

The v0.3 candidate reproduces this operation rather than substituting an approximate or rearranged average.

## 7. Stage-2 meaning and Truth Zero boundary

Before self-gauge eligibility, the established `fill_stage2` path:

1. selects the CFA-phase black level;
2. adds row/column residual black when present;
3. normalizes as `(raw - black) / max(WhiteLevel - black, 1.0f)`;
4. applies the gain field exactly once when present;
5. stores the result as Float32 Stage-2.

The retained artifact only remembers exact bits that already exist after these canonical operations. It does not recalibrate, alter black or white level, redefine Truth Zero, restore data, reconstruct additional evidence, change Float32/Float64 authority or restrict future precision choices.

## 8. Cardinality and memory admission

For dimensions `W x H`, the retained geometric upper bound is derived from the unchanged v0.2 10% border rule:

`(W - 2*borderX) * (H - 2*borderY)`

Actual retained count is less than or equal to this because clipped, non-positive and non-finite Stage-2 samples remain excluded.

For the established `4080 x 3072` geometry:

- `borderX = 408`;
- `borderY = 307`;
- geometric maximum = `8,022,912` retained positions;
- `uint32_t` ledger payload maximum = `32,091,648` bytes;
- approximately `30.60 MiB`.

This is a dimension-specific bound, not a universal D.RAW memory constant.

The candidate exposes two distinct budget concepts:

- the established `Options::memoryBudgetBytes` remains the total scientific logical-resident ceiling;
- `ArtifactOptions::retainedArtifactBudgetBytes` is an optional artifact-only admission ceiling.

A too-small artifact ceiling causes complete v0.2 fallback without redefining the general scientific memory contract.

Requested, reserved and used retained bytes are diagnostic-visible. The temporary ledger does not persist as scientific evidence.

## 9. Pass-artifact identity

Current host contract:

- artifact type: `EXACT_GAUGE_FLOAT32_BITS`;
- artifact version: `0.3`;
- producer semantics: `SMSB_V0_2_STAGE2_SELF_GAUGE`;
- consumer semantics: `EXACT_GAUGE_LOW16_V0_3`;
- eligibility identity: `SMSB_V0_2_POSITIVE_FINITE_RAW_LT_WHITE_CENTER80`.

The implementation remains scalar, deterministic and single-threaded. SIMD/NEON and concurrency are deferred until after real-device proof.

## 10. Host exact-parity result

The latest clean research-branch host gate completed successfully after removal of the superseded test draft.

The CI gate builds with warnings-as-errors and ASan/UBSan enabled and runs the exact-parity oracle against canonical v0.2.

The active oracle covers:

- pseudo-random data;
- constant even-cardinality median;
- constant odd-cardinality median;
- split median;
- sparse eligibility;
- signed-zero gain behavior;
- residual-black plus gain behavior;
- exact expected constant Stage-2 Float32 median bits;
- repeated-run determinism;
- canonical observer parity;
- non-finite Stage-2 rejection at the existing earlier Scientific Master boundary;
- explicit retained-artifact-budget fallback to complete v0.2.

For successful admitted synthetic cases, the oracle requires exact scientific parity for the Scientific Master SHA-256, self-gauge L0 bit representation, gauge metadata, scene binding, eligible count, tile count and single-frame/evidence identity while verifying that canonical v0.2 performs two gauge scans and the v0.3 candidate performs one.

This host result is evidence that the scalar specialization matches the tested v0.2 semantics. It is **not** a real-device performance result and does not by itself authorize promotion.

## 11. Diagnostics are not scientific evidence

The candidate may report:

- requested/actual route;
- artifact type/version;
- admission/fallback status;
- geometric and eligible retained counts;
- requested/reserved/used retained bytes;
- artifact and total resident budgets;
- selected lower/upper median Float32 bits for parity diagnostics;
- actual Stage-2 gauge scans;
- pass-2 rereads avoided;
- logical resident upper bound;
- timing provenance once Android integration exists;
- `optimization_applied`.

Route, memory, read-count and timing diagnostics do not affect Scientific Master identity or authority.

## 12. Permanent safety statement

This is a performance artifact only.

`source_values_modified=false`

`creates_new_evidence=false`

`scientific_writeback_allowed=false`

`restoration_authority_created=false`

`appearance_authority_created=false`

`truth_zero_redefined=false`

`free_resolution_or_raster_restricted=false`

`float32_declared_universal=false`

`float64_declared_universal=false`

The artifact may remember computation. It cannot become observation.

## 13. Android/device acceptance plan — next gate

Android runtime wiring has **not** yet been performed and the candidate is **not promoted**.

Before wiring, the Android bridge/CMake/telemetry path must be inspected so the specialized route remains selectable/diagnostic and canonical v0.2 remains a complete fail-closed fallback.

Only after inherited Android CI/build gates are green may a higher-versionCode APK be produced for the established real-device test. Device acceptance requires:

1. use the same established RAWs and a true current-run/cold timing path;
2. verify v0.3 admission and `optimization_applied=true` only on supported inputs;
3. expect approximately `3072` rather than `6144` Stage-2/RAW tile reads per RAW if the second reread is fully eliminated;
4. compare complete scientific Foundation content outside performance diagnostics against canonical v0.2;
5. inspect real resident-memory telemetry and fallback behavior;
6. preserve source/provenance/Scientific-Master/firewall invariants;
7. reject promotion on any scientific mismatch regardless of speed.

The read-count reduction is an engineering expectation, not a scientific criterion.

## 14. Deliberate non-goals of this first candidate

Do not combine this first retained-artifact experiment with:

- new reconstruction or model selection;
- restoration-model changes;
- new calibration/correction;
- Truth Zero changes;
- free-resolution/free-raster restrictions;
- Float32/Float64 architectural restrictions;
- Center-Excluded changes;
- further authority SHA micro-optimization;
- SIMD/NEON;
- full-frame Scientific Master caching;
- evidence promotion;
- appearance/export changes;
- changes to self-gauge meaning.

Keeping this experiment isolated is what makes the later device performance and exact-parity evidence attributable.
