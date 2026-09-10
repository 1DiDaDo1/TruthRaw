# Implementation report — Tile-Native DNG Source v0.1

## Result

A strict, bounded, random-access DNG/TIFF CFA source has been implemented as the front end of Full-Frame Streaming v0.1.

The final candidate is split into TIFF/common parsing, binding/open, and raw-read translation units without changing the public ABI. This split is an auditability refactor; the unit corpus and scientific semantics are unchanged.

Local source-level gates:

- GCC C++17 Release + `-Wall -Wextra -Werror`: PASS
- Clang C++17 Release + `-Wall -Wextra -Werror`: PASS
- Clang ASan/UBSan: PASS

The unit suite covers endian handling, strips, TIFF tiles, phase CFA mapping, BlackLevel/WhiteLevel, NoiseProfile, phase GainMap, POSIX pread, malformed locators, ambiguity and resource caps.

## Memory result

The source keeps small IFD/tag descriptors and compact gain map state. It does not load the complete DNG or complete RAW. Strip/tile locator arrays remain lazy.

For a synthetic 16320x12288 single-strip file, the steady source state after open is 4989 bytes and a 64x64 request transfers only 8192 RAW bytes. The synthetic byte source makes the remaining ~401 MB RAW region virtual so the test itself does not defeat the ownership test. This 4989-byte number is **not** the `open()` peak: temporary IFD discovery and OpcodeList2 buffers are bounded by explicit caps but are not yet separately peak-instrumented in v0.1.

## Scientific result

The reader is a transport/metadata-binding room, not a reconstruction room. It does not alter CFA values. GainMap is returned separately for the existing Stage-2 application. Color is an explicit external binding rather than an inferred identity/default.

## Repository integration result

Candidate commit `a47518c92c35e89f35b32516914de6dd04e08661` was tested by workflow run `34534894457` against the actual repository's byte-bound canonical v4.7i and Full-Frame Streaming v0.1 dependencies.

Repository gates:

- GCC Release: PASS
- Clang Release: PASS
- Clang ASan/UBSan: PASS
- sealed module/upstream-byte binding: PASS on all three matrix jobs
- Documentation Governance run `34534894476`: PASS

The controlled end-to-end fixture produced:

- `sdr_max_abs=0`
- `half_gain_max_abs=0`
- `stage2_diag_max_abs=0`
- `source_resident_bytes=8516`
- `raw_payload_bytes_read=40716`

This establishes exact numerical equivalence for the tested fixture, not universal DNG compatibility or Android device performance.

The preceding failed workflow run `34534705681` is retained in `FAILURE_HISTORY_v0_1.md`; its reader build and integrity gate passed, while the integration translation unit failed under `-Werror=unused-function`. The gate was not weakened.

## Remaining promotion gate

Create a new single-commit research branch directly from unchanged `main` and re-run Tile-Native DNG Source v0.1 Integrity plus Documentation Governance on those exact final bytes. Only a fully green clean branch is eligible for a non-force fast-forward promotion.
