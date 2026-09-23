# TruthRaw Camera-5 16320x12288 RAW10 v0.59 — real-device result

Date: 2026-09-23

Status: **DEVICE RESULT CLOSED / PUBLIC CAMERA2 HIGH-RES RAW10 DOES NOT EXPOSE A POPULATED 200MP SAMPLE DOMAIN**

Branch:

`test/cam5-16320x12288-raw10-v059`

Frozen production/scientific anchor remains unchanged:

`integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master`
at `172a100786eb18d4b08564bbfb025a44f42cfa1e`.

## Device artifacts

Evidence JSON:

`TRUTHRAW_1790175506221_CAM5_16320_RAW10_EVIDENCE_v059.json`

Sealed RAW10 source:

`TRUTHRAW_1790175506221_CAM5_16320_RAW10_SOURCE_16320x12288_v059.raw10`

Full source SHA-256:

`e058d33821bb3ec66242c6618c0e0083bf597645e31089e4877ecd3e31bb1ee6`

Full source bytes:

`250,675,200`

## Request/result identity

Observed:

- logical camera 0;
- physical Camera 5;
- physical-scoped request used;
- no logical/global SENSOR_PIXEL_MODE write;
- physical SENSOR_PIXEL_MODE MAXIMUM_RESOLUTION write attempted and accepted;
- builder readback = 1 / MAXIMUM_RESOLUTION;
- output declared MAXIMUM_RESOLUTION;
- Image = 16320x12288 RAW10;
- physical result camera id = 5;
- Image timestamp == physical SENSOR_TIMESTAMP;
- returned physical SENSOR_PIXEL_MODE = 0 after seal;
- RAW binning factor used = true;
- physical frame count = 1;
- independent evidence count = 1.

## Declared plane geometry

Android Image.Plane reports:

- width = 16320;
- height = 12288;
- packed RAW10 row bytes for declared width = 20400;
- rowStride = 20400;
- pixelStride = 0;
- total accessible bytes = 20400 x 12288 = 250,675,200;
- therefore the allocation is mathematically a canonical contiguous 16320x12288 RAW10 plane.

This is allocation/stride geometry only. It is not proof of payload population.

## Byte-population audit

A complete byte scan of the sealed 250,675,200-byte source found:

- non-zero bytes occur only before offset 15,728,620;
- the first 15,728,640 bytes form exactly 3072 slots of 5120 bytes;
- in every one of those 3072 slots:
  - first 5100 bytes are the 4080-pixel packed RAW10 data region;
  - final 20 bytes are exact zero padding;
- all 3072 effective 4080-wide rows contain non-zero image data;
- every one of the 61,440 effective padding bytes is zero;
- every byte after offset 15,728,640 is zero.

Therefore:

- standard-domain prefix = 15,728,640 bytes;
- actual packed pixel region = 5100 x 3072 = 15,667,200 bytes;
- zero tail after the standard-domain prefix = 234,946,560 bytes;
- zero-tail fraction of the declared 16320x12288 plane = 93.7254901961%.

The meaningful-prefix SHA-256 is:

`964cdcce24485f2a5f835ce3575c5dae1d05bc346d64ff2542c51da596423232`

The packed-data-only SHA-256, excluding the 20-byte zero padding of each effective row, is:

`19f6e9ec605003ed035c88d6ab8c9fd7ca9393e328f0bd7960a344150bc3ff99`

## Cross-control interpretation

v0.58 STANDARD RAW10 4080x3072 established the normal Camera-5 row layout:

- rowStride = 5120;
- packed pixel bytes/row = 5100;
- zero row padding = 20;
- 3072 rows;
- total plane bytes = 15,728,640.

v0.57 8160x6144 RAW10 contained the same 4080x3072 / 5120-byte-row layout at the start of a larger envelope.

v0.59 16320x12288 RAW10 now contains that same layout again, followed by an exact zero tail.

The v0.59 meaningful-prefix hash differs from v0.58 and from both v0.57 meaningful-prefix hashes, so this is a fresh capture, not a byte-identical cached payload.

## Classification

Use:

`APP_VISIBLE_4080x3072_PACKED_RAW10_PAYLOAD_EMBEDDED_IN_16320x12288_CAMERA2_ENVELOPE`

Do not use:

- NATIVE_200MP_RAW10_PROVEN
- UNTOUCHED_200MP_ADC
- FULLY_POPULATED_16320x12288_RAW10
- 200MP_SCIENTIFIC_MASTER_SOURCE

The evidence still supports only app-visible Camera2 RAW transport.

## Important correction to the runtime flag

`canonicalContiguousRaw10=true` in the v0.59 JSON means only that:

`rowStride == packed bytes for declared 16320 width`

and

`accessible bytes == rowStride x declared height`.

It does **not** mean that all declared samples are populated.

Future audits must therefore keep **allocation geometry** and **payload population topology** as separate fields.

## Public Camera2 conclusion

The observed Camera-5 public RAW evidence is now mutually consistent across:

- 16320x12288 RAW_SENSOR -> 4080x3072-equivalent populated U16 prefix;
- 8160x6144 RAW_SENSOR -> 4080x3072-equivalent populated U16 prefix;
- 8160x6144 RAW10 -> 4080x3072 standard packed-RAW10 prefix;
- 4080x3072 STANDARD RAW10 control -> native standard packed layout;
- 16320x12288 RAW10 -> the same 4080x3072 standard packed-RAW10 prefix.

Therefore the tested public Camera2 high-resolution RAW route is now considered **practically exhausted for discovering an independently populated 50MP/200MP sample domain** on this firmware.

This does not prove the physical sensor cannot perform a higher-resolution readout. It means such a signal is not exposed as independently populated RAW_SENSOR/RAW10 samples through the tested public route.

## Next research direction

Do not continue increasing public Camera2 RAW envelope sizes or resume blind vendor-key sweeps.

The next acquisition research should move toward the already recovered HONOR/QTI high-pixel/remosaic chain:

`UltraHighPixelMode -> hintUserValue=5 -> qcomRemosaicEnable=1 -> SMART_SCENE_MODE=5 -> pipeline4capbackremosaic.json`

The goal is not to treat the OEM JPEG as TruthRaw evidence. The goal is to identify whether a **pre-render / pre-JPEG high-pixel buffer or metadata surface** can be observed and independently sealed before the vendor presentation path.

Security/package/signature boundaries remain respected. No spoofing or privilege bypass is permitted.

## Vision binding

This result narrows only the Foundation/Measurement ingress.

The larger TruthRaw architecture remains unchanged:

`sealed source evidence -> measurement/de-ISP -> Scientific Master -> Dynamic Authority/uncertainty -> Open Scene / Free Scientific Space -> TruthNegative/projections -> appearance/export`

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
