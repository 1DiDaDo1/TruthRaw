# TruthRaw

TruthRaw is a single-frame RAW reconstruction system built around one rule:

> **Measured where measured. Reconstructed where necessary. Never invented.**

## Current repository state — 2026-09-11

This README is the current global entrypoint for the project state as audited on 2026-09-11.

Two repository states must be kept distinct:

- **promoted `main` head:** `514f2f4bde6aba5a6709e176c03b22c3b9aea912` — validated Technical Backplane v0.1 is the latest promoted main commit;
- **current integrated research/handoff basis:** `2fdf05ca1bbbc59cd8867df0cae117d1eec92d51` on `research/android-source-bound-color-preview-v0.1-2026-09-11` — this is a stacked research state, not a claim that all research has been promoted to `main`.

The documentation refresh is maintained on `docs/project-handoff-2026-09-11`, created from that exact integrated research head.

### Read these first

1. `START_HERE_NEW_CHAT.md`
2. `state/CURRENT_CANONICAL_STATE_2026-09-11.json`
3. `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-11.md`
4. `docs/CURRENT_MODULE_STATUS_2026-09-11.md`
5. `docs/CURRENT_CLAIM_MAP_2026-09-11.md`
6. `docs/CI_EVIDENCE_INDEX_2026-09-11.md`
7. `docs/DOCUMENT_STATUS_INDEX_2026-09-11.md`
8. `docs/DOCUMENTATION_SYNC_POLICY_2026-09-11.md`
9. `docs/CHAT_HANDOFF_2026-09-11.md`
10. `docs/PROJECT_STATE_AUDIT_2026-09-11.md`

Dated 2026-09-10 and earlier current-state files remain immutable historical snapshots. They are not the current bootstrap.

## Mandatory documentation synchronization

Every substantive TruthRaw change must review and update the affected module README/state overlay and, when the global continuation point changes, the current handoff/state documents in the same work cycle. A validated implementation head may temporarily precede its documentation overlay, but it must not become the new global handoff until the documentation/state synchronization is committed and governance-checked.

Historical, sealed and versioned evidence is never rewritten merely to look current. When an older README must remain exact for reproducibility, current status belongs in an overlay/global index instead. See `docs/DOCUMENTATION_SYNC_POLICY_2026-09-11.md`.

## Scientific invariants

- Direct CFA/original source evidence is immutable and sealed.
- Ordinary single-frame lineage remains `physicalFrameCount = 1` and `independentEvidenceCount = 1`.
- Measured, reconstructed, counterfactual, appearance and projection roles remain separate.
- A representation may exceed source/output range; knowledge claims may not exceed evidence.
- Virtual observations do not add photons, measurements, SNR or independent evidence.
- Counterfactual illumination/capture worlds never become evidence for the original captured world.
- Appearance processing may not mutate scientific evidence or the Scientific Scene Master.
- APK/GCam/computational-RAW data may not determine TruthRaw evidence, calibration, topology, color or noise authority.
- Resource capacity may change tile size, concurrency, caching, scheduling or admission; it may not change truth authority.

The zero-line remains a shared scene-reference gauge: `T = log2(L/L0)` for `L0 > 0`. It is not DNG BlackLevel, zero photons, clipping, display black or sensor black.

## Current architecture

TruthRaw now has two admitted source routes:

```text
                         +--> native-certified source --> Main House
sealed source --> router |
                         +--> external decode needed --> Professional RAW Gatehouse
                                                       --> sealed decoded-measurement handoff
                                                       --> decoder detached
                                                       --> Main House
```

The **Gatehouse is not a second scientific house**. It may decode, audit topology/provenance, enforce a separate memory envelope and produce a sealed handoff. It may not create a second zero-line, Scientific Master or evidence source.

The Main House keeps the 12-room model:

1. Archivist
2. MeasurementLab
3. Architect
4. Restorer
5. SceneRegistry
6. Surveyor
7. ManifoldConditioning
8. LightingStudioCicm
9. RoomCapsule
10. Colorist
11. Finisher
12. Exporter

## Native-direct RAW support

