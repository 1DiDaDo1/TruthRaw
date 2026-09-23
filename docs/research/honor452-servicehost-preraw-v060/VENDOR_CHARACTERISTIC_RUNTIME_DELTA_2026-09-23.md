# TruthRaw v0.60 — corrected runtime vendor-characteristic delta

Date: 2026-09-23

Authority: `CAMERA2_CHARACTERISTICS_DIRECT_TYPED_VENDOR_KEY_READ_ONLY`

This note records the newly returned v0.50 device report supplied on 2026-09-23. It supersedes the older **result interpretation only** that stated every direct typed lookup was null. Historical files are not rewritten.

## Important correction

The newly supplied v0.50 report contains non-null values for several APK-derived vendor characteristics.

For physical camera 5:

- `physicalCameraScene = [66]`
- `sceneCameraIdCapability = [1]`
- `teleSupport = 1`
- `rawZoomSupported = 1`
- `rawSensorResolution = null`
- `needOpenPhysicalCamera = null`
- `ultraResolutionSwitchSupportedSize = null`
- `ultraHighPixelMonoSupported` still throws tag-not-found.

## Cross-camera comparison

The decisive correction is that `physicalCameraScene=[66]` is **not Camera-5 specific**.

The same value is returned for rear logical/physical IDs:

- camera 0: `[66]`
- camera 2: `[66]`
- camera 4: `[66]`
- camera 5: `[66]`

The front camera 1 does not return this value.

Therefore scene 66 must not be promoted as a tele-only or 200MP-only scene based on this report.

Likewise, `sceneCameraIdCapability=[1]` is shared by the rear camera family.

## cameraIdCustomInfo structure observation

`cameraIdCustomInfo` has length 310.

For cameras 0, 2, 4 and 5 the entire 310-element array is byte/value-identical in the supplied report.

The first 90 entries form nine conspicuous 10-integer records:

```
[1,-1,-1,-1,-1,-1,10,-1,-1,8]
[1, 0,-1,-1,-1,-1,10,-1,-1,8]
[1,47,-1,-1,-1,-1,10,-1,-1,4]
[1,33,-1,-1,-1,-1, 9,-1,-1,2]
[1,59,-1,-1,-1,-1,10,-1,-1,8]
[1,71,-1,-1,-1,-1,10,-1,-1,2]
[1,53,-1,-1,-1,-1,10,-1,-1,0]
[1,65,-1,-1,-1,-1,10,-1,-1,6]
[1,67,-1,-1,-1,-1,10,-1,-1,2]
```

Entries 90..309 are zero in the rear-camera copy.

Camera 1 has the same total array length but only its first 10-entry record materially populated.

This strongly suggests `cameraIdCustomInfo` is a fixed-width OEM routing/configuration table rather than a per-sensor scalar property. Exact field semantics are **not yet proven**.

## Consequence

Do not interpret:

- `physicalCameraScene=66` as proof of UltraHighPixel;
- any value in `cameraIdCustomInfo` as a camera ID, scene ID, focal class or sensor role until the corresponding HONOR parser/consumer is traced in bytecode.

The next static target is therefore not the already-known ServiceHost RAW surface. It is the code that **parses/consumes `cameraIdCustomInfo` and `physicalCameraScene`**, because that can reveal the field schema and meaning of 66 without guessing.

## Separate v0.59 replication

Four independent v0.59 RAW10 captures have now been byte-audited. All four:

- allocate 250,675,200 bytes for the declared 16320x12288 RAW10 plane;
- have their last non-zero byte at offset 15,728,619;
- have a 15,728,640-byte effective prefix that reshapes exactly to 3072 rows x 5120 bytes;
- have 5100 data bytes + 20 zero padding bytes in every effective row;
- have all 3072 effective rows populated;
- are all-zero after byte 15,728,640;
- have different effective-prefix SHA-256 values, proving distinct captures rather than one repeated cached payload.

This materially strengthens the v0.59 topology classification while preserving the permanent boundary that app-visible Camera2 RAW is not untouched photodiode ADC proof.
