# TruthRaw APV Professional Video Route

Status: RESEARCH / NOT SELECTED FOR SCIENTIFIC MASTER

## Why APV belongs in TruthRaw research

Advanced Professional Video (APV) is a high-bitrate, intra-frame-only codec
designed for professional capture and post-production. It is deliberately
different from delivery-oriented inter-frame codecs.

Android 16 platform support describes APV as:
- perceptually lossless, close to raw video quality;
- low-complexity, high-throughput intra-frame-only coding;
- tile based, allowing parallel encode/decode;
- suitable for repeated decode/re-encode;
- able to carry HDR10/10+ and user-defined metadata;
- able to represent auxiliary streams such as depth, alpha and preview.

Android 16 guarantees the APV 422-10 profile: YUV 4:2:2, 10-bit, target
bitrates up to 2 Gbps. APV encode and decode are mandatory Android platform
formats beginning with Android 16, but hardware acceleration is not inferred
from format availability. TruthRaw must inspect MediaCodecInfo at runtime.

Sources:
- https://developer.android.com/about/versions/16/features#apv
- https://developer.android.com/media/platform/supported-formats
- https://developer.android.com/reference/android/media/MediaFormat
- https://developer.android.com/media/optimize/performance/codec

## Broader OpenAPV capability

The Academy Software Foundation OpenAPV reference implementation currently
documents support for:
- 422-10 / 422-12
- 444-10 / 444-12
- 4444-10 / 4444-12
- monochrome 400-10
- RGB content coding with 444 profiles via color-description signalling
- tile-based multi-threading and partial tile decoding
- ARM NEON optimization
- HDR/user metadata
- OpenAPV extensions including 16-bit source companded to 12-bit profiles.

This is materially richer than Android's mandatory 422-10 platform profile.
It is therefore a separate research backend, not evidence that a phone's
hardware APV block supports those richer profiles.

Sources:
- https://github.com/AcademySoftwareFoundation/openapv
- https://www.rfc-editor.org/info/rfc9924/

## Qualcomm relevance

Qualcomm explicitly lists Advanced Professional Video Codec in recent Snapdragon
camera/video feature briefs. This makes runtime hardware APV probing worthwhile
on Snapdragon devices.

However, a Qualcomm SoC name alone does not authorize a backend. TruthRaw must
query the Android codec list and record:
- encoder/decoder codec names;
- hardware-accelerated flag;
- vendor flag;
- aliases removed;
- advertised color formats;
- profile/level pairs;
- max width/height/bitrate;
- performance points when available.

Source:
- Qualcomm Snapdragon 8 Elite Gen 5 product brief:
  https://www.qualcomm.com/content/dam/qcomm-martech/dm-assets/images/company/news-media/media-center/press-kits/snapdragon-summit-2025-press-kit/day-2-/documents/Snapdragon8EliteGen5_ProductBrief.pdf

## TruthRaw role

### Allowed candidate roles

1. PROFESSIONAL_VIDEO_INTERMEDIATE
   - developed scene-linear/HDR material transformed to a declared APV working
     representation;
   - final encode offloaded to a validated hardware APV encoder where present.

2. VIDEO_EDIT_PROXY
   - high-quality all-intra proxy for timeline scrubbing/editing.

3. AUXILIARY_PREVIEW_STREAM
   - APV's auxiliary/preview support can be investigated for a future
     TruthRaw video container.

4. CAMERA_CAPTURE_VIDEO_TARGET
   - only if Camera2/MediaCodec routing on a specific device can deliver the
     required profile without silently applying vendor image processing that
     violates the declared route.

### Forbidden roles

APV must not be:
- the Scientific Master;
- Source Evidence;
- called lossless RAW;
- used to replace Float32 DNG/TIFF for still-photo Lightroom editing;
- allowed to upgrade Dynamic Authority;
- treated as bit-exact merely because its quality is perceptually lossless.

Android's guaranteed 422-10 profile chroma-subsamples color and quantizes to
10-bit, so it is fundamentally not equivalent to the current Float32 linear
TruthRaw domains.

## Runtime selection contract

A hardware APV route is selectable only when all are true:

1. MIME "video/apv" encoder is present.
2. MediaCodecInfo.isHardwareAccelerated == true.
3. Encoder is not an alias-only duplicate.
4. Required resolution and bitrate are supported.
5. Required profile/color format is advertised.
6. A known deterministic test frame can encode and decode successfully.
7. Decoded output error stays within the declared APV presentation/video
   tolerance.
8. Thermal and sustained-throughput test passes.
9. Failure falls back without changing scientific authority.

The decoder is probed separately from the encoder.

## OpenAPV software route

OpenAPV is not automatically preferable to Android MediaCodec. It is useful for
research because:
- source is inspectable;
- ARM NEON and tile threading are documented;
- richer 4:4:4/RGB profiles are available;
- test vectors exist.

Potential use:
- compare Android hardware APV 422-10 against OpenAPV 444-12;
- measure throughput, quality, file size and thermal load;
- establish whether a richer software APV intermediate offers value on devices
  whose hardware codec exposes only 422-10.

It must not be bundled into the production APK until license, binary size,
maintenance and conformance implications have been reviewed.

## Relationship to the adaptive compute router

APV is not a compute backend like Vulkan or NEON.

Correct hierarchy:

TruthRaw image/video processing
  -> CPU / multicore / Vulkan / vendor compute
  -> declared developed video frame
  -> APV hardware/software encoder
  -> professional intermediate stream

Therefore APV can offload the encoding stage and reduce CPU load, but it cannot
accelerate reconstruction, Scientific Master digest, color characterization,
HDR authority or Restoration by itself.

## Initial implementation

v0.1 adds a read-only Android MediaCodec probe. It selects nothing and changes
no pixels. The probe is allowed to run on Android versions where APV is absent;
absence simply produces an unavailable capability record.
