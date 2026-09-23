# TruthRaw final-chat handoff — 2026-09-23

**READ THIS FIRST IN A NEW CHAT.**

Status: **DOCUMENTATION-ONLY SNAPSHOT OF THE LAST STATE KNOWN TO THIS CHAT.**

This file intentionally does **not** claim to be the newest code anywhere in the repository. A newer branch, commit, APK, local experiment, or another chat may exist after or outside this snapshot. Before changing code, compare this handoff with repository history and any newer handoff/state file.

## Code-freeze / user instruction

The user ended this chat with an explicit instruction:

> Update the project with the latest knowledge from this chat, but do not change code further.

Therefore this handoff and its companion state file are documentation only. Do not interpret their commit time as a new code-bearing TruthRaw release.

## Frozen production/integration anchor

Frozen branch:

`integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master`

Frozen code commit:

`172a100786eb18d4b08564bbfb025a44f42cfa1e`

Frozen handoff:

`docs/handoff/TRUTHRAW_FINAL_CODE_FREEZE_V0843_2026-09-21.md`

Frozen APK identity known to this chat:

- file: `TruthRaw_v0.84.3_Final_Freeze_TruthNegative_200MP_FullColour_debug_arm64.apk`
- bytes: `6,095,333`
- SHA-256: `5805d291b163d66e68d5ab98aa4d2971e38b062b724325469d6002fbd8b1917e`

This frozen branch remains the production/scientific reference. Later Camera-5 work below is isolated experimental research and must not silently overwrite the frozen branch.

## Permanent scientific law

- Source Evidence / Direct-CFA evidence is immutable once sealed.
- Physical single-frame evidence remains `physicalFrameCount=1` and `independentEvidenceCount=1`.
- Measured, reconstructed and appearance domains remain separate.
- Scientific Master is separate from export/presentation.
- APK/GCam/OEM computational output cannot define TruthRaw evidence or calibration.
- Representation may exceed the source; knowledge claims may not exceed the evidence.
- A declared/envelope geometry is not proof that all samples are populated.
- No OEM 200MP result may be called measured 200MP CFA unless the actual sensor-visible evidence proves it.

## TruthNegative / Float32 architecture

Current known production concept:

`admitted 4080x3072 CFA -> camera-native Scientific Master -> TruthNegative 4x dense -> 16320x12288 Float32 LinearRaw DNG`

TruthNegative is reconstructive, not measured native 200MP evidence.

Known method:

`PIXEL_CENTER_BILINEAR_F32_EXACT_ORDER_V0_3`

Rules:
- exact 4x each axis;
- target authority `RECONSTRUCTED_DENSE_SUPPORT`;
- target measured claim count = 0;
- no new evidence;
- original master unchanged;
- negative and >1 Float32 values remain valid;
- no sharpening/AI/texture synthesis is part of the scientific dense-support construction.

## Camera-5 / Honor Magic 8 Pro anchor

Known device/camera identity used by the project:

- logical preview camera: 0
- main physical: 2
- wide physical: 4
- tele physical: 5
- tele focal length: 22.48 mm
- Android 17 / API 37 during the latest tests

Public Camera2 characteristics for physical Camera 5 report:

- STANDARD RAW_SENSOR / RAW10: 4080x3072
- MAXIMUM_RESOLUTION RAW_SENSOR / RAW10: 8160x6144
- maximum high-resolution RAW_SENSOR / RAW10: 16320x12288
- standard sensor array: 4080x3072
- maximum-resolution pixel array: 16320x12288
- standard `android.sensor.info.binningFactor`: 2x2

Do not infer 4x4 physical binning from the 4080 -> 16320 linear ratio.

## Previous 16320x12288 RAW_SENSOR result

The public physical-5 Camera2 route can return a 16320x12288 RAW_SENSOR envelope of 401,080,320 bytes.

Observed topology:
- rowStride = 32640
- pixelStride = 2
- only rows 0..767 populated
- rows 768..12287 zero
- meaningful prefix = 25,067,520 bytes
- byte-equivalent standard RAW domain = exactly 4080x3072 U16

Classification retained:

`APP_VISIBLE_4080x3072_BAYER_LIKE_RAW_PAYLOAD_EMBEDDED_IN_16320x12288_HAL_ENVELOPE`

This is app-visible Camera2 RAW evidence, not untouched photodiode/ADC proof.

## v0.56c — 8160x6144 RAW_SENSOR experiment

Experimental branch lineage:

`test/cam5-8160x6144-raw-v056`

Important successful source/evidence capture stem:

