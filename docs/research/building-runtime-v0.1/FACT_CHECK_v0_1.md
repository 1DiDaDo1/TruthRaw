# Building Runtime v0.1 — Fact Check

## PASS: evidence neutrality
A scheduler can route computations without increasing Fisher information or independent samples. Therefore the runtime pins `physicalFrameCount=1` and `independentEvidenceCount=1` and rejects contradictory input state.

## PASS: EV domain separation
Physical capture exposure, exact numerical conditioning gauge, and appearance exposure serve different semantics. The runtime stores them separately and never derives AppearanceEV from BestConditioningEV.

## PASS: no appearance-to-science feedback
Appearance may consume scientific/uncertainty output, but scientific rooms are statically forbidden from reading or writing appearance state in the runtime graph.

## PASS: fail-closed unknowns
Rooms requiring sigma are blocked when sigma is unknown. Non-finite critical capture/conditioning state is rejected. No unknown is interpreted as zero uncertainty.

## PASS: bounded mobile architecture
The core uses fixed arrays only; no heap allocation, strings, vectors, filesystem, GPU requirement, or dynamic module discovery is present in the hot scheduler.

## PASS: dependency safety
Graph cycles and forbidden scientific-domain capabilities are rejected before evaluation.

## OPEN: end-to-end Android integration
This research module does not yet bind to an Android application lifecycle, JNI layer, GPU scheduler, DNG writer, or actual full TruthRaw render graph.

## OPEN: scientific promotion
This module cannot promote research algorithms. Any room’s own frozen-parameter/heldout/topology gates remain authoritative.
