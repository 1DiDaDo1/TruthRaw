# TruthRaw v0.59 — LibRaw reference backend boundary

Date: 2026-09-19

Status: **REFERENCE/VALIDATION BACKEND — NOT YET A SCIENTIFIC RUNTIME DECODER**

Pinned upstream:

- project: LibRaw;
- release: 0.22.2;
- annotated tag commit: `b93f6e45c194f5df9b02a43b1af9a54b4f41f33f`;
- license choice for TruthRaw integration work: CDDL 1.0 path, preserving upstream notices.

## Why this exists

TruthRaw needs broad camera-company RAW coverage without allowing decoder convenience to weaken scientific authority.

The historical Universal RAW Evidence Adapter already established two rules that remain correct:

1. storage representation is separate from capture/processing lineage;
2. pixel decoding is separate from evidence classification.

v0.59 adds a **reference decoder backend** around LibRaw so TruthRaw can compare its own vendor-specific adapters against a mature independent decoder.

LibRaw is not automatically promoted into the Scientific Master path.

## Intended roles

LibRaw v0.59 may be used for:

- camera/format identification;
- decoder-function identification;
- reference sample decode;
- metadata comparison;
- cross-checking geometry/CFA/black/white fields;
- test-oracle comparison against TruthRaw native adapters.

It may not by itself establish:

- untouched ADC provenance;
- one stored sample = one physical photodiode measurement;
- independent physical color calibration;
- single-exposure provenance;
- a calibrated noise model;
- Scientific Master eligibility.

## Resource-invariance boundary

LibRaw `unpack()` commonly creates a full decoded RAW buffer. That conflicts with TruthRaw's preferred bounded-memory streaming architecture.

Therefore the first LibRaw backend is classified:

`REFERENCE_DECODER_FULL_RAW_MATERIALIZATION_ALLOWED_ONLY_OUTSIDE_CANONICAL_STREAMING_PATH`.

A future Android compatibility mode may use it only if all of the following are explicit:

- peak memory is measured and reported;
- full-frame materialization is visible in provenance;
- source bytes remain sealed and immutable;
- the resulting decoded CFA is a derived representation, not a replacement source;
- the canonical streaming-native decoder remains preferred where available;
- no resource-invariance claim is made for a LibRaw materializing route.

## Version pin

The CI reference backend must use the exact upstream commit:

`b93f6e45c194f5df9b02a43b1af9a54b4f41f33f`

corresponding to LibRaw 0.22.2.

No floating `master`, `latest` or unpinned package-manager version is allowed.

## First gate

v0.59 gate 1 is host-only:

1. fetch the pinned LibRaw source;
2. build it without post-processing extras needed by TruthRaw;
3. compile the TruthRaw reference probe;
4. confirm reported LibRaw version is exactly 0.22.2;
5. confirm camera list is available;
6. do not run `dcraw_process()`;
7. do not promote decoded output into Scientific Master.

After this gate, real vendor RAW fixtures can be cross-decoded in held-out tests before Android runtime integration.
