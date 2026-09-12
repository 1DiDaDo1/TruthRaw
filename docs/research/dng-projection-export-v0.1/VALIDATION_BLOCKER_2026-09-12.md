# DNG Projection Export v0.1 — Validation Blocker 2026-09-12

Status: **IMPLEMENTATION COMPLETE — EXECUTION VALIDATION BLOCKED BEFORE RUNNER ASSIGNMENT**

## Scope

This record distinguishes an infrastructure failure from a TruthRaw code/test failure.

The DNG projection implementation and Android integration are present on branch:

`research/dng-projection-export-v0.1-2026-09-12`

The module provides three downstream representations of an already-finalized single-frame lineage:

- `LINEAR_DNG_16_COMPATIBILITY_PROJECTION`;
- `CFA_DNG_16_RECONSTRUCTED_PROJECTION` (rawsensor-style CFA projection);
- TruthRaw-private Scientific Master `.trmaster` float32 transport.

No representation is permitted to create evidence, independent calibration, or `FULL_PHYSICAL` color authority.

## Hosted-runner evidence

### Linux workflow

Workflow: `DNG Projection Export v0.1`

Latest retry run: `34677365672`

Head: `3c63dff16f5df441b1ee6fe9a8f712ffd0918c59`

Observed for GCC Release, Clang Release, Clang ASan/UBSan, and arm64 Android APK jobs:

- `conclusion = failure`;
- `steps = []`;
- `runner_id = 0`;
- `runner_name = ""`;
- no repository checkout, compiler, test, SDK, NDK, Gradle, or APK command executed.

An earlier run (`34663000063`, head `8a4f85ec1d3a5bdcb6badfbed8c5efa1ae4d0157`) shows the same condition on `ubuntu-24.04`: all four jobs have zero executed steps and `runner_id = 0`. It produced no workflow artifacts.

### macOS rescue workflow

Workflow: `DNG Projection Export v0.1 macOS Rescue`

Run: `34677499457`

Head: `6798352302c14fec51450a331985eeb29b7aba0a`

The independent macOS-14 host, sanitizer, and Android rescue jobs also ended before a runner was assigned. No build/test step executed.

## Interpretation

These runs are **not evidence of a compiler error, unit-test failure, sanitizer finding, NDK/JNI failure, or APK-assembly failure**. They are retained as negative infrastructure evidence only.

TruthRaw must not relabel these runs as a research failure or validation pass.

Until a runner actually executes the workflow:

- host compile/test status for this exact DNG-export implementation remains `UNPROVEN`;
- ASan/UBSan status remains `UNPROVEN`;
- arm64 Android APK status remains `UNPROVEN`;
- no validated APK artifact exists for this module;
- physical HONOR export and Lightroom interoperability remain `UNPROVEN`.

## Static authority audit completed

The implementation has nevertheless been statically audited for the scientific boundary:

- export requires finalized phase-2 source/master lineage;
- source/master hashes are checked against admission and Technical Backplane;
- the Scientific Master is reconstructed and re-hashed during export;
- a digest mismatch is fail-closed as `SCIENTIFIC_IDENTITY_MISMATCH`;
- `physicalFrameCount = 1` and `independentEvidenceCount = 1` are required;
- no full-frame Scientific Master is materialized;
- DNG integer projections explicitly clamp/count out-of-range compatibility samples;
- source-bound color does not become independent calibration;
- Android writes through Storage Access Framework descriptors and truncates failed output to zero bytes;
- the current execution path remains CPU/NDK and adds no TruthRaw Vulkan dependency.

This static audit does not substitute for compilation or execution.

## Recovery condition

The blocker is cleared only when at least one hosted or authorized self-hosted runner actually executes the exact-head host tests and Android build. The resulting successful run must then be recorded with exact commit, job conclusions, APK bytes/SHA-256 and artifact digest before physical device claims are made.

Canonical rule remains:

**Measured where measured. Reconstructed where necessary. Never invented.**
