# TruthRaw v0.67 — full-resolution retreatable Restoration

Date: 2026-09-20

Branch:

`integration/truthraw-suite-v0-67-fullres-restoration`

App version:

`0.32-v0.67-fullres-restoration`

Status: **research integration / APK built / real-device artifact validation pending**.

## Purpose

v0.67 promotes Restoration from the bounded 384-pixel-edge Advanced preview into a real full-source-resolution derivative path while preserving the conservation rule:

> valid measured support is not overpainted; loss compensation remains detectable and retreatable.

The validated v0.63 TRUTHRAW PURE Float32 writer and Backplane CRC contract are unchanged.

## Full-resolution Restoration contract

The Android Advanced route can now export:

`*_truthraw_fullres_restoration_v0_67.trr`

This container is intentionally a TruthRaw restoration record rather than a conventional image container. It keeps the restored Float32 raster and the per-pixel restoration-role mask inseparable.

Stored pixel domain:

- source width × source height, no spatial resampling;
- camera-native Scientific-Master RGB;
- IEEE-754 binary32;
- canonical 64×64 cell sequence;
- one restoration-role byte per source pixel.

Restoration roles:

- `0 = PRESERVE_SCIENTIFIC_MASTER`;
- `1 = AESTHETIC_REINTEGRATION_ONLY`;
- `2 = UNRESOLVED_LOSS`.

## Eligibility and algorithm

A pixel is considered for restoration only when its underlying source CFA sample is at or above the admitted source WhiteLevel.

For eligible censored sites:

- support comes only from non-censored neighboring source sites;
- search radius is at most 2 source pixels;
- at least 3 finite non-censored neighbors are required;
- the presentation estimate is a distance-weighted RGB neighbor average;
- insufficient support leaves the Scientific-Master value unchanged and marks the site `UNRESOLVED_LOSS`.

Non-censored sites are copied bit-for-bit from the reconstructed Scientific Master into the derivative.

This is intentionally presentation restoration. A censored site is not promoted to measured or exact reconstructed scientific truth.

## Fail-closed lineage

Before release the exporter requires:

- sealed source SHA-256;
- source re-verification;
- current source-bound color admission;
- Scientific Master streaming binding;
- Technical Backplane phase 2;
- Backplane forbidden flags = 0;
- physicalFrameCount = 1;
- independentEvidenceCount = 1.

The exporter recomputes the complete camera-native Scientific-Master digest while producing the full-resolution derivative. If that replay digest differs from the admitted Scientific Master SHA-256, the file is truncated and rejected.

A second deterministic raster digest identifies the restoration derivative itself.

The saved header explicitly records:

- source/master/zero-line/scene-scale identities;
- exact serialized 180-byte Technical Backplane;
- restoration algorithm and support rule;
- preserved/censored/restored/unresolved counts;
- derivative content digest;
- `scientific_master_modified=0`;
- `scientific_writeback_allowed=0`;
- `creates_new_evidence=0`;
- `creates_second_scientific_world=0`;
- `retreatable=1`;
- `provenance_bound=1`.

Android reopens the exact destination and verifies those contract markers, lineage shapes and final file size before reporting success.

## Relationship to v0.66

v0.66 restored TruthNegative TN-2, the Open-World/Scene-Physics authority corridor, and Dynamic Authority into the current APK.

v0.67 adds the first full-resolution Restoration consumer of that scientific separation. The full-resolution path does not replace TN-2 and does not alter PURE.

Current high-level flow:

`SOURCE_EVIDENCE`
→ `SCIENTIFIC_MASTER`
→ `DYNAMIC_AUTHORITY / OPEN_SCENE`
→ optional `FULL_RES_RETREATABLE_RESTORATION`
→ later finite presentation/export.

## CI

Successful run:

`35498407249`

Results:

- host GCC PURE writer: SUCCESS;
- host Clang PURE writer: SUCCESS;
- OpenWorld/Dynamic-Authority/Restoration contract tests: SUCCESS;
- Android arm64 debug APK: SUCCESS.

Artifact ID:

`10601092611`

Artifact:

`truthraw-suite-v0-67-fullres-restoration-debug-arm64`

Extracted APK:

- bytes: `6088581`;
- SHA-256: `1ce0e75d409f3c2c8e5f6886e43cb39796dae58fc406d9aa3b78bc4406783c94`.

## Remaining gate

A real-device export must still be inspected before calling the new .trr artifact path device-validated.

The next useful test is one known DNG containing clipped/censored support. Verify:

1. source remains byte-identical before/after;
2. master replay passes;
3. output dimensions equal source dimensions;
4. `preserved + censored = all pixels`;
5. `restored + unresolved = censored`;
6. only role-1 pixels differ from the base Scientific Master;
7. no source-valid pixel is altered by restoration;
8. the file reopens and post-write verification passes.

A standard DNG/EXR/TIFF projection of the full-resolution restoration derivative can be added later, but it must preserve or accompany the restoration-role provenance rather than silently flattening it.
