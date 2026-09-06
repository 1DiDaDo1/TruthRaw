# TruthRaw Camera 5 — 200MP RAW capture v0.7

## Goal
Capture one real HONOR BKQ-N49 Camera2 `RAW_SENSOR` frame at **16320x12288** from public Camera ID **5** and bind it to the applied `TotalCaptureResult`.

## On the phone
1. Open the `camera5-200mp-probe-v07` project in Android Studio and install/run it on the Magic8 Pro.
2. Grant Camera permission.
3. Tap **Run Camera 5 200MP RAW proof**.
4. Leave the app open until it reports the RAW SHA-256 and manifest path. A 200MP RAW_SENSOR buffer is about 401 MB before possible row padding; DNG creation may take additional time/storage.
5. The evidence directory is the app external-files folder under `truthraw_camera5_200mp_v07`.
6. Return at minimum:
   - `truthraw_camera2_inventory_v07.json`
   - `truthraw_camera5_200mp_capture_manifest_v07.json`
   - the `.rawsensor` or `.rawbuffer` file
   - the `.dng` too if DngCreator succeeded.

## Fail-closed target
The app refuses 50MP as the v0.7 target. It explicitly requires Camera ID 5, focal length near 22.48 mm and `RAW_SENSOR 16320x12288` from the maximum-resolution **high-resolution output list**.

## Expected proof fields
- requested + applied `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION`
- Camera ID 5
- 16320x12288 RAW_SENSOR
- `Image.timestamp == SENSOR_TIMESTAMP`
- payload SHA-256
- row stride / pixel stride
- `SENSOR_RAW_BINNING_FACTOR_USED`
- dynamic black/white when reported
- NoiseProfile when reported
- `lensShadingApplied=true` remains an upstream-processing boundary

The capture can prove a full 200.54016 MP **app-visible** RAW lattice. It cannot by itself prove untouched photodiode/ADC output.
