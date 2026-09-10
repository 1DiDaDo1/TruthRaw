# Failure history — Tile-Native DNG Source v0.1

Failures are retained; gates were not weakened to obtain PASS.

## F1 — new parser rejected by `-Werror=misleading-indentation`

The first compact implementation placed multiple control statements on single lines. GCC correctly rejected the source under `-Wall -Wextra -Werror` because control flow was visually ambiguous.

Resolution: production code was reformatted/clarified. The warning was not disabled for new code.

## F2 — synthetic OpcodeList2 count became zero

The first fixture builder passed both `op.size()` and `std::move(op)` as arguments of one function call. Argument evaluation order allowed the vector to be moved before its size was observed, producing an OpcodeList2 TIFF count of zero and a correct fail-closed `OpcodeList2 too short` reader error.

Resolution: capture the opcode byte count in a scalar before moving the vector. The reader was not relaxed.

## 3. Manifest reseal after source split

The first promotion-candidate manifest still named the former monolithic source hash after the reader was split into common/bind/read translation units. Integrity verification rejected that stale seal. The verifier was not weakened; the manifest is regenerated from the final split bytes.
