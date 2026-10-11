#!/usr/bin/env python3
from pathlib import Path
import re

p = Path('docs/handoff/DRAW_44489_KNOWLEDGE_CAPSULE.md')
s = p.read_text()

s = s.replace(
    '# D.RAW 44489 — canonical current knowledge capsule — post-PR #131 integration — 2026-10-11',
    '# D.RAW 44489 — canonical current knowledge capsule — post-PR #131 / runtime-schema governance repair — 2026-10-11',
    1,
)

marker_start = '<!-- 44489_POST_AUDIT_GOVERNANCE_V01_START -->'
marker_end = '<!-- 44489_POST_AUDIT_GOVERNANCE_V01_END -->'
block = '''<!-- 44489_POST_AUDIT_GOVERNANCE_V01_START -->

## 0A. Binding post-audit governance correction — 2026-10-11

This block is authoritative over any conflicting older wording later in this file.

- Historical Exact Gauge PR #130 checkpoint `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f` remains the immutable narrow scientific/review reference.
- The **live PR #130 did not remain frozen**: PR #131 was merged into its head branch `fix/android-exact-gauge-pass-artifact-v03`, which advanced that live review object. PR #130 has therefore been **closed superseded/unmerged**, without force-resetting branch history.
- PR #131 is merged/closed. Final head: `c28b8bacfead4787fdc14cd65e9e00ff4bc7abb8`; merge commit: `28343adf054ccb8f4c741edff187604cf2b0c88a`.
- Post-merge checkpoint/base for the new architecture line: `bfc3dd2b587709498704ee6127854b4ef715383b`.
- Current architecture branch: `feat/draw-observation-evidence-architecture-v01`.
- Current architecture PR: **#132 — Architecture: separate runtime Source Capability evidence-envelope schema v0.1**; it remains a narrow schema/governance review unit until its final exact-head CI is rechecked.
- Runtime schema migration is complete at `e0527b374d6477a0b239125f7aa8d3f8750c5d32`:
  - canonical Physical Knowledge Map: `D.RAW/SourceCapabilityEnvelope/0.2`;
  - runtime parser-bound evidence envelope: `D.RAW/SourceCapabilityRuntimeEvidenceEnvelope/0.1`;
  - occurrence/evidence ledger: `D.RAW/ObservationEvidenceLedger/0.1`.
- The schema split creates no new parser, source read, lifecycle/admission transition, consumer, pixel operation, Scientific Master effect, Appearance effect, evidence or scientific writeback.
- Effective Black / Normalization v0.1 remains standalone/read-only and is **not yet runtime-envelope-exposed**.
- `LinearizationTable` and complete raw-to-linear validity/admission/use remain unresolved.
- A previous one-time 44489 refresh workflow failed fail-closed on a stale text anchor (`key anchor missing`) and made no project-code change. That temporary refresh workflow/script are governance-repair tooling only and are removed from the final clean PR #132 diff after this capsule refresh.
- Repository default branch `main` was not advanced by the PR #131 merge.

Recovery rule: never again describe the live PR #130 itself as frozen. Say **historical frozen checkpoint `4e4f358a...`; live PR #130 superseded/closed**.

<!-- 44489_POST_AUDIT_GOVERNANCE_V01_END -->'''

if marker_start in s and marker_end in s:
    s = re.sub(
        re.escape(marker_start) + r'.*?' + re.escape(marker_end),
        block,
        s,
        count=1,
        flags=re.S,
    )
else:
    motto = 'Permanent motto: **One Free World. Many sealed observations. One evidence law.**'
    idx = s.find(motto)
    if idx < 0:
        raise SystemExit('permanent motto anchor missing')
    insert_at = idx + len(motto)
    s = s[:insert_at] + '\n\n' + block + s[insert_at:]

old_branch = '''A fresh post-merge architecture branch must start from the corrected post-merge 44489 documentation state. Intended branch name:

`feat/draw-observation-evidence-architecture-v01`

If this branch already exists when 44489 is recovered, verify its exact head and ancestry rather than recreating it.'''
new_branch = '''The post-merge architecture branch now exists:

`feat/draw-observation-evidence-architecture-v01`

It was created from post-merge checkpoint `bfc3dd2b587709498704ee6127854b4ef715383b` and is represented by PR #132. Resolve the live PR #132 head on recovery rather than recreating the branch.'''
s = s.replace(old_branch, new_branch, 1)

old_schema = '''- canonical `D.RAW/SourceCapabilityEnvelope/0.2` = historical Physical Knowledge Map v0.2;
- authoritative Android occurrence-ledger identity = `D.RAW/ObservationEvidenceLedger/0.1`;
- the runtime Android envelope label is **legacy compatibility only**;
- no new scientific consumer may bind semantic authority to that ambiguous legacy label;
- before any new Source Capability serialized contract or `ADMITTED -> USED` consumer is introduced, the fresh architecture branch must migrate the runtime shadow envelope to a distinct schema namespace;
- planned namespace: `D.RAW/SourceCapabilityRuntimeEvidenceEnvelope/0.1`.

This migration is a **schema/governance separation**, not a scientific promotion.'''
new_schema = '''- canonical `D.RAW/SourceCapabilityEnvelope/0.2` = historical Physical Knowledge Map v0.2;
- authoritative Android occurrence-ledger identity remains `D.RAW/ObservationEvidenceLedger/0.1`;
- runtime parser-bound envelope identity is now `D.RAW/SourceCapabilityRuntimeEvidenceEnvelope/0.1`;
- the old runtime `D.RAW/SourceCapabilityEnvelope/0.2` label is retained only as non-authoritative legacy provenance;
- `canonical_physical_knowledge_map_serialization_equivalent=false`;
- schema migration commit: `e0527b374d6477a0b239125f7aa8d3f8750c5d32`;
- no new scientific consumer or authority was introduced by this migration.

This completed migration is a **schema/governance separation**, not a scientific promotion.'''
s = s.replace(old_schema, new_schema, 1)

