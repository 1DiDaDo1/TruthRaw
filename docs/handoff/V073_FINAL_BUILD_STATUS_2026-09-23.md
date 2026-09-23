# TruthRaw v0.73 Final Build Status

Date: 2026-09-23

Branch:
`test/physical5-hint-remosaic-oracle-v073`

Build commit:
`b0aea238b7a11899c96723c7b5ccc1278cf8e173`

Workflow:
`TruthRaw v0.73 Scene53 hint-remosaic oracle`

GitHub Actions run:
`35923309174`

Result:
`FAILURE`

Artifact count:
`0`

There is no valid v0.73 APK artifact.

The source is preserved and the intended experiment is documented. No code was changed after the user's final instruction not to modify code.

## Intended route

`cameraSceneMode=53 -> priming frame -> read hintUserValue -> test 23/24/32/33 -> derive qcomRemosaicEnable -> one physical-5 RAW10/MAX frame -> read hint + payload topology`

## Build failure

The failure occurred during:

`:app:compileDebugKotlin`

File:

`Physical5HintRemosaicOracleActivity.kt`

Representative compiler errors:

- line 363: unresolved reference `width`
- line 363: unresolved reference `height`
- line 612: argument type mismatch; String supplied where Boolean? expected
- line 612: missing `errorMessage`
- line 693: argument type mismatch and missing error fields
- line 724: argument mismatch and missing `errorMessage`
- line 816: unresolved `Size`
- lines 817-820: type inference and `width`/`height` errors around preview-size selection

The workflow never reached APK verification or artifact upload.

## Preservation rule

Do not present any older APK as v0.73.

The last fully built and device-tested APK in this research line remains v0.71:

`TruthRaw_v0.71_Semantic_Locked_Capture_Effect_debug_arm64.apk`

SHA-256:

`ba717793ed779804f0bfdcc9dd668dd46c2474d589ffdb1a6f386caa0a6ffdb9`

Bytes:

`6,472,481`

## Final source lineage

- `e6a73562912019a461eb7de13be5a98d3ce06a55` - connect hintUserValue to ServiceHost high-pixel scene routing
- `a5edea10cd6669347e4e7a378b9595a840260910` - add scene53 hint-remosaic runtime oracle
- `72f52d4583eb3460601d65c22b44c0ffefa2484f` - register hint-remosaic oracle
- `9738ac0082a55154d9360d0f0cecf3dd98177376` - expose hint-remosaic oracle
- `b0aea238b7a11899c96723c7b5ccc1278cf8e173` - add v0.73 build workflow

This document is intentionally documentation-only.
