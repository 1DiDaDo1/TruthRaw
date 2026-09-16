# TruthRaw v1.9 / v5.0g historical source recovery — 2026-09-16

Status: **CURRENT HISTORICAL RECOVERY EVIDENCE / NO SCIENTIFIC AUTHORITY UPGRADE BY ITSELF**

This addendum records a correction to the earlier 2026-09-15/16 project state. At that earlier point the exact historical `uncertainty_core_v5_0g.py`, v4.7i reconstruction source bytes used in the v1.9 lineage, and Scientific-Master digest sources were treated as missing or not repository-recoverable. That was a valid fail-closed state at the time.

On 2026-09-16 a full Git-history recovery pass, including the detached historical v4.7i staging commit, recovered the required source bytes and verified them against the **already frozen SHA-256 identities**. The recovery did not infer code from names and did not accept matching file size or Git SHA-1 as a substitute.

CI evidence:

- workflow: `.github/workflows/v19-historical-lineage-recovery-v0-1.yml`
- successful run: `35107730144`
- classification: `PASS_REQUIRED_HISTORICAL_SOURCE_BYTES_RECOVERED_EXACT`
- unit tests: 4/4 PASS
- required historical sources: all exact SHA-256 matches
- optional local `probe_dynamic_authority_v19.cpp`: not recovered

## Exact recovered sources

### v4.7i reconstruction source

Historical detached commit:

`36cd2ca16946a412fa28f04b70ae2f423167fb43`

The staging directory had later been deliberately removed from the live canonical tree. Direct `commit:path` byte recovery proves:

`staging/v47i-byte-exact/core.cpp`

- bytes: `26344`
- Git blob SHA-1: `f79b951ba54cff08db400023e528e4eb91909ba6`
- SHA-256: `68f52c4896d47c4605adc1cf670e55f95d42a687e18c06a167028a64ce37b94c`
- frozen v1.9 SHA-256 match: **EXACT**

`staging/v47i-byte-exact/core.h`

- bytes: `8593`
- Git blob SHA-1: `cfb9fd42bc310ddb4fd16ee8f26c3ed554a0f92a`
- SHA-256: `b7f6e2189d6ecccfc5fda80084041990bd12757145c5ff2c40f2ae0d0046b167`
- frozen v1.9 SHA-256 match: **EXACT**

The historical staging label remains provenance. Exact byte recovery does **not** promote that old staging tree back into the current canonical tree; it only closes the source-lineage recovery question for the frozen v1.9 experiment.

### Scientific-Master digest source

`docs/research/scientific-master-digest-v0.1/native/scientific_master_digest_v0_1.cpp`

- bytes: `14796`
- Git blob SHA-1: `669122bcae82dd5475b2a1dfa413bc3e8dff6c22`
- SHA-256: `70dfd24b86f9472a98cded66ecc1130da6dd7a838322fadd55ff5152d22bbbdf`
- frozen v1.9 SHA-256 match: **EXACT**

`docs/research/scientific-master-digest-v0.1/native/scientific_master_digest_v0_1.h`

- bytes: `2656`
- Git blob SHA-1: `6bb779d43270e99bb876727ce1ab7cb7b04ba200`
- SHA-256: `89aaac2329375f7ebae1b8682868480844c75f45541bc36d26fb5b9ed618a058`
- frozen v1.9 SHA-256 match: **EXACT**

### Exact v5.0g feature extractor

`canonical/uncertainty/v5.0g/source/uncertainty_core_v5_0g.py`

- bytes: `10023`
- Git blob SHA-1: `3a833147f892a970c60175a7ebe4ab1e8cf0221c`
- SHA-256: `b1cfbf061a32aa4abccb9d8bb86a9b5b9257f88498081eb9aa659f9402d0919d`
- frozen expected SHA-256 match: **EXACT**

This closes the previous blocker:

`OPEN_NEEDS_EXACT_V5G_FEATURE_EXTRACTOR_RECOVERY_OR_HASH_VERIFIED_EQUIVALENT_FEATURE_DEFINITION`

It does **not** close uncertainty binding end to end. The next blocker is:

`OPEN_NEEDS_EXACT_V5G_FEATURE_REPLAY_PARITY_AND_CURRENT_F64_TRACE_BINDING`

## Exact historical 18-feature semantics recovered from source

The exact extractor, rather than an earlier descriptive summary, is now the authority for historical feature semantics. Its feature labels are:

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

Historical compatibility quirk that must remain unchanged:

- role/training order: `B, G1, G2, R`
- emitted role feature labels: `role_R, role_G1, role_G2, role_B`

This is frozen historical behavior. It must not be “cleaned up” merely for aesthetic consistency.

## Anti-leak semantics now source-proven

The recovered source also resolves an important scientific ambiguity. The hidden-target protocol constructs prediction/features without using the central hidden measured target sample. The target value is read only after the feature vector/prediction state has been constructed for scoring, and a source-white hidden target is excluded from exact-value scoring.

Therefore future replay must preserve the same anti-leak order. A newly implemented feature adapter is not equivalent merely because it returns 18 numbers with matching names.

## Dynamic Authority v1.9 consequence

The following required v1.9 lineage source dependencies are now recovered with exact frozen hashes:

- v4.7i core `.cpp/.h`;
- Scientific-Master digest `.cpp/.h`;
- exact historical v5.0g feature extractor.

The historical local `probe_dynamic_authority_v19.cpp` remains unrecovered. Its expected frozen SHA-256 is:

`d82fc539643650fc68e4d307c3080cf0f4f1ada8dc7311ed7d8fb5005e7ce8c2`

That remains a provenance/source-archive gap, not a demonstrated failure of the frozen Dynamic Authority identity. The original v1.9 state explicitly recorded that the full authority payload was not persisted while the identity was deterministically recomputable and partition-invariant.

Current independent regeneration must still falsify itself against the frozen identities:

- full field: `7678a0b145f8721347cdcc5177fedb720ba19b91bf34984a3f18bd8216408098`
- R: `d0b8febb3e62d18968e72036a76f577453554553e117692a1fdd9d2c9b23e86f`
- G: `38b742784513e7f70fc31a6f39c6227617fe8719705f544c48524ffbfacc8a1f`
- B: `ab2ab62c7d6d2d29fc915374773ccdb805e1ee4870a38d7e523ae9074efe932f`

Only an exact match may close the independent current v1.9 regeneration gate.

## Engineering direction opened by this recovery

The correct next sequence is now:

`exact recovered v5.0g extractor`
`-> exact feature replay/parity against frozen historical evidence`
`-> exact current F64 reconstruction-trace adapter`
`-> independent v1.9 authority streaming regeneration`
`-> full-field + R/G/B digest falsification`
`-> real 4080x3072 Open Scene v0.7/v0.8 binding`
`-> Android reference-port validation`

This recovery narrows uncertainty. It does not increase captured evidence, does not change the frozen source, and does not promote research state to canonical/main.
