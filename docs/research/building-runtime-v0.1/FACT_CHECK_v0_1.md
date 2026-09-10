# Building Runtime v0.1 — Fact Check

## PASS — evidence neutrality
A scheduler can route computations without creating independent samples. The runtime therefore pins one physical frame and one independent evidence root and rejects contradictory input/corridor state.

## PASS — authority and EV separation
Physical capture exposure, exact numerical conditioning gauge and appearance exposure have different semantics. They are stored independently; Appearance EV is never derived from Best Conditioning EV. Floor transitions are monotone and Scientific rooms cannot consume Counterfactual/Appearance state.

## PASS — fail-closed unknowns
Rooms that require sigma are blocked if sigma is unknown. Non-finite critical scientific state, impossible evidence counts, pre-modified scientific masters and invalid corridor tokens are rejected rather than guessed through.

## PASS — bounded mobile architecture
The orchestration hot path uses fixed-size arrays and POD-like tokens. It has no heap allocation, strings, filesystem access, GPU requirement or dynamic module discovery. Pixel data is represented by an external handle in a `CorridorToken`, not copied into the scheduler.

## PASS — flexible device execution without flexible truth
Low/mid/high resource tiers alter memory leases, tile size, concurrency, cache policy and optional backend only. The room admissibility/claim decisions do not depend on device tier. Severe/critical thermal pressure reduces concurrency and disables optional Vulkan preference without changing scientific state.

## PASS — dependency safety
Cycles, backward truth-floor dependencies, Scientific-domain read/write access to Counterfactual/Appearance, scientific-master mutation capability and evidence-increase capability are rejected by graph validation.

## PASS — deterministic semantic state
Identical inputs produce identical meaningful runtime fields, room decisions, ledger events and execution plans. Raw `memcmp` of C++ structs is intentionally not a contract because padding bytes are not semantic state.

## OPEN — end-to-end Android integration
The native core does not yet bind to Android lifecycle callbacks, JNI, actual `ActivityManager` values, Vulkan command buffers or the complete production render graph.

## OPEN — performance generalization
The fixed-size scheduler itself is tiny, but real total app performance still depends on each room honoring its lease, tile contract and cache-eviction policy. Device-class/thermal benchmarking remains required.

## OPEN — scientific promotion
This runtime does not promote Restorer, Surveyor, CICM, Room Capsule or any other research algorithm. Every room's own evidence and promotion gates remain authoritative.
