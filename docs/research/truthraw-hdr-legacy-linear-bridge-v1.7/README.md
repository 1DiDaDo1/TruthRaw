# TruthRaw HDR legacy LinearRaw bridge v1.7

Status: **RESEARCH — exact historical full-frame payload verified; modern Scientific Master / Dynamic Authority binding still open**

## Purpose

v1.6 introduced a TruthRaw-owned RGB-to-PQ projection runtime, but correctly requires an exact Scientific Master SHA-256 and matching Dynamic Authority Field SHA-256 before a render can be called a scientific TruthRaw HDR projection.

The Library already contains a historical full-frame reconstructed LinearRaw DNG for the exact Adobe HDR field-trial source. v1.7 binds and audits that real artifact so it can be used as a transport/integration fixture **without relabelling it as the current Scientific Master**.

The governing rule remains:

> Representation may exceed the source; knowledge claims may not exceed the evidence.

## Exact source and exact historical artifact

Sealed source:

- file: `IMG_BNC_TRUTHRAW20260907_094449_565.dng`
- SHA-256: `7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67`
- one physical RAW_SENSOR frame / one evidence root
- HONOR BKQ-N49 tele, 4080x3072, BGGR, WhiteLevel 1023
- source-white censored samples: 217

Historical derived LinearRaw:

- file: `IMG_BNC_TRUTHRAW20260907_094449_565__TRUTHRAW_DERIVED_LINEAR_RAW_v0_4.dng`
- exact file SHA-256: `347cd68ade21f99607028b23ed7cdc0c6b498885d236e9c6443624228747766f`
- exact uncompressed RGB pixel-payload SHA-256: `71b42d01c2e0a54e0807ad671529d7dddebb3e0982c1d7246515b12b49db26eb`
- 4080x3072x3, uint16 LinearRaw
- `PhotometricInterpretation=34892`
- `Compression=1`
- `Orientation=3`
- `BlackLevel=(2048,2048,2048)`
- `WhiteLevel=(34816,34816,34816)`
- 48 strips of 64 rows; total pixel payload 75,202,560 bytes

The real file was materialized from the ChatGPT Library and independently inspected. Its file hash and raw strip-payload hash exactly match the historical release report.

## What the embedded XMP actually proves

The v0.4 DNG embeds:

- `Classification=DERIVED_RECONSTRUCTED_RAW`
- exact source filename and source SHA-256
- exact LinearRaw pixel payload SHA-256
- reconstruction label `research_edge_aware_support_limited_measured_preserving_v47i_logic`
- `UncertaintyStatus=BACKEND_BOUND_UNCERTAINTY_PROSPECTIVE_PASS`
- `MeasuredPhotonClaim=false`
- tone curve / sharpening / output acutance all false

It does **not** embed a modern Scientific Master SHA-256 or Dynamic Authority Field SHA-256. v1.7 treats that absence as an authority boundary, not as metadata to be filled in by inference.

## Quantization boundary

The storage mapping is:

`scene_linear = (stored_code - 2048) / 32768`

Theoretical half-code quantization bound:

`0.5 / 32768 = 1.52587890625e-05`

Historical measured maximum error versus the pre-quantization reconstructed master:

`1.531839370727539e-05`

The dequantized full RGB payload has deterministic float32 SHA-256:

`0def5ca38d3339e48e81435df68007745b2cdfeacd8e36d72ead06dafb44acd0`

Real decoded legacy-payload diagnostics:

- minimum scene component: `-0.00634765625`
- maximum scene component: `1.297760009765625`
- negative RGB components: `41,873`
- RGB components above unity: `125,419`

These values demonstrate that the compatibility payload retains signed and >1 scene coordinates. They do not make the quantized file the authoritative modern float Scientific Master.

## Source-bound P3-D65 bridge

Colorimetric V3 already records the exact source-specific L1 camera-to-XYZ(D50) matrix for this capture. v1.7 freezes the downstream matrix chain:

`camera RGB -> XYZ D50 -> Bradford D65 -> linear P3-D65`

Frozen CameraRGB -> P3-D65 matrix:

```text
[[ 1.9115759182105407,   0.0018793656065187432, -0.31304168608169364],
 [-0.14532939129426314,  1.1695822925561443,    -0.1484403396270572 ],
 [-0.03882691749797302, -0.432943438113552,      2.6768616785331276 ]]
```

Hash-bound transform record SHA-256:

`2105712a1be9950089976ba358afd4b062347680d5e6d8c500462c2e8d541533`

Authority is explicitly:

`SOURCE_METADATA_BOUND_L1_NOT_PHYSICAL_CALIBRATION`

This does not close the independent tele/lens physical color-calibration gate.

Applying that transform to the real quantized compatibility payload produced these integration diagnostics:

- P3 component minimum: `-0.018184110248606337`
- P3 component maximum: `2.618985313022203`
- positive P3 luminance maximum: `1.382045787901244`
- peak positive luminance relative to unity: `0.46680541376074286 EV`

These are **legacy-payload integration diagnostics**, not a sensor-DR measurement and not yet a completed v1.6 scientific HDR render.

## Fail-closed rules

The v1.7 bridge rejects file-hash mismatch, payload-hash mismatch, geometry/storage mismatch, XMP source/payload mismatch, and any historical file that unexpectedly asserts a modern Scientific Master or Dynamic Authority identity.

It also refuses to manufacture either missing modern hash. Therefore:

- the v0.4 compatibility payload may not be promoted to Scientific Master by naming convention;
- its pixel-payload SHA may not be substituted for Scientific Master SHA;
- its source-bound color transform may not be called independent physical calibration;
- it creates no new sensor evidence;
- it may not write back to the Scientific Master;
- it does not by itself satisfy the v1.6 science binding.

## Why this step matters

Before v1.7, the full-frame historical reconstructed RGB existed but was not connected to the modern HDR authority corridor. Now its exact container, pixel payload, signed/over-unity mapping, source XMP binding, and source-bound P3 transform are machine-checkable.

This closes an **integration-evidence** gap while deliberately leaving the scientific identity gap open.

## Next gate

The next legitimate step is still to persist/recompute for this exact source:

1. the deterministic modern float Scientific Master and its exact SHA-256;
2. the matching per-channel Dynamic Authority Field and exact SHA-256;
3. the complete source/master/authority/color binding;
4. then stream the real 4080x3072 master through v1.6 and encode a TruthRaw-owned P3-D65/PQ HDR exchange file.

Until those identities exist, any PQ experiment using the historical v0.4 payload must remain labelled `LEGACY_COMPATIBILITY_INPUT` / algorithm-integration evidence rather than a completed scientific HDR render.
