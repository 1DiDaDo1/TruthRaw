# Dynamic Authority v1.9 regeneration — v0.1 readiness foundation

Status: **RESEARCH FOUNDATION / REGENERATION READINESS ONLY / NOT A NEW AUTHORITY FIELD**

Date: 2026-09-16

This note records the exact boundary between the historical Dynamic Authority v1.9 recomputation evidence and the next independent streaming-regeneration attempt.

It exists to prevent two opposite errors:

1. falsely claiming that v1.9 is lost merely because the 37,601,280-record payload was not stored as a repository artifact;
2. falsely claiming that a new implementation is equivalent merely because it reproduces aggregate authority counts.

## 1. Historical-chat recovered knowledge

The previous TruthRaw development conversation had already distinguished the v1.9 **identity** from persistent storage of the full authority payload. It knew that the earlier implementation had computed/frozen the field identity and that a persistent/streaming artifact path was still a subsequent engineering gate.

That historical knowledge is useful context, but the current regeneration decision is not based on memory alone. The repository state below independently records the same boundary.

## 2. Repository-verified frozen v1.9 state

`state/TRUTHRAW_HDR_DYNAMIC_AUTHORITY_BINDING_V19_STATE.json` records:

- `payload_persisted_by_v19 = false`;
- `identity_deterministically_recomputable = true`;
- exact source DNG and decoded-CFA identities;
- exact Scientific Master identity;
- exact overall and per-channel Dynamic Authority digests;
- exact authority counts;
- two successful recomputations with distinct execution-band sizes (`192` and `257`) and `PASS_IDENTICAL` partition invariance.

Therefore:

> **Absence of the historical full payload is not evidence that the scientific identity was unreproducible.**

The frozen field identity remains:

`7678a0b145f8721347cdcc5177fedb720ba19b91bf34984a3f18bd8216408098`

Per-channel identities:

- R: `d0b8febb3e62d18968e72036a76f577453554553e117692a1fdd9d2c9b23e86f`
- G: `38b742784513e7f70fc31a6f39c6227617fe8719705f544c48524ffbfacc8a1f`
- B: `ab2ab62c7d6d2d29fc915374773ccdb805e1ee4870a38d7e523ae9074efe932f`

Frozen authority counts:

- `CALIBRATED_ESTIMATE = 12,533,543`
- `RECONSTRUCTED = 25,067,086`
- `CENSORED = 217`
- `UNKNOWN = 434`

Total RGB authority records:

`4080 * 3072 * 3 = 37,601,280`.

## 3. Exact historical implementation lineage

`tools/truthraw_hdr_dynamic_authority_binding_v19.py` freezes 13 implementation SHA-256 entries used for the empirical v1.9 observation:

- one local audit/orchestration source: `probe_dynamic_authority_v19.cpp`;
- twelve native scientific dependencies covering latent/TruthRange, Stage-2, canonical reconstruction, uncertainty runtime and Scientific-Master digest support.

The historical local probe has a frozen expected SHA-256:

`d82fc539643650fc68e4d307c3080cf0f4f1ada8dc7311ed7d8fb5005e7ce8c2`.

The readiness gate deliberately distinguishes three situations:

1. exact probe source present -> full frozen implementation lineage is available;
2. exact twelve repository dependencies present but probe source absent -> documented local-probe provenance gap, while the historical v1.9 identity/evidence remains valid;
3. missing/hash-drifted required repository dependency, ambiguous duplicate, or frozen-state drift -> readiness is blocked.

The second case does **not** permit pretending that a new regenerator is already equivalent. It only permits implementing an independent regenerator and attempting exact falsification against the frozen v1.9 output.

## 4. Why aggregate counts are insufficient

The four counts do not encode where authority classes occur in the 4080x3072x3 field, nor do they encode per-record value, p95 uncertainty, censor bound, channel ordering or binary digest layout.

Many different fields can have identical class totals.

Therefore:

> **Aggregate counts may be used as output checks, but they may never be used to reconstruct the missing field.**

The field and per-channel SHA-256 identities are mandatory falsification targets.

## 5. Frozen per-site semantics that the independent regenerator must preserve

The v1.9 binding defines, in global raster order:

- uncensored physical CFA channel -> `CALIBRATED_ESTIMATE`, retaining the Stage-2 value and DNG NoiseProfile Gaussian-equivalent p95 uncertainty;
- uncensored missing colour channels -> `RECONSTRUCTED`, using the Scientific-Master RGB value and finite v5.0g local-max-transport p95 uncertainty;
- source-white-censored physical CFA channel -> `CENSORED` with a `>=` lower bound;
- the two missing channels at every censored CFA site -> `UNKNOWN`;
- no counterfactual or appearance-only state enters the scientific field;
- no fixed scene EV ceiling is imposed.

Reconstruction authority remains separate from optical/detail/acutance authority.

## 6. New machine-readable readiness gate

Implementation:

- `tools/dynamic_authority_v19_regeneration_readiness_v01.py`
- `tests/test_dynamic_authority_v19_regeneration_readiness_v01.py`
- `.github/workflows/dynamic-authority-v19-regeneration-readiness-v0-1.yml`

The gate verifies:

- frozen source/CFA/master/field/channel identities;
- frozen authority counts and geometry;
- historical payload-not-persisted statement;
- historical deterministic-recomputability statement;
- historical 192/257 partition-invariance evidence;
- exact SHA-256 of every repository-resident frozen implementation dependency;
- the local-probe source as either exact-present or explicitly absent.

It fails closed on any required dependency hash drift or frozen-state identity drift.

## 7. What a PASS means

A readiness PASS means only:

> **The frozen v1.9 scientific identity and exact repository-resident implementation lineage remain coherent enough to justify an independent regeneration attempt.**

It does not mean:

- a new full authority field has been regenerated;
- the new implementation is scientifically equivalent;
- the missing historical local probe has been recreated byte-for-byte;
- a new source observation exists;
- any scientific authority has been upgraded.

## 8. Next falsification gate

The next implementation must stream-regenerate the field from the frozen source/scientific pipeline without deriving per-site state from the aggregate summary.

It must then reproduce all of the following exactly:

1. overall field SHA-256;
2. R/G/B channel SHA-256 values;
3. all global authority counts;
4. all channel authority counts;
5. signed-nonpositive estimate count;
6. value/p95/censor-bound extrema expected by the frozen run;
7. Scientific Master verification identity;
8. v5.0g anchor count and source-clip-skipped count;
9. identical output under at least two distinct execution partitions.

Until that exact falsification passes, the independent regenerator is **not equivalent to historical v1.9**.

## 9. Scientific consequence for Open Scene v0.7/v0.8

The new Open Scene Region/Full-Frame runtime must not fabricate a per-site v1.9 field from summary counts.

The real frozen 4080x3072 source may enter that path only after either:

- the exact historical field payload is recovered; or
- an independent regenerator reproduces the frozen v1.9 identities exactly.

This preserves the permanent TruthRaw rule:

> **Representation may exceed the source; knowledge claims may not exceed the evidence.**
