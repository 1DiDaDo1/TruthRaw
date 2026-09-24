# Unified Output Preview v0.1

A display preview must be derived from the same stored/selected output primary route as the selected output.

Supported source spaces:

- camera-native Scientific Master / TruthNegative camera-native raster;
- developed extended-linear sRGB Render/Edit primary;
- XYZ-D50 projected primary;
- committed display-sRGB JPEG bytes through the Android saved-output loader.

The native preview renderer samples the declared primary directly, converts only into display-linear sRGB where required, then clips display values to [0,1] and applies the standard sRGB transfer function.

Negative and >1 source values are never modified in the primary. Their display clipping is counted separately.

## Current exact route bindings

- PURE Float32 DNG -> random-access F64 Scientific-Master primary;
- Full Colour Scientific Master Float32 DNG -> random-access F64 Scientific-Master primary;
- Advanced Render/Edit Float32 DNG -> final Render/Edit primary;
- TruthNegative 200MP Float32 DNG -> dense TruthNegative primary;
- TruthNegative TN-4 scientific negative -> TN-4 Scientific-Master primary;
- Full-res Restoration and restoration projections -> restoration derivative primary;
- compatibility Linear DNG -> exact bounded-U16 primary representation;
- saved JPEG -> decoded from the actually committed JPEG bytes.

`RandomAccessScientificMasterSource` reconstructs only the requested preview rows/tiles through the active reconstruction backend; it does not materialize a second full-frame master.

`BoundedU16PrimarySource` mirrors the exact quantization domain of the compatibility Linear DNG before display rendering.

## Orientation

UOP1 carries `displayQuarterTurns` (0..3). The output route must bind this to the orientation actually stored by that output. The UI displays each result using its own UOP1 orientation contract rather than reusing whichever orientation happens to be active in the editor.

This matters because DNG/TIFF metadata orientation and raster-oriented formats such as EXR do not necessarily encode orientation in the same way.

## Authority boundary

The preview layer:

- adds no restoration, detail, HDR, sharpening or relight beyond what is already present in the chosen primary;
- grants no authority;
- writes nothing back into Scientific Master/Open Scene;
- is presentation only.

The UOP1 sidecar is a compact ARGB8888 preview payload used by the Android UI. It is not evidence.

**Same output primary route, smaller display projection.**
