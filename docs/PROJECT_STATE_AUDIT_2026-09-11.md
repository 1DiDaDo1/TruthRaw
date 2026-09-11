# TruthRaw project state audit — 2026-09-11

## Audit objective

Refresh the global repository documentation so a new chat/engineer can continue without relying on stale 2026-09-10 “current” files or conflating open research with promoted `main`.

## Audited surfaces

The audit reviewed:

- repository root entrypoints and directory structure;
- promoted `main` head;
- full branch listing available through the repository API at audit time;
- open draft PRs #9 through #20 individually;
- current and historical state snapshots;
- current/historical house/document-status dashboards;
- documentation-governance verifier/workflow;
- newest Android source-bound preview module README;
- professional RAW/Gatehouse branch lineage and known validation evidence;
- current Android integrated-head workflow evidence;
- decoded handoff/tile-source current-head workflow evidence;
- known failure histories described by validated modules/PRs.

## Material stale-state findings

Before this refresh:

1. root `README.md` still treated bounded-memory migration as the next major task;
2. `START_HERE_NEW_CHAT.md` still bootstrapped the 2026-09-10 state;
3. `state/README.md` pointed only to `CURRENT_CANONICAL_STATE_2026-09-10.json`;
4. documentation governance was hardcoded to 2026-09-10 current entrypoints;
5. the global architecture dashboard predated TileNative/Full-Frame Streaming completion, Gatehouse, persisted decoded handoff, preview-format policy, source binding and source-bound Android color preview;
6. current professional-RAW/Gatehouse work is spread across stacked branches and therefore cannot be understood correctly from the Android preview branch alone.

## Preservation decisions

The audit deliberately does **not** rewrite:

- older dated state snapshots;
- old dated architecture/document-status snapshots;
- canonical module/version documentation;
- module-local failure histories;
- research README bytes merely because later branches have newer global status.

Those files are evidence of what was true/claimed at their recorded stage. Current truth is layered above them through new 2026-09-11 entrypoints.

## Current repository distinction

Promoted `main`: `514f2f4bde6aba5a6709e176c03b22c3b9aea912`.

Integrated research handoff basis: `2fdf05ca1bbbc59cd8867df0cae117d1eec92d51`.

These are intentionally not conflated.

## Key audit conclusions

### Memory/runtime

The old “next task = memory architecture migration” statement is obsolete. Tile-native source and streaming interfaces now exist and are exercised by later integration work. Resource isolation is extended further by the Professional RAW Gatehouse concept.

### Professional RAW

TruthRaw has a robust compatibility/admission architecture but still lacks grounds for blanket vendor RAW support. LibRaw metadata probe validation does not prove pixel sample equivalence. Certified production vendor decode remains open.

### Gatehouse

The Gatehouse design is consistent with Main-House resource/truth separation. Handoff and decoded tile-source branches have current-head successful workflows. The later Main-House E2E branch exists but had no current-head workflow association during this audit.

### Android preview

The newest integrated research branch builds a source-bound DNG-metadata color appearance preview and portable JPEG path without promoting it to finalized Scientific Preview or Scientific Master authority. Physical target-device execution remains unproven.

### Documentation

The repository now needs one current global layer and immutable historical/module layers. The 2026-09-11 entrypoints implement that separation.

## Remaining risks / open evidence

- branch sprawl includes audit/temp/staging names; branch existence alone must never be used as status authority;
- open draft PRs are stacked on different bases and are not a single merge-ready chain by default;
- physical Android evidence is behind build/package evidence;
- source metadata color is weaker than independent calibration;
- Backplane phase 2 needs real Scientific Master identity;
- production professional RAW decoding/corpus/certification remains open;
- multi-capture evidence architecture remains future work.

## Audit result

Global current-state documentation is updated on `docs/project-handoff-2026-09-11` without modifying the scientific meaning of canonical/history files. CI governance must pass on the final documentation commit before this audit is called complete.