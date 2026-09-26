# TruthNegative Appearance Highlight Detail Audit v0.1

Status: **APPEARANCE/DISPLAY DIAGNOSTIC ONLY**

## Purpose

This audit tests whether the existing PRO Appearance/Display resolve maps
spatially distinct scene luminances onto the same display peak.

It does not modify the Appearance transform and it does not use any external
camera JPEG as scientific evidence.

A normal camera-app image may be used only as a human visual reference for the
kind of highlight structure that is desirable in a final presentation.

## Why this audit exists

The current neutral PRO preview configures:

- scene reference white: 100 nit;
- display reference white: 100 nit;
- display peak: 100 nit;
- sRGB transfer.

In FreeWorldAppearanceResolve v0.7, peak <= reference white takes the direct
clamp path. Therefore scene luminances above the display peak can be mapped
many-to-one to the same 100-nit value before RGB gamut clamping and before
sRGB encoding.

The pre-existing gamutOrDisplayClampApplied flag is evaluated later on the
display RGB channels. It is therefore not a complete detector for this
luminance-stage collapse.

## Exact measurements

For the complete audit raster and for 16x16 target tiles, v0.1 records:

- source samples above display reference white;
- samples mapped exactly to display peak;
- source CENSORED presence;
- existing gamut/display-clamp flag count;
- horizontal and vertical adjacent pairs;
- adjacent pairs with distinct source luminance;
- distinct source pairs whose two mapped luminances both equal display peak;
- the subset of those pairs touching source luminance above reference white;
- source and mapped absolute luminance-gradient sums;
- total and maximum source-luminance difference hidden inside peak-collapsed
  pairs.

There is no perceptual threshold and no MTF claim. A peak-collapsed pair is
only the exact statement that two numerically distinct neighboring source
luminances became the exact same mapped peak luminance.

Tile pair metrics exclude cross-tile boundaries; global pair metrics include
all horizontal and vertical adjacency.

## Interpretation boundary

mapped_peak_collapse_observed=true proves a many-to-one property of the
current Appearance/Display transform for the tested raster. It does not prove
that every collapsed difference would be perceptually visible, nor does it
claim that an external camera application's rendering is physically correct.

source_censored remains separate. This allows a future device test to
distinguish source censoring from display-stage flattening.

## Permanent invariants

- Direct CFA immutable;
- Scientific Master immutable;
- TruthNegative immutable;
- N2 unchanged;
- Appearance policy unchanged by this audit;
- source_scene_mutated=false;
- creates_new_evidence=false;
- scientific_writeback_allowed=false.
