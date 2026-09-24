# Unified Output Preview v0.1

A display preview must be derived from the same primary tile source as the selected output.

Supported source spaces:

- camera-native Scientific Master / TruthNegative dense camera-native raster;
- developed extended-linear sRGB Render/Edit primary;
- XYZ-D50 projected primary.

The preview renderer samples the declared primary directly, converts only into display-linear sRGB where required, then clips display values to [0,1] and applies the standard sRGB transfer function.

Negative and >1 source values are never modified in the primary. Their display clipping is counted separately.

The preview layer:

- adds no restoration, detail, HDR, sharpening or relight;
- grants no authority;
- writes nothing back into Scientific Master/Open Scene;
- is presentation only.

The UOP1 sidecar is a compact ARGB8888 preview payload used by the Android UI. It is not evidence.

**Same primary route, smaller display projection.**
