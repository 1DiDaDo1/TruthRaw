# Dynamic Authority v1.9 regeneration — v0.1 recovery and falsification foundation

Status: **RESEARCH FOUNDATION / REQUIRED HISTORICAL SOURCE RECOVERY PASS / FIELD REGENERATION STILL OPEN**

Date: 2026-09-16

This note records the boundary between the historical Dynamic Authority v1.9 observation and the new independent streaming-regeneration attempt.

It exists to prevent two opposite errors:

1. claiming that v1.9 is lost because the 37,601,280-record payload was not stored as a repository artifact;
2. claiming a new implementation is equivalent merely because it reproduces aggregate counts or plausible-looking authority state.

## 1. Frozen v1.9 identity remains the falsification target

`state/TRUTHRAW_HDR_DYNAMIC_AUTHORITY_BINDING_V19_STATE.json` records that:

- `payload_persisted_by_v19 = false`;
- `identity_deterministically_recomputable = true`;
- source DNG, decoded CFA and Scientific Master identities were frozen;
- full-field and per-channel Dynamic Authority digests were frozen;
- two recomputations with execution-band sizes `192` and `257` produced identical output.

Frozen field identity:

`7678a0b145f8721347cdcc5177fedb720ba19b91bf34984a3f18bd8216408098`

Per-channel identities:

- R: `d0b8febb3e62d18968e72036a76f577453554553e117692a1fdd9d2c9b23e86f`
- G: `38b742784513e7f70fc31a6f39c6227617fe8719705f544c48524ffbfacc8a1f`
- B: `ab2ab62c7d6d2d29fc915374773ccdb805e1ee4870a38d7e523ae9074efe932f`

Frozen counts:

- `CALIBRATED_ESTIMATE = 12,533,543`
- `RECONSTRUCTED = 25,067,086`
- `CENSORED = 217`
- `UNKNOWN = 434`

Total records:

`4080 * 3072 * 3 = 37,601,280`.

## 2. Historical source recovery is now SHA-256 proven

Earlier on 15/16 September the exact v5.0g extractor and several v1.9 support sources were correctly treated as unavailable and the project failed closed.

A new full-history recovery workflow now closes the **source-recovery** part of that blocker:

- tool: `tools/recover_v19_historical_lineage_v01.py`
- tests: `tests/test_recover_v19_historical_lineage_v01.py`
- CI: `.github/workflows/v19-historical-lineage-recovery-v0-1.yml`
- successful run: `35107730144`
- result: `PASS_REQUIRED_HISTORICAL_SOURCE_BYTES_RECOVERED_EXACT`

The recovery method requires SHA-256 of the actual historical bytes. Matching names, file lengths or Git blob SHA-1 values are never accepted as a substitute.

### Exact v4.7i reconstruction source

Recovered directly from detached historical commit:

`36cd2ca16946a412fa28f04b70ae2f423167fb43`

`staging/v47i-byte-exact/core.cpp`

- bytes: `26344`
- Git blob SHA-1: `f79b951ba54cff08db400023e528e4eb91909ba6`
- SHA-256: `68f52c4896d47c4605adc1cf670e55f95d42a687e18c06a167028a64ce37b94c`
- frozen v1.9 match: **EXACT**

`staging/v47i-byte-exact/core.h`

- bytes: `8593`
- Git blob SHA-1: `cfb9fd42bc310ddb4fd16ee8f26c3ed554a0f92a`
- SHA-256: `b7f6e2189d6ecccfc5fda80084041990bd12757145c5ff2c40f2ae0d0046b167`
- frozen v1.9 match: **EXACT**

The historical staging directory remains staging provenance; recovering its exact bytes does not promote it to present-day canonical code.

### Exact Scientific-Master digest source

`scientific_master_digest_v0_1.cpp`

- bytes: `14796`
- SHA-256: `70dfd24b86f9472a98cded66ecc1130da6dd7a838322fadd55ff5152d22bbbdf`
- frozen v1.9 match: **EXACT**

`scientific_master_digest_v0_1.h`

- bytes: `2656`
- SHA-256: `89aaac2329375f7ebae1b8682868480844c75f45541bc36d26fb5b9ed618a058`
- frozen v1.9 match: **EXACT**

### Exact v5.0g feature extractor

`canonical/uncertainty/v5.0g/source/uncertainty_core_v5_0g.py`

- bytes: `10023`
- Git blob SHA-1: `3a833147f892a970c60175a7ebe4ab1e8cf0221c`
- SHA-256: `b1cfbf061a32aa4abccb9d8bb86a9b5b9257f88498081eb9aa659f9402d0919d`
- historical expected match: **EXACT**

This closes the old blocker:

`OPEN_NEEDS_EXACT_V5G_FEATURE_EXTRACTOR_RECOVERY_OR_HASH_VERIFIED_EQUIVALENT_FEATURE_DEFINITION`

The replacement blocker is:

`OPEN_NEEDS_EXACT_V5G_FEATURE_REPLAY_PARITY_AND_CURRENT_F64_TRACE_BINDING`

