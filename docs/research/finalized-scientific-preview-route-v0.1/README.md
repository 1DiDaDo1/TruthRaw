# Finalized Scientific Preview Route v0.1

Status: **VALIDATED RESEARCH IMPLEMENTATION — DIRECT-NATIVE AUTHORITY ROUTE**

Parent validated streaming Scientific Master head:
`5af2d3474539dc3911d8e3420c08b3e17149b987`

Validated implementation head:
`89544bdced5fb225000adf3657673f37478f63c3`

Validation evidence:
- Finalized Scientific Preview Route v0.1 run `34642089037` — SUCCESS
  - GCC Release — SUCCESS
  - Clang Release — SUCCESS
  - Clang ASan/UBSan — SUCCESS
- Documentation Governance run `34642089078` — SUCCESS on the same implementation head

Negative evidence retained:
- run `34641953582` failed during compilation before tests because the new test used an ambiguous status name / cast form; this failed run remains part of project history and was not reclassified or erased.

## Purpose

This module closes the direct-native pre-master/post-master authority chain without allowing the caller to inject fixture/dummy Scientific Master, zero-line or scene-scale hashes.

The route is:

`prepared exact source/color binding`
`-> reverify exact source bytes by SHA-256`
`-> open TileNativeDngSource from those same bytes and prepared OpenOptions`
`-> bounded streaming camera-native Scientific Master + exact TruthRange self-gauge`
`-> Technical Backplane phase 2`
`-> existing Scientific Preview finalization admission`

Only after every step succeeds is `scientificPreviewReleaseAllowed=true`.

## Strong source binding

A source ID string alone is not accepted as sufficient proof. `finalize_direct_native()` receives the original random-access source bytes, reverifies them against the prepared source SHA-256, and then constructs `TileNativeDngSource` itself from that same byte-source object.

This prevents a caller from supplying a second unrelated CFA tile stream while reusing the original source ID.

A one-byte mutation after preparation fails during source reverification before Scientific Master computation.

## Color authority remains bounded

A complete Scientific Master/Backplane lineage does not manufacture stronger color truth.

- `SourceMetadataBound` or `GatehouseCertifiedMetadata` may finalize a Scientific Preview with their existing source-bound claim scope.
- `scientificClaimAllowed` in this v0.1 result is true only for `IndependentCalibration`.
- No source-metadata binding is promoted to `FULL_PHYSICAL` by finalization.

## Scientific state

The route uses the validated Scientific Master Streaming Binding v0.1:
- fixed 64x64 scientific grid;
- camera-native reconstructed RGB before XYZ/appearance;
- deterministic Scientific Master SHA-256;
- TruthRange v0.2 self-gauge from positive finite uncensored Stage-2 samples;
- one physical frame;
- one independent evidence root;
- no full-frame Scientific Master materialization.

Technical Backplane phase 2 then binds separately:
- exact source evidence SHA-256;
- Scientific Master SHA-256;
- zero-line identity;
- scene-scale identity.

The serialized Backplane remains exactly 180 bytes.

## Validated gates

The end-to-end synthetic DNG test proves the implementation can:
1. build a valid uncompressed 16-bit 2x2 CFA DNG in memory;
2. compute its real source SHA-256 seal;
3. prepare source-metadata-bound color authority;
4. reverify the same bytes immediately before finalization;
5. open TileNativeDngSource from those same bytes;
6. compute the streaming Scientific Master and self-gauge;
7. complete Technical Backplane phase 2;
8. finalize the existing Scientific Preview admission;
9. deserialize and validate the 180-byte Backplane;
10. reject a one-byte source mutation;
11. keep source-bound finalization from gaining independent physical color authority;
12. allow an independently calibrated binding to carry the stronger calibrated claim flag without changing frame/evidence counts.

## Scope / non-claims

This v0.1 closes the **direct-native** authority route only. It does not yet prove:
- physical Honor/MotionCam device execution;
- the four real uploaded HONOR DNG examples have completed this exact route;
- dual-illuminant DNG color interpolation;
- independent camera/lens calibration for those HONOR files;
- a Lightroom-readable TruthRaw compatibility DNG;
- Android Bitmap/JPEG rendering from the finalized admission;
- Gatehouse decoded-measurement finalization (requires a separate certified-handoff route);
- cross-CPU bit-identical v4.7i floating-point reconstruction.

## Next integration step

Use this validated admission as the release gate for the already designed Android Scientific Preview representation. The existing bounded sRGB/ARGB_8888/JPEG renderer is retained; the new work is an authority/release integration, not a new preview design. Source-bound appearance may remain available as a correctly labeled pre-finalization fallback, but it must never be relabeled as a finalized Scientific Preview when this gate has not passed.

**Measured where measured. Reconstructed where necessary. Never invented.**
