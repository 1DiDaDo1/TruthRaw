# DNG Color Binding Producer v0.1 — failure history

Failures are preserved as failures. None of the scientific/admission gates below were weakened to make CI pass.

## 2026-09-11 — run 34607859757 — strict test-harness warning

Head: `79b0972c66d7e0dd2494b16ad076418bfde59654`

- Contract gate: PASS.
- GCC Release: FAIL during compilation.
- Clang Release: FAIL during compilation.
- Sanitizer did not provide runtime evidence because compilation had not completed.
- Cause: the test fixture helpers used structured bindings by value (`const auto [n,d]`) over initializer-list pairs. With `-Werror`, GCC correctly rejected the unnecessary copies via `-Wrange-loop-construct`.
- Fix: use `const auto& [n,d]` in the fixture helpers.
- Scientific effect: none. DNG transform formulas, source sealing, authority classification and fail-closed rules were unchanged.

## 2026-09-11 — run 34608024699 — Clang dead-constant warning

Head: `a3f1b428f9aa2ff11d21c0ef479aab30b92b6ce2`

- Contract gate: PASS.
- GCC Release: PASS, including execution of the producer tests.
- Clang Release: FAIL during compilation.
- Clang ASan+UBSan: FAIL during compilation before sanitizer runtime execution.
- Cause: two TIFF constants (`kTiffShort`, `kTiffLong`) were declared but not used. Clang 18 reports them as `-Wunused-const-variable`; `-Werror` correctly stopped the build.
- Fix: remove only those two dead constants.
- Scientific effect: none. No test, sanitizer, source-binding or DNG-admission rule was relaxed.

These failures must not be relabeled as successful validation. A later all-green run is separate evidence.