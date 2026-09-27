# D.RAW ultra-wide direct-source verification — 2026-09-27

Status: **REAL USER-CAPTURED ULTRA-WIDE SOURCE; DIRECT SCIENTIFIC INGRESS**

Private source file: `IMG_20260927_084938.dng`. The source bytes are not committed to GitHub.

## Provenance boundary

The user explicitly states that this is a self-captured **ultra-wide RAW**.
That attestation establishes self-capture and lens role only. It does not
certify runtime Camera2 physical result ID, readout domain, sensor pixel mode,
sensel/ADC provenance, calibration or fusion.

The project device map associates ultra-wide with physical camera 4. This file
has no matching runtime TotalCaptureResult, so camera 4 is recorded as
`PROJECT_CANONICAL_DEVICE_MAP_NOT_RUNTIME_RESULT`.

## Source measurements

- source SHA-256: `14757aaac784b17421598121a232c22e044f217531571697a73e4c90bff28133`
- source bytes: 24,415,168
- 4032 × 3024 BGGR
- uncompressed 16-bit CFA DNG
- Orientation 1 / TOPLEFT
- WhiteLevel 1023
- BlackLevel approximately [64.60, 64.51, 63.96, 64.00]
- f/2.0
- focal-length metadata 1.82 mm
- ISO 9796
- exposure 0.06 s
- HONOR BKQ-N49
- DateTimeOriginal 2026:09:27 08:49:38

Serialized CFA strips:

- bytes: 24,385,536
- SHA-256: `87e56296cd51a076d921c5a529dfd64784dfc8b9789869b64644b672767a89a5`
- serialized-strip hash equals decoded little-endian uint16 raster hash: true

## Direct scientific route

The original DNG passes the existing common C++ scientific pipeline directly.
No orientation quarantine or compatibility container is required.

Two independent local executions produced byte-identical result JSON:

`faa97130687d9376618491946ae89f962fb43f7ee659de0cff13c42c2fb5bfe8`

Scientific Master: `949777edb5541064e190d148775ce27d303ce1e0d35f57d7a837fc092ad9f6d9`

Authority field: `9f954a5ff0680dd879f8eef2e04b4d1c3e2031a8334f86890add135bf47c1349`

TruthNegative parent: `5ebf1ecd6a0d291a88ffab6a427e1f89e8681ce9819002773ae05920b352c53c`

D.RAWnegative v0.1: `b0ee1322ff6260eb30072533c2b3e294be46375778ac4622b908d6c746e172e0`

The pipeline remains one-frame/one-evidence, source-local only, with no
sensel/ADC certification, no new evidence and no scientific writeback.

Capture sample domain, readout domain and sensor pixel mode remain UNKNOWN.
