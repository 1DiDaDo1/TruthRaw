# TruthRaw LibRaw Compatibility Probe v0.1

Status: **RESEARCH CANDIDATE — METADATA/TOPOLOGY PROBE ONLY; NO RAW UNPACK/SCIENTIFIC SAMPLE ADMISSION**

This optional compatibility layer uses LibRaw only to recognize/probe professional RAW sources before a decoder adapter is allowed to produce scientific samples.

## Scope

The probe calls `LibRaw::open_file()` and reads metadata already populated by that operation. It does **not** call LibRaw `unpack()`, `raw2image()`, `dcraw_process()` or another pixel materialization path.

Reported fields include:

- LibRaw library version and decoder name;
- camera make/model and normalized make/model;
- DNG version;
- `raw_count`;
- color count and LibRaw CFA `filters` code;
- raw and visible dimensions;
- topology classification;
- whether an explicit frame selection will be required.

## Topology rules

- nonzero `is_foveon` -> `LayeredFoveon`;
- `filters == 9` -> `XTrans6x6`;
- `filters == 0`, one color -> `Monochrome`;
- `filters == 0`, 3+ colors -> `LinearRgb`;
- `filters >= 1000` and exactly three colors -> `Bayer2x2`;
- other special filter codes remain `Unknown`.

The probe never converts X-Trans, Foveon, Leaf/special CFA or full-color data into Bayer merely to fit v4.7i.

## Container policy

`dng_version != 0` is sufficient to classify the container family as DNG because LibRaw exposes that fact explicitly. Other vendor container families remain `Unknown` in v0.1 until a separate signature/container router is certified. Filename extension is never used as evidence.

## Memory policy

This probe is deliberately pre-decode. It reports an informational `raw_sample_floor_bytes()` value to show the scale of a hypothetical uint16 sample materialization, but that number is **not** a decoder resident-memory upper bound.

A later LibRaw decode adapter must separately report its real `residentUpperBoundBytes`, `scratchUpperBoundBytes` and `FullFrameMaterialized` behavior to Professional RAW Decoder Adapter v0.1.

## Scientific boundary

Successful LibRaw recognition means only: `decoder capability known`.

It does **not** mean:

- samples are lossless-equivalent to stored measurement values;
- the file is single-frame evidence;
- the source topology is supported downstream;
- color calibration is authoritative;
- the file may enter the single-frame Scientific Scene Master.

Those decisions remain in Professional RAW Ingress / Decoder Adapter.

## Dependency and licensing

LibRaw is an optional external dependency and is not vendored by this research module. Distribution/packaging must make an explicit license choice compatible with LibRaw's upstream licensing. TruthRaw does not add or change its own repository license through this module.
