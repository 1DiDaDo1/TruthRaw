# Reconstructed Color Preview v0.1

Status: RESEARCH IMPLEMENTATION CANDIDATE — CI PENDING

Base: Preview Representation v0.1.

## Purpose

Provide the first bounded color preview surface that consumes the **existing** TruthRaw `StreamingTruthRawProcessor` output after reconstruction, camera-to-XYZ, XYZ-to-linear-sRGB, appearance processing and the global SDR exposure/tone plan.

This module does not parse RAW, does not own a decoder and does not create a Scientific Master. It is an `IStreamingSink` only.

## Contract

`BoundedSrgbPreviewSink`:

- accepts the existing linear-sRGB SDR tile stream;
- applies the standard sRGB OETF only at the presentation boundary;
- normalizes `Normal`, `Rotate180`, `Rotate90CW` and `Rotate90CCW` orientation in the preview pixel surface;
- center-samples source/display locations into a bounded preview surface;
- stores only one 32-bit opaque ARGB pixel plus one ownership byte per preview pixel;
- does not store a source-sized or output-sized full frame;
- observes but does not retain the half-log-gain stream in SDR v0.1;
- accepts diagnostics only when the processor says diagnostics are enabled;
- fails if any preview pixel is unwritten or written by overlapping source tiles.

The preview surface scales with requested preview dimensions, not with source megapixel count.

## Scientific boundary

The incoming stream has already been produced by the existing v4.7i/full-frame-streaming pipeline. The sink may not:

- alter reconstruction;
- alter exposure planning;
- alter appearance parameters;
- create measurement evidence;
- modify Scientific Master state;
- change `physicalFrameCount` or `independentEvidenceCount`;
- reinterpret half-log gain as source evidence;
- use preview pixels for calibration or numeric promotion proof.

Changing `maxEdge` must change only the presentation surface. The test runs the same source at two preview sizes and requires identical processor exposure/provenance outputs.

## Color semantics

`full_frame_streaming_v0_1_pass2.cpp` converts camera RGB through XYZ D50 to **linear sRGB**, applies appearance and the global SDR tone plan, then streams bounded RGB values through `writeSdrTile`. No sRGB display OETF is applied there.

This sink therefore applies the standard sRGB OETF before quantizing to 8-bit ARGB. That quantization is explicitly presentation-only.

## Low-memory behavior

For a preview of `W x H`, native surface ownership is approximately:

- `4 * W * H` bytes ARGB;
- `1 * W * H` bytes tile-ownership validation.

A 384 x 288 surface therefore needs about 540 KiB plus normal vector/object overhead, independently of whether the source is 4000x3000 or 16000x12000.

The runtime may choose a smaller preview edge on low-memory or thermally constrained devices without changing scientific output.

## Android/JPEG next binding

The next Android step is:

```text
BoundedSrgbPreviewSink ARGB/sRGB
        -> Android Bitmap.Config.ARGB_8888 (sRGB)
        -> live UI directly
        -> optional Bitmap.compress(JPEG, quality, caller-owned OutputStream)
```

No JPEG encode/decode round trip is required for the live UI.

## Critical real-file limitation

This host implementation proves the presentation sink using a valid synthetic TruthRaw streaming fixture. It does **not** authorize the existing Android preview-only parser sentinel for scientific color.

A real selected RAW must first have:

- true source-evidence identity;
- accepted camera/lens color binding;
- normal native-direct or Gatehouse/Main-House admission.

Only then may its reconstructed stream feed this color-preview sink.

## Validation targets

The CI candidate must prove:

- GCC Release;
- Clang Release;
- Clang ASan+UBSan;
- orientation-normalized preview completion;
- non-gray reconstructed color output from the synthetic fixture;
- sRGB OETF reference points;
- no adapter full-frame ownership;
- frame/evidence count remains 1/1;
- scientific master not modified by appearance;
- changing preview edge does not change exposure/backend provenance;
- same-aspect 12MP-ish and 192MP-ish dimensions produce the same preview surface resident bound.

## Non-claims

No claim yet of:

- physical Android rendering;
- real RAW color preview;
- JPEG encoder quality/performance;
- physical-device RSS/thermal/frame-time;
- Ultra HDR output;
- wide-gamut preview;
- production interpolation/downsampling quality.

The initial center-sampling surface is a bounded correctness/runtime proof, not the final high-quality preview resampler.