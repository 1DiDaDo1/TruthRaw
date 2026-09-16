# TruthRaw Android debug client v0.2

Status: **NON-CANONICAL DEBUG CLIENT** on `research/open-world-foundations-v01`.

This APK remains deliberately narrow and fail-closed. It uses Android's Storage Access Framework to select a file, streams the selected bytes through SHA-256, counts the bytes, and admits only the exact frozen source DNG:

- bytes: `25106120`
- SHA-256: `7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67`

For that exact source, v0.2 then copies the selected DNG to private cache and independently decodes the supported uncompressed 16-bit CFA payload on-device. Admission passes only if the decoded raster is 4080×3072 and hashes to:

- decoded CFA SHA-256: `883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c`

Only after both source-byte admission and decoded-CFA verification pass may the client display the already-frozen downstream research identities bound to that exact source:

- Scientific Master: `a86034da7b6f9663640ee3b4478ccf4294d23b720f2b087d083dca38f5fb4640`
- Dynamic Authority Field v1.9: `7678a0b145f8721347cdcc5177fedb720ba19b91bf34984a3f18bd8216408098`
- source-bound P3-D65 transform: `2105712a1be9950089976ba358afd4b062347680d5e6d8c500462c2e8d541533`

## Scientific boundary

Implemented on-device in v0.2:

- exact file byte-count + SHA-256 admission;
- supported DNG parsing for the frozen source structure;
- deterministic decoded-CFA SHA-256 verification.

Not yet recomputed on-device:

- Scientific Master;
- Dynamic Authority field;
- TruthRaw HDR projection / P3-D65/PQ transport.

Those later identities remain frozen references, not claims of a new mobile recomputation. A non-matching input inherits none of them. No APK result may modify canonical TruthRaw evidence or promote scientific authority.

`Representation may exceed the source; knowledge claims may not exceed the evidence.`

## Validated debug build

GitHub Actions run `35068164683` completed successfully for commit `1c0d2b8b61de4abf2c88c1007fcf4b31e7717744` and produced artifact `truthraw-debug-apk-v0.2`.

Extracted `app-debug.apk`:

- size: `19675` bytes
- SHA-256: `34922b434404bcada94f7c9e2e2cb139cc970870fb00b4ee9fe2c90a47d5518d`
- Android package: debug signed APK
- application ID: `io.truthraw.debug`
- minSdk: 26
- targetSdk: 35
- versionCode: 2
- versionName: `0.2-debug`

The workflow runs unit tests, assembles the debug APK, hashes it, and uploads both APK and hash as a workflow artifact. The app intentionally uses Android platform APIs only; there is no AndroidX or UI-framework dependency.
