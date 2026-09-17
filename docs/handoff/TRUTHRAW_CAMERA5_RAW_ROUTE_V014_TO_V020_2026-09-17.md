# TruthRaw Camera-5 RAW route handoff — v0.14 through v0.20

Date: 2026-09-17  
Status: **CURRENT DETAILED CAMERA-5 HANDOFF / REPRODUCTION MAP**

This document records the exact investigation path from the first preserved 16320x12288 Camera-5 RAW_SENSOR delivery to the current v0.20 finding that the populated source prefix is a unique 4080x3072 standard-RAW geometry candidate with Bayer-like structure.

It is written so that a future chat, developer or audit can find the relevant branch, workflow, patch, helper, evidence object and scientific boundary without reconstructing the history from memory.

---

## 0. Permanent rules before reading the timeline

1. The original app-visible Camera2 source bytes are immutable evidence.
2. The primary source is the sealed `Image.Plane[0]`, not the auxiliary DNG.
3. A `16320x12288` Image/HardwareBuffer declaration proves an app-visible envelope, not 200 million independent native ADC samples.
4. Measured evidence, route metadata, geometry interpretation, reconstruction and appearance remain separate authority classes.
5. A metadata mismatch may lower interpretation authority, but must not erase already captured primary evidence.
6. Failed and rejected routes remain project provenance.
7. v0.19/v0.20 content analysis is integer/source-domain first; F64 is only for derived statistics.
8. TruthRange/zero-line is downstream and does not participate in this acquisition/topology proof.

---

## 1. Static route discovery before v0.14

The decisive Camera-5 RAW_SENSOR inventory has four distinct lists:

1. standard map output sizes
2. standard map high-resolution sizes
3. maximum-resolution map output sizes
4. maximum-resolution map high-resolution sizes

Observed Camera-5 route family:

- standard RAW_SENSOR output: `4080x3072`
- standard RAW_SENSOR high-resolution: empty
- maximum-resolution RAW_SENSOR output: `8160x6144`
- maximum-resolution RAW_SENSOR high-resolution: `16320x12288`

Important lesson: omitting `maximumMap.getHighResolutionOutputSizes(ImageFormat.RAW_SENSOR)` falsely hides the 16320x12288 route.

Primary authority document:

`docs/HONOR_MAGIC8_PRO_TELE_200MP_FULL_RAW_V014_2026-09-17.md`

Static guard retained in repository:

- `tools/camera5_200mp_android_contract_v04.py`
- `tests/test_camera5_200mp_android_contract_v04.py`

---

## 2. v0.9 -> v0.13: failure ladder that established the correct route

### v0.9

Problem: staged discovery omitted the decisive maximum/high-resolution RAW_SENSOR list.

Finding: capability logic itself was incomplete.

Lesson: inventory all four stream-map paths independently.

### v0.10

Finding: exact `16320x12288` RAW_SENSOR appeared under the maximum-resolution high-resolution route. Preview through logical camera 0 at 3.7x could report active physical camera 5.

Lesson: route visibility and preview selection are useful telemetry, but do not prove a final physical RAW frame.

### v0.11

Problem: Android rejected the capture because sensor-pixel-mode and stream configuration did not agree.

Lesson: the maximum-resolution output/session declaration and request path must agree.

Baseline checked-in activity used by the staged patch chain:

`suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt`

### v0.12

Patch:

`tools/patch_fotograaf_v012_forced_physical_max.py`

Finding:

- logical/global MAX setting was not available on this route;
- physical Camera-5 MAX setting could be written;
- physical override advertisement still reported false.

Problem: the app required global MAX and blocked before submit.

Lesson: absence of the logical global key is not proof that the physical high-resolution route is impossible.

### v0.13

Patch:

`tools/patch_fotograaf_v013_physical_only_max_submit.py`

Finding: the app received an exact `16320x12288` Image, physical Camera-5 result, and exact Image/result timestamp binding.

Problem: returned physical `SENSOR_PIXEL_MODE=0` was interpreted as a veto before the source buffer was persisted.

Lesson: preserve primary bytes before interpreting contradictory/advisory metadata.

---

## 3. v0.14: source-first sealing becomes the acquisition authority

Key patch:

`tools/patch_fotograaf_v014_seal_raw_before_result_mode_gate.py`

Workflow:

`.github/workflows/android-truthraw-suite-v0-14-seal-200mp-raw.yml`

Primary route document:

`docs/HONOR_MAGIC8_PRO_TELE_200MP_FULL_RAW_V014_2026-09-17.md`

