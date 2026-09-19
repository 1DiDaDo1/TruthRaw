# TruthRaw v0.57 — generic RAW source adapter ABI

Date: 2026-09-19

Status: **INTEGRATION / DNG ROUTED THROUGH GENERIC ADAPTER / PROPRIETARY DECODERS STILL FAIL-CLOSED**

Branch:

`integration/truthraw-suite-v0-57-raw-source-adapter-abi`

## Purpose

v0.56 created one app with:

1. RAW file as the primary front door;
2. camera capture as the secondary front door;
3. both converging at sealed-source admission.

v0.57 begins the next layer: vendor/container-specific decoders must converge on one common scientific source contract instead of creating separate pipelines per company.

## Adapter contract

New research module:

`docs/research/multivendor-raw-source-adapter-v0.1/`

Core ABI:

`immutable sealed byte source`
→ `IRawSourceAdapter`
→ `streaming_v0_1::IRawTileSource`
→ existing Measurement/de-ISP / Scientific Master.

The adapter layer is deliberately below Scientific Master authority.

## Source sealing

The adapter does not invent or recalculate scientific source authority.

The caller must already provide a valid sealed source identity.

The adapter boundary checks:

- source handle is non-null;
- sourceEvidenceId is present;
- sealed byte length equals the actual immutable byte source length;
- memory budget is non-zero.

The exact source SHA remains owned by the existing source-seal/reverification path.

The descriptor therefore records:

`sourceSealAcceptedAtBoundary`

not a false claim that the adapter independently re-hashed the source.

## DNG

DNG is now the first real format routed through the generic adapter ABI.

The adapter does **not** replace the existing DNG parser.

It wraps:

`TileNativeDngSource v0.1`

after:

- exact source SHA seal;
- DNG metadata colour producer;
- source-bound preview admission.

Actual finalized Scientific Preview source creation and Linear DNG export source creation now call:

`RawSourceAdapterRegistry -> DNG adapter -> TileNativeDngSource`.

Therefore the current DNG scientific equations and parser are unchanged while the entry architecture becomes vendor-neutral.

## Synthetic non-DNG conformance fixture

A tiny synthetic `TRAWV001` container proves that a non-DNG adapter can satisfy the same `IRawTileSource` ABI.

The fixture validates:

- random-access source bytes;
- exact CFA sample readout;
- arbitrary tile/halo reads;
- zero row/column bias contract;
- no full RAW frame materialization;
- no direct sensor/ADC claim;
- explicit `syntheticConformanceOnly=true`.

It is **not** a real camera format and must never be advertised as such.

## Current proprietary status

Recognized user-file families still remain decoder-pending:

- Canon CR3 / CR2;
- Nikon NEF / NRW;
- Sony ARW;
- Fujifilm RAF;
- Panasonic/Lumix RW2;
- Olympus/OM System ORF;
- Pentax/Ricoh PEF;
- Leica RWL;
- Hasselblad 3FR / FFF;
- Phase One IIQ;
- Sigma X3F;
- Samsung SRW;
- Epson ERF;
- Kodak DCR/KDC;
- Minolta MRW;
- Mamiya MEF;
- generic RAW.

They may enter the workspace as immutable source handles, but the app creates no Scientific Master for them until a real adapter exists.

## Android integration

The Android native library now compiles the generic adapter module.

A bridge converts the already-sealed DNG admission into a generic `RawSourceOpenRequest`, registers the DNG adapter and obtains a common `IRawTileSource`.

DNG-specific audit metrics are retained only by dynamic access to the underlying `TileNativeDngSource` so existing empirical reporting remains intact.

New native adapter error range:

`7001..7008`

is surfaced to the Android UI for invalid arguments, seal-length mismatch, missing adapter, duplicate registration, invalid/unsupported container, decode failure and memory-budget failure.

## Camera convergence remains unchanged

Camera capture remains a second acquisition front door.

After exact RAW/Image timestamp pairing, the camera path creates and hashes a DNG and transfers that DNG to the same processor.

The DNG then enters the same generic adapter ABI as a manually selected DNG.

No camera-specific Scientific Master exists.

## Scientific nonclaims

v0.57 does not claim:

- real CR3/NEF/ARW/RAF decoding;
- untouched ADC provenance from container decode;
- native full-sensor authority from filename or dimensions;
- independent physical colour calibration merely because metadata exists;
- proprietary MakerNote semantics not explicitly decoded and validated;
- that the synthetic fixture is representative of a real vendor RAW.

## Next gate

After host + Android CI are green:

1. freeze this adapter ABI as the first decoder boundary;
2. add the first real proprietary adapter behind the registry;
3. validate that adapter against real files + independent reference decode;
4. compare exact CFA geometry/sample counts/black/white/CFA and metadata;
5. keep unsupported features fail-closed;
6. only then mark that one format family `NATIVE_PROCESSING_READY`.

The first real proprietary format should be chosen from an available known test file, not from brand preference.
