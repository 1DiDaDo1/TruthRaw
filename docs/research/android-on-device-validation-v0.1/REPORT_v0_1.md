# TruthRaw Android On-Device Validation v0.1 — Report

Status: **RESEARCH_HARNESS_HOST_CI_AND_ANDROID_ARM64_BUILD_PASS_DEVICE_EXECUTION_PENDING**

Branch: `research/android-on-device-validation-v0.1-2026-09-11`

Stacked base: `497bbea144f2e0b30c203aa7c5b3f40c8f47a16e`.

Validated host implementation: `b32b7a784f01a4792550e4c47cff70c55c850cd5`; host run `34566632602` — PASS.

Android arm64 build source: `74d10a84a02a8019d4a3c31e9e91050b5e1d0354`; run `34571628677`; job `103174785811` — PASS.

## Result

The native validation layer remains an observer above Building Runtime v0.1 and Room ABI v0.2. It does not schedule work or modify scene truth.

A separate Android harness now compiles the same native validator using the Android NDK and binds Android thermal status through JNI. The harness has no camera permission and no pixel-processing responsibility.

The successful build produced:
- ABI: `arm64-v8a`;
- JNI library: `lib/arm64-v8a/libtruthraw_validation_bridge.so`;
- packaged JNI size: `992968` bytes;
- APK SHA-256: `9e35956667e3d54eda139253c7664ee0d6940155831738e69517602e3f42d6c6`;
- artifact archive SHA-256: `950ed07a9057270aa5c042973940e7f7c6d5eb16337037480303f384343ac952`.

## Build failure history

The first APK attempt (`02e4ac...`, run `34571063395`) failed before native compilation because an unused AndroidX dependency was present without AndroidX mode. It was removed.

The second (`8ea6f426...`, run `34571334332`) reached Kotlin/Java compilation and failed on JVM-target mismatch. Java and Kotlin were aligned to 17.

The third (`74d10a84a02a8019d4a3c31e9e91050b5e1d0354`, run `34571628677`) passed upstream integrity, SDK/NDK setup, `assembleDebug`, native bridge packaging verification and artifact upload.

No scientific or runtime-admission rule was relaxed in these fixes.

## Host measurement contract remains unchanged

The host fixture still proves session-relative RSS budgeting, bounded histogram/timing aggregation, optional allocator coverage, thermal/background replan signaling, exact Technical Backplane preservation, and explicit absence of a scene ISO axis. Synthetic fixture timing is not Android performance evidence.

## Android adapter boundary

The Android harness proves that:
- the same C++ validation implementation is NDK-buildable;
- the JNI bridge can be packaged for arm64;
- Android thermal status mapping compiles;
- the app can be packaged without camera access.

It does not yet prove that `/proc/self/status` produces the expected values on the target phone, that allocator telemetry is available, or that a TruthRaw workload stays within its admitted budget under real thermal pressure.

## Current proof boundary

`HOST_CI`: PASS.

`ANDROID_ARM64_BUILD_INTEGRATION`: PASS.

`PHYSICAL_ANDROID_DEVICE_EXECUTION`: PENDING.

Next device evidence must record a real session with Backplane before/after identity, baseline/peak RSS, available allocator coverage, thermal state, timing/throughput and runtime-replan behavior while preserving physical frame count 1, independent evidence count 1 and scene ISO axis absent.

No FULL_PHYSICAL or Android-device-performance promotion is made.
