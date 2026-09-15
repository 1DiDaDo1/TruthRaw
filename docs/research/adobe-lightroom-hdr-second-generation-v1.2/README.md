# Adobe Lightroom HDR second-generation render v1.2

Status: **RESEARCH — NOT MAIN-PROMOTED**

## Purpose

Record the uploaded `IMG_BNC_TRUTHRAW20260907_094449_565 (4).avif` as a real second-generation Adobe HDR presentation observation and compare it against the frozen first-generation Adobe controls.

The immediate parent route of this AVIF is deliberately **not asserted** from embedded metadata alone. The file preserves `RawFileName=IMG_BNC_TRUTHRAW20260907_094449_565.dng`, but Adobe-rendered derivatives can preserve prior Camera Raw metadata without uniquely identifying whether the immediate parent was the original DNG, the Adobe-rewritten DNG, or a re-imported TIFF.

## Real observation

- SHA-256: `552a463095103d9d74baec594994a438c070e9c57beadae1b2f503b847d5bc9f`
- bytes: `10,641,173`
- geometry: `4064 x 3056`
- main color stream: AV1, 10-bit, 4:4:4
- transfer: SMPTE ST 2084 / PQ
- primaries: P3-D65 (`smpte432`)
- gain-map stream present
- `tmap` brand present
- embedded Adobe Lightroom: 11.5.22 Android
- HDR edit mode: on
- HDR limit: `+8.00 EV`
- Exposure2012: `0.00`
- Whites2012: `+2`
- GMap Content Light Level MaxCLL: `2878 nits`

Decoded main-stream diagnostic (full-range 10-bit luma code; PQ-EOTF conversion is a transport diagnostic, not scene radiance):

- max luma code: `889 / 1023` (~2932 nit PQ diagnostic)
- p99 luma code: `841 / 1023` (~1904 nit PQ diagnostic)
- p99.9 luma code: `857 / 1023` (~2199 nit PQ diagnostic)
- p99.99 luma code: `862 / 1023` (~2300 nit PQ diagnostic)

## Important finding: nominal controls are not render identity

The first tone-expanded control and the new AVIF both report:

- HDR limit `+8.00 EV`
- Exposure `0.00`
- Whites `+2`

But their tone curves are not the same.

First-generation tone-expanded control:

- ToneCurve: `(0,12) (152,228) (255,255)`
- ExtendedToneCurve: `(0,12) (152,228) (386,500)`
- MaxCLL: `5259 nits`
- max decoded luma code: `956`

Second-generation observation:

- ToneCurve: `(0,0) (133,196) (255,255)`
- ExtendedToneCurve: `(0,0) (133,196) (332,446) (500,500)`
- MaxCLL: `2878 nits`
- max decoded luma code: `889`

So the new export peak is about `0.5473x` the earlier tone-expanded MaxCLL, or about `-0.8697 EV`, despite equal HDR Limit / Exposure / Whites scalar settings.

This proves that **HDR Limit + Exposure + Whites are insufficient to identify an Adobe render state**. The tone curve itself and the encoded output must be hash-/measurement-bound.

## Same-gamut control

Compared against the unedited P3-D65 control (`MaxCLL 1383 nits`), the new P3-D65 observation reaches `2878 nits`, a factor of about `2.0810x` or `+1.0573 EV` of presentation peak.

This remains presentation expansion. It is not new sensor evidence.

## Scientific boundary

The new AVIF does not:

- create another physical exposure,
- increase measured CFA dynamic range,
- recover exact radiance from the 217 source-white censored samples,
- modify the Scientific Master,
- promote Adobe tone mapping or gain-map data to source authority.

Permanent rule remains:

> Representation may exceed the source; knowledge claims may not exceed the evidence.

## Immediate-parent caution

The exact immediate parent route is left unresolved until independently bound. Do not call this a TIFF round-trip or Adobe-DNG round-trip solely from the derivative XMP. A future trial must bind the parent file SHA before import and record the exported child SHA.

## Next gate

Build a TruthRaw-owned HDR presentation manifest that binds:

1. immutable source DNG SHA,
2. decoded CFA SHA,
3. Scientific Master SHA,
4. Dynamic Authority Field SHA,
5. exact Adobe/interchange parent SHA when Adobe is used,
6. tone-curve points and HDR scalar controls,
7. encoded output SHA and PQ/gamut metadata.

That will make a round-trip provenance claim independent of Adobe's retained `RawFileName` string.
