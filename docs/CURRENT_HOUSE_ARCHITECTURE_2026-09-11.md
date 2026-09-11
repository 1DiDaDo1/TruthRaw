# TruthRaw current house architecture — 2026-09-11

Status: **current global architecture handoff** for `docs/project-handoff-2026-09-11`.

This document supersedes `CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md` as the current dashboard. The older file remains an immutable historical snapshot.

## 1. Truth authority vs execution authority

TruthRaw has two orthogonal axes:

- **truth/evidence authority** — what is measured, reconstructed, hypothetical, appearance-only or projection-only;
- **resource/execution authority** — memory budget, tile size, workers, cache retention, thermal/background admission and backend choice.

A cheap phone and a flagship may execute differently. They may not obtain different evidence classes or different scientific authority from the same admitted measurements merely because one has more RAM/CPU/GPU.

## 2. Source House, Gatehouse, Main House

```text
SEALED SOURCE HOUSE
  immutable source bytes + source identity
          |
          v
SOURCE ROUTER
  |                                  |
  | native-certified                 | external decoder required
  v                                  v
MAIN HOUSE                    PROFESSIONAL RAW GATEHOUSE
                              decode / topology / provenance
                              resource quarantine / failure isolation
                                      |
                                      v
                              VERIFIED PERSISTED HANDOFF
                                      |
                              decoder/context detached
                                      |
                                      +---------------------> MAIN HOUSE
```

### Native-direct path

A source that is proven to fit a native TruthRaw reader profile bypasses the Gatehouse. Today the principal native route is the strict `TileNativeDngSource v0.1` DNG subset.

### Gatehouse path

The Gatehouse exists so heavy/external decoder state does not burden or contaminate the Main House. It has its own RAM/thread/scratch/cache/failure domain and should normally finish, seal its handoff and release decoder resources before heavy Main-House work starts.

The Gatehouse does **not** own a TruthRaw Scientific Master, zero-line or new evidence source. Decode representation remains the same physical capture/evidence lineage.

## 3. Main House rooms

The current conceptual 12-room Main House remains:

1. Archivist — sealed source/evidence custody
2. MeasurementLab — measurement semantics/likelihood/censoring
3. Architect — scientific reconstruction planning
4. Restorer — latent-scene estimation while preserving measured evidence
5. SceneRegistry — scene/master registry and lineage binding
6. Surveyor — supported scene analysis/uncertainty
7. ManifoldConditioning — exact/reversible conditioning; no evidence creation
8. LightingStudioCicm — counterfactual illumination/capture hypotheses only
9. RoomCapsule — bounded local execution/resource capsule
10. Colorist — appearance/color rendering under explicit authority
11. Finisher — supported detail/acutance/finish only
12. Exporter — projection/media/output formatting

Truth-domain order remains forward-only conceptually:

`Foundation -> Measurement -> Reconstruction -> Scene -> Counterfactual -> Appearance -> Projection`.

## 4. Scientific reconstruction

Canonical v4.7i is the current measured-preserving single-frame scientific reconstruction baseline:

- measured CFA components are reinjected exactly where required;
- edge-aware reconstruction does not create semantic/generative texture;
- camera-native scene-linear role is preserved;
- GainMap application is exactly once where admitted;
- derived LinearRaw is compatibility projection, never relabeled original RAW;
- tile/thread interfaces exist and are used by the streaming research stack.

v4.7j/v4.7k are appearance/detail/output-acutance successors, not replacements for the scientific master/reconstruction authority.

## 5. Bounded streaming architecture

The old full-memory migration described as future work on 2026-09-10 is no longer the current architecture target. Research now contains:

- `IRawTileSource` / random-access source contracts;
- `TileNativeDngSource` exact tile/strip reads;
- two-pass `StreamingTruthRawProcessor`;
- caller-owned streaming sinks;
- Room ABI/Building Runtime resource admission;
- bounded Android preview surfaces.

The source/UI adapters are required to report materialization ownership. Full-frame ownership may not be hidden behind an adapter.

## 6. Technical Backplane

Technical Backplane v0.1 remains exactly 180 bytes and binds lineage metadata including:

- sourceEvidence SHA-256;
- scientific master SHA-256;
- zero-line SHA-256;
- scene-scale SHA-256;
- physical-frame/evidence counts;
- 12 room statuses;
- claim status/reserved/CRC32.

It contains no pixels/calibration. One lineage has one shared Backplane.

The current Android phase-1 preview deliberately does **not** fabricate missing Scientific Master/zero-line/scene-scale hashes. Phase-2 finalization remains open until deterministic Scientific Master identity exists.

## 7. Professional RAW Gatehouse stack

Current research architecture:

`sealed original -> format/probe/router -> decoder adapter -> measurement/topology descriptor -> Professional RAW Ingress -> Gatehouse resource admission -> decode chamber -> sample/provenance audit -> persisted handoff -> detach -> IRawTileSource bridge -> Main House`.

The validated LibRaw compatibility module is metadata-only. `open_file/open_datastream` may discover compatibility/topology, but `unpack()` pixel decode is not silently granted scientific authority.

For an external decode to become `LosslessDecodedCertified`, named codec/sample-equivalence/source-binding evidence is required. A successful library call alone remains insufficient.

## 8. Android source-bound preview architecture

Current integrated phase-1 path:

```text
ACTION_OPEN_DOCUMENT Uri
  -> caller-owned ParcelFileDescriptor
  -> borrowed fd
  -> bounded SHA-256 of exact source bytes
  -> DNG Color Binding Producer v0.1
  -> SOURCE_METADATA_BOUND record
  -> Scientific Preview Source Binding v0.2 phase 1
  -> TileNativeDngSource
  -> StreamingTruthRawProcessor / v4.7i
  -> BoundedSrgbPreviewSink
  -> ARGB_8888 + explicit sRGB Bitmap
  -> optional baseline JPEG/sRGB presentation export
```

Authority at this stage:

- Main-House compute: allowed;
- labeled source-bound appearance release: allowed;
- finalized Scientific Preview: blocked;
- scientific claim: blocked;
- independent calibration: not implied;
- frame/evidence: 1/1.

## 9. Preview format policy

Preview is role-based, not source-format-based:

- runtime UI surface: bounded ARGB_8888/sRGB;
- portable compatibility preview: baseline JPEG 8-bit sRGB;
- lossless diagnostic preview: PNG;
- optional future HDR display derivative: Ultra HDR JPEG after physical SDR/device validation.

RAW/DNG/TIFF/HEIF/AVIF export choices do not dictate the live preview representation. Preview bytes never become CFA evidence, calibration or Scientific Master identity.

## 10. Current hard open boundaries

The architecture is not complete in the following areas:

- real physical Honor/MotionCam execution of the latest color route;
- deterministic Scientific Master serialization/hash and Backplane phase 2;
- independent camera/lens calibration;
- certified LibRaw/vendor pixel decode and real-file codec matrix;
- CI proof for current decoded-measurement Main-House E2E branch;
- real multi-capture evidence/fusion/HDR contracts;
- production encoders/exporters for all desired media formats.

No document may promote these from open work to proven capability without new evidence.