# TruthRaw LibRaw Android multi-vendor probe v0.1

Date: 2026-09-19

Status: **COMPILE/LINK PROBE ONLY / NOT YET A SCIENTIFIC RAW ADAPTER / NOT YET LINKED INTO THE TRUTHRAW APK**

## Purpose

TruthRaw v0.56/v0.57 already accepts multi-vendor file handles and natively processes the strict DNG route. Canon CR3/CR2, Nikon NEF/NRW, Sony ARW, Fujifilm RAF, Panasonic RW2, ORF, PEF, RWL, 3FR/FFF, IIQ and other proprietary RAW families remain `DECODER_PENDING`.

This probe answers the next prerequisite question:

> Can a pinned upstream LibRaw build be cross-compiled for the same Android arm64/NDK environment used by TruthRaw without changing the scientific house?

No proprietary RAW is admitted to Scientific Master by this probe.

## Pinned upstream identities

LibRaw:

- repository: `LibRaw/LibRaw`;
- release: `0.22.2`;
- release commit: `b93f6e45c194f5df9b02a43b1af9a54b4f41f33f`.

LibRaw CMake support scripts:

- repository: `LibRaw/LibRaw-cmake`;
- pinned commit: `eb98e4325aef2ce85d2eb031c2ff18640ca616d3`.

The CMake support repository is community-maintained/unmaintained by the LibRaw authors. It is used only as a build probe here.

## Probe configuration

Android target:

- ABI: `arm64-v8a`;
- platform: Android 31;
- NDK: `27.2.12479018`.

Optional LibRaw features are intentionally disabled for the first probe:

- OpenMP;
- LCMS;
- Jasper;
- RawSpeed;
- examples;
- X3FTools;
- Raspberry Pi extensions.

The goal is the smallest raw-unpack-capable base library, not appearance processing.

## Scientific boundary

Even if LibRaw compiles successfully, it does not automatically become TruthRaw evidence authority.

Before a proprietary RAW adapter can be promoted it must prove:

1. exact source bytes are sealed before decode;
2. decoder identity/version/options are bound to provenance;
3. unpacked CFA/sample geometry is explicit;
4. margins/crops/black/white/CFA layout are explicit;
5. no demosaic, auto-bright, white-balance rendering, gamma or tone curve feeds the Scientific Master;
6. decoded raw samples are independently checked against reference fixtures/tools;
7. color metadata is admitted through a separate explicit authority path;
8. one physical file remains one evidence observation;
9. decoded CFA is `MEASURED_FROM_SEALED_SOURCE_VIA_VALIDATED_DECODER`, not Direct-CFA sensor ADC proof merely because LibRaw returned samples.

## Licensing boundary

LibRaw 0.22.2 states a choice of LGPL 2.1 or CDDL 1.0 in its distributed headers/licenses.

This probe does not yet vendor or ship LibRaw inside the TruthRaw APK.

Any production integration must preserve required notices/source/license obligations and record the chosen licensing path before release.

## Next gate after compile success

After an Android compile/link PASS:

1. define `RawSourceAdapter v0.1`;
2. expose only `open + unpack` raw-domain state;
3. build one synthetic/reference fixture test;
4. test one real proprietary family first;
5. only then change that format from `DECODER_PENDING` to a versioned native adapter.
