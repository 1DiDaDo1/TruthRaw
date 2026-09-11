# TruthRaw Adaptive UI + Ingress v0.1 — First Interface Concept

## Design intent

TruthRaw should feel like a fast photography application even though its scientific engine is room-based, evidence-bound and tile-native. The interface therefore exposes human tasks, not the internal scientific implementation graph.

The first prototype has three presentations of the same state.

## Compact

Primary target: ordinary portrait phone or narrow multi-window.

```text
+--------------------------------+
| TruthRaw              RAW kiezen|
| Compact · 4 RAW geselecteerd   |
+--------------------------------+
|                                |
|       TruthRaw Scene Preview   |
|                                |
| proxy -> tiled -> final        |
|                                |
+--------------------------------+
| Uitkomst                       |
| [Afzonderlijk verwerken]       |
| [Verbeterde foto] [HDR]        |
+--------------------------------+
| 1 IMG_001.DNG  ready           |
| 2 IMG_002.DNG  ready           |
+--------------------------------+
```

The scene gets most of the vertical space. Room/tool choices remain below the image or behind a later bottom-sheet/navigation layer.

## Medium

Primary target: landscape phone, foldable half/full state, small tablet or larger split-screen window.

```text
+----------------+---------------------------+
| Ingang         |                           |
| IMG_001        |                           |
| IMG_002        |       Scene Preview       |
| IMG_003        |                           |
|                |                           |
| Uitkomst       |                           |
| Afzonderlijk   |                           |
| Verbeterd/HDR  |                           |
+----------------+---------------------------+
```

Input/batch state stays visible while the scene remains the dominant pane.

## Expanded

Primary target: tablet, unfolded large foldable, desktop-like Android window.

```text
+---------------+----------------------+----------------+
| Ingang        |                      | Kamers         |
| IMG_001       |                      | Natural        |
| IMG_002       |      Scene Preview   | Detail         |
| IMG_003       |                      | Illumination   |
| Queue         |                      | Export         |
+---------------+----------------------+----------------+
```

The third pane is presentation/tool state only. It does not own another full-resolution source or Scientific Master.

## User input model

### One RAW

Offer:

- `1 upload -> 1 output`
- `1 upload -> multiple outputs`

The second choice is expected to share one scientific reconstruction/master and fan out only where the requested outputs differ in appearance/projection.

### Multiple RAWs

Default:

- `Afzonderlijk verwerken`

Each selected source remains an independent lineage.

Optional explicit intent:

- `Verbeterde foto`
- `HDR`

These two choices are not evidence fusion in v0.1. They record user intent for a future Multi-Capture Fusion contract.

## Performance model

The UI never needs the complete source RAW payload merely to list, select or queue an image. Ingress stores document handles and tiny metadata only. Preview and final processing should later consume bounded proxies/tiles.

The UI event loop must remain independent of native TruthRaw work. Processing progress should be represented by small immutable state updates such as:

```text
jobId
phase
progress
previewVersion
runtimeThermalState
```

No CFA tile payload should cross into UI state merely to report progress.

## Adaptive axes

UI adaptation:

`current app window -> Compact / Medium / Expanded composition`

Execution adaptation:

`device memory + CPU + thermal + current Runtime policy -> tile/concurrency/workspace`

The axes are intentionally independent.

## Follow-up presentation layer

After this dependency-light proof compiles, presentation can migrate to current Material 3 Adaptive APIs, including posture/fold-aware panes, while preserving the same `BatchSession`, `RawJob`, route and no-full-RAW-ingress rules.
