# Full-Frame Streaming v0.1 — Preserved Failure / Design History

The following findings are intentionally preserved.

1. **Unused helper compile failure** — the first `-Wall -Wextra -Werror` build rejected two unused rectangle-count helpers. They were deleted; no gate was weakened.
2. **Release assert test-harness failure** — the first local planner harness used `assert()`. Under `-DNDEBUG`, Clang removed the checks and correctly reported a now-unused variable. Tests were changed to always-active requirements.
3. **Misleading-indentation test failure** — compact one-line test helpers triggered `-Werror=misleading-indentation`. The test code was reformatted; production semantics were unchanged.
4. **Superseded half-state-store design** — the initial v0.1 design externalized `halfSceneMax`/`halfCensor` to a half-resolution store. Audit of canonical dependencies showed they are not needed between the two image passes. The final candidate recomputes half-scene state tile-locally in pass 2 and derives censor dilation directly from the RAW halo. This removes potentially large scratch storage rather than merely moving RAM pressure to disk.

5. **Flow-harness unused-parameter compile failure** — the first local corridor-equivalence build failed under `-Werror` because the fake appearance backend had an unused height parameter. The fake test backend was corrected (`/*h*/`); production streaming code and tolerances were unchanged.

These findings are not evidence of pixel-equivalence. Real canonical equivalence remains a GitHub-CI gate for the candidate bytes.
