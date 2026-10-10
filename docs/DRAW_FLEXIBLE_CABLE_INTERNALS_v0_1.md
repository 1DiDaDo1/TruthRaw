# D.RAW Flexible Cable Internals v0.1

**Status:** architecture invariant — no scientific promotion  
**Date:** 2026-10-06  
**Continuation code:** `44489`

## Rule

**Every D.RAW cable must be flexible on the inside while keeping its outer evidence/provenance contract explicit and stable.**

A cable is therefore not a hard-wired chain of one implementation. It is an ordered, inspectable set of replaceable or extendable stages/adapters connected through declared capabilities and contracts.

This flexibility never relaxes the evidence law.

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

## What stays stable on the outside

Each cable boundary must retain enough information to prevent hidden authority drift:

- observation/source binding;
- provenance contract;
- authority contract;
- payload/type contract;
- uncertainty/censoring state where applicable;
- explicit writeback permissions;
- explicit failure/UNKNOWN behavior.

A downstream consumer may not infer stronger evidence merely because a different internal stage implementation is selected.

## What must remain flexible on the inside

Internal implementation may evolve through:

- additional deterministic adapters;
- alternative algorithms behind the same admitted contract;
- capability-based routing;
- optional diagnostics/telemetry stages;
- faster implementations that preserve the same scientific output contract;
- new presentation/output stages downstream of the Scientific Master;
- future source families that satisfy the same ingress contract.

The architecture must not require a rewrite of the whole cable merely to add one new supported implementation.

## No closed-world route assumptions

Internal stage IDs, role IDs and capability IDs should be open identifiers rather than an exhaustive closed list whenever the set is expected to grow. New internal stages must be addable without redefining scientific truth or changing unrelated callers.

Closed types remain acceptable where they represent a genuinely closed safety result or protocol state; they must not be used to freeze the conceptual architecture into one implementation path.

## Authority and flexibility are separate concerns

Flexibility does not mean arbitrary scientific mutation.

Every stage must still declare or inherit the authority/provenance contract under which it operates. A new stage that changes scientific state requires its own admitted scientific basis and tests. A presentation-only adapter remains presentation-only regardless of how modular or extensible it is.

The following remain prohibited unless separately admitted by the correct scientific contract:

- fabricating new `MEASURED` evidence;
- silently promoting `UNKNOWN`;
- modifying sealed source evidence;
- hidden Scientific Master writeback;
- converting appearance output into scientific authority;
- using AI/ML as a scientific inference layer.

## Current implementation anchor

The Android Workspace / Free Raster line now contains `DrawFlexibleCableContractV01`, an open descriptor for cable stages and capabilities. It deliberately uses open string identifiers plus extension maps so future stages can be added without an exhaustive enum rewrite.

The external presentation-raster path now also follows the rule directly: `PresentationRasterLoader` keeps a stable `load(...)` boundary but its inside is an ordered decoder-adapter list. The initial adapter is `android.bitmap_factory.v1`; future deterministic presentation decoders can be inserted behind the same outer `PRESENTATION_ONLY` contract without turning the loader into a second scientific pipeline.

## Design test for every future cable change

Before accepting a cable change, ask all of the following:

1. Can an internal implementation be replaced without changing the scientific meaning of the outer boundary?
2. Can a new compatible stage/capability be added without rewriting unrelated callers?
3. Are provenance and authority still explicit across the boundary?
4. Does unsupported input fail closed or remain `UNKNOWN` rather than being guessed?
5. Can diagnostics identify which internal implementation actually ran?
6. Does performance optimization preserve the exact admitted output/evidence contract?
7. Is any scientific writeback explicit, separately authorized and testable?

If the answer to 1–4 is no, the cable is too rigid or unsafe and should not be promoted.

## Project principle

**Stable outside. Flexible inside. Evidence law unchanged.**

**One Free World. Many sealed observations. One evidence law.**
