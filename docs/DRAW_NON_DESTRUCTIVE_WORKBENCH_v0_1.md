# D.RAW Non-Destructive Workbench v0.1

**Status:** binding product/architecture invariant — no scientific promotion  
**Date:** 2026-10-06  
**Continuation code:** `44489`

## Core rule

The D.RAW Workbench is non-destructive for **both scientific sources and ordinary raster images**.

Canonical model:

`IMMUTABLE_SOURCE + REVERSIBLE_EDIT_STATE -> PRESENTATION / OUTPUT`

and:

`EXPORT -> NEW_DERIVED_OUTPUT`

Never:

`SOURCE -> EDIT -> OVERWRITE SOURCE`

## What is immutable/read-only

Workbench editing must never mutate or overwrite:

- sealed RAW/CFA evidence;
- D.RAW Observation Records;
- the Scientific Master;
- source RAW/DNG files;
- imported JPEG/PNG/WebP source files;
- previously established provenance/authority facts.

JPEG/PNG/WebP being presentation rasters does **not** make them disposable. The imported file remains the immutable/read-only base of that edit session.

## What an edit actually is

An edit is a separate reversible downstream instruction. Examples include:

- exposure/brightness appearance;
- display black point;
- display white point;
- highlight roll-off/compression;
- shadows/midtones/contrast;
- warmth/tint/colorfulness/saturation/vibrance;
- detail/texture/appearance sharpening;
- crop/framing;
- rotation;
- Free Raster x/y placement;
- Free Raster scale;
- output/view transform;
- optional authority-overlay visibility.

These instructions are not new evidence and do not alter the source's scientific meaning.

## Reset, disable and history semantics

Every editable instruction must be addressable separately so it can be:

- changed;
- disabled;
- removed;
- reset;
- re-rendered from the same source.

A full reset means the same immutable source with no workbench edit operations. It must not depend on recovering pixels from a previously edited/exported raster.

## RAW / DNG semantics

For a RAW/DNG-backed D.RAW observation:

- sealed source evidence remains unchanged;
- Scientific Master remains unchanged by workbench UI edits;
- appearance settings remain downstream;
- Free Raster transformations remain output/view transformations;
- no UI control may silently write appearance values back into scientific state;
- export produces a new derivative and records/retains its lineage to the source/output state.

Scientific reconstruction/calibration remains governed by its own admitted scientific contracts. Calling something a workbench edit can never bypass those contracts.

## JPEG / PNG / WebP semantics

For an imported presentation raster:

- the source file remains read-only;
- edits are stored independently from the source pixels;
- live preview may be re-rendered many times without replacing the source;
- JPEG is not repeatedly decode→edit→re-encode→overwrite as the working state;
- a new JPEG compression step happens only when the user explicitly exports a new JPEG derivative;
- PNG/WebP exports likewise create new derivative files rather than replacing the source by default.

External raster authority remains:

`EXTERNAL_PRESENTATION_RASTER / PRESENTATION_ONLY`

## PURE / ADVANCED / PRO

PURE, ADVANCED and PRO are downstream routes/views over the same admitted source/scientific core. Switching routes must not duplicate, replace or mutate the underlying source.

- **PURE:** scientific-first/neutral downstream presentation.
- **ADVANCED:** reversible photographic appearance stack.
- **PRO:** shared core plus expert/research/output controls.

The route is part of output/edit state, not a selector for a different truth.

## Free Raster

Free Raster is non-destructive by construction:

- pan changes view/output state;
- zoom changes view/output state;
- x/y position changes projection state;
- scale changes projection state;
- crop/framing changes downstream output state;
- `Preview 1:1` changes display mapping only.

None of these operations creates a measured sample, changes CFA evidence, increases optical resolution, or changes Scientific Master authority.

## Preview-independent sibling output contract

Full-resolution JPEG is a sibling consumer of the admitted D.RAW source/scientific output cable. The UI preview is **not** a prerequisite, pixel source, evidence source or authority source for that export.

The current runtime binding freezes the complete downstream output context before Android's document picker opens:

- source job ID;
- source URI;
- PURE / ADVANCED / PRO route;
- route/appearance flags;
- downstream orientation;
- explicit derived-output safety flags.

When the picker returns, that frozen binding is revalidated fail-closed. A source/job/URI switch, route change, appearance-setting change or orientation change invalidates the pending JPEG request instead of silently retargeting it. Explicit source replacement or reprocessing also clears the pending binding.

The actual JPEG renderer remains full-resolution and source/job-bound. A small JPEG decoded **after** successful commit is presentation feedback only; it is not reused as the export's pixel source or scientific authority.

The safety state remains:

- `previewRequired=false`;
- `createsNewEvidence=false`;
- `sourceMutationAllowed=false`;
- `scientificWritebackAllowed=false`.

This sibling-output rule is the intended pattern for future Free Raster / PNG / other derived-output adapters: one admitted upstream state, multiple downstream raster/output consumers, no preview-as-authority shortcut.

## Export boundary

Export must be explicit and must create a **new derived output**. Source overwrite is not an admitted default workbench operation.

An export may have its own:

- filename/location;
- resolution;
- format/quality;
- colour/output encoding;
- metadata policy;
- authority overlay;
- provenance record.

Those choices do not rewrite the source or Scientific Master.

## Flexible-inside cable relation

This non-destructive rule is an outer cable invariant. The implementation inside may evolve through new renderers, edit operators, caches, GPU/CPU implementations or output adapters, provided all of the following remain true:

1. the same immutable source binding is retained;
2. edits remain separately represented and reversible;
3. scientific authority never increases because an edit/render implementation changed;
4. source mutation and Scientific Master writeback remain false for workbench appearance/output edits;
5. export remains a new derivative;
6. the exact internal implementation can be identified for diagnostics/provenance where needed.

Therefore the combined rule is:

**Stable outside. Flexible inside. Non-destructive throughout. Evidence law unchanged.**

## Runtime contract anchor

`suite_android/app/src/main/java/com/truthraw/adaptiveui/NonDestructiveWorkbenchStateV01.kt`

The v0.1 runtime contract stores an immutable `SourceBinding` plus an ordered set of reversible downstream `EditOperation` objects. Operation and parameter IDs are open identifiers so future workbench controls do not require a closed-world architecture rewrite.

Hard runtime constants remain false:

- `SOURCE_MUTATION_ALLOWED`
- `SCIENTIFIC_MASTER_WRITEBACK_ALLOWED`
- `OVERWRITE_SOURCE_ON_EXPORT_ALLOWED`

The contract currently defines state semantics; integration into every existing Workspace/ADVANCED control is a separate implementation and validation step. Its existence must not be reported as proof that every current UI path already uses it.
