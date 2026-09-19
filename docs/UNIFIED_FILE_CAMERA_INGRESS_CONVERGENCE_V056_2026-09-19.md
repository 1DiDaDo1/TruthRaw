# TruthRaw unified ingress convergence — file upload + camera capture — v0.56

Status: **ARCHITECTURE CONTRACT / NO NEW SENSOR AUTHORITY**

TruthRaw v0.56 has two front doors that must converge on the same scientific admission path:

`A. user-selected RAW file`
and
`B. camera capture`.

The two front doors are allowed to differ in acquisition mechanics, but after a valid source artifact exists they must enter the same sealed-source workflow.

## 1. Correct convergence

Preferred camera path:

`physical exposure`
→ camera/vendor acquisition path
→ actual RAW/DNG sensor-domain artifact
→ exact source-byte seal + source identity
→ vendor-neutral RAW ingress
→ Measurement/de-ISP
→ Scientific Master
→ Dynamic Authority / Open Scene / TruthNegative / PURE / HDR projections.

This means camera access is a second acquisition choice, not a second scientific house.

## 2. Important boundary: developed image is not original RAW

If the camera only produces a rendered/developed artifact such as JPEG/HEIF, TruthRaw may ingest that artifact as a source document, but it cannot recover or relabel the original sensor RAW as measured evidence.

A negative reconstructed from a developed image is:

`DEVELOPED_IMAGE_DERIVED_RECONSTRUCTION`

not:

`MEASURED_SENSOR_NEGATIVE`.

It may still be useful for restoration/Open-World work, but its authority must remain reconstructed/derived and cannot silently enter the Direct-CFA RAW ingress as if it were sensor evidence.

## 3. TruthNegative direction

TruthNegative is normally produced **after** Scientific Master admission:

`sealed RAW/DNG -> Measurement/de-ISP -> Scientific Master -> authority -> TruthNegative Core`.

Do not build the canonical TruthNegative first from a JPEG and then feed it backward into the RAW ingress.

A reconstructed negative from a developed-only source may exist as a separate derived-input class, but it must remain source-typed and authority-bounded.

## 4. Unified app rule

The app UI may expose:

- **RAW-bestand openen** — smartphone, mirrorless, DSLR, medium format, scanner/camera DNG, etc.;
- **Camera gebruiken** — capture locally and, where the camera/OS provides an admissible RAW/DNG artifact, seal that artifact and hand it to the exact same RAW ingress.

After source admission the downstream processing path is shared.

The file-ingress and camera-ingress paths must therefore converge at:

`SEALED_SOURCE_ADMISSION`

not at JPEG preview, not at appearance output, and not at a reconstructed negative.

## 5. Future camera adapter contract

A camera adapter is allowed to hand a capture to the common RAW ingress only when it can provide:

- stable source bytes;
- explicit source format/container;
- frame/evidence identity;
- capture provenance where available;
- no hidden pixel mutation after the source seal.

If only a developed file exists, route it through a future `DevelopedImageIngress` rather than the Direct-CFA/RAW ingress.

Permanent law:

> Measured where measured. Reconstructed where necessary. Never invented.
