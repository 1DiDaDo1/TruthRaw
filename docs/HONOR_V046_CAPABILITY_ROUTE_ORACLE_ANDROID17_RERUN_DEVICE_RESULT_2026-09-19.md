# Honor v0.46 — Android 17 unchanged-oracle rerun

Date: 2026-09-19

The same v0.46 schema was run immediately after the BKQ-N49 was updated from Android 16 to Android 17.

## Source

- file: `android 17.json`
- bytes: `7784`
- SHA-256: `8b03c6e2dc51666951524cddd80fa540a27e526e4e972753979f4495495531c1`

## Runtime transition

Before:

- Android 16
- SDK 36
- fingerprint `HONOR/BKQ-N49/HNBKQ:16/HONORBKQ-N49/10.0.0.199C636E4R106P1:user/release-keys`

After:

- Android 17
- SDK 37
- fingerprint `HONOR/BKQ-N49/HNBKQ:17/HONORBKQ-NXX/11.0.0.120C901E8:user/release-keys`

The report still records TruthRaw target SDK 35. This is useful because the observed runtime change is not caused by retargeting the TruthRaw package.

## Result

Standard Camera2 identity is stable:

- public IDs: `0`, `1`
- rear physical IDs: `2`, `4`, `5`
- physical 5: **22.48 mm / f2.6**

All five targeted Honor capability names remain absent from the CameraCharacteristics key set on every checked ID `0,1,2,4,5`:

- `physicalCameraScene`
- `sceneCameraIdCapability`
- `cameraIdCustomInfo`
- `needOpenPhysicalCamera`
- `ultraResolutionSwitchSupportedSize`

Therefore Android 17 did **not** make this particular static-APK capability surface visible to the existing third-party Camera2 characteristics reader.

## Boundaries

This does not say that Honor's internal high-pixel path is unchanged. It only says that these five names did not become ordinary CameraCharacteristics keys for this TruthRaw client.

The JSON does not include an installed-APK self-hash. It proves unchanged schema/targetSdk behavior, but TruthRaw does not promote "byte-identical v0.46 APK" as a device-measured fact from this file alone.

## Next

Proceed to v0.47 with Android-17-specific capability observation:

1. resolve `ImageFormat.RAW14` on the actual runtime;
2. enumerate RAW_SENSOR / RAW10 / RAW12 / RAW14 for default and maximum-resolution stream maps;
3. keep normal and high-resolution output lists separate;
4. query public Camera Extension support and the API-37 `isExtensionSupported(int)` surface without creating an extension session;
5. do not capture until a concrete new route is observed.
