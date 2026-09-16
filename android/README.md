# TruthRaw Android debug client v0.1

Status: **NON-CANONICAL DEBUG CLIENT** on `research/open-world-foundations-v01`.

This first APK is deliberately narrow. It uses Android's Storage Access Framework to select a file, streams the selected bytes through SHA-256, counts the bytes, and admits only the exact frozen source DNG:

- bytes: `25106120`
- SHA-256: `7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67`

When and only when those two source properties match, the client may display the already-frozen research identities bound to that exact source:

- decoded CFA: `883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c`
- Scientific Master: `a86034da7b6f9663640ee3b4478ccf4294d23b720f2b087d083dca38f5fb4640`
- Dynamic Authority Field v1.9: `7678a0b145f8721347cdcc5177fedb720ba19b91bf34984a3f18bd8216408098`
- source-bound P3-D65 transform: `2105712a1be9950089976ba358afd4b062347680d5e6d8c500462c2e8d541533`

## Scientific boundary

This APK does **not** yet recompute the decoded CFA, Scientific Master, Dynamic Authority field or HDR projection on-device. Those values are references, not claims of a new mobile recomputation. A non-matching input inherits none of those identities.

No APK result may modify canonical TruthRaw evidence or promote scientific authority. Presentation output, when it is added later, will remain downstream of immutable source evidence and the Scientific Master / Dynamic Authority chain.

`Representation may exceed the source; knowledge claims may not exceed the evidence.`

## Build

The GitHub workflow `truthraw-android-debug-apk.yml` runs unit tests, assembles the debug APK, writes its SHA-256, and uploads both files as a workflow artifact. The app uses only the Android platform APIs; there is no AndroidX or UI framework dependency.
