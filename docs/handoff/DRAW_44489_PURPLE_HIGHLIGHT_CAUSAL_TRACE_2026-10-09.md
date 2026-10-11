# D.RAW 44489 — Purple Highlight Causal Trace — 2026-10-09

Status: **SOURCE-BOUND CAUSAL EVIDENCE / NO SCIENTIFIC PROMOTION / NO PRODUCTION COLOUR CHANGE**

Repository: `1DiDaDo1/TruthRaw`  
PR: #131 (`feat/draw-workspace-free-raster-v01`)  
Trace baseline head before this documentation commit: `69775055fda734d281a75fbc50979099dda4dea0`

Permanent evidence law remains unchanged:

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

No source bytes, Scientific Master values, reconstruction authority, Warm Illuminant behavior, Natural Light Local Field Tone, HDR/detail/acutance, SAFE A/B semantics or scientific writeback state were changed to obtain this result.

## 1. Exact source-bound pair

The decisive comparison uses two outputs carrying the **same capture id**:

- ADVANCED full-resolution JPEG: `DRAW_CAPTURE_1791566217859_tele_4080x3072_draw_advanced_fullres.jpg`
  - bytes: `15,747,103`
  - SHA-256: `56120bcdc378faf10c0243a5c9245bef83eb75389f84b89976b9c9ea7dbe4b68`
- PURE Float32 scientific projection: `DRAW_CAPTURE_1791566217859_tele_4080x3072_draw_pure_float32_v0_63.dng`
  - bytes: `167,036,277`
  - SHA-256: `5695bdd3381a030d9cc13ec1e04fb2c7dae523e1ef4737fcf98630352e5ee6cc`

The PURE private contract identifies the stored primary as:

`TRUTHRAW_PURE_FLOAT32_XYZ_D50_LINEAR_DNG_PROJECTION`

with:

- `representation_only=1`
- `scientific_master_modified=0`
- `appearance_applied=0`
- sealed source SHA-256 `c2269585a4e071f75202e9e39a36bd0da22a32082812e1a48c42dac3002605c8`
- Scientific Master SHA-256 `526677c26b300a86047d6a33b89bab48e0f6dc31dbd4d78aa9a24eb582280ddd`
- `output_channel_authority_bound=1`
- `censored_support_pixels=120956`
- `censored_channels=362868`
- `scientific_writeback_allowed=0`
- `creates_new_evidence=0`

`362868 == 3 * 120956`: for the support-censored output pixels the conservative output authority classifies all three output channels as CENSORED.

## 2. The previous negative-G hypothesis is falsified for this frame

The current streaming production path does contain an early per-channel non-negativity clamp after:

`camera RGB -> XYZ D50 -> linear sRGB -> NeutralReferenceAppearance -> tone scale`

The clamp is real and remains an architectural item to review separately.

However, applying the **exact current XYZ D50 -> linear sRGB matrix** to the complete 4080x3072 PURE primary from this same capture yields:

- negative R components: `0`
- negative G components: **`0`**
- negative B components: `773`
- `G < 0 && R > 0 && B > 0`: **`0`**
- `G < 0 && R >= 0.75 && B >= 0.75`: **`0`**

Therefore a negative green component being hard-clamped to zero cannot be the primary cause of the broad purple highlight in capture `1791566217859`.

This also explains why the later APP15 gamut diagnostic reported:

- `early_negative_any=0`
- `early_g_negative_rb_positive=0`
- `early_g_boundary_input_rb_positive=0`
- `early_g_boundary_input_rb_high=0`

The old diagnostic was downstream of the streaming clamp, but the source-bound PURE projection independently demonstrates that G is not negative in the problematic region even before that loss boundary.

## 3. Purple chromaticity already exists before the negative clamp

For the previously used 192x192 CENSOR/highlight audit rectangle at source coordinates:

`x=1920, y=1344, width=192, height=192`

median linear-sRGB values derived from the PURE XYZ-D50 primary are approximately:

- R = `1.1235989`
- G = `0.8278903`
- B = **`3.8560086`**

There are **zero negative-G pixels** in this crop.

The corresponding ADVANCED JPEG crop has median encoded RGB approximately:

- R = `156`
- G = `128`
- B = `238`

Thus the strong blue/magenta direction is already present in the scene-linear colour ratio before the suspected negative-G clamp.

## 4. Large source-bound purple component

Across the complete frame:

- pixels with linear-sRGB B > 3.0: **`91,450`**
- median linear RGB for that class: approximately `(1.12249, 0.82698, 3.85810)`
- median ADVANCED JPEG RGB at those exact coordinates: **`(157, 128, 238)`**

Connected-component analysis of `B > 3.0` finds one dominant component:

- pixels: **`83,471`**
- source bbox: approximately `x=1961..2311`, `y=1312..1663`
- median ADVANCED JPEG RGB: **`(157,128,238)`**

This is the large flat purple lamp/highlight body visible on the real-device output.

A 10-pixel exterior ring around that dominant component, restricted to finite non-purple neighbouring content, is instead warm:

