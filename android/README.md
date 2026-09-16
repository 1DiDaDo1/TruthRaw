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

## CI signing and build identity

Early v0.2 CI builds used the runner-generated Android debug keystore. The app source was unchanged but the signed APK SHA-256 changed between clean runners, so those APK hashes are build-instance identities rather than reproducible application identities.

The research branch now installs a fixed, explicitly **non-secret CI debug signing key** from `android/ci-debug.keystore.b64` before Gradle runs. This key is for debug/test installation only and must never be used as release signing authority.

The first build using that stable CI key is GitHub Actions run `35068900834`, commit `7ce9ad7df86647957a5bfbeda650831b8d9f6e0e`.

Extracted `app-debug.apk` from that run:

- size: `19675` bytes
- SHA-256: `edda905acc40ab373eff77988148f342cf21b33b04c5e67acec10d93f7169bb9`
- Android package: debug signed APK
- application ID: `io.truthraw.debug`
- minSdk: 26
- targetSdk: 35
- versionCode: 2
- versionName: `0.2-debug`

A subsequent clean CI build is used to test whether the full signed APK is now byte-identical across runners. Reproducibility is not claimed until that comparison passes.

The workflow runs unit tests, assembles the debug APK, hashes it, and uploads both APK and hash as a workflow artifact. The app intentionally uses Android platform APIs only; there is no AndroidX or UI-framework dependency.
