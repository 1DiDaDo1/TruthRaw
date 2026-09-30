# D.RAW UX / route navigation checkpoint — 2026-09-30

## Why this exists

The scientific architecture grew beyond the older navigation. The UI is now
explicitly split into normal photography, research/JSON, and historical
diagnostics without deleting any existing scientific or export route.

## Start page

Normal use is first:

1. Start photo
   - File → existing RAW/DNG
   - Camera → Universal Physical Capture
2. The currently selected route is visible before input.
3. Route can be changed afterwards:
   - PURE = Scientific View
   - ADVANCED = Appearance / Restoration View
   - PRO = Open Scene / professional workbench
4. Research & knowledge is a separate explicit entry:
   - Research & JSON
   - What is implemented / how to use it

All three routes share the same sealed source and Scientific Master. PRO is more
tooling, not stronger evidence.

## Main workbench

The large Multi-observation / research panel is no longer forced into the
normal photo workflow.

Normal mode shows a compact Research & JSON entry card.

Explicit Research mode exposes:
- relation-based Calibration Observation Record JSON import;
- universal analysis of selected observations;
- Field Response Repeatability;
- Observation–World Field Separation;
- Free World Observation Geometry Foundation;
- source-bound research audits and holdouts in PRO.

Existing ordinary/professional exports remain directly available.

## Research & JSON hub

TruthRawResearchHubActivity is the central research entry.

It provides:
- direct Multi-observation / JSON workbench entry;
- direct RAW/DNG research entry;
- direct Calibration Observation Record import;
- Global Research Snapshot JSON export;
- practical explanation of which JSON is used for which purpose;
- links to the universal camera and implementation guide.

## Global Research Snapshot v0.1

GlobalResearchSnapshotV01 is source-independent documentation/machine-readable
project state.

It includes:
- normal workflow;
- route roles;
- research workflow;
- implemented candidate runtimes;
- promotion gate registry;
- universal identity-independence contract;
- permanent scientific laws;
- test-status UI convention;
- explicit validation state.

It contains no photo measurement and is never calibration evidence.

Session-specific capability state remains in the Free World Observation
Geometry Foundation export.

## Implementation guide

TruthRawImplementationGuideActivity explains:
- normal workflow;
- PURE / ADVANCED / PRO;
- universal camera;
- backside/frontside;
- Multi-observation / Calibration Atlas;
- current multidisciplinary candidate runtimes;
- which research route to use for one photo, repeated observations,
  stop-motion/time, colour/light/optics;
- JSON selection;
- scientific noise-reduction progression;
- permanent authority boundaries.

## Test / research run status invariant

The existing status principle remains mandatory for tests and heavy analyses:

- RUNNING = green dot + live elapsed timer;
- SUCCESS = green dot + final duration;
- ERROR = red dot + final duration;
- CANCELLED = muted status + final duration.

MainActivity keeps its existing operation status implementation.
TruthRawResearchStatusVisualV01 provides the same convention for new research
surfaces.

The dot/timer is UI state only and cannot change scientific authority.

## Camera route

Universal Physical Capture now states the practical chain:

physical capture → sealed RAW_SENSOR → derived DNG → Universal Intake →
Scientific Master → selected route.

Camera2, Camera-ID, focal length, and UI lens role remain acquisition/provenance
hints. No manual lens calibration is required for normal use.

The specialized 4K→200MP route remains separate.

## Scientific invariants

- MEASURED != RECONSTRUCTED != APPEARANCE
- BlackLevel != Zero-Line
- output raster density != optical resolution
- camera/lens/vendor/RAW identity cannot select scientific truth
- UNKNOWN residual is not automatically noise
- representation may exceed the source; knowledge claims may not
- candidate availability != promotion
- normal user calibration required = false
- Scientific Master writeback from research candidates = false

## Validation status

This UX/navigation implementation is intentionally unbuilt and untested at this
checkpoint, following the user instruction for this implementation wave.

No compile, CI, APK build, or device validation is claimed.
