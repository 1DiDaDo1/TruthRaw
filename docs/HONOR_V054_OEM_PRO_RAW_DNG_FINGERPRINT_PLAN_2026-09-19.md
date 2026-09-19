# TruthRaw v0.54 — Android 17 Honor Pro RAW/DNG exported-container fingerprint

Date: 2026-09-19

## Trigger

v0.53 reproduced the Android-16 v0.14 Camera-5 acquisition route on Android 17 and reproduced the same app-visible payload topology:

- 16320x12288 U16 envelope;
- 401,080,320 bytes;
- only rows 0..767 populated;
- 25,067,520-byte contiguous populated prefix;
- one unique advertised standard RAW candidate: 4080x3072.

The ordinary third-party Camera2 16320x12288 route is therefore no longer a useful place to search for a new Android-17 RAW14 path.

The user also reports that the live preview looks clearer on Android 17.

## Preview observation boundary

A source comparison of the historical Android-16 v0.14 staged activity and current v0.53 shows that the Camera2 preview path itself is semantically unchanged:

- same logical camera 0 preview;
- same selection rule: largest SurfaceTexture output <= 1920x1440;
- same SurfaceTexture default-buffer sizing;
- same TEMPLATE_PREVIEW request;
- same CONTROL_MODE/AUTO, AE ON and continuous-picture AF handling;
- same 3.7x CONTROL_ZOOM_RATIO request;
- same active-physical-ID observation.

The TextureView still uses MATCH_PARENT width and weight=1 height.

However, surrounding title/explanatory UI text changed between versions. That can change the measured TextureView viewport height and display scaling, so the user's clearer-preview observation is retained as **appearance observation only**. It is not promoted to a sensor/HAL sharpness improvement without a controlled matched-frame display test.

This preview observation does not change the v0.53 RAW-payload result.

## v0.54 goal

Test where Honor's Android-17 “RAW14 support” becomes visible in an actual **OEM-produced Pro RAW/DNG exported file**, without entering Honor's privileged capture pipeline.

TruthRaw remains passive.

The user chooses one run profile:

- `PRO_MAIN_RAW` — one Honor Pro RAW/DNG at 1x;
- `PRO_TELE_RAW` — one Honor Pro RAW/DNG at 3.7x if the Honor Pro UI permits it.

TruthRaw watches MediaStore. When a new stable DNG appears after observer start, it opens that exported file read-only and parses only TIFF/DNG container metadata.

## Metadata parsed

The classic-TIFF parser records IFD structure and selected metadata including:

- ImageWidth / ImageLength
- BitsPerSample
- Compression
- PhotometricInterpretation
- SamplesPerPixel
- CFARepeatPatternDim / CFAPattern
- DNGVersion / DNGBackwardVersion
- UniqueCameraModel
- BlackLevelRepeatDim / BlackLevel
- WhiteLevel
- DefaultCropOrigin / DefaultCropSize
- ActiveArea
- ColorMatrix1 / ColorMatrix2
- CalibrationIlluminant1 / CalibrationIlluminant2
- strip/tile offset and byte-count arrays as metadata summaries
- Exif IFD metadata when pointed to by the TIFF container

GPS IFDs are deliberately not followed.

Strip/tile **payload offsets are never followed into pixel data**.

## RAW14 interpretation gates

### Exported 14-bit container

If a CFA RAW IFD reports:

`BitsPerSample = 14`

that is evidence that the exported OEM DNG claims 14-bit sample storage at the container level.

It is not, by itself, untouched ADC evidence.

### 14 effective code bits in a 16-bit container

If:

- `BitsPerSample = 16`
- `WhiteLevel = 16383`

then the exported DNG may represent a 14-bit code domain inside 16-bit storage.

That remains a DNG metadata/code-domain claim, not a proof of native sensor ADC transport.

### Old 10-bit-like path

If:

- `BitsPerSample = 16`
- `WhiteLevel = 1023`

then the OEM exported DNG is still describing the same nominal 10-bit code ceiling seen in earlier Camera2/DngCreator evidence.

## Authority

`PASSIVE_EXPORTED_OEM_DNG_CONTAINER_METADATA_ONLY`

v0.54 does not:

- open a CameraDevice;
- submit a CaptureRequest;
- invoke Honor Binder;
- write vendor keys;
- decode a bitmap;
- decode RAW image samples;
- read strip/tile image payload bytes;
- follow GPS IFDs;
- grant capture/calibration authority.

The exported file is OEM-authored output. Its metadata can characterize the exported container, not the hidden upstream sensor pipeline.
