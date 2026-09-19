# Recovered universal RAW evidence adapter mapping — 2026-09-19

Status: **HISTORICAL DESIGN RECOVERY / MAPPED INTO v0.57 / NOT ACTIVE DECODER CODE**

Recovered historical references:

- `docs/history/recovered/TRUTHRAW_UNIVERSAL_RAW_EVIDENCE_ADAPTER_V1_2026-09-04.py`
- `docs/history/recovered/TRUTHRAW_UNIVERSAL_COMPATIBILITY_MATRIX_V1_2026-09-04.json`

These files predate the current C++/Android adapter work and contain important architectural rules that must not be lost.

## 1. Historical rule: storage representation != processing lineage

The recovered adapter explicitly separates:

- container decoder;
- storage representation;
- sample topology;
- processing lineage;
- physical exposure count;
- radiometric calibration;
- colour calibration.

Therefore:

`.dng`

does not mean:

`direct untouched sensor evidence`.

And:

`.cr3/.nef/.arw/.raf/etc.`

does not by itself prove Bayer topology, single exposure, native ADC values or any physical calibration state.

v0.57 now carries these distinctions into the native adapter descriptor.

## 2. Historical rule: metadata classification != pixel decoding

The recovered Python adapter deliberately kept metadata/evidence classification separate from pixel decoding and avoided allocating the full sensor image during classification.

v0.57 preserves that principle:

`immutable bytes -> source/evidence admission -> format adapter -> tile source`

with bounded tile access.

A filename/extension can select a candidate adapter, but cannot grant scientific decode authority.

## 3. Storage representation axis

Recovered categories include:

- CFA mosaic;
- packed CFA;
- LinearRaw / linear RGB;
- multi-channel linear;
- opaque vendor RAW.

v0.57 native descriptor now has:

`StorageRepresentation`.

The current DNG path reports `CfaMosaic`.

The synthetic conformance fixture reports `SyntheticCfa`.

Future adapters must declare representation explicitly.

## 4. Sample-topology axis

Recovered categories include:

- Bayer 2x2;
- other periodic CFA;
- Linear RGB;
- unknown/vendor CFA;
- remosaic/physical high-resolution topology.

v0.57 native descriptor now has:

`SampleTopologyFamily`.

Important current limitation:

the existing downstream `DngMetadata` / reconstruction interface is still Bayer-2x2-oriented.

Therefore Fujifilm/X-Trans-like 6x6, RGBW and other periodic CFA sources must **not** be marked native-processing-ready merely because their container decoder exists.

They require a later generic topology/source contract and topology-aware reconstruction gate.

## 5. Processing-lineage axis

Recovered historical classes include:

- direct sensor single-exposure certified;
- direct sensor multi-frame certified;
- direct CFA storage uncertified;
- remosaiced sensor CFA;
- computational CFA/merged RAW;
- linear scene RAW uncertified;
- computational linear RAW;
- virtual reconstructed RAW;
- unknown.

v0.57 native descriptor now has:

`ProcessingLineageClass`.

Current DNG adapter deliberately reports:

`DirectCfaStorageUncertified`

rather than promoting CFA storage to certified direct-sensor authority.

Synthetic conformance data reports:

`SyntheticConformanceOnly`.

## 6. Physical exposure count is independent

The recovered adapter separates stored streams/sample topology from physical exposures.

That rule remains mandatory.

A decoder may not infer:

- one physical exposure from one file;
- multiple independent measurements from multiple image planes;
- sqrt(N) gain from a computational RAW;
- one stored sample == one physical sensel.

v0.57 therefore keeps:

- `physicalExposureCountKnown`;
- `physicalFrameCount`;
- `singleExposureCertified`;
- `storedSampleSenselSemanticsCertified`

separate and false/unknown unless actual acquisition provenance proves them.

## 7. Computational RAW boundary

The historical adapter already distinguished:

- computational CFA/merged RAW;
- computational LinearRaw;
- virtual reconstructed RAW.

These sources can still be scientifically useful, but their upstream processing must remain provenance.

Their stored samples must not silently inherit direct-sensor noise/independence authority.

This remains compatible with the modern Dynamic Authority / TruthNegative / Open-World architecture.

## 8. DNG categories that must remain separate

The historical matrix distinguished at least:

- direct/uncertified CFA DNG;
- computational CFA DNG;
- LinearRaw DNG;
- computational linear RAW.

The current TileNative DNG path only admits the subset its parser/scientific contract actually supports.

Future DNG expansion must classify representation and lineage **before** selecting reconstruction semantics.

## 9. Native v0.57 consequence

The generic source adapter is therefore not just a switch on file extension.

The eventual target is:

`source bytes`
→ container parse
→ representation classification
→ sample-topology classification
→ lineage/evidence classification
→ physical-exposure accounting
→ radiometric/colour binding
→ topology-compatible tile source
→ Scientific Master.

The current v0.57 DNG bridge is the first real instance of this path.

## 10. Next ABI work forced by this recovery

Before claiming universal professional-camera support, the project must add a generic sample-topology layer beyond the current Bayer-2x2 `DngMetadata::cfa` enum.

Until then:

- Bayer-2x2 proprietary formats can be the first real adapter candidates;
- non-Bayer periodic CFA remains fail-closed after container decode;
- linear RAW needs a separate no-demosaic ingest route;
- computational RAW needs explicit lineage-aware admission;
- opaque/vendor-private sample layouts remain unsupported.

This recovered historical design is now a required compatibility constraint on all future multi-vendor work.
