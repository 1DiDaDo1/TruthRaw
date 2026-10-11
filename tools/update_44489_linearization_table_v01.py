from pathlib import Path

path = Path('docs/handoff/DRAW_44489_KNOWLEDGE_CAPSULE.md')
text = path.read_text(encoding='utf-8')

start = '<!-- 44489_LINEARIZATION_TABLE_V01_START -->'
end = '<!-- 44489_LINEARIZATION_TABLE_V01_END -->'
block = '''<!-- 44489_LINEARIZATION_TABLE_V01_START -->

## 0B. Binding LinearizationTable v0.1 standalone checkpoint — 2026-10-11

This block is authoritative over older LinearizationTable wording later in this capsule.

- PR #132 schema/governance separation is merged; merge commit: `4c27255ee1f5f95f4b623289d3785e5038c8cbe6`.
- PR #133 Effective Black / Normalization read-only runtime diagnostic is merged; integration anchor: `c81d4430f5c82d2de83cd480452c8bd01d60878b`.
- Current LinearizationTable branch: `feat/source-capability-linearization-table-shadow-v01`.
- Current PR: **#134 — Source Capability: LinearizationTable shadow validator v0.1**; open, draft, unmerged at this checkpoint.
- Standalone LinearizationTable validator: `D.RAW/SourceCapabilityRadiometricLinearizationTable/0.1`, status `SHADOW_ONLY_LINEARIZATION_TABLE_METADATA_CONTRACT`.
- Exact standalone code/CI checkpoint: `488a20065a9ef3f11695fa0167c70bc12bc7dac4`.
- Dedicated integrity run `38165699523`: **SUCCESS**.
- On exact `488a200...`, all triggered non-skipped checks completed without failure/action-required; Deep-Censor and Free Raster were path-scope skips.
- Physical tag `50712` remains represented by the existing parser occurrence as `TAG_50712`; parser semantic renaming is deliberately deferred because a rename would change the canonical Observation Evidence Ledger line/digest for unchanged source bytes.
- The validator binds numeric tag `50712` from the existing parsed `entries`; it creates no second parser or source-read path.
- Absent `LinearizationTable` uses the DNG identity default but mints no physical occurrence.
- Explicit-table validation enforces TIFF `SHORT`, count `2..65536`, rejects floating-point raw and `BitsPerSample > 16`, and deliberately does **not** require count `== 2^BitsPerSample`.
- Standalone regressions prove ledger digest and observation-evidence digest unchanged; the underlying tag-50712 ledger record remains `TAG_50712`, lifecycle `PARSED`, admission `UNSUPPORTED_SEMANTICS`, zero consumers.
- The validator is **not yet exposed through `SourceCapabilityRuntimeEvidenceEnvelope/0.1`**.
- `table_application_to_image_samples_validated=false`, `linearization_applied_to_image_samples=false`, `full_raw_to_linear_mapping_validated=false`, `image_sample_payload_read=false`, `scientific_admission_effect=false`, `pixel_route_effect=false`, `scientific_master_pixel_effect=false`, `appearance_pixel_effect=false`, `creates_new_evidence=false`, `scientific_writeback_allowed=false`.
- `VALIDATED != ADMITTED != USED` remains binding; CI success does not promote scientific authority.

Binding detailed handoff: `docs/handoff/DRAW_44489_LINEARIZATION_TABLE_AUDIT_V0_1_2026-10-11.md`.

Next boundary: first obtain exact-head green CI after this documentation refresh; then PR #134 may be merged only as a standalone validator/integrity checkpoint. Any runtime-envelope exposure belongs on a fresh branch/PR and must preserve ledger/observation digests, lifecycle/admission/consumers, source reads, pixels, Scientific Master, Appearance and writeback. Parser renaming remains a separate provenance/schema migration.

<!-- 44489_LINEARIZATION_TABLE_V01_END -->'''

if start in text or end in text:
    if start not in text or end not in text:
        raise SystemExit('partial LinearizationTable marker block; fail closed')
    a = text.index(start)
    b = text.index(end, a) + len(end)
    new_text = text[:a] + block + text[b:]
else:
    anchor = 'Permanent motto: **One Free World. Many sealed observations. One evidence law.**\n'
    if anchor not in text:
        raise SystemExit('capsule insertion anchor missing; fail closed')
    new_text = text.replace(anchor, anchor + '\n' + block + '\n', 1)

if new_text == text:
    print('capsule already current')
else:
    path.write_text(new_text, encoding='utf-8')
    print('capsule updated')
