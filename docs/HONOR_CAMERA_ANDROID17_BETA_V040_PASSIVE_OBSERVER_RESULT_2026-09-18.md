# Honor Camera Android-17-beta APK + v0.40 passive route observer — result and boundary

Date: 2026-09-18

## Scope

This line of research is explicitly **observation-only** with respect to TruthRaw scientific authority.

The supplied Honor Camera APK is used as a software-route map. It may identify interfaces, names,
service boundaries and candidate control surfaces, but it is not RAW evidence, calibration authority,
sensor truth or Scientific Master input.

APK identity used for static analysis:

- file: `Camera.apk`
- SHA-256: `3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52`
- installed package observed on device: `com.hihonor.camera`
- observed installed version: `171.0.10.452`
- minSdk: 36
- compileSdk: 36
- targetSdk: 37

The device run remained Android 16 / API 36. No Android-17 shared-camera API is used by TruthRaw v0.40.

## v0.40 passive observer invariants

TruthRaw v0.40:

- does not call `openCamera()`;
- does not create a CaptureRequest;
- does not submit a camera capture;
- does not instantiate ImageReader;
- does not access an image buffer;
- does not write vendor request/session keys;
- registers only CameraManager availability callbacks;
- optionally binds to Honor's exported CameraAccessoriseService;
- the initial handshake invokes no AIDL method.

Authority remains:

`PASSIVE_ROUTE_OBSERVATION_ONLY`

## First route observation

A user-driven Honor Camera run showed:

- logical Camera 0 became unavailable shortly after Honor Camera launch;
- it remained unavailable while the Honor camera session was active;
- Camera 0 returned available when Honor Camera released it;
- no physical-camera availability transition was observed when the user changed from the default main view to 200 MP.

Bounded interpretation:

- CameraService ownership of logical 0 is observable;
- absence of a physical availability callback does **not** prove a physical camera was unused;
- physical availability callbacks are availability signals, not active-lens telemetry;
- the internal main -> 200 MP pipeline switch therefore remains below the v0.40 CameraService observation boundary.

## Honor CameraAccessoriseService handshake

Component:

`com.hihonor.camera/com.hihonor.camera.accessorise.aidl.CameraAccessoriseService`

Service interface descriptor returned by the live Binder:

`com.hihonor.camera.accessorise.aidl.ICameraAccessoriseService`

Empirical v0.40 result:

- `bindServiceReturned = true`
- service connected
- Binder alive
- Binder ping = true
- expected interface descriptor returned
- no AIDL method invoked
- camera ownership did not change during the handshake

Classification:

`HONOR_ACCESSORISE_BINDER_CONNECTION_PASS__NO_METHOD_INVOCATION__NO_CAMERA_OWNERSHIP_CHANGE`

## Static AIDL reconstruction from the supplied APK

DEX method prototypes prove the service interface contains:

- `setPreviewSurface(android.view.Surface)`
- `exitPreview()`
- `registerOutputConfigCallback(IOutputConfigCallback)`
- `unregisterOutputConfigCallback(IOutputConfigCallback)`
- `registerPreviewStateCallback(IPreviewStateCallback)`
- `unregisterPreviewStateCallback(IPreviewStateCallback)`
- `registerCaptureEventCallback(ICaptureEventCallback)`
- `unregisterCaptureEventCallback(ICaptureEventCallback)`

The generated/obfuscated Binder stub's packed switch maps transaction codes exactly:

1. setPreviewSurface
2. exitPreview
3. registerOutputConfigCallback
4. unregisterOutputConfigCallback
5. registerPreviewStateCallback
6. unregisterPreviewStateCallback
7. registerCaptureEventCallback
8. unregisterCaptureEventCallback

Callback interfaces reconstructed from DEX:

`IOutputConfigCallback`
- transaction 1: `onPreviewConfigChanged(int width, int height)`
- transaction 2: `onCurrentModeChanged(String mode)`

`IPreviewStateCallback`
- transaction 1: `onPreviewStateChanged(int state)`

`ICaptureEventCallback`
- transaction 1: `onCaptureEvent(int event)`

These signatures are software-interface observations only. Names and integer/string values are not
promoted to vendor semantics without device evidence.

## Authorization boundary reconstructed from APK

All three callback registration methods begin by calling the same internal authorization routine.

That routine:

1. obtains the Binder calling UID;
2. obtains packages for that UID;
3. obtains package signing information;
4. computes SHA-256 of the caller certificate;
5. permits the caller only when package/signature matches an internal allowlist;
6. otherwise reports `enforceCallingPackage / client_not_allowed`;
7. throws `SecurityException("Package not allowed: " + callingUid)`.

Static package allowlist observed in the supplied APK:

- `com.huamei.badge`
- `com.hihonor.camera`

Static certificate SHA-256 allowlist entry observed:

`0EA6FCE70AB2A77DB537318F45FC12DEFC0D95C4EC8946150FB2E40567CD5D3E`

TruthRaw will not impersonate an allowed package, spoof a signature, bypass this check, patch the
Honor service, or call `setPreviewSurface`.

## v0.41 decision

Because all three callback registration paths share the same authorization gate, v0.41 should perform
**one isolated, read-only-output callback registration test first**:

`registerOutputConfigCallback`

Why this callback first:

- it is the most directly relevant to the passthrough hypothesis;
- if accepted, it can expose only mode-name and preview-dimension callbacks;
- it does not provide a Surface;
- it does not request a capture;
- it does not write camera settings;
- it does not require testing the other callback registrations before evidence is reviewed.

Stop rule:

- if registration is rejected, record the exact Binder/SecurityException result and do not probe the
  other two callback registrations in the same run;
- if registration succeeds, keep only this callback registered and let the user operate Honor Camera
  normally;
- do not invoke `setPreviewSurface` or `exitPreview`;
- unregister the output callback on explicit stop/service disconnect.

This keeps the experiment one-boundary-at-a-time and fail-closed.
