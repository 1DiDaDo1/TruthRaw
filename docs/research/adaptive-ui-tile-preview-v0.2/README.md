# TruthRaw Adaptive UI Tile Preview v0.2

Status: **RESEARCH PROTOTYPE — ANDROID ARM64 APK CI PASS**

Validated implementation SHA: `1aa38c25418ba5ed06b219e82ba48fa06323f90f`

Validation run: `34589023345`

## Historical supersession note — 2026-09-11

The v0.2 proof below remains valid as the historical **grayscale CFA diagnostic** route. On the stacked `android-source-bound-color-preview-v0.1` research candidate, the active Android UI no longer uses this sentinel route for its visible image. The active route is explicitly superseded by the source-sealed, `SOURCE_METADATA_BOUND` color pipeline documented in `docs/research/android-source-bound-color-preview-v0.1/`.

The old JNI gray proxy and its parser sentinel remain present as diagnostic code and retain **no scientific color authority**. The v0.2 verifier therefore still requires the historical native sentinel/bounded-read implementation, while allowing the active UI to be owned by the explicit source-bound successor. There is no silent fallback from the successor to this gray sentinel path.

Nothing in this supersession changes the original v0.2 validation metrics, proof boundary, or failure semantics recorded below.

## Purpose

v0.2 connects the already validated Android document-handle ingress to the existing Tile-Native DNG Source without creating a UI-owned full RAW frame.

The first visible result is deliberately a **grayscale CFA source proxy**. It is a presentation diagnostic, not demosaiced color, not a Scientific Scene Master and not a new evidence source.

## Android URI -> borrowed fd -> TileNativeDngSource

The path is:

`ACTION_OPEN_DOCUMENT Uri -> ContentResolver.openFileDescriptor("r") -> borrowed pfd.fd -> JNI -> PosixFdByteSource -> TileNativeDngSource -> bounded readRawTile calls`

The `ParcelFileDescriptor` stays owned by Kotlin/Android and remains alive for the synchronous JNI call. JNI does not detach or close it. `PosixFdByteSource` borrows the fd exactly as its upstream contract requires.

No `openInputStream`, `readBytes()` or UI-owned full RAW payload is introduced.

## Bounded preview workspace

The preview edge is capped at 384 px in Kotlin and 512 px absolutely in native code.

Native caller workspace is independent of source megapixel count:

- RAW chunk: 1024 uint16 samples;
- GainMap scratch when needed: 1024 float samples;
- preview payload: at most 512 x 512 ARGB pixels;
- TileNativeDngSource resident cap: 8 MiB.

The bridge scans only source rows needed by the preview and reads them in 1024-sample horizontal chunks. It never allocates a source-width row, much less a full source frame.

On the Kotlin side, the historical v0.2 JNI packet is copied directly into the Bitmap via `Bitmap.setPixels`. A second preview-sized `IntArray` is explicitly forbidden by the original v0.2 verifier. At the 384 px historical UI cap this avoids an otherwise redundant 589,824-byte pixel array, excluding normal object overhead.

Total bytes read may grow with source width and preview height; resident caller workspace does not scale with megapixel area.

## Scientific boundary

TileNativeDngSource v0.1 requires both `sourceEvidenceId` and an explicit color binding before it opens. Historical v0.2 did **not** yet have the final source-evidence hash/backplane identity or a calibrated arbitrary-camera color binding at UI selection time.

Therefore the historical JNI diagnostic call uses two local sentinel strings:

- `ui-preview-ephemeral-not-evidence-v0.2`
- `ui-preview-parser-sentinel-not-scientific-v0.2`

The color matrix attached to that sentinel is identity solely to satisfy the upstream parser/open contract. It is never used for the grayscale CFA preview, never exposed as calibrated color, never exported from JNI, and the `TileNativeDngSource` object is destroyed before the fd returns to Kotlin.

This sentinel **must never be reused for scientific reconstruction**. The newer source-bound successor does not promote or reuse this sentinel; it resolves a true SHA-256 source identity and accepted DNG metadata color binding independently.

## Preview normalization

Each sampled CFA value is normalized using the matching 2x2 BlackLevel phase and WhiteLevel:

`linear = clamp((sample - BlackLevel_phase) / (WhiteLevel - BlackLevel_phase), 0, 1)`

A square-root visibility curve is then used only for display. No channel interpolation, demosaic, white balance or color matrix is applied.

GainMap values are requested when the upstream source contract requires the destination buffer, but v0.2 does not multiply them into this diagnostic proxy. This avoids confusing a quick source proxy with the canonical scientific GainMap-once processing path.

## Audit transport

The historical JNI packet returns lightweight audit fields to the UI:

- source dimensions;
- TileNativeDngSource resident upper bound;
- RAW payload bytes read;
- metadata bytes read;
- tile read call count;
- `fullRawMaterialized`;
- GainMap presence;
- DNG orientation.

The historical Kotlin layer fails closed if `fullRawMaterialized=true` is ever reported.

## Validated Android build

The exact historical implementation at `1aa38c25418ba5ed06b219e82ba48fa06323f90f` passed the arm64 Android CI workflow on run `34589023345`.

Observed proof:

- Building Runtime v0.1 integrity: PASS;
- Technical Backplane v0.1 integrity: PASS;
- Tile-Native DNG Source v0.1 integrity: PASS;
- Adaptive UI Ingress v0.1 contract: PASS;
- Adaptive UI Tile Preview v0.2 contract: PASS;
- no camera permission: PASS;
- Kotlin/Java compilation: PASS;
- CMake/NDK arm64 compilation and JNI link: PASS;
- APK assembly: PASS;
- packaged native bridge: `lib/arm64-v8a/libtruthraw_ui_preview_bridge.so`, 605,112 bytes;
- APK size: 3,014,221 bytes;
- APK SHA-256: `0fb5c06f8e09d0345a34ed00236c45ba537e008e224facad2884d1f338d1f5d8`;
- artifact ID: `10194923633`;
- uploaded artifact ZIP SHA-256: `89ee82318afed183218b9878cfb5e65cf05340de02846020d37ce490b95765a0`;
- CI reports `source_raw_full_materialization=0_by_contract`;
- CI reports `preview_source_workspace=BOUNDED_CHUNKS`;
- CI reports `scientific_color_authority=0`;
- historical v0.2 verifier reports `java_second_pixel_array=FORBIDDEN`.

These results prove build/package/contract behavior for that historical implementation in CI. They do **not** prove successful physical-device execution or successful parsing/preview of an arbitrary real DNG on the target phone.

## Provider constraint

`PosixFdByteSource` uses `pread` and `fstat`; therefore v0.2 requires a seekable document-provider file descriptor with a usable size. A provider backed only by a stream/pipe is rejected rather than copied into a temporary full RAW file.

## Historical open items at v0.2

At the time of the v0.2 proof these remained open:

- real Android-device execution and frame-time/RSS profiling;
- successful real-DNG preview on the physical target device;
- true sourceEvidenceId/backplane binding from selected document bytes;
- accepted camera/lens color binding before scientific processing;
- Room ABI v0.2 source-handle admission for the selected descriptor;
- canonical reconstruction/streaming sink hookup;
- progressive reconstructed color preview;
- orientation-aware rendering;
- provider coverage beyond seekable descriptors;
- multi-capture fusion/HDR science.

Some source/color/reconstruction integration items are addressed by later stacked research candidates, but their own validation boundaries supersede rather than retroactively alter this v0.2 proof.
