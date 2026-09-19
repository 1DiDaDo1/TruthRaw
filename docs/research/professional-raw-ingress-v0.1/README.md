# TruthRaw Professional RAW Ingress / Decoder ABI v0.1

Status: **RESEARCH CANDIDATE — FORMAT ROUTING / EVIDENCE ADMISSION CONTRACT; NO UNIVERSAL CODEC CLAIM**

TruthRaw must not equate “a RAW file can be opened” with “the original sensor measurement is scientifically understood.” This module separates three independent questions:

1. **Container / format recognition** — what family and codec variant is this file?
2. **Decode certification** — is the byte-to-sample decode path actually verified for this variant?
3. **Scientific admission** — what kind of measurement is present, and may it enter the single-frame Direct-CFA scientific master?

The original source file remains sealed evidence regardless of decoder path. A converted DNG, unpacked buffer, or compatibility representation never replaces the original source object as evidence.

## Why this layer exists

The current Tile-Native DNG Source v0.1 intentionally supports only a strict DNG/TIFF subset. Professional still cameras use many additional families and sensor topologies, including CR2/CR3, NEF/NRW, ARW/SRF/SR2, RAF, RW2, ORF/ORI, PEF, 3FR/FFF, IIQ, MOS/MEF and X3F. A single filename extension is not a sufficient scientific description: files can differ by compression variant, bit packing, CFA topology, in-camera preprocessing, multi-shot composition, or computational processing.

TruthRaw therefore expands by **certified decoder adapters around the existing native low-memory DNG route**, not by weakening that route into a permissive generic reader.

## Core contract

### Archivist

The Archivist owns the immutable original container bytes and source-evidence identity. Decoder output is derived from that sealed object; it is never allowed to overwrite or replace it.

### Measurement Lab

The Measurement Lab owns format/codec classification and a decoder-adapter handle. It records decoder identity/version/build binding, camera make/model when available, codec variant, source bit depth, topology, compression semantics, and an explicit resource profile.

### Scientific admission

Admission is based on decoded measurement semantics, not on extension.

Current v0.1 evidence classes are:

- `DIRECT_NATIVE_CERTIFIED`
- `LOSSLESS_DECODED_CERTIFIED`
- `DERIVED_RAW_SUPPORTED`
- `COMPUTATIONAL_RAW`
- `MULTI_CAPTURE_RAW`
- `RESEARCH_ONLY`
- `FAIL_CLOSED_UNSUPPORTED`

Only a verified single physical frame + single independent evidence source with a certified downstream topology and a verified uncompressed/lossless 2x2 Bayer decode may enter the current single-frame Direct-CFA scientific master through this v0.1 contract.

`COMPUTATIONAL_RAW` and `MULTI_CAPTURE_RAW` are explicitly separated from Direct-CFA admission. Their decoded data may be useful, but they do not retroactively become original single-frame CFA evidence.

X-Trans, monochrome, layered/Foveon, linear-RGB and other topologies require their own certified downstream scientific path before they can leave `RESEARCH_ONLY`; decoding alone is not sufficient.

## Decoder provenance

Each adapter must expose a stable provenance record containing at least:

- adapter ID;
- adapter version;
- build/hash binding;
- camera make/model when known;
- codec variant;
- source bit depth;
- whether the byte-to-sample path is independently verified.

This provenance belongs behind an immutable handle/registry binding. It does **not** enlarge the fixed Technical Backplane v0.1 record and does not create new sensor evidence.

## Resource contract

Every decoder adapter reports its execution memory shape independently from scientific authority:

- random-access tile;
- bounded storage-unit decode;
- full-frame materialization;
- resident upper bound;
- scratch upper bound;
- random-access capability;
- full-frame-materialization requirement.

A low-memory phone may reject an adapter whose declared lease cannot fit. A flagship may admit it. If both execute the same certified decode, the evidence class and scientific authority are identical. Hardware capability changes availability/throughput, not truth.

The preferred long-term route for professional RAW is streaming/tile-native decoding so weak and strong devices can reach the same certified result under different resource envelopes. Full-frame third-party decoders are compatibility bridges, not the desired low-memory endpoint.

## Current format-family policy

`state/FORMAT_FAMILY_POLICY_v0_1.json` is a target registry, not a supported-camera marketing list.

- strict DNG subset: current native low-memory source exists separately in Tile-Native DNG Source v0.1;
- Canon CR2/CR3: adapter target, not implemented here;
- Nikon NEF/NRW: adapter target, not implemented here;
- Sony ARW/SRF/SR2: adapter target, not implemented here;
- Fujifilm RAF: adapter target; Bayer and X-Trans must not be conflated;
- Panasonic RW2: adapter target;
- Olympus/OM ORF/ORI: adapter target; composite/high-resolution products require multiplicity classification;
- Pentax PEF: adapter target;
- Hasselblad 3FR/FFF: adapter target;
- Phase One IIQ and Leaf MOS/MEF: adapter targets;
- Sigma X3F: separate layered topology target;
- cinema RAW/container families: outside this still-photo v0.1 scope.

No entry in this list means that every model/firmware/compression variant in that family is already certified.

## Room ABI v0.2 integration

Room ABI v0.2 should add a borrowed immutable **decoder provenance handle** and account decoder source/scratch residency in global admission. The Measurement Lab owns the active decoder lease; the Archivist owns the sealed source identity. The Technical Backplane remains one shared fixed record and must not be duplicated per tile or expanded merely to carry verbose decoder metadata.

The decoder resource profile is evaluated on the resource axis. The ingress/evidence class is evaluated on the scientific-authority axis. These axes must never be merged.

## Validation in this candidate

The native policy prototype verifies:

- unknown format/topology/codec fails closed;
- extension/container identity alone cannot certify evidence;
- native verified uncompressed Bayer can reach `DIRECT_NATIVE_CERTIFIED`;
- certified external lossless Bayer can reach `LOSSLESS_DECODED_CERTIFIED` without being relabelled native;
- X-Trans without a certified downstream topology remains `RESEARCH_ONLY`;
- computational RAW cannot enter Direct-CFA;
- multi-shot composite RAW cannot enter Direct-CFA;
- an unverified decoder byte path remains `RESEARCH_ONLY`;
- inconsistent decoder memory declarations fail validation.

GCC Release, Clang Release and Clang ASan/UBSan passed locally before repository staging.
