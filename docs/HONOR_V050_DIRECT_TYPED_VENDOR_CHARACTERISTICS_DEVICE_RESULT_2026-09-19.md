# TruthRaw v0.50 — direct typed Honor vendor-key device result

Date: 2026-09-19

## Source

- `TRUTHRAW_DIRECT_TYPED_VENDOR_CHARACTERISTICS_ORACLE_v050.json`
- bytes: `21,421`
- SHA-256: `735e4280909867b3485f72db11cb58ee8256f2c1b38e689d07e2fdae2a248fea`

Authority remained `CAMERA2_CHARACTERISTICS_DIRECT_TYPED_VENDOR_KEY_READ_ONLY`. No CameraDevice was opened and no capture, image-buffer access, vendor write, Binder call, extension session or identity spoof occurred.

## Exact result

Five IDs were checked:

- logical 0
- front 1
- physical 2
- physical 4
- physical 5

Nine exact Honor APK-derived key name/type pairs were tested per camera: **45 direct typed lookups total**.

No key was present in the ordinary `CameraCharacteristics.keys` enumeration.

### Eight keys: valid direct lookup path, null value

For all five camera IDs, these exact typed calls completed without exception but returned `null`:

- `physicalCameraScene : int[]`
- `rawSensorResolution : int[]`
- `sceneCameraIdCapability : int[]`
- `cameraIdCustomInfo : int[]`
- `needOpenPhysicalCamera : int[]`
- `ultraResolutionSwitchSupportedSize : int[]`
- `teleSupport : byte`
- `rawZoomSupported : byte`

Total: **40 completed null reads**.

### One key: tag not found

For all five camera IDs:

- `ultraHighPixelMonoSupported : byte`

throws:

`java.lang.IllegalArgumentException: Could not find tag for key 'com.hihonor.device.capabilities.ultraHighPixelMonoSupported'`

Total: **5 tag-not-found errors**.

## Meaning

The exact-key/type experiment closes the hypothesis that the values can be recovered simply by constructing the same typed `CameraCharacteristics.Key` objects used by Honor and bypassing normal enumeration.

There were **zero non-null values**.

The distinction between eight clean `null` results and one explicit "Could not find tag" result is technically meaningful. Android documents `CameraCharacteristics.get(Key)` as returning null when a valid field is not set, while an invalid key can throw `IllegalArgumentException`. The observed pattern is therefore consistent with eight names being resolvable by the metadata machinery but absent from the characteristics blob exposed to TruthRaw, while `ultraHighPixelMonoSupported` is not a known tag in this runtime registry.

That remains an inference about the underlying vendor-tag registry; it does **not** prove that package/UID filtering is the reason, nor that Honor Camera receives non-null values.

## Stop rule

Do not continue trying guessed Java types, guessed names, package spoofing or privileged-security bypasses for this fixed-characteristics route.

The next useful public-API route is `CameraDevice.CameraDeviceSetup`: it can query evidence-based hypothetical SessionConfigurations without opening a camera. This provides a cleaner way to test whether Android 17 accepts specific RAW14/RAW10/RAW_SENSOR configurations than blindly attempting a capture.
