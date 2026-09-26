# TruthNegative Appearance Highlight Headroom Sweep v0.2

Status: **APPEARANCE-ONLY DIAGNOSTIC — NO DEFAULT ROUTE CHANGE**

## Device finding that motivated v0.2

The real-device v0.1 Appearance Highlight Detail audit on the tested Camera-5
DNG measured a 192x145 PRO preview with:

- reference white = 100 nit;
- display peak = 100 nit;
- 502 source samples above reference white;
- the same 502 samples mapped exactly to display peak;
- source CENSORED count = 0;
- 851 distinct adjacent source-luminance pairs collapsed to the same peak;
- maximum hidden adjacent source-luminance difference ~18.9555 nit.

The strongest measured 16x16 tile was at preview (80,48), with 206/256
samples above reference white and 383 collapsed distinct neighbor pairs.

This proves a many-to-one property of the current display mapping for this
tested scene. It does not prove perceptual visibility of every lost numeric
difference and it does not make an external Honor-camera JPEG scientific
evidence.

## v0.2 design

The physical SDR target peak stays fixed at 100 nit.

The sweep changes only where the appearance shoulder begins:

1. baseline_100_100
2. shoulder_90_100
3. shoulder_80_100
4. shoulder_70_100

The existing Free-World Appearance Resolve v0.7 is used unchanged. Under the
current PRO viewing conditions (average surround, adapting/background ratio
1), lowering display reference white below peak reserves finite highlight
headroom inside the same 100-nit output.

Each variant gets its own v0.1 detail audit. v0.2 then binds and compares the
reports without selecting a winner.

## Required metrics

For every variant v0.2 records:

- mapped-at-peak sample count;
- source CENSORED count;
- gamut/display clamp count;
- peak-collapsed distinct adjacent pairs;
- source and mapped luminance-gradient sums;
- mapped-gradient retention;
- collapse fraction of distinct pairs;
- collapse reduction relative to the baseline;
- number of samples at/below the candidate knee;
- number of those lower-range samples whose mapped luminance changed.

The lower-range counter is an explicit guard against solving highlight
collapse by silently moving the region that the candidate promises to leave
unchanged.

## Non-goals

v0.2 does not:

- change Scientific Master;
- change TruthNegative;
- change N2;
- change the normal PRO preview;
- claim MTF or optical detail;
- select an automatic best tone curve;
- use the Honor camera-app image as calibration or evidence.

The Honor image is visual-only context.

## Permanent invariants

- Source Evidence immutable.
- Scientific Master immutable.
- TruthNegative immutable.
- N2 unchanged.
- source_scene_mutated=false.
- creates_new_evidence=false.
- scientific_writeback_allowed=false.
- automatic_winner_selected=false.
