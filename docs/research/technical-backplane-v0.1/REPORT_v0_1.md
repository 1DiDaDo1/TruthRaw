# Technical Backplane v0.1 — Engineering Report

## Decision

`REPOSITORY_CI_PASS_FIXED_180_BYTE_FORMAT_ZERO_LINE_SINGLE_BINDING`

## Layout

Offsets are explicit and stable:

- 0..7: magic `TRBACK01`
- 8..9: version (LE u16)
- 10..11: serialized size (LE u16)
- 12..15: forbidden flags (LE u32; valid v0.1 requires zero)
- 16..47: source-evidence SHA-256 binding
- 48..79: scientific-master SHA-256 binding
- 80..111: zero-line SHA-256 binding
- 112..143: scene-scale SHA-256 binding
- 144..147: physical frame count
- 148..151: independent evidence count
- 152..163: 12 room-status bytes
- 164: claim-status byte
- 165..175: reserved zero bytes
- 176..179: CRC32 over bytes 0..175

No C++ object representation is persisted directly.

## Local validation

GCC Release, Clang Release and Clang ASan/UBSan: PASS with byte-identical output.

Frozen metrics:

- serialized bytes: 180
- zero-line fixture occurrences: 1
- physical frame count: 1
- independent evidence count: 1
- deterministic fixture CRC32: 1570024631

## Failure preservation

The first launch harness deleted/recreated its own current working directory before compiling. The source files were created correctly, but relative compiler paths resolved from the detached old cwd and the run failed before compilation. The same untouched source bytes passed from a stable parent directory. This is classified as a tooling/workdir failure, not module evidence.

## Next integration

Room ABI v0.2 should pass this backplane by one external handle/reference and validate its scientific invariants at corridor entry. It should not copy the 180-byte record into every tile or room. Tile-Native DNG Source and Full-Frame Streaming source/sink resident bounds can then be accounted under per-room profiles without changing scientific algorithms.


## Repository validation

GitHub Actions run `34539553224` on staged candidate `958528e47e5d24aa76f1567c9d9c4e09fa4338d6`: GCC Release PASS, Clang Release PASS, Clang ASan/UBSan PASS. The sealed-module verifier passed before each build. Runtime metrics matched the local fixture exactly.
