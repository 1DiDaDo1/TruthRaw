# Failure history — Professional RAW Decoder Adapter v0.1

## F1 — local sanitizer shadow-memory reservation unavailable

**Classification:** local harness/resource limitation before sanitizer execution.

The constrained local execution environment could compile the Clang ASan/UBSan binary but AddressSanitizer could not reserve its shadow address-space (`ReserveShadowMemoryRange failed`). GCC Release and Clang Release compiled and executed the unchanged candidate successfully.

**Resolution:** do not weaken sanitizer flags. GitHub Actions on `ubuntu-24.04` remains the authoritative ASan/UBSan gate for repository promotion.

## F2 — first repository seal mismatch

**Classification:** repository byte-seal failure before compilation.

GitHub Actions run `34594450853` passed checkout but stopped in `Verify sealed module`. The repository bytes for `CMakeLists.txt` and `tests/test_professional_raw_decoder_adapter_v0_1.cpp` differed from the locally generated manifest hashes. No compiler or sanitizer step ran, and no scientific/admission gate was weakened.

**Resolution:** preserve the failure, bind the manifest to the exact committed repository bytes, and rerun the unchanged compiler/sanitizer matrix.
