# TruthRaw Camera 5 200MP Probe v0.7

Independent Camera2 evidence-capture app source for HONOR BKQ-N49 physical Camera 5.

Target is fail-closed: `RAW_SENSOR 16320x12288` only. v0.7 explicitly queries both `getOutputSizes()` and `getHighResolutionOutputSizes()` from `SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION`; the latter is required for the 200.54016 MP route reported by the device.

The original `Image.Plane[0]` buffer is hashed and written directly through `FileChannel` without allocating a full-frame Java `ByteArray`. If the returned RAW_SENSOR plane is exactly contiguous (`pixelStride=2`, `rowStride=32640`, bytes=401080320), the file is named `.rawsensor`; otherwise it is preserved as `.rawbuffer` and normalized off-device without discarding the original buffer.

The app also attempts an Android `DngCreator` container for RAW_SENSOR while retaining the raw buffer as primary evidence.

This app does not modify canonical TruthRaw reconstruction. It is acquisition/evidence tooling only.
