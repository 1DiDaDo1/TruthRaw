# TruthRaw Android On-Device Validation v0.1

Status: **RESEARCH_HARNESS_HOST_CI_AND_ANDROID_ARM64_BUILD_PASS_DEVICE_EXECUTION_PENDING**

Validated host implementation: `b32b7a784f01a4792550e4c47cff70c55c850cd5`.

Host validation: GitHub Actions run `34566632602` — **PASS** under GCC Release, Clang Release and ASan+UBSan after Building Runtime v0.1 and Technical Backplane v0.1 integrity passed.

Android arm64 build/integration proof: source commit `74d10a84a02a8019d4a3c31e9e91050b5e1d0354`, run `34571628677`, job `103174785811` — **PASS**. The debug APK contains `lib/arm64-v8a/libtruthraw_validation_bridge.so`; APK SHA-256 is `9e35956667e3d54eda139253c7664ee0d6940155831738e69517602e3f42d6c6`.

Stacked base: Room ABI v0.2 branch state `497bbea144f2e0b30c203aa7c5b3f40c8f47a16e`.

## Purpose

This module observes whether an already-admitted TruthRaw execution behaves within its mobile runtime envelope. It is a validator, not a second scheduler and not a reconstruction component.

Building Runtime v0.1 remains resource-policy authority. Room ABI v0.2 remains whole-house admission authority. The validator may never alter Direct-CFA evidence, scientific master, global zero-line, truth authority, canonical v4.7i, or introduce a scene ISO axis.

## Host contract

The native contract binds an already valid `AdaptiveAllRoomPlan` to the exact 180-byte Technical Backplane. It measures session-relative `VmRSS`, records `VmHWM` as context, accepts optional allocator telemetry, aggregates bounded timing data, observes Building Runtime `ThermalState`, and byte-compares the complete Backplane at finalization.

The host fixture proves the fail-closed measurement logic only; its timing values are synthetic and are not phone-performance measurements.

## Android arm64 integration

A separate validation harness exists at `capture/android/on-device-validation-v01/`. It intentionally has no camera permission and does not capture or reconstruct pixels.

The harness:
- targets Android SDK 35 / minSdk 31;
- builds only `arm64-v8a`;
- pins NDK `27.2.12479018` and CMake `3.22.1`;
- compiles Java/Kotlin for JVM 17;
- maps `PowerManager.currentThermalStatus` conservatively onto the existing Building Runtime thermal enum;
- loads the same native validation core through JNI;
- exposes `/proc/self/status` memory probing without creating a second resource policy;
- keeps `sceneIsoAxisPresent=false` and forbids scientific-state mutation.

The successful CI build packaged `lib/arm64-v8a/libtruthraw_validation_bridge.so` (992,968 bytes) and produced APK SHA-256 `9e35956667e3d54eda139253c7664ee0d6940155831738e69517602e3f42d6c6`. This proves **Android arm64 build/integration**, not physical-device execution.

## Preserved build history

Two failed Android build attempts remain part of the evidence:
1. `02e4ac569e3f2764838b2e72903d0709e6f0453c`, run `34571063395`: unused AndroidX dependency without AndroidX mode. Fixed by removing the unused dependency; no scientific/runtime gate changed.
2. `8ea6f426f1c419ef18ef384597df8d53b52f7024`, run `34571334332`: Java 1.8 versus Kotlin 17 target mismatch. Fixed by aligning both to 17; no scientific/runtime gate changed.

The next source head `74d10a84a02a8019d4a3c31e9e91050b5e1d0354` passed the complete arm64 APK build/package/upload gate.

## Proof boundary

Proven:
- host parser/recorder/fail-closed contract;
- live host `/proc/self/status` access;
- Android SDK/NDK arm64 compilation of the same native validator;
- Kotlin thermal adapter compilation;
- JNI bridge packaging in an APK;
- no scene ISO axis in the validation contract.

Still **not proven**:
- APK launch on a physical Android device;
- Android `/proc/self/status` behavior on the target phone;
- real Android RSS/allocator traces;
- real thermal transitions during TruthRaw processing;
- sustained throughput/throttling;
- Vulkan device behavior;
- real 200 MP workload;
- production DNG/JPEG/HEIF/AVIF encoder performance.

Until device execution evidence exists, this module must not be described as Android-device-performance validated.
