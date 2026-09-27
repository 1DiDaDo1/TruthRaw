# D.RAW main-camera source verification — 2026-09-27

Status: **REAL SOURCE PRE-ADMISSION CANDIDATE; NOT SCIENTIFICALLY ADMITTED**

Recovered Library source: `C2OBS_20260914_174825_895_4096x3072.dng`.

Verified directly from source bytes on 2026-09-27:

- file SHA-256: `a85cac58601d6cc8138bf8f9372e5161b85ab1e89afa70372f97c46461dcea79`;
- bytes: `25,195,932`;
- DNG/TIFF: little-endian, uncompressed CFA, 4096×3072, 16-bit;
- CFA: BGGR;
- focal length metadata: `6.55 mm`;
- ISO: `100`;
- exposure: `16,367,389 ns`;
- dynamic WhiteLevel: `1023`;
- dynamic BlackLevel: `[63.984375, 63.984375, 64, 64]`;
- serialized CFA strip bytes: `25,165,824`;
- serialized CFA strip SHA-256: `4818fd406cc528984ff57e036db49ac394ce0bb6b6d0830e2f9f1ad95ecc8c3c`;
- TIFF strip hash equals decoded little-endian uint16 raster hash: **true**.

Acquisition record `C2OBS_20260914_174825_895_camera2_observation_v0_1.json`
SHA-256:

`ecd949e0163b957d067c363e9452d849cda309587cd637726d609bdb353c014b`

Camera2 observation facts:

- logical camera 0;
- logical multi-camera = true;
- advertised physical children = 2, 4, 5;
- requested physical camera = null;
- measured active physical result = 2;
- RAW_SENSOR, direct CFA measurement;
- processed RGB input = false;
- multi-frame evidence merged = false;
- sensor/image timestamps both `491612870219361`;
- exact timestamp match = true;
- noise reduction result mode = 0;
- OIS result mode = 0.

The legacy acquisition record explicitly left `captureSampleDomainId` and
`gainReadoutStateId` unresolved. Therefore D.RAW Pre-Admission v0.2 records
the serialized storage domain as SOURCE_BOUND while capture sample domain,
readout domain and sensor pixel mode remain UNKNOWN.

No Scientific Master, D.RAWnegative, calibration, shared gauge,
cross-observation relation or fusion is admitted by this evidence record.

Manifest state SHA-256:

`cc85275fe542271bbc6c5406903d9277b03f859aa248aa89d16732c1aab07f38`