Trusted topology:

`logical 0`
`-> physical output 5`
`-> RAW_SENSOR ImageReader 16320x12288`
`-> OutputConfiguration physicalCameraId=5`
`-> addSensorPixelModeUsed(MAXIMUM_RESOLUTION)`
`-> physical-scoped still request where accepted`
`-> physical Camera-5 TotalCaptureResult`

Mandatory evidence order introduced by v0.14:

1. require `16320x12288` Image;
2. require physical result camera 5;
3. require exact Image timestamp = physical SENSOR_TIMESTAMP;
4. inspect plane/stride/buffer bounds;
5. **persist and SHA-256 seal Plane[0]**;
6. only then record returned pixel mode/binning/vendor metadata;
7. optional DNG last.

Qualifying historical v0.14 source facts:

- `16320x12288`
- `200,540,160` declared sample positions
- pixel stride `2`
- row stride `32640`
- payload bytes `401,080,320`
- canonical contiguous condition true
- historical qualifying source SHA-256 `af3ad73e5919b816881a661f00ffd84a7b537f23198a5877c242717c5d7526de`
- returned `SENSOR_PIXEL_MODE=0` preserved as a mismatch instead of destroying the evidence.

Current interpretation of v0.14: it proves one app-visible physical-Camera-5 `16320x12288` RAW_SENSOR envelope/capture delivery with exact timestamp binding and source sealing. It does not prove all declared positions carry independent or even populated source measurements.

---

## 4. v0.15: rejected direct-Camera-5 route

Historical branch:

`integration/truthraw-suite-v0-15-direct-camera5-full-raster`

The direct route attempted to open physical camera 5 as a public camera and used a simple 16-band non-zero test.

Device observation: public camera IDs were effectively logical IDs while physical 5 remained a child of logical 0; the direct-open assumption therefore failed on this device.

Permanent rejected-route lesson:

- do not replace the proven logical-0 -> physical-5 topology with direct-open Camera 5;
- a non-zero-band check alone is insufficient because repeated/interpolated content can pass it.

v0.15 is retained as historical technique/provenance, not as the trusted acquisition base.

---

## 5. v0.16: Stage 3.5 HAL / HardwareBuffer envelope

Branch:

`integration/truthraw-suite-v0-16-hal-buffer-envelope`

Workflow:

`.github/workflows/android-truthraw-suite-v0-16-hal-buffer-envelope.yml`

Key implementation helpers:

- `suite_android/app/src/main/java/com/truthraw/adaptiveui/Camera2EnvelopeProbe.kt`
- `suite_android/app/src/main/cpp/hardware_buffer_envelope_probe.cpp`

Goal: observe the delivery envelope around the same already-sealed RAW without locking, mapping or writing the HardwareBuffer.

Observed envelope class:

- Image `16320x12288`
- RAW_SENSOR format 32
- one plane
- pixel stride 2
- row stride 32640
- capacity 401080320
- HardwareBuffer `16320x12288`
- one layer
- native stride 16320 pixels
- Java/native buffer ID identity
- no probe lock/map/write.

Vendor metadata exposed valuable unknown-semantics sidecars, including HONOR binning/crop/sensor metadata and QTI multicamera/AF metadata.

Important architectural result: the useful 'header' is not an in-band file header. It is a **buffer envelope + capture/vendor metadata sidecar** around the raw pixel bytes.

---

## 6. v0.17: Camera-5 two-door airlock

Branch:

`integration/truthraw-suite-v0-17-camera5-route-fingerprint`

Workflow:

`.github/workflows/android-truthraw-suite-v0-17-camera5-route-fingerprint.yml`

Authority document:

`docs/HONOR_CAMERA5_TWO_DOOR_AIRLOCK_V017_2026-09-17.md`

Key request-side helper:

`suite_android/app/src/main/java/com/truthraw/adaptiveui/Camera2PreHalGate.kt`

Airlock model:

`Gate A: before session creation`
`-> Gate B: before still request build/submit`
`-> HONOR/QTI vendor pipeline`
`-> Gate C: delivered Image/HardwareBuffer/physical result`

Gate A/B records intent/provenance before vendor execution. It is not pre-HAL pixel access.

High-value route-candidate key names discovered include:

- `EnableIdealRAW`
- `RawCbSourceType`
- `EnableXCFAOptimization`
- `HALOutputBufferCombined`
- `EnableInsensorZoom`
- `EnableSnapshotOnlyInsensorZoom`
- `EnableMCXMasterCb`

