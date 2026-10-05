# D.RAW 44489 — Workspace / Free Raster real-device acceptance

**Status:** DEVICE ACCEPTANCE / GOVERNANCE EVIDENCE — NOT SCIENTIFIC PROMOTION  
**Date:** 2026-10-05  
**Workspace runtime line:** `feat/draw-workspace-free-raster-v01`  
**Runtime code checkpoint under acceptance:** `c85b9805a56681b1adbc39f58b95724c4810907c`  
**Frozen scientific/audit reference remains:** PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`

This record closes the first Workspace / Free Raster device-acceptance round. It records observed product/runtime output only. It does not promote any scientific candidate, change sealed evidence, authorize correction/writeback, or reinterpret UNKNOWN as certainty.

## 1. Device output set used for acceptance

One tele observation produced a self-consistent output family bound to sealed source SHA-256:

`fe88a0acd2f95d35353ef9e4c925e50923e100a257bd43f545df4c09b09340b8`

Inspected acceptance artifacts:

- full-colour Scientific Master Float32 DNG
- Observation Optical Field Chart v0.1
- Universal Observation Calibration Atlas v0.1
- Global Research Snapshot v0.1

Local artifact hashes from the received device outputs:

- Scientific Master DNG SHA-256: `a6d2c2a59b831a1379f4c21c5fb6f38915a9e93c94d93f4567fe81b352758bbb`
- Optical Field Chart JSON SHA-256: `6ac6a77a5b327c85c75f3a273cb96340b04fbf841002eb43223c084e170ddf2e`
- Universal Observation Calibration Atlas JSON SHA-256: `8787af88b64c7486bcf17f66bf05d77e7586b6b3222f9cd056093c1fa17d20fb`
- Global Research Snapshot JSON SHA-256: `4482e024fcf1bc2f9895db194fe3e43cebf48a969725101f2f23a2d7b75e6e49`

## 2. Scientific Master DNG acceptance

The DNG primary image is a full-colour, three-channel IEEE Float32 LinearRaw representation at 4080×3072, with an embedded non-authority rendered preview. Its private contract binds the output to the same sealed source SHA above and records:

- `representation_only=1`
- `scientific_master_modified=0`
- `appearance_applied=0`
- `counterfactual_observation_created=0`
- `physical_frame_count=1`
- `independent_evidence_count=1`
- `scene_gainmap_applied_exactly_once=1`
- `preview_role=NON_AUTHORITY_RENDERED_PREVIEW`
- `preview_scientific_writeback_allowed=0`
- `output_channel_authority_bound=1`
- `scientific_writeback_allowed=0`
- `creates_new_evidence=0`

Bound identities recorded in the DNG:

- Scientific Master SHA-256: `c57e78f045ad26e92c985d8f7a7ecdbe0f19ba1bf02f8aa53601202a7d0e5863`
- Zero-Line SHA-256: `e9249367d07e9d610c36cf996710fb9bbf1924ab9f09b2be78ba4eac0541c9cc`
- Scene-scale SHA-256: `90cca2a7932116f9059174e50028817a0ecc66df182d107eda92b1955cb6fdaf`

This is accepted as evidence that the Workspace/product layer did not force Appearance back into the scientific primary or create a second physical observation.

## 3. Universal Observation Calibration Atlas acceptance

The atlas identifies the source as `DNG_CFA_RAW` on `CAMERA_CAPTURE`, while keeping scientific identity independent of camera/lens/vendor naming. Backside/source measurement support is available, including CFA, black level, white level, noise profile and active area.

Frontside inspection remains explicitly `APPEARANCE_DERIVED_ONLY` and cannot create sensor evidence or write scientific state. Radiometric response, colour calibration, noise decomposition, illumination, optical support and temporal geometry remain fail-closed where not independently proven. Automatic correction, denoise, deconvolution and scientific writeback remain unauthorized.

This is the intended outcome. UNKNOWN and NOT-CALIBRATED states are not product failures.

## 4. Optical Field Chart acceptance

The field chart is available and source-bound. Its coordinate origin is the active-area geometric center only; optical axis is not claimed. It exports a measured composite scene/lens/sensor field signal from 12,288 source samples with radial and azimuth structure.

The runtime correctly refuses to relabel this as lens-only vignetting, separated scene illumination, separated sensor angular response or a proven optical axis. No cos^4 model is assumed, correction gain remains unauthorized, source samples/positions remain unchanged, and no new evidence or scientific writeback is created.

This validates the intended distinction between a measurable field pattern and a calibrated optical correction model.

## 5. Global Research Snapshot acceptance

The snapshot correctly identifies itself as `IMPLEMENTATION_MAP_NOT_PHOTO_EVIDENCE` and preserves the normal product route:

`CHOOSE_OR_KEEP_ROUTE -> OPEN_RAW_OR_DNG_OR_USE_UNIVERSAL_CAMERA -> SEAL_SOURCE -> UNIVERSAL_INTAKE -> SCIENTIFIC_MASTER -> ROUTE_SPECIFIC_VIEW_OR_EXPORT`

PURE, ADVANCED and PRO do not gain stronger evidence merely from route selection. The promotion registry remains fully fail-closed: candidate availability is not promotion, automatic promotion is disabled, and scientific writeback is not granted by the snapshot.

Its `new_multidisciplinary_wave_device_validated=false` field is intentionally not rewritten by this external acceptance record: build/device acceptance is governance evidence outside the scientific snapshot itself.

## 6. Workspace / Free Raster acceptance conclusion

The first real-device Workspace round is accepted for product continuation because:

1. the app was reported operational through the tested Workspace route;
2. exported scientific output remains bound to one sealed source and one physical frame;
3. Scientific Master and Appearance remain separated;
4. Free Raster / preview representation does not create measured evidence;
5. source/frontside/field-analysis roles remain correctly separated;
6. unsupported calibration axes remain UNKNOWN / NOT PROMOTED instead of being guessed;
7. no source mutation, candidate promotion or scientific writeback is observed in the supplied outputs.

This supersedes the pre-device wording in capsule section 13.4 only for Workspace v0.1 product acceptance. It does **not** mean every research candidate or future multi-observation/world-space feature is scientifically validated.

## 7. Final APK rule

The final Workspace APK must be produced from the accepted runtime code line at or after `c85b9805a56681b1adbc39f58b95724c4810907c`, with no subsequent scientific/runtime mutation unless separately validated. Documentation-only commits may follow without changing APK runtime identity.

Required APK provenance before delivery:

- exact runtime commit SHA
- successful Android assemble/verify job
- artifact ID
- APK byte size
- APK SHA-256
- stable signing-certificate SHA-256

## 8. Final accepted Workspace APK identity

The required build completed successfully without any further runtime/scientific source change after the accepted checkpoint.

- runtime code SHA: `c85b9805a56681b1adbc39f58b95724c4810907c`
- workflow: `D.RAW Suite Universal Intake v0.1`
- workflow run: `37370065875`
- run attempt: `2`
- build job: `111979622964`
- build conclusion: `SUCCESS`
- vision-contract verification: `SUCCESS`
- Android assembleDebug: `SUCCESS`
- APK verification: `SUCCESS`
- artifact: `DRAW_Full_Suite_Universal_Intake_v0.1_debug_arm64`
- artifact ID: `11371257135`
- artifact ZIP SHA-256: `1c8364f827eeb4939d98698c7bf96eff59fd6ddb4d0dae94b97b628d51eee366`
- APK bytes: `8,637,267`
- APK SHA-256: `642adbed19e67fdbf98650c1bb23da79945e01d4d7781fb1b9702f15407cd3ea`
- stable signing-certificate SHA-256: `a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`

The later repository commits on this branch are documentation-only acceptance/handoff updates. They do not change APK runtime identity. PR #130 remains the frozen scientific/audit reference and has not been promoted or merged by this acceptance round.

Permanent boundary remains:

**One Free World. Many sealed observations. One evidence law.**
