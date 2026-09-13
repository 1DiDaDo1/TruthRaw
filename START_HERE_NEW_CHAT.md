# START HERE — TruthRaw current bootstrap

This is the authoritative session/bootstrap entry point for the renewed TruthRaw house as of 2026-09-10.

## Mandatory reading order

1. `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md`
2. `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md`
3. `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
4. `state/CURRENT_CANONICAL_STATE_2026-09-10.json`
5. `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md`
6. only then: the canonical/research module documents relevant to the task

Do **not** bootstrap from older dated current-state snapshots or `docs/PROJECT_STATE_AUDIT_2026-09-08.md`. Those are preserved historical records.

## Non-negotiable scientific rules

The source RAW/CFA + capture metadata are immutable sealed evidence. The scientific Scene Master is a separate reconstructed state.

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Measured, reconstructed, censored/unknown, counterfactual and appearance data remain distinct. `physicalFrameCount=1` and `independentEvidenceCount=1` remain the single-frame evidence invariants.

For positive physical light, TruthRange may use `T = log2(L/L0)`. The zero-line `L0` is a gauge/reference; it is not sensor black, absolute darkness, DNG BlackLevel or clipping.

Appearance must never modify the scientific master. DNG/LinearRaw/export and preview encoding are downstream representations, not the Scientific Master.

APK/GCam/computational-RAW content must not determine TruthRaw evidence, calibration, topology, color, noise model or architecture.

## Renewed house execution rules

- truth floor controls epistemic permission;
- resource profile controls only RAM/CPU/GPU/tile/cache execution;
- hardware capability never upgrades scientific claims;
- corridors carry handles/provenance rather than duplicate full-frame payloads;
- zero-line/source/master/scene-scale identity belongs to one immutable shared binding;
- resource policy may be re-derived between room operations, never halfway through an indivisible scientific operation.

## Branch-local continuation: RGB / LinearRaw DNG restoration — 2026-09-13

When active branch is:

`research/restore-rgb-linearraw-output-v0.2-workbase-2026-09-13`

read, in this order:

1. `docs/research/linear-dng-projection-v0.2/README.md`
2. `docs/research/linear-dng-projection-v0.2/evidence/MODERN_FINALIZED_PREVIEW_ARCHITECTURE_2026-09-13.md`
3. `docs/research/preview-representation-v0.1/README.md`
4. `docs/research/preview-representation-v0.1/DNG_EMBEDDED_PREVIEW_NOTE.md`
5. `docs/research/preview-representation-v0.1/ANDROID_JPEG_NOTE.md`
6. `docs/research/linear-dng-projection-v0.2/evidence/HISTORICAL_ANDROID_PREVIEW_RESTORE_SOURCE_2026-09-13.md`
7. host/Android/Work evidence files in the same v0.2 evidence directory.

### Leading preview rule

Do **not** treat the old v0.7 IFD arrangement as the preview architecture.

The leading route is:

`source -> Scientific Master/digest -> TruthRange/zero-line -> Technical Backplane phase 2 -> finalized Scientific Preview -> bounded sRGB preview surface -> representation`

Representations include:

- runtime ARGB_8888/sRGB;
- standalone baseline JPEG/sRGB;
- embedded DNG JPEG compatibility preview;
- optional HDR display derivative.

The DNG container transports the embedded preview but does not define its scientific authority.

The old v0.7 `thumbnail IFD0 + LinearRaw SubIFD + JPEG preview SubIFD` pattern remains historical interoperability evidence and a candidate packaging topology only.

### Current restoration rules

- retain finalized Scientific Preview admission and exact source re-verification;
- retain bounded streaming and do not materialize a full RGB Scientific Master merely for DNG export;
- retain the validated first-candidate `2x` finite LinearRaw window and `BaselineExposure=+1 EV`;
- preserve source `Make`, `Model` and `UniqueCameraModel` for source-bound color metadata;
- reject output if reconstructed RGB exceeds the finite `2x` window;
- never let export or preview alter Scientific Master hash, zero-line, Backplane, frame/evidence counts or color authority;
- preserve old v0.1 states as historical/negative evidence.

Validated so far:

- GCC Release;
- Clang Release;
- Clang ASan/UBSan;
- Android arm64 v0.2 build and JNI/native-path verification.

Exact built pre-embedded-preview restoration APK:

- version `0.6-linear-dng-restore`;
- bytes `4335889`;
- SHA-256 `93e51245e0950c5c3140b83f2f3429d2f52ad48adcd5630409134e4c430b5654`.

Next independent gates: couple the **same finalized preview representation** into DNG preview transport, then test exact output bytes on Honor/Android, Lightroom/ACR, LibRaw and DNG SDK.

The old-chat phase label remembered as approximately `1.3` / `1.4` remains unassigned unless an exact historical source proves the label.

## One-sentence definition

**TruthRaw preserves one sealed RAW observation as immutable evidence, reconstructs a separate uncertainty-aware Scene Master, and executes specialized rooms through a resource-adaptive but scientifically invariant building runtime.**
