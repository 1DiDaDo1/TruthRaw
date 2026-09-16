# TruthRaw detailed handoff — 2026-09-16

Status: **CURRENT RESEARCH HANDOFF — research/open-world-foundations-v01 — NOT MAIN-PROMOTED**

This handoff supplements, but does not replace, the governed 2026-09-10 bootstrap architecture. Start with `START_HERE_NEW_CHAT.md`, then use this document for the current Adobe-HDR, Scientific-Master, Dynamic-Authority and Android-device validation state.

## 1. Permanent scientific laws

TruthRaw preserves one physical RAW/CFA observation as immutable evidence and reconstructs a separate uncertainty-aware scientific state.

**Measured where measured. Reconstructed where necessary. Never invented.**

**Representation may exceed the source; knowledge claims may not exceed the evidence.**

The old “sealed house” language applies to source-evidence immutability only. The reconstructed world is open: it may represent interiors, exteriors, streets, landscape, sky, distant geometry and arbitrary scene graphs. A Room Capsule is only a local computational work domain, never a boundary on the represented world.

Measured, calibrated-estimate, reconstructed, censored, unknown, counterfactual and appearance-only authority remain distinct. Presentation/export/Adobe/GCam/APK behavior must never write back scientific authority. `physicalFrameCount=1` and `independentEvidenceCount=1` remain the evidence invariants for this source.

## 2. Exact immutable source identity

Frozen source:

- filename: `IMG_BNC_TRUTHRAW20260907_094449_565.dng`
- bytes: `25,106,120`
- source SHA-256: `7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67`
- decoded CFA dimensions: `4080 × 3072`
- decoded CFA strips: `3072`
- decoded CFA bytes: `25,067,520`
- decoded CFA SHA-256: `883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c`
- source-white censored CFA samples: `217`
- one physical frame, one independent observation.

The decoded CFA hash is the canonical little-endian uint16 raster identity for this exact source. It is not an appearance/render hash.

## 3. Scientific Master and Dynamic Authority identities

The modern float Scientific Master was independently reproduced twice from the same exact source:

- Scientific Master SHA-256: `a86034da7b6f9663640ee3b4478ccf4294d23b720f2b087d083dca38f5fb4640`
- TruthRaw reference/gauge `L0`: `0.12564234435558319`

The full-frame RGB Dynamic Authority identity is now bound:

- field SHA-256: `7678a0b145f8721347cdcc5177fedb720ba19b91bf34984a3f18bd8216408098`
- R channel SHA-256: `d0b8febb3e62d18968e72036a76f577453554553e117692a1fdd9d2c9b23e86f`
- G channel SHA-256: `38b742784513e7f70fc31a6f39c6227617fe8719705f544c48524ffbfacc8a1f`
- B channel SHA-256: `ab2ab62c7d6d2d29fc915374773ccdb805e1ee4870a38d7e523ae9074efe932f`

Authority counts:

- `CALIBRATED_ESTIMATE`: `12,533,543`
- `RECONSTRUCTED`: `25,067,086`
- `CENSORED`: `217`
- `UNKNOWN`: `434`

Censored measured channels remain inequalities, not invented exact radiance. Missing channels at censored sites remain unknown. Reconstruction authority does not imply optical-detail/acutance authority.

The source-bound camera-RGB → P3-D65 transform identity is:

`2105712a1be9950089976ba358afd4b062347680d5e6d8c500462c2e8d541533`

Its authority remains `SOURCE_METADATA_BOUND_L1_NOT_PHYSICAL_CALIBRATION`.

## 4. HDR architecture and Adobe evidence

TruthRaw HDR is defined as a one-way presentation projection from the Scientific Master plus Dynamic Authority into a finite display/transport domain. The scene itself has no fixed upper `[0,1]` or fixed-EV container.

The v1.5/v1.6 projector separates scientific identity from display mapping and can generate P3-D65/PQ codes only where an exact scientific projection is authorized. `CENSORED`/`UNKNOWN` values may not be silently converted into exact scientific radiance. Counterfactual and appearance-only values cannot enter the scientific route.

Adobe Lightroom experiments established presentation behavior, not new sensor evidence:

- Lightroom-written DNG preserved all decoded source CFA samples exactly while changing metadata/preview/edit state.
- Rec.709/PQ and P3-D65/PQ AVIF exports from the same source demonstrate that finite HDR presentation can redistribute/compress/clip luminance without changing source evidence.
- Controlled P3-D65/PQ tone-only pair `(8)/(9)` kept parsed scalar Camera Raw settings fixed while changing the tone curve. The extreme variant filled the PQ ceiling across most pixels; this is presentation saturation, not recovered sensor dynamic range.
- Counterbalanced exports with `Exposure -5`, `Highlights -100`, `Whites -100` and a strongly raised tone curve showed that presentation headroom can be redistributed toward the extreme highlight tail without globally pushing the image to the PQ ceiling.
- `HDR Limit +8`, MaxCLL, gain-map behavior and PQ code peaks are presentation/transport evidence. None is sensor dynamic range by itself.

