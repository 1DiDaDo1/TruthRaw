# TruthRaw Android debug client v0.3

Status: **NON-CANONICAL DEBUG CLIENT** on `research/open-world-foundations-v01`.

This APK is deliberately narrow and fail-closed. It is a device-side validation/integration client; APK behavior never defines, calibrates or upgrades TruthRaw scientific evidence.

## Implemented on-device

v0.3 uses Android's Storage Access Framework to select a DNG, streams the file through SHA-256, counts its bytes, and admits only the exact frozen source:

- bytes: `25106120`
- SHA-256: `7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67`

For that exact source it independently decodes the supported uncompressed 16-bit CFA payload and requires:

- dimensions: `4080 × 3072`
- strips: `3072`
- decoded bytes: `25067520`
- decoded CFA SHA-256: `883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c`

Only after both source-byte admission and decoded-CFA verification pass may the client display the already-frozen downstream research identities:

- Scientific Master: `a86034da7b6f9663640ee3b4478ccf4294d23b720f2b087d083dca38f5fb4640`
- Dynamic Authority Field v1.9: `7678a0b145f8721347cdcc5177fedb720ba19b91bf34984a3f18bd8216408098`
- source-bound P3-D65 transform: `2105712a1be9950089976ba358afd4b062347680d5e6d8c500462c2e8d541533`
- reference `L0`: `0.12564234435558319`

v0.3 additionally exports a JSON verification report through `ACTION_CREATE_DOCUMENT` with schema `TruthRawAndroidVerificationReport/0.3` and classification:

`DEVICE_VALIDATION_ONLY_NO_SCIENTIFIC_WRITEBACK`

The report records the observed source/CFA result, frozen downstream references and observational device/time metadata. It explicitly records that Scientific Master, Dynamic Authority and HDR projection have **not** yet been recomputed on-device.

## Target-device evidence

The preceding v0.2 verifier has now passed on the target Honor phone for the exact source and decoded CFA. A user-supplied screenshot dated 2026-09-16 records:

- screenshot bytes: `206919`
- screenshot SHA-256: `f92e6461a9fa74b09854bc59f83c00c074648128e64fc9dedea93cfe2f479fb9`
- visible result: `PASS — exact source + decoded CFA verified on-device`
- source bytes/SHA: exact frozen-source match
- decoded CFA: `4080×3072`, `3072` strips, `25067520` bytes
- decoded CFA SHA: exact frozen-CFA match.

This closes the target-device gate for **source/CFA identity only**. It does not prove mobile Scientific Master recomputation, mobile Dynamic Authority recomputation or mobile HDR projection.

## Scientific boundary

Implemented on-device:

- exact file byte-count + SHA-256 admission;
- supported DNG parsing for the frozen source structure;
- deterministic decoded-CFA SHA-256 verification;
- export of a scoped device-validation JSON report.

Not yet recomputed on-device:

- Scientific Master;
- Dynamic Authority field;
- TruthRaw HDR projection / P3-D65/PQ transport.

A non-matching input inherits none of the downstream identities. Censored/unknown scientific values may not be replaced with exact display radiance. No APK result may write back to canonical evidence or promote scientific authority.

**Representation may exceed the source; knowledge claims may not exceed the evidence.**

## v0.3 build provenance

The CI build uses the fixed, explicitly **non-secret debug/test signing key** stored as `android/ci-debug.keystore.b64`. It is only for debug installation and must never be used as release signing authority.

The canonical APK payload digest is separate from the exact signed APK SHA because the APK Signing Block is not guaranteed to be byte-deterministic. The canonical payload digest is calculated by `android/tools/apk_payload_digest.py` over sorted ZIP-entry names and uncompressed entry bytes with explicit framing.

Initial v0.3 validation build:

- implementation commit: `24d2beaca7a7e6daf2141495a73f5f1b9bfe916e`
- GitHub Actions run: `35072702472`
- result: unit tests, `assembleDebug`, APK hash, canonical payload hash and artifact upload all `success`
- artifact: `truthraw-debug-apk-v0.3`
- artifact ID: `10436109297`
- exact signed APK SHA-256: `01f94593ebd9dcb8d7e5f8c5681eed1f79f469de3862b98723912fb9a36d8b61`
- canonical APK payload SHA-256: `33c616c8817403adb3aaa99aafd469282515169db7415a009701c5c091da804d`
- application ID: `io.truthraw.debug`
- minSdk: 26
- targetSdk: 35
- versionCode: 3
- versionName: `0.3-debug`

A later documentation/state-only commit does not invalidate that build evidence, but if Android source or build inputs change, the later successful Android CI artifact becomes the current installable build identity.

## Prior v0.2 deterministic payload evidence

v0.2 remains preserved as prior evidence:

- canonical APK payload SHA-256: `2a2b5ec59d7bafe3c810b446a440feaaa00641c88addf68e1c5d3f179c68bf1e`
- run: `35069372195`
- commit: `ae31675b3bb6c6b5c46fe26239ba7d8a06e9273a`
- exact APK SHA-256: `cd155f1903f30e76af40710465963b5dec60f2810cce4086bb55718edd9d5085`

The complete current project handoff is `docs/handoff/TRUTHRAW_DETAILED_HANDOFF_2026-09-16.md`.
