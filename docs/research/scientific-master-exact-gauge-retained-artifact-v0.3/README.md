# Scientific Master Exact Gauge Retained Artifact v0.3

Status: **DESIGN CHECKPOINT — CODE NOT YET STARTED**

This document defines the next isolated Scientific Master performance candidate after real-device Center-Excluded optimization, Scientific Master bind profiling and two-pass tile-read attribution.

It does **not** promote a new scientific model and does not authorize any change to Scientific Master values, authority, reconstruction, calibration, source evidence or writeback.

## 1. Established problem

The canonical Scientific Master streaming binder v0.2 currently traverses the canonical tile schedule twice.

On each of the two established device RAWs the measured/attributed count is:

- `3072` pass-1 Stage-2/RAW reads;
- `3072` pass-2 Stage-2/RAW rereads;
- `6144` total reads;
- `3072` reconstruction calls.

The second pass performs no reconstruction. Its remaining scientific purpose is to resolve the exact low 16 bits of the self-gauge median after pass 1 has established the selected high-16 bucket.

The optimization target is therefore not "cache the whole Scientific Master". It is narrower: avoid recomputing/re-reading information that pass 1 already observed and that pass 2 needs only for the exact self-gauge selection.

## 2. Candidate hypothesis

During pass 1, retain the exact Float32 bit patterns of only the samples that the existing v0.2 self-gauge eligibility logic already admits.

The retained representation is an implementation artifact, not evidence and not a new scientific state.

After the existing high-16 selection is complete, the v0.3 route may use those retained exact bits to build the same low-16 histogram/order statistic that v0.2 obtains through a second Stage-2 tile traversal.

If the result is bit-identical, pass 2 no longer needs to reread the RAW/Stage-2 tiles for this purpose.

## 3. Architecture: pass artifacts, not a hard-coded two-pass shortcut

The user's expansion-space requirement is binding: the current two-pass implementation must not become the universal architecture merely because it is today's optimization target.

The v0.3 design therefore introduces a versioned **pass-artifact contract** above the canonical v0.2 route.

Conceptually, an admitted artifact has at least:

- artifact type/version;
- producer semantic version;
- consumer semantic version or compatibility contract;
- explicit byte budget and retained-byte count;
- explicit eligibility/selection contract identity;
- deterministic lifecycle: begin, append/observe, finalize, consume, release;
- diagnostic-only route/fallback status;
- no authority of its own.

The Exact Gauge retained ledger is the first concrete artifact type. The abstraction must remain able to carry different future intermediate products for different future passes without changing the meaning of Scientific Master.

The general law remains:

**general semantic route -> versioned specialized route -> exact parity oracle -> fail-closed fallback**

## 4. Canonical v0.2 remains the fallback authority for behavior

v0.3 must not replace or delete the established v0.2 two-pass binder.

Fallback to the complete v0.2 route is mandatory when any of the following is true:

- memory budget cannot be reserved safely;
- artifact allocation fails;
- topology is unsupported;
- eligibility semantics/version do not match exactly;
- pass schedule/consumer contract differs from the one proven by the v0.3 oracle;
- retained count exceeds a declared bound;
- an integrity/parity precondition is not satisfied;
- a future extension requires data not represented by this artifact.

A partial mix such as "some tiles retained, remaining tiles reread with different selection semantics" is not an accepted scientific path for the first candidate. On admission failure, return to the complete canonical route.

## 5. Exact Gauge retained representation

The first implementation candidate should prefer the smallest representation that preserves the exact v0.2 decision.

The starting hypothesis is a deterministic ledger of exact IEEE-754 Float32 bits (`uint32` representation) for samples already admitted by the existing self-gauge eligibility rule.

Important constraints:

- do not round, normalize or re-encode the bits;
- do not retain a target value that the canonical algorithm did not already inspect for the gauge;
- do not use retained values to alter reconstruction or authority;
- do not broaden eligibility merely to simplify storage;
- retain canonical observation/order information only if the exact current median semantics require it;
- derive the exact memory/cardinality bound from source inspection before choosing the final container.

This document intentionally does not guess the final byte budget or exact eligible-sample count. Those must be derived from the current v0.2 source contract before code is written.

## 6. Memory-budget contract

