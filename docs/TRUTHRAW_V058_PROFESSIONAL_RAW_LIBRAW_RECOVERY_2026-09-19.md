# TruthRaw v0.58 — Professional RAW / LibRaw recovery

Date: 2026-09-19

Status: **HISTORICAL MODULE RECOVERY + CURRENT LIBRAW 0.22.2 COMPATIBILITY VALIDATION**

Branch:

`integration/truthraw-suite-v0-58-professional-raw-libraw-recovery`

## Purpose

v0.57 introduced the common vendor-neutral RAW adapter ABI used by the unified app.

The project history shows that an earlier professional-RAW line already existed and must not be reinvented or lost:

- Professional RAW Ingress v0.1;
- Professional RAW Decoder Adapter v0.1;
- LibRaw Compatibility Probe v0.1;
- borrowed-file-descriptor LibRaw datastream v0.1.

v0.58 recovers those modules by contract and validates them against the current RAW-adapter architecture before a real proprietary pixel decoder is admitted.

## Recovered historical laws

The recovered Professional RAW line already separated:

`container recognized`
!= `decoder exists`
!= `sample equivalence proven`
!= `topology certified`
!= `scientific admission`.

That law remains active.

Historical admission classes:

- `Blocked`;
- `ResearchOnly`;
- `DerivedMeasurement`;
- `SingleFrameDirectCfa`.

Historical evidence classes include:

- `FailClosedUnsupported`;
- `ResearchOnly`;
- `DerivedRawSupported`;
- `ComputationalRaw`;
- `MultiCaptureRaw`;
- `LosslessDecodedCertified`;
- `DirectNativeCertified`.

The key distinction is preserved:

> An external decoder can become `LosslessDecodedCertified` only after exact decoded-sample equivalence is independently validated. It is not relabeled `DirectNativeCertified`.

## Current LibRaw reference

v0.58 pins the host compatibility validation to **LibRaw 0.22.2** source archive:

`https://www.libraw.org/data/LibRaw-0.22.2.tar.gz`

Expected SHA-256:

`de86b035655accff8d4010f1a221fdf50d353cb7b1422ba26f14a0db92612cfa`

OpenMP is disabled for this TruthRaw compatibility gate.

The recovered historical compatibility probe remains metadata/topology-only:

- `open_file/open_datastream` identification is allowed;
- no `unpack()`;
- no `raw2image()`;
- no `dcraw_process()`;
- no LibRaw pixel buffer is admitted into Scientific Master from this probe.

Therefore a successful probe means **decoder/container compatibility evidence only**, not pixel-admission certification.

## Borrowed-fd recovery

The recovered `BorrowedFdDatastream` adapts an already-open file descriptor into `LibRaw_abstract_datastream` using bounded `pread`-style reads.

This is directly relevant to the Android app because Storage Access Framework documents are normally received as file descriptors rather than stable filesystem paths.

The datastream:

- borrows the fd; it does not own/close it;
- tracks its own logical seek position;
- supports random reads without copying the full source;
- reports only its small datastream object as resident state;
- is a transport layer, not decoder/evidence authority.

## Relationship to v0.57

v0.57:

`RAW document -> sealed source -> RawSourceAdapterRegistry -> IRawTileSource`.

v0.58 restores the missing professional-RAW decision layer around future external decoders:

`sealed proprietary RAW`
→ `LibRaw/container probe`
→ `format/topology/resource classification`
→ `external decoder`
→ `sample-equivalence gate`
→ `RawSourceAdapterRegistry / common IRawTileSource`
→ existing TruthRaw science.

The recovered historical modules do not replace the v0.57 ABI. They supply the missing professional-container and decoder-certification vocabulary.

## First proprietary pixel decoder rule

The next implementation is allowed to decode proprietary RAW pixels only if it records:

1. exact source hash and byte length before decode;
2. decoder identity/version/build;
3. camera make/model reported by the container parser;
4. sample topology;
5. decoded dimensions and active area;
6. source bit depth / decoded storage semantics;
7. whether decoding is lossless, lossy or unknown;
8. full-frame materialization and memory upper bounds honestly;
9. exact binding from decoded output to the sealed source;
10. an independent sample-equivalence validation before `LosslessDecodedCertified` promotion.

Until item 10 passes, a successful LibRaw `unpack()` path remains **ResearchOnly** for Scientific Master admission.

## Current target order

1. recover and test historical Professional RAW + LibRaw modules;
2. validate them against pinned LibRaw 0.22.2;
3. route Android document fds through the recovered borrowed-fd probe;
4. expose make/model/topology/decoder metadata in the unified app without unpacking pixels;
5. implement the first real LibRaw pixel adapter behind the v0.57 ABI;
6. validate one real proprietary RAW family with held-out sample-equivalence evidence;
7. only then allow that family into Scientific Master.

DNG remains the only current native scientific pixel route until a proprietary adapter completes the gate.