`1790122918857`

Evidence file:

`TRUTHRAW_1790122918857_CAM5_8160_PROBE_EVIDENCE_v056.json`

Sealed source:

`TRUTHRAW_1790122918857_CAM5_8160_PROBE_SOURCE_8160x6144_v056.rawsensor`

Measured container:
- 8160x6144
- 50,135,040 U16 positions
- 100,270,080 bytes
- rowStride = 16320
- pixelStride = 2
- SHA-256 = `756cf869184fc8b1d0a2c29ef0f7e79aa9ae16b8693ac26adf49b989517559be`

But only:
- rows 0..1535 are populated;
- rows 1536..6143 are zero;
- populated byte extent = exactly 25,067,520 bytes;
- exact unique advertised STANDARD RAW byte match = 4080x3072.

Therefore:

`8160x6144 RAW_SENSOR CAPABILITY != 8160x6144 POPULATED RAW SAMPLE DOMAIN`

The public max-resolution RAW_SENSOR route still exposes only a 4080x3072-equivalent meaningful payload in the observed capture.

The capture request was physical Camera 5, physical-scoped, MAX output-declared, with physical SENSOR_PIXEL_MODE request write/readback 1; the returned physical result still reported SENSOR_PIXEL_MODE 0 and `rawBinningFactorUsed=true`.

## v0.57 — 8160x6144 RAW10 experiment

Experimental branch:

`test/cam5-8160x6144-raw10-v057`

Successful code-bearing commit:

`5a95b7075eacfcd5e9b478cd2877e92924c93d57`

Successful CI run:

`35855642767`

Two user device captures were returned:

1. stem `1790163972953`
2. stem `1790164019677`

Both report:
- physical Camera 5
- 8160x6144
- RAW10
- timestamp identity PASS
- physical frame count 1
- independent evidence count 1
- rowStride = 10240
- pixelStride = 0 (packed RAW10)
- total accessible bytes = 62,914,560
- `rawBinningFactorUsed=true`
- no DNG/Main House/Scientific Master admission

Source hashes:
- capture 1: `0e8614f086aa9347877bfeeecf3abf4ff9bde72cb032c112ff618f8e2bb03601`
- capture 2: `68c11fa279d57b8f7c4fa77c0d7f8390daf768991bbfd639c410594a962ed48c`

The two captures are therefore independent/fresh byte payloads.

Manual byte-structure analysis performed in this chat found the same topology in both:

- full Image.Plane envelope: 10240 bytes/row x 6144 rows = 62,914,560 bytes;
- meaningful region occupies only the first 3072 rows;
- each meaningful row is consistent with:
  - 5100 bytes packed 4080-pixel RAW10 data
  - 20 zero padding bytes
  - 5120 bytes effective row slot
- first meaningful region = 5120 x 3072 = 15,728,640 bytes;
- packed pixel data alone = 5100 x 3072 = 15,667,200 bytes;
- remaining tail of the 8160x6144 envelope is zero.

Working classification:

`APP_VISIBLE_4080x3072_PACKED_RAW10_PAYLOAD_EMBEDDED_IN_8160x6144_CAMERA2_ENVELOPE`

This is stronger evidence that the 12.5MP public sample-domain is not merely a RAW_SENSOR/U16 interpretation artifact, because a different Camera2 storage format independently points to the same 4080x3072 domain.

Do not promote this to native sensor/ADC geometry proof.

## v0.58 — 4080x3072 STANDARD RAW10 control

The next controlled experiment was built to compare the v0.57 embedded payload with a normal standard RAW10 capture.

Branch:

`test/cam5-4080x3072-raw10-control-v058`

Code-bearing build head:

`e8639e79197ce589a8bb8a096d1dd8cb926757ec`

Successful CI run:

`35857800114`

Artifact:
- `truthraw-cam5-4080x3072-raw10-control-v058-debug-arm64`
- GitHub artifact id: `10747379436`
- artifact ZIP digest: `sha256:94b9eceedb13ce484295b5158e5654c2b9af587be6b39fb344299fb0bdd5fa2a`

Experiment design:
- physical Camera 5
- STANDARD RAW10 4080x3072
- no MAXIMUM_RESOLUTION OutputConfiguration declaration
- no physical SENSOR_PIXEL_MODE MAX request
- source-first sealing
- automatic source/evidence export
- analysis-only; no DNG/Main House/Scientific Master promotion

**Important:** the device result for v0.58 had not yet been returned to this chat when the user requested this final handoff. A newer chat may already contain that result. Check before repeating the experiment.

