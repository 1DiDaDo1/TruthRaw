# TruthRaw Android On-Device Validation v0.1 — Failure History

Failures are retained as evidence. They are not rewritten as passes.

## 1. AndroidX configuration failure

- source: `02e4ac569e3f2764838b2e72903d0709e6f0453c`
- workflow run: `34571063395`
- result: APK assemble FAIL
- cause: unused `androidx.core:core-ktx` dependency while AndroidX mode was not enabled
- correction: remove the unused dependency
- scientific/runtime gates relaxed: **no**

## 2. JVM target mismatch

- source: `8ea6f426f1c419ef18ef384597df8d53b52f7024`
- workflow run: `34571334332`
- result: APK assemble FAIL
- cause: Java compile target 1.8 and Kotlin target 17 were inconsistent
- correction: set Java source/target and Kotlin JVM target to 17
- scientific/runtime gates relaxed: **no**

## 3. Android arm64 build/integration pass

- source: `74d10a84a02a8019d4a3c31e9e91050b5e1d0354`
- workflow run: `34571628677`
- job: `103174785811`
- result: **PASS**
- `assembleDebug`: PASS
- arm64 JNI packaging verification: PASS
- artifact upload: PASS
- APK SHA-256: `9e35956667e3d54eda139253c7664ee0d6940155831738e69517602e3f42d6c6`
- physical Android execution: **NOT RUN**

The PASS closes only the Android build/integration boundary. Device-runtime evidence remains open.
