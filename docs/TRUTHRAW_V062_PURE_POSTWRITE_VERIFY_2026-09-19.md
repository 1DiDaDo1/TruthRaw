# TruthRaw v0.62 — PURE post-write verified DNG

Date: 2026-09-19

Status: **IMPLEMENTED / HOST GCC + CLANG + ANDROID CI GREEN / APK VERIFIED**

Active branch:

`integration/truthraw-suite-v0-62-pure-postwrite-verify`

App version:

`0.27-v0.62-pure-postwrite-verify`

## Why v0.62 exists

A real-device check exposed an ambiguity that code-level CI alone could not exclude: the installed app reported the v0.61 package version, while the inspected exported DNG still carried the older v0.60 private role. The launcher also still displayed a hard-coded v0.60 label.

v0.62 closes both gaps.

The v0.61 scientific writer contract remains unchanged:

`TRUTHRAW_PURE_SELF_BINDING_V0_61`

The new v0.62 behavior is an **artifact-level post-write gate**. After the native writer commits and closes the output, Android re-opens the exact destination URI and verifies the saved DNG itself before reporting success.

## Post-write gate

The saved artifact must contain the current PURE role and v0.61 self-binding markers, including source/master hashes, Zero-Line/L0, scene-scale, exact Technical Backplane serialization, CRC32, precision policy and runtime reconstruction identity.

The verifier additionally requires:

- SHA-256 values are exactly 64 hexadecimal characters;
- `zero_line_l0_f64_bits` is exactly one 64-bit hexadecimal payload;
- serialized Technical Backplane is exactly 180 bytes / 360 hexadecimal characters;
- Backplane CRC32 has the expected 32-bit hexadecimal shape;
- precision and runtime provenance are non-empty;
- frame/evidence remain 1/1;
- the old `LINEAR_DNG_XYZ_D50_COMPATIBILITY_PROJECTION` marker is absent.

If post-write verification fails, the app returns fail-closed, attempts to delete the destination, and **does not report the DNG as successfully saved**.

## User-visible disambiguation

The launcher now identifies the active line as v0.62.

New PURE exports default to:

`*_truthraw_pure_float32_v0_62.dng`

A successful export message explicitly includes:

`self-binding verified=true`

This separates a newly verified v0.62 artifact from older v0.60/v0.61 test files.

## Scientific scope

v0.62 does not change PURE pixel mathematics, Scientific Master reconstruction, Zero-Line semantics, scene-scale semantics or the frozen 180-byte Technical Backplane. It adds an independent read-after-write verification boundary around the already existing v0.61 self-binding writer.

## CI and APK validation

v0.62 workflow run `35466767939` passed:

- host GCC: **SUCCESS**;
- host Clang: **SUCCESS**;
- Android arm64: **SUCCESS**;
- artifact ID: `10591985003`;
- artifact name: `truthraw-suite-v0-62-pure-postwrite-verify-debug-arm64`;
- artifact ZIP SHA-256: `04a40760a8d234fceb1e59d55a70c2324ecddc32d387c778bd02dff02a7b280f`;
- extracted APK bytes: `5850141`;
- extracted APK SHA-256: `36e468a8a7e5449d6c649006a109f3a9163dbd5f3f746ed0c7cf521a4f88c447`.

Independent APK inspection confirmed the Kotlin-side v0.62 filename/post-write-success markers and the native v0.61 self-binding writer markers for PURE role, exact L0 and serialized Technical Backplane.

The remaining gate is now physical: generate a new v0.62 DNG on-device, require `self-binding verified=true`, then independently inspect that exact saved file.