The target architecture remains:

`immutable CFA → Scientific Master → Dynamic Authority → source-bound color transform → authority-aware HDR projection → standards-explicit P3-D65/PQ transport`

A full TruthRaw-owned encoded HDR exchange file still needs to be produced and round-trip validated before HDR transport is called complete.

## 5. Historical LinearRaw bridge

The older full-frame 16-bit reconstructed LinearRaw is useful compatibility/integration evidence only. It is explicitly reconstructed, not a measured-photon master, and may not substitute its file or pixel hash for the modern Scientific Master identity.

Its signed scene-linear decode can carry values below 0 and above 1; that is valid estimator/domain behavior and does not create additional evidence.

## 6. Android client — verified device boundary

Android v0.2 is an installable **non-canonical debug verifier**. It admits the exact frozen source by byte count + source SHA and independently decodes/hashes the uncompressed 16-bit CFA raster.

A user-supplied target-device screenshot dated 2026-09-16 records an on-device PASS for the exact source:

- screenshot bytes: `206,919`
- screenshot SHA-256: `f92e6461a9fa74b09854bc59f83c00c074648128e64fc9dedea93cfe2f479fb9`
- visible result: `PASS — exact source + decoded CFA verified on-device`
- observed source bytes: `25,106,120`
- observed source SHA-256: exact frozen source match
- decoded CFA: `4080×3072`, `3072` strips, `25,067,520` bytes
- decoded CFA SHA-256: exact frozen CFA match.

This closes the v0.2 target-device gate for **source/CFA identity only**. It does not show on-device Scientific Master recomputation, Dynamic Authority recomputation or HDR projection. Those remain frozen references and must be ported/matched independently.

APK behavior is validation/integration evidence and cannot define or modify TruthRaw science.

## 7. Android v0.3 direction

v0.3 keeps the exact v0.2 admission/CFA verifier and adds an exportable JSON device-validation report. The report is explicitly classified:

`DEVICE_VALIDATION_ONLY_NO_SCIENTIFIC_WRITEBACK`

It records observed source/CFA identity, frozen downstream references, app/device/timestamp observational metadata and hard `false` flags for on-device Scientific Master, Dynamic Authority and HDR execution until those runtimes truly exist. Report export uses Android's Storage Access Framework; no report field may upgrade authority.

## 8. Current hard boundaries

Do not claim any of the following until separately proven:

- physical sensor calibration from the source-bound L1 color transform;
- recovered exact radiance at source-censored samples;
- new dynamic range from Adobe/AVIF/PQ/MaxCLL/HDR Limit;
- mobile Scientific Master equivalence merely because source/CFA identity passes;
- mobile Dynamic Authority equivalence before the exact authority runtime reproduces the frozen field SHA;
- complete HDR transport before a full-frame TruthRaw-owned encoded output is round-trip validated;
- main promotion from research-branch CI alone.

## 9. Next gates

1. Build/install Android v0.3 and export a PASS verification JSON from the target Honor device.
2. Port the exact Scientific Master runtime to Android/native without changing the frozen contract; reproduce `a86034da…4640` on-device.
3. Port Dynamic Authority v1.9; reproduce `7678a0b1…8098` and channel identities on-device.
4. Bind the verified master + authority tuple to the P3-D65 projector.
5. Define an explicit presentation-only policy for withheld censored/unknown pixels without inventing radiance.
6. Encode a standards-explicit full-frame TruthRaw-owned P3-D65/PQ file and validate decoder/encoder round-trip.
7. Compare/round-trip that output through Lightroom with no scientific-authority writeback.
8. Only after equivalence and governance gates pass, consider promotion beyond the research branch.

## 10. Bootstrap rule for the next chat

Read in this order:

1. `START_HERE_NEW_CHAT.md`
2. `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md`
3. `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md` with the open-world clarification in this handoff
4. `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
5. `state/CURRENT_CANONICAL_STATE_2026-09-10.json`
6. `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md`
7. **this handoff**
8. `state/TRUTHRAW_HDR_DYNAMIC_AUTHORITY_BINDING_V19_STATE.json`
9. latest Android debug client state.

Never bootstrap from an older snapshot merely because its filename says CURRENT. Never promote presentation evidence into source evidence.
