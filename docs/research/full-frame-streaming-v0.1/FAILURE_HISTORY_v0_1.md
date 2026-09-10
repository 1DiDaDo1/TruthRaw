# Full-Frame Streaming v0.1 — Preserved Failure / Design History

The following findings are intentionally preserved.

1. **Unused helper compile failure** — the first `-Wall -Wextra -Werror` build rejected two unused rectangle-count helpers. They were deleted; no gate was weakened.
2. **Release assert test-harness failure** — the first local planner harness used `assert()`. Under `-DNDEBUG`, Clang removed the checks and correctly reported a now-unused variable. Tests were changed to always-active requirements.
3. **Misleading-indentation test failure** — compact one-line test helpers triggered `-Werror=misleading-indentation`. The test code was reformatted; production semantics were unchanged.
4. **Superseded half-state-store design** — the initial v0.1 design externalized `halfSceneMax`/`halfCensor` to a half-resolution store. Audit of canonical dependencies showed they are not needed between the two image passes. The final candidate recomputes half-scene state tile-locally in pass 2 and derives censor dilation directly from the RAW halo. This removes potentially large scratch storage rather than merely moving RAM pressure to disk.

5. **Flow-harness unused-parameter compile failure** — the first local corridor-equivalence build failed under `-Werror` because the fake appearance backend had an unused height parameter. The fake test backend was corrected (`/*h*/`); production streaming code and tolerances were unchanged.

These findings are not evidence of pixel-equivalence. Real canonical equivalence remains a GitHub-CI gate for the candidate bytes.

## F6 — Staging blob byte-transcription mismatch

A manually transcribed upload of `native/full_frame_streaming_v0_1_internal.h` produced Git blob `87ac4625...` instead of the locally sealed `a29c0b27...` because explanatory comment lines were omitted during transfer. The blob was rejected before it entered any candidate tree. The exact local bytes were re-uploaded and matched `a29c0b270110d81dbdf33619525c186fe4db5afb`.

## F7 — GitHub Clang Release blocked by frozen upstream warning

First real-repository CI run `34527663822` reached the exact canonical v4.7i source. GCC Release compiled and passed the canonical equivalence executable, while Clang 18.1.3 stopped before execution on the pre-existing `-Wmisleading-indentation` diagnostic at frozen `canonical/reconstruction/v4.7i/native/src/core.cpp:196`.

This is a CI-harness/upstream-warning compatibility failure, not streaming pixel evidence. Canonical v4.7i is not edited. The corrective CMake structure compiles the frozen upstream source as a separate object; new streaming/test sources retain strict `-Werror`, while Clang receives only `-Wno-error=misleading-indentation` for that frozen upstream object. The warning remains visible.


## F8 — Final-clean manifest root/module path mismatch

Clean candidate `102a994c9878409e9c1f1bcc28f326d9f096d223`, GitHub Actions run `34528617225`, failed at the sealed-module verifier before any compiler/equivalence step. The generated module manifest incorrectly included `.github/workflows/full-frame-streaming-v0-1-integrity.yml` as if it were relative to the module directory, while the workflow correctly lives at repository root. The verifier therefore reported `missing:.github/workflows/full-frame-streaming-v0-1-integrity.yml`.

The workflow was already independently SHA-256-bound by `verify_integrity_v0_1.py`; duplicating it in the module-local manifest was both redundant and path-wrong. The fix removes only that manifest entry. Workflow hash binding remains unchanged. No streaming algorithm, tolerance, canonical v4.7i byte, or scientific claim is changed.
