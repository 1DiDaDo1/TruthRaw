# Fact check — LibRaw Compatibility Probe v0.1

## Proven by module design/tests

- topology classification is independent of filename extension;
- DNG classification uses LibRaw's explicit `dng_version` metadata;
- X-Trans (`filters == 9`) is preserved;
- LibRaw special filter codes below 1000 are not silently promoted to Bayer;
- Foveon is preserved as layered topology;
- `raw_count == 0` stays unknown;
- invalid file paths fail explicitly;
- the probe API reports `pixelsUnpacked=false`;
- the source verifier forbids pixel-unpack/process calls in the probe implementation;
- no source ISO value is introduced into scene/topology admission.

## External facts relied upon

LibRaw documents that `open_file()`/`open_datastream()` fill metadata structures, while RAW image buffers are filled by `unpack()`. LibRaw also documents the meanings of `raw_count`, `is_foveon`, `dng_version`, `colors`, `filters`, X-Trans code 9 and special filter codes.

## Not proven

- no real CR3/NEF/ARW/RAF/IIQ file is certified by this module yet;
- no decoded sample equivalence is claimed;
- no full-frame decode memory upper bound is measured;
- no vendor container other than explicit DNG is classified in v0.1;
- no unsupported LibRaw build/SDK feature is promoted to support;
- no Android device behavior is implied.
