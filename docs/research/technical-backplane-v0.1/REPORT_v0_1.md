# Technical Backplane v0.1 — Engineering Report

## Decision before repository CI

`LOCAL_PASS_FIXED_180_BYTE_FORMAT_ZERO_LINE_SINGLE_BINDING_REPOSITORY_CI_OPEN`

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