The current native scientific route is intentionally strict. `TileNativeDngSource v0.1` supports the proven classic TIFF/DNG subset used by its contract: 2x2 RGB Bayer, one unsigned 16-bit sample per pixel, `Compression=1`, supported scalar metadata/opcode conditions and bounded tile/strip reads.

Packed 10/12/14-bit, compressed DNG, BigTIFF, X-Trans/non-2x2 CFA and other unsupported variants must fail closed until separately implemented and certified. A `.dng` extension alone is never sufficient scientific admission.

## Professional RAW status

Professional RAW compatibility is being built as adapters around, not replacements for, the native Main-House path.

Implemented research layers include:

- Professional RAW Ingress v0.1: container/topology/compression/certification/evidence/resource classification;
- Professional RAW Decoder Adapter v0.1: decoder provenance/resource/sample-equivalence contract;
- LibRaw Compatibility Probe v0.1: metadata-only `open_file/open_datastream` probing with no pixel `unpack()` authority;
- borrowed-fd LibRaw datastream implementation;
- Professional RAW Gatehouse Runtime v0.1;
- Decoded Measurement Handoff v0.1;
- Decoded Measurement Tile Source v0.1.

**Do not claim universal professional RAW decode support.** The current project does not yet contain a certified production LibRaw pixel-decode adapter for real CR3/NEF/ARW/RAF/IIQ/etc. A library being able to open a file is not scientific certification.

## Android and preview status

The current Android research stack has progressed beyond the historical gray CFA proxy:

`ParcelFileDescriptor -> source SHA-256 -> DNG Color Binding Producer -> Scientific Preview Source Binding v0.2 phase 1 -> TileNativeDngSource -> v4.7i StreamingTruthRawProcessor -> bounded sRGB preview -> Android Bitmap -> optional JPEG`.

Current role of the visible result is **`SOURCE_BOUND_APPEARANCE_PREVIEW`** with color authority **`SOURCE_METADATA_BOUND`**.

That means:

- source-bound appearance release is allowed;
- final Scientific Preview release is still blocked pre-master;
- scientific claim is still blocked pre-master;
- no dummy Scientific Master, zero-line or scene-scale hash is generated;
- the historical parser sentinel/identity matrix is not permitted as scientific color authority.

The current integrated Android branch head `2fdf05ca1bbbc59cd8867df0cae117d1eec92d51` has green repository CI for the source-bound color-preview APK build, reconstructed color preview, Adaptive UI/Ingress, Adaptive UI Tile Preview and canonical integrity. This proves build/package/static-contract integration, **not physical Honor execution**.

Preview representation policy:

- live UI baseline: bounded `ARGB_8888`, explicit sRGB;
- portable compatibility preview: baseline JPEG, 8-bit sRGB;
- PNG: lossless diagnostics/regression only;
- Ultra HDR JPEG: optional future display derivative after SDR validation;
- preview bytes are never source evidence or Scientific Master identity.

## Current proof boundary

Still open / not claimed:

- physical Honor/MotionCam DNG execution of the new source-bound color route;
- on-device RSS, thermal, latency and frame-time validation;
- independent camera/lens spectral calibration or `FULL_PHYSICAL` color truth;
- deterministic Scientific Master digest and phase-2 Technical Backplane finalization;
- finalized Scientific Preview authority;
- certified real vendor RAW pixel decoding through LibRaw/Gatehouse;
- a real-file professional RAW certification corpus;
- validated decoded-measurement Gatehouse-to-Main-House E2E on the current `decoded-measurement-main-house-e2e-v0.1` head;
- multi-capture registration/fusion/HDR science.

## Documentation policy

- every substantive project change must obey `docs/DOCUMENTATION_SYNC_POLICY_2026-09-11.md`;
- `canonical/**` READMEs are version/module authority, not global current-state dashboards;
- `docs/research/**` READMEs are module-local research contracts/evidence and may describe the state of their own branch at a specific time;
- dated old `CURRENT_*` and project-audit files remain historical snapshots and are never silently rewritten into success;
- failure histories are preserved as failures;
- global current status is defined only by the 2026-09-11 entrypoints listed above.

See `docs/DOCUMENT_STATUS_INDEX_2026-09-11.md` for the authority map.