The optimization explicitly trades bounded temporary memory for fewer Stage-2/RAW rereads.

Therefore:

- a memory budget is a route-admission requirement, not a best-effort suggestion;
- requested/reserved/used/peak retained bytes must be diagnostic-visible;
- failure to reserve the declared budget causes complete v0.2 fallback;
- `residentPeakBytes` may truthfully increase and is not required to match v0.2;
- Scientific Master values/hashes and every authority/safety gate still must match exactly;
- no artifact may persist as scientific evidence after its computational lifetime.

The first code candidate should remain scalar/deterministic. SIMD/NEON is a later optimization only after the scalar v0.3 route is proven exact.

## 7. Mandatory exact-parity oracle

Host/CI promotion to a device APK requires exact comparison of v0.2 and v0.3 for the same admitted input.

At minimum compare:

1. exact Scientific Master SHA-256;
2. exact self-gauge median Float32 bits;
3. exact encoded gauge metadata/bit representation where exposed;
4. exact per-sample scientific values and states;
5. exact measured/reconstructed/censored/unknown counts;
6. exact authority digest, record counts and classification counts;
7. exact reconstruction outputs and quality/status fields;
8. exact source/anchor immutability flags;
9. exact promotion/firewall state;
10. repeated-run determinism;
11. exact result after forced memory-budget fallback;
12. exact result after unsupported-topology/semantic-version fallback.

Any scientific mismatch blocks the fast path. Timing improvement never overrides parity failure.

## 8. Required diagnostics

Performance diagnostics may report, without affecting scientific identity:

- route requested;
- route actually used;
- artifact type/version;
- admission status;
- fallback reason;
- eligible retained sample count;
- retained bytes;
- budget bytes;
- peak resident delta/upper bound;
- pass-2 Stage-2 rereads avoided;
- source-read call counts;
- current-run timing provenance;
- `optimization_applied`.

Diagnostics must explicitly state that timing, memory and route choice are not scientific evidence and may not change scientific authority.

## 9. Device acceptance plan

Only after host exact parity and the full inherited CI/build matrix are green:

1. build a higher-versionCode Android candidate;
2. install over the current app without clearing app data unless the validation design explicitly needs a cache epoch change;
3. use the same two established RAWs;
4. require a true current-run/cold execution for timing evidence;
5. export Free World Observation Geometry Foundation v0.1 JSON;
6. verify v0.3 route admission and `optimization_applied=true` for supported inputs;
7. expect approximately `3072` rather than `6144` Stage-2/RAW tile reads per RAW if the second reread pass is fully eliminated;
8. compare complete scientific Foundation content outside performance diagnostics against the canonical baseline;
9. inspect memory/resident telemetry and fallback behavior;
10. do not promote if speed improves but any exact parity/firewall condition fails.

The read-count reduction is an engineering expectation, not a scientific criterion.

## 10. Deliberate non-goals for v0.3 first candidate

Do not combine this work with:

- further authority byte/SHA micro-optimization;
- Center-Excluded changes;
- SIMD/NEON;
- new reconstruction/model selection;
- new calibration or correction;
- full-frame Scientific Master caching;
- evidence promotion;
- appearance/export changes;
- changes to the meaning of self-gauge.

Keep the experiment isolated so device performance and parity remain attributable.

## 11. Source-inspection gate before implementation

Before writing v0.3 code, inspect and document from the current merged source:

- the exact v0.2 gauge eligibility predicate;
- where high-16 counts are accumulated;
- exact median/rank convention for even/odd cardinalities;
- whether canonical iteration order contributes to any tie/rank semantics;
- exact Float32 bit treatment including NaN/Inf/signed-zero policy if reachable;
- exact eligible cardinality on the established RAW dimensions;
- current workspace/resident memory bounds;
- all call sites that depend on the second pass beyond low-16 gauge resolution.

If source inspection shows pass 2 has another scientific consumer, the retained-artifact design must be expanded or abandoned; do not silently drop that consumer.

## 12. Permanent safety statement

This is a performance artifact only.

`source_values_modified=false`

`candidate_applied=false`

`creates_new_evidence=false`

`scientific_writeback_allowed=false`

The artifact can remember computation. It cannot become observation.