# Android Source-Bound Color Preview v0.1 — Failure History

Failure history is immutable evidence. A failed run remains a failed run after later fixes.

## Failure 1 — arm64 NDK strict-warning incompatibility in frozen canonical v4.7i source

- Date: 2026-09-11
- Candidate commit: `3ea45d9e65a3281b862378e9affefec643b4af19`
- Workflow run: `34612805148`
- Job: `build-arm64-apk` (`103307239253`)
- Result: **FAIL**

### What passed before the failure

- Android source-bound preview contract: PASS
- Building Runtime v0.1 integrity: PASS
- Technical Backplane v0.1 integrity: PASS
- Tile-Native DNG Source v0.1 integrity: PASS
- Adaptive UI ingress contract: PASS
- source/color authority boundary: PASS
- no Android camera permission: PASS
- Kotlin compilation: PASS (only the already-known `setDecorFitsSystemWindows` deprecation warning)
- new JNI bridge translation unit: compiled
- Tile-Native DNG translation units: compiled
- Full-Frame Streaming translation units: compiled
- reconstructed-color bounded preview sink: compiled
- Scientific Preview Source Binding v0.1/v0.2: compiled
- DNG Color Binding Producer v0.1: compiled
- Technical Backplane v0.1: compiled

### Exact failure

Android NDK Clang 18.1.8 compiled the frozen canonical file:

`canonical/reconstruction/v4.7i/native/src/core.cpp`

with the integration target's strict `-Wall -Wextra -Werror` flags and rejected line 196:

`error: misleading indentation; statement is not part of the previous 'for' [-Werror,-Wmisleading-indentation]`

The compact canonical bytes contain `}return Status::ok();` after nested compact loops. This was a compiler/style diagnostic, not a failed scientific assertion or runtime/sanitizer finding.

### Classification

`FROZEN_CANONICAL_V47I_ANDROID_CLANG_STYLE_WARNING`

The canonical v4.7i bytes must remain unchanged. The repair therefore must not reformat or modify `core.cpp` and must not globally disable `-Werror`. A narrowly source-scoped suppression for this one known diagnostic on the frozen canonical translation unit is permitted as a toolchain-compatibility adapter, while all integration-owned and research-module sources remain under strict `-Wall -Wextra -Werror`.

No evidence, color-authority, memory, frame/evidence, Backplane, reconstruction, or appearance gate may be weakened by the repair.
