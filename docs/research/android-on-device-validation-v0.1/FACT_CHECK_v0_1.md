# TruthRaw Android On-Device Validation v0.1 — Fact Check

## Proven on host CI

Implementation `b32b7a784f01a4792550e4c47cff70c55c850cd5` passed run `34566632602` with Building Runtime integrity, Technical Backplane integrity, GCC Release, Clang Release and ASan+UBSan.

The host proof covers the measurement contract: session-relative RSS budgeting, bounded telemetry, thermal/background replan signaling, full 180-byte Backplane identity preservation, physical frame count 1, independent evidence count 1, and absence of a scene ISO axis.

## Proven as Android arm64 build/integration

Source commit `74d10a84a02a8019d4a3c31e9e91050b5e1d0354` passed Android build run `34571628677` / job `103174785811`.

Verified by CI:
- Android SDK/target 35, minSdk 31;
- NDK `27.2.12479018`;
- CMake `3.22.1`;
- Gradle 8.9;
- `arm64-v8a` only;
- same native validation core compiled through the NDK;
- JNI bridge packaged at `lib/arm64-v8a/libtruthraw_validation_bridge.so`;
- JNI bridge packaged size 992,968 bytes;
- APK SHA-256 `9e35956667e3d54eda139253c7664ee0d6940155831738e69517602e3f42d6c6`;
- artifact archive digest `950ed07a9057270aa5c042973940e7f7c6d5eb16337037480303f384343ac952`;
- Kotlin `PowerManager.currentThermalStatus` adapter compiled.

This is a build/integration proof. It is **not** evidence that the APK has executed on physical Android hardware.

## Preserved failures

- `02e4ac...` / run `34571063395`: build stopped on an unnecessary AndroidX dependency. Fix: remove the unused dependency.
- `8ea6f426...` / run `34571334332`: Java/Kotlin JVM target mismatch. Fix: align both to 17.
- `74d10a84...` / run `34571628677`: APK build, native-library packaging check and artifact upload PASS.

No truth, ISO, Backplane, memory-admission or reconstruction gate was weakened to obtain the Android build PASS.

## Not proven

Still open are physical-device APK execution, Android RSS/allocator measurements, real thermal behavior, real throughput/throttling, Vulkan behavior, 200 MP processing and production media encoding performance.

## Scientific conclusion

The result promotes only the validation harness from host-only proof to **host-CI plus Android arm64 build/integration proof**. Runtime telemetry remains execution evidence, never scene evidence. The global zero-line and scientific identity remain immutable, and ISO remains absent as a scene axis.
