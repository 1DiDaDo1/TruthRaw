# TruthRaw Building Runtime v0.1

Research-only orchestration layer for the TruthRaw “two-house” architecture.

The runtime is the corridor/dependency system, not a new source of image evidence. It may decide which research room is admissible and in what order, while preserving immutable capture provenance and scientific claims.

## Hard invariants

- One physical capture remains one physical capture.
- Virtual/counterfactual states and activated rooms do not increase independent evidence.
- `scientificMasterModified=false` is mandatory for this runtime layer.
- `PhysicalCaptureEV`, `BestConditioningEV`, and `AppearanceEV` are separate domains.
- Appearance state never feeds back into scientific evidence/confidence decisions.
- Required unknown scientific state fails closed; unknown sigma is never silently replaced by zero.
- Research/candidate/rejected/open status is preserved; this runtime never performs scientific promotion.
- Dependency graph must be acyclic.
- Fixed-size structures keep the hot orchestration path bounded and mobile-friendly.
- The input evidence fingerprint is immutable identity; runtime decisions are emitted to a separate bounded ledger.

## Default rooms

Manifold Conditioning, Counterfactual Illumination (CICM), Room Capsule, Uncertainty, Appearance, Output Projection.

Room availability is contract-driven, not semantic-scene-label-driven.
