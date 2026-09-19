# TruthRaw v0.54 — Android 17 Honor Pro TELE RAW/DNG device result

Date: 2026-09-19

## Authority

`PASSIVE_EXPORTED_OEM_DNG_CONTAINER_METADATA_ONLY`

TruthRaw did not open a camera, submit a capture, decode image samples, follow strip/tile payload offsets, invoke Honor Binder, or write vendor request keys.

## Device and OEM app

- HONOR BKQ-N49
- Android 17 / SDK 37
- TruthRaw target SDK 37
- Honor Camera 171.0.10.706
- Honor Camera target SDK 37

Run profile: `PRO_TELE_RAW`

## Exported file

Honor created:

`IMG_20260919_110414.dng`

MediaStore/container facts:

- geometry: 4080x3072
- file size: 25,097,536 bytes
- path: DCIM/Camera/RAW/
- classic TIFF, little-endian
- DNGVersion 1.4.0.0
- DNGBackwardVersion 1.1.0.0
- UniqueCameraModel: BKQ-N49-HONOR-HONOR

## RAW IFD

The only CFA RAW IFD reports:

- ImageWidth = 4080
- ImageLength = 3072
- BitsPerSample = 16
- Compression = 1 (uncompressed)
- PhotometricInterpretation = 32803 (CFA)
- SamplesPerPixel = 1
- RowsPerStrip = 1
- 3072 strips
- every strip = 8160 bytes
- strip byte-count sum = 25,067,520 bytes
- CFARepeatPatternDim = 2x2
- CFAPattern = [2,1,1,0]
- BlackLevel = [64,64,64,64]
- WhiteLevel = 1023
- DefaultCropOrigin = [8,8]
- DefaultCropSize = [4064,3056]
- ActiveArea = [0,0,3072,4080]

The storage geometry is exact:

`4080 * 2 = 8160 bytes/row`

`4080 * 3072 * 2 = 25,067,520 payload bytes`

That is exactly the same U16 payload byte count already associated with physical Camera 5's standard 4080x3072 RAW route.

## Tele identity corroboration

The DNG itself reports:

- FocalLength = 22.48 mm
- FNumber = 2.6
- exposure = 0.009999993 s
- ISO = 1181

The 22.48 mm / f2.6 pair independently corroborates that this exported Pro RAW is the tele path associated with Camera 5. It remains exported-container metadata, not direct runtime physical-ID binding.

## RAW14 result

This Pro TELE DNG does **not** expose RAW14 at the DNG container level.

It is:

- 16-bit storage;
- uncompressed;
- nominal WhiteLevel 1023;
- BlackLevel 64.

Therefore this file describes a nominal 10-bit code ceiling inside 16-bit storage, not a 14-bit 16383 code ceiling and not a 14-bit BitsPerSample container.

Classification:

`ANDROID17_HONOR_PRO_TELE_DNG_EXPORT_IS_4080x3072_UNCOMPRESSED_U16_CFA_WITH_10BIT_NOMINAL_CODE_CEILING`

RAW14 conclusion:

`NO_EXPORTED_RAW14_EVIDENCE_IN_THIS_PRO_TELE_DNG_ROUTE`

## Important cross-layer consequence

This result connects three previously separate observations without authority promotion:

1. public Camera2 physical-5 standard RAW is 4080x3072;
2. the malformed 16320x12288 app-visible envelope contains exactly 25,067,520 populated bytes;
3. Honor's own Android-17 Pro TELE DNG exports a real 4080x3072 uncompressed U16 CFA payload whose strip-byte sum is exactly 25,067,520 bytes.

This is very strong structural corroboration that the ordinary tele RAW data domain exposed/exported here is the 4080x3072 domain.

It still does not explain how the OEM 200MP JPEG path creates its 16320x12288 output and does not prove untouched ADC provenance.

## Next controlled experiment

Use the same v0.54 APK with `PRO_MAIN_RAW`.

If MAIN also exports 16-bit storage + WhiteLevel 1023, then the advertised Android-17 RAW14 feature is not visible in either ordinary Honor Pro main or tele DNG export and research should move to identifying the exact UI/mode/feature gate that enables it rather than assuming all Pro RAW captures use RAW14.
