# TruthRaw HDR Dynamic Authority binding v1.9

v1.9 closes the final identity gate left open by v1.8 for the exact single-frame HONOR BKQ-N49 tele source `IMG_BNC_TRUTHRAW20260907_094449_565.dng`.

The immutable source DNG is SHA-256 `7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67`; its decoded CFA raster is `883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c`; and the independently recomputed float Scientific Master from v1.8 is `a86034da7b6f9663640ee3b4478ccf4294d23b720f2b087d083dca38f5fb4640` with self-gauge `L0=0.12564234435558319`.

## Real full-frame authority result

The v1.9 native probe rebuilt the exact Scientific Master, verified its hash before authority generation, rebuilt the v5.0g uncertainty anchors, and then streamed the HDR-facing RGB authority field in strict global raster order. The resulting field identity is:

`7678a0b145f8721347cdcc5177fedb720ba19b91bf34984a3f18bd8216408098`

Per-channel identities are R `d0b8febb3e62d18968e72036a76f577453554553e117692a1fdd9d2c9b23e86f`, G `38b742784513e7f70fc31a6f39c6227617fe8719705f544c48524ffbfacc8a1f`, and B `ab2ab62c7d6d2d29fc915374773ccdb805e1ee4870a38d7e523ae9074efe932f`.

Across 37,601,280 RGB channel records, v1.9 contains 12,533,543 `CALIBRATED_ESTIMATE`, 25,067,086 `RECONSTRUCTED`, 217 `CENSORED`, and 434 `UNKNOWN` records. This is fail-closed at clipping: each of the 217 source-white-censored CFA sites keeps the measured channel only as a `>=` lower-bound record and keeps both missing colour channels `UNKNOWN`. Every uncensored site keeps the direct Stage-2 CFA channel plus two reconstructed missing channels with finite p95 uncertainty.

The bound estimate range is signed (`-0.0063377907499670982` to `1.2970229387283325`), with 90,484 non-positive estimates retained as signed coordinates rather than clamped light. p95 absolute uncertainty spans `0.00040458221337758005` to `0.084321193397045135`. Censored lower bounds span `1.185887336730957` to `1.2051938772201538`. None of these numbers define a fixed sensor or scene EV ceiling.

## Reconstruction authority versus appearance/detail authority

v1.9 deliberately separates two concepts that must not be conflated. A missing CFA colour can be a `RECONSTRUCTED` Scientific-Master estimate when exact measured-channel reinjection is preserved and the source-class-bound uncertainty model supplies a finite p95 band. Optical MTF calibration is still required for claims about recovered spatial detail, acutance, or sharpening authority, but it is not a prerequisite for recording that a demosaiced colour channel is a reconstruction with uncertainty.

This does not promote reconstructed values to measured evidence. `RECONSTRUCTED` remains distinct from `CALIBRATED_ESTIMATE`, `CENSORED`, and `UNKNOWN`.

## Determinism

The same source was recomputed with full-width vertical execution bands of 192 rows and 257 rows. The complete textual run records, all authority counts, all statistics, every per-channel digest, and the combined field digest were identical. Execution partition therefore did not change scientific content.

The local native audit probe itself is frozen by SHA-256 in `tools/truthraw_hdr_dynamic_authority_binding_v19.py`; the canonical binary field layout is documented in `AUTHORITY_DIGEST_SPEC_V19.md`. All repository dependencies used for recomputation are separately hash-bound.

## What v1.9 closes

The real v1.6 scientific identity tuple can now be instantiated without placeholders:

`source DNG -> decoded CFA -> Scientific Master -> Dynamic Authority Field -> source-bound P3-D65 transform`

The color route remains `SOURCE_METADATA_BOUND_L1_NOT_PHYSICAL_CALIBRATION`. v1.9 creates no new photons, does not recover exact radiance at censored sites, does not make Adobe presentation metadata scientific evidence, and never permits HDR output to write back into the Scientific Master.

v1.9 binds the authority identity but does not persist the ~full-frame authority payload as a repository artifact. The next render stage may deterministically regenerate/stream it from the sealed source and frozen lineage instead of trusting a legacy derivative.
