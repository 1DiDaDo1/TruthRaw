# TruthRaw HDR real Scientific Master binding v1.8

v1.8 closes the first of the two hard scientific gates left open by the v1.7 legacy LinearRaw bridge: the exact immutable tele source now has a deterministically recomputed **float Scientific Master identity**.

The source is `IMG_BNC_TRUTHRAW20260907_094449_565.dng`, SHA-256 `7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67`. Its already-bound decoded CFA identity is `883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c`.

## Real recomputation

The archived frozen native Scientific Master streaming implementation from repository commit `4f842d8fe86eca2b5808afb10fbb9f11a2631fed` was compiled against the exact source and executed twice independently. Both runs reported exactly:

- Scientific Master SHA-256: `a86034da7b6f9663640ee3b4478ccf4294d23b720f2b087d083dca38f5fb4640`
- self-gauge `L0`: `0.12564234435558319`
- gauge: `SELF_GAUGE_STAGE2_Q0.500000`
- backend: `research_edge_aware_support_limited_measured_preserving_v47i`
- scene scale: `TRUTHRANGE_SELF_GAUGE_STAGE2_V0_2`
- eligible gauge samples: `8002727`
- master tiles: `3072`
- Stage-2 gauge passes: `2`
- logical workspace peak: `138932` bytes
- logical resident upper bound: `1295156` bytes
- raw payload bytes read: `59808528`
- native tile reads: `6144`

The implementation files used for the recomputation are individually SHA-bound in `REAL_SCIENTIFIC_MASTER_OBSERVATION.json` and in `tools/truthraw_hdr_scientific_master_binding_v18.py`.

## What this proves

For this exact single physical source, the Scientific Master identity is no longer an unknown/null placeholder. The bound identity is:

`a86034da7b6f9663640ee3b4478ccf4294d23b720f2b087d083dca38f5fb4640`

This identity is deliberately distinct from the source-DNG SHA, decoded-CFA SHA, historical v0.4 LinearRaw file SHA, historical LinearRaw pixel-payload SHA, and its dequantized float32 compatibility SHA. The old 16-bit LinearRaw therefore remains a compatibility/integration derivative and cannot be promoted by hash substitution.

The v1.8 observation itself has deterministic canonical SHA-256 `2a3a2e2bf34ff910c453e00e11b0ccb0d351b126bf2903c0cb87103b3fe901ad`. The current real binding manifest with Dynamic Authority still unset has SHA-256 `8ca1c9e8d44f6e88c551fdbfab1e715d6af82f5b300a1d896ff676638c414d2a`.

## What remains open

v1.8 does **not** fabricate the second missing identity. `dynamic_authority_sha256` remains `null`, so the v1.6 full real HDR science binding is still fail-closed. Also, v1.8 binds a deterministically recomputable master identity; it does not claim that a new full-frame Scientific Master payload file was persisted by this step.

The next gate is to build/persist the matching per-channel Dynamic Authority Field for this same master, hash-bind it, and only then instantiate the complete v1.6 `source → CFA → Scientific Master → Dynamic Authority → P3-D65/PQ` scientific projection chain.

No Adobe HDR limit, AVIF MaxCLL, tone curve, PQ code, or presentation clipping value is permitted to change the Scientific Master identity or create new sensor evidence.