Critical safety rule: v0.17 writes none of these merely from their names.

Observed clues that became hypotheses rather than conclusions:

- HONOR `binningFactor=4`
- AEC crop close to the 4080x3072 domain
- ISP/active-array metadata in the 16320x12288 domain
- `isInSensorZoom=0` in earlier envelope runs
- 40-byte `sensorCustomMetaData`
- QTI multicamera sidecars
- QTI AF lens-position sidecar.

---

## 7. Parallel v0.18 focus probe — deliberately not in the trusted chain

Branch:

`integration/truthraw-suite-v0-18-camera5-physical-focus-probe`

Purpose: investigate direct physical focus control and bind requested focus -> Android result focus -> QTI lens position/PD state.

Why it is not the parent of v0.19/v0.20: the discovery that the apparent 200 MP raster might contain only a small populated prefix was more fundamental. The trusted content-analysis chain therefore returned to the v0.17 airlock baseline and left focus work isolated.

Current focus knowledge retained:

- Camera 5 exposes Android focus-distance controls/results;
- QTI metadata exposes a physical/vendor lens position;
- multicamera sidecars are present;
- future multi-focus must preserve each physical RAW as its own evidence object.

---

## 8. Screenshot/DNG symptom that changed priority

A gallery view of the auxiliary RAW/DNG showed a narrow horizontal strip of visible image data at the beginning of the raster and black over most of the declared frame.

This visually matched a previously suspicious `768`-row pattern:

`12288 / 16 = 768`.

Critical exact byte identity:

`16320 * 768 * 2 = 25,067,520 bytes`

`4080 * 3072 * 2 = 25,067,520 bytes`.

At this point the project stopped treating DNG/viewer behavior as the main suspect and built a source-file audit that could decide whether the zero area already existed in Plane[0].

---

## 9. v0.19: Stage 3.6 Full Raster Write Audit

Branch:

`integration/truthraw-suite-v0-19-full-raster-write-audit`

Workflow:

`.github/workflows/android-truthraw-suite-v0-19-full-raster-write-audit.yml`

Patch:

`tools/patch_fotograaf_v019_full_raster_write_audit.py`

Core analyzer:

`suite_android/app/src/main/java/com/truthraw/adaptiveui/RawSensorRasterAudit.kt`

Safety order:

`seal original Plane[0]`
`-> observe live envelope`
`-> close Image/session/reader/camera`
`-> reopen only the sealed file read-only`
`-> audit all rows/bands`

Stage 3.6 records:

- second full-file SHA-256 and source-seal identity;
- every one of 12,288 rows;
- 16 equal bands of 768 rows;
- zero/non-zero counts;
- min/max;
- exact integer sum/sum-of-squares;
- row and band hashes;
- exact repetition offsets.

Decisive v0.19 capture classification:

`ONLY_FIRST_768_ROWS_NONZERO__EXACT_12P5MP_BYTE_PAYLOAD_SIGNATURE`

Observed source facts:

- populated rows exactly `0..767`;
- rows `768..12287` all zero;
- populated bytes exactly `25,067,520`;
- all-zero bands 1..15;
- second-read SHA equals sealed source SHA.

This proved the gallery/DNG black area was already present in the primary source buffer. DngCreator/Google Photos did not create the missing 15/16.

Repository v0.20 contract document initially recorded the v0.19 finding:

`docs/HONOR_MAGIC8_PRO_TELE_PAYLOAD_GEOMETRY_V020_2026-09-17.md`

---

## 10. v0.20: Stage 3.7 Payload Geometry Decoder

Branch:

`integration/truthraw-suite-v0-20-payload-geometry-decoder`

Workflow:

`.github/workflows/android-truthraw-suite-v0-20-payload-geometry-decoder.yml`

Patch:

`tools/patch_fotograaf_v020_payload_geometry_decoder.py`

Core decoder:

`suite_android/app/src/main/java/com/truthraw/adaptiveui/RawPayloadGeometryDecoder.kt`

v0.20 is downstream-only. It reconstructs v0.19 first and changes no trusted acquisition behavior.

Stage 3.7 procedure:

1. take Stage-3.6 populated-prefix byte count;
2. read physical Camera-5 standard RAW_SENSOR output sizes at runtime;
3. compare exact U16 byte counts;
4. accept a geometry candidate only if exactly one advertised standard RAW size matches;
5. copy `[0,payloadBytes)` byte-for-byte to a derived `.rawpayload`;
6. hash it and compare with Stage-3.6 first-band hash;
7. analyze U16 codes, 2x2 phases and neighbor correlations;
8. create an appearance-only grayscale diagnostic PNG.

