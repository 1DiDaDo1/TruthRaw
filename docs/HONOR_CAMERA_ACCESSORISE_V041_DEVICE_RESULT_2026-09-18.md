# HONOR CameraAccessorise v0.41 — device result

Date: 2026-09-18

## Classification

**OUTPUT_CONFIG_CALLBACK_REGISTRATION_REJECTED_BY_SECURITY_BOUNDARY**

This is an empirical device result on Android 16 / API 36.

## Scientific / authority boundary

TruthRaw remained observation-only:

- Scientific Master modified: false
- capture evidence granted: false
- Honor callback semantic authority granted: false
- camera opened by TruthRaw: false
- capture submitted by TruthRaw: false
- image buffer accessed by TruthRaw: false
- vendor request written by TruthRaw: false
- preview Surface provided by TruthRaw: false
- exitPreview invoked by TruthRaw: false
- CaptureEvent callback registration invoked: false
- PreviewState callback registration invoked: false

Exactly one Honor service-method boundary was tested:

`registerOutputConfigCallback(IOutputConfigCallback)`

Service transaction code: `3`.

No package/signature spoofing or allowlist bypass was attempted.

## Device timeline

Runtime:

- Android release: 16
- SDK: 36
- TruthRaw target SDK: 35

Observed sequence:

1. passive observer started;
2. Camera 0 and Camera 1 were available;
3. output-config registration probe began;
4. `bindService()` returned true;
5. Honor CameraAccessoriseService connected;
6. live Binder was alive and pingable;
7. live descriptor:
   `com.hihonor.camera.accessorise.aidl.ICameraAccessoriseService`;
8. TruthRaw sent exactly service transaction 3 with an
   `IOutputConfigCallback` Binder;
9. Binder transact itself returned true;
10. `Parcel.readException()` returned:
    `java.lang.SecurityException: Package not allowed: 10573`;
11. callback registration remained false;
12. no other Honor callback-registration method was attempted.

Elapsed time from service-connected event to registration-result event was approximately 2.7 ms.
Elapsed time from probe-start to the SecurityException result was approximately 14.8 ms.

## Static prediction vs device result

The supplied Honor Camera APK had already shown that all three callback registration methods call a
shared authorization routine that validates Binder calling package/signature.

Static allowlist observed in the APK:

- `com.huamei.badge`
- `com.hihonor.camera`

Static certificate SHA-256 allowlist entry:

`0EA6FCE70AB2A77DB537318F45FC12DEFC0D95C4EC8946150FB2E40567CD5D3E`

The v0.41 device result confirms that an ordinary TruthRaw process is rejected at this boundary.

## Closed route decision

Do **not** continue with:

- `registerPreviewStateCallback`;
- `registerCaptureEventCallback`;
- package-name impersonation;
- certificate/signature spoofing;
- patching Honor's service;
- `setPreviewSurface`;
- `exitPreview`.

Reason:

The first registration method reached the shared security boundary and was rejected exactly as the
static APK analysis predicted. Testing the two sibling registrations would not add an independent
authorization route; it would repeat the same gate.

## What remains open

The broader passthrough idea remains viable only through interfaces that do not require bypassing
Honor's package/signature trust boundary.

The next clean observation layer should therefore move outside CameraAccessoriseService and observe
system-visible consequences while Honor remains camera owner:

- CameraManager availability timeline;
- user mode/shutter markers;
- MediaStore insertion/change timeline;
- output metadata only: URI/ID, owner package when exposed, MIME type, width, height, byte size,
  date taken/added/modified, relative path, pending state;
- no file pixel read;
- no EXIF/image decode required for the first pass;
- no Honor Binder callback;
- no camera ownership by TruthRaw.

This can test whether a manual 200 MP switch/capture produces a distinct system-visible output event
and geometry without intercepting the capture pipeline.

## Bounded conclusion

v0.40 proved:

`BINDER_CONNECTION_PASS`

v0.41 proves:

`BINDER_METHOD_AUTHORIZATION_REJECT`

Together they define the boundary precisely:

`HONOR_ACCESSORISE_SERVICE_IS_BINDABLE_BUT_CALLBACK_REGISTRATION_IS_PACKAGE_SIGNATURE_GATED_FOR_ORDINARY_TRUTHRAW_PROCESS`

This is a useful negative result and closes the direct Honor callback side-door without weakening any
TruthRaw scientific or provenance rule.
