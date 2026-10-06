# D.RAW Free Raster v0.2 — runtime test checkpoint — 2026-10-06

This checkpoint exists to bind the Android test build to the completed downstream Unified Output presentation cable.

Runtime parent before this documentation-only commit:

`85112272a33b6b66e6c48b7453ac66234d306e1d`

Completed candidate cable:

`UnifiedOutputPreviewResult.Ready -> UnifiedOutputPresentationBridge -> D.RAW Workspace -> Free Raster`

The bridge transports an independent process-local presentation copy and descriptive source/route binding. It does not render, reconstruct, infer scientific authority, create evidence, or write back to Scientific Master.

Lifecycle behavior is fail-closed: the bridge-owned copy may survive a normal finish/back transition or configuration change, while unexpected MainActivity teardown clears stale presentation state.

External JPG/PNG/WebP remains `EXTERNAL_PRESENTATION_RASTER / PRESENTATION_ONLY`. RAW/DNG remains on Universal Intake.

Permanent boundaries remain unchanged:

- sealed source evidence is immutable;
- `MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`;
- `SOURCE_MUTATION_ALLOWED=false`;
- `SCIENTIFIC_MASTER_WRITEBACK_ALLOWED=false`;
- `OVERWRITE_SOURCE_ON_EXPORT_ALLOWED=false`;
- Free Raster creates no new evidence or authority;
- no second RAW decoder, Scientific Master, reconstruction route or T5 evaluation is introduced.

This is a runtime/build candidate checkpoint only. Physical device acceptance remains pending until the generated APK is tested on the real device. PR #131 remains draft until that acceptance is complete.
