# Adobe Lightroom HDR controlled tone experiment v1.3

**Status:** RESEARCH — NOT MAIN-PROMOTED

## Purpose

This experiment isolates Lightroom's tone-curve effect from source evidence and, for the P3 pair, from output-gamut changes.

All compared renders descend from the same single HONOR BKQ-N49 tele DNG source and therefore from the same measured CFA evidence. Adobe HDR output remains presentation evidence only.

## Empirical files

### `(8).avif` — P3-D65 / PQ extreme fill stress

- SHA-256: `56856918ceac9077cbec86a399f5d1db07357c92274edc607b05c903ff6ea109`
- 4064x3056
- AVIF / AV1 High / 10-bit 4:4:4
- primaries: P3-D65 (`smpte432`)
- transfer: SMPTE ST 2084 (PQ)
- Lightroom HDR mode: on
- HDR limit: +8.00 EV
- Exposure2012: 0.00
- Whites2012: +2
- all 124 parsed scalar `crs:` settings match `(9).avif`
- tone curve: `(0,0), (17,255), ... , (255,255)`
- extended tone curve: `(0,0), (25,500)`
- MaxCLL: 10000 nits
- decoded luma maximum: 1023/1023
- decoded luma ceiling fraction: 0.8806715265181185
- decoded luma >=1000 fraction: 0.907052764408212

This is a deliberate presentation stress case. It fills the PQ code space so aggressively that most luma samples hit the output ceiling. It does **not** create measured scene dynamic range.

### `(9).avif` — P3-D65 / PQ moderate curve

- SHA-256: `d7b5cd36f16a5500b99f6b61d824322824331e19d9b340dd8dd2cfb3e3ad30d8`
- 4064x3056
- AVIF / AV1 High / 10-bit 4:4:4
- primaries: P3-D65 (`smpte432`)
- transfer: SMPTE ST 2084 (PQ)
- Lightroom HDR mode: on
- HDR limit: +8.00 EV
- Exposure2012: 0.00
- Whites2012: +2
- tone curve: `(0,28), (139,215), (255,255)`
- extended tone curve: `(0,28), (139,215), (329,427)`
- MaxCLL: 4652 nits
- decoded luma maximum: 943/1023
- decoded luma ceiling fraction: 0

### `(6).avif` — Rec.709 / PQ comparison

- SHA-256: `db397e9c10bbc1d81d9ebe2a4338993ad0ba86297717c14d3324c3b1df7068ff`
- primaries: Rec.709
- transfer: SMPTE ST 2084 (PQ)
- HDR limit: +8.00 EV
- Exposure2012: 0.00
- Whites2012: +2
- MaxCLL: 5383 nits
- decoded luma maximum: 959/1023

This file is useful, but it is **not** a tone-only control against the P3 files because both gamut and tone curve differ.

## Controlled result

`(8).avif` versus `(9).avif` is the clean pair. They have:

- the same single RAW/CFA source lineage,
- the same P3-D65 output gamut,
- the same PQ transfer,
- the same 10-bit 4:4:4 pixel format,
- the same HDR limit (+8 EV),
- the same exposure (0 EV),
- the same Whites (+2),
- no difference in the 124 parsed scalar Lightroom `crs:` settings,
- different tone curves.

The only relevant edit-state difference is therefore the tone curve (aside from export timestamp/provenance identifiers).

Changing the tone curve from `(9)` to the extreme `(8)` raises declared MaxCLL from 4652 to 10000 nits, a factor of `2.1496130696474633` or `+1.104076998076231 EV`, while driving 88.067% of decoded luma samples to code 1023.

## Scientific interpretation

This is direct empirical proof of the TruthRaw separation:

`same measured CFA world -> different HDR presentation mapping`

The tone curve can radically redistribute a fixed source into the HDR output address space, including saturating the presentation ceiling, without adding any new measured radiance.

Therefore:

- `HDR Limit` is a presentation boundary, not sensor DR.
- `MaxCLL` is an output/render property, not a source-evidence claim.
- a tone curve may expand/compress or clip presentation luminance without changing Scientific Master authority.
- an output ceiling hit must never be interpreted as recovered highlight evidence.
- source-censored CFA samples remain censored even when the presentation is driven to 10000-nit PQ code values.

## Implementation

- `tools/adobe_hdr_controlled_tone_v13.py`
- `tests/test_adobe_hdr_controlled_tone_v13.py`
- `state/ADOBE_LIGHTROOM_HDR_CONTROLLED_TONE_V13_STATE.json`

## Promotion rule

Do not promote this to main as a scientific dynamic-range expansion. The result certifies an Adobe presentation-control behavior. A TruthRaw-owned HDR projector must preserve source authority, uncertainty and censoring independently of the selected presentation curve.
