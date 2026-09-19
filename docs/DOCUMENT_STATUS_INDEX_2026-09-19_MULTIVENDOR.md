# TruthRaw document status index — multi-vendor integration — 2026-09-19

Status: **CURRENT FOR `integration/truthraw-suite-v0-59-nef-radiometric-admission`**

This index does not rewrite older dated TruthRaw documents. It classifies which files a new chat should treat as the current integration bootstrap and which remain branch-local or historical provenance.

## Current bootstrap

Read first:

1. `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-19.md`
2. `state/CURRENT_PROJECT_STATE_2026-09-19.json`
3. `START_HERE_NEW_CHAT.md`
4. `docs/TRUTHRAW_FULL_GENEALOGY_RECOVERY_AUDIT_2026-09-19.md`
5. `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
6. `docs/UNIFIED_FILE_CAMERA_INGRESS_CONVERGENCE_V056_2026-09-19.md`
7. `docs/research/multivendor-raw-source-adapter-v0.1/README.md`
8. `docs/research/nikon-nef-radiometric-admission-v0.59/README.md`

## Current app/integration state

Current branch:

`integration/truthraw-suite-v0-59-nef-radiometric-admission`

Current app:

`0.24-v0.59-nef-radiometric-admission`

Current product rule:

`RAW file import (primary) OR camera capture (secondary) -> SEALED_SOURCE_ADMISSION -> one shared scientific pipeline`.

Current proprietary RAW state:

- DNG: native through generic RAW adapter ABI;
- Nikon NEF: strict uncompressed 16-bit CFA sample decoder;
- Nikon black/saturation: exact-scope radiometric admission contract exists;
- real Nikon calibration pack: not yet admitted;
- NEF Scientific Master eligibility: still fail-closed pending noise/uncertainty, source-bound color and held-out validation.

## Current scientific architecture retained from 2026-09-16

These remain current scientific architecture/background for this integration branch:

- `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
- `docs/CURRENT_SCENE_PHYSICS_RESTORATION_200MP_2026-09-16.md`
- `state/CURRENT_PROJECT_STATE_2026-09-16.json` — historical global integration snapshot, superseded as current navigation by `state/CURRENT_PROJECT_STATE_2026-09-19.json`, but still authoritative for the detailed scientific state it froze.

The permanent scientific laws did not reset when the app/ingress work advanced.

## Current recovery and genealogy

Current recovery sources:

- `docs/TRUTHRAW_FULL_GENEALOGY_RECOVERY_AUDIT_2026-09-19.md`
- `state/FULL_GENEALOGY_PURE_OPENWORLD_RECOVERY_V0_1_2026-09-19.json`
- `docs/PURE_RECOVERY_TRANSPLANT_MATRIX_2026-09-19.md`

These restore/retain the PURE path, Open World, HDR, water/material/detail line, conservation-restoration line and Camera-5/HONOR investigation genealogy.

## Current multi-vendor RAW modules

- `docs/TRUTHRAW_V056_MULTIVENDOR_RAW_INGRESS_PLAN_2026-09-19.md`
- `docs/UNIFIED_FILE_CAMERA_INGRESS_CONVERGENCE_V056_2026-09-19.md`
- `state/TRUTHRAW_V056_MULTIVENDOR_RAW_INGRESS_2026-09-19.json`
- `docs/research/multivendor-raw-source-adapter-v0.1/README.md`
- `state/TRUTHRAW_V058_NIKON_NEF_SAMPLE_ADAPTER_2026-09-19.json`
- `docs/research/nikon-nef-radiometric-admission-v0.59/README.md`
- `state/TRUTHRAW_V059_NIKON_NEF_RADIOMETRIC_ADMISSION_2026-09-19.json`

## Camera-5 current interpretation

Use the recovery handoff for the concise precedence:

1. v0.53 current Android-17 replay/control;
2. v0.19/v0.20 detailed payload forensics;
3. v0.14 historical first source-first successful acquisition;
4. v0.54 independent OEM Pro TELE DNG corroboration.

Historical detailed Camera-5 files remain:

- `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md`
- `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json`
- `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md`

Do not promote the 16320×12288 envelope into proof of a fully populated/native 200MP ADC raster.

## Historical/current-for-their-branch snapshots retained

These are intentionally retained and must not be deleted or rewritten as if they knew later results:

- `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-09-16.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-09-17.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-09-19_TRUTHNEGATIVE_BRANCH.md`
- `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md`
- `state/CURRENT_CANONICAL_STATE_2026-09-06.json`
- `state/CURRENT_CANONICAL_STATE_2026-09-08.json`
- `state/CURRENT_CANONICAL_STATE_2026-09-09.json`
- `state/CURRENT_CANONICAL_STATE_2026-09-10.json`
- `docs/PROJECT_STATE_AUDIT_2026-09-08.md`
- `docs/handoff/TRUTHRAW_CONSOLIDATED_HANDOFF_2026-09-16.md`
- `docs/handoff/TRUTHRAW_DETAILED_HANDOFF_2026-09-16.md`
- `docs/handoff/TRUTHNEGATIVE_BRANCH_HANDOFF_2026-09-19.md`

TruthNegative remains a valid research overlay but is not the first bootstrap on the active multi-vendor integration branch.

## Research foundations still current

- `docs/research/scene-physics-calibration-structure-hdr-v0.1/README.md`
- `docs/research/conservation-restoration-authority-v0.1/README.md`

## CI evidence for current head family

v0.59 host radiometric admission:

- run `35456672651`
- result: SUCCESS on GCC and Clang.

v0.59 Android:

- run `35456737363`
- result: SUCCESS;
- artifact `truthraw-suite-v0-59-nef-radiometric-admission-debug-arm64`;
- artifact ID `10588558376`;
- archive SHA-256 `4de4346d18fc9c3477ed5740d08f400c3e96500b89d9626a9a553e1cad0793b8`.

## Current next gate

Do not set Nikon NEF `scientificAdmissionReady=true` until all of these are independently closed:

1. exact-scope noise/uncertainty admission;
2. source-bound color admission;
3. held-out validation.

A successful decoder or radiometric pack is not sufficient by itself.
