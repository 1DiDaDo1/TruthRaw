# v0.41 build log — Honor output-config callback boundary probe

Date: 2026-09-18

Branch:
`integration/truthraw-suite-v0-41-honor-output-config-callback-probe`

## Run 1 — failed before build

GitHub Actions run: `35383673901`

Head:
`cd1ba36065e2f1ccce57c763b0429cc22be71a1d`

Result: **FAILURE in invariant validation, before Android build**.

Cause:

The Python invariant asserted that the literal text `setPreviewSurface` must not occur anywhere in
the service source. The service intentionally contains evidence labels such as
`setPreviewSurfaceInvoked=false`, even though it never calls the method. The shell-level
method-call check `setPreviewSurface(` was already correct.

Fix:

Narrow invariant from substring absence to actual method-call syntax:

`setPreviewSurface(`

and equivalently for `exitPreview(`.

Scientific/runtime code was not weakened.

## Run 2 — failed before build

GitHub Actions run: `35383725740`

Head:
`6980e0bc36155264114b5a61f190dfabd054e46f`

Result: **FAILURE in invariant validation, before Android build**.

Cause:

The Python invariant asserted that the literal text `ImageReader` must not occur anywhere. The
service KDoc truthfully states that v0.41 uses "no ImageReader", so the documentation itself tripped
the test.

Fix:

Narrow the invariant to prove that the forbidden Android class is not imported:

`import android.media.ImageReader`

The existing checks for camera-open/capture-request/vendor-write method calls remain.

Scientific/runtime code was not weakened.

## Run 3 — successful build

GitHub Actions run: `35383782841`

Head:
`5cede9289ec8434f1208740c7db1d13a5264e279`

Result: **SUCCESS**

Verified stages:

- checkout
- v0.41 single-boundary invariant suite
- Java 17 setup
- Android SDK/NDK setup
- Gradle setup
- arm64 debug assembly
- APK/native bridge verification
- artifact upload

Build log markers:

`v0.41 single output-config Binder boundary invariants PASS`

`BUILD SUCCESSFUL in 2m 6s`

APK:

- bytes: `5,069,413`
- SHA-256:
  `9a3acbfd72f3aa25570e800cc89db32d5c6f9cfbf7e90b5a85d3aa73728b410c`

Artifact:

- artifact ID: `10563480355`
- name:
  `truthraw-suite-v0-41-honor-output-config-callback-probe-debug-arm64`
- ZIP digest:
  `sha256:e2afdb46af689b70c229a9a2abe77e9f0fda69e5f8716d5f89f51bd53d6e45fa`

## Build conclusion

The final APK preserves the v0.41 experiment contract:

- one Honor service registration boundary only;
- service transaction 3 only for the experiment;
- transaction 4 only for cleanup after a successful registration;
- no Surface;
- no Honor camera open;
- no capture request;
- no image buffer;
- no vendor request/session key;
- no preview-state callback registration;
- no capture-event callback registration;
- no package/signature spoofing or authorization bypass.

The two earlier failures are retained as provenance and are not erased from the development line.
