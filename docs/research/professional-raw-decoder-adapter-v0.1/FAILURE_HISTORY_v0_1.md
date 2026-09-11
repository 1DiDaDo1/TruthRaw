# Failure history — Professional RAW Decoder Adapter v0.1

## F1 — local sanitizer shadow-memory reservation unavailable

**Classification:** local harness/resource limitation before sanitizer execution.

The constrained local execution environment could compile the Clang ASan/UBSan binary but AddressSanitizer could not reserve its shadow address-space (`ReserveShadowMemoryRange failed`). GCC Release and Clang Release compiled and executed the unchanged candidate successfully.

**Resolution:** do not weaken sanitizer flags. GitHub Actions on `ubuntu-24.04` remains the authoritative ASan/UBSan gate for repository promotion.
