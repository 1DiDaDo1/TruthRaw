# TruthRaw v0.42 passive MediaStore + Camera timeline — build record

Date: 2026-09-18

## Build source

Branch:

`integration/truthraw-suite-v0-42-passive-mediastore-camera-timeline`

GitHub Actions run:

`35385935882`

Build source commit:

`6bb7f63844bc61565fe94fe79a78240c6bc0abb2`

The run completed successfully.

## Verified pre-build invariants

The workflow verified that v0.42:

- declares authority `PASSIVE_SYSTEM_VISIBLE_OUTPUT_OBSERVATION_ONLY`;
- never opens a camera;
- never creates a CaptureRequest;
- never creates ImageReader;
- never reads a media input stream;
- never invokes BitmapFactory or ImageDecoder;
- never invokes ExifInterface;
- never binds an Honor service;
- never sets a physical camera key;
- never inserts/updates/deletes MediaStore rows;
- registers CameraManager availability callbacks;
- registers a MediaStore ContentObserver;
- queries MediaStore database metadata only;
- records width, height, MIME type, size, owner package and timing fields;
- requests Android media permission through the normal runtime-permission path.

Invariant result:

`v0.42 passive MediaStore + CameraManager invariants PASS`

## Build

Gradle result:

`BUILD SUCCESSFUL in 1m 39s`

APK:

- bytes: `5,102,397`
- SHA-256:
  `d56cd1876a146294e1b06bc81f4a9419ee85d1f65cc4375147b809d519df596e`

Independent post-download extraction produced the same byte count and SHA-256.

## GitHub artifact

Artifact ID:

`10563743644`

Artifact name:

`truthraw-suite-v0-42-passive-mediastore-camera-timeline-debug-arm64`

Artifact ZIP size:

`1,669,171 bytes`

Artifact digest:

`sha256:3a8aeec224c6824db535cd4eaaf0e346ac9c07dbed3cf1bd1c38acdffef02cc3`

Artifact expiry observed:

`2026-12-17T19:26:23Z`

## Provenance note

Documentation/build-record commits created after the successful build are not part of the built APK.

The APK authority is therefore tied to source commit:

`6bb7f63844bc61565fe94fe79a78240c6bc0abb2`

not to the later documentation-only branch head.

## First device-run protocol

The first v0.42 device run should contain one Honor 200 MP photo only:

1. grant MediaStore image permission if offered;
2. start v0.42 observer;
3. mark and move TruthRaw to background;
4. open Honor Camera manually;
5. keep default main preview stable briefly;
6. switch manually to 200 MP;
7. wait 2–3 seconds;
8. make exactly one photo;
9. wait 3–5 seconds;
10. leave Honor Camera;
11. return to TruthRaw and mark return;
12. request one MediaStore snapshot;
13. stop observer;
14. save/upload the JSON.

No photo file itself is required for this first pass.

## Evidence boundary

A MediaStore row is system-visible output metadata. It is not RAW capture evidence, sensor-native
geometry proof, remosaic proof, physical-lens proof, calibration authority or Scientific Master input.
