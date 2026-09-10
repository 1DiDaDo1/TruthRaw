# Implementation report — Tile-Native DNG Source v0.1

## Result so far

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

## Promotion blockers

Repository CI must still compile against the actual current:

- canonical v4.7i `core.h` / `core.cpp`;
- Full-Frame Streaming v0.1 source and ABI.

The integration test must show identical exposure, SDR, half-gain and Stage-2 diagnostic output for a synthetic DNG and the corresponding canonical in-memory frame. Until that passes, status remains research candidate.
