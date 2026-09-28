# D.RAW Universal Camera Input v0.4 — orientation + physical preview proof

Date: 2026-09-28

Status: TEST BRANCH — NOT MERGED TO MAIN

## Problem observed on device

1. Ultra-wide and Wide/Main could appear identical in the live viewer.
2. Captures from ultra-wide, main and tele reached Universal Intake, but the derived Android DNG failed closed with:
   `code=7; message=unsupported Orientation`.

The sealed RAW_SENSOR plane itself was already written and hashed before the DNG was created.

## Root causes addressed

### Derived DNG Orientation

Historical device evidence already proved that Android DngCreator on this device can emit TIFF Orientation=9. TIFF/Exif Orientation uses the standard range 1..8 and the native TileNativeDngSource correctly rejects unsupported topology.

For the camera-created compatibility DNG only, v0.4 now explicitly calls:

`DngCreator.setOrientation(ExifInterface.ORIENTATION_NORMAL)`

This sets storage-coordinate Orientation=1 before DNG serialization.

Authority boundary:

- sealed RAW_SENSOR bytes are unchanged;
- DNG remains a derived compatibility container;
- storage orientation 1 does not claim world/presentation orientation;
- presentation orientation authority remains UNKNOWN;
- no new evidence is created.

### Lens role / live preview

The previous UI role assignment ordered lenses only by reported focal length. That is not sufficient across different sensor sizes.

v0.4 uses reported sensor physical size + focal length to build an acquisition-only diagonal field-of-view score when available. Focal length remains the fallback hint only.

For a physical-camera-bound live route, preview telemetry now requires the selected physical camera to appear in `physicalCameraResults`. If it does not, the live route is shown as unproven and capture stays blocked.

Telemetry shows:

- effective camera id;
- requested physical camera id when applicable;
- result focal length;
- AF/focus/ISO/exposure.

These values are acquisition diagnostics only and never upgrade scientific authority.

## Unchanged laws

- Camera2 remains transport/plumbing, not scientific truth.
- The exact app-visible RAW_SENSOR plane is sealed before DNG creation.
- No AI/ML/learned model is used.
- Standard RAW_SENSOR capture remains separate from the special 4K → 200MP route.
- Lens role remains a UI/acquisition hint only.
- DNG never replaces the primary RAW_SENSOR evidence.
