#!/usr/bin/env python3
from pathlib import Path

p = Path('docs/handoff/DRAW_44489_KNOWLEDGE_CAPSULE.md')
s = p.read_text()

old = '- Current architecture PR: **#132 — Architecture: separate runtime Source Capability evidence-envelope schema v0.1**; it remains a narrow schema/governance review unit until its final exact-head CI is rechecked.'
new = '- Current architecture PR: **#132 — Architecture: separate runtime Source Capability evidence-envelope schema v0.1**; final clean schema code/test checkpoint `7e176006c8e4e7be379ce8a613a695ef750900b1` has all triggered non-skipped checks SUCCESS. PR #132 remains deliberately open/draft/unmerged as the narrow review boundary before any new Source Capability consumer.'
if old not in s:
    raise SystemExit('PR132 status anchor missing')
s = s.replace(old, new, 1)

old = '- A previous one-time 44489 refresh workflow failed fail-closed on a stale text anchor (`key anchor missing`) and made no project-code change. That temporary refresh workflow/script are governance-repair tooling only and are removed from the final clean PR #132 diff after this capsule refresh.'
new = '- A previous one-time 44489 refresh workflow failed fail-closed on a stale text anchor (`key anchor missing`) and made no project-code change. The replacement deterministic refresh succeeded; all temporary refresh workflow/script files were then removed from the clean PR #132 diff.'
if old not in s:
    raise SystemExit('refresh cleanup anchor missing')
s = s.replace(old, new, 1)

start = s.find('## 25. Exact next execution order')
end = s.find('\n---\n\n## 26.', start)
if start < 0 or end <= start:
    raise SystemExit('section 25 boundary missing')
section = '''## 25. Exact current execution state and next order

Completed in the 2026-10-11 three-chat governance repair:

1. historical Exact Gauge checkpoint `4e4f358a...` is preserved as the frozen narrow scientific/review reference;
2. the live PR #130, which had advanced after PR #131 was merged into its head branch, is closed **superseded/unmerged** without force-resetting history;
3. PR #131 remains merged/closed at integration anchor `28343adf...` and `main` remains unadvanced by that merge;
4. runtime schema separation is complete at `e0527b374d6477a0b239125f7aa8d3f8750c5d32`;
5. canonical 44489 has been repaired to distinguish the historical Physical Knowledge Map, runtime evidence envelope and occurrence ledger;
6. the failed temporary 44489 refresh tooling was replaced by a deterministic refresh, succeeded, and all temporary refresh files were removed from the PR diff;
7. final clean schema code/test checkpoint `7e176006c8e4e7be379ce8a613a695ef750900b1` passed every triggered non-skipped check observed in the audit; Deep-Censor and Free Raster were path-scope SKIPPED, not failed;
8. PR #132 is now a clean four-file schema/governance boundary and remains deliberately draft/unmerged.

Next scientific order — not yet implemented by this governance repair:

1. expose the already-existing Effective Black / Normalization v0.1 report only as an additional **read-only runtime diagnostic**;
2. prove that exposure changes no ledger digest, records, lifecycle/admission, consumers, source reads, pixels, Scientific Master, Appearance or writeback;
3. implement a separate `LinearizationTable` shadow validator anchored to DNG 1.7.1.0;
4. keep complete raw-to-linear validity false until linearization + black subtraction + normalization contracts are jointly validated;
5. even after joint validation keep `VALIDATED`, `ADMITTED` and `USED` separate;
6. later treat colour-calibration families independently and source-bound;
7. keep pending physical/output/Appearance validation separate from the scientific metadata line.
'''
s = s[:start] + section + s[end:]

recovery_start = s.find('## 27. Recovery sentence')
if recovery_start < 0:
    raise SystemExit('recovery section missing')
prefix = s[:recovery_start]
recovery = '''## 27. Recovery sentence

When continuation code **44489** is given, recover this state:

**PR #131 is merged and closed. Its final green head is `c28b8bac...` and merge integration anchor is `28343adf...` on `fix/android-exact-gauge-pass-artifact-v03`; `main` was not advanced by that merge. Historical Exact Gauge checkpoint `4e4f358a...` remains the frozen narrow scientific/review reference, but the live PR #130 advanced after the PR #131 merge and is now closed superseded/unmerged rather than force-reset. The post-merge branch `feat/draw-observation-evidence-architecture-v01` and draft PR #132 carry the runtime schema/governance separation. Runtime schema migration is complete at `e0527b374d6477a0b239125f7aa8d3f8750c5d32`: canonical Physical Knowledge Map remains `D.RAW/SourceCapabilityEnvelope/0.2`, runtime evidence envelope is `D.RAW/SourceCapabilityRuntimeEvidenceEnvelope/0.1`, and occurrence ledger remains `D.RAW/ObservationEvidenceLedger/0.1`; no new scientific consumer or authority was created. The clean schema code/test checkpoint is `7e176006c8e4e7be379ce8a613a695ef750900b1`, where every triggered non-skipped check observed in the audit succeeded; Deep-Censor and Free Raster were skipped by path scope. The PR #132 diff is limited to the schema integrity workflow, canonical 44489 capsule, runtime schema change and focused test; temporary governance-refresh tooling has been removed. Parser-occurrence, raw-IFD, radiometric semantics, encoding consistency, DNG compatibility and spatial BlackLevel dependencies already exist. Effective Black / Normalization v0.1 at `bc93fe...` remains CI-green, metadata-only, standalone and non-promoting; runtime-envelope exposure is the next narrow scientific diagnostic and is not yet implemented. `LinearizationTable` remains unresolved, so complete raw-to-linear validity/admission/use remains false. Exact Gauge, reconstruction, T5/Room Capsule, High-Fidelity JPEG and Appearance histories retain their separate authority and physical-acceptance states. One Free World. Many sealed observations. One evidence law.**
'''
s = prefix + recovery

required = [
    '7e176006c8e4e7be379ce8a613a695ef750900b1',
    'closed superseded/unmerged',
    'D.RAW/SourceCapabilityRuntimeEvidenceEnvelope/0.1',
    'temporary governance-refresh tooling has been removed',
    'runtime-envelope exposure is the next narrow scientific diagnostic and is not yet implemented',
]
for needle in required:
    if needle not in s:
        raise SystemExit(f'missing final marker: {needle}')

p.write_text(s)
