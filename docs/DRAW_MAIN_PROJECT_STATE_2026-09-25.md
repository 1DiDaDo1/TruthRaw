# D.RAW main project state — 2026-09-25

Status: **CURRENT ACTIVE INTEGRATION**

Active integration branch:

`integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master`

Code-bearing checkpoint promoted from the Free-World research line:

`e1fde59a9edf097fe2ce3fdf996fe181a94ac5e0`

## Current architecture

The Free-World output-pixel research arc v0.2 through v0.7 is now compiled into the Android main native library:

```text
sealed Source Evidence
  -> Scientific Master
  -> Continuous 2D Scene Field        (v0.2)
  -> Scientific/Open Scene binding    (v0.3)
  -> Deep Scene contributions         (v0.4)
  -> object/region + geometry/radiometry authority (v0.5)
  -> Light Transport state            (v0.6)
  -> Appearance / Viewing / Display Resolve (v0.7)
  -> finite visible output
```

Permanent rule:

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

The promotion does not grant new evidence. The downstream layers keep:

- `physicalFrameCount = 1`;
- `independentEvidenceCount = 1`;
- `createsNewEvidence = false`;
- `scientificWritebackAllowed = false`.

## v0.7 appearance boundary

The v0.7 layer makes viewing and display state explicit rather than hiding it inside arbitrary RGB curves.

It binds:

- scene colorimetry identity;
- viewing-condition identity;
- display-target identity;
- appearance-policy identity;
- output identity.

Reference display encodings:

- Linear normalized;
- sRGB;
- PQ / ST 2084.

Changing viewing conditions or the display target changes the visible derivative, not the source-scene identity or scientific authority.

## Android route UI

The launcher now uses the architecture directly:

- **D.RAW PURE -> Scientific View**
- **D.RAW ADVANCED -> Appearance / Restoration View**
- **D.RAW PRO -> Open Scene / Light Transport**

File and camera input labels adapt to the selected route.

The main processing screen shows the same route semantics.

ADVANCED is explicitly downstream appearance/restoration. PRO exposes the Open Scene / Deep Scene / light-transport architecture.

## Important integration boundary

The v0.2-v0.7 native modules are now part of the Android main build, but existing validated scientific/export paths are **not silently rerouted through v0.7**.

A production pixel/export route must opt into the new resolver through a dedicated validated bridge.

This preserves previously validated Scientific Master, TruthNegative, Restoration and Unified Output Preview behavior while allowing the new architecture to become the shared downstream framework.

## Green validation

Main merge checkpoint `e1fde59a9edf097fe2ce3fdf996fe181a94ac5e0`:

- Android signed ARM64 build run `36079724937`: **SUCCESS**
- Scientific Master F64 + Color Audit run `36079724798`: **SUCCESS**
- Open Scene Field/local authority run `36079724888`: **SUCCESS**
- Unified Output Preview run `36079725063`: **SUCCESS**

Research validation before promotion:

- Free-World v0.2-v0.7 GCC: **PASS**
- Free-World v0.2-v0.7 Clang: **PASS**
- ASan/UBSan: **PASS**

## Current APK artifact

GitHub Actions artifact:

- artifact ID: `10841122875`
- artifact name: `truthraw-v0-84-2-compute-router-debug-arm64`
- artifact ZIP digest: `sha256:d3a8fcda7f412540773cd1f89fe81ff12132e5b4d16a59e4214747277e3b913b`
- APK size: `6297315` bytes
- extracted APK SHA-256: `23c3c3fcf0c119f517ab102a30d8eb896da1156bcb9d0ab3273934367ac87448`

Stable development signing certificate SHA-256:

`a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`

The stable signing identity remains development/test-only and unchanged.

## Next implementation boundary

The next safe production step is not to invent a new Scientific Master. It is to create explicit bridges that consume the already-bound Open Scene/light-transport/appearance state for selected ADVANCED/PRO preview/export paths while preserving route authority and exact source lineage.
