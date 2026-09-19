# Multi-vendor RAW Source Adapter v0.1

Status: **RESEARCH / HOST ABI / DNG BRIDGE + SYNTHETIC NON-DNG CONFORMANCE / NO NEW VENDOR SUPPORT CLAIM**

## Purpose

TruthRaw now has two product front doors:

1. RAW file import;
2. camera capture.

Both converge at sealed-source admission. The next architectural requirement is that DNG and future proprietary RAW formats feed the **same** scientific tile-source contract without duplicating the Scientific Master or relaxing provenance.

This module introduces that adapter boundary.

## Core flow

`sealed source bytes`
→ declared RAW family
→ versioned source adapter
→ common `IRawTileSource`
→ existing Measurement/de-ISP
→ existing Scientific Master
→ Dynamic Authority / TruthNegative / PURE / HDR / Open World.

The adapter layer is a transport/decoder boundary. It does not create stronger evidence.

## Public ABI

`IRawByteSource`

Vendor-neutral random-access immutable byte transport.

`RawSourceSeal`

Caller-provided source identity that must already be sealed before decode. The adapter verifies the declared byte length against the byte source before parsing.

`IRawSourceAdapter`

One format-family adapter that may open the immutable source into the existing `streaming_v0_1::IRawTileSource` contract.

`RawSourceAdapterRegistry`

Fail-closed dispatch. If no adapter exists for the declared format, the result is `ADAPTER_UNAVAILABLE`.

## DNG bridge

The DNG adapter wraps the existing validated:

`TileNativeDngSource v0.1`

It does not rewrite the DNG implementation and does not create a second DNG parser.

The adapter requires:

- valid sealed source identity;
- exact byte-length match;
- admitted source-bound color binding;
- bounded resident-memory budget.

It reports exact CFA sample availability but explicitly keeps:

`directSensorAdcClaimAllowed = false`.

A correctly decoded container does not by itself prove untouched ADC provenance.

## Synthetic non-DNG conformance fixture

A deliberately synthetic `TRAWV001` fixture proves that a non-DNG decoder can satisfy the exact same `IRawTileSource` ABI.

This fixture:

- stores a tiny Bayer/CFA plane;
- is random-access;
- is tile-read only;
- does not materialize a full RAW frame;
- exposes row/column bias as zero;
- is marked `syntheticConformanceOnly = true`;
- must never be advertised as support for any real manufacturer format.

The test exists to validate architecture, not image quality.

## Proprietary formats

CR3/CR2, NEF/NRW, ARW, RAF, RW2, ORF, PEF, RWL, 3FR/FFF, IIQ, X3F and the other v0.56 families remain:

`ADAPTER_UNAVAILABLE / DECODER_PENDING`

until a real decoder adapter and independent fixture validation exist.

## Authority rules

The source adapter may establish:

- exact source-container decode;
- exact CFA/sample values exposed by that decode;
- metadata fields that are explicitly parsed and bound.

It may not infer:

- untouched ADC provenance;
- photodiode count;
- native full-resolution authority;
- independent physical color calibration;
- unsupported MakerNote semantics;
- measured values reconstructed from a rendered preview.

## First validation gate

Host tests must prove:

1. duplicate adapter registration fails closed;
2. an unregistered real-vendor family fails with `ADAPTER_UNAVAILABLE`;
3. source byte-length mismatch is rejected before decode;
4. a synthetic non-DNG source can populate the common `IRawTileSource` ABI;
5. arbitrary tile reads return exact source sample codes;
6. no full RAW frame is materialized by the adapter.

After this gate is green, the next step is to compile the adapter ABI into the Android app and route DNG through the registry before adding the first real proprietary RAW decoder.
