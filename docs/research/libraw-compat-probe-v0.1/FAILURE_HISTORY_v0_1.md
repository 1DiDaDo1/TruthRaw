# Failure history — LibRaw Compatibility Probe v0.1

## F1 — Clang could not find OpenMP header from distro LibRaw build

**Classification:** external dependency/toolchain integration failure before TruthRaw probe compilation completed.

GitHub Actions run `34595384703` installed Ubuntu 24.04 `libraw-dev 0.21.2-2.1ubuntu0.24.04.2`. The metadata-only contract verifier passed and GCC Release passed. Clang Release and Clang ASan/UBSan failed while including the system LibRaw headers because that distro LibRaw build expects `omp.h`, while the Clang runner environment did not yet have the OpenMP development headers installed.

**Resolution:** preserve the failure and install `libomp-dev` for the Clang toolchain. Do not change the probe source, topology rules, evidence boundary, or sanitizer settings.
