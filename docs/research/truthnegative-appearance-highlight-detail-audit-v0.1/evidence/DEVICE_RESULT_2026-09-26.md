# Device evidence — Appearance Highlight Detail v0.1 — 2026-09-26

Status: **REAL-DEVICE APPEARANCE/DISPLAY DIAGNOSTIC EVIDENCE**

Source device route: admitted physical Camera-5 DNG through the existing D.RAW
Scientific Master -> TruthNegative Continuous -> Free-World Appearance v0.7
preview route.

Uploaded audit filename:

`TRUTHRAW_1790437787770_CAM5_ADMITTED_4080x3072_source_draw_appearance_highlight_detail_audit_v0_1.json`

Uploaded JSON bytes: `61661`

Uploaded JSON whole-file SHA-256:

`ff9ec10c6012cc75578b21c9c8df694817086552a0474022d512ccb19e95e6df`

## Bound identities reported by the audit

- Source Evidence SHA-256:
  `0aceb48f9f66dc4192935a097176b129bd15bc50586861487144c3f4debbf772`
- Scientific Master SHA-256:
  `07f94e6b7745626e07961e123215ec9d785834e31ea9a23aa808cda8b299a385`
- Authority Field SHA-256:
  `71277746d8cd7e0c3b63d7f8e41ff987faab4d5019fa4b0dd50f4880921ce78c`
- TruthNegative state SHA-256:
  `6727c51962a602925e287536a0b6c5399703890ea932f0f06a7bd51246a6fa73`
- Appearance state SHA-256:
  `e649780c36b29a477fa63268bb722c219444481e18105af2b66bbb91d30a5d98`
- Appearance Highlight Detail audit SHA-256:
  `e448f1d5faa453519d72529c14338ae95de633806db4f846cc45d3b7a3b40536`

## Exact tested display configuration

- preview raster: `192 x 145`
- tile edge: `16`
- display reference white: `100 nit`
- display peak: `100 nit`
- `no_highlight_headroom=true`
- `mapped_peak_collapse_observed=true`

The normal PRO bridge therefore exercises the no-headroom branch in
Free-World Appearance Resolve v0.7 for this display configuration.

## Global result

- samples: `27,840`
- source samples above reference white: `502`
- mapped exactly to display peak: `502`
- source CENSORED samples: `0`
- gamut/display-clamp flagged samples: `768`
- distinct adjacent source-luminance pairs: `55,343`
- distinct adjacent pairs collapsed to the same display peak: `851`
- bright collapsed distinct pairs: `851`
- peak-collapse fraction of distinct neighbor pairs: about `1.5377%`
- source absolute luminance-gradient sum: `136222.244756152795`
- mapped absolute luminance-gradient sum: `131815.222728787980`
- mapped/source gradient retention: about `0.9676483`
- aggregate gradient reduction: about `3.2352%`
- source-gradient hidden inside collapsed peak pairs:
  `1894.091180580895`
- largest single hidden adjacent source-luminance difference:
  `18.955463394113 nit`

## Strongest measured highlight tile

Preview tile `x=80, y=48, 16x16`:

- source above reference white: `206 / 256`
- mapped at peak: `206 / 256`
- source CENSORED: `0`
- gamut/display clamp: `219 / 256`
- distinct adjacent pairs: `480`
- peak-collapsed distinct adjacent pairs: `383 / 480`
- collapsed-pair fraction: about `79.79%`
- source gradient sum: `1868.532245895969`
- mapped gradient sum: `1202.025966854790`
- hidden source gradient inside collapsed pairs: `506.086894312670`
- maximum hidden adjacent difference: `8.114692339262 nit`

Nearby tiles also carry collapse, including `x=96,y=48` with 163 collapsed
pairs and `x=128,y=48` with the global maximum hidden adjacent difference
of about 18.9555 nit.

## Interpretation boundary

This result proves a **many-to-one property of the current Appearance/Display
mapping** for the tested raster.

It does **not** prove:

- that every collapsed numeric difference is individually visible to a human;
- that the external Honor-camera rendering is physically correct;
- that the Honor JPEG is calibration;
- that the source sensor itself is unclipped everywhere outside this preview;
- MTF, PSF or optical-resolution claims.

Within this tested Appearance audit, `source_censored=0`. Therefore the
measured display-stage peak collapse is distinct from source CENSORED
authority.

## Honor camera image

A normal Honor camera-app image of the bright exterior structure was supplied
as a visual comparison. It demonstrates that a finished rendering can retain
visible structure in the bright white surface.

Classification:

`VISUAL_REFERENCE_ONLY`

It is not Source Evidence, not calibration, not a color target and not a
scientific authority source for D.RAW.

## Consequence

Do not weaken N2 structure/censor protection to solve this appearance problem.

The next bounded experiment is Appearance Highlight Headroom Sweep v0.2:
keep the SDR peak fixed at 100 nit while testing shoulders beginning at
100, 90, 80 and 70 nit through the unchanged v0.7 Appearance resolver.

Scientific Master, TruthNegative, N2 and normal PRO output remain unchanged.