No geometry candidate replaces the sealed source.

---

## 11. Current v0.20 physical result — the endpoint reached in this handoff

Uploaded evidence:

`TRUTHRAW_1789654132526_CAM5_200MP_EVIDENCE_v020.json`

Uploaded exact-prefix view:

`TRUTHRAW_1789654132526_CAM5_PAYLOAD_STANDARD_RAW_BYTE_MATCH_v020.rawpayload`

Uploaded diagnostic image:

`TRUTHRAW_1789654132526_CAM5_PAYLOAD_GEOMETRY_DIAGNOSTIC_v020.png`

Primary source named by evidence:

`TRUTHRAW_1789654132526_CAM5_200MP_16320x12288_v020.rawsensor`

Primary source facts:

- source SHA-256: `39da26192d1278af797b0c304d6ad29862735b886bdcf768b6239bc74f6a3636`
- bytes: `401,080,320`
- physical result: camera 5
- Image/result timestamp identity: pass
- rowStride: `32640`
- pixelStride: `2`
- returned pixel mode: `0`
- requested/output maximum-resolution route: true
- rawBinningFactorUsed: true.

Stage 3.7 selected exactly one runtime-advertised standard RAW geometry:

`4080x3072`

because:

`4080 * 3072 * 2 = 25,067,520`.

Derived exact-prefix object:

- bytes: `25,067,520`
- SHA-256: `d41d48d644c4aa8804825a529a0ea651352979a7023e4948e39913b71ce16e3e`
- source range: `[0, 25,067,520)`
- transform: none
- hash equals Stage-3.6 first-band hash: true.

Candidate code range in this capture:

- min `64`
- max `1023`.

2x2 phase means:

- `(0,0) = 131.978941`
- `(0,1) = 174.729906`
- `(1,0) = 174.945847`
- `(1,1) = 127.204966`.

Sampled pair diagnostics:

- horizontal dx=1: `r=0.845771`, mean abs diff `45.7344`
- horizontal dx=2: `r=0.984627`, mean abs diff `7.6461`
- vertical dy=1: `r=0.879186`, mean abs diff `43.6312`
- vertical dy=2: `r=0.981845`, mean abs diff `7.3667`.

Interpretation:

- same-CFA-phase distance-2 neighbors are much more correlated than immediate cross-colour neighbors;
- the two cross phases have almost identical means;
- the diagnostic PNG is a coherent full-frame street scene when indexed as 4080x3072.

Current bounded research summary:

`APP_VISIBLE_4080x3072_BAYER_LIKE_RAW_PAYLOAD_EMBEDDED_IN_16320x12288_HAL_ENVELOPE`

This remains an app-visible topology interpretation, not native-ADC geometry proof.

---

## 12. Current vendor/HAL evidence around the v0.20 payload

Request/session surface exposes, among other controls:

- `EnableIdealRAW`
- `RawCbSourceType`
- `EnableXCFAOptimization`
- `HALOutputBufferCombined`
- `EnableInsensorZoom`
- `EnableSnapshotOnlyInsensorZoom`
- `EnableMCXMasterCb`.

In the untouched v0.20 control:

- Gate A/B changed no vendor keys;
- `EnableXCFAOptimization` builder state was visible as zero;
- `EnableIdealRAW` available, builder value null;
- `RawCbSourceType` available, builder value null.

Same-capture post-HAL clues include:

- HONOR `binningFactor=4`;
- HONOR `AECRealCropWindow` beginning `[11,8,4058,3055,...]`;
- HONOR `allISPCropWindow` beginning `[0,0,16320,12288,...]`;
- 40-byte `sensorCustomMetaData`;
- QTI `ActiveCameraInfo` sidecar;
- QTI AF `lenspos=2856` in the v0.20 capture.

Those are retained as route clues, not promoted semantics.

---

## 13. Current files to read first

Current Camera-5 reading order:

1. `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md`
2. `docs/HONOR_MAGIC8_PRO_TELE_200MP_FULL_RAW_V014_2026-09-17.md`
3. `docs/HONOR_CAMERA5_TWO_DOOR_AIRLOCK_V017_2026-09-17.md`
4. `docs/HONOR_MAGIC8_PRO_TELE_PAYLOAD_GEOMETRY_V020_2026-09-17.md`
5. this file: `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md`
6. `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json`

