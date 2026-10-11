from pathlib import Path

path = Path("docs/handoff/DRAW_44489_KNOWLEDGE_CAPSULE.md")
text = path.read_text(encoding="utf-8")
start = "<!-- 44489_LINEARIZATION_TABLE_V01_START -->"
end = "<!-- 44489_LINEARIZATION_TABLE_V01_END -->"

if text.count(start) != 1 or text.count(end) != 1:
    raise SystemExit("LinearizationTable 44489 markers are not unique; refusing to modify capsule")

start_i = text.index(start)
end_i = text.index(end) + len(end)
if start_i >= end_i:
    raise SystemExit("LinearizationTable 44489 marker order invalid; refusing to modify capsule")

replacement = r'''<!-- 44489_LINEARIZATION_TABLE_V01_START -->

## 0B. Binding LinearizationTable v0.1 runtime-diagnostic checkpoint — 2026-10-11

This block is authoritative over older LinearizationTable wording later in this capsule.

- PR #132 schema/governance separation is merged; merge commit: `4c27255ee1f5f95f4b623289d3785e5038c8cbe6`.
- PR #133 Effective Black / Normalization read-only runtime diagnostic is merged; integration anchor: `c81d4430f5c82d2de83cd480452c8bd01d60878b`.
- PR #134 `Source Capability: LinearizationTable shadow validator v0.1` is merged; final head `c4474904ec9bd00552f1378fd50896dac0d7be61`, merge commit `f9547cfe9b04c732d5543d2b42cd4513e83ab9ab`.
- Current branch: `feat/source-capability-linearization-table-runtime-diagnostic-v01`.
- Current PR: **#135 — Source Capability: expose LinearizationTable read-only runtime diagnostic v0.1**; still draft/unmerged at this checkpoint.
- Standalone validator remains `D.RAW/SourceCapabilityRadiometricLinearizationTable/0.1`, status `SHADOW_ONLY_LINEARIZATION_TABLE_METADATA_CONTRACT`.
- Runtime exposure adds exactly one read-only `radiometric_linearization_table_v0_1` report through `SourceCapabilityRuntimeEvidenceEnvelope/0.1` / `SourceCapabilityEnvelopeV02.describe(...)`.
- Exposure code commit: `090b7330f2ca93e682bf864cb307a4fb3b75aecc`.
- Permanent runtime-diagnostic integrity workflow commit: `5a201fc0293c27f68f1cbfdbe25c8685d474faab`.
- The older standalone integrity gate initially failed on that post-exposure head because it still forbade any envelope call. That was a stale pre-exposure governance assertion, not a validator/pixel regression.
- The stale assertion was replaced, without weakening the remaining firewall, at exact validated code/workflow head `d0e2ed9921662c1bce9324115705989e8dee99d4`: parser rename remains forbidden; exactly one report key and exactly one validator call are required.
- `Source Capability LinearizationTable v0.1 Integrity`, run `38167611304`: **SUCCESS** on `d0e2ed99...`.
- `Source Capability LinearizationTable Runtime Diagnostic v0.1 Integrity`, run `38167611297`: **SUCCESS** on `d0e2ed99...`.
- All other triggered non-skipped workflows on exact `d0e2ed99...` also completed **SUCCESS**; Free Raster and Deep-Censor were path-scope skips. No queued/in-progress/failure/action-required result remained at completion.
- Physical tag `50712` remains represented by the existing parser occurrence as `TAG_50712`; parser semantic renaming remains deliberately deferred because a rename would change canonical Observation Evidence Ledger identity for unchanged source bytes.
- The validator consumes numeric tag `50712` from the existing parsed `entries`; there is no second parser, source-read or source-hash path.
- Absent `LinearizationTable` uses the DNG identity default but mints no physical occurrence.
- Explicit-table validation enforces TIFF `SHORT`, count `2..65536`, rejects floating-point raw and `BitsPerSample > 16`, and deliberately does **not** require count `== 2^BitsPerSample`.
- The runtime-exposure regression locks pre-exposure identities: `ledger_digest_sha256=6740e6e04c18797e8b57604f30c1884c6e8428e9a469e53cb139c26abe59afb3` and `observation_evidence_digest_sha256=2bdaa44fe3435929310b79fa38d6ce3b0aa0cc05ef68f9522b4554c6862b38e6`.
- The tag-50712 record remains `TAG_50712`, lifecycle `PARSED`, admission `UNSUPPORTED_SEMANTICS`, authority `UNKNOWN_SEMANTICS`, zero consumers.
- `table_application_to_image_samples_validated=false`, `linearization_applied_to_image_samples=false`, `full_raw_to_linear_mapping_validated=false`, `image_sample_payload_read=false`, `record_lifecycle_effect=false`, `scientific_admission_effect=false`, `pixel_route_effect=false`, `scientific_master_pixel_effect=false`, `appearance_pixel_effect=false`, `source_values_modified=false`, `creates_new_evidence=false`, `scientific_writeback_allowed=false`.
- `VALIDATED != ADMITTED != USED` remains binding. Green CI and future merge status do not promote scientific authority.

Binding detailed handoffs:

- `docs/handoff/DRAW_44489_LINEARIZATION_TABLE_AUDIT_V0_1_2026-10-11.md`
- `docs/handoff/DRAW_44489_LINEARIZATION_TABLE_RUNTIME_DIAGNOSTIC_V0_1_2026-10-11.md`

Next boundary: validate the documentation-refresh head itself. Only if that exact head, PR scope, mergeability and review state are clean may PR #135 be merged as a **read-only diagnostic integration boundary**. After integration, the next scientific task is a separate read-only audit of `Linearization -> Black Subtraction -> Normalization -> Clipping`; no pixel application is authorized here. Parser semantic renaming remains a separate provenance/schema migration.

<!-- 44489_LINEARIZATION_TABLE_V01_END -->'''

updated = text[:start_i] + replacement + text[end_i:]
if updated == text:
    raise SystemExit("Capsule already matches requested LinearizationTable checkpoint; refusing empty write")

path.write_text(updated, encoding="utf-8")
print("Updated exactly one LinearizationTable 44489 marker block")
