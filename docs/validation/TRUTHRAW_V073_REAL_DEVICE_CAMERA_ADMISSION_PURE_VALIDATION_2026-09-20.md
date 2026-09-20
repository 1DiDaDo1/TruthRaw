# TruthRaw v0.73 real-device Camera-5 admission + PURE validation — 2026-09-20

## Scope

Real-device validation on HONOR BKQ-N49, Android 17, app:

`0.38-v0.73-camera-source-admission-tn3`

Installed APK SHA-256:

`f85dab558fe8511c7555c1382797183b96098b2bac18396c639cf5544f15fa41`

The validation covers the product path:

`launcher -> Gebruik camera -> Camera-5 capture -> admitted 4080x3072 processing DNG -> Main House -> Finalized Scientific Preview -> PURE Float32 DNG`.

It does not prove native 200 MP sensor geometry, untouched ADC values, optical 200 MP resolving power or independently calibrated physical colour.

## Empirical Main-House admission

Uploaded empirical JSON:

`TRUTHRAW_1789911850851_CAM5_ADMITTED_4080x3072_source_truthraw_raw_ingress_empirical_v0_2.json`

Key facts:

- ingress route: `CAMERA_CAPTURE`;
- admitted DNG: `TRUTHRAW_1789911850851_CAM5_ADMITTED_4080x3072_source.dng`;
- admitted DNG bytes: `25,097,788`;
- admitted DNG SHA-256: `a2e3742c0616a9cd28cb8d2de347363dfb7ab852d9a7d734b8421911d36aa359`;
- source stable across empirical wrapper: true;
- colour probe stable across empirical wrapper: true;
- preview source dimensions: `4080x3072`;
- preview outcome: `READY`;
- physical frame count: `1`;
- independent evidence count: `1`;
- full RAW materialized: false.

Colour metadata is source-bound rather than independently physical:
- metadata form: `DUAL_ILLUMINANT_V0_2`;
- ForwardMatrix used: true;
- CameraCalibration present/signature-matched/applied: true;
- resolved white temperature: `4652.272 K`;
- independent physical colour claim allowed: false.

Runtime:
- finalized-tile-preview wall time: `87,956.759 ms`;
- worker CPU: `87,697.665 ms`;
- PSS before/peak/after: `112,447 / 115,189 / 111,639 KiB`;
- thermal start/peak/end: `NONE / NONE / NONE`;
- visible UI p95: `16.600 ms`.

Interpretation: current processing is strongly CPU-bound while memory and frame pacing remain controlled. This is a performance issue, not evidence-authority failure.

## Independently parsed uploaded PURE DNG

Uploaded file:

`TRUTHRAW_1789911850851_CAM5_ADMITTED_4080x3072_source_truthraw_pure_float32_v0_63.dng`

Whole-file bytes:

`151,022,004`

Whole-file SHA-256:

`bc8ca31aad0c50f202eb5c98df7c2acdf0cf352449d40f7620e784a5ab4f0bed`

TIFF/DNG structure:
- width: 4080;
- height: 3072;
- samples per pixel: 3;
- BitsPerSample: 32 / 32 / 32;
- SampleFormat: IEEE Float for all 3 channels;
- PhotometricInterpretation: LinearRaw;
- compression: none;
- tile size: 64x64;
- tile count: 3072;
- tile byte count: 49,152 each.

The private TruthRaw contract decodes as:

`TRUTHRAW_PURE_SELF_BINDING_V0_63`

Embedded source SHA-256 exactly matches the empirical admitted DNG:

`a2e3742c0616a9cd28cb8d2de347363dfb7ab852d9a7d734b8421911d36aa359`

Embedded Scientific Master SHA-256:

`3d16fe6a76d2cce46b41fe54d020d1018ccecf564c569e4ef78ea22841bf431e`

Zero-Line:
- SHA-256: `b15777508f6c29da54db12d0c6e989748a43df758051869f64cc92af646a9991`;
- mode: `SELF_GAUGE`;
- exact L0 f64 bits: `0x3fa44fb500000000`;
- decoded L0: `0.03967061638832092`;
- cross-scene comparable: false;
- absolute physical units: false.

Scene-scale:
- SHA-256: `90cca2a7932116f9059174e50028817a0ecc66df182d107eda92b1955cb6fdaf`;
- id: `TRUTHRANGE_SELF_GAUGE_STAGE2_V0_2`;
- gain map applied exactly once: true.

Technical Backplane:
- serialized bytes: 180;
- CRC scope: prefix 176 bytes;
- stored CRC32: `0xa7ed8a97`;
- independently recomputed CRC32: `0xa7ed8a97`;
- CRC validation: PASS.

Runtime reconstruction backend:

`research_edge_aware_support_limited_measured_preserving_v47i`

Precision policy remains the controlled F32 scientific master/projection policy after F64 branch-sensitive reference computation.

Valid raster statistics from the 4080x3072x3 Float32 payload:
- finite components: `37,601,280`;
- non-finite: `0`;
- negative components: `294,148`;
- components >1: `0`;
- minimum: `-0.011165712960064411`;
- maximum: `0.7907839417457581`.

The absence of >1 samples in this one scene does not change the contract: the Float32 representation remains capable of preserving >1 values when they occur.

## Result

### CLOSED on this real-device sample

- `Gebruik camera` reaches a real camera capture path.
- The product route reaches an admitted `4080x3072` camera-origin DNG.
- Main House processes that DNG as `CAMERA_CAPTURE`.
- Source ancestry remains visible and authority is not inherited.
- Finalized Scientific Preview releases at `4080x3072`.
- PURE Float32 DNG is generated and self-bound.
- Source SHA, Scientific Master SHA, Zero-Line, L0, scene-scale and 180-byte Backplane are embedded.
- Backplane CRC independently verifies.
- physical frame / independent evidence remains `1/1`.

### Still open

- native ADC / physical sensor geometry mechanism;
- full-population 16320x12288 scientific admission;
- independent physical colour calibration;
- camera-domain PTC/noise/uncertainty promotion;
- optical MTF/SFR/CA/shading/flare calibration;
- performance optimisation of the ~88 s finalized preview path.

## UI finding

The compact launcher now fits the output row inside the viewport, but the two-line `TRUTHRAW PURE` and `TRUTHRAW ADVANCED` title rows still clip the second title line.

A v0.73.1 presentation-only fix increases the explicit title-row height/minLines/font padding. It does not alter camera admission, Scientific Master, Zero-Line, Backplane, PURE math, TN-3, restoration or projection science.