s = s.replace(
    '| Runtime Source Capability envelope label | legacy ambiguous label | must migrate before new consumers |',
    '| Runtime Source Capability evidence envelope | `D.RAW/SourceCapabilityRuntimeEvidenceEnvelope/0.1` | schema-separated; no new consumer/authority |',
)
s = s.replace(
    '| Effective Black runtime-envelope exposure | not yet current | after schema migration only |',
    '| Effective Black runtime-envelope exposure | not yet current | next narrow diagnostic step after clean PR #132 boundary |',
)
s = s.replace(
    '| PR #130 | frozen reference `4e4f358a...` | preserve scientific/audit meaning |',
    '| PR #130 | **closed superseded/unmerged**; historical checkpoint `4e4f358a...` remains frozen | live PR advanced after #131 merge; never call live PR frozen |',
)

s = s.replace(
    '- introduce a new Source Capability consumer before runtime schema identity migration;',
    '- reintroduce the ambiguous legacy runtime schema identity or treat it as canonical Physical Knowledge Map serialization;',
)

start = s.find('## 25. Exact next execution order')
end = s.find('\n---\n\n## 26.', start)
if start >= 0 and end > start:
    replacement = '''## 25. Exact next execution order

On `feat/draw-observation-evidence-architecture-v01`:

1. preserve the completed runtime schema separation at `e0527b374d6477a0b239125f7aa8d3f8750c5d32`; do not redo it;
2. keep historical checkpoint `4e4f358a...` distinct from the superseded/closed live PR #130 review object;
3. remove the temporary one-time 44489 refresh workflow/script after this capsule refresh and recheck the exact clean PR #132 head;
4. keep PR #132 limited to schema/governance separation and closed scientific firewalls;
5. after that boundary is clean, expose Effective Black / Normalization only as an additional read-only diagnostic;
6. prove that exposure changes no ledger digest, records, lifecycle/admission, consumers, source reads, pixels, Scientific Master, Appearance or writeback;
7. next implement a separate `LinearizationTable` shadow validator anchored to DNG 1.7.1.0;
8. keep complete raw-to-linear validity false until linearization + black subtraction + normalization contracts are jointly validated;
9. even after joint validation, keep `VALIDATED`, `ADMITTED` and `USED` separate;
10. later treat colour-calibration families independently and source-bound;
11. keep pending physical/output/Appearance validation separate from the scientific metadata line.
'''
    s = s[:start] + replacement + s[end:]

s = s.replace(
    '### Source Capability / radiometry\n\n- old canonical Physical Knowledge Map v0.2 integration:',
    '### Source Capability / radiometry\n\n- runtime schema migration: `e0527b374d6477a0b239125f7aa8d3f8750c5d32`;\n- current schema/governance PR: #132 on `feat/draw-observation-evidence-architecture-v01`;\n- old canonical Physical Knowledge Map v0.2 integration:',
    1,
)

s = s.replace(
    '- frozen PR #130 reference: `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`',
    '- historical frozen PR #130 checkpoint: `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`; live PR #130 is superseded/closed unmerged',
    1,
)

recovery_start = s.find('## 27. Recovery sentence')
if recovery_start >= 0:
    prefix = s[:recovery_start]
    recovery = '''## 27. Recovery sentence

When continuation code **44489** is given, recover this state:

**PR #131 is merged and closed. Its final green head is `c28b8bac...` and merge integration anchor is `28343adf...` on `fix/android-exact-gauge-pass-artifact-v03`; `main` was not advanced by that merge. Historical Exact Gauge checkpoint `4e4f358a...` remains the frozen narrow scientific/review reference, but the live PR #130 advanced after the PR #131 merge and has therefore been closed superseded/unmerged rather than force-reset. The post-merge architecture branch `feat/draw-observation-evidence-architecture-v01` and PR #132 carry the runtime schema/governance separation. At `e0527b374d6477a0b239125f7aa8d3f8750c5d32`, canonical Physical Knowledge Map remains `D.RAW/SourceCapabilityEnvelope/0.2`, runtime evidence envelope is `D.RAW/SourceCapabilityRuntimeEvidenceEnvelope/0.1`, and occurrence ledger remains `D.RAW/ObservationEvidenceLedger/0.1`; no new scientific consumer or authority was created. Parser-occurrence, raw-IFD, radiometric semantics, encoding consistency, DNG compatibility and spatial BlackLevel dependencies already exist. Effective Black / Normalization v0.1 at `bc93fe...` remains CI-green, metadata-only, standalone and non-promoting; runtime-envelope exposure is the next narrow scientific diagnostic only after the clean schema boundary. `LinearizationTable` remains unresolved, so complete raw-to-linear validity/admission/use remains false. Exact Gauge, reconstruction, T5/Room Capsule, High-Fidelity JPEG and Appearance histories retain their separate authority and physical-acceptance states. One Free World. Many sealed observations. One evidence law.**
'''
    s = prefix + recovery

required = [
    'closed superseded/unmerged',
    'D.RAW/SourceCapabilityRuntimeEvidenceEnvelope/0.1',
    'e0527b374d6477a0b239125f7aa8d3f8750c5d32',
    'Effective Black / Normalization',
    'LinearizationTable',
]
for needle in required:
    if needle not in s:
        raise SystemExit(f'missing required final marker: {needle}')

p.write_text(s)
