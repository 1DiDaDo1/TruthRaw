# TruthRaw Open-World Dynamic Runtime v0.6

Status: **RESEARCH — NOT MAIN-PROMOTED**.

v0.6 turns the v0.5 Dynamic Authority Field contract into a streamed, RGB-channel-aware scientific sidecar bound to the existing Technical Backplane v0.1 lineage.

## Core result

The represented world remains open-ended in radiometric extent, while evidence authority remains finite and explicit. v0.6 does not add sensor dynamic range. It persists, per RGB channel and per scene pixel, what is directly source-bound, reconstructed, censored or unknown.

The runtime path is:

`sealed source evidence -> v4.7i/Scene Master + v0.4 uncertainty/censoring -> v0.6 RGB Dynamic Authority sidecar -> presentation consumers`

The sidecar never writes back into the Scientific Master.

## Per-channel authority, not one label per pixel

A Bayer pixel contains one physically sampled colour role and two missing colour roles. One authority flag per pixel would therefore destroy a critical distinction. v0.6 stores an independent authority record for R, G and B at every pixel.

For the current v4.7i Stage-2 bridge, the physically sampled colour defaults to `CALIBRATED_ESTIMATE`, not `MEASURED`, because Stage-2 is already a calibrated transform of the raw sensor code (black/white normalization and the admitted gain-map path). The record still carries the exact source-sample index and its uncertainty binding.

The two absent Bayer colours become `RECONSTRUCTED` only when the v0.4 result proves all of the following:

- exact measured-channel reinjection;
- the reconstruction/uncertainty backend is the validated binding;
- uncertainty is in-domain for that source class;
- reconstructed support is positive.

If those conditions are not met, the missing channels are `UNKNOWN`; they are not silently borrowed from another camera mode or calibration domain.

## Censoring is an inequality

A clipped highlight is not a recovered value. v0.6 stores the known physical constraint explicitly:

- highlight saturation -> `scene value >= bound`;
- shadow floor -> `scene value <= bound`.

At a censored Bayer site, the sampled channel is `CENSORED` and the two missing channels currently fail closed to `UNKNOWN`. A later evidence-backed model may refine this, but it may not relabel the censored observation as measured detail.

## Technical Backplane binding

Technical Backplane v0.1 remains exactly 180 bytes. v0.6 intentionally does **not** change that format or consume its reserved bytes.

Instead, the streamed Dynamic Authority artifact is externally hash-bound to the validated backplane. The v0.6 parser mirrors the native layout and verifies:

- `TRBACK01` magic, version and exact 180-byte size;
- CRC32 over the first 176 bytes;
- four non-zero SHA-256 lineage bindings;
- `physicalFrameCount = 1`;
- `independentEvidenceCount = 1`;
- zero forbidden scientific-mutation/evidence-promotion flags;
- valid room/claim status bytes and zero reserved bytes.

The resulting lineage binding includes the backplane SHA-256, source/master/zero-line/scene-scale bindings, the RGB field content hash and the persisted artifact hash.

As in Technical Backplane v0.1, validating an embedded SHA-256 is not the same as proving that it matches an external file. Archivist admission remains responsible for external-byte identity.

## Bounded-memory persistence

The artifact writer accepts global-raster-ordered scanline spans and writes canonical NDJSON records directly to a binary sink. It retains only three v0.5 streaming accumulators plus the caller-provided span; no full-frame Dynamic Authority pixel vector is required.

Span boundaries are not written into the scientific content. Therefore the same pixel sequence emitted as one large span or many small spans produces byte-identical output and the same artifact/content hashes.

This is the cheap-phone rule in executable form: weaker hardware may use smaller compute workspaces and smaller persistence spans; stronger hardware may use larger ones. Hardware may change execution geometry and throughput, not scientific authority.

## Compute tiles are not authority regions

A compute tile is a memory/scheduling device, not an epistemic boundary. Upstream v0.4 authority-region semantics must therefore remain canonical and hardware-independent. A cheap phone is not allowed to obtain different reconstruction authority merely because it split the frame into smaller compute tiles.

v0.6 enforces hardware-independent persistence once channel records exist. Fully separating v0.4 authority-region derivation from device-specific compute tiling remains an explicit next gate.

## 200 MP boundary retained

The current v5.0g-p1 uncertainty certification is for the 4080x3072 HONOR BKQ-N49 22.48 mm tele source class. It is not automatically valid for the 16320x12288 maximum-resolution Camera-5 RAW route.

When v0.4 returns `BLOCKED_UNCERTAINTY_OUT_OF_DOMAIN`, v0.6 can still preserve the source-bound sampled CFA channel, but the two missing RGB channels remain `UNKNOWN`. Thus a 200 MP capture cannot gain reconstructed dynamic authority from the 12.5 MP uncertainty model by assumption.

## Claim boundary

v0.6 proves a fail-closed runtime contract and streamed persistence format. Synthetic tests prove the contract behavior, Technical Backplane compatibility, censor inequalities, lineage binding and chunk-size-independent output. It does **not** yet prove a real full-frame Dynamic Authority artifact from a new camera capture.

Next work:

- feed real 4080x3072 Scene Master/v0.4 tiles into v0.6 and persist the first real sidecar;
- make v0.4 authority-region geometry canonical and independent from execution tiling;
- bind real per-sample censor bounds and measured uncertainty bands from the acquisition/noise pipeline;
- connect presentation/HDR projection to the sidecar without authority writeback;
- separately validate 16320x12288 Camera-5 uncertainty before reconstructed 200 MP authority is admitted.
