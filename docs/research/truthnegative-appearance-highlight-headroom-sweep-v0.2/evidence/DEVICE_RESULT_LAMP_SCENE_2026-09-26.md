# Device evidence — Appearance Highlight Headroom Sweep v0.2 — lamp scene — 2026-09-26

Status: **REAL-DEVICE APPEARANCE/DISPLAY DIAGNOSTIC EVIDENCE**

This is a second real scene, visually dominated by a dark interior and a bright
warm lamp/highlight. It is separate from the earlier white exterior/window
scene used for Appearance Highlight Detail v0.1.

Uploaded audit filename:

`TRUTHRAW_1790439828626_CAM5_ADMITTED_4080x3072_source_draw_appearance_highlight_headroom_sweep_v0_2.json`

Uploaded JSON bytes: `4391`

Uploaded JSON whole-file SHA-256:

`54f24894c6fdf5c17c9dbeb675604ecad9a80c1d9315117de85f01b64c61f0dd`

## Bound identities

- Source Evidence SHA-256:
  `45d44d6dc294267456d17248d117bda1375432e8c08d870e67b5e512cabdc2ba`
- Scientific Master SHA-256:
  `01f22f57e7ae43f7d944adb7660fa96ab7242f3a387e3bd6fd34cbd63f528fd8`
- Authority Field SHA-256:
  `5ad5f49782a7ae71f4685178b92b9a2fb06be95dc9f0bfc1bd80292324251f38`
- TruthNegative state SHA-256:
  `73c908b06d42b57d07461c8d3d6aa84f7496e041ac87c8ce7170479ca3c002c5`
- Headroom sweep SHA-256:
  `1da86458ec1df716fb9102e660fb11d02d419d269709a249edee265f30167b24`

The report states:

- `source_identity_consistent=true`;
- `source_gradient_consistent=true`;
- `source_censor_count_consistent=true`;
- `baseline_is_first=true`;
- `lower_range_preservation_audited=true`;
- `automatic_winner_selected=false`;
- `source_scene_mutated=false`;
- `creates_new_evidence=false`;
- `scientific_writeback_allowed=false`.

## Baseline 100/100

192x145 diagnostic raster:

- sample count: `27,840`;
- mapped exactly to peak: `500`;
- source CENSORED: `0`;
- gamut/display clamp: `1,058`;
- distinct adjacent source-luminance pairs: `55,343`;
- peak-collapsed distinct adjacent pairs: `941`;
- bright peak-collapsed pairs: `941`;
- collapse fraction: `0.017003053683`;
- source gradient sum: `24291.707530206400`;
- mapped gradient sum: `19595.970537907091`;
- gradient retention: `0.806693828070`;
- below-knee samples: `27,340`;
- below-knee changed: `0`;
- `no_highlight_headroom=true`;
- `mapped_peak_collapse_observed=true`.

## Shoulder 90/100

- mapped exactly to peak: `0`;
- source CENSORED: `0`;
- gamut/display clamp: `1,058`;
- peak-collapsed distinct adjacent pairs: `0`;
- collapse reduction versus baseline: `1.0`;
- gradient retention: `0.805214987810`;
- below-knee samples: `27,260`;
- below-knee changed: `0`;
- `no_highlight_headroom=false`;
- `mapped_peak_collapse_observed=false`.

For this tested scene, 90/100 removes the exact luminance-peak many-to-one
collapse measured by this audit while leaving all samples at/below the
candidate knee unchanged by mapped luminance.

## Shoulder 80/100

- mapped at peak: `0`;
- collapsed pairs: `0`;
- collapse reduction versus baseline: `1.0`;
- gradient retention: `0.798226819538`;
- below-knee changed: `0`;
- gamut/display clamp: `1,058`.

## Shoulder 70/100

- mapped at peak: `0`;
- collapsed pairs: `0`;
- collapse reduction versus baseline: `1.0`;
- gradient retention: `0.785890051535`;
- below-knee changed: `0`;
- gamut/display clamp: `1,058`.

## Bounded interpretation

This is now a second materially different real scene in which the current
100/100 Appearance mapping exhibits exact peak-collapse with
`source_censored=0`.

The 90/100, 80/100 and 70/100 candidates remove the exact peak-collapse counted
by this diagnostic.

Within this four-variant sweep, 90/100 is the most conservative non-baseline
candidate with respect to global mapped-gradient retention.

This does **not** by itself promote 90/100 to the default PRO Appearance route.

The unchanged `gamut_or_display_clamp=1058` across all variants is a separate
remaining display/color-boundary phenomenon and must not be confused with the
luminance peak-collapse solved by headroom.

## Next appearance gate

A future appearance-only A/B/Delta diagnostic may compare:

- A = baseline 100/100;
- B = candidate 90/100.

Scientific Master, TruthNegative, N2 and Source Evidence remain unchanged.