## 3. Exact 18-feature semantics are no longer guessed from names

The recovered source is now authoritative for the historical feature vector:

1. `log1p_abs_prediction_stage2`
2. `log1p_sigma_stage2_x1e4`
3. `log1p_predicted_snr`
4. `log1p_support_std_over_sigma`
5. `log1p_support_range_over_sigma`
6. `log1p_pair_min_disagreement_over_sigma`
7. `log1p_pair_median_disagreement_over_sigma`
8. `log1p_green_direction_disagreement_over_sigma`
9. `log1p_local_mosaic_range_over_sigma`
10. `gain_at_target`
11. `neighbor_censor_fraction`
12. `radial_position_norm`
13. `prediction_negative_flag`
14. `prediction_over1_flag`
15. `role_R`
16. `role_G1`
17. `role_G2`
18. `role_B`

Historical compatibility quirk that must remain exact:

- internal/training role order: `B, G1, G2, R`
- emitted role feature labels: `role_R, role_G1, role_G2, role_B`

A new implementation must not “correct” this ordering.

## 4. Anti-leak semantics are now source-proven

The recovered extractor confirms that the central hidden measured target sample is not used to construct the prediction/features. The target is read only after the feature vector/prediction state exists for scoring, and source-white hidden targets are excluded from exact-value scoring.

Therefore a future 18-feature adapter is equivalent only if it preserves the same dependency/order semantics, not merely the feature names.

## 5. Local v1.9 audit probe remains a provenance gap

The historical local `probe_dynamic_authority_v19.cpp` has not yet been recovered.

Frozen expected SHA-256:

`d82fc539643650fc68e4d307c3080cf0f4f1ada8dc7311ed7d8fb5005e7ce8c2`

This is now classified as:

`PROVENANCE_GAP_NOT_FIELD_IDENTITY_FAILURE`

because all required support source bytes have been recovered exactly and the historical v1.9 state independently froze deterministic recomputability and partition invariance.

The missing probe still means we may not claim byte-for-byte recovery of the original orchestration implementation.

## 6. Aggregate counts remain insufficient

The four class totals do not encode where authority classes occur, nor per-record value, p95 uncertainty, censor bounds, channel ordering or the fixed binary digest layout.

Therefore:

> **Aggregate counts may validate a regenerated field, but may never be used to reconstruct it.**

The full-field and R/G/B SHA-256 identities remain mandatory falsification targets.

## 7. Frozen per-site semantics to preserve

In global raster order:

- uncensored physical CFA channel -> `CALIBRATED_ESTIMATE`, Stage-2 value + DNG NoiseProfile Gaussian-equivalent p95 uncertainty;
- uncensored missing channels -> `RECONSTRUCTED`, Scientific-Master RGB value + finite v5.0g local-max-transport p95 uncertainty;
- source-white-censored physical CFA channel -> `CENSORED` with a `>=` lower bound;
- two missing channels at a censored CFA site -> `UNKNOWN`;
- counterfactual/appearance-only state is excluded;
- no fixed scene-EV ceiling is imposed.

Reconstruction authority remains separate from optical/detail/acutance authority.

## 8. Historical negative readiness evidence is preserved

The earlier `dynamic_authority_v19_regeneration_readiness_v01/v02` checks were correct for the repository state they inspected: required historical source files were not then available in the live worktree.

They remain useful negative provenance and must not be rewritten into a retroactive PASS.

The new history-recovery gate adds new evidence: required source **bytes** are recoverable from Git history and match the previously frozen hashes exactly.

## 9. Next exact replay/falsification sequence

The correct next sequence is now:

1. replay the recovered exact v5.0g extractor against frozen historical parity/prospective evidence;
2. bind those exact feature semantics to the present F64 reconstruction trace without leakage;
3. construct an independent streaming Dynamic Authority v1.9 regenerator from the recovered frozen scientific dependencies;
4. regenerate the 4080x3072x3 field without using aggregate counts as construction input;
5. require exact full-field + R/G/B SHA-256 matches;
6. re-run under at least two execution partitions and require identical identity;
7. only then admit the real frozen 4080x3072 field into Open Scene Region v0.7 / full-frame v0.8.

A regeneration PASS must also reproduce the frozen class counts, channel counts, signed-nonpositive count, extrema, Scientific-Master identity, v5.0g anchor count and source-clip-skipped count.

## 10. Scientific meaning of the recovery PASS

The recovery PASS means:

> **The source-code lineage required to attempt exact historical replay has been recovered and cryptographically matched.**

It does **not** mean:

- the v1.9 field has already been independently regenerated today;
- the new implementation is equivalent before digest falsification;
- the local historical audit probe has been recovered;
- uncertainty has been rebound to every current F64 trace site;
- any new sensor evidence exists;
- scientific authority has been upgraded;
- research state has been promoted to main/canonical.

The permanent rule remains:

> **Representation may exceed the source; knowledge claims may not exceed the evidence.**

Detailed recovery chronology is preserved in `docs/history/TRUTHRAW_V19_V5G_HISTORICAL_RECOVERY_2026-09-16.md`.