Implementation/build chain:

- v0.14 patch: `tools/patch_fotograaf_v014_seal_raw_before_result_mode_gate.py`
- v0.16 post-HAL probe: `Camera2EnvelopeProbe.kt` + `hardware_buffer_envelope_probe.cpp`
- v0.17 request gate: `Camera2PreHalGate.kt`
- v0.19 source audit: `RawSensorRasterAudit.kt`
- v0.20 payload decoder: `RawPayloadGeometryDecoder.kt`

Workflows:

- `.github/workflows/android-truthraw-suite-v0-14-seal-200mp-raw.yml`
- `.github/workflows/android-truthraw-suite-v0-16-hal-buffer-envelope.yml`
- `.github/workflows/android-truthraw-suite-v0-17-camera5-route-fingerprint.yml`
- `.github/workflows/android-truthraw-suite-v0-19-full-raster-write-audit.yml`
- `.github/workflows/android-truthraw-suite-v0-20-payload-geometry-decoder.yml`

Branches:

- `integration/honor-magic8pro-tele-200mp-v014-project-sync-2026-09-17`
- `integration/truthraw-suite-v0-15-direct-camera5-full-raster` — rejected direct-open lineage
- `integration/truthraw-suite-v0-16-hal-buffer-envelope`
- `integration/truthraw-suite-v0-17-camera5-route-fingerprint`
- `integration/truthraw-suite-v0-18-camera5-physical-focus-probe` — isolated parallel focus research
- `integration/truthraw-suite-v0-19-full-raster-write-audit`
- `integration/truthraw-suite-v0-20-payload-geometry-decoder`

---

## 14. What changed scientifically from v0.14 to v0.20

v0.14 statement:

> a physical-Camera-5 app-visible `16320x12288` RAW_SENSOR frame/envelope can be delivered, timestamp-bound and source-sealed.

v0.20 refinement:

> in the tested route, the full declared 401 MB envelope is not fully populated. Only the first 25,067,520 bytes carry non-zero source codes; those bytes exactly match one advertised standard `4080x3072` RAW_SENSOR byte count, form a coherent full-frame image under that geometry, and exhibit Bayer-like 2x2 spatial structure.

Therefore:

- **v0.14 remains valid acquisition provenance**;
- **v0.19/v0.20 materially narrow the content interpretation of that envelope**;
- file size/raster declaration can no longer be used as evidence that all 200,540,160 positions contain meaningful source measurements;
- the next route investigation belongs upstream at session/request control, not in downstream 401 MB repair.

---

## 15. Next experiment contract

Use v0.20 as the untouched control.

First single-variable route experiment:

`EnableIdealRAW`

Required rules:

1. separate branch/build;
2. discover the actual key object from Camera-5 characteristics;
3. verify runtime type before setting;
4. do not assume name semantics;
5. set no second unknown vendor key in the same first experiment;
6. preserve logical0 -> physical5 -> exact 16320x12288 MAX output topology;
7. preserve source-first sealing;
8. preserve Gate A/B and Gate C evidence;
9. repeat Stage 3.6 and 3.7 unchanged;
10. compare against the exact v0.20 baseline.

Primary comparison outputs:

- populated source byte count;
- populated row/band topology;
- selected geometry candidate;
- source/prefix hashes;
- HardwareBuffer dimensions/stride/usage;
- returned SENSOR_PIXEL_MODE;
- rawBinningFactorUsed;
- HONOR binning/crop sidecars;
- all visible vendor result fingerprints.

Only after that first differential capture should `RawCbSourceType`, XCFA-related controls, in-sensor zoom or other route switches be tested separately.

---

## 16. Scientific boundary carried forward

Do not infer from the current endpoint that:

- the physical sensor is only 12.5 MP;
- the physical sensor is truly delivering untouched 200 MP elsewhere;
- HONOR definitely performs one particular binning/remosaic mechanism;
- 64 is a calibrated black level;
- 1023 proves a 10-bit ADC;
- the vendor key `EnableIdealRAW` necessarily means untouched ADC RAW;
- a future all-nonzero 200 MP raster automatically proves 200 MP optical independence.

The correct continuation remains:

`capture-route proof`
`-> source topology/content proof`
`-> readout-domain precision/uncertainty`
`-> noise/PTC`
`-> shading`
`-> colour/illuminant calibration`
`-> SFR/MTF / optical support`
`-> Scientific Master admission`.
