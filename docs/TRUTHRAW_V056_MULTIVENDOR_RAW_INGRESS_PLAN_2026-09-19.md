# TruthRaw v0.56 — unified multi-vendor RAW ingress

Date: 2026-09-19

Status: **INTEGRATION STEP / DNG NATIVE NOW / PROPRIETARY RAW ADAPTERS FAIL-CLOSED PENDING**

Branch:

`integration/truthraw-suite-v0-56-multivendor-raw-ingress`

## Goal

Bring the recovered TruthRaw scientific house, the existing Android processor, and the camera acquisition route into one app with two front doors:

1. **RAW file** — primary route;
2. **Camera** — secondary acquisition route.

Both must converge at the same source-admission boundary:

`source artifact -> sealed source admission -> Measurement/de-ISP -> Scientific Master -> Dynamic Authority -> downstream TruthRaw products`.

Camera access is not a second scientific pipeline.

## File-first product rule

The launcher now presents:

- `1 · RAW-bestand openen · smartphone / professionele camera`;
- `2 · Camera gebruiken · capture → dezelfde RAW-ingang`.

Research/diagnostic tools remain available below those primary product routes.

## Current format support

### Native processing path now

`DNG` is the first multi-vendor native route.

The existing Tile-Native DNG v0.1 decoder remains the scientific implementation. It is strict and fail-closed. A DNG may still be rejected when it uses unsupported compression/storage/topology or lacks required source/color bindings.

No DNG rejection is silently converted into guessed pixels.

### Recognized ingress handles, decoder adapter pending

The app now recognizes common proprietary RAW filename families so they can enter the workspace without pretending they are already scientifically decoded:

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
- generic `.raw`.

For these formats v0.56 stores only the immutable document handle + lightweight metadata and reports `DECODER_PENDING`.

It does **not** create a Scientific Master until a source adapter is implemented and validated.

## Camera convergence

The existing FotoGraaf Camera2 path remains acquisition/metrology authority only.

After a successful exact RAW/Image timestamp pairing, the activity creates a DNG, hashes it, writes the acquisition observation, and now hands the internal DNG path directly to `MainActivity`.

`MainActivity` classifies that source as:

`sourceRoute = CAMERA_CAPTURE`

and passes it through the same DNG ingress used for a manually selected file.

The camera DNG is therefore a source artifact entering the common RAW house, not a separately processed camera picture.

## Developed-image boundary

JPEG/HEIF is not promoted to original RAW.

A future developed-image ingress may create a derived/reconstructed scene or negative, but it must remain:

`DEVELOPED_IMAGE_DERIVED_RECONSTRUCTION`

and may not enter the Direct-CFA/RAW evidence route as measured sensor data.

## Vendor-neutrality rule

Vendor/company identity may affect:

- file parsing;
- metadata interpretation;
- decoder selection;
- calibration-pack lookup;
- capture provenance.

It may **not** change the scientific laws:

- measured/reconstructed/unknown authority;
- one-frame evidence accounting;
- source immutability;
- Scientific Master semantics;
- TruthRange/Backplane rules;
- Dynamic Authority;
- TruthNegative/PURE/Open-World boundaries.

## Next decoder architecture

The next step after v0.56 builds cleanly is a versioned source-adapter boundary:

`Document/File source -> format parser/decoder -> exact CFA/sample-domain + metadata contract -> common IRawTileSource`.

The preferred order is:

1. prove the adapter ABI with one synthetic proprietary fixture;
2. add one real vendor family at a time;
3. seal exact source bytes before decode;
4. preserve manufacturer MakerNote/private metadata as provenance even when not scientifically interpreted;
5. never use a rendered preview as the scientific RAW payload;
6. compare decoded CFA/sample counts and metadata against independent reference tools/fixtures;
7. only then mark that adapter `NATIVE_PROCESSING_READY`.

## Nonclaims

v0.56 does not claim:

- universal DNG compatibility;
- CR3/NEF/ARW/RAF/etc. decoding yet;
- native ADC provenance merely because a professional-camera file is accepted;
- that proprietary metadata has known scientific semantics without explicit validation;
- that camera-generated DNG is stronger evidence than the capture/provenance actually supports.
