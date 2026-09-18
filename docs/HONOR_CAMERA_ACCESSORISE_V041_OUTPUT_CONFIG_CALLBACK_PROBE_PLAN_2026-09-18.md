# HONOR CameraAccessorise v0.41 — single output-config callback boundary probe

Date: 2026-09-18

## Why v0.41 exists

v0.40 proved that an ordinary TruthRaw process can bind to the exported Honor CameraAccessoriseService
and receive a live Binder whose descriptor is:

`com.hihonor.camera.accessorise.aidl.ICameraAccessoriseService`

That handshake did not invoke an AIDL method and did not change camera ownership.

Static reconstruction of the supplied Honor Camera APK then showed that all three passive callback
registration methods call the same authorization routine before registering anything.

v0.41 therefore tests exactly one boundary:

`registerOutputConfigCallback(IOutputConfigCallback)`

No other Honor service method is invoked before this result is reviewed.

## Supplied APK identity

- APK SHA-256:
  `3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52`
- package: `com.hihonor.camera`
- observed installed version: `171.0.10.452`
- minSdk 36
- compileSdk 36
- targetSdk 37
- tested runtime: Android 16 / API 36

The APK remains software-route evidence only.

## DEX-derived service contract

Exact service descriptor:

`com.hihonor.camera.accessorise.aidl.ICameraAccessoriseService`

The Binder stub uses a packed switch with first key 1 and eight entries. Exact transaction map
reconstructed from the onTransact bytecode:

1. `setPreviewSurface(Surface)`
2. `exitPreview()`
3. `registerOutputConfigCallback(IOutputConfigCallback)`
4. `unregisterOutputConfigCallback(IOutputConfigCallback)`
5. `registerPreviewStateCallback(IPreviewStateCallback)`
6. `unregisterPreviewStateCallback(IPreviewStateCallback)`
7. `registerCaptureEventCallback(ICaptureEventCallback)`
8. `unregisterCaptureEventCallback(ICaptureEventCallback)`

v0.41 permits only transaction 3, plus transaction 4 **only if** transaction 3 succeeded and cleanup
is required.

Transactions 1, 2, 5, 6, 7 and 8 are outside this experiment.

## DEX-derived callback contract

Exact output callback descriptor:

`com.hihonor.camera.accessorise.aidl.IOutputConfigCallback`

The Honor proxy code proves:

- callback transaction 1:
  `onPreviewConfigChanged(int width, int height)`
- callback transaction 2:
  `onCurrentModeChanged(String mode)`

The proxy uses ordinary two-way Binder transact calls and reads exceptions from the reply.

TruthRaw v0.41 implements only this callback descriptor and these two transaction layouts.
Unknown callback transaction codes are not interpreted.

## Authorization gate

The beginning of each of the three Honor callback registration methods calls the same internal
authorization method before touching the supplied callback.

The DEX-derived gate:

1. `Binder.getCallingUid()`
2. `PackageManager.getPackagesForUid(uid)`
3. package signing-info retrieval
4. SHA-256 certificate digest
5. package allowlist OR certificate allowlist test
6. on rejection: report `enforceCallingPackage / client_not_allowed`
7. throw `SecurityException("Package not allowed: " + uid)`

Static package allowlist:

- `com.huamei.badge`
- `com.hihonor.camera`

Static signing certificate SHA-256 allowlist:

`0EA6FCE70AB2A77DB537318F45FC12DEFC0D95C4EC8946150FB2E40567CD5D3E`

This static result predicts that an ordinary TruthRaw debug build will probably be rejected, but the
prediction is not promoted to a device result until v0.41 performs exactly one registration call.

## v0.41 runtime rules

TruthRaw v0.41:

- does not call CameraManager.openCamera;
- does not create a CaptureRequest;
- does not submit a capture;
- does not create ImageReader;
- does not receive or read a camera image buffer;
- does not set vendor request/session keys;
- does not provide a preview Surface;
- does not call `setPreviewSurface`;
- does not call `exitPreview`;
- does not register capture-event callback;
- does not register preview-state callback;
- does not bypass or spoof Honor's package/signature authorization.

It does:

1. start a foreground passive observer;
2. register Camera2 availability callbacks;
3. bind the Honor CameraAccessoriseService;
4. verify the live Binder descriptor;
5. execute one raw Binder transaction:
   `registerOutputConfigCallback` / code 3;
6. call `Parcel.readException()` and record the exact returned exception boundary;
7. stop there if rejected;
8. if accepted, keep only the output-config callback alive and record exact callback payload values;
9. unregister that same callback on stop with transaction 4.

## Stop rules

### Rejection

If registration returns a SecurityException or another error:

- record the exact class/message;
- classify the boundary;
- do not try PreviewState registration;
- do not try CaptureEvent registration;
- do not open Honor Camera for this run;
- save the v0.41 JSON and review before any next experiment.

### Unexpected success

If registration succeeds:

- do not register any other callback;
- leave the callback alive;
- user may open Honor Camera manually;
- record `onPreviewConfigChanged(width,height)`;
- record `onCurrentModeChanged(modeString)`;
- preserve values exactly as observations;
- do not assign semantic truth from names/values alone;
- unregister on stop.

## Scientific authority

All v0.41 results remain:

`PASSIVE_SOFTWARE_ROUTE_OBSERVATION_ONLY`

An Honor callback is not RAW evidence, calibration, sensor-native geometry proof, or Scientific Master
input. It can only guide later controlled acquisition tests.
