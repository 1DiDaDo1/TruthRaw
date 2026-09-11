# LibRaw Compatibility Probe v0.1 — Engineering Report

## Decision target

`OPTIONAL_PROBE_ONLY_EXTERNAL_COMPATIBILITY_LAYER_NO_PIXEL_UNPACK`

## Placement

`sealed source handle -> LibRaw metadata probe -> topology/capability descriptor -> Professional RAW Ingress/Decoder Adapter -> later explicit decode decision`

The probe sits before sample decoding and has no authority to create or relabel measurement evidence.

## Why probe first

Broad professional RAW recognition is useful for UI/preflight and decoder routing, but immediately calling a full-frame decoder would conflict with TruthRaw's low-memory baseline. Metadata-only probing separates compatibility discovery from expensive sample materialization.

## Memory interpretation

For a 16320x12288 Bayer source, a simple uint16 one-sample-per-pixel materialization is 401080320 bytes. The module reports this as an informational scale/floor only, not as a bound on LibRaw or total process RSS.

## Promotion boundary

This module can become a compatibility-probe candidate after repository CI proves:

- system LibRaw can build/link the probe;
- source integrity verifier passes;
- GCC and Clang tests pass;
- ASan/UBSan passes;
- no forbidden unpack/pixel-processing call exists in the production probe source.

Actual RAW decoding remains a separate future adapter and requires named real-file corpus evidence.
