# TruthRaw document status index — 2026-09-23 final chat snapshot

Status: **DOCUMENTATION-ONLY NAVIGATION SNAPSHOT**

This index was added at the end of the 2026-09-23 chat to preserve the latest state known to that conversation. It does not assert that no newer branch, code commit, APK, device result or handoff exists elsewhere.

## Read first

1. `state/CURRENT_CHAT_KNOWN_STATE_2026-09-23.json`
2. `docs/handoff/TRUTHRAW_FINAL_CHAT_HANDOFF_2026-09-23.md`
3. `docs/handoff/TRUTHRAW_FINAL_CODE_FREEZE_V0843_2026-09-21.md`
4. `state/CURRENT_PROJECT_STATE_2026-09-21.json`

## Code authority

Frozen production/integration reference remains:

`integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master @ 172a100786eb18d4b08564bbfb025a44f42cfa1e`

The user explicitly requested no further code changes after the final handoff.

## Latest Camera-5 research known to this chat

- v0.56c: 8160x6144 RAW_SENSOR envelope, meaningful 4080x3072 U16 payload.
- v0.57: 8160x6144 RAW10 envelope, meaningful 4080x3072 packed RAW10 payload in two independent captures.
- v0.58: 4080x3072 STANDARD RAW10 control build succeeded; device result was still pending in this chat.

## Important precedence rule

If a later handoff/state file or branch contains device evidence after v0.58, treat that later evidence as newer. Do not repeat experiments merely because this snapshot says they were pending.

## Historical documents remain immutable

Older dated state and handoff files are historical snapshots. Do not rewrite them as if they knew the 2026-09-23 Camera-5 results.

## Final instruction

Documentation may be reconciled with newer evidence.

**Do not change TruthRaw code unless the user explicitly authorizes code changes again.**