## OEM 200MP / Honor Camera static route

Installed Honor Camera Android-17 package previously fingerprinted as version `171.0.10.706`.

The app-level high-pixel architecture retains:
- `50M -> UltraResolutionMode`
- `200M -> UltraHighPixelMode`
- ServiceHost-based `UltraHighPixelModeProcessor`
- pipeline names including:
  - `pipeline4capdavinci.json`
  - `pipeline4rawmfultrahighpixelcap.json`
  - `pipeline4arcmfnrmscap.json`
  - `pipeline4hdrcap.json`
  - `pipeline4capbackremosaic.json`
  - `pipeline4mfdncap.json`

Static software analysis recovered a strong chain:

`hintUserValue=5 -> isRemosaicEnable=1 -> qcomRemosaicEnable=1 -> SMART_SCENE_MODE=5 -> scene5 -> pipeline4capbackremosaic.json`

This is software-route evidence only.

For scene 5, app-layer code was observed to set capture-count/delay state consistent with a single app-level capture request. This weakens a simple ordinary multi-frame-fusion explanation, but does not prove that ServiceHost/native/HAL does not create hidden internal frames or processing stages.

Current working hypothesis remains:

`one high-resolution sensor/readout context -> vendor remosaic/high-pixel processing -> 16320x12288 internal/output buffer -> JPEG`

This is an inference, not sensor evidence.

## Public Camera2 boundary now strongly constrained

Combined observed evidence:

- 16320x12288 RAW_SENSOR envelope -> 4080x3072 meaningful U16 payload
- 8160x6144 RAW_SENSOR envelope -> 4080x3072 meaningful U16 payload
- 8160x6144 RAW10 envelope -> 4080x3072 meaningful packed RAW10 payload

Therefore the public Camera2 path observed by TruthRaw has not exposed an independently populated 50MP/200MP RAW sample-domain on physical Camera 5.

This does **not** prove the tele sensor cannot internally perform a higher-resolution readout. It means that such a signal, if used by Honor's 200MP route, is not visible through the tested public RAW_SENSOR/RAW10 path.

## Closed / low-value routes

Do not restart blind vendor-key sweeps without a new concrete surface.

Previously tested, representation-correct controls include:
- EnableIdealRAW
- RawCbSourceType
- EnableXCFAOptimization
- HALOutputBufferCombined
- EnableInsensorZoom
- EnableSnapshotOnlyInsensorZoom
- EnableMCXMasterCb
- inSensorZoomEnable
- EnableVSR
- ExtendedMaxZoom
- enableQLL

These did not change the observed public RAW topology in their tested contexts.

Honor CameraAccessoriseService:
- Binder connection works;
- callback registration is package/signature gated;
- `registerOutputConfigCallback` returned SecurityException;
- do not spoof package/signature or bypass this boundary.

RAW14:
- Android 17 runtime exposes ImageFormat.RAW14;
- physical Camera 5 did not advertise RAW14 output sizes in the tested public characteristics;
- no straightforward RAW14 app route was found in the exact Honor Camera .452/.706 static comparison.

## Correct next-read order

A new chat should first determine whether there is newer work than this handoff.

If not, read in this order:

1. `state/CURRENT_CHAT_KNOWN_STATE_2026-09-23.json`
2. this handoff
3. `docs/handoff/TRUTHRAW_FINAL_CODE_FREEZE_V0843_2026-09-21.md`
4. `state/CURRENT_PROJECT_STATE_2026-09-21.json`
5. Camera-5 historical route handoff:
   `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md`
6. `docs/HONOR_CAMERA_STATIC_DIFF_171_0_10_452_TO_706_2026-09-19.md`
7. exact v0.56/v0.57/v0.58 branch history and device evidence before changing any capture code.

## Immediate continuation if no newer work exists

First task:

**Inspect whether a v0.58 device capture/result already exists.**

If it does, compare the normal 4080x3072 STANDARD RAW10 row structure against v0.57:
- rowStride
- packed bytes per row
- row padding
- total accessible bytes
- all-row population
- byte/pixel decoder behavior

If the standard control produces the same 5100 packed bytes + 20 padding per row for all 3072 rows, then v0.57 can be described even more precisely as the ordinary standard 4080x3072 RAW10 row layout embedded at the start of the larger 8160x6144 envelope.

Do not repeat the v0.58 capture if a newer chat already completed it.

## Final user instruction

No further code changes were requested after this handoff was created.

Documentation may be updated to reconcile newer evidence, but code must remain untouched unless the user explicitly authorizes code changes again.
