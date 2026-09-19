# TruthRaw v0.61 — TRUTHRAW PURE self-binding Float32 DNG

Date: 2026-09-19

Status: **IMPLEMENTED ON INTEGRATION BRANCH / CI + APK VALIDATION REQUIRED**

Active branch:

`integration/truthraw-suite-v0-61-pure-self-binding-dng`

App version:

`0.26-v0.61-pure-self-binding-dng`

## Why v0.61 exists

v0.60 restored the historical 32-bit IEEE Float XYZ-D50 LinearRaw writer and exact Scientific-Master digest gate. The previous-chat endpoint identified one remaining gap: the saved DNG did not itself carry enough of the already-existing TruthRange / scene-scale / Technical-Backplane state to interpret its scientific values independently of the live process that created it.

Older project history also showed that an earlier PURE branch had already experimented with an in-file certificate carrying zero-line, scene-scale and Backplane-derived identity. That historical `TRCERT01` schema remains legacy verification material and is **not** silently reactivated here.

v0.61 instead extends the current PURE private-data contract directly and versionably without changing Scientific-Master pixels, the frozen 180-byte Technical Backplane layout, or the historical writer Software identity.

## Pixel route unchanged

`sealed source DNG`
→ source-bound color
→ generic DNG adapter
→ Scientific Master streaming
→ Technical Backplane Phase 2
→ canonical Scientific-Master replay
→ exact master-digest gate
→ `cameraToXyzD50`
→ **32-bit IEEE Float XYZ-D50 LinearRaw DNG**.

The numerical pixel projection is unchanged from v0.60.

## New self-binding private contract

The DNG now carries:

- `private_contract=TRUTHRAW_PURE_SELF_BINDING_V0_61`;
- explicit PURE role rather than the old compatibility-role text;
- sealed source SHA-256;
- Scientific Master SHA-256;
- zero-line SHA-256;
- exact zero-line `L0` as IEEE-754 binary64 bit pattern;
- zero-line mode, gauge ID and authority flags;
- scene-scale SHA-256;
- scene-scale ID and normalization/GainMap flags;
- the exact canonical 180-byte Technical Backplane serialized as hex;
- Technical Backplane CRC32;
- precision-policy identity;
- actual runtime reconstruction-backend identity;
- source-evidence and color-binding identities;
- explicit no-master-mutation, no-appearance and no-counterfactual flags;
- `physicalFrameCount=1`;
- `independentEvidenceCount=1`.

The writer deserializes and validates the embedded Backplane before output. Its source/master/zero-line/scene-scale identities must match the projection descriptor exactly, evidence count must remain 1/1 and forbidden mutation flags must remain zero.

## Precision provenance

The private record distinguishes project precision policy from the actual runtime backend.

Policy identity:

`EXACT_SOURCE__F64_BRANCH_SENSITIVE_REFERENCE_POLICY__F64_CAL_OPT_COV_REFERENCE_POLICY__CONTROLLED_F32_MASTER_STORAGE__F32_PURE_PROJECTION`

The current runtime reconstruction backend is also serialized separately. This avoids falsely claiming that the export layer itself proves a particular F64 execution path while preserving the project-wide F64 reference policy.

## Historical certificate finding

The 2026-09-14 PURE branch contained a 296-byte `TRCERT01` certificate and DNGPrivateData embedder that already carried source/master/zero-line/scene-scale and a Backplane CRC. That work is important historical proof that self-contained backside provenance was intended and had been implemented.

Current recovery governance classifies that schema as **LEGACY_VERIFY_ONLY**. v0.61 therefore does not reuse it as the current certification authority. Future PTC/Dynamic-Authority certification must be added as a new versioned binding and may not alter PURE pixels or the frozen Backplane layout.

## Fail-closed tests

Host writer tests now additionally require:

- the v0.61 private contract and PURE role are present;
- exact `L0` binary64 bits are serialized;
- zero-line and scene-scale identities are present;
- exact Technical Backplane bytes are present;
- precision/runtime provenance is present;
- a corrupted serialized Backplane is rejected;
- an invalid zero-line payload is rejected;
- deterministic output remains required;
- wrong Scientific-Master digest still aborts transactionally.

## Remaining authority boundaries

v0.61 does not make a projection measured CFA evidence, does not increase evidence count, does not make source metadata independent physical calibration and does not promote Nikon NEF into Scientific Master eligibility.

Dynamic Authority / current Pure Truth Certificate integration remains a separate versioned certification task. It must bind lineage without changing Scientific-Master identity, zero-line identity, the frozen Backplane bytes or PURE pixel values.
