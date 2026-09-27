# LibRaw Compatibility Probe v0.1 — Engineering Report

## Decision

`REPOSITORY_CI_PASS_OPTIONAL_PROBE_ONLY_EXTERNAL_COMPATIBILITY_LAYER_NO_PIXEL_UNPACK`

## Placement

`sealed source handle -> LibRaw metadata probe -> topology/capability descriptor -> Professional RAW Ingress/Decoder Adapter -> later explicit decode decision`

The probe sits before sample decoding and has no authority to create or relabel measurement evidence.

## Why probe first

Broad professional RAW recognition is useful for UI/preflight and decoder routing, but immediately calling a full-frame decoder would conflict with TruthRaw's low-memory baseline. Metadata-only probing separates compatibility discovery from expensive sample materialization.

## Memory interpretation

For a 16320x12288 Bayer source, a simple uint16 one-sample-per-pixel materialization is 401080320 bytes. The module reports this as an informational scale/floor only, not as a bound on LibRaw or total process RSS.

## Repository validation

Validated implementation head: `1d6a165cad53bfb8257cfb764ee63c4c5c5819d9`.

GitHub Actions run `34595504942`:

- LibRaw system dependency install: PASS;
- metadata-only contract verifier: PASS;
- GCC Release: PASS;
- Clang Release: PASS;
- Clang ASan/UBSan: PASS.

Tested system library: `LibRaw 0.21.2-Release`; reported camera count: `1182`.

The prior run `34595384703` is retained as a toolchain/dependency failure because Clang lacked `omp.h` for the distro LibRaw build. Installing `libomp-dev` fixed the toolchain without changing probe semantics or sanitizer settings.

## Next implementation

Provide an Android/Linux borrowed-file-descriptor `LibRaw_abstract_datastream` so URI/file-descriptor based ingress can use the same metadata probe without first copying the complete professional RAW to an app-private path. Pixel `unpack()` remains outside that step.