- median linear RGB approximately `(0.70747, 0.52592, 0.39504)`
- median ADVANCED JPEG RGB approximately **`(214,174,152)`**

So the broad censored interior and the nearby reliable boundary have radically different chromaticity: the interior is violet/purple while the immediate local boundary is warm.

## 5. Why PURE looks nearly white while ADVANCED becomes purple

`unified_output_preview_v0_1` converts camera-native/XYZ input to linear sRGB and then performs direct display clipping per channel:

`srgb_u8(linear) -> clamp(linear, 0, 1) -> sRGB transfer`

For a representative censored value near `(1.12,0.83,3.86)`, direct display clipping makes R and B both hit 1.0 while G remains high, producing a near-white / pale-pink preview.

The ADVANCED streaming path instead performs luminance/tone scaling with common RGB gain before the later presentation stages. A common gain preserves the underlying chromaticity. Once the extended values are brought below display white, the previously hidden ratio becomes visible as purple.

Therefore the PURE/ADVANCED visual difference is expected from the two display boundaries; it is not evidence that Scientific Master itself was rewritten between modes.

## 6. Why the previous highlight candidate missed the real failure class

The removed candidate looked for a numerically near-white presentation signature such as:

- max channel > 0.92;
- R and B both high;
- G low relative to R/B;
- or near-neutral min-channel/luma gates.

Real-device evidence shows the broad failure class is different:

- it originates from **source-censored/highlight authority**;
- after tone mapping its display luminance is no longer necessarily near white;
- its dominant purple body can be around encoded `(157,128,238)`;
- the previous numerical near-white signature therefore selects only a tiny subset of the actual affected region.

This explains why the old support-aware chroma contraction changed only a small number of highlight pixels and failed to remove the broad purple body.

The current head correctly keeps `presentation_highlight_chroma_rolloff_v0_1` as bit-preserving observation-only telemetry.

## 7. Correct causal interpretation

The strongest current explanation is:

`RAW/CFA clipping/censoring -> reconstruction / colour transformation produces a finite but chromatically unreliable full-colour value -> Scientific Master correctly retains its numeric value with CENSORED authority -> ADVANCED tone scaling preserves that unreliable chromatic ratio -> the ratio becomes visible as broad purple presentation colour`

This is different from:

`negative G -> clamp G to zero -> purple`.

The latter mechanism exists in code but is not the cause of this real-device frame.

The project must therefore not solve this by pretending the censored chromaticity is known. The correct downstream problem is **how to render CENSORED colour conservatively in Appearance while preserving luminance/structure and any separately supported illuminant context**.

## 8. Next candidate direction

Do **not** reintroduce the old R/B-high G-low detector and do not add global desaturation.

The next candidate should be authority-driven:

1. keep historical centre-only `censored` semantics for restoration/HDR/Natural-Light Local Field;
2. keep the stricter reconstruction-support censor authority isolated to the final highlight-colour boundary;
3. use censor authority itself, not presentation brightness or hue, to decide that chromaticity is unreliable;
4. preserve luminance/structure;
5. allow the already accepted source-white Warm Illuminant stage to remain downstream so a conservative censored fallback does not erase legitimate warm-illuminant appearance;
6. use support density / censor fraction rather than a single Boolean if possible, so an interior fully censored region can be treated more conservatively than a boundary with only one censored support sample;
7. no scientific recovery claim: result remains `APPEARANCE_ONLY / DERIVED_PRESENTATION_OUTPUT`;
8. PURE remains isolated;
9. no source/Scientific-Master mutation, promotion or scientific writeback;
10. physical A/B validation remains mandatory before acceptance.

A promising implementation form is an **authority-bound censored chroma fallback** before Warm Illuminant retention and after the stages that must keep their historical censor semantics. It should contract only the unsupported chroma component toward the D50 neutral axis in proportion to reconstruction-support censor fraction; Warm Illuminant retention can then reintroduce its separately source-bound bounded warmth. This is a candidate design, not yet a physical PASS.

## 9. External scientific consistency

This interpretation is consistent with established RAW/HDR image-formation knowledge: saturated sensor samples lose information; colour in clipped regions cannot be treated as fully observed merely because a deterministic reconstruction/matrix produces finite RGB. Published HDR work likewise treats over-exposed RAW regions as information-deficient and relies on remaining less-saturated channels or additional support where available. D.RAW remains stricter: no learned/generative inference is admitted as scientific evidence, and where colour support is absent the authority remains CENSORED/UNKNOWN.

## 10. Current conclusion

**The broad purple tele highlight is now source-bound to censored chromaticity that exists before the negative-G clamp. The negative-G clamp is not the causal mechanism for capture `1791566217859`. The old near-white chroma guard failed because tone mapping had already moved the censored colour away from its assumed numeric white-boundary signature. The next safe direction is authority-bound Appearance handling of censored chroma, with luminance/structure retained and the accepted Warm Illuminant stage preserved downstream.**
